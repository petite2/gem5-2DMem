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
#include "mem/cache/prefetch/vldp.hh"

using namespace std;

VLDPrefetcher::VLDPrefetcher(const VLDPrefetcherParams *p)
    : QueuedPrefetcher(p),
      page_size(p->page_size),
      delta_history_buffer(2, {p->delta_history_buffer_size}),
      offset_prediction_table(2, {p->offset_prediction_table_size}),
      delta_prediction_tables(2, {p->delta_prediction_table_size}),
      useMasterId(p->use_master_id),
      degree(p->degree)
{
    // Don't consult stride prefetcher on instruction accesses
    onInst = false;
    std::cout << "MJL_VLDPrefetcher" << std::endl;
}

void
VLDPrefetcher::calculatePrefetch(const PacketPtr &pkt,
                                    std::vector<AddrPriority> &addresses)
{
    MemCmd::MJL_DirAttribute dummy_MJL_cmdDir;
    MJL_calculatePrefetch(pkt, addresses, dummy_MJL_cmdDir);
}
/* MJL_Begin */
void
VLDPrefetcher::MJL_calculatePrefetch(const PacketPtr &pkt,
                                    std::vector<AddrPriority> &addresses, 
                                    MemCmd::MJL_DirAttribute &MJL_cmdDir)
{
    if (!pkt->req->hasPC()) {
        DPRINTF(HWPrefetch, "Ignoring request with no PC.\n");
        return;
    }

    // Get required packet info
    Addr pkt_addr = pkt->getAddr();
    // Addr pc = pkt->req->getPC(); // VLDP does not need the PC information
    /* MJL_TODO: commenting these now, but would need to deal with them in the future
    bool is_secure = pkt->isSecure();
    MasterID master_id = useMasterId ? pkt->req->masterId() : 0;
     */
    int MJL_triggerDir_type = 0;
    if (pkt->MJL_cmdIsColumn()) {
        MJL_triggerDir_type = 1;
    }

    uint64_t block_number = pkt_addr/blkSize;

    uint64_t page_number = block_number / this->page_size;
    int page_offset = block_number % this->page_size;
    int delta = this->delta_history_buffer[MJL_triggerDir_type].update(page_number, page_offset);
    if (delta == 0)
        return;
    DeltaHistoryBufferData &dhb_data = this->delta_history_buffer[MJL_triggerDir_type].find(page_number)->data;
    if (dhb_data.num_times_used == 1) {
        /* On the first access to a page, the OPT is looked up using the page offset, and if the accuracy bit is set
            * for this entry, a prefetch is issued with the predicted delta*/
        OffsetPredictionTable::Entry *entry = this->offset_prediction_table[MJL_triggerDir_type].find(page_offset);
        if (!entry || entry->data.accuracy == 0) {
            return;
        } else {
            addresses.push_back(AddrPriority((entry->data.pred + page_number * this->page_size)*blkSize,0));
            return;
        }
    }
    else if (dhb_data.num_times_used == 2) {
        /* On the second access to a page, a delta can be computed and compared with the contents of the OPT. */
        uint64_t first_offset = page_offset - delta;
        this->offset_prediction_table[MJL_triggerDir_type].update(first_offset, delta);
    }
    else {
        this->delta_prediction_tables[MJL_triggerDir_type].update(page_number, dhb_data.deltas, dhb_data.last_predictor);
    }
    ShiftRegister deltas = dhb_data.deltas;
    for (int i = 0; i < this->degree; i += 1) {
        pair<int, int> pred = this->delta_prediction_tables[MJL_triggerDir_type].get_prediction(page_number, deltas);
        int delta = pred.first;
        int table_index = pred.second;
        /* Despite the fact that the DPT yields 4 separate predictions in this case, the accuracy is updated only
            * once for the table that issued the original prediction. */
        if (i == 0)
            dhb_data.last_predictor = table_index;
        /* While issuing multi degree prefetches, we only accept predictions of tables 2-3 for degrees greater
            * than 1. */
        if (delta == 0 || (i > 0 && table_index == 0))
            break;
        block_number += delta;
        /* do not prefetch beyond page boundaries */
        if (block_number / this->page_size == page_number) {
            addresses.push_back(AddrPriority(block_number*blkSize,0));
        }
        deltas.insert(delta);
    }
    return;
}
/* MJL_End */

VLDPrefetcher*
VLDPrefetcherParams::create()
{
    return new VLDPrefetcher(this);
}

VLDPrefetcher::Table::Table(int width, int height) : width(width), height(height), cells(height, vector<string>(width)) {}
    
void 
VLDPrefetcher::Table::set_row(int row, const vector<string> &data, int start_col/* = 0*/) {
    assert(data.size() + start_col == this->width);
    for (unsigned col = start_col; col < this->width; col += 1)
        this->set_cell(row, col, data[col]);
}

void 
VLDPrefetcher::Table::set_col(int col, const vector<string> &data, int start_row/* = 0*/) {
    assert(data.size() + start_row == this->height);
    for (unsigned row = start_row; row < this->height; row += 1)
        this->set_cell(row, col, data[row]);
}

void 
VLDPrefetcher::Table::set_cell(int row, int col, string data) {
    assert(0 <= row && row < (int)this->height);
    assert(0 <= col && col < (int)this->width);
    this->cells[row][col] = data;
}

void 
VLDPrefetcher::Table::set_cell(int row, int col, double data) {
    this->oss.str("");
    this->oss << setw(11) << fixed << setprecision(8) << data;
    this->set_cell(row, col, this->oss.str());
}

void 
VLDPrefetcher::Table::set_cell(int row, int col, int64_t data) {
    this->oss.str("");
    this->oss << setw(11) << std::left << data;
    this->set_cell(row, col, this->oss.str());
}

void 
VLDPrefetcher::Table::set_cell(int row, int col, int data) { this->set_cell(row, col, (int64_t)data); }

void 
VLDPrefetcher::Table::set_cell(int row, int col, uint64_t data) { this->set_cell(row, col, (int64_t)data); }

string 
VLDPrefetcher::Table::to_string() {
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
VLDPrefetcher::Table::data_row(int row, const vector<int> &widths) {
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
VLDPrefetcher::Table::top_line(const vector<int> &widths) { return Table::line(widths, "┌", "┬", "┐"); }

string 
VLDPrefetcher::Table::mid_line(const vector<int> &widths) { return Table::line(widths, "├", "┼", "┤"); }

string 
VLDPrefetcher::Table::bot_line(const vector<int> &widths) { return Table::line(widths, "└", "┴", "┘"); }

string 
VLDPrefetcher::Table::line(const vector<int> &widths, string left, string mid, string right) {
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

VLDPrefetcher::ShiftRegister::ShiftRegister(unsigned size/* = 4*/) : size(size), width(64 / size) {}
    
void 
VLDPrefetcher::ShiftRegister::insert(int x) {
    x &= (1 << this->width) - 1;
    this->reg = (this->reg << this->width) | x;
}

/**
    * @return Returns raw buffer data in range [le, ri).
    * Note that more recent data have a smaller index.
    */
uint64_t 
VLDPrefetcher::ShiftRegister::get_code(unsigned le, unsigned ri) {
    assert(le < this->size);
    assert(le < ri && ri <= this->size);
    uint64_t mask = (1ull << (this->width * (ri - le))) - 1ull;
    return (this->reg >> (le * this->width)) & mask;
}

/**
    * @return Returns integer value of data at specified index.
    */
int 
VLDPrefetcher::ShiftRegister::get_value(int i) {
    int x = this->get_code(i, i + 1);
    /* sign extend */
    int d = 32 - this->width;
    return (x << d) >> d;
}

VLDPrefetcher::DeltaPredictionTables::DeltaPredictionTables(int delta_prediction_table_size)
    /* Our DPT implementation uses a set of 3 DPT tables, allowing for histories up to 3 deltas long. */
    : delta_prediction_table(3, DeltaPredictionTable(delta_prediction_table_size)) {}

void 
VLDPrefetcher::DeltaPredictionTables::update(uint64_t page_number, VLDPrefetcher::ShiftRegister deltas, int last_predictor) {
    /* last predictor is either 0, 1 or 2 (since there are 3 DPTs) */
    uint64_t key = deltas.get_code(1, last_predictor + 2);
    /* The DHB entry for a page also stores the ID of the DPT table which was most recently used to predict the
        * prefetch candidates for this page. This ID is used to update the accuracy of the DPT */
    DeltaPredictionTable::Entry *entry = delta_prediction_table[last_predictor].find(key);
    if (entry) {
        delta_prediction_table[last_predictor].set_mru(key);
        int last_delta = deltas.get_value(0);
        DeltaPredictionTableData &dpt_data = entry->data;
        /* Based on the accuracy of the previous prediction the accuracy bits of that prediction entry can be
            * updated; incremented in the case of an accurate prediction, decremented otherwise. */
        if (dpt_data.pred == last_delta)
            dpt_data.accuracy.inc();
        else {
            dpt_data.accuracy.dec();
            /* Finally, if the prediction accuracy is sufficiently low, the delta prediction field may be updated to
                * reflect the new delta. */
            if (dpt_data.accuracy.get_cnt() == 0)
                dpt_data.pred = last_delta;
            /* Inaccurate predictions in a DPT table T will prompt the promotion of the delta pattern to the next
                * table T+1. */
            if (last_predictor != 2 && deltas.get_value(last_predictor + 2) != 0) {
                uint64_t new_key = deltas.get_code(1, last_predictor + 3);
                /* NOTE: promoted entries are assigned an accuracy of zero */
                /* MJL_Begin 
                SaturatingCounter tempCounter(deltas.get_value(0));
                 MJL_End */
                delta_prediction_table[last_predictor + 1].insert(new_key, /* MJL_Begin tempCounter MJL_End *//* MJL_Comment */{ deltas.get_value(0) }/* */);
                delta_prediction_table[last_predictor + 1].set_mru(new_key);
            }
        }
    }
    /* If a matching delta pattern is not currently present in any DPT tables, then an entry is created for the
        * latest delta in the shortest-history table. */
    for (int i = 2; i >= 0; i -= 1) {
        DeltaPredictionTable &dpt = delta_prediction_table[i];
        uint64_t key = deltas.get_code(1, i + 2);
        DeltaPredictionTable::Entry *entry = dpt.find(key);
        if (entry)
            return;
    }
    /* MJL_Begin */
    SaturatingCounter tempCounter(deltas.get_value(0));
    /* MJL_End */
    delta_prediction_table[0].insert(deltas.get_code(1, 2), /* MJL_Begin tempCounter MJL_End *//* MJL_Comment */{ deltas.get_value(0) }/* */);
    delta_prediction_table[0].set_mru(deltas.get_code(1, 2));
}

/**
    * @return A pair containing (delta prediction, table index).
    */
pair<int, int> 
VLDPrefetcher::DeltaPredictionTables::get_prediction(uint64_t page_number, VLDPrefetcher::ShiftRegister deltas) {
    /* When searching for a delta to prefetch, VLDP prefers to use predictions that come from DPT tables that track
        * longer histories. */
        /* DPT lookups may produce matches in more than one of the DPT tables. In these cases, VLDP prioritizes
        * predictions made by the table that uses the longest matching delta history, which maximizes accuracy. */
    for (int i = 2; i >= 0; i -= 1) {
        DeltaPredictionTable &dpt = delta_prediction_table[i];
        uint64_t key = deltas.get_code(0, i + 1);
        DeltaPredictionTable::Entry *entry = dpt.find(key);
        if (entry) {
            dpt.set_mru(key);
            return make_pair(entry->data.pred, i);
        }
    }
    return make_pair(0, 0);
}
