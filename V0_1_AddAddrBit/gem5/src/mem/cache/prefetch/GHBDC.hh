/* MJL_Begin */

/**
 * @file
 * Describes a global history buffer based delta correlation prefetcher.
 */

#ifndef __MEM_CACHE_PREFETCH_GHBDC_HH__
#define __MEM_CACHE_PREFETCH_GHBDC_HH__

#include <list>
#include <unordered_map>

#include "mem/cache/prefetch/queued.hh"
#include "params/GHBDC.hh"

class GHBDCPrefetcher : public QueuedPrefetcher
{
  protected:
    const int index_table_size;
    const int GHB_size;
    const int degree;

    struct GHBEntry
    {
        Addr tag;
        Addr addr;
        GHBEntry* prevEntry;
        bool isSecure;
        MemCmd::MJL_DirAttribute MJL_lastDir;

        GHBEntry() : tag(0), addr(0), prevEntry(nullptr), isSecure(false)
        { }
    };

    std::unordered_map<Addr, GHBEntry *> IndexTable;

    std::list<GHBEntry *> GHB;

    class PCTable
    {
      public:
        PCTable(int assoc, int sets, const std::string name) :
            pcTableAssoc(assoc), pcTableSets(sets), _name(name) {}
        StrideEntry** operator[] (int context) {
            auto it = entries.find(context);
            if (it != entries.end())
                return it->second;

            return allocateNewContext(context);
        }

        ~PCTable();
      private:
        const std::string name() {return _name; }
        const int pcTableAssoc;
        const int pcTableSets;
        const std::string _name;
        std::unordered_map<int, StrideEntry**> entries;

        StrideEntry** allocateNewContext(int context);
    };
    PCTable pcTable;

    bool pcTableHit(Addr pc, bool is_secure, int master_id, StrideEntry* &entry);
    StrideEntry* pcTableVictim(Addr pc, int master_id);

    Addr pcHash(Addr pc) const;
  public:

    StridePrefetcher(const StridePrefetcherParams *p);

    void calculatePrefetch(const PacketPtr &pkt,
                           std::vector<AddrPriority> &addresses);

    void MJL_calculatePrefetch(const PacketPtr &pkt,
                           std::vector<AddrPriority> &addresses, 
                           MemCmd::MJL_DirAttribute &MJL_cmdDir);
};

#endif // __MEM_CACHE_PREFETCH_GHBDC_HH__

/* MJL_End */
