/*
 * Copyright (c) 2012-2014 ARM Limited
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
 * Definitions of a base set associative tag store.
 */

#include "mem/cache/tags/base_set_assoc.hh"

#include <string>

#include "base/intmath.hh"
#include "sim/core.hh"

using namespace std;

BaseSetAssoc::BaseSetAssoc(const Params *p)
    :BaseTags(p), assoc(p->assoc), allocAssoc(p->assoc),
     numSets(p->size / (p->block_size * p->assoc)),
     sequentialAccess(p->sequential_access)
{
    // Check parameters
    if (blkSize < 4 || !isPowerOf2(blkSize)) {
        fatal("Block size must be at least 4 and a power of 2");
    }
    if (numSets <= 0 || !isPowerOf2(numSets)) {
        fatal("# of sets must be non-zero and a power of 2");
    }
    if (assoc <= 0) {
        fatal("associativity must be greater than zero");
    }

    blkMask = blkSize - 1;
    /* MJL_Begin */
    MJL_byteMask = sizeof(uint64_t) - 1;
    MJL_rowShift = floorLog2(sizeof(uint64_t));
    MJL_wordMask = blkSize/sizeof(uint64_t) - 1;
    MJL_colShift = floorLog2(MJL_rowWidth) + floorLog2(blkSize);
    MJL_blkMaskColumn = (MJL_wordMask << MJL_colShift) | MJL_byteMask;
    MJL_commonHighMask = ~(MJL_rowWidth * blkSize * blkSize/sizeof(uint64_t) - 1);
    MJL_commonLowMask = (MJL_rowWidth - 1) << floorLog2(blkSize);
    /* MJL_End */
    setShift = floorLog2(blkSize);
    setMask = numSets - 1;
    tagShift = setShift + floorLog2(numSets);
    /* MJL_Begin */
    if (MJL_oracleProxy) {
        setMask = numSets/2 - 1;
        tagShift = setShift + floorLog2(numSets/2);
    }
    /* MJL_End */
    /** @todo Make warmup percentage a parameter. */
    warmupBound = numSets * assoc;

    sets = new SetType[numSets];
    blks = new BlkType[numSets * assoc];
    // allocate data storage in one big chunk
    numBlocks = numSets * assoc;
    dataBlks = new uint8_t[numBlocks * blkSize];

    unsigned blkIndex = 0;       // index into blks array
    for (unsigned i = 0; i < numSets; ++i) {
        sets[i].assoc = assoc;

        sets[i].blks = new BlkType*[assoc];

        // link in the data blocks
        for (unsigned j = 0; j < assoc; ++j) {
            // locate next cache block
            BlkType *blk = &blks[blkIndex];
            blk->data = &dataBlks[blkSize*blkIndex];
            ++blkIndex;

            // invalidate new cache block
            blk->invalidate();

            //EGH Fix Me : do we need to initialize blk?

            // Setting the tag to j is just to prevent long chains in the hash
            // table; won't matter because the block is invalid
            blk->tag = j;
            /* MJL_Begin */
            // MJL_TODO: May not be needed, can be set to invalid to check if the direction information has been passed properly
            blk->MJL_blkDir = CacheBlk::MJL_CacheBlkDir::MJL_IsRow;
            /* if (MJL_oracleProxy) {
                blk->MJL_accessPCList = new std::list< Addr >();
                blk->MJL_accessAddrList = new std::list< Addr >();
            } */
            /* MJL_End */
            blk->whenReady = 0;
            blk->isTouched = false;
            blk->size = blkSize;
            sets[i].blks[j]=blk;
            blk->set = i;
            blk->way = j;
        }
    }
}

BaseSetAssoc::~BaseSetAssoc()
{
    delete [] dataBlks;
    delete [] blks;
    delete [] sets;
}

CacheBlk*
BaseSetAssoc::findBlock(Addr addr, bool is_secure) const
{
    Addr tag = extractTag(addr);
    unsigned set = extractSet(addr);
    BlkType *blk = sets[set].findBlk(tag, is_secure);
    return blk;
}
/* MJL_Begin */
CacheBlk* 
BaseSetAssoc::MJL_findBlock(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure) const
{
    if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos || this->name().find("l3") != std::string::npos) {
        Addr tag = MJL_extractTag(addr, MJL_cacheBlkDir);
        unsigned set = MJL_extractSet(addr, MJL_cacheBlkDir);
        BlkType *blk = sets[set].MJL_findBlk(tag, MJL_cacheBlkDir, is_secure);
        return blk;
    } else {
        return findBlock(addr, is_secure);
    }
}

CacheBlk* 
BaseSetAssoc::MJL_findCrossBlock(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure, unsigned MJL_offset) const
{
    Addr tag = MJL_extractTag(addr, MJL_cacheBlkDir);
    unsigned set = MJL_extractSet(addr, MJL_cacheBlkDir);
    BlkType *blk = sets[set].MJL_findCrossBlk(tag, MJL_cacheBlkDir, is_secure, MJL_offset);
    return blk;
}

bool 
BaseSetAssoc::MJL_hasCrossing(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure) const
{
    assert(this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos || this->name().find("l3") != std::string::npos);
    uint64_t MJL_wordMask = blkSize/sizeof(uint64_t) - 1;
    int MJL_colShift = floorLog2(MJL_rowWidth) + floorLog2(blkSize);
    Addr tileMask = ~((Addr)(blkSize-1) | (MJL_wordMask << MJL_colShift));
    Addr blkaddr = addr;
    for (unsigned offset = 0; offset < blkSize/sizeof(uint64_t); ++offset) {
        if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
            blkaddr = (addr & tileMask) | ((Addr)offset*sizeof(uint64_t)) ;
            Addr tag = MJL_extractTag(blkaddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
            unsigned set = MJL_extractSet(blkaddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
            BlkType *blk = sets[set].MJL_findBlk(tag, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn, is_secure);
            if (blk) {
                return true;
            }
        } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) {
            blkaddr = (addr & tileMask) | ((Addr)offset << MJL_colShift);
            Addr tag = MJL_extractTag(blkaddr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow);
            unsigned set = MJL_extractSet(blkaddr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow);
            BlkType *blk = sets[set].MJL_findBlk(tag, CacheBlk::MJL_CacheBlkDir::MJL_IsRow, is_secure);
            if (blk) {
                return true;
            }
        } else {
            assert(MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow || MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
        }
    } 
    return false;
}

bool 
BaseSetAssoc::MJL_hasCrossingDirtyOrWritable(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure) const
{
    assert(this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos || this->name().find("l3") != std::string::npos);
    uint64_t MJL_wordMask = blkSize/sizeof(uint64_t) - 1;
    int MJL_colShift = floorLog2(MJL_rowWidth) + floorLog2(blkSize);
    Addr tileMask = ~((Addr)(blkSize-1) | (MJL_wordMask << MJL_colShift));
    Addr blkaddr = addr;
    for (unsigned offset = 0; offset < blkSize/sizeof(uint64_t); ++offset) {
        if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
            blkaddr = (addr & tileMask) | ((Addr)offset*sizeof(uint64_t)) ;
            Addr tag = MJL_extractTag(blkaddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
            unsigned set = MJL_extractSet(blkaddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
            BlkType *blk = sets[set].MJL_findBlk(tag, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn, is_secure);
            if (blk && (blk->isDirty() || blk->MJL_wasDirty || blk->isWritable() || !blk->isReadable())) {
                return true;
            }
        } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) {
            blkaddr = (addr & tileMask) | ((Addr)offset << MJL_colShift);
            Addr tag = MJL_extractTag(blkaddr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow);
            unsigned set = MJL_extractSet(blkaddr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow);
            BlkType *blk = sets[set].MJL_findBlk(tag, CacheBlk::MJL_CacheBlkDir::MJL_IsRow, is_secure);
            if (blk && (blk->isDirty() || blk->MJL_wasDirty || blk->isWritable() || !blk->isReadable())) {
                return true;
            }
        } else {
            assert(MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow || MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
        }
    } 
    return false;
}

bool 
BaseSetAssoc::MJL_hasCrossingDirty(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure) const
{
    assert(this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos || this->name().find("l3") != std::string::npos);
    uint64_t MJL_wordMask = blkSize/sizeof(uint64_t) - 1;
    int MJL_colShift = floorLog2(MJL_rowWidth) + floorLog2(blkSize);
    Addr tileMask = ~((Addr)(blkSize-1) | (MJL_wordMask << MJL_colShift));
    Addr blkaddr = addr;
    for (unsigned offset = 0; offset < blkSize/sizeof(uint64_t); ++offset) {
        if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
            blkaddr = (addr & tileMask) | ((Addr)offset*sizeof(uint64_t)) ;
            Addr tag = MJL_extractTag(blkaddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
            unsigned set = MJL_extractSet(blkaddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
            BlkType *blk = sets[set].MJL_findBlk(tag, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn, is_secure);
            if (blk && (blk->isDirty() || blk->MJL_wasDirty || !blk->isReadable())) {
                return true;
            }
        } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) {
            blkaddr = (addr & tileMask) | ((Addr)offset << MJL_colShift);
            Addr tag = MJL_extractTag(blkaddr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow);
            unsigned set = MJL_extractSet(blkaddr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow);
            BlkType *blk = sets[set].MJL_findBlk(tag, CacheBlk::MJL_CacheBlkDir::MJL_IsRow, is_secure);
            if (blk && (blk->isDirty() || blk->MJL_wasDirty || !blk->isReadable())) {
                return true;
            }
        } else {
            assert(MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow || MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
        }
    } 
    return false;
}

bool 
BaseSetAssoc::MJL_hasCrossingWritableRevoked(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure)
{
    assert(this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos || this->name().find("l3") != std::string::npos);
    uint64_t MJL_wordMask = blkSize/sizeof(uint64_t) - 1;
    int MJL_colShift = floorLog2(MJL_rowWidth) + floorLog2(blkSize);
    Addr tileMask = ~((Addr)(blkSize-1) | (MJL_wordMask << MJL_colShift));
    Addr blkaddr = addr;
    bool has_crossing_writable = false;
    for (unsigned offset = 0; offset < blkSize/sizeof(uint64_t); ++offset) {
        if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
            blkaddr = (addr & tileMask) | ((Addr)offset*sizeof(uint64_t)) ;
            Addr tag = MJL_extractTag(blkaddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
            unsigned set = MJL_extractSet(blkaddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
            BlkType *blk = sets[set].MJL_findBlk(tag, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn, is_secure);
            if (blk && blk->isWritable()) {
                blk->status &= ~BlkWritable;
                has_crossing_writable |= true;
                assert(!(blk->isDirty() || blk->MJL_wasDirty || !blk->isReadable()));
            }
        } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) {
            blkaddr = (addr & tileMask) | ((Addr)offset << MJL_colShift);
            Addr tag = MJL_extractTag(blkaddr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow);
            unsigned set = MJL_extractSet(blkaddr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow);
            BlkType *blk = sets[set].MJL_findBlk(tag, CacheBlk::MJL_CacheBlkDir::MJL_IsRow, is_secure);
            if (blk && blk->isWritable()) {
                blk->status &= ~BlkWritable;
                has_crossing_writable |= true;
                assert(!(blk->isDirty() || blk->MJL_wasDirty || !blk->isReadable()));
            }
        } else {
            assert(MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow || MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
        }
    } 
    return has_crossing_writable;
}
/* MJL_End */

CacheBlk*
BaseSetAssoc::findBlockBySetAndWay(int set, int way) const
{
    return sets[set].blks[way];
}

std::string
BaseSetAssoc::print() const {
    std::string cache_state;
    for (unsigned i = 0; i < numSets; ++i) {
        // link in the data blocks
        for (unsigned j = 0; j < assoc; ++j) {
            BlkType *blk = sets[i].blks[j];
            if (blk->isValid())
                cache_state += csprintf("\tset: %d block: %d %s\n", i, j,
                        blk->print());
        }
    }
    if (cache_state.empty())
        cache_state = "no valid tags\n";
    return cache_state;
}

void
BaseSetAssoc::cleanupRefs()
{
    for (unsigned i = 0; i < numSets*assoc; ++i) {
        if (blks[i].isValid()) {
            totalRefs += blks[i].refCount;
            ++sampledRefs;
        }
    }
}

void
BaseSetAssoc::computeStats()
{
    for (unsigned i = 0; i < ContextSwitchTaskId::NumTaskId; ++i) {
        occupanciesTaskId[i] = 0;
        for (unsigned j = 0; j < 5; ++j) {
            ageTaskId[i][j] = 0;
        }
    }

    for (unsigned i = 0; i < numSets * assoc; ++i) {
        if (blks[i].isValid()) {
            assert(blks[i].task_id < ContextSwitchTaskId::NumTaskId);
            occupanciesTaskId[blks[i].task_id]++;
            assert(blks[i].tickInserted <= curTick());
            Tick age = curTick() - blks[i].tickInserted;

            int age_index;
            if (age / SimClock::Int::us < 10) { // <10us
                age_index = 0;
            } else if (age / SimClock::Int::us < 100) { // <100us
                age_index = 1;
            } else if (age / SimClock::Int::ms < 1) { // <1ms
                age_index = 2;
            } else if (age / SimClock::Int::ms < 10) { // <10ms
                age_index = 3;
            } else
                age_index = 4; // >10ms

            ageTaskId[blks[i].task_id][age_index]++;
        }
    }
}
