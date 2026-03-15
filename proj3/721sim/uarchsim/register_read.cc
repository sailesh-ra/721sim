#include "pipeline.h"


void pipeline_t::register_read(unsigned int lane_number) {
   unsigned int index;

   // Check if there is an instruction in the Register Read Stage of the specified Execution Lane.
   if (Execution_Lanes[lane_number].rr.valid) {

      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Get the instruction's index into PAY.
      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      index = Execution_Lanes[lane_number].rr.index;

      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      // FIX_ME #11a
      // If the instruction has a destination register AND its latency is 1-cycle AND it is not a load AND it is not an AMO instruction:
      // (1) Broadcast its destination tag to the IQ to wakeup its dependent instructions.
      // (2) Set the corresponding ready bit in the Physical Register File Ready Bit Array.
      //
      // Multi-cycle instructions perform wakeup at an equivalent stage (see FIX_ME #11b).
      //
      // Load instructions conservatively delay broadcasting their destination tags until
      // their data are available, because they may stall in the Execute Stage. I.e., in the current
      // simulator implementation, loads do NOT speculatively wakeup their dependent instructions.
      //
      // Atomic memory instructions (AMO) that perform compute (load-compute-store) and have
      // a destination register, must not perform wakeup at all. They execute at retire, followed
      // by a full squash.
      //
      // Tips:
      // 1. At this point of the code, 'index' is the instruction's index into PAY.buf[] (payload).
      // 2. The easiest way to tell if this instruction is a load or not, is to test the instruction's
      //    flags (in its payload) via the IS_LOAD() macro (see pipeline.h).
      //    Similarly, the IS_AMO() macro is for testing AMO status.
      // 3. The instruction's latency is provided for you as "lat" below: it's the number of
      //    Execute Stages within the lane (lane::ex_depth).
      //    Single-cycle producers must perform wakeup here.
      //    Multi-cycle producers must perform wakeup at an equivalent stage -- in their
      //    second-to-last cycle of execute, which is at execute.cc, pipeline_t::execute(), FIX_ME #11b.
      // 4. If the instruction has a destination register AND its latency is 1-cycle AND it is not a load AND it is not an AMO instruction, then:
      //    a. Wakeup dependents in the IQ using its wakeup() port (see issue_queue.h for arguments to the wakeup port;
      //       note that you are only coding a register-dependency wakeup, so pass in "true" (no quotes) for the second argument).
      //    b. Set the destination register's ready bit.
      //////////////////////////////////////////////////////////////////////////////////////////////////////////

      unsigned int lat = Execution_Lanes[lane_number].ex_depth;

      // FIX_ME #11a BEGIN
      // FIX_ME #11a END

      // If the issued instruction is a store:
      // (1) Wakeup any dependent store and/or load(s).
      // (2) Remove the store's inum from its LFST entry if still there.
      if (IS_STORE(PAY.buf[index].flags)) {
         IQ.wakeup(PAY.buf[index].AL_index /* store's inum */, false /* memory dependency */);
         if (STORE_SETS || ORACLE_MDP) {
            bool oracle_addr_valid = false;
            uint64_t oracle_addr = 0xDEADBEEF;
            if (PAY.buf[index].good_instruction) {
               db_t *actual = get_pipe()->peek(PAY.buf[index].db_index);
               oracle_addr_valid = actual->a_addr.valid;
               oracle_addr = actual->a_addr.addr;
            }
            MDP->invalidateLFST(PAY.buf[index].pc /* ssid = SSIT[pc], index LFST using ssid */, PAY.buf[index].AL_index /* store's inum */,
                                /* add-ons for oracle */ oracle_addr_valid, oracle_addr, PAY.buf[index].size);
         }
      }

      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      // FIX_ME #12
      // Read source register(s) from the Physical Register File.
      //
      // Tips:
      // 1. At this point of the code, 'index' is the instruction's index into PAY.buf[] (payload).
      // 2. If the instruction has a first source register (A), then read its doubleword value from
      //    the Physical Register File.
      // 3. If the instruction has a second source register (B), follow the same procedure for it.
      // 4. If the instruction has a third source register (D), follow the same procedure for it.
      // 5. Be sure to record any doubleword value(s) in the instruction's payload, for use in the
      //    subsequent Execute Stage. The values in the payload use a union type (can be referenced
      //    as either a single doubleword or as two words separately); see the comments in file
      //    payload.h regarding referencing a value as a single doubleword.
      //////////////////////////////////////////////////////////////////////////////////////////////////////////

      // FIX_ME #12 BEGIN
      // FIX_ME #12 END

      //////////////////////////////////////////////////////////////////////////////////////////////////////////
      // Advance the instruction to the Execution Stage.
      //////////////////////////////////////////////////////////////////////////////////////////////////////////

      // There must be space in the Execute Stage because Execution Lanes are free-flowing.
      assert(!Execution_Lanes[lane_number].ex[0].valid);

      // Copy instruction to Execute Stage.
      Execution_Lanes[lane_number].ex[0].valid = true;
      Execution_Lanes[lane_number].ex[0].index = Execution_Lanes[lane_number].rr.index;
      Execution_Lanes[lane_number].ex[0].branch_mask = Execution_Lanes[lane_number].rr.branch_mask;

      // Remove instruction from Register Read Stage.
      Execution_Lanes[lane_number].rr.valid = false;
   }
}
