#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <vector>
#include <algorithm>

typedef int64_t int64;

// Globals
#define MAX_IDS 2000000
int64 IDs[MAX_IDS];
int HR[MAX_IDS * 53];
int numIDs = 1;
int maxHR = 0;
int64 maxID = 0;
int numcards = 0; 

// Cactus Kev Primes
const int primes[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41 };

int eval_5hand_fast(int c1, int c2, int c3, int c4, int c5) {
    int q = (c1 | c2 | c3 | c4 | c5) >> 16;
    short s;
    // Removed incorrect flush short-circuit
    
    int r[5] = { (c1>>8)&0xF, (c2>>8)&0xF, (c3>>8)&0xF, (c4>>8)&0xF, (c5>>8)&0xF };
    std::sort(r, r+5);
    
    bool straight = true;
    for(int i=0; i<4; i++) if(r[i+1] != r[i]+1) straight = false;
    if(!straight && r[0]==0 && r[1]==1 && r[2]==2 && r[3]==3 && r[4]==12) straight = true; // Wheel
    
    bool flush = (c1 & c2 & c3 & c4 & c5 & 0xF000);
    
    if(flush && straight) {
        if (r[4] == 12 && r[0] == 8) return 1; // Royal Flush (A, K, Q, J, T -> 12, 11, 10, 9, 8)
        return 10; // Other Straight Flush
    } 
    
    int counts[13] = {0};
    for(int x : r) counts[x]++;
    
    int four=0, three=0, pair=0, pair2=0;
    for(int i=0; i<13; i++) {
        if(counts[i]==4) four=i+1;
        if(counts[i]==3) three=i+1;
        if(counts[i]==2) { if(pair) pair2=i+1; else pair=i+1; } 
    }
    
    if(four) return 11; 
    if(three && pair) return 167; 
    if(flush) return 323; 
    if(straight) return 1600; 
    if(three) return 1610; 
    if(pair && pair2) return 2468; 
    if(pair) return 3326; 
    return 6186; 
}

int eval_7hand(int* wk) {
    int best = 9999;
    int sub[5];
    
    for(int i=0; i<7; i++) {
        for(int j=i+1; j<7; j++) {
            int idx=0;
            for(int k=0; k<7; k++) {
                if(k!=i && k!=j) {
                    if(idx<5) sub[idx++] = wk[k];
                }
            }
            int score = eval_5hand_fast(sub[0], sub[1], sub[2], sub[3], sub[4]);
            if(score < best) best = score;
        }
    }
    return best;
}

int DoEval(int64 IDin) {
    int result = 0;
    int cardnum;
    int wkcard;
    int rank;
    int suit;
    int mainsuit = 20; 
    int suititerator = 1;
    int holdrank;
    int wk[8] = {0}; 
    int holdcards[8] = {0};
    int numevalcards = 0;

    if (IDin) {
        for (cardnum = 0; cardnum < 7; cardnum++) {
            holdcards[cardnum] = (int)((IDin >> (8 * cardnum)) & 0xff);
            if (holdcards[cardnum] == 0) break;
            numevalcards++;
            if ((suit = holdcards[cardnum] & 0xf)) {
                mainsuit = suit;
            }
        }

        for (cardnum = 0; cardnum < numevalcards; cardnum++) {
            wkcard = holdcards[cardnum];
            rank = (wkcard >> 4) - 1; 
            suit = wkcard & 0xf;
            if (suit == 0) {
                suit = suititerator++;
                if (suititerator == 5) suititerator = 1;
                if (suit == mainsuit) {
                    suit = suititerator++;
                    if (suititerator == 5) suititerator = 1;
                }
            }
            wk[cardnum] = primes[rank] | (rank << 8) | (1 << (suit + 11)) | (1 << (16 + rank));
        }

        // To harmonize with NaiveEvaluator, we extract the best 5-card hand features
        // and use the unified encode_score logic.
        // Simplified mapping for the generator to match NaiveEvaluator's new logic:
        
        auto get_score = [&](int c1, int c2, int c3, int c4, int c5) -> int32_t {
            int r[5] = { (c1>>8)&0xF, (c2>>8)&0xF, (c3>>8)&0xF, (c4>>8)&0xF, (c5>>8)&0xF };
            std::sort(r, r+5);
            bool flush = (c1 & c2 & c3 & c4 & c5 & 0xF000);
            bool straight = true;
            for(int i=0; i<4; i++) if(r[i+1] != r[i]+1) straight = false;
            if(!straight && r[0]==0 && r[1]==1 && r[2]==2 && r[3]==3 && r[4]==12) {
                straight = true; // Wheel
                // For scoring, wheel is 5-high
                r[0]=3; r[1]=2; r[2]=1; r[3]=0; r[4]=-1; // 5, 4, 3, 2, A(1)
                std::sort(r, r+5, std::greater<int>());
            } else {
                std::sort(r, r+5, std::greater<int>());
            }

            int counts[13] = {0};
            for(int i=0; i<5; i++) counts[r[i]]++;

            // Scoring logic matching NaiveEvaluator::encode_score
            // Builds ranks as base-15 number iteratively (matching hand_types.h)
            auto encode = [](int type, const std::vector<int>& ranks) {
                int32_t relative = 0;
                for (size_t i = 0; i < ranks.size() && i < 5; ++i) {
                    relative = relative * 15 + ranks[i];
                }
                return type * 1000000 + relative;
            };

            if (flush && straight) {
                if (r[0] == 12 && r[4] == 8) return encode(9, {14, 13, 12, 11, 10}); // Royal
                return encode(8, {r[0]+2}); // SF
            }

            int four= -1, three= -1, pair1= -1, pair2= -1;
            std::vector<int> kickers;
            for(int i=12; i>=0; i--) {
                if(counts[i]==4) four = i;
                else if(counts[i]==3) three = i;
                else if(counts[i]==2) { if(pair1 == -1) pair1 = i; else pair2 = i; }
                else if(counts[i]==1) kickers.push_back(i);
            }

            if(four != -1) return encode(7, {four+2, kickers[0]+2});
            if(three != -1 && pair1 != -1) return encode(6, {three+2, pair1+2});
            if(flush) return encode(5, {r[0]+2, r[1]+2, r[2]+2, r[3]+2, r[4]+2});
            if(straight) return encode(4, {r[0]+2});
            if(three != -1) return encode(3, {three+2, kickers[0]+2, kickers[1]+2});
            if(pair1 != -1 && pair2 != -1) return encode(2, {pair1+2, pair2+2, kickers[0]+2});
            if(pair1 != -1) return encode(1, {pair1+2, kickers[0]+2, kickers[1]+2, kickers[2]+2});
            return encode(0, {r[0]+2, r[1]+2, r[2]+2, r[3]+2, r[4]+2});
        };

        int32_t best = 0;
        int sub[5];
        if (numevalcards == 5) {
            best = get_score(wk[0], wk[1], wk[2], wk[3], wk[4]);
        } else if (numevalcards == 6) {
            for(int i=0; i<6; i++) {
                int idx=0;
                for(int k=0; k<6; k++) if(k!=i) sub[idx++] = wk[k];
                int32_t s = get_score(sub[0], sub[1], sub[2], sub[3], sub[4]);
                if(s > best) best = s;
            }
        } else if (numevalcards == 7) {
            for(int i=0; i<7; i++) {
                for(int j=i+1; j<7; j++) {
                    int idx=0;
                    for(int k=0; k<7; k++) if(k!=i && k!=j) sub[idx++] = wk[k];
                    int32_t s = get_score(sub[0], sub[1], sub[2], sub[3], sub[4]);
                    if(s > best) best = s;
                }
            }
        }
        result = best;
    }
    return result;
}

int64 MakeID(int64 IDin, int newcard) {
    int64 ID = 0;
    int suitcount[5] = {0};
    int rankcount[14] = {0};
    int wk[8] = {0};
    int getout = 0;

    for (int cardnum = 0; cardnum < 6; cardnum++) {
        wk[cardnum + 1] = (int)((IDin >> (8 * cardnum)) & 0xff);
    }

    newcard--; 
    wk[0] = (((newcard >> 2) + 1) << 4) + (newcard & 3) + 1;

    for (numcards = 0; wk[numcards]; numcards++) {
        suitcount[wk[numcards] & 0xf]++;
        rankcount[(wk[numcards] >> 4) & 0xf]++;
        if (numcards && wk[0] == wk[numcards]) getout = 1;
    }
    if (getout) return 0;

    if (numcards > 4) {
        for (int rank = 1; rank < 14; rank++) if (rankcount[rank] > 4) return 0;
    }

    int needsuited = numcards - 2;
    if (needsuited > 1) {
        for (int cardnum = 0; cardnum < numcards; cardnum++) {
            if (suitcount[wk[cardnum] & 0xf] < needsuited) wk[cardnum] &= 0xf0;
        }
    }

#define SWAP(I,J) {if (wk[I] < wk[J]) {wk[I]^=wk[J]; wk[J]^=wk[I]; wk[I]^=wk[J];}}
    SWAP(0, 4); SWAP(1, 5); SWAP(2, 6); SWAP(0, 2); SWAP(1, 3);
    SWAP(4, 6); SWAP(2, 4); SWAP(3, 5); SWAP(0, 1); SWAP(2, 3);
    SWAP(4, 5); SWAP(1, 4); SWAP(3, 6); SWAP(1, 2); SWAP(3, 4);
    SWAP(5, 6);

    ID = (int64)wk[0] + ((int64)wk[1] << 8) + ((int64)wk[2] << 16) +
         ((int64)wk[3] << 24) + ((int64)wk[4] << 32) + ((int64)wk[5] << 40) +
         ((int64)wk[6] << 48);
    return ID;
}

int SaveID(int64 ID) {
    if (ID == 0) return 0;
    if (ID >= maxID) {
        if (ID > maxID) {
            if (numIDs >= MAX_IDS) {
                printf("\nERROR: ID overflow! numIDs=%d\n", numIDs);
                exit(1);
            }
            IDs[numIDs++] = ID;
            maxID = ID;
        }
        return numIDs - 1;
    }
    int low = 0, high = numIDs - 1;
    while (high - low > 1) {
        int holdtest = (high + low + 1) / 2;
        int64 testval = IDs[holdtest] - ID;
        if (testval > 0) high = holdtest;
        else if (testval < 0) low = holdtest;
        else return holdtest;
    }
    
    if (numIDs >= MAX_IDS) {
        printf("\nERROR: ID overflow during insert! numIDs=%d\n", numIDs);
        exit(1);
    }
    memmove(&IDs[high + 1], &IDs[high], (numIDs - high) * sizeof(IDs[0]));
    IDs[high] = ID;
    numIDs++;
    return high;
}

int main() {
    printf("Starting 2+2 Table Generation...\n");
    clock_t timer = clock();

    memset(IDs, 0, sizeof(IDs));
    memset(HR, 0, sizeof(HR));

    int IDnum, card, holdid;
    int64 ID;

    printf("Getting Card IDs!\n");
    for (IDnum = 0; IDs[IDnum] || IDnum == 0; IDnum++) {
        for (card = 1; card < 53; card++) {
            ID = MakeID(IDs[IDnum], card);
            if (numcards < 7) holdid = SaveID(ID);
        }
        if (IDnum % 1000 == 0) printf("\rID - %d", IDnum);
    }

    printf("\nSetting HandRanks!\n");
    for (IDnum = 0; IDs[IDnum] || IDnum == 0; IDnum++) {
        for (card = 1; card < 53; card++) {
            ID = MakeID(IDs[IDnum], card);
            int IDslot;
            if (numcards < 7) {
                IDslot = SaveID(ID) * 53 + 53;
            } else {
                IDslot = DoEval(ID);
            }
            maxHR = IDnum * 53 + card + 53;
            HR[maxHR] = IDslot;
        }
        if (numcards == 6 || numcards == 7) {
            HR[IDnum * 53 + 53] = DoEval(IDs[IDnum]);
        }
        if (IDnum % 1000 == 0) printf("\rID - %d", IDnum);
    }

    printf("\nNumber IDs = %d\nmaxHR = %d\n", numIDs, maxHR);
    timer = clock() - timer;
    printf("Generation time: %.2f seconds\n", (float)timer/CLOCKS_PER_SEC);

    FILE * fout = fopen("HandRanks.dat", "wb");
    if (!fout) {
        printf("Problem creating the Output File!\n");
        return 1;
    }
    // Only write the actually used portion of the table (up to maxHR + 1)
    size_t bytes_to_write = (maxHR + 1) * sizeof(int);
    fwrite(HR, bytes_to_write, 1, fout);
    fclose(fout);

    printf("HandRanks.dat generated (%zu MB).\n", bytes_to_write / (1024*1024));
    return 0;
}
