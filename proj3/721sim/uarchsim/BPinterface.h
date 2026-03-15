////////////////////////////////////////////////////
// Abstract base class for all branch predictors.
//
// Ensures all branch predictors adhere to the
// same interface.
////////////////////////////////////////////////////

class BPinterface_t {
private:
public:
   BPinterface_t() {};
   ~BPinterface_t() {};

   ///////////////////////////////////////////////
   // Called by the Fetch Unit's FETCH1 stage.
   ///////////////////////////////////////////////

   // Get "m" cond. branch predictions or 1 indirect target prediction.
   // "pc" is the start PC of the fetch bundle.
   virtual uint64_t predict(uint64_t pc) = 0;

   // Save the predictor's context prior to speculatively updating it.
   virtual void save_fetch2_context() = 0;

   // Speculatively update the predictor's context.
   // predictions: conditional branch predictions (the least significant bit is the first/oldest prediction in the fetch bundle).
   // num: number of conditional branch predictions in the fetch bundle.
   // pc: start PC of the fetch bundle.
   // next_pc: start PC of the next fetch bundle.
   // pop_ras, push_ras, push_ras_pc: information for speculative RAS actions.
   virtual void spec_update(uint64_t predictions, uint64_t num,                     /* for speculatively updating branch history */
                            uint64_t pc, uint64_t next_pc,                          /* for speculatively updating path history */
                            bool pop_ras, bool push_ras, uint64_t push_ras_pc) = 0; /* for speculative RAS actions */

   ///////////////////////////////////////////////
   // Called by the Fetch Unit's FETCH2 stage.
   ///////////////////////////////////////////////

   // Restore the predictor's context due to a misfetch.
   virtual void restore_fetch2_context() = 0;

   // Begin logging: perform initialization, if any, to prepare for logging branches in the FETCH2 bundle.
   virtual void log_begin() = 0;

   // Log the predictor's context w.r.t. a branch in the FETCH2 bundle.
   // log_id: log entry to use.
   // branch_type: the type of the branch.
   // taken: true for taken, false for not-taken.
   // pc: start PC of the fetch bundle containing this branch.
   // next_pc: PC of the instruction after this branch.
   virtual void log_branch(uint64_t log_id, btb_branch_type_e branch_type, bool taken, uint64_t pc, uint64_t next_pc) = 0;

   ///////////////////////////////////////////////
   // Called when a branch resolves.
   ///////////////////////////////////////////////

   // Restore the predictor's context due to a misprediction.
   // log_id: log entry of the mispredicted branch.
   // iscond: true for conditional branch, false for all other branches.
   // taken: the corrected branch direction.
   // next_pc: the corrected target.
   virtual void mispredict(uint64_t log_id, bool iscond, bool taken, uint64_t next_pc) = 0;

   ///////////////////////////////////////////////
   // Called when there is a full pipeline flush.
   ///////////////////////////////////////////////

   // Restore the predictor's context due to a full squash.
   // log_id: log entry corresponding to the commit point of the pipeline.
   virtual void flush(uint64_t log_id) = 0;

   ///////////////////////////////////////////////
   // Called when a branch retires.
   ///////////////////////////////////////////////

   // Train any predictor state that should be trained non-speculatively (e.g., prediction tables).
   // log_id: log entry of the retiring branch.
   // pc: start PC of the fetch bundle that contains the branch.
   // branch_in_bundle: which branch this is within the fetch bundle containing it (0: first branch, 1: second branch, etc.).
   // taken: the taken/not-taken direction of the branch.
   // next_pc: PC of the instruction after this branch.
   virtual void commit(uint64_t log_id, uint64_t pc, uint64_t branch_in_bundle, bool taken, uint64_t next_pc) = 0;
};
