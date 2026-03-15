#include <cinttypes>
#include <cassert>
#include "decode.h"
#include "fetchunit_types.h"
#include "BPinterface.h"
#include "tage-sc-l-wrapper.h"

////////////////////////////////////////////////////
// TAGE-SC-L branch predictor wrapper.
//
// This is a wrapper to integrate the original
// TAGE class with 721sim's pipeline.
////////////////////////////////////////////////////

tagescl_wrapper_t::tagescl_wrapper_t(uint64_t bq_size) {
   TAGE = new tagescl_t();
   log = new tage_log_t[bq_size];
}

tagescl_wrapper_t::~tagescl_wrapper_t() {
}

// Get "m" cond. branch predictions or 1 indirect target prediction.
// "pc" is the start PC of the fetch bundle.
uint64_t tagescl_wrapper_t::predict(uint64_t pc) {
   return (uint64_t) TAGE->getPrediction(pc);
}

// Save the predictor's context prior to speculatively updating it.
void tagescl_wrapper_t::save_fetch2_context() {
   TAGE->getTageHist(fetch2_TagePhist,
                     fetch2_TagePTGhist, fetch2_TageHist,
                     fetch2_TageGHIST,
                     fetch2_TageIMLIcount, fetch2_TageIMHIST,
                     fetch2_TageL_shist, fetch2_TageS_slhist,
                     fetch2_TageT_slhist,
                     fetch2_TageCh_i,
                     fetch2_TageCh_t0,
                     fetch2_TageCh_t1,
                     fetch2_TageSeed);
}

// Speculatively update the predictor's context.
// predictions: conditional branch predictions (the least significant bit is the first/oldest prediction in the fetch bundle).
// num: number of conditional branch predictions in the fetch bundle.
// pc: start PC of the fetch bundle.
// next_pc: start PC of the next fetch bundle.
// pop_ras, push_ras, push_ras_pc: information for speculative RAS actions.
void tagescl_wrapper_t::spec_update(uint64_t predictions, uint64_t num,                  /* for speculatively updating branch history */
                                    uint64_t pc, uint64_t next_pc,                       /* for speculatively updating path history */
                                    bool pop_ras, bool push_ras, uint64_t push_ras_pc) { /* for speculative RAS actions */
                                                                                         // Question for Anirudh:
                                                                                         // Why does TAGE need both pc (start PC of the fetch bundle) and next_pc (start PC of the next fetch bundle) to update path history?
                                                                                         // Using both seems redundant.
   assert(num <= 1);
   if (num != 0)
      TAGE->SpecHistoryUpdate(pc, OPTYPE_JMP_DIRECT_COND, (bool) predictions, next_pc);
}

// Restore the predictor's context due to a misfetch.
void tagescl_wrapper_t::restore_fetch2_context() {
   TAGE->HistoryRevert(fetch2_TagePhist,
                       fetch2_TagePTGhist,
                       fetch2_TageHist,
                       fetch2_TageGHIST,
                       fetch2_TageIMLIcount,
                       fetch2_TageIMHIST,
                       fetch2_TageL_shist,
                       fetch2_TageS_slhist,
                       fetch2_TageT_slhist,
                       fetch2_TageCh_i,
                       fetch2_TageCh_t0,
                       fetch2_TageCh_t1);
   //printf ("Restoring context due to misfetch here\n");
}

// Begin logging: perform initialization, if any, to prepare for logging branches in the FETCH2 bundle.
void tagescl_wrapper_t::log_begin() {
}

// Log the predictor's context w.r.t. a branch in the FETCH2 bundle.
// log_id: log entry to use.
// branch_type: the type of the branch.
// taken: true for taken, false for not-taken.
// pc: start PC of the fetch bundle containing this branch.
// next_pc: PC of the instruction after this branch.
void tagescl_wrapper_t::log_branch(uint64_t log_id, btb_branch_type_e branch_type, bool taken, uint64_t pc, uint64_t next_pc) {
   log[log_id].fetch_pc = pc;
   log[log_id].TagePhist = fetch2_TagePhist;
   log[log_id].TagePTGhist = fetch2_TagePTGhist;
   log[log_id].TageSeed = fetch2_TageSeed;
   for (int i = 0; i < 4096; i++)
      log[log_id].TageHist[i] = fetch2_TageHist[i];
   log[log_id].TageGHIST = fetch2_TageGHIST;
   log[log_id].TageIMLIcount = fetch2_TageIMLIcount;
   for (int i = 0; i < 256; i++)
      log[log_id].TageIMHIST[i] = fetch2_TageIMHIST[i];
   for (int i = 0; i < 256; i++)
      log[log_id].TageL_shist[i] = fetch2_TageL_shist[i];
   for (int i = 0; i < 16; i++)
      log[log_id].TageS_slhist[i] = fetch2_TageS_slhist[i];
   for (int i = 0; i < 16; i++)
      log[log_id].TageT_slhist[i] = fetch2_TageT_slhist[i];
   for (int i = 0; i < 37; i++)
      log[log_id].TageCh_i[i] = fetch2_TageCh_i[i];
   for (int i = 0; i < 37; i++)
      log[log_id].TageCh_t0[i] = fetch2_TageCh_t0[i];
   for (int i = 0; i < 37; i++)
      log[log_id].TageCh_t1[i] = fetch2_TageCh_t1[i];

   TAGE->getPredictionContext(
      log[log_id].TageBimIndex,
      log[log_id].TageBimPred,
      log[log_id].TageIndex,
      log[log_id].TageTag,
      log[log_id].TagePredTaken,
      log[log_id].TagePred,
      log[log_id].TageAltPred,
      log[log_id].TageAltConf,
      log[log_id].TageLongestMatchPred,
      log[log_id].TageHitBank,
      log[log_id].TageAltBank,
      log[log_id].TageSeed,
      log[log_id].Tagepredloop,
      log[log_id].TageLIB,
      log[log_id].TageLI,
      log[log_id].TageLHIT,
      log[log_id].TageLTAG,
      log[log_id].TageLVALID,
      log[log_id].TagePredInter,
      log[log_id].TageLSUM,
      log[log_id].Tageupdatethreshold,
      log[log_id].TageHighConf,
      log[log_id].TageMedConf,
      log[log_id].TageLowConf,
      log[log_id].TageTHRES);

   //Following not needed for one branch prediction per cycle
   if (branch_type == BTB_BRANCH) {
      TAGE->MyHistoryUpdate(pc, OPTYPE_JMP_DIRECT_COND, taken, next_pc,
                            fetch2_TagePhist, fetch2_TagePTGhist,
                            fetch2_TageHist, fetch2_TageGHIST,
                            fetch2_TageIMLIcount, fetch2_TageIMHIST,
                            fetch2_TageL_shist, fetch2_TageS_slhist,
                            fetch2_TageT_slhist,
                            fetch2_TageCh_i, fetch2_TageCh_t0, fetch2_TageCh_t1);
   }
}

// Restore the predictor's context due to a misprediction.
// log_id: log entry of the mispredicted branch.
// iscond: true for conditional branch, false for all other branches.
// taken: the corrected branch direction.
// next_pc: the corrected target.
void tagescl_wrapper_t::mispredict(uint64_t log_id, bool iscond, bool taken, uint64_t next_pc) {
   TAGE->HistoryRevertAndUpdate(log[log_id].fetch_pc, OPTYPE_JMP_DIRECT_COND,
                                taken, next_pc, log[log_id].TagePhist,
                                log[log_id].TagePTGhist, log[log_id].TageHist,
                                log[log_id].TageGHIST,
                                log[log_id].TageIMLIcount, log[log_id].TageIMHIST,
                                log[log_id].TageL_shist, log[log_id].TageS_slhist,
                                log[log_id].TageT_slhist,
                                log[log_id].TageCh_i,
                                log[log_id].TageCh_t0,
                                log[log_id].TageCh_t1);
   //printf ("Restoring context due to mispredict here\n");
}

// Restore the predictor's context due to a full squash.
// log_id: log entry corresponding to the commit point of the pipeline.
void tagescl_wrapper_t::flush(uint64_t log_id) {
   TAGE->HistoryRevert(log[log_id].TagePhist,
                       log[log_id].TagePTGhist,
                       log[log_id].TageHist,
                       log[log_id].TageGHIST,
                       log[log_id].TageIMLIcount,
                       log[log_id].TageIMHIST,
                       log[log_id].TageL_shist,
                       log[log_id].TageS_slhist,
                       log[log_id].TageT_slhist,
                       log[log_id].TageCh_i,
                       log[log_id].TageCh_t0,
                       log[log_id].TageCh_t1);
   //printf ("Restoring context due to flush here\n");
}

// Train any predictor state that should be trained non-speculatively (e.g., prediction tables).
// log_id: log entry of the retiring branch.
// pc: start PC of the fetch bundle that contains the branch.
// branch_in_bundle: which branch this is within the fetch bundle containing it (0: first branch, 1: second branch, etc.).
// taken: the taken/not-taken direction of the branch.
// next_pc: PC of the instruction after this branch.
void tagescl_wrapper_t::commit(uint64_t log_id, uint64_t pc, uint64_t branch_in_bundle, bool taken, uint64_t next_pc) {
   // When using TAGE-SC-L, the simulator should be configured to terminate fetch bundles at every branch, including not-taken conditional branches.
   // Thus, branch_in_bundle -- which conditional branch in the fetch bundle -- should always be 0.
   assert(branch_in_bundle == 0);
   TAGE->UpdatePredictorMicro(pc,
                              OPTYPE_JMP_DIRECT_COND, taken,
                              taken, next_pc,
                              log[log_id].TageBimIndex, log[log_id].TageBimPred,
                              log[log_id].TageIndex, log[log_id].TageTag,
                              log[log_id].TagePredTaken, log[log_id].TagePred,
                              log[log_id].TageAltPred,
                              log[log_id].TageLongestMatchPred,
                              log[log_id].TageHitBank, log[log_id].TageAltBank,
                              log[log_id].TageAltConf, log[log_id].TagePhist,
                              log[log_id].TagePTGhist, log[log_id].TageSeed,
                              log[log_id].Tagepredloop, log[log_id].TageLIB,
                              log[log_id].TageLI, log[log_id].TageLHIT,
                              log[log_id].TageLTAG, log[log_id].TageLVALID,
                              log[log_id].TagePredInter, log[log_id].TageLSUM,
                              log[log_id].TageHighConf, log[log_id].TageMedConf,
                              log[log_id].TageLowConf, log[log_id].TageGHIST,
                              log[log_id].TageTHRES, log[log_id].TageIMLIcount,
                              log[log_id].TageIMHIST, log[log_id].TageL_shist,
                              log[log_id].TageS_slhist, log[log_id].TageT_slhist);
}
