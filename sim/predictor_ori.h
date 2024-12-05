#ifndef _PREDICTOR_H_
#define _PREDICTOR_H_

#include "utils.h"
#include "tracer.h"

#define UINT16      unsigned short int



/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

class PREDICTOR{

  // The state is defined for Gshare, change for your design

 private:


  UINT32  ghr;           // global history register
  UINT32  *pht;          // pattern history table
  UINT32  historyLength; // history length
  UINT32  numPhtEntries; // entries in pht


  //add for local predictor
  //local pattern history table
  //UINT32 pht_local_bit_size;
  UINT32 *pht_local;
  UINT32 numPhtLocalEntries;

  //branch history table for local branch predictor
  UINT32 bht_history_length;
  UINT32 numBhtEntries;
  UINT32 bht_bit_size;
  UINT16 *bht;

  //for tournament counter
  UINT32 *predictorChooseCounter;
  UINT32 numTournamentCounter;




 public:

  // The interface to the four functions below CAN NOT be changed

  PREDICTOR(void);
  bool    GetPrediction(UINT32 PC);

  //add for tournament predictor
  bool    GetLocalPrediction(UINT32 PC);
  bool    GetGlobalPrediction(UINT32 PC);

  void    UpdatePredictor(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget);
  void    TrackOtherInst(UINT32 PC, OpType opType, UINT32 branchTarget);


  // Contestants can define their own functions below

};


/***********************************************************/
#endif

