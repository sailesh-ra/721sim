////////////////////////////////////////////////////
// TAGE-SC-L branch predictor wrapper.
//
// This is a wrapper to integrate the original
// TAGE class with 721sim's pipeline.
////////////////////////////////////////////////////

// This is the implementation of TAGE-SC-L.
#include "tage-sc-l.h"

// Anirudh:
// Define a class for this predictor's local log entry.

class tage_log_t {
public:
   uint64_t fetch_pc;
   int TageBimIndex;
   int TageBimPred;
   int TageIndex[37];
   uint TageTag[37];
   bool TagePredTaken;
   bool TagePred;
   bool TageAltPred;
   bool TageLongestMatchPred;
   int TageHitBank;
   int TageAltBank;
   bool TageAltConf;
   long long TagePhist;
   int TagePTGhist;
   uint8_t TageHist[4096];
   int TageSeed;
   bool Tagepredloop;
   int TageLIB;
   int TageLI;
   int TageLHIT;
   int TageLTAG;
   bool TageLVALID;
   bool TagePredInter;
   int TageLSUM;
   int Tageupdatethreshold;
   bool TageHighConf;
   bool TageMedConf;
   bool TageLowConf;
   long long TageGHIST;
   int TageTHRES;
   long long TageIMLIcount;
   long long TageIMHIST[256];
   long long TageL_shist[1 << 8];
   long long TageS_slhist[1 << 4];
   long long TageT_slhist[16];
   folded_history TageCh_i[37];
   folded_history TageCh_t0[37];
   folded_history TageCh_t1[37];
};

class tagescl_wrapper_t : public BPinterface_t {
private:
   // Anirudh:
   // - member variable for TAGE object
   tagescl_t *TAGE;

   // - member variable for local log
   tage_log_t *log;

   // - member variable(s) for fetch2 state
   long long fetch2_TagePhist;
   int fetch2_TagePTGhist;
   uint8_t fetch2_TageHist[4096];
   long long fetch2_TageGHIST;
   long long fetch2_TageIMLIcount;
   long long fetch2_TageIMHIST[256];
   long long fetch2_TageL_shist[1 << 8];
   long long fetch2_TageS_slhist[1 << 4];
   long long fetch2_TageT_slhist[16];
   folded_history fetch2_TageCh_i[37];
   folded_history fetch2_TageCh_t0[37];
   folded_history fetch2_TageCh_t1[37];
   int fetch2_TageSeed;

#if 0
      int fetch2_TageBimIndex;
      int fetch2_TageBimPred;
      int fetch2_TageIndex[37];
      uint fetch2_TageTag[37];
      bool fetch2_TagePredTaken;
      bool fetch2_TagePred;
      bool fetch2_TageAltPred;
      bool fetch2_TageLongestMatchPred;
      int fetch2_TageHitBank;
      int fetch2_TageAltBank;
      bool fetch2_TageAltConf;
      int fetch2_TageSeed;
      bool fetch2_Tagepredloop;
      int fetch2_TageLIB;
      int fetch2_TageLI;
      int fetch2_TageLHIT;
      int fetch2_TageLTAG;
      bool fetch2_TageLVALID;
      bool fetch2_TagePredInter;
      int fetch2_TageLSUM;
      int fetch2_Tageupdatethreshold;
      bool fetch2_TageHighConf;
      bool fetch2_TageMedConf;
      bool fetch2_TageLowConf;
      int fetch2_TageTHRES;
#endif

public:
   tagescl_wrapper_t(uint64_t bq_size);
   ~tagescl_wrapper_t();

   ///////////////////////////////////////////////
   // Called by the Fetch Unit's FETCH1 stage.
   ///////////////////////////////////////////////

   // Get "m" cond. branch predictions or 1 indirect target prediction.
   // "pc" is the start PC of the fetch bundle.
   uint64_t predict(uint64_t pc);

   // Save the predictor's context prior to speculatively updating it.
   void save_fetch2_context();

   // Speculatively update the predictor's context.
   // predictions: conditional branch predictions (the least significant bit is the first/oldest prediction in the fetch bundle).
   // num: number of conditional branch predictions in the fetch bundle.
   // pc: start PC of the fetch bundle.
   // next_pc: start PC of the next fetch bundle.
   // pop_ras, push_ras, push_ras_pc: information for speculative RAS actions.
   void spec_update(uint64_t predictions, uint64_t num,                 /* for speculatively updating branch history */
                    uint64_t pc, uint64_t next_pc,                      /* for speculatively updating path history */
                    bool pop_ras, bool push_ras, uint64_t push_ras_pc); /* for speculative RAS actions */

   ///////////////////////////////////////////////
   // Called by the Fetch Unit's FETCH2 stage.
   ///////////////////////////////////////////////

   // Restore the predictor's context due to a misfetch.
   void restore_fetch2_context();

   // Begin logging: perform initialization, if any, to prepare for logging branches in the FETCH2 bundle.
   void log_begin();

   // Log the predictor's context w.r.t. a branch in the FETCH2 bundle.
   // log_id: log entry to use.
   // branch_type: the type of the branch.
   // taken: true for taken, false for not-taken.
   // pc: start PC of the fetch bundle containing this branch.
   // next_pc: PC of the instruction after this branch.
   void log_branch(uint64_t log_id, btb_branch_type_e branch_type, bool taken, uint64_t pc, uint64_t next_pc);

   ///////////////////////////////////////////////
   // Called when a branch resolves.
   ///////////////////////////////////////////////

   // Restore the predictor's context due to a misprediction.
   // log_id: log entry of the mispredicted branch.
   // iscond: true for conditional branch, false for all other branches.
   // taken: the corrected branch direction.
   // next_pc: the corrected target.
   void mispredict(uint64_t log_id, bool iscond, bool taken, uint64_t next_pc);

   ///////////////////////////////////////////////
   // Called when there is a full pipeline flush.
   ///////////////////////////////////////////////

   // Restore the predictor's context due to a full squash.
   // log_id: log entry corresponding to the commit point of the pipeline.
   void flush(uint64_t log_id);

   ///////////////////////////////////////////////
   // Called when a branch retires.
   ///////////////////////////////////////////////

   // Train any predictor state that should be trained non-speculatively (e.g., prediction tables).
   // log_id: log entry of the retiring branch.
   // pc: start PC of the fetch bundle that contains the branch.
   // branch_in_bundle: which branch this is within the fetch bundle containing it (0: first branch, 1: second branch, etc.).
   // taken: the taken/not-taken direction of the branch.
   // next_pc: PC of the instruction after this branch.
   void commit(uint64_t log_id, uint64_t pc, uint64_t branch_in_bundle, bool taken, uint64_t next_pc);
};
