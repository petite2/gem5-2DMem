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

BingoPrefetcher::BingoPrefetcher(const BingoPrefetcherParams *p)
    : QueuedPrefetcher(p),
    pattern_len(p->pattern_len), filter_table(p->filter_table_size),
    accumulation_table(p->accumulation_table_size, p->pattern_len),
    pht(p->pattern_history_table_size, p->pattern_len, p->min_addr_width, p->max_addr_width, p->pc_width),
    useMasterId(p->use_master_id)
{
    // Don't consult stride prefetcher on instruction accesses
    onInst = false;

    assert(isPowerOf2(pcTableSets));
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
    bool is_secure = pkt->isSecure();
    MasterID master_id = useMasterId ? pkt->req->masterId() : 0;

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

vector<bool> BingoPrefetcher::find_in_pht(uint64_t pc, uint64_t address) {
    if (this->debug_level >= 1) {
        cerr << "[Bingo] find_in_phts(pc=" << pc << ", address=" << address << ")" << endl;
    }
    return this->pht.find(pc, address);
}

void BingoPrefetcher::insert_in_pht(const AccumulationTable::Entry &entry) {
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
