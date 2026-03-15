#ifndef UARCHSIM_MDP_H
#define UARCHSIM_MDP_H

/////////////////////////////////////////////////////////////
// Abstract base class for all memory dependence predictors.
//
// Ensures all memory dependence predictors adhere to the
// same interface.
//
// Though the interface is supposed to be generic,
// comments above member functions are w.r.t. Store Sets.
/////////////////////////////////////////////////////////////

#include <cstdint>
#include <cstdio>

class MemoryDependencePredictor {
public:
   MemoryDependencePredictor() {};
   virtual ~MemoryDependencePredictor() {};

   // Called by the dispatch stage.
   // Predict whether a store or load should wait for a preceding store.
   virtual uint64_t predict(bool &valid, uint32_t &rq_index, uint64_t pc, bool store, uint64_t AL_index,
                            /* add-ons for oracle */ bool oracle_addr_valid, uint64_t oracle_addr, uint64_t size) = 0;

   // Checkpoint the recovery queue tail.
   virtual void checkpoint(uint32_t &rq_index, bool &rq_phase) = 0;

   // Invalidate the LFST entry after issuing a store.
   virtual void invalidateLFST(uint64_t pc, uint64_t AL_index,
                               /* add-ons for oracle */ bool oracle_addr_valid, uint64_t oracle_addr, uint64_t size) = 0;

   // Use the recovery queue to repair the LFST.
   virtual void rollback(uint32_t rq_index, bool rq_phase) = 0;

   // Pop the oldest entry from the recovery queue.
   virtual void pop_recovery_queue() = 0;

   // Train the predictor when there is a load violation.
   virtual void update(uint64_t load_pc, uint64_t store_pc_violation) = 0;

   // Squash the recovery queue and invalidate all entries in the LFST.
   virtual void squash() = 0;

   // Clear the SSIT periodically if not using prediction feedback.
   virtual void cyclicClear(uint64_t cycle) = 0;

   // Dump the configuration of the predictor.
   virtual void dump_config(FILE *fout) = 0;
};

#endif
