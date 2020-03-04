//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
// First do this:
//docker run --rm -it -v //c/Users/47181/Desktop/cse240a:/home/cse240a prodromou87/ucsd_cse240a

#include <stdio.h>
#include <string.h>
#include "predictor.h"

//
// Student Information
//
const char *studentName = "Dingqian Zhao and Kexin Hong";
const char *studentID   = "A53319585 and A53311871";
const char *email       = "dizhao@eng.ucsd.edu and k6hong@eng.ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// Add your own Branch Predictor data structures here
//

uint32_t globalHR; // global history register
uint32_t* globalPHT;  // global pattern history table
uint32_t* localPHT;  // local pattern history table ???
uint32_t* localBHT;  // local branch history table
uint32_t* choicePHT;

uint32_t gMask;  // global mask
uint32_t pcMask; // pc mask
uint32_t lMask;  // local mask


//------------------------------------//
//        Helper Functions            //
//------------------------------------//

// Updates the global history register from outcome.
void updateGlobalHR(uint8_t outcome) {
  globalHR = (globalHR << 1) | outcome;
  globalHR = globalHR & gMask;
}

// Updates 2-bit saturation counter: If taken, counter increments 
// when not in ST state; else, it decrements when not in SN state.
void updateGlobalPHT(uint8_t outcome, uint32_t index) {
  if (outcome) {
    if (globalPHT[index] < 3) 
      globalPHT[index]++;
  } else {
    if (globalPHT[index] > 0) 
      globalPHT[index]--; 
  }
}
void updateLocalPHT(uint8_t outcome, uint32_t index) {
  if (outcome) {
    if (localPHT[index] < 3) 
      localPHT[index]++;
  } else {
    if (localPHT[index] > 0) 
      localPHT[index]--; 
  }
}

// Updates the Choice Pattern History Table.
void updateChoicePHT(uint32_t pc, uint8_t outcome) {
  
  uint8_t globalChoice = globalPHT[globalHR & gMask] >> 1;
  uint32_t index = (localBHT[pc & pcMask]) & lMask;
  uint8_t localChoice = localPHT[index] >> 1;

  uint8_t globalCorrectness = (outcome == globalChoice) ? 1 : 0;
  uint8_t localCorrectness = (outcome == localChoice) ? 1 : 0;

  uint32_t ghrMasked = globalHR & gMask;
  if (globalCorrectness && localCorrectness == 0 && choicePHT[ghrMasked] > 0) {   
    choicePHT[ghrMasked]--;
  } else if (globalCorrectness == 0 && localCorrectness && choicePHT[ghrMasked] < 3) {
    choicePHT[ghrMasked]++;
  }
}

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize Branch Predictor Data Structures
//
void
init_predictor()
{
  globalHR = 0;

  gMask = (1 << ghistoryBits) - 1;
  pcMask = (1 << pcIndexBits) - 1;
  lMask = (1 << lhistoryBits) - 1;

  switch (bpType) {
    // Initialize history registers
    case CUSTOM:
    case GSHARE: 
      // Initialize global pattern history table all 2-bit predictors to Weakly Not Taken
      globalPHT = malloc(sizeof(uint32_t) * (1<<ghistoryBits));
      memset(globalPHT, 1, sizeof(globalPHT));
      break;

    case TOURNAMENT:
      // global
      globalPHT = malloc(sizeof(uint32_t) * (1<<ghistoryBits));
      memset(globalPHT, 1, sizeof(globalPHT));
      // local
      localBHT = malloc(sizeof(uint32_t) * (1<<pcIndexBits));
      memset(localBHT, 0, sizeof(localBHT));
      localPHT = malloc(sizeof(uint32_t) * (1<<lhistoryBits));
      memset(localPHT, 1, sizeof(localPHT));
      // choice 
      choicePHT = malloc(sizeof(uint32_t) * (1<<ghistoryBits));
      memset(choicePHT, 1, sizeof(choicePHT));
      break;
    default:
      break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //Implement prediction scheme
  uint32_t index;
 
  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
    case CUSTOM:
      index = (globalHR & gMask) ^ (pc & gMask);  // XOR hashing for Gshare
      return globalPHT[index] >> 1;

    case TOURNAMENT:
      index = globalHR & gMask;
      if (choicePHT[index] < 2) {  
        // Choice Predictor : 00,01 -- Global Predictor
        return globalPHT[index] >> 1;
      } else { 
        // Choice Predictor : 10,11 -- Local Predictor
        index = (localBHT[pc & pcMask]) & lMask;
        return localPHT[index] >> 1;
      }
      break;
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  // Implement Predictor training
  uint32_t index;
  uint32_t pcIndex;
  switch (bpType) {
    case CUSTOM:
    case GSHARE:
      index = (globalHR & gMask) ^ (pc & gMask);
      updateGlobalPHT(outcome, index);
      updateGlobalHR(outcome);
      break;

    case TOURNAMENT:
      index = globalHR & gMask;
      updateGlobalPHT(outcome, index);
      updateGlobalHR(outcome);

      pcIndex = pc & pcMask;
      index = (localBHT[pcIndex]) & lMask;
      updateLocalPHT(outcome, index);
      localBHT[pcIndex] = (localBHT[pcIndex] << 1) | outcome;
      localBHT[pcIndex] = localBHT[pcIndex] & lMask;

      updateChoicePHT(pc, outcome);
      break;
    default:
      break;
  }
  return;
}
