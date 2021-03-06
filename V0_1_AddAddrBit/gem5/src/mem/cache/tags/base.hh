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
 *          Ron Dreslinski
 */

/**
 * @file
 * Declaration of a common base class for cache tagstore objects.
 */

#ifndef __BASE_TAGS_HH__
#define __BASE_TAGS_HH__

#include <string>

#include "base/callback.hh"
#include "base/statistics.hh"
#include "mem/cache/blk.hh"
#include "params/BaseTags.hh"
#include "sim/clocked_object.hh"

class BaseCache;

/**
 * A common base class of Cache tagstore objects.
 */
class BaseTags : public ClockedObject
{
  protected:
    /** The block size of the cache. */
    const unsigned blkSize;
    /* MJL_Begin */
    /** The size of a row in the memory system (how many cachelines) */
    const unsigned MJL_rowWidth;
    const Cycles MJL_timeStep;
    bool MJL_timeStepScheduled;
    const bool MJL_sameSetMapping;
    const bool MJL_oracleProxy;
    /* MJL_End */
    /** The size of the cache. */
    const unsigned size;
    /** The tag lookup latency of the cache. */
    const Cycles lookupLatency;
    /**
     * The total access latency of the cache. This latency
     * is different depending on the cache access mode
     * (parallel or sequential)
     */
    const Cycles accessLatency;
    /** Pointer to the parent cache. */
    BaseCache *cache;

    /**
     * The number of tags that need to be touched to meet the warmup
     * percentage.
     */
    int warmupBound;
    /** Marked true when the cache is warmed up. */
    bool warmedUp;

    /** the number of blocks in the cache */
    unsigned numBlocks;

    // Statistics
    /**
     * @addtogroup CacheStatistics
     * @{
     */

    /** Number of replacements of valid blocks per thread. */
    Stats::Vector replacements;
    /** Per cycle average of the number of tags that hold valid data. */
    Stats::Average tagsInUse;
    /* MJL_Begin */
    unsigned MJL_tagsInUse;
    Stats::Scalar MJL_rowInUse;
    Stats::Scalar MJL_colInUse;
    Stats::Formula MJL_rowUtilization;
    Stats::Formula MJL_colUtilization;
    Stats::Formula MJL_utilization;
    Stats::Scalar MJL_Duplicates;
    Stats::Formula MJL_DuplicatePercentage;
    /* MJL_End */

    /** The total number of references to a block before it is replaced. */
    Stats::Scalar totalRefs;

    /**
     * The number of reference counts sampled. This is different from
     * replacements because we sample all the valid blocks when the simulator
     * exits.
     */
    Stats::Scalar sampledRefs;

    /**
     * Average number of references to a block before is was replaced.
     * @todo This should change to an average stat once we have them.
     */
    Stats::Formula avgRefs;

    /** The cycle that the warmup percentage was hit. */
    Stats::Scalar warmupCycle;

    /** Average occupancy of each requestor using the cache */
    Stats::AverageVector occupancies;

    /** Average occ % of each requestor using the cache */
    Stats::Formula avgOccs;

    /** Occupancy of each context/cpu using the cache */
    Stats::Vector occupanciesTaskId;

    /** Occupancy of each context/cpu using the cache */
    Stats::Vector2d ageTaskId;

    /** Occ % of each context/cpu using the cache */
    Stats::Formula percentOccsTaskId;

    /** Number of tags consulted over all accesses. */
    Stats::Scalar tagAccesses;
    /** Number of data blocks consulted over all accesses. */
    Stats::Scalar dataAccesses;

    /**
     * @}
     */

  public:
    typedef BaseTagsParams Params;
    BaseTags(const Params *p);

    /**
     * Destructor.
     */
    virtual ~BaseTags() {}

    /**
     * Set the parent cache back pointer.
     * @param _cache Pointer to parent cache.
     */
    void setCache(BaseCache *_cache);

    /**
     * Register local statistics.
     */
    void regStats();

    /**
     * Average in the reference count for valid blocks when the simulation
     * exits.
     */
    virtual void cleanupRefs() {}

    /**
     * Computes stats just prior to dump event
     */
    virtual void computeStats() {}

    /**
     * Print all tags used
     */
    virtual std::string print() const = 0;

    /**
     * Find a block using the memory address
     */
    virtual CacheBlk * findBlock(Addr addr, bool is_secure) const = 0;
    /* MJL_Begin */
    virtual CacheBlk * MJL_findBlock(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure) const = 0;
    virtual CacheBlk * MJL_findCrossBlock(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure, unsigned MJL_offset) const = 0;
    virtual bool MJL_hasCrossing(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure) const = 0;
    virtual bool MJL_hasCrossingDirtyOrWritable(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure) const = 0;
    virtual bool MJL_hasCrossingDirty(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure) const = 0;
    virtual bool MJL_hasCrossingWritableRevoked(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure) = 0;
    /* MJL_End */

    /**
     * Calculate the block offset of an address.
     * @param addr the address to get the offset of.
     * @return the block offset.
     */
    int extractBlkOffset(Addr addr) const
    {
        return (addr & (Addr)(blkSize-1));
    }
    /* MJL_Begin */
    int MJL_extractBlkOffset(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir) {
        int MJL_rowShift = floorLog2(sizeof(uint64_t));
        uint64_t MJL_wordMask = blkSize/sizeof(uint64_t) - 1;
        int MJL_colShift = floorLog2(MJL_rowWidth) + floorLog2(blkSize);

        Addr new_row = (addr >> MJL_colShift) & (Addr)MJL_wordMask;
        if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
            return (addr & (Addr)(blkSize-1));
        } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) {
            return ((new_row << MJL_rowShift) | (addr & (Addr)(sizeof(uint64_t) - 1)) );
        } else {
            return (addr & (Addr)(blkSize-1));
        }
    }
    /* MJL_End */

    /**
     * Find the cache block given set and way
     * @param set The set of the block.
     * @param way The way of the block.
     * @return The cache block.
     */
    virtual CacheBlk *findBlockBySetAndWay(int set, int way) const = 0;
    /* MJL_Begin */
    virtual CacheBlk *MJL_findBlockByTile(CacheBlk *blk, int i) const = 0;
    /* MJL_End */

    /**
     * Limit the allocation for the cache ways.
     * @param ways The maximum number of ways available for replacement.
     */
    virtual void setWayAllocationMax(int ways)
    {
        panic("This tag class does not implement way allocation limit!\n");
    }

    /**
     * Get the way allocation mask limit.
     * @return The maximum number of ways available for replacement.
     */
    virtual int getWayAllocationMax() const
    {
        panic("This tag class does not implement way allocation limit!\n");
        return -1;
    }

    virtual unsigned getNumSets() const = 0;

    virtual unsigned getNumWays() const = 0;
    /* MJL_Begin */
    unsigned MJL_get_tagsInUse () const {return MJL_tagsInUse;}
    /* MJL_End */

    virtual void invalidate(CacheBlk *blk) = 0;

    virtual CacheBlk* accessBlock(Addr addr, bool is_secure, Cycles &lat,
                                  int context_src) = 0;
    /* MJL_Begin */
    virtual CacheBlk* MJL_accessBlock(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure, Cycles &lat,
                                  int context_src) = 0;
    virtual CacheBlk* MJL_accessBlockOneWord(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure, Cycles &lat,
                          int context_src) = 0;
    virtual CacheBlk* MJL_accessCrossBlock(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure, Cycles &lat,
                          int context_src, unsigned MJL_offset) = 0;
    virtual CacheBlk* MJL_findWritebackBlk(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure, int MJL_offset) = 0;
    virtual int MJL_tileExists(Addr addr, bool is_secure) = 0;
    /* MJL_End */

    virtual Addr extractTag(Addr addr) const = 0;
    /* MJL_Begin */
    virtual Addr MJL_extractTag(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir) const = 0;
    /* MJL_End */

    virtual void insertBlock(PacketPtr pkt, CacheBlk *blk) = 0;

    virtual Addr regenerateBlkAddr(Addr tag, unsigned set) const = 0;
    /* MJL_Begin */
    virtual Addr MJL_regenerateBlkAddr(Addr tag, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, unsigned set) const = 0;
    /* MJL_End */


    virtual CacheBlk* findVictim(Addr addr) = 0;
    /* MJL_Begin */
    virtual CacheBlk* MJL_findVictim(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir) = 0;
    /* MJL_End */

    virtual int extractSet(Addr addr) const = 0;
    /* MJL_Begin */
    virtual int MJL_extractSet(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir) const = 0;
    virtual Addr MJL_blkAlign(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir) const = 0;
    virtual Addr MJL_swapRowColBits(Addr addr) const = 0;
    virtual Addr MJL_movColRight(Addr addr) const = 0;
    virtual Addr MJL_movColLeft(Addr addr) const = 0;

    uint64_t MJL_tempUtilizationRow;
    uint64_t MJL_tempUtilizationColumn;

    class MJL_UtilizationVisitor : public CacheBlkVisitor
    {
      public:
        MJL_UtilizationVisitor(BaseTags* tags): _tags(tags) {}

        bool operator()(CacheBlk &blk) override {
            if (blk.isValid()) {
                if (blk.MJL_isRow()) {
                    _tags->MJL_tempUtilizationRow++;
                } else {
                    _tags->MJL_tempUtilizationColumn++;
                }
            }
            return true;
        }
      private:
        BaseTags* _tags;
    };

    void MJL_printUtilization() {
        /* MJL_Test: see if utilization calculation is correct
        MJL_tempUtilizationRow = 0;
        MJL_tempUtilizationColumn = 0;
        MJL_UtilizationVisitor MJL_utilizationVisitor(this);
        forEachBlk(MJL_utilizationVisitor);
        std::cout << this->name() << "::MJL_Test: Utilization(Row) foreach[" << MJL_tempUtilizationRow << "]/inuse[" << MJL_rowInUse.value() << "], Utilization(Col) foreach[" << MJL_tempUtilizationColumn << "]/inuse[" << MJL_colInUse.value() << "]" << std::endl;
        assert(MJL_tempUtilizationRow == MJL_rowInUse.value());
        assert(MJL_tempUtilizationColumn == MJL_colInUse.value());
         */
        std::cout << this->name() << "::MJL_intermOutput: Cycle[" << ticksToCycles(curTick()) << "], ";
        std::cout << "Utilization(Row) " << (MJL_rowInUse.value() / float(numBlocks)) << "[" << MJL_rowInUse.value() << "/" << numBlocks << "], ";
        std::cout << "Utilization(Col) " << (MJL_colInUse.value() / float(numBlocks)) << "[" << MJL_colInUse.value() << "/" << numBlocks << "], ";
        std::cout << "Utilization(All) " << ((MJL_rowInUse.value() + MJL_colInUse.value()) / float(numBlocks)) << "[" << MJL_rowInUse.value() + MJL_colInUse.value() << "/" << numBlocks << "], ";
        std::cout << "Duplicate " << (MJL_Duplicates.value()/8/(MJL_rowInUse.value() + MJL_colInUse.value())) << "[" << MJL_Duplicates.value() << "/" << 8*(MJL_rowInUse.value() + MJL_colInUse.value()) << "]" << std::endl;
        schedule(MJL_printUtilizationEvent, curTick() + cyclesToTicks(MJL_timeStep));
    }
    EventWrapper<BaseTags, &BaseTags::MJL_printUtilization> MJL_printUtilizationEvent;
    /* MJL_End */
    
    virtual void forEachBlk(CacheBlkVisitor &visitor) = 0;
};

class BaseTagsCallback : public Callback
{
    BaseTags *tags;
  public:
    BaseTagsCallback(BaseTags *t) : tags(t) {}
    virtual void process() { tags->cleanupRefs(); };
};

class BaseTagsDumpCallback : public Callback
{
    BaseTags *tags;
  public:
    BaseTagsDumpCallback(BaseTags *t) : tags(t) {}
    virtual void process() { tags->computeStats(); };
};

#endif //__BASE_TAGS_HH__
