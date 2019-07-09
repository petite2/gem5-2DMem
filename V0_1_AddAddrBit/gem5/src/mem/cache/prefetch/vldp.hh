/*
 * Copyright (c) 2012-2013, 2015 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Ron Dreslinski
 */

/**
 * @file
 * Describes a strided prefetcher.
 */

#ifndef __MEM_CACHE_PREFETCH_VLDP_HH__
#define __MEM_CACHE_PREFETCH_VLDP_HH__

#include <unordered_map>

#include "mem/cache/prefetch/queued.hh"
#include "params/VLDPrefetcher.hh"

#include <bits/stdc++.h>

class VLDPrefetcher : public QueuedPrefetcher
{
  private:
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
            entries[key] = { key, 0, key, true, data };
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
                for (int dir = 0; dir < num_ways; dir += 1)
                    entries[i][dir].valid = false;
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
            victim = { key, index, tag, true, data };
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
                for (int dir = 0; dir < num_ways; dir += 1)
                    if (entries[i][dir].valid)
                        valid_entries.push_back(entries[i][dir]);
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
        uint64_t t = 0;
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
    
    class ShiftRegister {
      public:
        /* the maximum total capacity of this shift register is 64 bits */
        ShiftRegister(unsigned size = 4);
    
        void insert(int x);
    
        /**
         * @return Returns raw buffer data in range [le, ri).
         * Note that more recent data have a smaller index.
         */
        uint64_t get_code(unsigned le, unsigned ri);
    
        /**
         * @return Returns integer value of data at specified index.
         */
        int get_value(int i);
    
      private:
        unsigned size;
        unsigned width;
        uint64_t reg = 0;
    };
    
    class SaturatingCounter {
      public:
        SaturatingCounter(int size = 2) : size(size), max((1 << size) - 1) {}
    
        int inc() {
            this->cnt += 1;
            if (this->cnt > this->max)
                this->cnt = this->max;
            return this->cnt;
        }
    
        int dec() {
            this->cnt -= 1;
            if (this->cnt < 0)
                this->cnt = 0;
            return this->cnt;
        }
    
        int get_cnt() { return this->cnt; }
    
      private:
        int size, max, cnt = 0;
    };
    
    class DeltaHistoryBufferData {
      public:
        uint64_t last_addr;
        ShiftRegister deltas;
        int last_predictor;
        int num_times_used;
        /* Each entry in the DHB contains the following data for a tracked physical page: (i) page number, (ii) page offset
         * of the last address accessed in this page, (iii) sequence of up to 4 recently observed deltas, (iv) the DPT level
         * used for the latest delta prediction, (v) the number of times this page has been used, and (vi) sequence of up to
         * 4 recently prefetched offsets. */
    };
    
    class DeltaHistoryBuffer : public NMRUFullyAssociativeCache<DeltaHistoryBufferData> {
        typedef NMRUFullyAssociativeCache<DeltaHistoryBufferData> Super;
    
      public:
        using Super::Super; /* inherit constructors */
    
        int update(uint64_t page_number, uint64_t page_offset) {
            Entry *entry = this->find(page_number);
            if (entry) {
                this->set_mru(page_number);
                DeltaHistoryBufferData &data = entry->data;
                int delta = page_offset - data.last_addr;
                if (delta == 0)
                    return 0;
                data.deltas.insert(delta);
                data.last_addr = page_offset;
                data.num_times_used += 1;
                return delta;
            }
            else {
                /* If no matching entry is found (DHB miss), then a not-Most Recently Used (nMRU) DHB entry is evicted and
                 * assigned to the new page number (nMRU replacement policy). */
                this->insert(page_number, { page_offset, ShiftRegister(4), 0, 1 });
                this->set_mru(page_number);
                return -1;
            }
        }
    };
    
    class OffsetPredictionTableData {
      public:
        int pred;
        bool accuracy;
    };
    
    class OffsetPredictionTable : public DirectMappedCache<OffsetPredictionTableData> {
        typedef DirectMappedCache<OffsetPredictionTableData> Super;
    
      public:
        using Super::Super; /* inherit constructors */
    
        void update(int first_offset, int delta) {
            /* If the OPT prediction matches the observed delta, the accuracy bit is set to 1, or remains 1 if it was
             * already 1. If the OPT prediction does not match the observed delta, the accuracy bit is set to 0. If the
             * accuracy bit was already 0, the old predicted delta is replaced with the new observed delta, and the accuracy
             * bit remains 0. */
            Entry *entry = this->find(first_offset);
            if (entry) {
                OffsetPredictionTableData &data = entry->data;
                if (data.pred == delta) {
                    data.accuracy = 1;
                }
                else {
                    if (data.accuracy == 0)
                        data.pred = delta;
                    data.accuracy = 0;
                }
            }
            else {
                this->insert(first_offset, { delta, 0 });
            }
        }
    };
    
    class DeltaPredictionTableData {
      public:
        int pred;
        SaturatingCounter accuracy;
        /* The delta histories obtained from the DHB are used as the keys, and the delta predictions stored in the DPT are
         * the values. Also, each DPT entry has a 2-bit accuracy counter, and a 1-bit nMRU value. */
        /* Each entry in the table consists of 4 basic elements: a delta history (delta), a delta prediction (pred), a 2-bit
         * accuracy counter, and a single nMRU bit. */
        DeltaPredictionTableData() {}
        DeltaPredictionTableData(int input):pred(input) {}
    };
    
    class DeltaPredictionTable : public NMRUFullyAssociativeCache<DeltaPredictionTableData> {
        typedef NMRUFullyAssociativeCache<DeltaPredictionTableData> Super;
    
      public:
        using Super::Super; /* inherit constructors */
    };
    
    class DeltaPredictionTables {
      public:
        DeltaPredictionTables(int delta_prediction_table_size);
    
        void update(uint64_t page_number, ShiftRegister deltas, int last_predictor);
    
        /**
         * @return A pair containing (delta prediction, table index).
         */
        std::pair<int, int> get_prediction(uint64_t page_number, ShiftRegister deltas);
    
      private:
        std::vector<DeltaPredictionTable> delta_prediction_table;
    };

  protected:
    int page_size;
    std::vector<DeltaHistoryBuffer> delta_history_buffer;
    std::vector<OffsetPredictionTable> offset_prediction_table;
    std::vector<DeltaPredictionTables> delta_prediction_tables;

    const bool useMasterId;

    const int degree;

  public:

    VLDPrefetcher(const VLDPrefetcherParams *p);

    void calculatePrefetch(const PacketPtr &pkt,
                           std::vector<AddrPriority> &addresses);
    /* MJL_Begin */
    void MJL_calculatePrefetch(const PacketPtr &pkt,
                           std::vector<AddrPriority> &addresses, 
                           MemCmd::MJL_DirAttribute &MJL_cmdDir);
    /* MJL_End */
};

#endif // __MEM_CACHE_PREFETCH_VLDP_HH__
