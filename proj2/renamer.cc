// renamer.cc
#include "renamer.h"
#include <cassert>
#include <cstring>

renamer::renamer(uint64_t n_log_regs, uint64_t n_phys_regs,
                 uint64_t n_branches, uint64_t n_active)
{

    // Save sizes
    n_log = n_log_regs;
    n_phys = n_phys_regs;
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
        ready[p] = true;
    }

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
    if (fl_head == fl_tail) {
        if (fl_head_phase == fl_tail_phase)
            return 0;               // empty
        else
            return fl_size;         // full
    }

    if (fl_tail_phase == fl_head_phase)
        return fl_tail - fl_head;
    else
        return fl_size - (fl_head - fl_tail);
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
    return fl_occupancy() < bundle_dst;
}

bool renamer::stall_branch(uint64_t bundle_branch)
{

  uint64_t used = (uint64_t)__builtin_popcountll((unsigned long long)(GBM & gbm_mask));
  uint64_t free = n_br - used;
  return free < bundle_branch;

}

uint64_t renamer::get_branch_mask()
{
    return GBM & gbm_mask;
}

uint64_t renamer::rename_rsrc(uint64_t log_reg)
{

    assert(log_reg < n_log);

    return RMT[log_reg];
}

uint64_t renamer::rename_rdst(uint64_t log_reg)
{
    assert(log_reg < n_log);
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
    
    return p_new;
}

uint64_t renamer::checkpoint()
{
  uint64_t inuse = GBM & gbm_mask;
  for (uint64_t id = 0; id < n_br; id++) {
    uint64_t bit = 1ULL << id;
    if ((inuse & bit) == 0) {
      GBM |= bit;

      CKPT[id].gbm = GBM & gbm_mask;
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
    PRF[phys_reg] = value;
}
void renamer::set_complete(uint64_t AL_index) {
    assert(AL_index < al_size);
    AL[AL_index].completed = true;
}
void renamer::resolve(uint64_t AL_index, uint64_t branch_ID, bool correct) {
  assert(branch_ID < n_br);
  uint64_t bit = 1ULL << branch_ID;

  // clear from all checkpointed GBMs
  for (uint64_t i = 0; i < n_br; i++) CKPT[i].gbm &= ~bit;

  if (correct) {
    GBM &= ~bit;
    GBM &= gbm_mask;
    return;
  }

  // mispredict recovery
  GBM = CKPT[branch_ID].gbm & gbm_mask;
  GBM &= ~bit;

  for (uint64_t r = 0; r < n_log; r++) RMT[r] = CKPT[branch_ID].shadow_RMT[r];

  fl_head = CKPT[branch_ID].fl_head;
  fl_head_phase = CKPT[branch_ID].fl_head_phase;

  // restore AL tail to entry after branch; invalidate younger entries
  uint64_t new_tail = AL_index + 1;
  bool new_tail_phase = al_head_phase;
  if (new_tail >= al_size) new_tail -= al_size;
  if (new_tail < al_head) new_tail_phase = !al_head_phase;

  uint64_t idx = new_tail;
  bool phase = new_tail_phase;
  while (!((idx == al_tail) && (phase == al_tail_phase))) {
    AL[idx].valid = false;
    idx++;
    if (idx == al_size) { idx = 0; phase = !phase; }
  }

  al_tail = new_tail;
  al_tail_phase = new_tail_phase;
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

    // Must have something to commit
    assert(!al_empty());

    AL_entry &e = AL[al_head];

    // Commit should only be called when instruction is ready to retire
    assert(e.completed);
    assert(!e.exception);
    assert(!e.load_viol);

    // In Phase-1 perfect BP, these should not be set (but don't assume)
    // assert(!e.br_misp);
    // assert(!e.val_misp);

    // If instruction has a destination register, update AMT and free old mapping
    if (e.dest_valid) {
        uint64_t log = e.log_reg;
        uint64_t newp = e.phys_reg;

        assert(log < n_log);
        assert(newp < n_phys);

        uint64_t oldp = AMT[log];

        // oldp must be a valid physical reg
        assert(oldp < n_phys);

        // Free list must have space to push oldp (should always, but assert)
        assert(!(fl_head == fl_tail && fl_head_phase != fl_tail_phase)); // not full

        // Push old committed mapping onto Free List tail
        FL[fl_tail] = oldp;
        fl_tail++;
        if (fl_tail == fl_size) {
            fl_tail = 0;
            fl_tail_phase = !fl_tail_phase;
        }

        // Update committed mapping
        AMT[log] = newp;
    }

    // Pop Active List head
    e.valid = false; // optional but useful for debugging

    al_head++;
    if (al_head == al_size) {
        al_head = 0;
        al_head_phase = !al_head_phase;
    }
}
void renamer::squash() {
    // 1) Empty Active List
    for (uint64_t i = 0; i < al_size; i++) {
        AL[i].valid = false;   // for debugging
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
    bool *used = new bool[n_phys];
    for (uint64_t p = 0; p < n_phys; p++) used[p] = false;

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

    delete[] used;

    // 4) If you have branch state, reset it for squash (Phase-1 safe)
    GBM = 0;
    for (uint64_t i = 0; i < n_br; i++) CKPT[i].gbm = 0;
}
void renamer::set_exception(uint64_t AL_index) { AL[AL_index].exception = true; }
void renamer::set_load_violation(uint64_t AL_index) { AL[AL_index].load_viol = true; }
void renamer::set_branch_misprediction(uint64_t AL_index) { AL[AL_index].br_misp = true; }
void renamer::set_value_misprediction(uint64_t AL_index) { AL[AL_index].val_misp = true; }
bool renamer::get_exception(uint64_t AL_index) { 
    
    assert(AL_index < al_size);
    return AL[AL_index].exception;

 }