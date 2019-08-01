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
#include "mem/cache/prefetch/bo.hh"

using namespace std;

BestOffsetPrefetcher::BestOffsetPrefetcher(const BestOffsetPrefetcherParams *p)
    : QueuedPrefetcher(p),
      blocks_in_page(p->blocks_in_page), 
      prefetch_offset(2, 0),
      best_offset_learning(2, {p->blocks_in_page}),
      recent_requests_table(p->recent_requests_table_size),
      useMasterId(p->use_master_id),
      degree(p->degree)
{
    // Don't consult stride prefetcher on instruction accesses
    onInst = false;
    std::cout << "MJL_BestOffsetPrefetcher" << std::endl;
}

void
BestOffsetPrefetcher::calculatePrefetch(const PacketPtr &pkt,
                                    std::vector<AddrPriority> &addresses)
{
    MemCmd::MJL_DirAttribute dummy_MJL_cmdDir;
    MJL_calculatePrefetch(pkt, addresses, dummy_MJL_cmdDir);
}
/* MJL_Begin */
void
BestOffsetPrefetcher::MJL_calculatePrefetch(const PacketPtr &pkt,
                                    std::vector<AddrPriority> &addresses, 
                                    MemCmd::MJL_DirAttribute &MJL_cmdDir)
{
    if (!pkt->req->hasPC()) {
        DPRINTF(HWPrefetch, "Ignoring request with no PC.\n");
        return;
    }

    // Get required packet info
    Addr pkt_addr = pkt->getAddr();
    // Addr pc = pkt->req->getPC(); Best Offset prefetcher does not use the PC information
    /* MJL_TODO: commenting these now, but would need to deal with them in the future
    bool is_secure = pkt->isSecure();
    MasterID master_id = useMasterId ? pkt->req->masterId() : 0;
     */
    int MJL_triggerDir_type = 0;
    uint64_t block_number = pkt_addr / blkSize;
    // uint64_t block_number = MJL_movColRight(pkt_addr)/blkSize;
    if (pkt->MJL_cmdIsColumn() && MJL_colPf) {
        MJL_triggerDir_type = 1;
        // block_number = MJL_movColRight(MJL_swapRowColBits(pkt_addr)) / blkSize;
        block_number = MJL_swapRowColSegments(pkt_addr)/blkSize;
    }

    // uint64_t page_number = block_number / this->blocks_in_page; // Only used in debug output that's been commented
    int page_offset = block_number % this->blocks_in_page;
    /* ... and if X and X + D lie in the same memory page, a prefetch request for line X + D is sent to the L3
        * cache. */
    if (this->debug) {
        Addr restore_prefetch_offset = this->prefetch_offset[MJL_triggerDir_type] * blkSize;
        // Addr restore_prefetch_offset = MJL_movColLeft(this->prefetch_offset[MJL_triggerDir_type] * blkSize);
        if (pkt->MJL_cmdIsColumn() && MJL_colPf) {
            // restore_prefetch_offset = MJL_swapRowColBits(MJL_movColLeft(this->prefetch_offset[MJL_triggerDir_type] * blkSize));
            // restore_prefetch_offset = MJL_swapRowColBits(this->prefetch_offset[MJL_triggerDir_type] * blkSize);
            restore_prefetch_offset = MJL_swapRowColSegments(restore_prefetch_offset);
        }
        // cerr << "[BOP] block_number=" << std::hex << block_number * blkSize << std::dec << endl;
        // cerr << "[BOP] page_number=" << std::hex << page_number << std::dec << endl;
        // cerr << "[BOP] page_offset=" << std::hex << page_offset * blkSize << std::dec << endl;
        cerr << "[BOP] best_offset=" << std::hex << restore_prefetch_offset << std::dec << endl;
    }

    for (int i = 1; i <= this->degree; i += 1) {
        Addr pf_addr = (block_number + i * this->prefetch_offset[MJL_triggerDir_type]) * blkSize;
        // Addr pf_addr = MJL_movColLeft((block_number + i * this->prefetch_offset[MJL_triggerDir_type]) * blkSize);
        if (pkt->MJL_cmdIsColumn() && MJL_colPf) {
            // pf_addr = MJL_swapRowColBits(MJL_movColLeft(pf_addr));
            // pf_addr = MJL_swapRowColBits(pf_addr);
            pf_addr = MJL_swapRowColSegments(pf_addr);
        }
        if (this->prefetch_offset[MJL_triggerDir_type] != 0 && is_inside_page(page_offset + i * this->prefetch_offset[MJL_triggerDir_type]))
            addresses.push_back(AddrPriority(pf_addr, 0));
        else {
            pfSpanPage += this->degree - i + 1;
            if (this->debug)
                cerr << "[BOP] X and X + " << i << " * D do not lie in the same memory page, no prefetch issued"
                        << endl;
            break;
        }
    }
    MJL_cmdDir = MJL_predictDir(block_number, pkt->MJL_getCmdDir(), pkt->isSecure());

    int old_offset = this->prefetch_offset[MJL_triggerDir_type];
    /* On every eligible L2 read access (miss or prefetched hit), we test an offset di from the list. */
    if (MJL_colPf) {
        this->prefetch_offset[MJL_triggerDir_type] = this->best_offset_learning[MJL_triggerDir_type].test_offset(block_number, recent_requests_table, pkt->MJL_getCmdDir(), pkt->isSecure());
    } else {
        this->prefetch_offset[MJL_triggerDir_type] = this->best_offset_learning[MJL_triggerDir_type].test_offset(block_number, recent_requests_table, MemCmd::MJL_DirAttribute::MJL_IsRow, pkt->isSecure());
    }
    if (this->debug) {
        if (old_offset != this->prefetch_offset[MJL_triggerDir_type]) {
            Addr old_offset_addr = old_offset * blkSize;
            Addr new_offset_addr = this->prefetch_offset[MJL_triggerDir_type] * blkSize;
            // Addr old_offset_addr = MJL_movColLeft(old_offset * blkSize);
            // Addr new_offset_addr = MJL_movColLeft(this->prefetch_offset[MJL_triggerDir_type] * blkSize);
            if (pkt->MJL_cmdIsColumn() && MJL_colPf) {
                // old_offset_addr = MJL_swapRowColBits(MJL_movColLeft(old_offset * blkSize));
                // new_offset_addr = MJL_swapRowColBits(MJL_movColLeft(this->prefetch_offset[MJL_triggerDir_type] * blkSize));
                old_offset_addr = MJL_swapRowColSegments(old_offset_addr);
                new_offset_addr = MJL_swapRowColSegments(new_offset_addr);
            }
            cerr << "[BOP] offset changed from " << std::hex << old_offset_addr << " to " << new_offset_addr << std::dec << endl;
        }
        // cerr << this->recent_requests_table[MJL_triggerDir_type].log();
        // cerr << this->best_offset_learning[MJL_triggerDir_type].log();
    }
    return;
}
/* MJL_End */

BestOffsetPrefetcher*
BestOffsetPrefetcherParams::create()
{
    return new BestOffsetPrefetcher(this);
}

bool 
BestOffsetPrefetcher::is_inside_page(int page_offset) {
    return (0 <= page_offset && page_offset < this->blocks_in_page); 
}

MemCmd::MJL_DirAttribute 
BestOffsetPrefetcher::MJL_predictDir(uint64_t block_number, MemCmd::MJL_DirAttribute MJL_cmdDir, bool is_secure) {
    MemCmd::MJL_DirAttribute MJL_predDir = MJL_cmdDir;
    int page_offset = block_number % this->blocks_in_page;
    int crossDirEnablingOffset = 64;
    int MJL_triggerDir_type = 0;
    // int currentOffset = MJL_movColLeft(this->prefetch_offset[MJL_triggerDir_type]*blkSize);
    int currentOffset = this->prefetch_offset[MJL_triggerDir_type]*blkSize;
    if (MJL_cmdDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
        MJL_triggerDir_type = 1;
        // currentOffset= MJL_swapRowColBits(MJL_movColLeft(this->prefetch_offset[MJL_triggerDir_type]*blkSize));
        currentOffset = MJL_swapRowColSegments(this->prefetch_offset[MJL_triggerDir_type]*blkSize);
    }
    
    bool found =
        is_inside_page(page_offset - this->prefetch_offset[MJL_triggerDir_type] - crossDirEnablingOffset) && recent_requests_table.find(block_number - this->prefetch_offset[MJL_triggerDir_type] - crossDirEnablingOffset, MJL_cmdDir);
    if (found && this->best_offset_learning[MJL_triggerDir_type].is_warmed_up() && this->prefetch_offset[MJL_triggerDir_type] != 0) {
        if (MJL_cmdDir == MemCmd::MJL_DirAttribute::MJL_IsRow && currentOffset % (MJL_getRowWidth() * blkSize/2) == 0) {
            MJL_predDir = MemCmd::MJL_DirAttribute::MJL_IsColumn;
        } else if (MJL_cmdDir == MemCmd::MJL_DirAttribute::MJL_IsColumn && currentOffset % (MJL_getRowWidth() * blkSize)/2 != 0) {
            MJL_predDir = MemCmd::MJL_DirAttribute::MJL_IsRow;
        }
    }

    Addr test_addr = (block_number - this->prefetch_offset[MJL_triggerDir_type] - crossDirEnablingOffset) * blkSize;
    if (MJL_cmdDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
        test_addr = MJL_swapRowColSegments(test_addr);
    }
    if (MJL_cmdDir != MJL_predDir && !MJL_inCache(test_addr, MJL_cmdDir, is_secure)) {
        predInRRNotInCache++;
    }
    if (recent_requests_table.find(block_number - this->prefetch_offset[MJL_triggerDir_type] - crossDirEnablingOffset, MJL_cmdDir) && this->best_offset_learning[MJL_triggerDir_type].is_warmed_up() && this->prefetch_offset[MJL_triggerDir_type] != 0 && ( MJL_cmdDir == MemCmd::MJL_DirAttribute::MJL_IsRow && currentOffset % (MJL_getRowWidth() * blkSize/2) == 0 || MJL_cmdDir == MemCmd::MJL_DirAttribute::MJL_IsColumn && currentOffset % (MJL_getRowWidth() * blkSize)/2 != 0 ) && !is_inside_page(page_offset - this->prefetch_offset[MJL_triggerDir_type] - crossDirEnablingOffset)) {
        predSpanPage++;
    }
    /* MJL_Test 
    Addr test_addr = MJL_movColLeft((block_number - this->prefetch_offset[MJL_triggerDir_type] - crossDirEnablingOffset)*blkSize);
    if (MJL_cmdDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
        test_addr = MJL_swapRowColBits(test_addr);
    }
    std::cerr << "MJL_Prefetcher::MJL_predictDir() predict: " << MJL_predDir << ":" << found << ", " << MJL_cmdDir << ":" << std::hex << test_addr << std::dec << std::endl;
     */
    return MJL_predDir;
}

void 
BestOffsetPrefetcher::MJL_cache_fill(Addr addr, MemCmd::MJL_DirAttribute MJL_cmdDir, bool prefetch) {
    uint64_t block_number = addr/blkSize;
    // uint64_t block_number = MJL_movColRight(addr)/blkSize;
    bool MJL_cmdIsColumn = false;
    if (MJL_cmdDir == MemCmd::MJL_DirAttribute::MJL_IsColumn && MJL_colPf) {
        MJL_cmdIsColumn = true;
    }
    int MJL_triggerDir_type = 0;
    if (MJL_cmdIsColumn) {
        MJL_triggerDir_type = 1;
        // block_number = MJL_movColRight(MJL_swapRowColBits(addr)) / blkSize;
        block_number = MJL_swapRowColSegments(addr)/blkSize;
    }
    int page_offset = block_number % this->blocks_in_page;
    if (this->prefetch_offset[MJL_triggerDir_type] == 0 && prefetch)
        return;
    if (this->prefetch_offset[MJL_triggerDir_type] != 0 && !prefetch)
        return;
    if (!this->is_inside_page(page_offset - this->prefetch_offset[MJL_triggerDir_type]))
        return;
    if (MJL_colPf) {
        this->recent_requests_table.insert(block_number - this->prefetch_offset[MJL_triggerDir_type], MJL_cmdDir);
    } else {
        this->recent_requests_table.insert(block_number - this->prefetch_offset[MJL_triggerDir_type], MemCmd::MJL_DirAttribute::MJL_IsRow);
    }
}

void 
BestOffsetPrefetcher::set_debug_level(int debug_level) {
    bool enable = (bool)debug_level;
    this->debug = enable;
    this->best_offset_learning[0].set_debug_mode(enable);
    this->best_offset_learning[1].set_debug_mode(enable);
    this->recent_requests_table.set_debug_mode(enable);
}

void 
BestOffsetPrefetcher::regStats() {
    BasePrefetcher::regStats();

    testInRRNotInCache
        .name(name() + ".testInRRNotInCache")
        .desc("number offset tests where the tested cache line is in the RR table but not in the cache")
        ;
    
    predInRRNotInCache
        .name(name() + ".predInRRNotInCache")
        .desc("number prediction tests where the tested cache line is in the RR table but not in the cache")
        ;

    predSpanPage
        .name(name() + ".predSpanPage")
        .desc("number of predictions not generated due to page crossing");
}

BestOffsetPrefetcher::Table::Table(int width, int height) : width(width), height(height), cells(height, vector<string>(width)) {}

void 
BestOffsetPrefetcher::Table::set_row(int row, const vector<string> &data, int start_col/* = 0*/) {
    assert(data.size() + start_col == this->width);
    for (unsigned col = start_col; col < this->width; col += 1)
        this->set_cell(row, col, data[col]);
}

void 
BestOffsetPrefetcher::Table::set_col(int col, const vector<string> &data, int start_row/* = 0*/) {
    assert(data.size() + start_row == this->height);
    for (unsigned row = start_row; row < this->height; row += 1)
        this->set_cell(row, col, data[row]);
}

void 
BestOffsetPrefetcher::Table::set_cell(int row, int col, string data) {
    assert(0 <= row && row < (int)this->height);
    assert(0 <= col && col < (int)this->width);
    this->cells[row][col] = data;
}

void 
BestOffsetPrefetcher::Table::set_cell(int row, int col, double data) {
    this->oss.str("");
    this->oss << setw(11) << fixed << setprecision(8) << data;
    this->set_cell(row, col, this->oss.str());
}

void 
BestOffsetPrefetcher::Table::set_cell(int row, int col, int64_t data) {
    this->oss.str("");
    this->oss << setw(11) << std::left << data;
    this->set_cell(row, col, this->oss.str());
}

void 
BestOffsetPrefetcher::Table::set_cell(int row, int col, int data) { this->set_cell(row, col, (int64_t)data); }

void 
BestOffsetPrefetcher::Table::set_cell(int row, int col, uint64_t data) { this->set_cell(row, col, (int64_t)data); }

string 
BestOffsetPrefetcher::Table::to_string() {
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

string 
BestOffsetPrefetcher::Table::data_row(int row, const vector<int> &widths) {
    string out;
    for (unsigned i = 0; i < this->width; i += 1) {
        string data = this->cells[row][i];
        data.resize(widths[i] - 2, ' ');
        out += " | " + data;
    }
    out += " |\n";
    return out;
}

string 
BestOffsetPrefetcher::Table::top_line(const vector<int> &widths) { return Table::line(widths, "┌", "┬", "┐"); }

string 
BestOffsetPrefetcher::Table::mid_line(const vector<int> &widths) { return Table::line(widths, "├", "┼", "┤"); }

string 
BestOffsetPrefetcher::Table::bot_line(const vector<int> &widths) { return Table::line(widths, "└", "┴", "┘"); }

string 
BestOffsetPrefetcher::Table::line(const vector<int> &widths, string left, string mid, string right) {
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

BestOffsetPrefetcher::RecentRequestsTable::RecentRequestsTable(int size) : Super(size) {
    assert(__builtin_popcount(size) == 1);
    this->hash_w = __builtin_ctz(size);
}

BestOffsetPrefetcher::RecentRequestsTable::Entry 
BestOffsetPrefetcher::RecentRequestsTable::insert(uint64_t base_address, MemCmd::MJL_DirAttribute MJL_cmdDir) {
    uint64_t key = this->hash(base_address);
    return Super::insert(key, {base_address, MJL_cmdDir});
}

bool 
BestOffsetPrefetcher::RecentRequestsTable::find(uint64_t base_address, MemCmd::MJL_DirAttribute MJL_cmdDir) {
    uint64_t key = this->hash(base_address);
    return (Super::find(key) != nullptr && Super::find(key)->data.MJL_cmdDir == MJL_cmdDir);
}

string 
BestOffsetPrefetcher::RecentRequestsTable::log() {
    vector<string> headers({"Hash", "Base Address"});
    return Super::log(headers, this->write_data);
}

void 
BestOffsetPrefetcher::RecentRequestsTable::write_data(Entry &entry, BestOffsetPrefetcher::Table &table, int row) {
    table.set_cell(row, 0, bitset<20>(entry.key).to_string());
    table.set_cell(row, 1, entry.data.base_address);
}

/* The RR table is accessed through a simple hash function. For instance, for a 256-entry RR table, we XOR the 8
* least significant line address bits with the next 8 bits to obtain the table index. For 12-bit tags, we skip the
* 8 least significant line address bits and extract the next 12 bits. */
uint64_t 
BestOffsetPrefetcher::RecentRequestsTable::hash(uint64_t input) {
    int next_w_bits = ((1 << hash_w) - 1) & (input >> hash_w);
    uint64_t output = ((1 << 20) - 1) & (next_w_bits ^ input);
    if (this->debug) {
        cerr << "[RR] hash( " << bitset<32>(input).to_string() << " ) = " << bitset<20>(output).to_string() << endl;
    }
    return output;
}

BestOffsetPrefetcher::BestOffsetLearning::BestOffsetLearning(int blocks_in_page) : blocks_in_page(blocks_in_page) {
    /* Useful offset values depend on the memory page size, as the BO prefetcher does not prefetch across page
        * boundaries. For instance, assuming 4KB pages and 64B lines, a page contains 64 lines, and there is no point
        * in considering offset values greater than 63. However, it may be useful to consider offsets greater than 63
        * for systems having superpages. */
    /* We propose a method for offset sampling that is algorithmic and not totally arbitrary: we include in our list
        * all the offsets between 1 and 256 whose prime factorization does not contain primes greater than 5. */
    /* Nothing prevents a BO prefetcher to use negative offset values. Although some applications might benefit from
        * negative offsets, we did not observe any benefit in our experiments. Hence we consider only positive offsets
        * in this study. */
    for (int i = 1; i < blocks_in_page; i += 1) {
        int n = i;
        for (int j = 2; j <= 5; j += 1)
            while (n % j == 0)
                n /= j;
        if (n == 1)
            offset_list.push_back({i, 0});
    }
}

/**
    * @return The current best offset.
    */
int 
BestOffsetPrefetcher::BestOffsetLearning::test_offset(uint64_t block_number, BestOffsetPrefetcher::RecentRequestsTable &recent_requests_table, MemCmd::MJL_DirAttribute MJL_cmdDir, bool is_secure) {
    int page_offset = block_number % this->blocks_in_page;
    Entry &entry = this->offset_list[this->index_to_test];
    bool found =
        is_inside_page(page_offset - entry.offset) && recent_requests_table.find(block_number - entry.offset, MJL_cmdDir);
    if (this->debug) {
        cerr << "[BOL] testing offset=" << entry.offset << " with score=" << entry.score << endl;
        cerr << "[BOL] match=" << found << endl;
    }
    if (found) {
        entry.score += 1;
        if (entry.score > this->best_score) {
            this->best_score = entry.score;
            this->local_best_offset = entry.offset;
        }
        Addr test_addr = (block_number - entry.offset) * blkSize;
        if (MJL_cmdDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
            test_addr = MJL_swapRowColSegments(test_addr);
        }
        if (!MJL_inCache(test_addr, MJL_cmdDir, is_secure)) {
            testInRRNotInCache++;
        }
    }
    this->index_to_test = (this->index_to_test + 1) % this->offset_list.size();
    /* test round termination */
    if (this->index_to_test == 0) {
        if (this->debug) {
            cerr << "[BOL] round=" << this->round << " finished" << endl;
        }
        this->round += 1;
        /* The current learning phase finishes at the end of a round when either of the two following events happens
            * first: one of the scores equals SCOREMAX, or the number of rounds equals ROUNDMAX (a fixed parameter). */
        if (this->best_score >= SCORE_MAX || this->round == ROUND_MAX) {
            if (this->best_score <= BAD_SCORE)
                this->global_best_offset = 0; /* turn off prefetching */
            else
                this->global_best_offset = this->local_best_offset;
            if (this->debug) {
                cerr << "[BOL] learning phase finished, winner=" << this->global_best_offset << endl;
                cerr << this->log();
            }
            this->warmedUp = true;
            /* MJL_Test */
            std::clog << "[BOL]" << MJL_cmdDir << ": BO " << this->global_best_offset << ", score " << this->best_score << ", round " << this->round << std::endl;
            /* */
            /* reset all internal state */
            for (auto &entry : this->offset_list)
                entry.score = 0;
            this->local_best_offset = 0;
            this->best_score = 0;
            this->round = 0;
        }
    }
    return this->global_best_offset;
}

string 
BestOffsetPrefetcher::BestOffsetLearning::log() {
    Table table(2, offset_list.size() + 1);
    table.set_row(0, {"Offset", "Score"});
    for (unsigned i = 0; i < offset_list.size(); i += 1) {
        table.set_cell(i + 1, 0, offset_list[i].offset);
        table.set_cell(i + 1, 1, offset_list[i].score);
    }
    return table.to_string();
}

void 
BestOffsetPrefetcher::BestOffsetLearning::set_debug_mode(bool enable) { this->debug = enable; }

bool 
BestOffsetPrefetcher::BestOffsetLearning::is_warmed_up() const { return this->warmedUp; }

bool 
BestOffsetPrefetcher::BestOffsetLearning::is_inside_page(int page_offset) { return (0 <= page_offset && page_offset < this->blocks_in_page); }
