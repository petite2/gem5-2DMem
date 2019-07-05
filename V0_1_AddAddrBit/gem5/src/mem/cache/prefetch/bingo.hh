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
    using namespace std;
  public:
    Table(int width, int height) : width(width), height(height), cells(height, vector<string>(width)) {}

    void set_row(int row, const vector<string> &data, int start_col = 0) {
        assert(data.size() + start_col == this->width);
        for (unsigned col = start_col; col < this->width; col += 1)
            this->set_cell(row, col, data[col]);
    }

    void set_col(int col, const vector<string> &data, int start_row = 0) {
        assert(data.size() + start_row == this->height);
        for (unsigned row = start_row; row < this->height; row += 1)
            this->set_cell(row, col, data[row]);
    }

    void set_cell(int row, int col, string data) {
        assert(0 <= row && row < (int)this->height);
        assert(0 <= col && col < (int)this->width);
        this->cells[row][col] = data;
    }

    void set_cell(int row, int col, double data) {
        this->oss.str("");
        this->oss << setw(11) << fixed << setprecision(8) << data;
        this->set_cell(row, col, this->oss.str());
    }

    void set_cell(int row, int col, int64_t data) {
        this->oss.str("");
        this->oss << setw(11) << std::left << data;
        this->set_cell(row, col, this->oss.str());
    }

    void set_cell(int row, int col, int data) { this->set_cell(row, col, (int64_t)data); }

    void set_cell(int row, int col, uint64_t data) { this->set_cell(row, col, (int64_t)data); }

    string to_string() {
        vector<int> widths;
        for (unsigned i = 0; i < this->width; i += 1) {
            int max_width = 0;
            for (unsigned j = 0; j < this->height; j += 1)
                max_width = max(max_width, (int)this->cells[j][i].size());
            widths.push_back(max_width + 2);
        }
        string out;
        out += Table::top_line(widths);
        out += this->data_row(0, widths);
        for (unsigned i = 1; i < this->height; i += 1) {
            out += Table::mid_line(widths);
            out += this->data_row(i, widths);
        }
        out += Table::bot_line(widths);
        return out;
    }

    string data_row(int row, const vector<int> &widths) {
        string out;
        for (unsigned i = 0; i < this->width; i += 1) {
            string data = this->cells[row][i];
            data.resize(widths[i] - 2, ' ');
            out += " | " + data;
        }
        out += " |\n";
        return out;
    }

    static string top_line(const vector<int> &widths) { return Table::line(widths, "┌", "┬", "┐"); }

    static string mid_line(const vector<int> &widths) { return Table::line(widths, "├", "┼", "┤"); }

    static string bot_line(const vector<int> &widths) { return Table::line(widths, "└", "┴", "┘"); }

    static string line(const vector<int> &widths, string left, string mid, string right) {
        string out = " " + left;
        for (unsigned i = 0; i < widths.size(); i += 1) {
            int w = widths[i];
            for (int j = 0; j < w; j += 1)
                out += "─";
            if (i != widths.size() - 1)
                out += mid;
            else
                out += right;
        }
        return out + "\n";
    }

  private:
    unsigned width;
    unsigned height;
    vector<vector<string>> cells;
    ostringstream oss;
};

template <class T> class InfiniteCache {
    using namespace std;
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
    string log(vector<string> headers, function<void(Entry &, Table &, int)> write_data) {
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
    unordered_map<uint64_t, Entry> entries;
    int debug_level = 0;
};

template <class T> class SetAssociativeCache {
    using namespace std;
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
        : size(size), num_ways(num_ways), num_sets(size / num_ways), entries(num_sets, vector<Entry>(num_ways)),
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
        vector<Entry> &set = this->entries[index];
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
    string log(vector<string> headers, function<void(Entry &, Table &, int)> write_data) {
        vector<Entry> valid_entries = this->get_valid_entries();
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

    vector<Entry> get_valid_entries() {
        vector<Entry> valid_entries;
        for (int i = 0; i < num_sets; i += 1)
            for (int j = 0; j < num_ways; j += 1)
                if (entries[i][j].valid)
                    valid_entries.push_back(entries[i][j]);
        return valid_entries;
    }

    int size;
    int num_ways;
    int num_sets;
    vector<vector<Entry>> entries;
    vector<unordered_map<uint64_t, int>> cams;
    int debug_level = 0;
};

template <class T> class LRUSetAssociativeCache : public SetAssociativeCache<T> {
    using namespace std;
    typedef SetAssociativeCache<T> Super;

  public:
    LRUSetAssociativeCache(int size, int num_ways)
        : Super(size, num_ways), lru(this->num_sets, vector<uint64_t>(num_ways)) {}

    void set_mru(uint64_t key) { *this->get_lru(key) = this->t++; }

    void set_lru(uint64_t key) { *this->get_lru(key) = 0; }

  protected:
    /* @override */
    int select_victim(uint64_t index) {
        vector<uint64_t> &lru_set = this->lru[index];
        return min_element(lru_set.begin(), lru_set.end()) - lru_set.begin();
    }

    uint64_t *get_lru(uint64_t key) {
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        int way = this->cams[index][tag];
        return &this->lru[index][way];
    }

    vector<vector<uint64_t>> lru;
    uint64_t t = 1;
};

template <class T> class NMRUSetAssociativeCache : public SetAssociativeCache<T> {
    using namespace std;
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

    vector<int> mru;
};

template <class T> class LRUFullyAssociativeCache : public LRUSetAssociativeCache<T> {
    using namespace std;
    typedef LRUSetAssociativeCache<T> Super;

  public:
    LRUFullyAssociativeCache(int size) : Super(size, size) {}
};

template <class T> class NMRUFullyAssociativeCache : public NMRUSetAssociativeCache<T> {
    using namespace std;
    typedef NMRUSetAssociativeCache<T> Super;

  public:
    NMRUFullyAssociativeCache(int size) : Super(size, size) {}
};

template <class T> class DirectMappedCache : public SetAssociativeCache<T> {
    using namespace std;
    typedef SetAssociativeCache<T> Super;

  public:
    DirectMappedCache(int size) : Super(size, 1) {}
};

/** End Of Cache Framework **/

class FilterTableData {
    using namespace std;
  public:
    uint64_t pc;
    int offset;
};

class FilterTable : public LRUFullyAssociativeCache<FilterTableData> {
    using namespace std;
    typedef LRUFullyAssociativeCache<FilterTableData> Super;

  public:
    FilterTable(int size) : Super(size) { assert(__builtin_popcount(size) == 1); }

    Entry *find(uint64_t region_number) {
        Entry *entry = Super::find(region_number);
        if (!entry)
            return nullptr;
        this->set_mru(region_number);
        return entry;
    }

    void insert(uint64_t region_number, uint64_t pc, int offset) {
        assert(!this->find(region_number));
        Super::insert(region_number, {pc, offset});
        this->set_mru(region_number);
    }
};

class AccumulationTableData {
    using namespace std;
  public:
    uint64_t pc;
    int offset;
    vector<bool> pattern;
};

class AccumulationTable : public LRUFullyAssociativeCache<AccumulationTableData> {
    using namespace std;
    typedef LRUFullyAssociativeCache<AccumulationTableData> Super;

  public:
    AccumulationTable(int size, int pattern_len) : Super(size), pattern_len(pattern_len) {
        assert(__builtin_popcount(size) == 1);
        assert(__builtin_popcount(pattern_len) == 1);
    }

    /**
     * @return A return value of false means that the tag wasn't found in the table and true means success.
     */
    bool set_pattern(uint64_t region_number, int offset) {
        Entry *entry = Super::find(region_number);
        if (!entry)
            return false;
        entry->data.pattern[offset] = true;
        this->set_mru(region_number);
        return true;
    }

    Entry insert(FilterTable::Entry &entry) {
        assert(!this->find(entry.key));
        vector<bool> pattern(this->pattern_len, false);
        pattern[entry.data.offset] = true;
        Entry old_entry = Super::insert(entry.key, {entry.data.pc, entry.data.offset, pattern});
        this->set_mru(entry.key);
        return old_entry;
    }

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
    using namespace std;
  public:
    vector<SC2> pattern;
};

class PatternHistoryTable : LRUSetAssociativeCache<PatternHistoryTableData> {
    using namespace std;
    typedef LRUSetAssociativeCache<PatternHistoryTableData> Super;

  public:
    PatternHistoryTable(
        int size, int pattern_len, int min_addr_width, int max_addr_width, int pc_width, int num_ways = 16)
        : Super(size, num_ways), pattern_len(pattern_len), min_addr_width(min_addr_width),
          max_addr_width(max_addr_width), pc_width(pc_width) {
        assert(this->pc_width >= 0);
        assert(this->min_addr_width >= 0);
        assert(this->max_addr_width >= 0);
        assert(this->max_addr_width >= this->min_addr_width);
        assert(this->pc_width + this->min_addr_width > 0);
        assert(__builtin_popcount(pattern_len) == 1);
        this->index_len = __builtin_ctz(this->num_sets);
    }

    /* address is actually block number */
    void insert(uint64_t pc, uint64_t address, vector<bool> pattern) {
        assert((int)pattern.size() == this->pattern_len);
        int offset = address % this->pattern_len;
        pattern = my_rotate(pattern, -offset);
        uint64_t key = this->build_key(pc, address);

        Entry *entry = Super::find(key);
        if (!entry) {
            Super::insert(key, {vector<SC2>(this->pattern_len)});
            entry = Super::find(key);
        }
        for (int i = 0; i < this->pattern_len; i += 1)
            entry->data.pattern[i].input(pattern[i]);

        this->set_mru(key);
    }

    /**
     * @return An un-rotated pattern if match was found, otherwise an empty vector.
     * Finds best match and in case of ties, uses the MRU entry.
     */
    vector<bool> find(uint64_t pc, uint64_t address) {
        uint64_t key = this->build_key(pc, address);
        uint64_t index = key % this->num_sets;
        uint64_t tag = key / this->num_sets;
        auto &set = this->entries[index];
        uint64_t min_tag_mask = (1 << (this->pc_width + this->min_addr_width - this->index_len)) - 1;
        uint64_t max_tag_mask = (1 << (this->pc_width + this->max_addr_width - this->index_len)) - 1;
        vector<vector<SC2>> min_matches;
        vector<SC2> pattern;
        for (int i = 0; i < this->num_ways; i += 1) {
            if (!set[i].valid)
                continue;
            bool min_match = ((set[i].tag & min_tag_mask) == (tag & min_tag_mask));
            bool max_match = ((set[i].tag & max_tag_mask) == (tag & max_tag_mask));
            vector<SC2> &cur_pattern = set[i].data.pattern;
            if (max_match) {
                this->set_mru(set[i].key);
                pattern = cur_pattern;
                break;
            }
            if (min_match) {
                min_matches.push_back(cur_pattern);
            }
        }

        vector<bool> ret(this->pattern_len);
        if (pattern.empty()) {
            /* no max match was found, time for a vote! */
            ret = this->vote(min_matches);
        } else {
            for (int i = 0; i < this->pattern_len; i += 1)
                ret[i] = pattern[i].output();
        }

        int offset = address % this->pattern_len;
        ret = my_rotate(ret, +offset);
        return ret;
    }

  private:
    uint64_t build_key(uint64_t pc, uint64_t address) {
        pc &= (1 << this->pc_width) - 1;            /* use [pc_width] bits from pc */
        address &= (1 << this->max_addr_width) - 1; /* use [addr_width] bits from address */
        uint64_t offset = address & ((1 << this->min_addr_width) - 1);
        uint64_t base = (address >> this->min_addr_width);
        /* base + pc + offset */
        uint64_t key = (base << (this->pc_width + this->min_addr_width)) | (pc << this->min_addr_width) | offset;
        /* CRC */
        uint64_t tag = ((pc << this->min_addr_width) | offset);
        do {
            tag >>= this->index_len;
            key ^= tag & ((1 << this->index_len) - 1);
        } while (tag > 0);
        return key;
    }

    vector<bool> vote(const vector<vector<SC2>> &x, float thresh = THRESH) {
        int n = x.size();
        vector<bool> ret(this->pattern_len, false);
        for (int i = 0; i < n; i += 1)
            assert((int)x[i].size() == this->pattern_len);
        for (int i = 0; i < this->pattern_len; i += 1) {
            int cnt = 0;
            for (int j = 0; j < n; j += 1)
                if (x[j][i].output())
                    cnt += 1;
            if (1.0 * cnt / n >= thresh)
                ret[i] = true;
        }
        return ret;
    }

    int pattern_len, index_len;
    int min_addr_width, max_addr_width, pc_width;
};


class BingoPrefetcher : public QueuedPrefetcher
{
    using namespace std;
  private:
    int pattern_len;
    FilterTable filter_table;
    AccumulationTable accumulation_table;
    PatternHistoryTable pht;
    int debug_level = 0;
    const bool useMasterId;

    vector<bool> find_in_pht(uint64_t pc, uint64_t address);
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
