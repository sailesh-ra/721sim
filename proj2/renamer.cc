// renamer.cc
#include "renamer.h"
#include <cassert>
#include <cstring>
#include <vector>
#include <cstdio>
#include <cstring>

renamer::renamer(uint64_t n_log_regs, uint64_t n_phys_regs,
                 uint64_t n_branches, uint64_t n_active)
{

    // Save sizes
    n_log = n_log_regs;
    n_phys = n_phys_regs;
    this->n_active = n_active;
    al_size = n_active;
    n_br = n_branches;

    // Basic sanity
    assert(n_phys > n_log);
    assert(al_size > 0);

    // Allocate map tables + PRF + ready bits
    RMT = new uint64_t[n_log];
    AMT = new uint64_t[n_log];
    PRF = new uint64_t[n_phys];
    ready = new bool[n_phys];

    // Initialize AMT/RMT: logical r maps to physical p=r initially
    for(uint64_t r =0; r < n_log; r++) {
        AMT[r] = r;
        RMT[r] = r;
    }
    // Initialize PRF + ready bits
    // Committed regs (0..n_log-1) are ready; others can default to ready too.

    for(uint64_t p = 0; p < n_phys; p++) {
        PRF[p] = 0;
        ready[p] = (p < n_log); // default: not ready
    }

    for(uint64_t p = 0; p < n_log; p++) {
        ready[p] = true; // commited regs ready
    }

    PRF[0] = 0;
    ready[0] = true;

    // Free List holds only extra physical regs: [n_log .. n_phys -1]
    fl_size = n_phys - n_log;
    FL = new uint64_t[fl_size];

    for(uint64_t i = 0; i < fl_size; i++) {
        FL[i] = n_log + i;
    }

    // Free list initially full:
    // head = 0, tail = 0, and different phases => full for circular FIFO

    fl_head = 0;
    fl_tail = 0;
    fl_head_phase = 0;
    fl_tail_phase = 1; // different => full 

    // Active List allocate + initialize empty
    AL = new AL_entry[al_size];
    for(uint64_t i = 0; i < al_size; i++) {
        AL[i].valid = false;
    }

    al_head = 0;
    al_tail = 0;
    al_head_phase = 0;
    al_tail_phase = 0; // same => empty

    GBM = 0;
    gbm_mask = (n_br == 64) ? ~0ULL : ((1ULL << n_br) - 1ULL);

    CKPT = new br_ckpt_t[n_br];
    for (uint64_t i = 0; i < n_br; i++) {
    CKPT[i].shadow_RMT = new uint64_t[n_log];
    CKPT[i].fl_head = 0;
    CKPT[i].fl_head_phase = 0;
    CKPT[i].gbm = 0;
    }
}



uint64_t renamer::fl_occupancy() const
{
    if (fl_head == fl_tail)
        return (fl_head_phase == fl_tail_phase) ? 0 : fl_size;

    if (fl_head_phase == fl_tail_phase) {
        // tail ahead of head in same phase
        return (fl_tail >= fl_head) ? (fl_tail - fl_head)
                                    : (fl_size - (fl_head - fl_tail));
    } else {
        // head ahead of tail (occupancy is "wrapped")
        return (fl_head >= fl_tail) ? (fl_size - (fl_head - fl_tail))
                                    : (fl_tail - fl_head); // should be rare if phases maintained
    }
}

bool renamer::al_empty() const
{
    return (al_head == al_tail) &&
           (al_head_phase == al_tail_phase);
}

bool renamer::al_full() const
{
    return (al_head == al_tail) &&
           (al_head_phase != al_tail_phase);
}

uint64_t renamer::al_occupancy() const
{
    if (al_head == al_tail) {
        return (al_head_phase == al_tail_phase) ? 0 : al_size;
    }

    if (al_tail_phase == al_head_phase)
        return al_tail - al_head;
    else
        return al_size - (al_head - al_tail);
}

renamer::~renamer() {
    delete[] RMT;
    delete[] AMT;
    delete[] PRF;
    delete[] ready;
    delete[] FL;
    delete[] AL;

    for (uint64_t i = 0; i < n_br; i++) delete[] CKPT[i].shadow_RMT;
    delete[] CKPT;
}

bool renamer::stall_reg(uint64_t bundle_dst)
{
    static int k = 0;
    if (k < 50) {
        fprintf(stderr, "STALL_REG[%02d] bundle=%lu fl_occ=%lu head=%lu/%d tail=%lu/%d\n",
            k, bundle_dst, fl_occupancy(),
            fl_head, (int)fl_head_phase, fl_tail, (int)fl_tail_phase);
        fflush(stderr);
        k++;
    }

    return fl_occupancy() < bundle_dst;
}

bool renamer::stall_branch(uint64_t bundle_branch)
{
    static int k = 0;
    uint64_t used = (uint64_t)__builtin_popcountll((unsigned long long)(GBM & gbm_mask));
    uint64_t free = n_br - used;

    if (k < 50) {
        fprintf(stderr, "STALL_BR[%02d] bundle=%lu GBM=0x%lx used=%lu free=%lu\n",
            k, bundle_branch, GBM, used, free);
        fflush(stderr);
        k++;
    }

    return free < bundle_branch;

}

uint64_t renamer::get_branch_mask()
{
    return GBM & gbm_mask;
}

uint64_t renamer::rename_rsrc(uint64_t log_reg)
{

    assert(log_reg < n_log);
    if (log_reg == 0) return 0;
    return RMT[log_reg];
}

uint64_t renamer::rename_rdst(uint64_t log_reg)
{
    assert(log_reg < n_log);

    if (log_reg == 0) {
    // x0: ignore writes
    return 0;
    }

    // Must have atleast one free physical register
    assert(fl_occupancy() > 0);

    // Pop from Free List head
    uint64_t p_new = FL[fl_head];

    fl_head++;

    if (fl_head == fl_size) {
        fl_head = 0;
        fl_head_phase = !fl_head_phase;
    }

    // Update speculative mapping
    RMT[log_reg] = p_new;

    // Dest not ready until producer completes
    ready[p_new] = false;

    assert(RMT[0] == 0);
    assert(AMT[0] == 0);
    assert(PRF[0] == 0);
    assert(ready[0] == true);
    
    return p_new;
}

uint64_t renamer::checkpoint()
{
  uint64_t inuse = GBM & gbm_mask;
  for (uint64_t id = 0; id < n_br; id++) {
    uint64_t bit = 1ULL << id;
    if ((inuse & bit) == 0) {
      uint64_t old_gbm = GBM & gbm_mask;
      GBM |= bit;

      CKPT[id].gbm = old_gbm;
      CKPT[id].fl_head = fl_head;
      CKPT[id].fl_head_phase = fl_head_phase;
      
      for (uint64_t r = 0; r < n_log; r++) CKPT[id].shadow_RMT[r] = RMT[r];

      return id;
        }
    }
  assert(false && "checkpoint: no free checkpoint (caller should have stalled)");
  return 0;
}

bool renamer::stall_dispatch(uint64_t bundle_inst)
{
    static int k = 0;
    if (k < 50) {
        fprintf(stderr, "STALL_DISP[%02d] bundle=%lu freeAL=%lu occAL=%lu head=%lu/%d tail=%lu/%d\n",
            k, bundle_inst, (al_size - al_occupancy()), al_occupancy(),
            al_head, (int)al_head_phase, al_tail, (int)al_tail_phase);
        fflush(stderr);
        k++;
    }
    // The simulator may pass a maximum bundle size (e.g., 8) regardless of perf.
    //uint64_t effective = 1;
    return (al_size - al_occupancy()) < bundle_inst;
}

uint64_t renamer::dispatch_inst(bool dest_valid,
                                uint64_t log_reg,
                                uint64_t phys_reg,
                                bool load,
                                bool store,
                                bool branch,
                                bool amo,
                                bool csr,
                                uint64_t PC)
{

    // AL full check (phase-bit FIFO full condition)
    assert(!al_full());
    
    
    uint64_t idx = al_tail;

    // Fill entry
    AL[idx].valid = true;
    AL[idx].completed = false;

    AL[idx].exception = false;
    AL[idx].load_viol = false;
    AL[idx].br_misp = false;
    AL[idx].val_misp = false;

    AL[idx].load = load;
    AL[idx].store = store;
    AL[idx].branch = branch;
    AL[idx].amo = amo;
    AL[idx].csr = csr;

   if (dest_valid && log_reg == 0) {
    // x0 is hardwired to 0: treat as no-dest
    dest_valid = false;
    log_reg = 0;
    phys_reg = 0;
    }

    AL[idx].dest_valid = dest_valid;
    AL[idx].log_reg = dest_valid ? log_reg : 0;
    AL[idx].phys_reg = dest_valid ? phys_reg : 0;

    AL[idx].PC = PC;

    // Advance tail
    al_tail++;
    if (al_tail == al_size) {
        al_tail = 0;
        al_tail_phase = !al_tail_phase;
    }

static int d_count = 0;
if (d_count < 12) {
  fprintf(stderr,
    "DISP%02d: idx=%lu al_head=%lu/%d al_tail=%lu/%d occ=%lu dest=%d log=%lu phys=%lu PC=0x%lx,load=%d store=%d br=%d amo=%d csr=%d\n",
    d_count, idx,
    al_head, (int)al_head_phase, al_tail, (int)al_tail_phase, al_occupancy(),
    (int)AL[idx].dest_valid, AL[idx].log_reg, AL[idx].phys_reg, AL[idx].PC,load, store, branch, amo, csr);
  fflush(stderr);
  d_count++;
}


    return idx;
}
bool renamer::is_ready(uint64_t phys_reg) { 
    assert(phys_reg < n_phys);
    return ready[phys_reg];
}
void renamer::clear_ready(uint64_t phys_reg) {
    assert(phys_reg < n_phys);
    ready[phys_reg] = false;
}
uint64_t renamer::read(uint64_t phys_reg) { 
    assert(phys_reg < n_phys);
    return PRF[phys_reg];
}
void renamer::set_ready(uint64_t phys_reg) {
    assert(phys_reg < n_phys);
    ready[phys_reg] = true; 
}
void renamer::write(uint64_t phys_reg, uint64_t value) {
    assert(phys_reg < n_phys);
    if (phys_reg == 0) {

    PRF[0] = 0;
    ready[0] = true;

    assert(RMT[0] == 0);
    assert(AMT[0] == 0);
    assert(PRF[0] == 0);
    assert(ready[0] == true);

    return;
  }
    PRF[phys_reg] = value;
    //ready[phys_reg] = true;
    //assert(ready[phys_reg] == true);   // if pipeline guarantees set_complete happened first
}

void renamer::set_complete(uint64_t AL_index)
{
    assert(AL_index < al_size);
    assert(AL[AL_index].valid);

    assert(AL[AL_index].completed == false);
    AL[AL_index].completed = true;

    static int c_once = 0;
    if (!c_once) {
      c_once = 1;
        fprintf(stderr,
          "COMP0: AL_index=%lu valid=%d dest=%d log=%lu phys=%lu ready=%d\n",
            AL_index, (int)AL[AL_index].valid,
            (int)AL[AL_index].dest_valid, AL[AL_index].log_reg, AL[AL_index].phys_reg,
            (AL[AL_index].dest_valid ? (int)ready[AL[AL_index].phys_reg] : -1)
            );
        fflush(stderr);
      }


    if (AL[AL_index].dest_valid) {
        uint64_t p = AL[AL_index].phys_reg;
        //assert(p != 0);
        assert(p < n_phys);
        ready[p] = true;
    }
}

void renamer::resolve(uint64_t AL_index, uint64_t branch_ID, bool correct) {
    // Phase 1: perfect BP — resolve is always correct, no recovery needed.
    // Just clear the branch's bit from the GBM and all checkpointed GBMs.
    assert(branch_ID < n_br);
    uint64_t bit = 1ULL << branch_ID;
    for (uint64_t i = 0; i < n_br; i++) CKPT[i].gbm &= ~bit;
    GBM &= ~bit;
}

bool renamer::precommit(bool &completed,
                        bool &exception, bool &load_viol, bool &br_misp, bool &val_misp,
                        bool &load, bool &store, bool &branch, bool &amo, bool &csr,
                        uint64_t &PC)
{

    if (al_empty()) {
        return false;
    }

    const AL_entry &e = AL[al_head];

    // If you keep valid bits, This should hold when not empty:
    // assert(e.valid);

    completed = e.completed;
    exception = e.exception;
    load_viol = e.load_viol;
    br_misp = e.br_misp;
    val_misp = e.val_misp;

    load = e.load;
    store = e.store;
    branch = e.branch;
    amo = e.amo;
    csr = e.csr;
    
    PC = e.PC;

    return true;
}

void renamer::commit() {
  assert(!al_empty());
  assert(al_occupancy() < al_size);

  AL_entry e = AL[al_head];
  assert(e.valid);

  static int cmt_count = 0;
  if (cmt_count < 4) {
    //once = 1;
    fprintf(stderr,
    "COMMIT0: al_head=%lu/%d al_tail=%lu/%d occ=%lu "
    "dest=%d log=%lu newp=%lu completed=%d AMT[log]=%lu "
    "FL head=%lu/%d tail=%lu/%d fl_occ=%lu\n",
    al_head, al_head_phase, al_tail, al_tail_phase, al_occupancy(),
    e.dest_valid, e.log_reg, e.phys_reg, e.completed,
    (e.dest_valid ? AMT[e.log_reg] : 999999UL),
    fl_head, fl_head_phase, fl_tail, fl_tail_phase, fl_occupancy()
    );
    fflush(stderr);
    cmt_count++;
  }

  assert(e.completed);
  assert(!e.exception);
  assert(!e.load_viol);

  if (e.dest_valid) {
    uint64_t log  = e.log_reg;
    uint64_t newp = e.phys_reg;

    assert(log < n_log);
    assert(newp < n_phys);

    if (log != 0) { 
      uint64_t oldp = AMT[log];
      assert(oldp < n_phys);
      //assert(oldp < n_log);
      //assert(oldp != 0);

      // FL must not be full
      if (oldp != newp) {
      assert(fl_occupancy() < fl_size);
      FL[fl_tail] = oldp;
      fl_tail++;
      if (fl_tail == fl_size) {
          fl_tail = 0;
          fl_tail_phase = !fl_tail_phase;
        }
    }

      AMT[log] = newp;
    }
  }

  // ALWAYS retire the head entry
  std::memset(&AL[al_head], 0, sizeof(AL_entry));

  al_head++;
  if (al_head == al_size) {
    al_head = 0;
    al_head_phase = !al_head_phase;
  }

  // x0 invariants
  assert(RMT[0] == 0);
  assert(AMT[0] == 0);
  assert(PRF[0] == 0);
  assert(ready[0] == true);

}

void renamer::squash() {
    // 1) Empty Active List
    for (uint64_t i = 0; i < al_size; i++) {
       std::memset(&AL[i], 0, sizeof(AL_entry)); //AL[i].valid = false;   // for debugging
    }
    al_head = 0;
    al_tail = 0;
    al_head_phase = 0;
    al_tail_phase = 0; // same => empty

    // 2) Restore RMT = AMT (committed state)
    for (uint64_t r = 0; r < n_log; r++) {
        RMT[r] = AMT[r];
    }

    // 3) Rebuild Free List from AMT mappings:
    //    Mark phys regs referenced by AMT as "used".
    // bool *used = new bool[n_phys];
    std::vector<bool> used(n_phys, false); // std::vector<char> used(n_phys, 0);


    for (uint64_t r = 0; r < n_log; r++) {
        uint64_t p = AMT[r];
        assert(p < n_phys);
        used[p] = true;
    }

    // Fill FL with all phys regs that are not used by AMT.
    // fl_size should equal n_phys - n_log
    uint64_t k = 0;
    for (uint64_t p = 0; p < n_phys; p++) {
        if (!used[p]) {
            assert(k < fl_size);
            FL[k++] = p;
        }
    }
    assert(k == fl_size);

    // Set Free List to "full" state (phase-bit convention)
    fl_head = 0;
    fl_tail = 0;
    fl_head_phase = 0;
    fl_tail_phase = 1;   // different -> full

   // for (uint64_t p = 0; p < n_phys; p++) ready[p] = true;
   // ready[0] = true;

    // 4) If you have branch state, reset it for squash (Phase-1 safe)
    GBM = 0;
    for (uint64_t i = 0; i < n_br; i++) CKPT[i].gbm = 0;

    assert(RMT[0] == 0);
    assert(AMT[0] == 0);
    assert(PRF[0] == 0);
    assert(ready[0] == true);
}

void renamer::set_exception(uint64_t AL_index) { AL[AL_index].exception = true; }
void renamer::set_load_violation(uint64_t AL_index) { AL[AL_index].load_viol = true; }
void renamer::set_branch_misprediction(uint64_t AL_index) { AL[AL_index].br_misp = true; }
void renamer::set_value_misprediction(uint64_t AL_index) { AL[AL_index].val_misp = true; }
bool renamer::get_exception(uint64_t AL_index) { 
    
    assert(AL_index < al_size);
    return AL[AL_index].exception;

 }