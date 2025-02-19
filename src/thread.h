/*
  Clover is a UCI chess playing engine authored by Luca Metehau.
  <https://github.com/lucametehau/CloverEngine>

  Clover is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Clover is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#include "board.h"
#include "tt.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

/// search params

int seeVal[] = { 0, 93, 308, 346, 521, 994, 20000 };

int nmpR = 3;
int nmpDepthDiv = 4;
int nmpEvalDiv = 135;

int RazorCoef = 381;

int SNMPDepth = 10;
int SNMPCoef1 = 87;
int SNMPCoef2 = 13;

int seeCoefQuiet = 71;
int seeCoefNoisy = 10;
int seeDepthCoef = 15;

int probcutDepth = 10;
int probcutMargin = 100;
int probcutR = 3;

int fpMargin = 100;
int fpCoef = 103;

int histDiv = 5102;

int chCoef = -2000;
int fhCoef = -2000;

int seePruningQuietDepth = 8;
int seePruningNoisyDepth = 8;
int lmpDepth = 8;

int nodesSearchedDiv = 10000;

int lmrMargin = 10;
int lmrDiv = 25;
int lmrCapDiv = 15;

int lmpStart1 = 3, lmpMult1 = 1, lmpDiv1 = 2;
int lmpStart2 = 3, lmpMult2 = 1, lmpDiv2 = 1;

int tmScoreDiv = 111;
int tmBestMoveStep = 50;
int tmBestMoveMax = 1250;
int tmNodesSearchedMaxPercentage = 1570;

int quiesceFutilityCoef = 200;

int aspirationWindow = 10;

int pawnAttackedCoef = 36;
int pawnPushBonus = 9520;
int kingAttackBonus = 3579;

const int TERMINATED_BY_USER = 1;
const int TERMINATED_BY_LIMITS = 2;
const int TERMINATED_SEARCH = 3; /// 1 | 2

struct Search {
    Search() : threads(nullptr), params(nullptr) {
        threadCount = flag = checkCount = 0;
        principalSearcher = terminateSMP = SMPThreadExit = false;
        lazyFlag = 0;

        for (int i = 0; i < 64; i++) { /// depth
            for (int j = 0; j < 64; j++) { /// moves played 
                lmrRed[i][j] = 1.0 * lmrMargin / 10 + log(i) * log(j) / (1.0 * lmrDiv / 10);
                lmrRedNoisy[i][j] = lmrRed[i][j] / (1.0 * lmrCapDiv / 10);
            }
        }
        for (int i = 1; i < 20; i++) {
            lmrCnt[0][i] = (lmpStart1 + lmpMult1 * i * i) / lmpDiv1;
            lmrCnt[1][i] = (lmpStart2 + lmpMult2 * i * i) / lmpDiv2;
        }
    }

    ~Search() {
        //std::cout << "its not even called\n";
        releaseThreads();
    }

    Search(const Search&) = delete;
    Search& operator = (const Search&) = delete;

    void initSearch();
    void clearForSearch();
    void clearKillers();
    void clearHistory();
    void clearStack();
    void clearBoard();
    void setThreadCount(int nrThreads);
    void startPrincipalSearch(Info* info);
    void stopPrincipalSearch();
    void isReady();

    void _setFen(std::string fen);
    void _makeMove(uint16_t move);

    std::pair <int, uint16_t> startSearch(Info* info);
    int quiesce(int alpha, int beta, StackEntry* stack, bool useTT = true); /// for quiet position check (tuning)
    int search(int alpha, int beta, int depth, bool cutNode, StackEntry* stack, uint16_t excluded = NULLMOVE);
    int rootSearch(int alpha, int beta, int depth, int multipv, StackEntry* stack);

    void setTime(Info* tInfo) { info = tInfo; }

    void startWorkerThreads(Info* info);
    void flagWorkersStop();
    void stopWorkerThreads();
    void lazySMPSearcher();
    void releaseThreads();
    void killMainThread();

    void printPv();
    void updatePv(int ply, int move);

    bool checkForStop();

    uint64_t nodesSearched[2][64][64];
    uint16_t pvTable[DEPTH + 5][2 * DEPTH + 5];
    int pvTableLen[DEPTH + 5];
    uint16_t cmTable[2][13][64];
    int hist[2][64][64];
    TablePieceTo continuationHistory[13][64];
    int capHist[13][64][7];
    int lmrCnt[2][20];
    int lmrRed[64][64], lmrRedNoisy[64][64];
    int bestMoves[256], scores[256];
    MeanValue values[10];

    volatile int flag;

    uint64_t tbHits;
    uint64_t t0;
    Info* info;
    int checkCount;

    int64_t cnt, cnt2;
    int bestMoveCnt;

    bool lastNullMove;

    int threadCount;
    int tDepth, selDepth;

    int rootEval, rootDepth;

    std::unique_ptr <std::thread> principalThread;
    std::mutex readyMutex;

    uint64_t nodes, qsNodes;
    bool principalSearcher;
    Board board;

    //tt::HashTable* threadTT;

    std::unique_ptr <std::thread[]> threads;
    std::unique_ptr <Search[]> params;
    std::condition_variable lazyCV;
    volatile bool lazyFlag;
    volatile bool SMPThreadExit;

    bool isLazySMP() {
        return lazyFlag;
    }

    void resetLazySMP() {
        lazyFlag = 0;
    }

    bool terminateSMP;

};