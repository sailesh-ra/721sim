class gshare_index_t {
private:
   // Global branch history register.
   uint64_t bhr;     // current state of global branch history register
   uint64_t bhr_msb; // used to set the msb of the bhr

   // Parameters for index generation.
   uint64_t pc_mask;
   uint64_t bhr_shamt;

   // User can query what its predictor size should be.
   uint64_t size;

public:
   gshare_index_t(uint64_t pc_length, uint64_t bhr_length);

   ~gshare_index_t();

   uint64_t table_size();

   // Functions to generate gshare index.
   uint64_t index(uint64_t pc);                      // Uses the speculative BHR within this class.
   uint64_t index(uint64_t pc, uint64_t commit_bhr); // Uses a previously recorded BHR for predictor updates.

   // Function to update bhr.
   void update_bhr(bool taken);

   // Function to update a user-provided bhr.
   uint64_t update_my_bhr(uint64_t my_bhr, bool taken);

   // Functions to get and set the bhr, e.g., for checkpoint/restore purposes.
   uint64_t get_bhr();
   void set_bhr(uint64_t bhr);
};


class gshare_log_t {
public:
   uint64_t precise_bhr; // Precise BHR (all prior branches included).  Restore this BHR after a misprediction or flush.
   uint64_t fetch_bhr;   // BHR prior to the fetch bundle containing this branch.
                         // This is the BHR that was used for indexing the table for this prediction.  Thus, also use this BHR for training the prediction table.
};


class gshare_t : public BPinterface_t {
private:
   bool condbp;          // The gshare branch predictor type.  True: conditional branch predictor.  False: indirect target predictor.
   uint64_t width;       // Number of conditional branch predictions per cycle (used only for conditional branch predictor type).
   gshare_index_t index; // The gshare index.
   uint64_t *table;      // The prediction table.
   gshare_log_t *log;    // The branch log.

   // Special register containing the BHR prior to the fetch2 bundle.
   // It is used in the FETCH2 stage to either restore the predictor's BHR in the case of a misfetch or log the fetch_bhr.
   uint64_t fetch2_bhr;

   // Temp for logging the precise BHR.
   uint64_t log_precise_bhr;

public:
   gshare_t(bool condbp, uint64_t width, uint64_t pc_length, uint64_t bhr_length, uint64_t bq_size);
   ~gshare_t();

   ///////////////////////////////////////////////
   // Called by the Fetch Unit's FETCH1 stage.
   ///////////////////////////////////////////////

   // Get "m" cond. branch predictions or 1 indirect target prediction.
   // "pc" is the start PC of the fetch bundle.
   uint64_t predict(uint64_t pc);

   // Save the BHR in the fetch2_bhr register.
   void save_fetch2_context();

   // Speculatively update the BHR with "num" predictions from "predictions".
   void spec_update(uint64_t predictions, uint64_t num,                 /* used: for speculatively updating branch history */
                    uint64_t pc, uint64_t next_pc,                      /* unused: for speculatively updating path history */
                    bool pop_ras, bool push_ras, uint64_t push_ras_pc); /* unused: for speculative RAS actions */

   ///////////////////////////////////////////////
   // Called by the Fetch Unit's FETCH2 stage.
   ///////////////////////////////////////////////

   // Misfetch: restore the BHR to fetch2_bhr.
   void restore_fetch2_context();

   // Begin logging.  Simply sets log_precise_bhr to fetch2_bhr.
   void log_begin();

   // Log a branch: record the precise_bhr (log_precise_bhr) and fetch_bhr (fetch2_bhr) w.r.t. the branch.
   // If it is a conditional branch, also update the ongoing log_precise_bhr based on "taken".
   void log_branch(uint64_t log_id,
                   btb_branch_type_e branch_type,
                   bool taken,
                   uint64_t pc, uint64_t next_pc); /* unused */

   ///////////////////////////////////////////////
   // Called when a branch resolves.
   ///////////////////////////////////////////////

   // Restore the BHR to the indicated checkpointed BHR.
   // If the mispredicted branch is a conditional branch ("iscond"), then also update the BHR with the provided outcome ("taken").
   //
   // The corrected target ("next_pc") is not used; it may apply to path-history based predictors.
   void mispredict(uint64_t log_id, bool iscond, bool taken, uint64_t next_pc);

   ///////////////////////////////////////////////
   // Called when there is a full pipeline flush.
   ///////////////////////////////////////////////

   void flush(uint64_t log_id);

   ///////////////////////////////////////////////
   // Called when a branch retires.
   ///////////////////////////////////////////////

   // Train the prediction table for the indicated branch.
   void commit(uint64_t log_id,
               // Original fetch bundle PC:
               uint64_t pc,
               // If this gshare is for conditional branches:
               uint64_t branch_in_bundle,
               bool taken,
               // If this gshare is for indirect branches:
               uint64_t next_pc);
};
