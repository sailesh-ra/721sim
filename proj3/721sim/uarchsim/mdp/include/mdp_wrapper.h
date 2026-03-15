#ifndef UARCHSIM_MDP_WRAPPER_H
#define UARCHSIM_MDP_WRAPPER_H
#include "mdp.h"

class OracleMDPWrapper : public MemoryDependencePredictor {
private:
   void *handle;

public:
   OracleMDPWrapper();
   ~OracleMDPWrapper() override;
   uint64_t predict(bool &valid, uint32_t &rq_index, uint64_t pc, bool store, uint64_t AL_index, bool oracle_addr_valid, uint64_t oracle_addr, uint64_t size) override;
   void checkpoint(uint32_t &rq_index, bool &rq_phase) override;
   void invalidateLFST(uint64_t pc, uint64_t AL_index, bool oracle_addr_valid, uint64_t oracle_addr, uint64_t size) override;
   void rollback(uint32_t rq_index, bool rq_phase) override;
   void pop_recovery_queue() override;
   void update(uint64_t load_pc, uint64_t store_pc_violation) override;
   void squash() override;
   void cyclicClear(uint64_t cycle) override;
   void dump_config(FILE *fout) override;
};


class StoreSetsMDPWrapper : public MemoryDependencePredictor {
private:
   void *handle;

public:
   StoreSetsMDPWrapper(size_t ssit_size, size_t lfst_size, size_t recovery_queue_size, int clear_period, bool always_predict_conflict);
   ~StoreSetsMDPWrapper() override;
   uint64_t predict(bool &valid, uint32_t &rq_index, uint64_t pc, bool store, uint64_t AL_index, bool oracle_addr_valid, uint64_t oracle_addr, uint64_t size) override;
   void checkpoint(uint32_t &rq_index, bool &rq_phase) override;
   void invalidateLFST(uint64_t pc, uint64_t AL_index, bool oracle_addr_valid, uint64_t oracle_addr, uint64_t size) override;
   void rollback(uint32_t rq_index, bool rq_phase) override;
   void pop_recovery_queue() override;
   void update(uint64_t load_pc, uint64_t store_pc_violation) override;
   void squash() override;
   void cyclicClear(uint64_t cycle) override;
   void dump_config(FILE *fout) override;
};

#endif
