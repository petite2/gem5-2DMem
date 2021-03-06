/*
 * Copyright (c) 2012-2013 ARM Limited
 * All rights reserved.
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
 * Copyright (c) 2003-2005 The Regents of The University of Michigan
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
 * Authors: Erik Hallnor
 */

/**
 * @file
 * Definition of BaseCache functions.
 */

#include "mem/cache/base.hh"

#include "debug/Cache.hh"
#include "debug/Drain.hh"
#include "mem/cache/cache.hh"
#include "mem/cache/mshr.hh"
#include "mem/cache/tags/fa_lru.hh"
#include "mem/cache/tags/lru.hh"
#include "mem/cache/tags/random_repl.hh"
#include "sim/full_system.hh"

using namespace std;

BaseCache::CacheSlavePort::CacheSlavePort(const std::string &_name,
                                          BaseCache *_cache,
                                          const std::string &_label)
    : QueuedSlavePort(_name, _cache, queue), queue(*_cache, *this, _label),
      blocked(false), mustSendRetry(false), sendRetryEvent(this)
{
}

BaseCache::BaseCache(const BaseCacheParams *p, unsigned blk_size)
    : MemObject(p),
      cpuSidePort(nullptr), memSidePort(nullptr),/* MJL_Begin */
      MJL_rowColBloomFilter(nullptr), MJL_Test_rowColBloomFilters(nullptr), 
      MJL_bloomFilterSize(p->MJL_bloomFilterSize), MJL_bloomFilterHashFuncId(p->MJL_bloomFilterHashFuncId), /* MJL_End */
      mshrQueue("MSHRs", p->mshrs, 0, p->demand_mshr_reserve), // see below
      writeBuffer("write buffer", p->write_buffers, /* MJL_Begin */p->MJL_2D_Cache? 9 * p->mshrs : 8+/* MJL_End */p->mshrs), // see below
      blkSize(blk_size),
      /* MJL_Begin */
      MJL_rowWidth(p->MJL_row_width),
      MJL_defaultColumn(p->MJL_default_column),
      MJL_2DCache(p->MJL_2D_Cache),
      MJL_has2DLLC(p->MJL_has2DLLC),
      MJL_2DTransferType(p->MJL_2D_Transfer_Type),
      MJL_extra2DWriteLatency(p->MJL_extra2DWrite_latency),
      MJL_oracleProxy(p->MJL_oracleProxy),
      MJL_oracleProxyReplay(p->MJL_oracleProxyReplay),
      /* MJL_End */
      lookupLatency(p->tag_latency),
      dataLatency(p->data_latency),
      forwardLatency(p->tag_latency),
      fillLatency(p->data_latency),
      responseLatency(p->response_latency),
      numTarget(p->tgts_per_mshr),
      forwardSnoops(true),
      isReadOnly(p->is_read_only),
      blocked(0),
      order(0),
      noTargetMSHR(nullptr),
      missCount(p->max_miss_count),
      addrRanges(p->addr_ranges.begin(), p->addr_ranges.end()),
      system(p->system)/* MJL_Begin */, MJL_Debug_Out(false)/* MJL_End */
{
    // the MSHR queue has no reserve entries as we check the MSHR
    // queue on every single allocation, whereas the write queue has
    // as many reserve entries as we have MSHRs, since every MSHR may
    // eventually require a writeback, and we do not check the write
    // buffer before committing to an MSHR

    // forward snoops is overridden in init() once we can query
    // whether the connected master is actually snooping or not
    /* MJL_Begin */
    assert(MJL_2DTransferType == 0 || MJL_2DTransferType == 1);
    /* MJL_End */
}

void
BaseCache::CacheSlavePort::setBlocked()
{
    assert(!blocked);
    DPRINTF(CachePort, "Port is blocking new requests\n");
    blocked = true;
    // if we already scheduled a retry in this cycle, but it has not yet
    // happened, cancel it
    if (sendRetryEvent.scheduled()) {
        owner.deschedule(sendRetryEvent);
        DPRINTF(CachePort, "Port descheduled retry\n");
        mustSendRetry = true;
    }
}

void
BaseCache::CacheSlavePort::clearBlocked()
{
    assert(blocked);
    DPRINTF(CachePort, "Port is accepting new requests\n");
    blocked = false;
    if (mustSendRetry) {
        // @TODO: need to find a better time (next cycle?)
        owner.schedule(sendRetryEvent, curTick() + 1);
    }
}

void
BaseCache::CacheSlavePort::processSendRetry()
{
    DPRINTF(CachePort, "Port is sending retry\n");

    // reset the flag and call retry
    mustSendRetry = false;
    sendRetryReq();
}

void
BaseCache::init()
{
    if (!cpuSidePort->isConnected() || !memSidePort->isConnected())
        fatal("Cache ports on %s are not connected\n", name());
    cpuSidePort->sendRangeChange();
    forwardSnoops = cpuSidePort->isSnooping();
}

BaseMasterPort &
BaseCache::getMasterPort(const std::string &if_name, PortID idx)
{
    if (if_name == "mem_side") {
        return *memSidePort;
    }  else {
        return MemObject::getMasterPort(if_name, idx);
    }
}

BaseSlavePort &
BaseCache::getSlavePort(const std::string &if_name, PortID idx)
{
    if (if_name == "cpu_side") {
        return *cpuSidePort;
    } else {
        return MemObject::getSlavePort(if_name, idx);
    }
}

bool
BaseCache::inRange(Addr addr) const
{
    for (const auto& r : addrRanges) {
        if (r.contains(addr)) {
            return true;
       }
    }
    return false;
}

void
BaseCache::regStats()
{
    MemObject::regStats();

    using namespace Stats;

    // Hit statistics
    for (int access_idx = 0; access_idx < MemCmd::NUM_MEM_CMDS; ++access_idx) {
        MemCmd cmd(access_idx);
        const string &cstr = cmd.toString();

        hits[access_idx]
            .init(system->maxMasters())
            .name(name() + "." + cstr + "_hits")
            .desc("number of " + cstr + " hits")
            .flags(total | nozero | nonan)
            ;
        for (int i = 0; i < system->maxMasters(); i++) {
            hits[access_idx].subname(i, system->getMasterName(i));
        }
    }

// These macros make it easier to sum the right subset of commands and
// to change the subset of commands that are considered "demand" vs
// "non-demand"
#define SUM_DEMAND(s) \
    (s[MemCmd::ReadReq] + s[MemCmd::WriteReq] + s[MemCmd::WriteLineReq] + \
     s[MemCmd::ReadExReq] + s[MemCmd::ReadCleanReq] + s[MemCmd::ReadSharedReq])

// should writebacks be included here?  prior code was inconsistent...
#define SUM_NON_DEMAND(s) \
    (s[MemCmd::SoftPFReq] + s[MemCmd::HardPFReq])

    demandHits
        .name(name() + ".demand_hits")
        .desc("number of demand (read+write) hits")
        .flags(total | nozero | nonan)
        ;
    demandHits = SUM_DEMAND(hits);
    for (int i = 0; i < system->maxMasters(); i++) {
        demandHits.subname(i, system->getMasterName(i));
    }

    overallHits
        .name(name() + ".overall_hits")
        .desc("number of overall hits")
        .flags(total | nozero | nonan)
        ;
    overallHits = demandHits + SUM_NON_DEMAND(hits);
    for (int i = 0; i < system->maxMasters(); i++) {
        overallHits.subname(i, system->getMasterName(i));
    }

    // Miss statistics
    for (int access_idx = 0; access_idx < MemCmd::NUM_MEM_CMDS; ++access_idx) {
        MemCmd cmd(access_idx);
        const string &cstr = cmd.toString();

        misses[access_idx]
            .init(system->maxMasters())
            .name(name() + "." + cstr + "_misses")
            .desc("number of " + cstr + " misses")
            .flags(total | nozero | nonan)
            ;
        for (int i = 0; i < system->maxMasters(); i++) {
            misses[access_idx].subname(i, system->getMasterName(i));
        }
    }

    demandMisses
        .name(name() + ".demand_misses")
        .desc("number of demand (read+write) misses")
        .flags(total | nozero | nonan)
        ;
    demandMisses = SUM_DEMAND(misses);
    for (int i = 0; i < system->maxMasters(); i++) {
        demandMisses.subname(i, system->getMasterName(i));
    }

    overallMisses
        .name(name() + ".overall_misses")
        .desc("number of overall misses")
        .flags(total | nozero | nonan)
        ;
    overallMisses = demandMisses + SUM_NON_DEMAND(misses);
    for (int i = 0; i < system->maxMasters(); i++) {
        overallMisses.subname(i, system->getMasterName(i));
    }

    // Miss latency statistics
    for (int access_idx = 0; access_idx < MemCmd::NUM_MEM_CMDS; ++access_idx) {
        MemCmd cmd(access_idx);
        const string &cstr = cmd.toString();

        missLatency[access_idx]
            .init(system->maxMasters())
            .name(name() + "." + cstr + "_miss_latency")
            .desc("number of " + cstr + " miss cycles")
            .flags(total | nozero | nonan)
            ;
        for (int i = 0; i < system->maxMasters(); i++) {
            missLatency[access_idx].subname(i, system->getMasterName(i));
        }
    }

    demandMissLatency
        .name(name() + ".demand_miss_latency")
        .desc("number of demand (read+write) miss cycles")
        .flags(total | nozero | nonan)
        ;
    demandMissLatency = SUM_DEMAND(missLatency);
    for (int i = 0; i < system->maxMasters(); i++) {
        demandMissLatency.subname(i, system->getMasterName(i));
    }

    overallMissLatency
        .name(name() + ".overall_miss_latency")
        .desc("number of overall miss cycles")
        .flags(total | nozero | nonan)
        ;
    overallMissLatency = demandMissLatency + SUM_NON_DEMAND(missLatency);
    for (int i = 0; i < system->maxMasters(); i++) {
        overallMissLatency.subname(i, system->getMasterName(i));
    }

    // access formulas
    for (int access_idx = 0; access_idx < MemCmd::NUM_MEM_CMDS; ++access_idx) {
        MemCmd cmd(access_idx);
        const string &cstr = cmd.toString();

        accesses[access_idx]
            .name(name() + "." + cstr + "_accesses")
            .desc("number of " + cstr + " accesses(hits+misses)")
            .flags(total | nozero | nonan)
            ;
        accesses[access_idx] = hits[access_idx] + misses[access_idx];

        for (int i = 0; i < system->maxMasters(); i++) {
            accesses[access_idx].subname(i, system->getMasterName(i));
        }
    }

    demandAccesses
        .name(name() + ".demand_accesses")
        .desc("number of demand (read+write) accesses")
        .flags(total | nozero | nonan)
        ;
    demandAccesses = demandHits + demandMisses;
    for (int i = 0; i < system->maxMasters(); i++) {
        demandAccesses.subname(i, system->getMasterName(i));
    }

    overallAccesses
        .name(name() + ".overall_accesses")
        .desc("number of overall (read+write) accesses")
        .flags(total | nozero | nonan)
        ;
    overallAccesses = overallHits + overallMisses;
    for (int i = 0; i < system->maxMasters(); i++) {
        overallAccesses.subname(i, system->getMasterName(i));
    }

    // miss rate formulas
    for (int access_idx = 0; access_idx < MemCmd::NUM_MEM_CMDS; ++access_idx) {
        MemCmd cmd(access_idx);
        const string &cstr = cmd.toString();

        missRate[access_idx]
            .name(name() + "." + cstr + "_miss_rate")
            .desc("miss rate for " + cstr + " accesses")
            .flags(total | nozero | nonan)
            ;
        missRate[access_idx] = misses[access_idx] / accesses[access_idx];

        for (int i = 0; i < system->maxMasters(); i++) {
            missRate[access_idx].subname(i, system->getMasterName(i));
        }
    }

    demandMissRate
        .name(name() + ".demand_miss_rate")
        .desc("miss rate for demand accesses")
        .flags(total | nozero | nonan)
        ;
    demandMissRate = demandMisses / demandAccesses;
    for (int i = 0; i < system->maxMasters(); i++) {
        demandMissRate.subname(i, system->getMasterName(i));
    }

    overallMissRate
        .name(name() + ".overall_miss_rate")
        .desc("miss rate for overall accesses")
        .flags(total | nozero | nonan)
        ;
    overallMissRate = overallMisses / overallAccesses;
    for (int i = 0; i < system->maxMasters(); i++) {
        overallMissRate.subname(i, system->getMasterName(i));
    }

    // miss latency formulas
    for (int access_idx = 0; access_idx < MemCmd::NUM_MEM_CMDS; ++access_idx) {
        MemCmd cmd(access_idx);
        const string &cstr = cmd.toString();

        avgMissLatency[access_idx]
            .name(name() + "." + cstr + "_avg_miss_latency")
            .desc("average " + cstr + " miss latency")
            .flags(total | nozero | nonan)
            ;
        avgMissLatency[access_idx] =
            missLatency[access_idx] / misses[access_idx];

        for (int i = 0; i < system->maxMasters(); i++) {
            avgMissLatency[access_idx].subname(i, system->getMasterName(i));
        }
    }

    demandAvgMissLatency
        .name(name() + ".demand_avg_miss_latency")
        .desc("average overall miss latency")
        .flags(total | nozero | nonan)
        ;
    demandAvgMissLatency = demandMissLatency / demandMisses;
    for (int i = 0; i < system->maxMasters(); i++) {
        demandAvgMissLatency.subname(i, system->getMasterName(i));
    }

    overallAvgMissLatency
        .name(name() + ".overall_avg_miss_latency")
        .desc("average overall miss latency")
        .flags(total | nozero | nonan)
        ;
    overallAvgMissLatency = overallMissLatency / overallMisses;
    for (int i = 0; i < system->maxMasters(); i++) {
        overallAvgMissLatency.subname(i, system->getMasterName(i));
    }

    blocked_cycles.init(NUM_BLOCKED_CAUSES);
    blocked_cycles
        .name(name() + ".blocked_cycles")
        .desc("number of cycles access was blocked")
        .subname(Blocked_NoMSHRs, "no_mshrs")
        .subname(Blocked_NoTargets, "no_targets")
        ;


    blocked_causes.init(NUM_BLOCKED_CAUSES);
    blocked_causes
        .name(name() + ".blocked")
        .desc("number of cycles access was blocked")
        .subname(Blocked_NoMSHRs, "no_mshrs")
        .subname(Blocked_NoTargets, "no_targets")
        ;

    avg_blocked
        .name(name() + ".avg_blocked_cycles")
        .desc("average number of cycles each access was blocked")
        .subname(Blocked_NoMSHRs, "no_mshrs")
        .subname(Blocked_NoTargets, "no_targets")
        ;

    avg_blocked = blocked_cycles / blocked_causes;

    unusedPrefetches
        .name(name() + ".unused_prefetches")
        .desc("number of HardPF blocks evicted w/o reference")
        .flags(nozero)
        ;

    writebacks
        .init(system->maxMasters())
        .name(name() + ".writebacks")
        .desc("number of writebacks")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system->maxMasters(); i++) {
        writebacks.subname(i, system->getMasterName(i));
    }

    // MSHR statistics
    // MSHR hit statistics
    for (int access_idx = 0; access_idx < MemCmd::NUM_MEM_CMDS; ++access_idx) {
        MemCmd cmd(access_idx);
        const string &cstr = cmd.toString();

        mshr_hits[access_idx]
            .init(system->maxMasters())
            .name(name() + "." + cstr + "_mshr_hits")
            .desc("number of " + cstr + " MSHR hits")
            .flags(total | nozero | nonan)
            ;
        for (int i = 0; i < system->maxMasters(); i++) {
            mshr_hits[access_idx].subname(i, system->getMasterName(i));
        }
    }

    demandMshrHits
        .name(name() + ".demand_mshr_hits")
        .desc("number of demand (read+write) MSHR hits")
        .flags(total | nozero | nonan)
        ;
    demandMshrHits = SUM_DEMAND(mshr_hits);
    for (int i = 0; i < system->maxMasters(); i++) {
        demandMshrHits.subname(i, system->getMasterName(i));
    }

    overallMshrHits
        .name(name() + ".overall_mshr_hits")
        .desc("number of overall MSHR hits")
        .flags(total | nozero | nonan)
        ;
    overallMshrHits = demandMshrHits + SUM_NON_DEMAND(mshr_hits);
    for (int i = 0; i < system->maxMasters(); i++) {
        overallMshrHits.subname(i, system->getMasterName(i));
    }

    // MSHR miss statistics
    for (int access_idx = 0; access_idx < MemCmd::NUM_MEM_CMDS; ++access_idx) {
        MemCmd cmd(access_idx);
        const string &cstr = cmd.toString();

        mshr_misses[access_idx]
            .init(system->maxMasters())
            .name(name() + "." + cstr + "_mshr_misses")
            .desc("number of " + cstr + " MSHR misses")
            .flags(total | nozero | nonan)
            ;
        for (int i = 0; i < system->maxMasters(); i++) {
            mshr_misses[access_idx].subname(i, system->getMasterName(i));
        }
    }

    demandMshrMisses
        .name(name() + ".demand_mshr_misses")
        .desc("number of demand (read+write) MSHR misses")
        .flags(total | nozero | nonan)
        ;
    demandMshrMisses = SUM_DEMAND(mshr_misses);
    for (int i = 0; i < system->maxMasters(); i++) {
        demandMshrMisses.subname(i, system->getMasterName(i));
    }

    overallMshrMisses
        .name(name() + ".overall_mshr_misses")
        .desc("number of overall MSHR misses")
        .flags(total | nozero | nonan)
        ;
    overallMshrMisses = demandMshrMisses + SUM_NON_DEMAND(mshr_misses);
    for (int i = 0; i < system->maxMasters(); i++) {
        overallMshrMisses.subname(i, system->getMasterName(i));
    }

    // MSHR miss latency statistics
    for (int access_idx = 0; access_idx < MemCmd::NUM_MEM_CMDS; ++access_idx) {
        MemCmd cmd(access_idx);
        const string &cstr = cmd.toString();

        mshr_miss_latency[access_idx]
            .init(system->maxMasters())
            .name(name() + "." + cstr + "_mshr_miss_latency")
            .desc("number of " + cstr + " MSHR miss cycles")
            .flags(total | nozero | nonan)
            ;
        for (int i = 0; i < system->maxMasters(); i++) {
            mshr_miss_latency[access_idx].subname(i, system->getMasterName(i));
        }
    }

    demandMshrMissLatency
        .name(name() + ".demand_mshr_miss_latency")
        .desc("number of demand (read+write) MSHR miss cycles")
        .flags(total | nozero | nonan)
        ;
    demandMshrMissLatency = SUM_DEMAND(mshr_miss_latency);
    for (int i = 0; i < system->maxMasters(); i++) {
        demandMshrMissLatency.subname(i, system->getMasterName(i));
    }

    overallMshrMissLatency
        .name(name() + ".overall_mshr_miss_latency")
        .desc("number of overall MSHR miss cycles")
        .flags(total | nozero | nonan)
        ;
    overallMshrMissLatency =
        demandMshrMissLatency + SUM_NON_DEMAND(mshr_miss_latency);
    for (int i = 0; i < system->maxMasters(); i++) {
        overallMshrMissLatency.subname(i, system->getMasterName(i));
    }

    // MSHR uncacheable statistics
    for (int access_idx = 0; access_idx < MemCmd::NUM_MEM_CMDS; ++access_idx) {
        MemCmd cmd(access_idx);
        const string &cstr = cmd.toString();

        mshr_uncacheable[access_idx]
            .init(system->maxMasters())
            .name(name() + "." + cstr + "_mshr_uncacheable")
            .desc("number of " + cstr + " MSHR uncacheable")
            .flags(total | nozero | nonan)
            ;
        for (int i = 0; i < system->maxMasters(); i++) {
            mshr_uncacheable[access_idx].subname(i, system->getMasterName(i));
        }
    }

    overallMshrUncacheable
        .name(name() + ".overall_mshr_uncacheable_misses")
        .desc("number of overall MSHR uncacheable misses")
        .flags(total | nozero | nonan)
        ;
    overallMshrUncacheable =
        SUM_DEMAND(mshr_uncacheable) + SUM_NON_DEMAND(mshr_uncacheable);
    for (int i = 0; i < system->maxMasters(); i++) {
        overallMshrUncacheable.subname(i, system->getMasterName(i));
    }

    // MSHR miss latency statistics
    for (int access_idx = 0; access_idx < MemCmd::NUM_MEM_CMDS; ++access_idx) {
        MemCmd cmd(access_idx);
        const string &cstr = cmd.toString();

        mshr_uncacheable_lat[access_idx]
            .init(system->maxMasters())
            .name(name() + "." + cstr + "_mshr_uncacheable_latency")
            .desc("number of " + cstr + " MSHR uncacheable cycles")
            .flags(total | nozero | nonan)
            ;
        for (int i = 0; i < system->maxMasters(); i++) {
            mshr_uncacheable_lat[access_idx].subname(
                i, system->getMasterName(i));
        }
    }

    overallMshrUncacheableLatency
        .name(name() + ".overall_mshr_uncacheable_latency")
        .desc("number of overall MSHR uncacheable cycles")
        .flags(total | nozero | nonan)
        ;
    overallMshrUncacheableLatency =
        SUM_DEMAND(mshr_uncacheable_lat) +
        SUM_NON_DEMAND(mshr_uncacheable_lat);
    for (int i = 0; i < system->maxMasters(); i++) {
        overallMshrUncacheableLatency.subname(i, system->getMasterName(i));
    }

#if 0
    // MSHR access formulas
    for (int access_idx = 0; access_idx < MemCmd::NUM_MEM_CMDS; ++access_idx) {
        MemCmd cmd(access_idx);
        const string &cstr = cmd.toString();

        mshrAccesses[access_idx]
            .name(name() + "." + cstr + "_mshr_accesses")
            .desc("number of " + cstr + " mshr accesses(hits+misses)")
            .flags(total | nozero | nonan)
            ;
        mshrAccesses[access_idx] =
            mshr_hits[access_idx] + mshr_misses[access_idx]
            + mshr_uncacheable[access_idx];
    }

    demandMshrAccesses
        .name(name() + ".demand_mshr_accesses")
        .desc("number of demand (read+write) mshr accesses")
        .flags(total | nozero | nonan)
        ;
    demandMshrAccesses = demandMshrHits + demandMshrMisses;

    overallMshrAccesses
        .name(name() + ".overall_mshr_accesses")
        .desc("number of overall (read+write) mshr accesses")
        .flags(total | nozero | nonan)
        ;
    overallMshrAccesses = overallMshrHits + overallMshrMisses
        + overallMshrUncacheable;
#endif

    // MSHR miss rate formulas
    for (int access_idx = 0; access_idx < MemCmd::NUM_MEM_CMDS; ++access_idx) {
        MemCmd cmd(access_idx);
        const string &cstr = cmd.toString();

        mshrMissRate[access_idx]
            .name(name() + "." + cstr + "_mshr_miss_rate")
            .desc("mshr miss rate for " + cstr + " accesses")
            .flags(total | nozero | nonan)
            ;
        mshrMissRate[access_idx] =
            mshr_misses[access_idx] / accesses[access_idx];

        for (int i = 0; i < system->maxMasters(); i++) {
            mshrMissRate[access_idx].subname(i, system->getMasterName(i));
        }
    }

    demandMshrMissRate
        .name(name() + ".demand_mshr_miss_rate")
        .desc("mshr miss rate for demand accesses")
        .flags(total | nozero | nonan)
        ;
    demandMshrMissRate = demandMshrMisses / demandAccesses;
    for (int i = 0; i < system->maxMasters(); i++) {
        demandMshrMissRate.subname(i, system->getMasterName(i));
    }

    overallMshrMissRate
        .name(name() + ".overall_mshr_miss_rate")
        .desc("mshr miss rate for overall accesses")
        .flags(total | nozero | nonan)
        ;
    overallMshrMissRate = overallMshrMisses / overallAccesses;
    for (int i = 0; i < system->maxMasters(); i++) {
        overallMshrMissRate.subname(i, system->getMasterName(i));
    }

    // mshrMiss latency formulas
    for (int access_idx = 0; access_idx < MemCmd::NUM_MEM_CMDS; ++access_idx) {
        MemCmd cmd(access_idx);
        const string &cstr = cmd.toString();

        avgMshrMissLatency[access_idx]
            .name(name() + "." + cstr + "_avg_mshr_miss_latency")
            .desc("average " + cstr + " mshr miss latency")
            .flags(total | nozero | nonan)
            ;
        avgMshrMissLatency[access_idx] =
            mshr_miss_latency[access_idx] / mshr_misses[access_idx];

        for (int i = 0; i < system->maxMasters(); i++) {
            avgMshrMissLatency[access_idx].subname(
                i, system->getMasterName(i));
        }
    }

    demandAvgMshrMissLatency
        .name(name() + ".demand_avg_mshr_miss_latency")
        .desc("average overall mshr miss latency")
        .flags(total | nozero | nonan)
        ;
    demandAvgMshrMissLatency = demandMshrMissLatency / demandMshrMisses;
    for (int i = 0; i < system->maxMasters(); i++) {
        demandAvgMshrMissLatency.subname(i, system->getMasterName(i));
    }

    overallAvgMshrMissLatency
        .name(name() + ".overall_avg_mshr_miss_latency")
        .desc("average overall mshr miss latency")
        .flags(total | nozero | nonan)
        ;
    overallAvgMshrMissLatency = overallMshrMissLatency / overallMshrMisses;
    for (int i = 0; i < system->maxMasters(); i++) {
        overallAvgMshrMissLatency.subname(i, system->getMasterName(i));
    }

    // mshrUncacheable latency formulas
    for (int access_idx = 0; access_idx < MemCmd::NUM_MEM_CMDS; ++access_idx) {
        MemCmd cmd(access_idx);
        const string &cstr = cmd.toString();

        avgMshrUncacheableLatency[access_idx]
            .name(name() + "." + cstr + "_avg_mshr_uncacheable_latency")
            .desc("average " + cstr + " mshr uncacheable latency")
            .flags(total | nozero | nonan)
            ;
        avgMshrUncacheableLatency[access_idx] =
            mshr_uncacheable_lat[access_idx] / mshr_uncacheable[access_idx];

        for (int i = 0; i < system->maxMasters(); i++) {
            avgMshrUncacheableLatency[access_idx].subname(
                i, system->getMasterName(i));
        }
    }

    overallAvgMshrUncacheableLatency
        .name(name() + ".overall_avg_mshr_uncacheable_latency")
        .desc("average overall mshr uncacheable latency")
        .flags(total | nozero | nonan)
        ;
    overallAvgMshrUncacheableLatency =
        overallMshrUncacheableLatency / overallMshrUncacheable;
    for (int i = 0; i < system->maxMasters(); i++) {
        overallAvgMshrUncacheableLatency.subname(i, system->getMasterName(i));
    }
    /* MJL_Begin */
    MJL_overallRowVecMisses
        .name(name() + ".MJL_overallRowVecMisses")
        .desc("number of overall vector misses with row preference")
        .flags(nozero)
        ;

    MJL_overallColVecMisses
        .name(name() + ".MJL_overallColVecMisses")
        .desc("number of overall vector misses with column preference")
        .flags(nozero)
        ;
    
    MJL_overallVecMisses
        .name(name() + ".MJL_overallVecMisses")
        .desc("number of overall vector misses")
        .flags(total | nozero)
        ;
    MJL_overallVecMisses = MJL_overallRowVecMisses + MJL_overallColVecMisses;

    MJL_overallRowVecHits
        .name(name() + ".MJL_overallRowVecHits")
        .desc("number of overall vector hits with row preference")
        .flags(nozero)
        ;

    MJL_overallColVecHits
        .name(name() + ".MJL_overallColVecHits")
        .desc("number of overall vector hits with column preference")
        .flags(nozero)
        ;
    
    MJL_overallVecHits
        .name(name() + ".MJL_overallVecHits")
        .desc("number of overall vector hits")
        .flags(total | nozero)
        ;
    MJL_overallVecHits = MJL_overallRowVecHits + MJL_overallColVecHits;

    MJL_overallRowVecAccesses
        .name(name() + ".MJL_overallRowVecAccesses")
        .desc("number of overall vector accesses with row preference")
        .flags(total | nozero)
        ;
    MJL_overallRowVecAccesses = MJL_overallRowVecHits + MJL_overallRowVecMisses;

    MJL_overallColVecAccesses
        .name(name() + ".MJL_overallColVecAccesses")
        .desc("number of overall vector accesses with column preference")
        .flags(total | nozero)
        ;
    MJL_overallColVecAccesses = MJL_overallColVecHits + MJL_overallColVecMisses;
    
    MJL_overallVecAccesses
        .name(name() + ".MJL_overallVecAccesses")
        .desc("number of overall vector accesses")
        .flags(total | nozero)
        ;
    MJL_overallVecAccesses = MJL_overallRowVecAccesses + MJL_overallColVecAccesses;

    MJL_overallRowMisses
        .name(name() + ".MJL_overallRowMisses")
        .desc("number of overall misses with row preference")
        .flags(nozero)
        ;

    MJL_overallColumnMisses
        .name(name() + ".MJL_overallColumnMisses")
        .desc("number of overall misses with column preference")
        .flags(nozero)
        ;

    MJL_overallRowHits
        .name(name() + ".MJL_overallRowHits")
        .desc("number of overall hits with row preference")
        .flags(nozero)
        ;

    MJL_overallColumnHits
        .name(name() + ".MJL_overallColumnHits")
        .desc("number of overall hits with column preference")
        .flags(nozero)
        ;

    MJL_overallRowAccesses
        .name(name() + ".MJL_overallRowAccesses")
        .desc("number of overall accesses with row preference")
        .flags(total | nozero | nonan)
        ;
    MJL_overallRowAccesses = MJL_overallRowHits + MJL_overallRowMisses;

    MJL_overallColumnAccesses
        .name(name() + ".MJL_overallColumnAccesses")
        .desc("number of overall accesses with column preference")
        .flags(total | nozero | nonan)
        ;
    MJL_overallColumnAccesses = MJL_overallColumnHits + MJL_overallColumnMisses;

    MJL_rowAccColHit
        .name(name() + ".MJL_rowAccColHit")
        .desc("number of column hits with row data")
        .flags(nozero)
        ;
    
     MJL_colAccRowHit
        .name(name() + ".MJL_colAccRowHit")
        .desc("number of row hits with column data")
        .flags(nozero)
        ;

    MJL_conflictWBCount1
        .name(name() + ".MJL_conflictWBCount1")
        .desc("number of writebacks that are caused by conflict accesses")
        .flags(nozero)
        ;
    MJL_conflictWBCount2
        .name(name() + ".MJL_conflictWBCount2")
        .desc("number of writebacks that are caused by conflict accesses")
        .flags(nozero)
        ;
    MJL_conflictWBCount3
        .name(name() + ".MJL_conflictWBCount3")
        .desc("number of writebacks that are caused by conflict accesses")
        .flags(nozero)
        ;
    MJL_conflictWBCount4
        .name(name() + ".MJL_conflictWBCount4")
        .desc("number of writebacks that are caused by conflict accesses")
        .flags(nozero)
        ;
    
    MJL_mshrConflictCount
        .name(name() + ".MJL_mshrConflictCount")
        .desc("number of mshr conflicts (conflict access are strictly in order, may cause blocking)")
        .flags(nozero)
        ;

    MJL_requestedBytes
        .name(name() + ".MJL_requestedBytes")
        .desc("total number of bytes requested by this cache")
        .flags(nozero)
        ;

    MJL_touchedBytes
        .name(name() + ".MJL_touchedBytes")
        .desc("total number of bytes touched in this cache at the time of eviction")
        .flags(nozero)
        ;

    MJL_overallInterestedRowMisses
        .name(name() + ".MJL_overallInterestedRowMisses")
        .desc("number of overall row misses that are caused by interested instructions")
        .flags(nozero)
        ;
    
    MJL_overallInterestedRowHits
        .name(name() + ".MJL_overallInterestedRowHits")
        .desc("number of overall of row hits that are caused by interested instructions")
        .flags(nozero)
        ;

    MJL_overallInterestedColumnMisses
        .name(name() + ".MJL_overallInterestedColumnMisses")
        .desc("number of overall column misses that are caused by interested instructions")
        .flags(nozero)
        ;

    MJL_overallInterestedColumnHits
        .name(name() + ".MJL_overallInterestedColumnHits")
        .desc("number of overall column hits that are caused by interested instructions")
        .flags(nozero)
        ;
    
    MJL_overallInterestedMisses
        .name(name() + ".MJL_overallInterestedMisses")
        .desc("number of overall misses caused by interested instruction")
        .flags(total | nozero | nonan)
        ;
    MJL_overallInterestedMisses = MJL_overallInterestedRowMisses + MJL_overallInterestedColumnMisses;

    MJL_overallInterestedHits
        .name(name() + ".MJL_overallInterestedHits")
        .desc("number of overall hits caused by interested instruction")
        .flags(total | nozero | nonan)
        ;
    MJL_overallInterestedHits = MJL_overallInterestedRowHits + MJL_overallInterestedColumnHits;

    MJL_overallInterestedRowAccesses
        .name(name() + ".MJL_overallInterestedRowAccesses")
        .desc("number of overall row accesses caused by interested instruction")
        .flags(total | nozero | nonan)
        ;
    MJL_overallInterestedRowAccesses = MJL_overallInterestedRowHits + MJL_overallInterestedRowMisses;

    MJL_overallInterestedColumnAccesses
        .name(name() + ".MJL_overallInterestedColumnAccesses")
        .desc("number of overall column accesses caused by interested instruction")
        .flags(total | nozero | nonan)
        ;
    MJL_overallInterestedColumnAccesses = MJL_overallInterestedColumnHits + MJL_overallInterestedColumnMisses;

    MJL_overallInterestedAccesses
        .name(name() + ".MJL_overallInterestedAccesses")
        .desc("number of overall accesses caused by interested instruction")
        .flags(total | nozero | nonan)
        ;
    MJL_overallInterestedAccesses = MJL_overallInterestedColumnAccesses + MJL_overallInterestedRowAccesses;

    MJL_crossReadHitWords
        .name(name() + ".MJL_crossReadHitWords")
        .desc("number of overall demanded words in missed reads that has cross hit")
        .flags(nozero | nonan)
        ;

    MJL_crossReadHits
        .name(name() + ".MJL_crossReadHits")
        .desc("number of overall missed reads that has partial cross hit")
        .flags(nozero | nonan)
        ;

    MJL_crossFullReadHits
        .name(name() + ".MJL_crossFullReadHits")
        .desc("number of overall missed reads that has full cross hit")
        .flags(nozero | nonan)
        ;

    MJL_crossWriteHitWords
        .name(name() + ".MJL_crossWriteHitWords")
        .desc("number of overall demanded words in missed writes that has cross hit")
        .flags(nozero | nonan)
        ;

    MJL_crossWriteHits
        .name(name() + ".MJL_crossWriteHits")
        .desc("number of overall missed writes that has partial cross hit")
        .flags(nozero | nonan)
        ;

    MJL_crossFullWriteHits
        .name(name() + ".MJL_crossFullWriteHits")
        .desc("number of overall missed writes that has full cross hit")
        .flags(nozero | nonan)
        ;

    MJL_crossHitWords
        .name(name() + ".MJL_crossHitWords")
        .desc("number of overall demanded words in misses that has cross hit")
        .flags(nozero | nonan)
        ;
    MJL_crossHitWords = MJL_crossReadHitWords + MJL_crossWriteHitWords;
    
    MJL_crossHits
        .name(name() + ".MJL_crossHits")
        .desc("number of overall misses that has partial cross hit")
        .flags(nozero | nonan)
        ;
    MJL_crossHits = MJL_crossReadHits + MJL_crossWriteHits;
    
    MJL_crossFullHits
        .name(name() + ".MJL_crossFullHits")
        .desc("number of overall misses that has full cross hit")
        .flags(nozero | nonan)
        ;
    MJL_crossFullHits = MJL_crossFullReadHits + MJL_crossFullWriteHits;

    MJL_bloomFilterFalsePositives
        .name(name() + ".MJL_bloomFilterFalsePositives")
        .desc("number of false positives in the bloom filter")
        .flags(nonan)
        ;

    MJL_bloomFilterTruePositives
        .name(name() + ".MJL_bloomFilterTruePositives")
        .desc("number of true positives in the bloom filter")
        .flags(nonan)
        ;

    MJL_bloomFilterTrueNegatives
        .name(name() + ".MJL_bloomFilterTrueNegatives")
        .desc("number of true negatives in the bloom filter")
        .flags(nonan)
        ;

    MJL_predTrue
        .name(name() + ".MJL_predTrue")
        .desc("number of misses with predicted direction that is the same as oracle direction")
        .flags(nozero | nonan)
        ;

    MJL_predFalse
        .name(name() + ".MJL_predFalse")
        .desc("number of misses with predicted direction that is different from oracle direction")
        .flags(nozero | nonan)
        ;
    
    MJL_unusedRowPrefetches
        .name(name() + ".MJL_unusedRowPrefetches")
        .desc("number of HardPF row blocks evicted w/o reference")
        .flags(nozero)
        ;

    MJL_unusedColumnPrefetches
        .name(name() + ".MJL_unusedColumnPrefetches")
        .desc("number of HardPF column blocks evicted w/o reference")
        .flags(nozero)
        ;

    MJL_numOfPredicts
        .name(name() + ".MJL_numOfPredicts")
        .desc("number of orientation predictions")
        .flags(nozero)
        ;

    MJL_numOfMSHRPredicts
        .name(name() + ".MJL_numOfMSHRPredicts")
        .desc("number of mshr based orientation predictions")
        .flags(nozero)
        ;

    MJL_numOfPfPredicts
        .name(name() + ".MJL_numOfPfPredicts")
        .desc("number of prefetcher based orientation predictions")
        .flags(nozero)
        ;
    
    MJL_numOfDiffPredicts
        .name(name() + ".MJL_numOfDiffPredicts")
        .desc("number of times where mshr based and prefetcher based orientation predictions differ")
        .flags(nozero)
        ;

    MJL_numOfWarmupPredicts_MSHR
        .name(name() + ".MJL_numOfWarmupPredicts_MSHR")
        .desc("number of times where mshr based orientation prediction is still invalid")
        .flags(nozero)
        ;

    MJL_numOfWarmupPredicts_Pf
        .name(name() + ".MJL_numOfWarmupPredicts_Pf")
        .desc("number of times where prefetcher based orientation prediction is still invalid")
        .flags(nozero)
        ;

    MJL_numOfCopyMSHRQueueEvictNonResolved
        .name(name() + ".MJL_numOfCopyMSHRQueueEvictNonResolved")
        .desc("number of evictions from copyMSHRQueue where the corresponding MSHR entry hasn't been released yet")
        .flags(nozero)
        ;

    MJL_pfDropped
        .init(11)
        .name(name() + ".MJL_pfDropped")
        .desc("Number of prefetches dropped due to coherence complications")
        .flags(total | nozero | nonan)
        ;
    MJL_pfDropped.subname(0, "inCache");
    MJL_pfDropped.subname(1, "crossInCache");
    MJL_pfDropped.subname(2, "crossDirtyInCache");
    MJL_pfDropped.subname(3, "crossDirtyOrWritableInCache");
    MJL_pfDropped.subname(4, "inMSHR");
    MJL_pfDropped.subname(5, "crossInMSHR");
    MJL_pfDropped.subname(6, "inWriteBuffer");
    MJL_pfDropped.subname(7, "crossInWriteBuffer");
    MJL_pfDropped.subname(8, "1DCoherence");
    MJL_pfDropped.subname(9, "MDACoherence");
    MJL_pfDropped.subname(10, "hasCrossWritableRevoked");

    MJL_untouchedPrefetches
        .name(name() + ".MJL_untouchedPrefetches")
        .desc("number of prefetched cache lines that were never touched by demand requests")
        .flags(nozero)
        ;
    /* MJL_End */

}
