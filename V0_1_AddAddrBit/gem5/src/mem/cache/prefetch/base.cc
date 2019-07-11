/*
 * Copyright (c) 2013-2014 ARM Limited
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
 *          Mitch Hayenga
 */

/**
 * @file
 * Hardware Prefetcher Definition.
 */

#include <list>

#include "base/intmath.hh"
#include "mem/cache/prefetch/base.hh"
#include "mem/cache/base.hh"
#include "sim/system.hh"

BasePrefetcher::BasePrefetcher(const BasePrefetcherParams *p)
    : ClockedObject(p), cache(nullptr), blkSize(0), lBlkSize(0),
      system(p->sys), onMiss(p->on_miss), onRead(p->on_read),
      onWrite(p->on_write), onData(p->on_data), onInst(p->on_inst),/* MJL_Begin */
      MJL_colPf(p->MJL_colPf), MJL_colPageSize(p->MJL_colPageSize),
      MJL_pfBasedPredictDir(p->MJL_pfBasedPredictDir),/* MJL_End */
      masterId(system->getMasterId(name())),
      pageBytes(system->getPageBytes())
{
}

void
BasePrefetcher::setCache(BaseCache *_cache)
{
    assert(!cache);
    cache = _cache;
    blkSize = cache->getBlockSize();
    lBlkSize = floorLog2(blkSize);
}

void
BasePrefetcher::regStats()
{
    ClockedObject::regStats();

    pfIssued
        .name(name() + ".num_hwpf_issued")
        .desc("number of hwpf issued")
        ;

}

bool
BasePrefetcher::observeAccess(const PacketPtr &pkt) const
{
    Addr addr = pkt->getAddr();
    bool fetch = pkt->req->isInstFetch();
    bool read = pkt->isRead();
    bool inv = pkt->isInvalidate();
    bool is_secure = pkt->isSecure();

    if (pkt->req->isUncacheable()) return false;
    if (fetch && !onInst) return false;
    if (!fetch && !onData) return false;
    if (!fetch && read && !onRead) return false;
    if (!fetch && !read && !onWrite) return false;
    if (!fetch && !read && inv) return false;
    if (pkt->cmd == MemCmd::CleanEvict) return false;

    if (onMiss) {
        /* MJL_Begin */
        return !MJL_inCache(addr, pkt->MJL_getCmdDir(), is_secure) &&
               !MJL_inMissQueue(addr, pkt->MJL_getCmdDir(), is_secure);
        /* MJL_End */
        /* MJL_Comment
        return !inCache(addr, is_secure) &&
               !inMissQueue(addr, is_secure);
        */
    }

    return true;
}

bool
BasePrefetcher::inCache(Addr addr, bool is_secure) const
{
    if (cache->inCache(addr, is_secure)) {
        return true;
    }
    return false;
}

/* MJL_Begin */
bool
BasePrefetcher::MJL_inCache(Addr addr, MemCmd::MJL_DirAttribute MJL_cacheBlkDir, bool is_secure) const
{
    if (cache->MJL_inCache(addr, MJL_cacheBlkDir, is_secure)) {
        return true;
    }
    return false;
}
/* MJL_End */

bool
BasePrefetcher::inMissQueue(Addr addr, bool is_secure) const
{
    if (cache->inMissQueue(addr, is_secure)) {
        return true;
    }
    return false;
}

/* MJL_Begin */
bool
BasePrefetcher::MJL_inMissQueue(Addr addr, MemCmd::MJL_DirAttribute MJL_cacheBlkDir, bool is_secure) const
{
    if (cache->MJL_inMissQueue(addr, MJL_cacheBlkDir, is_secure)) {
        return true;
    }
    return false;
}

bool
BasePrefetcher::MJL_crossDirtyInCache(const PacketPtr &pkt) const
{
    if (cache->MJL_crossDirtyInCache(pkt)) {
        return true;
    }
    return false;
}

bool
BasePrefetcher::MJL_crossDirtyInMissQueue(const PacketPtr &pkt) const
{
    if (cache->MJL_crossDirtyInMissQueue(pkt)) {
        return true;
    }
    return false;
}

bool
BasePrefetcher::MJL_crossDirtyInWriteBuffer(const PacketPtr &pkt) const
{
    if (cache->MJL_crossDirtyInWriteBuffer(pkt)) {
        return true;
    }
    return false;
}

bool
BasePrefetcher::MJL_crossDirtyInCache(Addr addr, MemCmd::MJL_DirAttribute MJL_cacheBlkDir, bool is_secure) const
{
    if (cache->MJL_crossDirtyInCache(addr, MJL_cacheBlkDir, is_secure)) {
        return true;
    }
    return false;
}

bool
BasePrefetcher::MJL_crossDirtyInMissQueue(Addr addr, MemCmd::MJL_DirAttribute MJL_cacheBlkDir, bool is_secure) const
{
    if (cache->MJL_crossDirtyInMissQueue(addr, MJL_cacheBlkDir, is_secure)) {
        return true;
    }
    return false;
}

bool
BasePrefetcher::MJL_crossDirtyInWriteBuffer(Addr addr, MemCmd::MJL_DirAttribute MJL_cacheBlkDir, bool is_secure) const
{
    if (cache->MJL_crossDirtyInWriteBuffer(addr, MJL_cacheBlkDir, is_secure)) {
        return true;
    }
    return false;
}

bool 
BasePrefetcher::MJL_is2DCache() const
{
    return cache->MJL_is2DCache();
}

unsigned 
BasePrefetcher::MJL_getRowWidth() const
{
    return cache->MJL_getRowWidth();
}

bool
BasePrefetcher::MJL_colSamePage(Addr a, Addr b) const
{
    return roundDown(a, MJL_colPageSize) == roundDown(b, MJL_colPageSize);
}

Addr MJL_movColRight(Addr addr) const 
{
    int MJL_rowShift = floorLog2(sizeof(uint64_t));
    int MJL_colShift = floorLog2(MJL_getRowWidth()) + floorLog2(blkSize);
    int MJL_wordShift = floorLog2(blkSize/sizeof(uint64_t));
    uint64_t MJL_highMask = ~((1 << (MJL_colShift + MJL_wordShift)) - 1);
    uint64_t MJL_middleMask = (MJL_getRowWidth() - 1) << floorLog2(blkSize);;
    uint64_t MJL_wordMask = blkSize/sizeof(uint64_t) - 1;
    uint64_t MJL_offsetMask = sizeof(uint64_t) - 1;

    Addr same = addr & ((Addr)MJL_highMask | (Addr)MJL_offsetMask);
    Addr middle = addr & (Addr)MJL_middleMask;
    Addr col = (addr >> MJL_colShift) & (Addr)MJL_wordMask;
    Addr row = (addr >> MJL_rowShift) & (Addr)MJL_wordMask;
    return (same | (middle << MJL_wordShift) | (col << (MJL_wordShift + MJL_rowShift)) | (row << MJL_rowShift));
}

Addr MJL_movColLeft(Addr addr) const 
{
    int MJL_rowShift = floorLog2(sizeof(uint64_t));
    int MJL_colShift = floorLog2(MJL_getRowWidth()) + floorLog2(blkSize);
    int MJL_wordShift = floorLog2(blkSize/sizeof(uint64_t));
    uint64_t MJL_highMask = ~((1 << (MJL_colShift + MJL_wordShift)) - 1);
    uint64_t MJL_middleMask = (MJL_getRowWidth() - 1) << floorLog2(blkSize);;
    uint64_t MJL_wordMask = blkSize/sizeof(uint64_t) - 1;
    uint64_t MJL_offsetMask = sizeof(uint64_t) - 1;

    Addr same = addr & ((Addr)MJL_highMask | (Addr)MJL_offsetMask);
    Addr middle = addr & ((Addr)MJL_middleMask << MJL_wordShift);
    Addr row = (addr >> MJL_rowShift) & (Addr)MJL_wordMask;
    Addr col = (addr >> (MJL_rowShift + MJL_wordShift)) & (Addr)MJL_wordMask;
    return (same | (col << MJL_colShift) | (middle >> MJL_wordShift) | (row << MJL_rowShift));
}

Addr MJL_swapRowColBits(Addr addr) const 
{
    int MJL_rowShift = floorLog2(sizeof(uint64_t));
    uint64_t MJL_wordMask = blkSize/sizeof(uint64_t) - 1;
    int MJL_colShift = floorLog2(MJL_getRowWidth()) + floorLog2(blkSize);

    Addr new_row = (addr >> MJL_colShift) & (Addr)MJL_wordMask;
    Addr new_col = (addr >> MJL_rowShift) & (Addr)MJL_wordMask;
    return ((addr & ~(((Addr)MJL_wordMask << MJL_colShift) | ((Addr)MJL_wordMask << MJL_rowShift))) | (new_row << MJL_rowShift) | (new_col << MJL_colShift));
}
/* MJL_End */

bool
BasePrefetcher::samePage(Addr a, Addr b) const
{
    return roundDown(a, pageBytes) == roundDown(b, pageBytes);
}

Addr
BasePrefetcher::blockAddress(Addr a) const
{
    return a & ~(blkSize-1);
}

Addr
BasePrefetcher::blockIndex(Addr a) const
{
    return a >> lBlkSize;
}

Addr
BasePrefetcher::pageAddress(Addr a) const
{
    return roundDown(a, pageBytes);
}

Addr
BasePrefetcher::pageOffset(Addr a) const
{
    return a & (pageBytes - 1);
}

Addr
BasePrefetcher::pageIthBlockAddress(Addr page, uint32_t blockIndex) const
{
    return page + (blockIndex << lBlkSize);
}
