#include "ras_recover.h"

class ras_log_t {
public:
   uint64_t tos_pointer; // TOS pointer prior to this branch.
   uint64_t tos_content; // TOS content prior to this branch.
   bool iscall;          // If true, this branch is a call instruction and affected the RAS.
   bool isreturn;        // If true, this branch is a return instruction and affected the RAS.
};

class ras_t : public BPinterface_t {
private:
   // The RAS itself.
   uint64_t *ras;
   uint64_t size;
   uint64_t tos;

   // The RAS undo log and recovery approach.
   ras_log_t *log;
   ras_recover_e recovery_approach;
   uint64_t tail; // Track where the Fetch Unit's BQ tail is (for walk-based recovery).
   uint64_t log_size;

   // State used in the FETCH2 stage to either restore the RAS in the case of a misfetch or log information in the undo log.
   // These are the TOS pointer and TOS content prior to the fetch bundle.
   uint64_t fetch2_tos_pointer;
   uint64_t fetch2_tos_content;

   void push(uint64_t x); // a call pushes its return address onto the RAS
   uint64_t pop();        // a return pops its predicted return address from the RAS

public:
   ras_t(uint64_t size, ras_recover_e recovery_approach, uint64_t bq_size);
   ~ras_t();

   ///////////////////////////////////////////////
   // Called by the Fetch Unit's FETCH1 stage.
   ///////////////////////////////////////////////

   // Get 1 return target prediction.
   //
   // "pc" is the start PC of the fetch bundle; it is unused.
   uint64_t predict(uint64_t pc);

   // Save the TOS pointer and TOS content as they exist prior to the fetch bundle.
   void save_fetch2_context();

   // Speculatively update the RAS.
   void spec_update(uint64_t predictions, uint64_t num,                 /* unused: for speculatively updating branch history */
                    uint64_t pc, uint64_t next_pc,                      /* unused: for speculatively updating path history */
                    bool pop_ras, bool push_ras, uint64_t push_ras_pc); /* used: for speculative RAS actions */

   ///////////////////////////////////////////////
   // Called by the Fetch Unit's FETCH2 stage.
   ///////////////////////////////////////////////

   // Restore the RAS after a misfetch.
   void restore_fetch2_context();

   // Begin logging.  Nothing needed here.
   void log_begin();

   // Log a branch: record the TOS pointer (fetch2_tos_pointer) and TOS content (fetch2_tos_content) w.r.t. the branch.
   // These are the TOS pointer and TOS content prior to the fetch bundle containing the branch:
   // ** this assumes fetch bundles are terminated after calls and returns, i.e., the RAS is unchanged within a fetch bundle. **
   // Also record whether the branch is a call or return instruction.
   void log_branch(uint64_t log_id,
                   btb_branch_type_e branch_type,
                   bool taken, uint64_t pc, uint64_t next_pc); /* unused */

   ///////////////////////////////////////////////
   // Called when a branch resolves.
   ///////////////////////////////////////////////

   // Restore the RAS after a mispredicted branch.
   void mispredict(uint64_t log_id,
                   bool iscond, bool taken, uint64_t next_pc); /* unused */

   ///////////////////////////////////////////////
   // Called when there is a full pipeline flush.
   ///////////////////////////////////////////////

   void flush(uint64_t log_id);

   ///////////////////////////////////////////////
   // Called when a branch retires.
   ///////////////////////////////////////////////

   // This predictor doesn't have any state that is non-speculatively trained.
   // Nevertheless, declare and define this function because it is a pure virtual function in the base (abstract) class.
   void commit(uint64_t log_id,
               uint64_t pc,
               uint64_t branch_in_bundle,
               bool taken,
               uint64_t next_pc);
};
