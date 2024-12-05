#ifndef _PREDICTOR_H_
#define _PREDICTOR_H_

#include "tracer.h"
#include "utils.h"
#include <bits/stdc++.h>


// TAGE_SC_L_32KB
//         _______________________________
//        |  _______       _____ ______   |
//---PC-->| |__   __|/\   / ____|  ____|  |--TAGE pridection of PC Confidence
//        |    | |  /  \ | |  __| |__     |                  |
//        |    | | / /\ \| | |_ |  __|    |             _____V_____
//--GHR-->|    | |/ ____ \ |__| | |____   |            |           |
//        |    |_/_/    \_\_____|______|  |            | Corrector |
//        |_______________________________|            |___________|
//                                                           |
//         ______________________                      ______V_____
//        |                      |                    |            |
//---PC-->|    Loop Predictor    |------------------->| Prediction |
//        |______________________|                    |____________|
//

// Type definitions for clarity
#define INT8 char
#define UINT8 unsigned char
#define UINT16 unsigned short int
#define UINT128 __uint128_t

#define bitmask(a) ((1 << a) - 1)
#define lowbits(a, b) (a & bitmask(b))

// Switches for predictor configuration
#define LOOP_ON
#define CF_ON

#define MAGIC_NUMBER 19260817
#define LARGE_PRIME 1000000007

// Constants for predictor configuration
#define BASE_TABLE_ENTRY_NUM (1 << 13)
#define BASE_CTR_INIT 2
#define BASE_CTR_MAX 3

#define TAGE_TABLE_NUM 4
#define TAGE_TAG_WIDTH 9
#define TAGE_TABLE_INDEX_WIDTH 12
#define TAGE_CTR_INIT 0
#define TAGE_CTR_MAX 7
#define TAGE_CTR_STRONG 5
#define TAGE_CTR_WEAK 2
#define TAGE_U_MAX 3
#define TAGE_WEAK_CORRECT 4
const UINT32 TAGE_TABLE_HISTORY_WIDTH[TAGE_TABLE_NUM] = {
      5, 16, 37, 91}; // History widths for TAGE tables

#define USE_CF_INIT 8
#define USE_CF_THRESHOLD 7
#define USE_CF_MAX 15

#define LOOP_TABLE_ENTRY_NUM 512
#define LOOP_TABLE_INDEX_WIDTH 9
#define LOOP_TAG_WIDTH 14
#define LOOP_CONFIDENC_WIDTH 2
#define LOOP_COUNT_WIDTH 14
#define LOOP_AGE_WIDTH 8

#define CF_CTR_MAX 63
#define CF_CTR_STRONG 40
#define CF_CTR_WEAK 22
#define CF_TAG_WIDTH 7
#define CF_CTR_NUM 252

#define CLOCK_HIGH 1 << 18
#define CLOCK_MAX 1 << 19

// Structure for TAGE table entries
struct TageEntry {
  UINT8 pred;
  UINT16 tag;
  UINT8 u;
};

// Structure for loop predictor entries
struct LoopEntry {
  UINT16 tag; // Tag for loop entry
  UINT8 ctr;  // Counter for loop entry
  UINT8 age;  // Age of loop entry
  UINT16 pcr; // Previous count register
  UINT16 ncr; // Next count register
};

// Base predictor class
class BasePredictor {
private:
  UINT8  base_table[BASE_TABLE_ENTRY_NUM]; // Base table for predictions
  UINT32 base_table_entry_num;
  UINT32 base_table_index;
  UINT8 base_counter;
  bool high_conf;

public:
  BasePredictor();
  bool predict(UINT32 PC);
  void update(bool resolveDir);
  bool highConf();
};



// TAGE predictor class
class TAGE {
private:
  TageEntry *tag_table; // Tag table for TAGE
  UINT128 *ghr;         // Global history register
  UINT32 tag_table_entry_num;
  UINT32 tag_history_width;
  UINT32 tag;
  UINT32 index;

public:
  TAGE(UINT32 history_width, UINT128 *ghr);
  bool match(UINT32 PC);
  bool predict();
  bool isNewEntry();
  bool highConf();
  void updateHit(bool resolveDir);
  void updateMiss();
  void updateMissNewEntry(bool resolveDir);
  void updateU(bool resolveDir, bool predDir);
  void resetU(UINT8 mask);
  UINT16 getTag(UINT32 PC); // hash 2
  UINT32 getTagTableIndex(UINT32 PC); // hash 1 to get tag from table
  UINT8 getU();
};

// Loop predictor class
class LoopPredictor {
private:
  LoopEntry table[LOOP_TABLE_ENTRY_NUM]; // Loop predictor table
  
  UINT32 index;
  UINT16 tag;
  bool use_loop;
  bool pred;

public:
  LoopPredictor();
  void predict(UINT32 pc);
  void update(bool resolveDir, bool tage_pred);
  void resetEntry(UINT32 idx);
  bool useLoop();
  bool prediction();
};

// Corrector filter class
class CorrectorFilter {
private:
  UINT8 ctr[CF_CTR_NUM]; // Counter array
  UINT8 tag_table[CF_CTR_NUM]; // Tag array
  UINT32 index;
  UINT32 tag;

public:
  CorrectorFilter();
  bool predict(UINT32 pc, bool tage_result, bool highconf);
  void update(bool tage_result, bool resolveDir, bool highconf);
};

// Main predictor class
class PREDICTOR {
private:
  UINT128 ghr; // Global history register
  UINT32 clock;
  INT32 first_predictor;
  INT32 second_predictor;
  bool first_prediction;
  bool second_prediction;

  bool tage_prediction;
  bool cf_prediction;
  bool high_conf;
  bool pred_is_new_entry;

  UINT16 use_cf;

  std::vector<std::unique_ptr<TAGE>> tage_list; // List of TAGE predictors
  BasePredictor bp;                             // Base predictor
  LoopPredictor lp;                             // Loop predictor
  CorrectorFilter cf;                           // Corrector filter

public:
  PREDICTOR(void);
  bool GetPrediction(UINT32 PC);
  void UpdatePredictor(UINT32 PC, bool resolveDir, bool predDir,
                       UINT32 branchTarget);
  void TrackOtherInst(UINT32 PC, OpType opType, UINT32 branchTarget);
};

#endif
