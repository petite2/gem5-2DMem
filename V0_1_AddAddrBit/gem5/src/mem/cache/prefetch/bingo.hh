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
#include "params/BingoPrefetcher.hh"

#include <bits/stdc++.h>
#include <vector>

class Table {
  public:
    Table(int width, int height);

    void set_row(int row, const std::vector<std::string> &data, int start_col = 0);

    void set_col(int col, const std::vector<std::string> &data, int start_row = 0);

    void set_cell(int row, int col, std::string data);

    void set_cell(int row, int col, double data);

    void set_cell(int row, int col, int64_t data);

    void set_cell(int row, int col, int data);

    void set_cell(int row, int col, uint64_t data);

    std::string to_string();

    std::string data_row(int row, const std::vector<int> &widths);

    static std::string top_line(const std::vector<int> &widths);

    static std::string mid_line(const std::vector<int> &widths);

    static std::string bot_line(const std::vector<int> &widths);

    static std::string line(const std::vector<int> &widths, std::string left, std::string mid, std::string right);

  private:
    unsigned width;
    unsigned height;
    std::vector<std::vector<std::string>> cells;
    std::ostringstream oss;
};

template <class T> class InfiniteCache {
  public:
    class Entry {
      public:
        uint64_t key;
        uint64_t index;
        uint64_t tag;
        bool valid;
        T data;
    };

    Entry *erase(uint64_t key) {
        Entry *entry = this->find(key);
        if (!entry)
            return nullptr;
        entry->valid = false;
        this->last_erased_entry = *entry;
        int num_erased = this->entries.erase(key);
        assert(num_erased == 1);
        return &this->last_erased_entry;
    }

    /**
     * @return The old state of the entry that was written to.
     */
    Entry insert(uint64_t key, const T &data) {
        Entry *entry = this->find(key);
        if (entry != nullptr) {
            Entry old_entry = *entry;
            entry->data = data;
            return old_entry;
        }
        entries[key] = {key, 0, key, true, data};
        return {};
    }

    Entry *find(uint64_t key) {
        auto it = this->entries.find(key);
        if (it == this->entries.end())
            return nullptr;
        Entry &entry = (*it).second;
        assert(entry.tag == key && entry.valid);
        return &entry;
    }

    /**
     * For debugging purposes.
     */
    std::string log(std::vector<std::string> headers, std::function<void(Entry &, Table &, int)> write_data) {
        Table table(headers.size(), entries.size() + 1);
        table.set_row(0, headers);
        unsigned i = 0;
        for (auto &x : this->entries)
            write_data(x.second, table, ++i);
        return table.to_string();
    }

    void set_debug_level(int debug_level) { this->debug_level = debug_level; }

  protected:
    Entry last_erased_entry;
    std::unordered_map<uint64_t, Entry> entries;
    int debug_level = 0;
};

template <class T> class SetAssociativeCache {
  public:
    class Entry {
      public:
        uint64_t key;
        uint64_t index;
        uint64_t tag;
        bool valid;
        T data;
    };

    SetAssociativeCache(int size, int num_ways)
        : size(size), num_ways(num_ways), num_sets(size / num_ways), entries(num_sets, std::vector<Entry>(num_ways)),
          cams(num_sets) {
        assert(size % num_ways == 0);
        for (int i = 0; i < num_sets; i += 1)
            for (int j = 0; j < num_ways; j += 1)
                entries[i][j].valid = false;
    }

    Entry *erase(uint64_t key) {
        Entry *entry = this->find(key);
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        auto &cam = cams[index];
        int num_erased = cam.erase(tag);
        if (entry)
            entry->valid = false;
        assert(entry ? num_erased == 1 : num_erased == 0);
        return entry;
    }

    /**
     * @return The old state of the entry that was written to.
     */
    Entry insert(uint64_t key, const T &data) {
        Entry *entry = this->find(key);
        if (entry != nullptr) {
            Entry old_entry = *entry;
            entry->data = data;
            return old_entry;
        }
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        std::vector<Entry> &set = this->entries[index];
        int victim_way = -1;
        for (int i = 0; i < this->num_ways; i += 1)
            if (!set[i].valid) {
                victim_way = i;
                break;
            }
        if (victim_way == -1) {
            victim_way = this->select_victim(index);
        }
        Entry &victim = set[victim_way];
        Entry old_entry = victim;
        victim = {key, index, tag, true, data};
        auto &cam = cams[index];
        if (old_entry.valid) {
            int num_erased = cam.erase(old_entry.tag);
            assert(num_erased == 1);
        }
        cam[tag] = victim_way;
        return old_entry;
    }

    Entry *find(uint64_t key) {
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        auto &cam = cams[index];
        if (cam.find(tag) == cam.end())
            return nullptr;
        int way = cam[tag];
        Entry &entry = this->entries[index][way];
        assert(entry.tag == tag && entry.valid);
        return &entry;
    }

    /**
     * For debugging purposes.
     */
    std::string log(std::vector<std::string> headers, std::function<void(Entry &, Table &, int)> write_data) {
        std::vector<Entry> valid_entries = this->get_valid_entries();
        Table table(headers.size(), valid_entries.size() + 1);
        table.set_row(0, headers);
        for (unsigned i = 0; i < valid_entries.size(); i += 1)
            write_data(valid_entries[i], table, i + 1);
        return table.to_string();
    }

    void set_debug_level(int debug_level) { this->debug_level = debug_level; }

  protected:
    /**
     * @return The way of the selected victim.
     */
    virtual int select_victim(uint64_t index) {
        /* random eviction policy if not overriden */
        return rand() % this->num_ways;
    }

    std::vector<Entry> get_valid_entries() {
        std::vector<Entry> valid_entries;
        for (int i = 0; i < num_sets; i += 1)
            for (int j = 0; j < num_ways; j += 1)
                if (entries[i][j].valid)
                    valid_entries.push_back(entries[i][j]);
        return valid_entries;
    }

    int size;
    int num_ways;
    int num_sets;
    std::vector<std::vector<Entry>> entries;
    std::vector<std::unordered_map<uint64_t, int>> cams;
    int debug_level = 0;
};

template <class T> class LRUSetAssociativeCache : public SetAssociativeCache<T> {
    typedef SetAssociativeCache<T> Super;

  public:
    LRUSetAssociativeCache(int size, int num_ways)
        : Super(size, num_ways), lru(this->num_sets, std::vector<uint64_t>(num_ways)) {}

    void set_mru(uint64_t key) { *this->get_lru(key) = this->t++; }

    void set_lru(uint64_t key) { *this->get_lru(key) = 0; }

  protected:
    /* @override */
    int select_victim(uint64_t index) {
        std::vector<uint64_t> &lru_set = this->lru[index];
        return min_element(lru_set.begin(), lru_set.end()) - lru_set.begin();
    }

    uint64_t *get_lru(uint64_t key) {
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        int way = this->cams[index][tag];
        return &this->lru[index][way];
    }

    std::vector<std::vector<uint64_t>> lru;
    uint64_t t = 1;
};

template <class T> class NMRUSetAssociativeCache : public SetAssociativeCache<T> {
    typedef SetAssociativeCache<T> Super;

  public:
    NMRUSetAssociativeCache(int size, int num_ways) : Super(size, num_ways), mru(this->num_sets) {}

    void set_mru(uint64_t key) {
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        int way = this->cams[index][tag];
        this->mru[index] = way;
    }

  protected:
    /* @override */
    int select_victim(uint64_t index) {
        int way = rand() % (this->num_ways - 1);
        if (way >= mru[index])
            way += 1;
        return way;
    }

    std::vector<int> mru;
};

template <class T> class LRUFullyAssociativeCache : public LRUSetAssociativeCache<T> {
    typedef LRUSetAssociativeCache<T> Super;

  public:
    LRUFullyAssociativeCache(int size) : Super(size, size) {}
};

template <class T> class NMRUFullyAssociativeCache : public NMRUSetAssociativeCache<T> {
    typedef NMRUSetAssociativeCache<T> Super;

  public:
    NMRUFullyAssociativeCache(int size) : Super(size, size) {}
};

template <class T> class DirectMappedCache : public SetAssociativeCache<T> {
    typedef SetAssociativeCache<T> Super;

  public:
    DirectMappedCache(int size) : Super(size, 1) {}
};

/** End Of Cache Framework **/

class FilterTableData {
  public:
    uint64_t pc;
    int offset;
};

class FilterTable : public LRUFullyAssociativeCache<FilterTableData> {
    typedef LRUFullyAssociativeCache<FilterTableData> Super;

  public:
    FilterTable(int size);

    Entry *find(uint64_t region_number);

    void insert(uint64_t region_number, uint64_t pc, int offset);
};

class AccumulationTableData {
  public:
    uint64_t pc;
    int offset;
    std::vector<bool> pattern;
};

class AccumulationTable : public LRUFullyAssociativeCache<AccumulationTableData> {
    typedef LRUFullyAssociativeCache<AccumulationTableData> Super;

  public:
    AccumulationTable(int size, int pattern_len);

    /**
     * @return A return value of false means that the tag wasn't found in the table and true means success.
     */
    bool set_pattern(uint64_t region_number, int offset);

    Entry insert(FilterTable::Entry &entry);

  private:
    int pattern_len;
};

template <class T> std::vector<T> my_rotate(const std::vector<T> &x, int n) {
    std::vector<T> y;
    int len = x.size();
    n = n % len;
    for (int i = 0; i < len; i += 1)
        y.push_back(x[(i - n + len) % len]);
    return y;
}

#define THRESH 0.20

class PatternHistoryTableData {
  public:
    std::vector<SC2> pattern;
};

class PatternHistoryTable : LRUSetAssociativeCache<PatternHistoryTableData> {
    typedef LRUSetAssociativeCache<PatternHistoryTableData> Super;

  public:
    PatternHistoryTable(
        int size, int pattern_len, int min_addr_width, int max_addr_width, int pc_width, int num_ways = 16);

    /* address is actually block number */
    void insert(uint64_t pc, uint64_t address, std::vector<bool> pattern);

    /**
     * @return An un-rotated pattern if match was found, otherwise an empty vector.
     * Finds best match and in case of ties, uses the MRU entry.
     */
    std::vector<bool> find(uint64_t pc, uint64_t address);

  private:
    uint64_t build_key(uint64_t pc, uint64_t address);

    std::vector<bool> vote(const std::vector<std::vector<SC2>> &x, float thresh = THRESH);

    int pattern_len, index_len;
    int min_addr_width, max_addr_width, pc_width;
};


class BingoPrefetcher : public QueuedPrefetcher
{
  private:
    int pattern_len;
    FilterTable filter_table;
    AccumulationTable accumulation_table;
    PatternHistoryTable pht;
    int debug_level = 0;
    const bool useMasterId;

    std::vector<bool> find_in_pht(uint64_t pc, uint64_t address);
    void insert_in_pht(const AccumulationTable::Entry &entry);

  public:

    BingoPrefetcher(const BingoPrefetcherParams *p);

    void calculatePrefetch(const PacketPtr &pkt,
                           std::vector<AddrPriority> &addresses);

    void MJL_calculatePrefetch(const PacketPtr &pkt,
                           std::vector<AddrPriority> &addresses, 
                           MemCmd::MJL_DirAttribute &MJL_cmdDir);
    void MJL_eviction(Addr addr) override;
    
};

#endif // __MEM_CACHE_PREFETCH_GHBDC_HH__

/* MJL_End */
