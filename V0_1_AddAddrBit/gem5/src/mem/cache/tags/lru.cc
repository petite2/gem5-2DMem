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
 * Copyright (c) 2003-2005,2014 The Regents of The University of Michigan
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
 * Definitions of a LRU tag store.
 */

#include "mem/cache/tags/lru.hh"

#include "debug/CacheRepl.hh"
#include "mem/cache/base.hh"

LRU::LRU(const Params *p)
    : BaseSetAssoc(p)
{
}

CacheBlk*
LRU::accessBlock(Addr addr, bool is_secure, Cycles &lat, int master_id)
{
    CacheBlk *blk = BaseSetAssoc::accessBlock(addr, is_secure, lat, master_id);

    if (blk != nullptr) {
        // move this block to head of the MRU list
        sets[blk->set].moveToHead(blk);
        DPRINTF(CacheRepl, "set %x: moving blk %x (%s) to MRU\n",
                blk->set, regenerateBlkAddr(blk->tag, blk->set),
                is_secure ? "s" : "ns");
    }

    return blk;
}
/* MJL_Begin */
CacheBlk*
LRU::MJL_accessBlock(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure, Cycles &lat, int master_id)
{
    CacheBlk *blk = BaseSetAssoc::MJL_accessBlock(addr, MJL_cacheBlkDir, is_secure, lat, master_id);

    if (cache->MJL_is2DCache() && blk != nullptr) {
        int MJL_startOffset = blk->set%sizeof(uint64_t);
        int MJL_way = blk->way;

        for (int i = 0; i < getBlockSize()/sizeof(uint64_t); ++i) {
            sets[blk->set + i - MJL_startOffset].moveToHead(BaseSetAssoc::findBlockBySetAndWay(blk->set + i - MJL_startOffset, MJL_way));
        }
    } else {
        if (blk != nullptr) {
            // move this block to head of the MRU list
            sets[blk->set].moveToHead(blk);
            DPRINTF(CacheRepl, "set %x: moving blk %x (%s) to MRU\n",
                    blk->set, MJL_regenerateBlkAddr(blk->tag, blk->MJL_blkDir, blk->set),
                    is_secure ? "s" : "ns");
        }
    }

    return blk;
}

CacheBlk*
LRU::MJL_accessCrossBlock(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure, Cycles &lat,
                          int context_src, unsigned MJL_offset)
{
    CacheBlk *blk = BaseSetAssoc::MJL_accessCrossBlock(addr, MJL_cacheBlkDir, is_secure, lat, context_src, MJL_offset);

    if (cache->MJL_is2DCache() && blk != nullptr) {
        int MJL_startOffset = blk->set%sizeof(uint64_t);
        int MJL_way = blk->way;

        for (int i = 0; i < getBlockSize()/sizeof(uint64_t); ++i) {
            assert(blk->tag == BaseSetAssoc::findBlockBySetAndWay(blk->set + i - MJL_startOffset, MJL_way)->tag || (!BaseSetAssoc::findBlockBySetAndWay(blk->set + i - MJL_startOffset, MJL_way)->isValid() && !BaseSetAssoc::findBlockBySetAndWay(blk->set + i - MJL_startOffset, MJL_way)->MJL_hasCrossValid()));
            sets[blk->set + i - MJL_startOffset].moveToHead(BaseSetAssoc::findBlockBySetAndWay(blk->set + i - MJL_startOffset, MJL_way));
        }
    } else {
        if (blk != nullptr) {
            // move this block to head of the MRU list
            sets[blk->set].moveToHead(blk);
            DPRINTF(CacheRepl, "set %x: moving blk %x (%s) to MRU\n",
                    blk->set, MJL_regenerateBlkAddr(blk->tag, blk->MJL_blkDir, blk->set),
                    is_secure ? "s" : "ns");
        }
    }

    return blk;
}

CacheBlk*
LRU::MJL_accessBlockOneWord(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure, Cycles &lat,
                          int master_id)
{
    CacheBlk *blk = BaseSetAssoc::MJL_accessBlockOneWord(addr, MJL_cacheBlkDir, is_secure, lat, master_id);

    if (blk != nullptr) {
        // move this block to head of the MRU list
        sets[blk->set].moveToHead(blk);
        DPRINTF(CacheRepl, "set %x: moving blk %x (%s) to MRU\n",
                blk->set, MJL_regenerateBlkAddr(blk->tag, blk->MJL_blkDir, blk->set),
                is_secure ? "s" : "ns");
    }

    return blk;
}
/* MJL_End */

CacheBlk*
LRU::findVictim(Addr addr)
{
    int set = extractSet(addr);
    // grab a replacement candidate
    BlkType *blk = nullptr;
    for (int i = assoc - 1; i >= 0; i--) {
        BlkType *b = sets[set].blks[i];
        if (b->way < allocAssoc) {
            blk = b;
            break;
        }
    }
    assert(!blk || blk->way < allocAssoc);

    if (blk && blk->isValid()) {
        DPRINTF(CacheRepl, "set %x: selecting blk %x for replacement\n",
        /* MJL_Begin */
                set, MJL_regenerateBlkAddr(blk->tag, blk->MJL_blkDir, set));
        /* MJL_End */
        /* MJL_Comment
                set, regenerateBlkAddr(blk->tag, set));
        */
    }

    return blk;
}
/* MJL_Begin */
CacheBlk*
LRU::MJL_findVictim(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir)
{
    if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos || this->name().find("l3") != std::string::npos) {
        int set = MJL_extractSet(addr, MJL_cacheBlkDir);
        // grab a replacement candidate
        BlkType *blk = nullptr;
        for (int i = assoc - 1; i >= 0; i--) {
            BlkType *b = sets[set].blks[i];
            if (b->way < allocAssoc) {
                blk = b;
                break;
            }
        }
        assert(!blk || blk->way < allocAssoc);

        if (blk && blk->isValid()) {
            DPRINTF(CacheRepl, "set %x: selecting blk %x for replacement\n",
                    set, MJL_regenerateBlkAddr(blk->tag, blk->MJL_blkDir, set));
        }

        return blk;
    } else {
        return findVictim(addr);
    }
}
/* MJL_End */

void
LRU::insertBlock(PacketPtr pkt, BlkType *blk)
{
    BaseSetAssoc::insertBlock(pkt, blk);

    /* MJL_Begin */
    if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos || this->name().find("l3") != std::string::npos) {
        int set = MJL_extractSet(pkt->getAddr(), pkt->MJL_getDataDir());
        if (cache->MJL_is2DCache()) {
            set = MJL_extractSet(pkt->getAddr(), MemCmd::MJL_DirAttribute::MJL_IsRow);
            int MJL_startOffset = set%sizeof(uint64_t);
            int MJL_way = blk->way;

            for (int i = 0; i < getBlockSize()/sizeof(uint64_t); ++i) {
                assert(blk->tag == BaseSetAssoc::findBlockBySetAndWay(set + i - MJL_startOffset, MJL_way)->tag || (!BaseSetAssoc::findBlockBySetAndWay(set + i - MJL_startOffset, MJL_way)->isValid() && !BaseSetAssoc::findBlockBySetAndWay(set + i - MJL_startOffset, MJL_way)->MJL_hasCrossValid()));
                sets[set + i - MJL_startOffset].moveToHead(BaseSetAssoc::findBlockBySetAndWay(set + i - MJL_startOffset, MJL_way));
            };
        } else {
            sets[set].moveToHead(blk);
        }
    } else {
        int set = extractSet(pkt->getAddr());
        sets[set].moveToHead(blk);
    }
    /* MJL_End */
    /* MJL_Comment
    int set = extractSet(pkt->getAddr());
    sets[set].moveToHead(blk);
    */
}

// MJL_TODO: Needs to change for 2D Cache?
void
LRU::invalidate(CacheBlk *blk)
{
    BaseSetAssoc::invalidate(blk);

    // should be evicted before valid blocks
    int set = blk->set;
    sets[set].moveToTail(blk);
}

LRU*
LRUParams::create()
{
    return new LRU(this);
}
