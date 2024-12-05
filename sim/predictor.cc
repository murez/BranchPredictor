#include "predictor.h"
#include "utils.h"
#include <algorithm>
#include <cstring>

BasePredictor::BasePredictor() {
  // Set all entries to the initial counter value
  std::memset(base_table, BASE_CTR_INIT, sizeof(base_table));
}

bool BasePredictor::predict(UINT32 PC) {
  // Calculate the index for the base predictor table
  base_table_index = PC % BASE_TABLE_ENTRY_NUM;

  // Retrieve the counter value from the table
  base_counter = base_table[base_table_index];

  // Determine if the prediction is high confidence
  high_conf = (base_counter == 0 || base_counter == BASE_CTR_MAX);

  // Return the prediction based on the counter value
  return base_counter > BASE_CTR_MAX / 2;
}

void BasePredictor::update(bool resolveDir) {
  // Update the counter value based on the actual outcome (resolveDir)
  base_table[base_table_index] = (resolveDir == TAKEN)
                                     ? SatIncrement(base_counter, BASE_CTR_MAX)
                                     : SatDecrement(base_counter);
}

bool BasePredictor::highConf() {
  // Return the high confidence status
  return high_conf;
}

TAGE::TAGE(UINT32 history_width, UINT128 *ghr) {
  this->ghr = ghr;
  tag_history_width = history_width;
  tag_table_entry_num = 1 << TAGE_TABLE_INDEX_WIDTH;
  tag_table = new TageEntry[tag_table_entry_num];

  // Initialize the TAGE table entries
  for (UINT32 i = 0; i < tag_table_entry_num; i++) {
    tag_table[i].tag = 0;
    tag_table[i].u = 0;
    tag_table[i].pred = TAGE_CTR_INIT;
  }
}

UINT16 TAGE::getTag(UINT32 PC) {
  // Calculate the tag using the global history register and the PC
  UINT128 temp_ghr = *ghr;
  temp_ghr = lowbits(temp_ghr, TAGE_TAG_WIDTH);
  return lowbits((temp_ghr + PC * LARGE_PRIME), TAGE_TAG_WIDTH);
}

UINT32 TAGE::getTagTableIndex(UINT32 PC) {
  // Calculate the index for the tag table using the PC and global history
  // register
  UINT128 temp_ghr = *ghr;
  int history_width = tag_history_width;
  UINT32 temp_pc = lowbits(PC, TAGE_TABLE_INDEX_WIDTH);

  // Fold the global history register into the index
  while (history_width > 0) {
    int block_width = std::min(history_width, TAGE_TABLE_INDEX_WIDTH);
    temp_pc ^= lowbits(temp_ghr, block_width);
    temp_ghr = temp_ghr >> block_width;
    history_width -= block_width;
  }

  return lowbits(temp_pc, TAGE_TABLE_INDEX_WIDTH);
}

bool TAGE::match(UINT32 PC) {
  // Check if the tag matches the entry in the tag table
  tag = getTag(PC);
  index = getTagTableIndex(PC);
  return tag_table[index].tag == tag;
}

bool TAGE::predict() {
  // Predict the outcome based on the counter value
  return tag_table[index].pred > TAGE_CTR_MAX / 2;
}

bool TAGE::highConf() {
  // Determine if the prediction is high confidence
  return tag_table[index].pred <= TAGE_CTR_WEAK ||
         tag_table[index].pred >= TAGE_CTR_STRONG;
}

void TAGE::updateHit(bool resolveDir) {
  // Update the counter based on the actual outcome
  tag_table[index].pred =
      (resolveDir == TAKEN) ? SatIncrement(tag_table[index].pred, TAGE_CTR_MAX)
                            : SatDecrement(tag_table[index].pred);
}

void TAGE::updateMiss() {
  // Decrement the usefulness counter on a miss
  tag_table[index].u = SatDecrement(tag_table[index].u);
}

void TAGE::updateMissNewEntry(bool resolveDir) {
  // Allocate a new entry on a miss
  tag_table[index].tag = tag;
  tag_table[index].u = 0;
  tag_table[index].pred =
      resolveDir ? TAGE_WEAK_CORRECT : TAGE_WEAK_CORRECT - 1;
}

void TAGE::updateU(bool resolveDir, bool predDir) {
  // Update the usefulness counter based on the prediction accuracy
  UINT8 u = tag_table[index].u;
  tag_table[index].u =
      (resolveDir == predDir) ? SatIncrement(u, TAGE_U_MAX) : SatDecrement(u);
}

void TAGE::resetU(UINT8 mask) {
  // Periodically reset the usefulness counters
  for (UINT32 i = 0; i < tag_table_entry_num; i++) {
    tag_table[i].u = tag_table[i].u & mask;
  }
}

UINT8 TAGE::getU() {
  // Get the usefulness counter value
  return tag_table[index].u;
}

// LoopPredictor
LoopPredictor::LoopPredictor() {
  for (UINT32 i = 0; i < LOOP_TABLE_ENTRY_NUM; i++) {
    resetEntry(i);
  }
  use_loop = false;
  pred = NOT_TAKEN;
  index = 0;
  tag = 0;
}
void LoopPredictor::predict(UINT32 pc) {

  use_loop = false;
  pred = NOT_TAKEN;

  index = lowbits(pc, LOOP_TABLE_INDEX_WIDTH);
  tag = lowbits((pc >> LOOP_TABLE_INDEX_WIDTH), LOOP_TAG_WIDTH);

  // Check if the tag matches
  if (tag != table[index].tag) {
    return;
  }

  // Determine loop prediction based on iteration counts
  pred = (table[index].pcr > table[index].ncr);

  // Set use_loop flag based on confidence counter
  use_loop = (table[index].ctr == bitmask(LOOP_CONFIDENC_WIDTH));
}

void LoopPredictor::resetEntry(UINT32 idx) {
  table[idx] = {0, 0, 0, 0, 0};
}

void LoopPredictor::update(bool resolveDir, bool tage_pred) {


  // If the tag does not match
  if (tag != table[index].tag) {
    if (table[index].age == 0) {
      // Allocate new entry if age is 0
      table[index].pcr = bitmask(LOOP_COUNT_WIDTH);
      table[index].ncr = 0;
      table[index].tag = tag;
      table[index].ctr = 0;
      table[index].age = bitmask(LOOP_AGE_WIDTH);
    } else {
      table[index].age = SatDecrement(table[index].age);
    }
    return;
  }

  // If the tag matches
  table[index].ncr++;

  // If the prediction is incorrect
  if (pred != resolveDir) {
    if (table[index].age == bitmask(LOOP_AGE_WIDTH) && table[index].ctr <= 1) {
      // New allocated entry
      table[index].pcr = table[index].ncr;
      table[index].ncr = 0;
    } else {
      // Reset entry cause previous prediction was incorrect
      resetEntry(index);
    }
    return;
  }

  // If the prediction is correct
  if (resolveDir == NOT_TAKEN) {
    table[index].ncr = 0;
    if (tage_pred != resolveDir) {
      table[index].ctr =
          SatIncrement(table[index].ctr, bitmask(LOOP_CONFIDENC_WIDTH));
      table[index].age =
          SatIncrement(table[index].age, bitmask(LOOP_AGE_WIDTH));
    }
  }
}

bool LoopPredictor::useLoop() { return use_loop; }

bool LoopPredictor::prediction() { return pred; }

// CorrectorFilter

/*
┌──────────────────────────┐
│   Program Counter (PC)   │
└─────────────┬────────────┘
              │
              ▼
   Hash Function (Index + Tag)
       ┌─────────────┐
       │ index, tag  │
       └─────────────┘
              │
              ▼
 ┌───────────────────────────┐
 │     Corrector Filter      │
 │   ctr[]    |  tag_table[] │
 └────────────┬──────────────┘
              │
    ┌─────────┴─────────┐
    ▼                   ▼
 (Mismatched Tag)   (Matched Tag)
  │                      │
  ▼                      ▼
TAGE Result       Check `ctr[index]`
 (Default)             ┌────────────┐
                       │ Strong or  │
                       │ Weak State │
                       └────┬───────┘
                            │
               ┌────────────▼────────────┐
               │ Corrector Filter Result │
               └─────────────────────────┘
*/

CorrectorFilter::CorrectorFilter() {
  for (UINT32 i = 0; i < CF_CTR_NUM; i++) {
    ctr[i] = CF_CTR_MAX / 2; // Initialize to mid-point (strong neutral state)
    tag_table[i] = 0;
  }
}

bool CorrectorFilter::predict(UINT32 pc, bool tage_result, bool highconf) {
  // If the prediction is high confidence, return the TAGE result
  if (highconf)
    return tage_result;

  // Calculate the index and tag for the corrector filter
  index = (pc * MAGIC_NUMBER + (int)tage_result) % CF_CTR_NUM;
  tag = lowbits((pc >> 6), CF_TAG_WIDTH);

  // If the tag does not match, return the TAGE result
  if (tag_table[index] != tag) {
    return tage_result;
  }

  // If the tag matches and the counter is strong enough, return the corrector
  // filter result
  if (ctr[index] >= CF_CTR_STRONG || ctr[index] <= CF_CTR_WEAK) {
    return ctr[index] >= CF_CTR_MAX / 2;
  }

  // Default to returning the TAGE result
  return tage_result;
}

void CorrectorFilter::update(bool tage_result, bool resolveDir, bool highconf) {
  // If the prediction is high confidence, do not update the corrector filter
  if (highconf)
    return;

  // If the tag does not match and the TAGE result is correct, do not update
  if (tag_table[index] != tag && tage_result == resolveDir) {
    return;
  }

  // If the tag matches, update the counter based on the resolve direction
  if (tag_table[index] == tag) {
    ctr[index] = resolveDir ? SatIncrement(ctr[index], CF_CTR_MAX)
                            : SatDecrement(ctr[index]);
    return;
  }

  // If the counter is weak or the corrector filter result matches the TAGE
  // result, update the tag and reset the counter
  if ((ctr[index] >= 29 && ctr[index] <= 33) ||
      ((ctr[index] >= CF_CTR_MAX / 2) == tage_result)) {
    tag_table[index] = tag;
    ctr[index] = resolveDir ? CF_CTR_MAX / 2 : CF_CTR_MAX / 2 - 1;
    return;
  }

  // Otherwise, update the counter with a lower probability
  ctr[index] = resolveDir ? SatIncrement(ctr[index], CF_CTR_MAX)
                          : SatDecrement(ctr[index]);
}

// PREDICTOR

PREDICTOR::PREDICTOR(void) {
  srand(MAGIC_NUMBER);
  ghr = 0;
  clock = 0;
  use_cf = USE_CF_INIT;

  for (UINT32 i = 0; i < TAGE_TABLE_NUM; i++) {
    tage_list.push_back(
        std::make_unique<TAGE>(TAGE_TABLE_HISTORY_WIDTH[i], &ghr));
  }

  bp = BasePredictor();
  lp = LoopPredictor();
  cf = CorrectorFilter();
}

bool PREDICTOR::GetPrediction(UINT32 PC) {
  // Get prediction from the base predictor
  first_prediction = bp.predict(PC);

  // Initialize tage and alternate predictor components
  first_predictor = -1;
  second_predictor = -1;
  second_prediction = first_prediction;

  // Iterate through TAGE tables to find a matching entry
  for (UINT32 i = 0; i < TAGE_TABLE_NUM; i++) {
    if (tage_list[i]->match(PC)) {
      // Update second predictor component and prediction
      second_predictor = first_predictor;
      second_prediction = first_prediction;

      // Update tage component and prediction
      first_predictor = i;
      first_prediction = tage_list[i]->predict();
    }
  }

  // Determine high confidence status
  high_conf = (first_predictor == -1) ? bp.highConf()
                                      : tage_list[first_predictor]->highConf();

  tage_prediction = first_prediction;

#ifdef CF_ON
  // Get prediction from the corrector filter
  cf_prediction = cf.predict(PC, tage_prediction, high_conf);
#endif

#ifdef LOOP_ON
  // Get prediction from the loop predictor
  lp.predict(PC);

  // Final prediction decision
  if (lp.useLoop()) {
    return lp.prediction();
  }
#endif

#ifdef CF_ON
  // Return the prediction from the tage component or the corrector filter
  return (use_cf > USE_CF_THRESHOLD) ? cf_prediction : tage_prediction;
#else
  // Return the prediction from the tage component
  return tage_prediction;
#endif
}

void PREDICTOR::UpdatePredictor(UINT32 PC, bool resolveDir, bool predDir,
                                UINT32 branchTarget) {
#ifdef LOOP_ON
  // Update the loop predictor
  lp.update(resolveDir, first_prediction);
#endif

  // Update the base predictor or the tage component
  if (first_predictor == -1) {
    bp.update(resolveDir);
  } else {
    tage_list[first_predictor]->updateHit(resolveDir);
  }

  // Allocate new entry if prediction is incorrect and not the last table
  if (resolveDir != first_prediction && first_predictor != TAGE_TABLE_NUM - 1) {
    // Vector to store indices of unallocated entries
    std::vector<int> unalloc_indices;

    // Identify unallocated entries
    for (UINT32 i = first_predictor + 1; i < TAGE_TABLE_NUM; ++i) {
      if (tage_list[i]->getU() == 0) {
        unalloc_indices.push_back(i);
      }
    }

    if (unalloc_indices.empty()) {
      // No unallocated entries: decrement all U counters in the range
      for (UINT32 i = first_predictor + 1; i < TAGE_TABLE_NUM; ++i) {
        tage_list[i]->updateMiss();
      }
    } else {
      // Allocate an entry probabilistically
      int total_probability = bitmask(unalloc_indices.size());
      int random_value = rand() % total_probability;

      // Determine the chosen index based on probabilities
      int chosen_idx = -1;
      for (UINT32 i = 0; i < unalloc_indices.size(); ++i) {
        if (random_value >= bitmask(i) && random_value < bitmask((i + 1))) {
          chosen_idx = unalloc_indices[unalloc_indices.size() - 1 - i];
          break;
        }
      }

      // Allocate the chosen entry
      if (chosen_idx != -1) {
        tage_list[chosen_idx]->updateMissNewEntry(resolveDir);
      }
    }
  }

  // Update u counter for the tage component
  if (second_prediction != first_prediction && first_predictor != -1) {
    tage_list[first_predictor]->updateU(resolveDir, first_prediction);
  }

  // Periodically reset u counters
  clock++;
  if (clock == CLOCK_HIGH) {
    for (UINT32 i = 0; i < TAGE_TABLE_NUM; i++) {
      tage_list[i]->resetU(1);
    }
  }

  if (clock == CLOCK_MAX) {
    for (UINT32 i = 0; i < TAGE_TABLE_NUM; i++) {
      tage_list[i]->resetU(2);
    }
    clock = 0;
  }

#ifdef CF_ON
  // Update corrector filter
  cf.update(tage_prediction, resolveDir, high_conf);
  if (tage_prediction != cf_prediction) {
    use_cf = (cf_prediction == resolveDir) ? SatIncrement(use_cf, USE_CF_MAX)
                                           : SatDecrement(use_cf);
  }
#endif

  // Update global history register
  ghr <<= 1;
  if (resolveDir) {
    ghr += 1;
  }
}

void PREDICTOR::TrackOtherInst(UINT32 PC, OpType opType, UINT32 branchTarget) {
  // No operation for other instructions
  return;
}
