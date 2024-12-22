#ifndef RRIP_REPL_H_
#define RRIP_REPL_H_

#include "repl_policies.h"

// Static RRIP
class SRRIPReplPolicy : public ReplPolicy {
    protected:
        // add class member variables here
        int32_t* rpvArray; // rrpv for each cache line
        uint32_t numLines;
        const int32_t rpvMax;

    public:
        // add member methods here, refer to repl_policies.h
        explicit SRRIPReplPolicy(uint32_t _numLines, int32_t _rpvMax) : numLines(_numLines), rpvMax(_rpvMax) {
            rpvArray = gm_calloc<int32_t>(numLines);
            for (uint32_t i = 0; i < numLines; i++)
                rpvArray[i] = rpvMax;
        }

        ~SRRIPReplPolicy() {
            gm_free(rpvArray);
        }

        void update(uint32_t id, const MemReq* req) override {
            rpvArray[id] = 0;
        }

        void replaced(uint32_t id) override {
            rpvArray[id] = rpvMax - 1;
        }
        
       template <typename C> inline uint32_t rank(const MemReq* req, C candidate) {
            //find candidate with max rrpv
            // On a cache miss, the RRIP victim selection policy 
            // selects the victim block by finding the first block that 
            // is predicted to be re-referenced in the distant future (i.e., the block whose RRPV is 2M–1)
            for (typename C::iterator i = candidate.begin(); i != candidate.end(); i.inc())
                if (rpvArray[*i] == rpvMax)
                    return *i;

            // if no max found, then increment
            // In the event that RRIP is unable to find a block
            // with a distant re-reference interval,
            // RRIP updates the re-reference predictions by 
            // incrementing the RRPVs of all blocks in the cache set 
            // and repeats the search until a block with a distant
            // re-reference interval is found.
            for (typename C::iterator i = candidate.begin(); i != candidate.end(); i.inc())
                rpvArray[*i] = std::min(rpvArray[*i] + 1, rpvMax);
            
            // try again and find the candidate with rpvMax
            // pdating RRPVs at victim selection time allows RRIP to adapt to 
            // changes in the application working set by removing stale blocks from the cache
            for (typename C::iterator i = candidate.begin(); i != candidate.end(); i.inc())
                if (rpvArray[*i] == rpvMax)
                    return *i;

            //we did not find candidate
            return *candidate.begin();
        }

        DECL_RANK_BINDINGS;
};
#endif // RRIP_REPL_H_
