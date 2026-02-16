// renamer.cc
#include "renamer.h"

renamer::renamer(uint64_t n_log_regs, uint64_t n_phys_regs,
                 uint64_t n_branches, uint64_t n_active) {
  // TODO: initialize
}

renamer::~renamer() {}

bool renamer::stall_reg(uint64_t bundle_dst) {
  return false; // TODO
}

bool renamer::stall_branch(uint64_t bundle_branch) {
  return false; // TODO
}

uint64_t renamer::get_branch_mask() {
  return 0; // TODO
}

uint64_t renamer::rename_rsrc(uint64_t log_reg) {
  return 0; // TODO
}

uint64_t renamer::rename_rdst(uint64_t log_reg) {
  return 0; // TODO
}

uint64_t renamer::checkpoint() {
  return 0; // TODO
}

bool renamer::stall_dispatch(uint64_t bundle_inst) {
  return false; // TODO
}

uint64_t renamer::dispatch_inst(bool dest_valid,
                            uint64_t log_reg,
                            uint64_t phys_reg,
                            bool load,
                            bool store,
                            bool branch,
                            bool amo,
                            bool csr,
                            uint64_t PC) {
  // TODO
}

bool renamer::is_ready(uint64_t phys_reg) { return false; }
void renamer::clear_ready(uint64_t phys_reg) {}
uint64_t renamer::read(uint64_t phys_reg) { return 0; }
void renamer::set_ready(uint64_t phys_reg) {}
void renamer::write(uint64_t phys_reg, uint64_t value) {}
void renamer::set_complete(uint64_t AL_index) {}
void renamer::resolve(uint64_t AL_index, uint64_t branch_ID, bool correct) {}

bool renamer::precommit(bool &completed,
                        bool &exception, bool &load_viol, bool &br_misp, bool &val_misp,
                        bool &load, bool &store, bool &branch, bool &amo, bool &csr,
                        uint64_t &PC) {
  return false; // TODO
}

void renamer::commit() {}
void renamer::squash() {}
void renamer::set_exception(uint64_t AL_index) {}
void renamer::set_load_violation(uint64_t AL_index) {}
void renamer::set_branch_misprediction(uint64_t AL_index) {}
void renamer::set_value_misprediction(uint64_t AL_index) {}
bool renamer::get_exception(uint64_t AL_index) { return false; }