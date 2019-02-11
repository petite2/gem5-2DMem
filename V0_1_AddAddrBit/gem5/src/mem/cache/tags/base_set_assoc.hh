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
 * Declaration of a base set associative tag store.
 */

#ifndef __MEM_CACHE_TAGS_BASESETASSOC_HH__
#define __MEM_CACHE_TAGS_BASESETASSOC_HH__

#include <cassert>
#include <cstring>
#include <list>

#include "mem/cache/base.hh"
#include "mem/cache/blk.hh"
#include "mem/cache/tags/base.hh"
#include "mem/cache/tags/cacheset.hh"
#include "mem/packet.hh"
#include "params/BaseSetAssoc.hh"

/**
 * A BaseSetAssoc cache tag store.
 * @sa  \ref gem5MemorySystem "gem5 Memory System"
 *
 * The BaseSetAssoc tags provide a base, as well as the functionality
 * common to any set associative tags. Any derived class must implement
 * the methods related to the specifics of the actual replacment policy.
 * These are:
 *
 * BlkType* accessBlock();
 * BlkType* findVictim();
 * void insertBlock();
 * void invalidate();
 */
class BaseSetAssoc : public BaseTags
{
  public:
    /** Typedef the block type used in this tag store. */
    typedef CacheBlk BlkType;
    /** Typedef for a list of pointers to the local block class. */
    typedef std::list<BlkType*> BlkList;
    /** Typedef the set type used in this tag store. */
    typedef CacheSet<CacheBlk> SetType;


  protected:
    /** The associativity of the cache. */
    const unsigned assoc;
    /** The allocatable associativity of the cache (alloc mask). */
    unsigned allocAssoc;
    /** The number of sets in the cache. */
    const unsigned numSets;
    /** Whether tags and data are accessed sequentially. */
    const bool sequentialAccess;

    /** The cache sets. */
    SetType *sets;

    /** The cache blocks. */
    BlkType *blks;
    /** The data blocks, 1 per cache block. */
    uint8_t *dataBlks;

    /** The amount to shift the address to get the set. */
    int setShift;
    /** The amount to shift the address to get the tag. */
    int tagShift;
    /** Mask out all bits that aren't part of the set index. */
    unsigned setMask;
    /** Mask out all bits that aren't part of the block offset. */
    unsigned blkMask;
    /* MJL_Begin */
    // Mask first and then shift to get the value
    /** Mask out all bits that aren't part of the column block offset. */
    uint64_t MJL_blkMaskColumn;
    /** Mask out all bits that aren't part of the byte offset. */
    uint64_t MJL_byteMask;
    /** Mask out all bits that aren't part of the word offset. */
    uint64_t MJL_wordMask;
    /** Mask out all bits that aren't part of the same for both row and column (the higher part). */
    uint64_t MJL_commonHighMask;
    /** Mask out all bits that aren't part of the same for both row and column (the lower part). */
    uint64_t MJL_commonLowMask;
    /** The amount to shift the address to get the column. */
    int MJL_colShift;
    /** The amount to shift the address to get the row. */
    int MJL_rowShift;
    /* MJL_End */

public:

    /** Convenience typedef. */
     typedef BaseSetAssocParams Params;

    /**
     * Construct and initialize this tag store.
     */
    BaseSetAssoc(const Params *p);

    /**
     * Destructor
     */
    virtual ~BaseSetAssoc();

    /**
     * Return the block size.
     * @return the block size.
     */
    unsigned
    getBlockSize() const
    {
        return blkSize;
    }

    /**
     * Return the subblock size. In the case of BaseSetAssoc it is always
     * the block size.
     * @return The block size.
     */
    unsigned
    getSubBlockSize() const
    {
        return blkSize;
    }

    /**
     * Return the number of sets this cache has
     * @return The number of sets.
     */
    unsigned
    getNumSets() const override
    {
        return numSets;
    }

    /**
     * Return the number of ways this cache has
     * @return The number of ways.
     */
    unsigned
    getNumWays() const override
    {
        return assoc;
    }

    /**
     * Find the cache block given set and way
     * @param set The set of the block.
     * @param way The way of the block.
     * @return The cache block.
     */
    CacheBlk *findBlockBySetAndWay(int set, int way) const override;

    /* MJL_Begin */
    /**
     * Find the i-th row block in the tile
     * @param blk A cache block in the tile to be searched
     * @param i The row of the block in the tile to be returned
     * @return The cache block
     */
    CacheBlk *MJL_findBlockByTile(CacheBlk *blk, int i) const override
    {
        int set = blk->set;
        int way = blk->way;
        int MJL_startOffset = set % (blkSize/sizeof(uint64_t));
        return sets[set - MJL_startOffset + i].blks[way];
    }
    /* MJL_End */

    /**
     * Invalidate the given block.
     * @param blk The block to invalidate.
     */
    void invalidate(CacheBlk *blk) override
    {
        assert(blk);
        /* MJL_Begin */
        if (!cache->MJL_is2DCache()) {
            assert(blk->isValid());
        }
        /* MJL_End */
        /* MJL_Comment
        assert(blk->isValid());
        */
        // MJL_Note: Guess I'll just assume that every thing is 1/8 of the presented value in physically 2D cache
        tagsInUse--;
        /* MJL_Begin */
        MJL_tagsInUse--;
        if (blk->MJL_isRow()) {
            MJL_rowInUse--;
        } else if (blk->MJL_isColumn()) {
            MJL_colInUse--;
            //std::cout << "MJL_Test: colInUse--(" << MJL_colInUse.value() << ")" << std::endl;
        }
        // Add to bloom filter 
        /* MJL_Test */
        if (cache->MJL_get_Test_rowColBloomFilters()) {
            cache->MJL_get_Test_rowColBloomFilters()->test_remove(MJL_regenerateBlkAddr(blk->tag, blk->MJL_blkDir, blk->set), blk->MJL_blkDir);
        }
        /* */
        if (MJL_oracleProxy) {
            cache->collect_stats(blk);
        }
        if (cache->MJL_get_rowColBloomFilter()) {
            cache->MJL_get_rowColBloomFilter()->remove(MJL_regenerateBlkAddr(blk->tag, blk->MJL_blkDir, blk->set), blk->MJL_blkDir);
        }
        if (!cache->MJL_is2DCache()) {
            CacheBlk * MJL_dupBlk = nullptr;
            Addr baseAddr = MJL_regenerateBlkAddr(blk->tag, blk->MJL_blkDir, blk->set);
            for (int i = 0; i < blkSize; i += sizeof(uint64_t)) {
                MJL_dupBlk = nullptr;
                Addr wordAddr = cache->MJL_addOffsetAddr(baseAddr, blk->MJL_blkDir, i);
                if (blk->MJL_isRow()) {
                    MJL_dupBlk = MJL_findBlock(wordAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn, blk->isSecure());
                } else if (blk->MJL_isColumn()) {
                    MJL_dupBlk = MJL_findBlock(wordAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow, blk->isSecure());
                }
                if (MJL_dupBlk) {
                    MJL_Duplicates--;
                }
            }
        }
        /* MJL_End */
        assert(blk->srcMasterId < cache->system->maxMasters());
        occupancies[blk->srcMasterId]--;
        blk->srcMasterId = Request::invldMasterId;
        blk->task_id = ContextSwitchTaskId::Unknown;
        blk->tickInserted = curTick();
    }

    /**
     * Access block and update replacement data. May not succeed, in which case
     * nullptr is returned. This has all the implications of a cache
     * access and should only be used as such. Returns the access latency as a
     * side effect.
     * @param addr The address to find.
     * @param is_secure True if the target memory space is secure.
     * @param asid The address space ID.
     * @param lat The access latency.
     * @return Pointer to the cache block if found.
     */
    CacheBlk* accessBlock(Addr addr, bool is_secure, Cycles &lat,
                          int context_src) override
    {
        /* MJL_Begin */
        if (!MJL_timeStepScheduled && MJL_timeStep > 0) {
            schedule(MJL_printUtilizationEvent, curTick() + 1);
            MJL_timeStepScheduled = true;
        }
        /* MJL_End */
        Addr tag = extractTag(addr);
        int set = extractSet(addr);
        BlkType *blk = sets[set].findBlk(tag, is_secure);

        // Access all tags in parallel, hence one in each way.  The data side
        // either accesses all blocks in parallel, or one block sequentially on
        // a hit.  Sequential access with a miss doesn't access data.
        tagAccesses += allocAssoc;
        if (sequentialAccess) {
            if (blk != nullptr) {
                dataAccesses += 1;
            }
        } else {
            dataAccesses += allocAssoc;
        }

        if (blk != nullptr) {
            // If a cache hit
            lat = accessLatency;
            // Check if the block to be accessed is available. If not,
            // apply the accessLatency on top of block->whenReady.
            if (blk->whenReady > curTick() &&
                cache->ticksToCycles(blk->whenReady - curTick()) >
                accessLatency) {
                lat = cache->ticksToCycles(blk->whenReady - curTick()) +
                accessLatency;
            }
            blk->refCount += 1;
        } else {
            // If a cache miss
            lat = lookupLatency;
        }

        return blk;
    }
    /* MJL_Begin */
    CacheBlk* MJL_accessBlock(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure, Cycles &lat,
                          int context_src) override
    {
        if (!MJL_timeStepScheduled && MJL_timeStep > 0) {
            schedule(MJL_printUtilizationEvent, curTick() + 1);
            MJL_timeStepScheduled = true;
        }
        if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos || this->name().find("l3") != std::string::npos) {
            Addr tag = MJL_extractTag(addr, MJL_cacheBlkDir);
            int set = MJL_extractSet(addr, MJL_cacheBlkDir);
            BlkType *blk = sets[set].MJL_findBlk(tag, MJL_cacheBlkDir, is_secure);

            // Access all tags in parallel, hence one in each way.  The data side
            // either accesses all blocks in parallel, or one block sequentially on
            // a hit.  Sequential access with a miss doesn't access data.
            tagAccesses += allocAssoc;
            if (sequentialAccess) {
                if (blk != nullptr) {
                    dataAccesses += 1;
                }
            } else {
                dataAccesses += allocAssoc;
            }

            if (blk != nullptr) {
                // If a cache hit
                lat = accessLatency;
                // Check if the block to be accessed is available. If not,
                // apply the accessLatency on top of block->whenReady.
                if (blk->whenReady > curTick() &&
                    cache->ticksToCycles(blk->whenReady - curTick()) >
                    accessLatency) {
                    lat = cache->ticksToCycles(blk->whenReady - curTick()) +
                    accessLatency;
                }
                blk->refCount += 1;
            } else {
                // If a cache miss
                lat = lookupLatency;
            }

            return blk;
        } else {
            return accessBlock(addr, is_secure, lat, context_src);
        }
    }

    /**
     * Access column block in a physically 2D cache and update replacement data. May not succeed, in which case
     * nullptr is returned. This has all the implications of a cache
     * access and should only be used as such. Returns the access latency as a
     * side effect.
     * @param addr The address to find.
     * @param MJL_cacheBlkDir Should be row because all cache blocks have row block.
     * @param is_secure True if the target memory space is secure.
     * @param asid The address space ID.
     * @param lat The access latency.
     * @param MJL_offset Which column of the row is this being accessed.
     * @return Pointer to the cache block if found.
     */
    CacheBlk* MJL_accessCrossBlock(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure, Cycles &lat,
                          int context_src, unsigned MJL_offset) override
    {
        Addr tag = MJL_extractTag(addr, MJL_cacheBlkDir);
        int set = MJL_extractSet(addr, MJL_cacheBlkDir);
        BlkType *blk = sets[set].MJL_findCrossBlk(tag, MJL_cacheBlkDir, is_secure, MJL_offset);

        // Access all tags in parallel, hence one in each way.  The data side
        // either accesses all blocks in parallel, or one block sequentially on
        // a hit.  Sequential access with a miss doesn't access data.
        tagAccesses += allocAssoc;
        if (sequentialAccess) {
            if (blk != nullptr) {
                dataAccesses += 1;
            }
        } else {
            dataAccesses += allocAssoc;
        }

        if (blk != nullptr) {
            // If a cache hit
            lat = accessLatency;
            // Check if the block to be accessed is available. If not,
            // apply the accessLatency on top of block->whenReady.
            if (blk->whenReady > curTick() &&
                cache->ticksToCycles(blk->whenReady - curTick()) >
                accessLatency) {
                lat = cache->ticksToCycles(blk->whenReady - curTick()) +
                accessLatency;
            }
            blk->refCount += 1;
        } else {
            // If a cache miss
            lat = lookupLatency;
        }

        return blk;
    }

    CacheBlk* MJL_accessBlockOneWord(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure, Cycles &lat,
                          int context_src) override 
    {
        BlkType *blk = MJL_accessBlock(addr, MJL_cacheBlkDir, is_secure, lat, context_src);
        Cycles templat = lat;
        if (blk == nullptr && (cache->name().find("dcache") != std::string::npos || cache->name().find("l2") != std::string::npos || this->name().find("l3") != std::string::npos)) {
            if ( MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow ) {
                blk = MJL_accessBlock(addr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn, is_secure, lat, context_src);
            } else if ( MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn ) {
                blk = MJL_accessBlock(addr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow, is_secure, lat, context_src);
            } else {
                assert( (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) || (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) );
            }
            if (!MJL_sameSetMapping && !MJL_oracleProxy) {
                lat = templat + lat;
            }
        }

        return blk;
    }

    /**
     * In physically 2D cache, even if the cache line is not valid,
     * as long as the tile is valid, a writeback can be served.
     * Here a block is returned if such tile exists, and the block is set to valid.
     * @param addr An address in the tile that needs to be checked
     * @param MJL_offset Which column is the requested block
     * @return Pointer to the blk that is not valid but the tile is
     */
    CacheBlk* MJL_findWritebackBlk(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure, int MJL_offset) {
        Addr tag = MJL_extractTag(addr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow);
        int set = MJL_extractSet(addr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow);
        CacheBlk* blk = nullptr;
        CacheBlk* retBlk = nullptr;
        int MJL_startOffset = set%sizeof(uint64_t);
        int way = 0;
        for (way = 0; way < assoc; ++way) {
            for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                blk = findBlockBySetAndWay(set + i - MJL_startOffset, way);
                if (blk->tag == tag && 
                    blk->MJL_blkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow && 
                    blk->isSecure() == is_secure && 
                    (blk->isValid() || blk->MJL_hasCrossValid())) {
                    if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) {
                        retBlk = findBlockBySetAndWay(set - MJL_startOffset, way);
                    } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
                        retBlk = findBlockBySetAndWay(set, way);
                    }
                    assert((MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow && !retBlk->isValid())|| (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn && !retBlk->MJL_crossValid[MJL_offset/sizeof(uint64_t)]));
                    break;
                }
            }
            if (retBlk) {
                break;
            }
        }
        if (retBlk && MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) {
            for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                CacheBlk * tile_blk = findBlockBySetAndWay(set + i - MJL_startOffset, way);
                tile_blk->MJL_crossValid[MJL_offset/sizeof(uint64_t)] = true;
                if (tile_blk->MJL_allCrossValid()) {
                    tile_blk->status |= BlkValid;
                }
            }
        } else if (retBlk && MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
            retBlk->status |= BlkValid;
            bool all_valid = true;
            for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                all_valid &= findBlockBySetAndWay(set + i - MJL_startOffset, way)->isValid();
            }
            if (all_valid) {
                for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                    findBlockBySetAndWay(set + i - MJL_startOffset, way)->MJL_setAllCrossValid();
                }
            }
        }
        return retBlk;
    }

    int MJL_tileExists(Addr addr, bool is_secure) {
        int tileExists = assoc;
        Addr tag = MJL_extractTag(addr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow);
        int set = MJL_extractSet(addr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow);
        CacheBlk* blk = nullptr;
        int MJL_startOffset = set%sizeof(uint64_t);
        for (int way = 0; way < assoc; ++way) {
            for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                blk = findBlockBySetAndWay(set + i - MJL_startOffset, way);
                if (blk->tag == tag && 
                    blk->MJL_blkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow && 
                    blk->isSecure() == is_secure && 
                    (blk->isValid() || blk->MJL_hasCrossValid())) {
                    tileExists = way;
                    break;
                }
            }
            if (tileExists != assoc) {
                break;
            }
        }
        
        return tileExists;
    }
    /* MJL_End */

    /**
     * Finds the given address in the cache, do not update replacement data.
     * i.e. This is a no-side-effect find of a block.
     * @param addr The address to find.
     * @param is_secure True if the target memory space is secure.
     * @param asid The address space ID.
     * @return Pointer to the cache block if found.
     */
    CacheBlk* findBlock(Addr addr, bool is_secure) const override;
    /* MJL_Begin */
    CacheBlk * MJL_findBlock(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure) const override;
    CacheBlk * MJL_findCrossBlock(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure, unsigned MJL_offset) const override;
    bool MJL_hasCrossing(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure) const override;
    /* MJL_End */

    /**
     * Find an invalid block to evict for the address provided.
     * If there are no invalid blocks, this will return the block
     * in the least-recently-used position.
     * @param addr The addr to a find a replacement candidate for.
     * @return The candidate block.
     */
    CacheBlk* findVictim(Addr addr) override
    {
        BlkType *blk = nullptr;
        int set = extractSet(addr);

        // prefer to evict an invalid block
        for (int i = 0; i < allocAssoc; ++i) {
            blk = sets[set].blks[i];
            if (!blk->isValid())
                break;
        }

        return blk;
    }
    /* MJL_Begin */
    CacheBlk* MJL_findVictim(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir) override
    {
        if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos || this->name().find("l3") != std::string::npos) {
            BlkType *blk = nullptr;
            int set = MJL_extractSet(addr, MJL_cacheBlkDir);

            // prefer to evict an invalid block
            for (int i = 0; i < allocAssoc; ++i) {
                // For physically 2D cache, the whole tile should be checked to see it it's invalid
                if (cache->MJL_is2DCache()) {
                    bool MJL_valid = false;
                    int MJL_startOffset = set%sizeof(uint64_t);

                    for (int j = 0; j < getBlockSize()/sizeof(uint64_t); ++j) {
                        MJL_valid |= sets[set + j - MJL_startOffset].blks[i]->isValid();
                        MJL_valid |= sets[set + j - MJL_startOffset].blks[i]->MJL_hasCrossValid();
                    };
                    if (!MJL_valid)
                        break;
                } else {
                    blk = sets[set].blks[i];

                    if (!blk->isValid())
                        break;
                }
            }

            return blk;
        } else {
            return findVictim(addr);
        }
    }
    /* MJL_End */

    /**
     * Insert the new block into the cache.
     * @param pkt Packet holding the address to update
     * @param blk The block to update.
     */
     void insertBlock(PacketPtr pkt, CacheBlk *blk) override
     {
         Addr addr = pkt->getAddr();
         MasterID master_id = pkt->req->masterId();
         uint32_t task_id = pkt->req->taskId();

         /* MJL_Begin */
         bool MJL_tileIsTouched = false;
         bool MJL_tileValid = false;
         if (cache->MJL_is2DCache()) {
             for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                    CacheBlk* tile_blk = MJL_findBlockByTile(blk, i);
                    // Get whether there are valid blocks in the tile
                    MJL_tileValid |= tile_blk->isValid();
                    MJL_tileValid |= tile_blk->MJL_hasCrossValid();
                    // Get whether the tile has been touched
                    MJL_tileIsTouched |= blk->isTouched;
             }
             if (!MJL_tileIsTouched) {
                 // Get stats about tagsInUse, assume 8 sets in a tile.
                 for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                    tagsInUse++;
                    MJL_tagsInUse++;
                    MJL_findBlockByTile(blk, i)->isTouched = true;
                 }
                 if (!warmedUp && tagsInUse.value() >= warmupBound) {
                     warmedUp = true;
                     warmupCycle = curTick();
                 }
             }
             if (MJL_tileValid) {
                 // Get stats about replacement and ref count
                 replacements[0]++;
                 ++sampledRefs;
                 for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                     CacheBlk* tile_blk = MJL_findBlockByTile(blk, i);
                     totalRefs += tile_blk->refCount;
                     tile_blk->refCount = 0;
                     assert(tile_blk->srcMasterId < cache->system->maxMasters());
                     // deal with evicted block
                     /* MJL_Test 
                     std::cout << "MJL_Debug: point C occupancies[" << blk->srcMasterId << "]=" << occupancies[tile_blk->srcMasterId].value() << std::endl;
                     if (i == blk->set%(blkSize/sizeof(uint64_t))) {
                         assert(blk == findBlockBySetAndWay(blk->set, blk->way));
                         assert(blk == tile_blk);
                     }
                      */
                     occupancies[blk->srcMasterId]--;
                     /* MJL_Test 
                     std::cout << "MJL_Debug: point D occupancies[" << blk->srcMasterId << "]=" << occupancies[tile_blk->srcMasterId].value() << std::endl;
                      */
                     tile_blk->invalidate();
                     tile_blk->MJL_clearCrossValid();
                     MJL_rowInUse--; 
                 }
             }
             for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                CacheBlk* tile_blk =  MJL_findBlockByTile(blk, i);
                tile_blk->isTouched = true;
                tile_blk->tag = MJL_extractTag(addr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow);
                tile_blk->MJL_blkDir = CacheBlk::MJL_CacheBlkDir::MJL_IsRow;

                tile_blk->srcMasterId = master_id;
                tile_blk->task_id = task_id;
                tile_blk->tickInserted = curTick();
                MJL_rowInUse++;
                // deal with what we are bringing in
                assert(master_id < cache->system->maxMasters());
                occupancies[master_id]++;
            }

            // We only need to write into one tag and one data block.
            tagAccesses += 1;
            dataAccesses += 1;

         } else {
         /* MJL_End */
         if (!blk->isTouched) {
             tagsInUse++;
             MJL_tagsInUse++;
             blk->isTouched = true;
             if (!warmedUp && tagsInUse.value() >= warmupBound) {
                 warmedUp = true;
                 warmupCycle = curTick();
             }
         }

         // If we're replacing a block that was previously valid update
         // stats for it. This can't be done in findBlock() because a
         // found block might not actually be replaced there if the
         // coherence protocol says it can't be.
         if (blk->isValid()) {
             /* MJL_Begin */
             if (blk->MJL_isRow()) {
                 MJL_rowInUse--;
             } else if (blk->MJL_isColumn()) {
                 MJL_colInUse--;
                 //std::cout << "MJL_Test: colInUse--(" << MJL_colInUse.value() << ")" << std::endl;
             }
             // Remove from bloom filter 
             /* MJL_Test */
             if (cache->MJL_get_Test_rowColBloomFilters()) {
                 cache->MJL_get_Test_rowColBloomFilters()->test_remove(MJL_regenerateBlkAddr(blk->tag, blk->MJL_blkDir, blk->set), blk->MJL_blkDir);
             }
             /* */
             if (MJL_oracleProxy) {
                 cache->collect_stats(blk);
             }
             if (cache->MJL_get_rowColBloomFilter()) {
                 cache->MJL_get_rowColBloomFilter()->remove(MJL_regenerateBlkAddr(blk->tag, blk->MJL_blkDir, blk->set), blk->MJL_blkDir);
             }
             if (!cache->MJL_is2DCache()) {
                 CacheBlk * MJL_dupBlk = nullptr;
                 Addr baseAddr = MJL_regenerateBlkAddr(blk->tag, blk->MJL_blkDir, blk->set);
                 for (int i = 0; i < blkSize; i += sizeof(uint64_t)) {
                     MJL_dupBlk = nullptr;
                     Addr wordAddr = cache->MJL_addOffsetAddr(baseAddr, blk->MJL_blkDir, i);
                     if (blk->MJL_isRow()) {
                         MJL_dupBlk = MJL_findBlock(wordAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn, blk->isSecure());
                     } else if (blk->MJL_isColumn()) {
                         MJL_dupBlk = MJL_findBlock(wordAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow, blk->isSecure());
                     }
                     if (MJL_dupBlk) {
                         MJL_Duplicates--;
                     }
                 }
             }
             /* MJL_End */
             replacements[0]++;
             totalRefs += blk->refCount;
             ++sampledRefs;
             blk->refCount = 0;

             // deal with evicted block
             assert(blk->srcMasterId < cache->system->maxMasters());
             occupancies[blk->srcMasterId]--;

             blk->invalidate();
         }

         blk->isTouched = true;

         // Set tag for new block.  Caller is responsible for setting status.
         /* MJL_Comment
         blk->tag = extractTag(addr);
         */
         /* MJL_Begin */
         if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos || this->name().find("l3") != std::string::npos) {
             blk->tag = MJL_extractTag(addr, pkt->MJL_getDataDir());
             // Add to bloom filter 
             /* MJL_Test */
             if (cache->MJL_get_Test_rowColBloomFilters()) {
                 cache->MJL_get_Test_rowColBloomFilters()->test_add(addr, pkt->MJL_getDataDir());
             }
             /* */
             if (cache->MJL_get_rowColBloomFilter()) {
                 cache->MJL_get_rowColBloomFilter()->add(addr, pkt->MJL_getDataDir());
             }
         } else {
             blk->tag = extractTag(addr);
         }
         blk->MJL_blkDir = pkt->MJL_getDataDir();
         if (blk->MJL_isRow()) {
             MJL_rowInUse++;
         } else if (blk->MJL_isColumn()) {
             MJL_colInUse++;
             //std::cout << "MJL_Test: colInUse++(" << MJL_colInUse.value() << ")" << std::endl;
         }
         if (!cache->MJL_is2DCache()) {
            CacheBlk * MJL_dupBlk = nullptr;
            for (int i = 0; i < blkSize; i += sizeof(uint64_t)) {
                MJL_dupBlk = nullptr;
                Addr wordAddr = cache->MJL_addOffsetAddr(addr, blk->MJL_blkDir, i);
                if (blk->MJL_isRow()) {
                    MJL_dupBlk = MJL_findBlock(wordAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn, pkt->isSecure());
                } else if (blk->MJL_isColumn()) {
                    MJL_dupBlk = MJL_findBlock(wordAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow, pkt->isSecure());
                }
                if (MJL_dupBlk) {
                    MJL_Duplicates++;
                }
            }
        }
         /* MJL_End */

         // deal with what we are bringing in
         assert(master_id < cache->system->maxMasters());
         occupancies[master_id]++;
         blk->srcMasterId = master_id;
         blk->task_id = task_id;
         blk->tickInserted = curTick();

         // We only need to write into one tag and one data block.
         tagAccesses += 1;
         dataAccesses += 1;
         /* MJL_Begin */
         }
         /* MJL_End */
     }

    /**
     * Limit the allocation for the cache ways.
     * @param ways The maximum number of ways available for replacement.
     */
    virtual void setWayAllocationMax(int ways) override
    {
        fatal_if(ways < 1, "Allocation limit must be greater than zero");
        allocAssoc = ways;
    }

    /**
     * Get the way allocation mask limit.
     * @return The maximum number of ways available for replacement.
     */
    virtual int getWayAllocationMax() const override
    {
        return allocAssoc;
    }

    /**
     * Generate the tag from the given address.
     * @param addr The address to get the tag from.
     * @return The tag of the address.
     */
    Addr extractTag(Addr addr) const override
    {
        return (addr >> tagShift);
    }
    /* MJL_Begin */
    Addr MJL_extractTag(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir) const override
    {
        if (MJL_sameSetMapping) {
            Addr commonHigh = (MJL_movColRight(addr) >> (tagShift + MJL_rowShift)) << MJL_rowShift;
            if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
                return (commonHigh | ((addr >> MJL_colShift) & (Addr)MJL_wordMask));
            } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) {
                return (commonHigh | ((addr >> MJL_rowShift) & (Addr)MJL_wordMask));
            } else {
                return (addr >> tagShift);
            }
        } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
            return (MJL_movColRight(addr) >> tagShift);
        } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) { // MJL_Temp: temporary fix for column 
            return (MJL_movColRight(MJL_swapRowColBits(addr)) >> tagShift);
        } else {
            return (addr >> tagShift);
        }
    }
    /* MJL_End */

    /**
     * Calculate the set index from the address.
     * @param addr The address to get the set from.
     * @return The set index of the address.
     */
    int extractSet(Addr addr) const override
    {
        return ((addr >> setShift) & setMask);
    }
    /* MJL_Begin */
    int MJL_extractSet(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir) const override
    {
        if (MJL_sameSetMapping) {
            return ((MJL_movColRight(addr) >> (setShift + MJL_rowShift)) & setMask);
        } else if (MJL_oracleProxy) {
            if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
                return ((MJL_movColRight(addr) >> setShift) & setMask);
            } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) {
                return ((MJL_movColRight(MJL_swapRowColBits(addr)) >> setShift) & setMask) + (numSets/2);
            } else {
                assert(MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow || MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
                return ((addr >> setShift) & setMask);
            }
        } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
            return ((MJL_movColRight(addr) >> setShift) & setMask);
        } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) { // MJL_Temp: temporary fix for column 
            return ((MJL_movColRight(MJL_swapRowColBits(addr)) >> setShift) & setMask);
        } else {
            return ((addr >> setShift) & setMask);
        }
    }
    /* MJL_End */

    /**
     * Align an address to the block size.
     * @param addr the address to align.
     * @return The block address.
     */
    Addr blkAlign(Addr addr) const
    {
        return (addr & ~(Addr)blkMask);
    }
    /* MJL_Begin */
    Addr MJL_blkAlign(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir) const
    {
        if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
            return (addr & ~(Addr)blkMask);
        } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) {
            return (addr & ~(Addr)MJL_blkMaskColumn);
        } else {
            return (addr & ~(Addr)blkMask);
        }
    }
    /* MJL_End */

    /**
     * Regenerate the block address from the tag.
     * @param tag The tag of the block.
     * @param set The set of the block.
     * @return The block address.
     */
    Addr regenerateBlkAddr(Addr tag, unsigned set) const override
    {
        return ((tag << tagShift) | ((Addr)set << setShift));
    }
    /* MJL_Begin */
    Addr MJL_regenerateBlkAddr(Addr tag, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, unsigned set) const override
    {
        if (MJL_sameSetMapping) {
            Addr commonHigh = (((tag >> MJL_rowShift) << tagShift) | ((Addr)set << setShift)) << MJL_rowShift;
            if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
                return MJL_movColLeft(commonHigh | ((tag & (Addr)MJL_wordMask) << setShift));
            } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) {
                return MJL_swapRowColBits(MJL_movColLeft(commonHigh | ((tag & (Addr)MJL_wordMask) << setShift)));
            } else {
                return ((tag << tagShift) | ((Addr)set << setShift));
            }
        } else if (MJL_oracleProxy) {
            if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
                return MJL_movColLeft(((tag << tagShift) | ((Addr)set << setShift)));
            } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) { // MJL_Temp: temporary fix for column 
                return MJL_swapRowColBits(MJL_movColLeft(((tag << tagShift) | ((Addr)(set - numSets/2) << setShift))));
            } else {
                assert(MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow || MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
                return ((tag << tagShift) | ((Addr)set << setShift));
            }

        } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
            return MJL_movColLeft(((tag << tagShift) | ((Addr)set << setShift)));
        } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) { // MJL_Temp: temporary fix for column 
            return MJL_swapRowColBits(MJL_movColLeft(((tag << tagShift) | ((Addr)set << setShift))));
        } else {
            return ((tag << tagShift) | ((Addr)set << setShift));
        }
    }
    Addr MJL_swapRowColBits(Addr addr) const override
    {
        Addr new_row = (addr >> MJL_colShift) & (Addr)MJL_wordMask;
        Addr new_col = (addr >> MJL_rowShift) & (Addr)MJL_wordMask;
        return ((addr & ~(((Addr)MJL_wordMask << MJL_colShift) | ((Addr)MJL_wordMask << MJL_rowShift))) | (new_row << MJL_rowShift) | (new_col << MJL_colShift));
    }
    /** 
     * Move column offset bits to right before row offset bits
     */
    Addr MJL_movColRight(Addr addr) const override
    {
        Addr col = (addr >> MJL_colShift) & (Addr)MJL_wordMask;
        Addr common_high = addr & MJL_commonHighMask;
        Addr common_low = addr & MJL_commonLowMask;
        Addr blk_offset = addr & blkMask;
        return (common_high | (common_low << (setShift - MJL_rowShift)) | (col << setShift) | blk_offset);
    }
    /**
     * Move the bits right before row offset bits to the column bits' place
     */
    Addr MJL_movColLeft(Addr addr) const override
    {
        Addr col = (addr >> setShift) & (Addr)MJL_wordMask;
        Addr common_high = addr & MJL_commonHighMask;
        Addr common_low = addr & (MJL_commonLowMask << (setShift - MJL_rowShift));
        Addr blk_offset = addr & blkMask;
        return (common_high | (common_low >> (setShift - MJL_rowShift)) | (col << MJL_colShift) | blk_offset);
    }
    /* MJL_End */

    /**
     * Called at end of simulation to complete average block reference stats.
     */
    void cleanupRefs() override;

    /**
     * Print all tags used
     */
    std::string print() const override;

    /**
     * Called prior to dumping stats to compute task occupancy
     */
    void computeStats() override;

    /**
     * Visit each block in the tag store and apply a visitor to the
     * block.
     *
     * The visitor should be a function (or object that behaves like a
     * function) that takes a cache block reference as its parameter
     * and returns a bool. A visitor can request the traversal to be
     * stopped by returning false, returning true causes it to be
     * called for the next block in the tag store.
     *
     * \param visitor Visitor to call on each block.
     */
    void forEachBlk(CacheBlkVisitor &visitor) override {
        for (unsigned i = 0; i < numSets * assoc; ++i) {
            if (!visitor(blks[i]))
                return;
        }
    }
};

#endif // __MEM_CACHE_TAGS_BASESETASSOC_HH__
