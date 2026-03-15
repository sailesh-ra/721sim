#include <cinttypes>
#include "decode.h"
#include "fetchunit_types.h"
#include "BPinterface.h"
#include "gshare.h"

/////////////////////////////////////
// gshare_index_t member functions
/////////////////////////////////////

gshare_index_t::gshare_index_t(uint64_t pc_length, uint64_t bhr_length) {
   // Global branch history register.
   bhr = 0;
   bhr_msb = ((1 << bhr_length) >> 1);

   // Parameters for index generation.
   pc_mask = ((1 << pc_length) - 1);
   if (pc_length > bhr_length) {
      bhr_shamt = (pc_length - bhr_length);
      size = (1 << pc_length);
   }
   else {
      bhr_shamt = 0;
      size = (1 << bhr_length);
   }
}

gshare_index_t::~gshare_index_t() {
}

uint64_t gshare_index_t::table_size() {
   return (size);
}

// Function to generate gshare index, using the speculative BHR within this class.
uint64_t gshare_index_t::index(uint64_t pc) {
   return (((pc >> 2) & pc_mask) ^ (bhr << bhr_shamt));
}

// Function to generate gshare index, using a previously recorded BHR for predictor updates.
uint64_t gshare_index_t::index(uint64_t pc, uint64_t commit_bhr) {
   return (((pc >> 2) & pc_mask) ^ (commit_bhr << bhr_shamt));
}

// Function to update bhr.
void gshare_index_t::update_bhr(bool taken) {
   bhr = ((bhr >> 1) | (taken ? bhr_msb : 0));
}

// Function to update a user-provided bhr.
uint64_t gshare_index_t::update_my_bhr(uint64_t my_bhr, bool taken) {
   return (((my_bhr >> 1) | (taken ? bhr_msb : 0)));
}


// Functions to get and set the bhr, e.g., for checkpoint/restore purposes.

uint64_t gshare_index_t::get_bhr() {
   return (bhr);
}

void gshare_index_t::set_bhr(uint64_t bhr) {
   this->bhr = bhr;
}

/////////////////////////////////////
// gshare_t member functions
/////////////////////////////////////

gshare_t::gshare_t(bool condbp, uint64_t width, uint64_t pc_length, uint64_t bhr_length, uint64_t bq_size) : condbp(condbp),                // Set the type of the predictor (conditional or indirect).
                                                                                                             width(width),                  // Set the conditional branch predictions per cycle.
                                                                                                             index(pc_length, bhr_length) { // Configure parameters of the gshare index.

   // Memory-allocate the prediction table.
   table = new uint64_t[index.table_size()];

   // If this is a conditional branch predictor, initialize its counters.
   if (condbp) {
      for (uint64_t i = 0; i < index.table_size(); i++)
         table[i] = 0xaaaaaaaa; // Initialize counters to weakly-taken.
   }
   else {
      for (uint64_t i = 0; i < index.table_size(); i++)
         table[i] = 0;
   }

   // Memory-allocate the branch log.
   log = new gshare_log_t[bq_size];
   for (size_t i = 0; i < bq_size; i++) {
      log[i].fetch_bhr = 0;
      log[i].precise_bhr = 0;
   }
}

gshare_t::~gshare_t() {
   delete[] table;
   delete[] log;
}

uint64_t gshare_t::predict(uint64_t pc) {
   if (condbp) {
      uint64_t counters = table[index.index(pc)];
      uint64_t predictions = 0;
      for (uint64_t i = 0; i < width; i++) {
         if ((counters & 3) >= 2)
            predictions |= (1ULL << i);
         counters = (counters >> 2);
      }
      return (predictions);
   }
   else {
      return (table[index.index(pc)]);
   }
}

void gshare_t::save_fetch2_context() {
   fetch2_bhr = index.get_bhr();
}

void gshare_t::spec_update(uint64_t predictions, uint64_t num,                  /* used: for speculatively updating branch history */
                           uint64_t pc, uint64_t next_pc,                       /* unused: for speculatively updating path history */
                           bool pop_ras, bool push_ras, uint64_t push_ras_pc) { /* unused: for speculative RAS actions */
   bool taken;
   for (uint64_t i = 0; i < num; i++) {
      // The least significant bit of "predictions" corresponds to the next taken/not-taken prediction (because we shift it right, subsequently).
      // From this bit, set the taken flag, accordingly.
      taken = ((predictions & 1) == 1);

      // Shift out the used-up prediction bit, to set up the next conditional branch.
      predictions = (predictions >> 1);

      // Update the BHR.
      index.update_bhr(taken);
   }
}

void gshare_t::restore_fetch2_context() {
   index.set_bhr(fetch2_bhr);
}

void gshare_t::log_begin() {
   log_precise_bhr = fetch2_bhr;
}

void gshare_t::log_branch(uint64_t log_id,
                          btb_branch_type_e branch_type,
                          bool taken,
                          uint64_t pc, uint64_t next_pc) { /* unused */
   log[log_id].precise_bhr = log_precise_bhr;
   log[log_id].fetch_bhr = fetch2_bhr;
   if (branch_type == BTB_BRANCH)
      log_precise_bhr = index.update_my_bhr(log_precise_bhr, taken);
}

void gshare_t::mispredict(uint64_t log_id, bool iscond, bool taken, uint64_t next_pc /*unused*/) {
   index.set_bhr(log[log_id].precise_bhr);
   if (iscond)
      index.update_bhr(taken);
}

void gshare_t::flush(uint64_t log_id) {
   index.set_bhr(log[log_id].precise_bhr);
}

void gshare_t::commit(uint64_t log_id,
                      // Original fetch bundle PC:
                      uint64_t pc,
                      // If this gshare is for conditional branches:
                      uint64_t branch_in_bundle,
                      bool taken,
                      // If this gshare is for indirect branches:
                      uint64_t next_pc) {
   if (condbp) {
      uint64_t *counters;
      uint64_t shamt;
      uint64_t mask;
      uint64_t ctr;

      // Re-reference the conditional branch predictor, using the same context that was used by
      // the fetch bundle that this branch was a part of.
      // Using this original context, we re-reference the same "m" counters from the conditional branch predictor.
      // "m" two-bit counters are packed into a uint64_t.
      counters = &(table[index.index(pc, log[log_id].fetch_bhr)]);

      // Prepare for reading and writing the 2-bit counter that was used to predict this branch.
      // We need a shift-amount ("shamt") and a mask ("mask") that can be used to read/write just that counter.
      // "shamt" = the branch's position in the entry times 2, for 2-bit counters.
      // "mask" = (3 << shamt).
      shamt = (branch_in_bundle << 1);
      mask = (3 << shamt);

      // Extract a local copy of the 2-bit counter that was used to predict this branch.
      ctr = (((*counters) & mask) >> shamt);

      // Increment or decrement the local copy of the 2-bit counter, based on the branch's outcome.
      if (taken) {
         if (ctr < 3)
            ctr++;
      }
      else {
         if (ctr > 0)
            ctr--;
      }

      // Write the modified local copy of the 2-bit counter back into the predictor's entry.
      *counters = (((*counters) & (~mask)) | (ctr << shamt));
   }
   else {
      // Re-reference the indirect branch predictor, using the same context that was used by
      // the fetch bundle that this branch was a part of.
      table[index.index(pc, log[log_id].fetch_bhr)] = next_pc;
   }
}
