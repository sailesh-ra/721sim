
class bq_entry_t {
public:
   // The type of branch.
   btb_branch_type_e branch_type;

   // Information about the fetch bundle that contains this branch.
   uint64_t fetch_pc;             // Start PC of the fetch bundle that contains this branch.
   uint64_t fetch_cbID_in_bundle; // If this is a conditional branch (cb), this is its ID in the fetch bundle: 0 (first cb), 1 (second cb), etc.
                                  // It is not relevant for unconditional branches.

   // Branch prediction/outcome.  It is a prediction until confirmed/disconfirmed, and then it is an outcome.
   bool taken; // For conditional branches.
   uint64_t next_pc;

   // This flag indicates whether or not the branch was mispredicted.  It is needed for measuring mispredictions at retirement.
   bool misp;
};


class bq_t {
private:
   uint64_t size;
   uint64_t head;
   uint64_t tail;
   bool head_phase;
   bool tail_phase;

public:
   // This is the branch queue.  It holds information for all outstanding branch predictions.
   // The user of this class can directly access branch queue entries, given indices from this class.
   bq_entry_t *bq;

   bq_t(uint64_t size);
   ~bq_t();
   void push(uint64_t &pred_tag, bool &pred_tag_phase);
   void pop(uint64_t &pred_tag, bool &pred_tag_phase);
   void rollback(uint64_t pred_tag, bool pred_tag_phase, bool do_checks);
   void mark(uint64_t &pred_tag, bool &pred_tag_phase);
   bool empty() { return (head == tail) && (head_phase == tail_phase); };
   bool full() { return (head == tail) && (head_phase != tail_phase); };
   uint64_t flush();
};
