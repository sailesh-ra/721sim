#include <cinttypes>
#include <cassert>
#include "decode.h"
#include "fetchunit_types.h"
#include "BPinterface.h"
#include "ras.h"

ras_t::ras_t(uint64_t size, ras_recover_e recovery_approach, uint64_t bq_size) {
   this->size = ((size > 0) ? size : 1);
   ras = new uint64_t[this->size];
   for (size_t i = 0; i < this->size; i++) {
      ras[i] = 0;
   }

   tos = 0;

   log = new ras_log_t[bq_size];
   this->recovery_approach = recovery_approach;
   tail = 0;
   log_size = bq_size;
}

ras_t::~ras_t() {
   delete[] ras;
   delete[] log;
}

// a call pushes its return address onto the RAS
void ras_t::push(uint64_t x) {
   tos++;
   if (tos == size)
      tos = 0;

   ras[tos] = x;
}

// a return pops its predicted return address from the RAS
uint64_t ras_t::pop() {
   uint64_t x = ras[tos];
   tos = ((tos > 0) ? (tos - 1) : (size - 1));
   return (x);
}

// Get 1 return target prediction.
//
// "pc" is the start PC of the fetch bundle; it is unused.
uint64_t ras_t::predict(uint64_t pc) {
   return (ras[tos]);
}

// Save the TOS pointer and TOS content as they exist prior to the fetch bundle.
void ras_t::save_fetch2_context() {
   fetch2_tos_pointer = tos;
   fetch2_tos_content = ras[tos];
}

// Speculatively update the RAS.
void ras_t::spec_update(uint64_t predictions, uint64_t num,                  /* unused: for speculatively updating branch history */
                        uint64_t pc, uint64_t next_pc,                       /* unused: for speculatively updating path history */
                        bool pop_ras, bool push_ras, uint64_t push_ras_pc) { /* used: for speculative RAS actions */
   if (pop_ras) {
      assert(!push_ras);
      pop();
   }
   if (push_ras) {
      assert(!pop_ras);
      push(push_ras_pc);
   }
}

// Restore the RAS after a misfetch.
void ras_t::restore_fetch2_context() {
   tos = fetch2_tos_pointer;

   // The following assertion assumes we don't have multiple calls and returns of the following nature in the same fetch bundle:
   // return, call
   // Always terminating a fetch bundle at a call or return meets this requirement.
   assert(ras[tos] == fetch2_tos_content);
}

void ras_t::log_begin() {
}

// Log a branch: record the TOS pointer (fetch2_tos_pointer) and TOS content (fetch2_tos_content) w.r.t. the branch.
// These are the TOS pointer and TOS content prior to the fetch bundle containing the branch:
// ** this assumes fetch bundles are terminated after calls and returns, i.e., the RAS is unchanged within a fetch bundle. **
// Also record whether the branch is a call or return instruction.
void ras_t::log_branch(uint64_t log_id,
                       btb_branch_type_e branch_type,
                       bool taken, uint64_t pc, uint64_t next_pc) { /* unused */
   log[log_id].tos_pointer = fetch2_tos_pointer;
   log[log_id].tos_content = fetch2_tos_content;
   log[log_id].iscall = ((branch_type == BTB_CALL_DIRECT) || (branch_type == BTB_CALL_INDIRECT));
   log[log_id].isreturn = (branch_type == BTB_RETURN);

   assert(tail == log_id);
   assert(tail < log_size);
   tail++;
   if (tail == log_size)
      tail = 0;
}

// Restore the RAS after a mispredicted branch.
void ras_t::mispredict(uint64_t log_id,
                       bool iscond, bool taken, uint64_t next_pc) { /* unused */
   // Step 1: Restore the RAS, either totally or partially (depending on the RAS recovery model), to what it was prior to this mispredicted branch.
   flush(log_id);

   // Step 2: If *this* mispredicted branch is a call or return, then incrementally update the RAS for this call or return.
   if (log[log_id].iscall) {
      assert(!iscond);
      assert(!log[log_id].isreturn);
      //push(next_pc);
      tos++;
      if (tos == size)
         tos = 0;
   }
   if (log[log_id].isreturn) {
      assert(!iscond);
      assert(!log[log_id].iscall);
      pop();
   }

   // Step 3: Increment the log tail past this branch.
   assert(tail == log_id);
   assert(tail < log_size);
   tail++;
   if (tail == log_size)
      tail = 0;
}

// Restore the RAS after a full pipeline flush.
void ras_t::flush(uint64_t log_id) {
   switch (recovery_approach) {
   case ras_recover_e::RAS_RECOVER_TOS_POINTER:
      tos = log[log_id].tos_pointer;
      tail = log_id;
      break;

   case ras_recover_e::RAS_RECOVER_TOS_POINTER_AND_CONTENT:
      tos = log[log_id].tos_pointer;
      ras[tos] = log[log_id].tos_content;
      tail = log_id;
      break;

   case ras_recover_e::RAS_RECOVER_WALK:
      do {
         tail = ((tail > 0) ? (tail - 1) : (log_size - 1));
         tos = log[tail].tos_pointer;
         ras[tos] = log[tail].tos_content;
      } while (tail != log_id);
      break;

   default:
      assert(0);
      break;
   }

   assert(tail == log_id);
}

void ras_t::commit(uint64_t log_id,
                   uint64_t pc,
                   uint64_t branch_in_bundle,
                   bool taken,
                   uint64_t next_pc) {
}
