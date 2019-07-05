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
 *          Steve Reinhardt
 */

/**
 * @file
 * Stride Prefetcher template instantiations.
 */

#include "base/random.hh"
#include "debug/HWPrefetch.hh"
#include "mem/cache/prefetch/bingo.hh"

using namespace std;

BingoPrefetcher::BingoPrefetcher(const BingoPrefetcherParams *p)
    : QueuedPrefetcher(p),
    pattern_len(p->pattern_len), filter_table(p->filter_table_size),
    accumulation_table(p->accumulation_table_size, p->pattern_len),
    pht(p->pattern_history_table_size, p->pattern_len, p->min_addr_width, p->max_addr_width, p->pc_width),
    useMasterId(p->use_master_id)
{
    // Don't consult stride prefetcher on instruction accesses
    onInst = false;
    onWrite = false;
    std::cout << "MJL_BingoPrefetcher" << std::endl;
}

void
BingoPrefetcher::calculatePrefetch(const PacketPtr &pkt,
                                    std::vector<AddrPriority> &addresses)
{
    MemCmd::MJL_DirAttribute MJL_cmdDir;
    MJL_calculatePrefetch(pkt, addresses, MJL_cmdDir);
}
/* MJL_Begin */
void
BingoPrefetcher::MJL_calculatePrefetch(const PacketPtr &pkt,
                                    std::vector<AddrPriority> &addresses, 
                                    MemCmd::MJL_DirAttribute &MJL_cmdDir)
{
    if (!pkt->req->hasPC()) {
        DPRINTF(HWPrefetch, "Ignoring request with no PC.\n");
        return;
    }

    // Get required packet info
    Addr pkt_addr = pkt->getAddr();
    Addr pc = pkt->req->getPC();
    /* MJL_TODO: commenting these now, but would need to deal with them in the future
    bool is_secure = pkt->isSecure();
    MasterID master_id = useMasterId ? pkt->req->masterId() : 0;
    */

    uint64_t block_number = pkt_addr/blkSize;

    if (this->debug_level >= 1) {
        cerr << "[Bingo] access(block_number=" << block_number << ", pc=" << pc << ")" << endl;
    }
    uint64_t region_number = block_number / this->pattern_len;
    int region_offset = block_number % this->pattern_len;
    bool success = this->accumulation_table.set_pattern(region_number, region_offset);
    if (success)
        return;
    FilterTable::Entry *entry = this->filter_table.find(region_number);
    if (!entry) {
        /* trigger access */
        this->filter_table.insert(region_number, pc, region_offset);
        vector<bool> pattern = this->find_in_phts(pc, block_number);
        if (pattern.empty())
            return;
        for (int i = 0; i < this->pattern_len; i += 1)
            if (pattern[i]) {
                if (this->debug_level >= 1) {
                    cerr << "[Bingo] access(prefetch_addr=" << (region_number * this->pattern_len + i) * blkSize << ")" << endl;
                }
                addresses.push_back(AddrPriority((region_number * this->pattern_len + i) * blkSize, 0));
            }
        return;
    }
    if (entry->data.offset != region_offset) {
        /* move from filter table to accumulation table */
        AccumulationTable::Entry victim = this->accumulation_table.insert(*entry);
        this->accumulation_table.set_pattern(region_number, region_offset);
        this->filter_table.erase(region_number);
        if (victim.valid) {
            /* move from accumulation table to pattern history table */
            this->insert_in_phts(victim);
        }
    }
    return;
}
/* MJL_End */

void BingoPrefetcher::MJL_eviction(Addr addr) {
    uint64_t block_number = addr/blkSize;
    if (this->debug_level >= 1) {
        cerr << "[Bingo] eviction(block_number=" << block_number << ")" << endl;
    }
    /* end of generation */
    uint64_t region_number = block_number / this->pattern_len;
    this->filter_table.erase(region_number);
    AccumulationTable::Entry *entry = this->accumulation_table.erase(region_number);
    if (entry) {
        /* move from accumulation table to pattern history table */
        this->insert_in_phts(*entry);
    }
}

vector<bool> BingoPrefetcher::find_in_phts(uint64_t pc, uint64_t address) {
    if (this->debug_level >= 1) {
        cerr << "[Bingo] find_in_phts(pc=" << pc << ", address=" << address << ")" << endl;
    }
    return this->pht.find(pc, address);
}

void BingoPrefetcher::insert_in_phts(const AccumulationTable::Entry &entry) {
    if (this->debug_level >= 1) {
        cerr << "[Bingo] insert_in_phts(...)" << endl;
    }
    uint64_t pc = entry.data.pc;
    uint64_t address = entry.key * this->pattern_len + entry.data.offset;
    const vector<bool> &pattern = entry.data.pattern;
    this->pht.insert(pc, address, pattern);
}

BingoPrefetcher*
BingoPrefetcherParams::create()
{
    return new BingoPrefetcher(this);
}

Table::Table(int width, int height) : width(width), height(height), cells(height, vector<string>(width)) {}

void Table::set_row(int row, const vector<string> &data, int start_col/* = 0*/) {
    assert(data.size() + start_col == this->width);
    for (unsigned col = start_col; col < this->width; col += 1)
        this->set_cell(row, col, data[col]);
}

void Table::set_col(int col, const vector<string> &data, int start_row/* = 0*/) {
    assert(data.size() + start_row == this->height);
    for (unsigned row = start_row; row < this->height; row += 1)
        this->set_cell(row, col, data[row]);
}

void Table::set_cell(int row, int col, string data) {
    assert(0 <= row && row < (int)this->height);
    assert(0 <= col && col < (int)this->width);
    this->cells[row][col] = data;
}

void Table::set_cell(int row, int col, double data) {
    this->oss.str("");
    this->oss << setw(11) << fixed << setprecision(8) << data;
    this->set_cell(row, col, this->oss.str());
}

void Table::set_cell(int row, int col, int64_t data) {
    this->oss.str("");
    this->oss << setw(11) << std::left << data;
    this->set_cell(row, col, this->oss.str());
}

void Table::set_cell(int row, int col, int data) { this->set_cell(row, col, (int64_t)data); }

void Table::set_cell(int row, int col, uint64_t data) { this->set_cell(row, col, (int64_t)data); }

string Table::to_string() {
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

string Table::data_row(int row, const vector<int> &widths) {
    string out;
    for (unsigned i = 0; i < this->width; i += 1) {
        string data = this->cells[row][i];
        data.resize(widths[i] - 2, ' ');
        out += " | " + data;
    }
    out += " |\n";
    return out;
}

string Table::top_line(const vector<int> &widths) { return Table::line(widths, "┌", "┬", "┐"); }

string Table::mid_line(const vector<int> &widths) { return Table::line(widths, "├", "┼", "┤"); }

string Table::bot_line(const vector<int> &widths) { return Table::line(widths, "└", "┴", "┘"); }

string Table::line(const vector<int> &widths, string left, string mid, string right) {
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

FilterTable::FilterTable(int size) : Super(size) { assert(__builtin_popcount(size) == 1); }

FilterTable::Entry * FilterTable::find(uint64_t region_number) {
    Entry *entry = Super::find(region_number);
    if (!entry)
        return nullptr;
    this->set_mru(region_number);
    return entry;
}

void FilterTable::insert(uint64_t region_number, uint64_t pc, int offset) {
    assert(!this->find(region_number));
    Super::insert(region_number, {pc, offset});
    this->set_mru(region_number);
}

AccumulationTable::AccumulationTable(int size, int pattern_len) : Super(size), pattern_len(pattern_len) {
    assert(__builtin_popcount(size) == 1);
    assert(__builtin_popcount(pattern_len) == 1);
}

/**
    * @return A return value of false means that the tag wasn't found in the table and true means success.
    */
bool AccumulationTable::set_pattern(uint64_t region_number, int offset) {
    Entry *entry = Super::find(region_number);
    if (!entry)
        return false;
    entry->data.pattern[offset] = true;
    this->set_mru(region_number);
    return true;
}

AccumulationTable::Entry AccumulationTable::insert(FilterTable::Entry &entry) {
    assert(!this->find(entry.key));
    vector<bool> pattern(this->pattern_len, false);
    pattern[entry.data.offset] = true;
    Entry old_entry = Super::insert(entry.key, {entry.data.pc, entry.data.offset, pattern});
    this->set_mru(entry.key);
    return old_entry;
}

PatternHistoryTable::PatternHistoryTable(
    int size, int pattern_len, int min_addr_width, int max_addr_width, int pc_width, int num_ways/* = 16*/)
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
void PatternHistoryTable::insert(uint64_t pc, uint64_t address, vector<bool> pattern) {
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
vector<bool> PatternHistoryTable::find(uint64_t pc, uint64_t address) {
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

uint64_t PatternHistoryTable::build_key(uint64_t pc, uint64_t address) {
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

vector<bool> PatternHistoryTable::vote(const vector<vector<SC2>> &x, float thresh/* = THRESH*/) {
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
