/*
 * Copyright (c) 2012-2013, 2015-2016 ARM Limited
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
 *          Steve Reinhardt
 *          Ron Dreslinski
 */

/**
 * @file
 * Declares a basic cache interface BaseCache.
 */

#ifndef __MEM_CACHE_BASE_HH__
#define __MEM_CACHE_BASE_HH__

#include <algorithm>
#include <list>
#include <string>
#include <vector>

#include "base/misc.hh"
#include "base/statistics.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "debug/Cache.hh"
#include "debug/CachePort.hh"
#include "mem/cache/mshr_queue.hh"
#include "mem/cache/write_queue.hh"
#include "mem/mem_object.hh"
#include "mem/packet.hh"
#include "mem/qport.hh"
#include "mem/request.hh"
#include "params/BaseCache.hh"
#include "sim/eventq.hh"
#include "sim/full_system.hh"
#include "sim/sim_exit.hh"
#include "sim/system.hh"

/**
 * A basic cache interface. Implements some common functions for speed.
 */
class BaseCache : public MemObject
{
  protected:
    /**
     * Indexes to enumerate the MSHR queues.
     */
    enum MSHRQueueIndex {
        MSHRQueue_MSHRs,
        MSHRQueue_WriteBuffer
    };

  public:
    /**
     * Reasons for caches to be blocked.
     */
    enum BlockedCause {
        Blocked_NoMSHRs = MSHRQueue_MSHRs,
        Blocked_NoWBBuffers = MSHRQueue_WriteBuffer,
        Blocked_NoTargets,
        NUM_BLOCKED_CAUSES
    };

  protected:

    /**
     * A cache master port is used for the memory-side port of the
     * cache, and in addition to the basic timing port that only sends
     * response packets through a transmit list, it also offers the
     * ability to schedule and send request packets (requests &
     * writebacks). The send event is scheduled through schedSendEvent,
     * and the sendDeferredPacket of the timing port is modified to
     * consider both the transmit list and the requests from the MSHR.
     */
    class CacheMasterPort : public QueuedMasterPort
    {

      public:

        /**
         * Schedule a send of a request packet (from the MSHR). Note
         * that we could already have a retry outstanding.
         */
        void schedSendEvent(Tick time)
        {
            DPRINTF(CachePort, "Scheduling send event at %llu\n", time);
            reqQueue.schedSendEvent(time);
        }

      protected:

        CacheMasterPort(const std::string &_name, BaseCache *_cache,
                        ReqPacketQueue &_reqQueue,
                        SnoopRespPacketQueue &_snoopRespQueue) :
            QueuedMasterPort(_name, _cache, _reqQueue, _snoopRespQueue)
        { }

        /**
         * Memory-side port always snoops.
         *
         * @return always true
         */
        virtual bool isSnooping() const { return true; }
    };

    /**
     * A cache slave port is used for the CPU-side port of the cache,
     * and it is basically a simple timing port that uses a transmit
     * list for responses to the CPU (or connected master). In
     * addition, it has the functionality to block the port for
     * incoming requests. If blocked, the port will issue a retry once
     * unblocked.
     */
    class CacheSlavePort : public QueuedSlavePort
    {

      public:

        /** Do not accept any new requests. */
        void setBlocked();

        /** Return to normal operation and accept new requests. */
        void clearBlocked();

        bool isBlocked() const { return blocked; }

      protected:

        CacheSlavePort(const std::string &_name, BaseCache *_cache,
                       const std::string &_label);

        /** A normal packet queue used to store responses. */
        RespPacketQueue queue;

        bool blocked;

        bool mustSendRetry;

      private:

        void processSendRetry();

        EventWrapper<CacheSlavePort,
                     &CacheSlavePort::processSendRetry> sendRetryEvent;

    };

    CacheSlavePort *cpuSidePort;
    CacheMasterPort *memSidePort;

    /* MJL_Begin */
    /**
     * Bloom filter to track the existance of row/column cache lines within a tile
     */
    class MJL_RowColBloomFilter {
        protected: 
            /** Entry of the bloom filter */
            class MJL_RowColBloomFilterEntry {
                private:
                    bool row_exist;
                    bool col_exist;
                    int row_count;
                    int col_count;
                    const int cache_num_of_cachelines;
                    bool hasRow() const { assert((row_count != 0) == row_exist) ; return row_exist; }
                    bool hasCol() const { assert((col_count != 0) == col_exist); return col_exist; }
                public:
                    MJL_RowColBloomFilterEntry(int in_cache_num_of_cachelines) : 
                        row_exist(false), col_exist(false), row_count(0), col_count(0), cache_num_of_cachelines(in_cache_num_of_cachelines) {}
                    ~MJL_RowColBloomFilterEntry() {}
                    /** Return the total number of cache lines counted */
                    unsigned total() const {
                        assert(row_count + col_count >= 0);
                        return (unsigned) (row_count + col_count);
                    }
                    /** Check if there exists crossing cache lines */
                    bool hasCross(MemCmd::MJL_DirAttribute blkDir) const {
                        bool cross_exist = false;
                        if (blkDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                            cross_exist = hasCol();
                        } else if (blkDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                            cross_exist = hasRow();
                        }
                        return cross_exist;
                    }
                    /** Increase the number of row/col cache lines in the entry */
                    void countBlkAlloc(MemCmd::MJL_DirAttribute blkDir) {
                        assert(row_count + col_count < cache_num_of_cachelines);
                        if (blkDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                            row_count++;
                            row_exist |= true;
                        } else if (blkDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                            col_count++;
                            col_exist |= true;
                        }
                    }
                    /** Decrease the number of row/col cache lines in the entry */
                    void countBlkEvict(MemCmd::MJL_DirAttribute blkDir) {
                        if (blkDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                            assert(row_count > 0);
                            row_count--;
                            if (row_count == 0) {
                                row_exist &= false;
                            }
                        } else if (blkDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                            assert(col_count > 0);
                            col_count--;
                            if (col_count == 0) {
                                col_exist &= false;
                            }
                        }
                    }
                    /** Test use */
                    std::string print() const {
                        std::ostringstream str;
                        str << "r_ex " << row_exist << ", c_ex " << col_exist << ", r_cnt " << row_count << ", c_cnt " << col_count;
                        return str.str();
                    }
            };
            std::string name;
            const unsigned size;
            const unsigned cache_num_of_cachelines;
            const unsigned MJL_rowWidth;
            /** The block size of the parent cache. */
            const unsigned blkSize;
            const unsigned hash_func_id;
            std::vector<MJL_RowColBloomFilterEntry> rol_col_BloomFilter;
            /** Stats */
            uint64_t MJL_bloomFilterFalsePositives;
            uint64_t MJL_bloomFilterTruePositives;
            uint64_t MJL_bloomFilterTrueNegatives;

            int floorLog2(unsigned x) const {
                assert(x > 0);

                int y = 0;

                if (x & 0xffff0000) { y += 16; x >>= 16; }
                if (x & 0x0000ff00) { y +=  8; x >>=  8; }
                if (x & 0x000000f0) { y +=  4; x >>=  4; }
                if (x & 0x0000000c) { y +=  2; x >>=  2; }
                if (x & 0x00000002) { y +=  1; }

                return y;
            }
            /** Obtain the tile address from address */
            Addr getTileAddr(Addr addr) const {
                int MJL_wordShift = floorLog2(sizeof(uint64_t));
                int MJL_lineShift = floorLog2(blkSize);
                int MJL_colShift = floorLog2(MJL_rowWidth) + floorLog2(blkSize);
                int MJL_colTileShift = MJL_colShift + MJL_lineShift - MJL_wordShift;
                uint64_t MJL_rowMask = (1 << MJL_colShift) -1;

                Addr tile_row_Id = addr >> MJL_colTileShift;
                Addr tile_col_Id = (addr & MJL_rowMask) >> MJL_lineShift;

                return (tile_row_Id << (MJL_colShift - MJL_lineShift)) | tile_col_Id;
            }
            /** Obtain hash of the tile address */
            Addr tileAddrHash(Addr tileAddr) const {
                Addr hash = tileAddr;
                Addr hash_row = tileAddr >> (floorLog2(MJL_rowWidth) + 2);
                Addr hash_col = (tileAddr & ((1 << floorLog2(MJL_rowWidth)) - 1)) >> 2;
                switch(hash_func_id) {
                    case 0: hash = tileAddr % size;
                        break;
                    case 1: hash = ((hash_row & ((1 << (floorLog2(size) - floorLog2(size)/2)) - 1)) << floorLog2(size)/2) | (hash_col & ((1 << (floorLog2(size)/2)) - 1));
                            assert(hash < size);
                        break;
                    case 2: hash_row = hash_row ^ (hash_row >> (floorLog2(size) - floorLog2(size)/2));
                            hash_col = hash_col ^ (hash_col >> floorLog2(size)/2);
                            hash = ((hash_row & ((1 << (floorLog2(size) - floorLog2(size)/2)) - 1)) << floorLog2(size)/2) | (hash_col & ((1 << (floorLog2(size)/2)) - 1));
                            assert(hash < size);
                        break;
                    default: hash = tileAddr % size;
                        break;
                }
                return hash;
            }
            /** Obtain hash of the address */
            Addr addrHash(Addr addr) const {
                return tileAddrHash(getTileAddr(addr));
            }
        public:
            MJL_RowColBloomFilter(std::string in_name, unsigned in_size, unsigned in_cache_num_of_cachelines, unsigned in_MJL_rowWidth, unsigned in_blkSize, unsigned in_hash_func_id):name(in_name), size(in_size), cache_num_of_cachelines(in_cache_num_of_cachelines), MJL_rowWidth(in_MJL_rowWidth), blkSize(in_blkSize), hash_func_id(in_hash_func_id), MJL_bloomFilterFalsePositives(0), MJL_bloomFilterTruePositives(0), MJL_bloomFilterTrueNegatives(0) {
                for (unsigned i = 0; i < size; ++i) {
                    rol_col_BloomFilter.emplace_back(cache_num_of_cachelines);
                }
            }
            ~MJL_RowColBloomFilter() {}
            /** Return the total number of cache lines counted */
            unsigned total() const {
                unsigned total = 0;
                for (auto bloomFilterEntry : rol_col_BloomFilter) {
                    total += bloomFilterEntry.total();
                }
                return total;
            }
            /** Add the cacheline to the bloom filter */
            void add(Addr addr, MemCmd::MJL_DirAttribute blkDir) {
                rol_col_BloomFilter[addrHash(addr)].countBlkAlloc(blkDir);
            }
            /** Remove the cacheline from the bloom filter */
            void remove(Addr addr, MemCmd::MJL_DirAttribute blkDir) {
                rol_col_BloomFilter[addrHash(addr)].countBlkEvict(blkDir);
            }
            /** Check in the bloom filter if there may exist crossing cachelines in the cache */
            bool hasCross(Addr addr, MemCmd::MJL_DirAttribute blkDir) const {
                return rol_col_BloomFilter[addrHash(addr)].hasCross(blkDir);
            }
            /** Count for the stats */
            void countStats(Addr addr, MemCmd::MJL_DirAttribute blkDir, bool hasCross) {
                bool bloomFilterHasCross = rol_col_BloomFilter[addrHash(addr)].hasCross(blkDir);
                if (!bloomFilterHasCross) {
                    assert(!hasCross);
                    MJL_bloomFilterTrueNegatives++;
                }
                if (!hasCross && bloomFilterHasCross) {
                    MJL_bloomFilterFalsePositives++;
                } else if (hasCross && bloomFilterHasCross) {
                    MJL_bloomFilterTruePositives++;
                }
            }
            /** Test use */
            std::string print() const {
                std::ostringstream str;
                str << name << ": " << std::endl;
                for (unsigned i = 0; i < size; ++i) {
                    if (rol_col_BloomFilter[i].hasCross(MemCmd::MJL_DirAttribute::MJL_IsRow) || rol_col_BloomFilter[i].hasCross(MemCmd::MJL_DirAttribute::MJL_IsColumn)) {
                        str << i << ": " << rol_col_BloomFilter[i].print() << std::endl;
                    }
                }
                return str.str();
            }

            std::string printStats() const {
                std::ostringstream str;
                str << name << " " << MJL_bloomFilterTrueNegatives << " " << MJL_bloomFilterFalsePositives << " " << MJL_bloomFilterTruePositives << std::endl;
                return str.str();
            }
    };
    /** Pointer to the Bloom Filter */
    MJL_RowColBloomFilter * MJL_rowColBloomFilter;

    /** For test use, explore the efficiency of multiple hash functions */
    class MJL_Test_RowColBloomFilters {
        private:
            const unsigned cache_num_of_cachelines;
            const unsigned MJL_rowWidth;
            /** The block size of the parent cache. */
            const unsigned blkSize;
            std::map< unsigned, std::map<unsigned, MJL_RowColBloomFilter*> > test_row_col_BloomFilters;
            std::vector< unsigned > hash_func_ids;
            std::vector< unsigned > sizes;
        public:
            MJL_Test_RowColBloomFilters(std::string in_name, unsigned in_cache_num_of_cachelines, unsigned in_MJL_rowWidth, unsigned in_blkSize, std::vector< unsigned > &in_hash_func_ids, std::vector< unsigned > &in_sizes):cache_num_of_cachelines(in_cache_num_of_cachelines), MJL_rowWidth(in_MJL_rowWidth), blkSize(in_blkSize), hash_func_ids(in_hash_func_ids), sizes(in_sizes) {
                for (auto hash_func_id : hash_func_ids) {
                    for (auto size : sizes) {
                        test_row_col_BloomFilters[hash_func_id][size] = new MJL_RowColBloomFilter(in_name + ".MJL_Test_bloomfilter_" + std::to_string(hash_func_id) + "_" + std::to_string(size), size, cache_num_of_cachelines, MJL_rowWidth, blkSize, hash_func_id);
                    }
                }
                std::cout << "MJL_TestBloomFilters_Configs:" << std::endl;
                std::cout << "hash_func_ids: ";
                for (auto hash_func_id : hash_func_ids) {
                    std::cout << hash_func_id << " ";
                }
                std::cout << std::endl;
                std::cout << "sizes: ";
                for (auto size : sizes) {
                    std::cout << size << " ";
                }
                std::cout << std::endl;
            }
            void test_add(Addr addr, MemCmd::MJL_DirAttribute blkDir) {
                for (auto hash_func_id : hash_func_ids) {
                    for (auto size : sizes) {
                        test_row_col_BloomFilters[hash_func_id][size]->add(addr, blkDir);
                    }
                }
            }
            void test_remove(Addr addr, MemCmd::MJL_DirAttribute blkDir) {
                for (auto hash_func_id : hash_func_ids) {
                    for (auto size : sizes) {
                        test_row_col_BloomFilters[hash_func_id][size]->remove(addr, blkDir);
                    }
                }
            }
            void test_hasCrossStatCountBloomFilters(Addr addr, MemCmd::MJL_DirAttribute blkDir, bool hasCross) {
                for (auto hash_func_id : hash_func_ids) {
                    for (auto size : sizes) {
                        test_row_col_BloomFilters[hash_func_id][size]->countStats(addr, blkDir, hasCross);
                    }
                }
            }
            void test_total(unsigned in_total) {
                for (auto hash_func_id : hash_func_ids) {
                    for (auto size : sizes) {
                        assert(in_total == test_row_col_BloomFilters[hash_func_id][size]->total());
                    }
                }
            }
            /** For test use */
            std::string print() {
                std::ostringstream str;
                for (auto hash_func_id : hash_func_ids) {
                    for (auto size : sizes) {
                        str << test_row_col_BloomFilters[hash_func_id][size]->print();
                    }
                }
                return str.str();
            }

            std::string printStats() {
                std::ostringstream str;
                for (auto hash_func_id : hash_func_ids) {
                    for (auto size : sizes) {
                        str << test_row_col_BloomFilters[hash_func_id][size]->printStats();
                    }
                }
                return str.str();
            }
    };
    MJL_Test_RowColBloomFilters * MJL_Test_rowColBloomFilters;
    
    /** The size of the bloom filter, 0 when there's no bloom filter */
    unsigned MJL_bloomFilterSize;
    /** 
     * Which function to use for the address hash
     * 0 and default: tileAddr % size
     */
    unsigned MJL_bloomFilterHashFuncId;
    /* MJL_End */

  protected:

    /** Miss status registers */
    MSHRQueue mshrQueue;

    /** Write/writeback buffer */
    WriteQueue writeBuffer;

    /**
     * Mark a request as in service (sent downstream in the memory
     * system), effectively making this MSHR the ordering point.
     */
    void markInService(MSHR *mshr, bool pending_modified_resp)
    {
        bool wasFull = mshrQueue.isFull();
        mshrQueue.markInService(mshr, pending_modified_resp);

        if (wasFull && !mshrQueue.isFull()) {
            clearBlocked(Blocked_NoMSHRs);
        }
    }

    void markInService(WriteQueueEntry *entry)
    {
        bool wasFull = writeBuffer.isFull();
        writeBuffer.markInService(entry);

        if (wasFull && !writeBuffer.isFull()) {
            clearBlocked(Blocked_NoWBBuffers);
        }
    }

    /**
     * Determine if we should allocate on a fill or not.
     *
     * @param cmd Packet command being added as an MSHR target
     *
     * @return Whether we should allocate on a fill or not
     */
    virtual bool allocOnFill(MemCmd cmd) const = 0;

    /**
     * Write back dirty blocks in the cache using functional accesses.
     */
    virtual void memWriteback() = 0;
    /**
     * Invalidates all blocks in the cache.
     *
     * @warn Dirty cache lines will not be written back to
     * memory. Make sure to call functionalWriteback() first if you
     * want the to write them to memory.
     */
    virtual void memInvalidate() = 0;
    /**
     * Determine if there are any dirty blocks in the cache.
     *
     * \return true if at least one block is dirty, false otherwise.
     */
    virtual bool isDirty() const = 0;

    /**
     * Determine if an address is in the ranges covered by this
     * cache. This is useful to filter snoops.
     *
     * @param addr Address to check against
     *
     * @return If the address in question is in range
     */
    bool inRange(Addr addr) const;

    /** Block size of this cache */
    const unsigned blkSize;
    /* MJL_Begin */
    /** Row size of this memory system in number of cachelines */
    const unsigned MJL_rowWidth;
    const bool MJL_defaultColumn;
    /** Whether this cache physically 2D */
    const bool MJL_2DCache;
    const bool MJL_has2DLLC;
    const int MJL_2DTransferType;
    const Cycles MJL_extra2DWriteLatency;
    /* MJL_End */

    /**
     * The latency of tag lookup of a cache. It occurs when there is
     * an access to the cache.
     */
    const Cycles lookupLatency;

    /**
     * The latency of data access of a cache. It occurs when there is
     * an access to the cache.
     */
    const Cycles dataLatency;

    /**
     * This is the forward latency of the cache. It occurs when there
     * is a cache miss and a request is forwarded downstream, in
     * particular an outbound miss.
     */
    const Cycles forwardLatency;

    /** The latency to fill a cache block */
    const Cycles fillLatency;

    /**
     * The latency of sending reponse to its upper level cache/core on
     * a linefill. The responseLatency parameter captures this
     * latency.
     */
    const Cycles responseLatency;

    /** The number of targets for each MSHR. */
    const int numTarget;

    /** Do we forward snoops from mem side port through to cpu side port? */
    bool forwardSnoops;

    /**
     * Is this cache read only, for example the instruction cache, or
     * table-walker cache. A cache that is read only should never see
     * any writes, and should never get any dirty data (and hence
     * never have to do any writebacks).
     */
    const bool isReadOnly;

    /**
     * Bit vector of the blocking reasons for the access path.
     * @sa #BlockedCause
     */
    uint8_t blocked;

    /** Increasing order number assigned to each incoming request. */
    uint64_t order;

    /** Stores time the cache blocked for statistics. */
    Cycles blockedCycle;

    /** Pointer to the MSHR that has no targets. */
    MSHR *noTargetMSHR;

    /** The number of misses to trigger an exit event. */
    Counter missCount;

    /**
     * The address range to which the cache responds on the CPU side.
     * Normally this is all possible memory addresses. */
    const AddrRangeList addrRanges;

  public:
    /** System we are currently operating in. */
    System *system;

    // Statistics
    /**
     * @addtogroup CacheStatistics
     * @{
     */

    /** Number of hits per thread for each type of command.
        @sa Packet::Command */
    Stats::Vector hits[MemCmd::NUM_MEM_CMDS];
    /** Number of hits for demand accesses. */
    Stats::Formula demandHits;
    /** Number of hit for all accesses. */
    Stats::Formula overallHits;

    /** Number of misses per thread for each type of command.
        @sa Packet::Command */
    Stats::Vector misses[MemCmd::NUM_MEM_CMDS];
    /** Number of misses for demand accesses. */
    Stats::Formula demandMisses;
    /** Number of misses for all accesses. */
    Stats::Formula overallMisses;

    /**
     * Total number of cycles per thread/command spent waiting for a miss.
     * Used to calculate the average miss latency.
     */
    Stats::Vector missLatency[MemCmd::NUM_MEM_CMDS];
    /** Total number of cycles spent waiting for demand misses. */
    Stats::Formula demandMissLatency;
    /** Total number of cycles spent waiting for all misses. */
    Stats::Formula overallMissLatency;

    /** The number of accesses per command and thread. */
    Stats::Formula accesses[MemCmd::NUM_MEM_CMDS];
    /** The number of demand accesses. */
    Stats::Formula demandAccesses;
    /** The number of overall accesses. */
    Stats::Formula overallAccesses;

    /** The miss rate per command and thread. */
    Stats::Formula missRate[MemCmd::NUM_MEM_CMDS];
    /** The miss rate of all demand accesses. */
    Stats::Formula demandMissRate;
    /** The miss rate for all accesses. */
    Stats::Formula overallMissRate;

    /** The average miss latency per command and thread. */
    Stats::Formula avgMissLatency[MemCmd::NUM_MEM_CMDS];
    /** The average miss latency for demand misses. */
    Stats::Formula demandAvgMissLatency;
    /** The average miss latency for all misses. */
    Stats::Formula overallAvgMissLatency;

    /** The total number of cycles blocked for each blocked cause. */
    Stats::Vector blocked_cycles;
    /** The number of times this cache blocked for each blocked cause. */
    Stats::Vector blocked_causes;

    /** The average number of cycles blocked for each blocked cause. */
    Stats::Formula avg_blocked;

    /** The number of times a HW-prefetched block is evicted w/o reference. */
    Stats::Scalar unusedPrefetches;

    /** Number of blocks written back per thread. */
    Stats::Vector writebacks;

    /** Number of misses that hit in the MSHRs per command and thread. */
    Stats::Vector mshr_hits[MemCmd::NUM_MEM_CMDS];
    /** Demand misses that hit in the MSHRs. */
    Stats::Formula demandMshrHits;
    /** Total number of misses that hit in the MSHRs. */
    Stats::Formula overallMshrHits;

    /** Number of misses that miss in the MSHRs, per command and thread. */
    Stats::Vector mshr_misses[MemCmd::NUM_MEM_CMDS];
    /** Demand misses that miss in the MSHRs. */
    Stats::Formula demandMshrMisses;
    /** Total number of misses that miss in the MSHRs. */
    Stats::Formula overallMshrMisses;

    /** Number of misses that miss in the MSHRs, per command and thread. */
    Stats::Vector mshr_uncacheable[MemCmd::NUM_MEM_CMDS];
    /** Total number of misses that miss in the MSHRs. */
    Stats::Formula overallMshrUncacheable;

    /** Total cycle latency of each MSHR miss, per command and thread. */
    Stats::Vector mshr_miss_latency[MemCmd::NUM_MEM_CMDS];
    /** Total cycle latency of demand MSHR misses. */
    Stats::Formula demandMshrMissLatency;
    /** Total cycle latency of overall MSHR misses. */
    Stats::Formula overallMshrMissLatency;

    /** Total cycle latency of each MSHR miss, per command and thread. */
    Stats::Vector mshr_uncacheable_lat[MemCmd::NUM_MEM_CMDS];
    /** Total cycle latency of overall MSHR misses. */
    Stats::Formula overallMshrUncacheableLatency;

#if 0
    /** The total number of MSHR accesses per command and thread. */
    Stats::Formula mshrAccesses[MemCmd::NUM_MEM_CMDS];
    /** The total number of demand MSHR accesses. */
    Stats::Formula demandMshrAccesses;
    /** The total number of MSHR accesses. */
    Stats::Formula overallMshrAccesses;
#endif

    /** The miss rate in the MSHRs pre command and thread. */
    Stats::Formula mshrMissRate[MemCmd::NUM_MEM_CMDS];
    /** The demand miss rate in the MSHRs. */
    Stats::Formula demandMshrMissRate;
    /** The overall miss rate in the MSHRs. */
    Stats::Formula overallMshrMissRate;

    /** The average latency of an MSHR miss, per command and thread. */
    Stats::Formula avgMshrMissLatency[MemCmd::NUM_MEM_CMDS];
    /** The average latency of a demand MSHR miss. */
    Stats::Formula demandAvgMshrMissLatency;
    /** The average overall latency of an MSHR miss. */
    Stats::Formula overallAvgMshrMissLatency;

    /** The average latency of an MSHR miss, per command and thread. */
    Stats::Formula avgMshrUncacheableLatency[MemCmd::NUM_MEM_CMDS];
    /** The average overall latency of an MSHR miss. */
    Stats::Formula overallAvgMshrUncacheableLatency;
    /* MJL_Begin */
    Stats::Scalar MJL_overallRowVecMisses;
    Stats::Scalar MJL_overallColVecMisses;
    Stats::Formula MJL_overallVecMisses;
    Stats::Scalar MJL_overallRowVecHits;
    Stats::Scalar MJL_overallColVecHits;
    Stats::Formula MJL_overallVecHits;
    Stats::Formula MJL_overallRowVecAccesses;
    Stats::Formula MJL_overallColVecAccesses;
    Stats::Formula MJL_overallVecAccesses;
    Stats::Scalar MJL_overallRowMisses;
    Stats::Scalar MJL_overallColumnMisses;
    Stats::Scalar MJL_overallRowHits;
    Stats::Scalar MJL_overallColumnHits;
    Stats::Formula MJL_overallRowAccesses;
    Stats::Formula MJL_overallColumnAccesses;
    Stats::Scalar MJL_rowAccColHit;
    Stats::Scalar MJL_colAccRowHit;
    Stats::Scalar MJL_conflictWBCount1;
    Stats::Scalar MJL_conflictWBCount2;
    Stats::Scalar MJL_conflictWBCount3;
    Stats::Scalar MJL_conflictWBCount4;
    Stats::Scalar MJL_mshrConflictCount;
    Stats::Scalar MJL_requestedBytes;
    Stats::Scalar MJL_touchedBytes;
    Stats::Scalar MJL_overallInterestedRowMisses;
    Stats::Scalar MJL_overallInterestedRowHits;
    Stats::Scalar MJL_overallInterestedColumnMisses;
    Stats::Scalar MJL_overallInterestedColumnHits;
    Stats::Formula MJL_overallInterestedMisses;
    Stats::Formula MJL_overallInterestedHits;
    Stats::Formula MJL_overallInterestedRowAccesses;
    Stats::Formula MJL_overallInterestedColumnAccesses;
    Stats::Formula MJL_overallInterestedAccesses;
    Stats::Scalar MJL_crossReadHitWords;
    Stats::Scalar MJL_crossReadHits;
    Stats::Scalar MJL_crossFullReadHits;
    Stats::Scalar MJL_crossWriteHitWords;
    Stats::Scalar MJL_crossWriteHits;
    Stats::Scalar MJL_crossFullWriteHits;
    Stats::Formula MJL_crossHitWords;
    Stats::Formula MJL_crossHits;
    Stats::Formula MJL_crossFullHits;
    Stats::Scalar MJL_bloomFilterFalsePositives;
    Stats::Scalar MJL_bloomFilterTruePositives;
    Stats::Scalar MJL_bloomFilterTrueNegatives;
    bool MJL_Debug_Out;
    /* MJL_End */

    /**
     * @}
     */

    /**
     * Register stats for this object.
     */
    virtual void regStats();

  public:
    BaseCache(const BaseCacheParams *p, unsigned blk_size);
    ~BaseCache() {}

    virtual void init();

    virtual BaseMasterPort &getMasterPort(const std::string &if_name,
                                          PortID idx = InvalidPortID);
    virtual BaseSlavePort &getSlavePort(const std::string &if_name,
                                        PortID idx = InvalidPortID);

    /**
     * Query block size of a cache.
     * @return  The block size
     */
    unsigned
    getBlockSize() const
    {
        return blkSize;
    }


    Addr blockAlign(Addr addr) const { return (addr & ~(Addr(blkSize - 1))); }
    /* MJL_Begin */
    bool MJL_is2DCache() const {
        return MJL_2DCache;
    }

    bool MJL_getHas2DLLC() const {
        return MJL_has2DLLC;
    }

    Addr MJL_blockAlign(Addr addr, MemCmd::MJL_DirAttribute MJL_dir) const {
        if (MJL_dir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
            return (addr & ~(Addr(blkSize - 1)));
        } else if (MJL_dir == MemCmd::MJL_DirAttribute::MJL_IsColumn) { // MJL_temp: temporary fix for column
            return (addr & ~(Addr((blkSize/sizeof(uint64_t) - 1) << (floorLog2(MJL_rowWidth) + floorLog2(blkSize))) | (sizeof(uint64_t) - 1)));
        } else {
            return (addr & ~(Addr(blkSize - 1)));
        }
    }

    Addr MJL_swapRowColBits(Addr addr) const {
        int MJL_rowShift = floorLog2(sizeof(uint64_t));
        uint64_t MJL_wordMask = blkSize/sizeof(uint64_t) - 1;
        int MJL_colShift = floorLog2(MJL_rowWidth) + floorLog2(blkSize);

        Addr new_row = (addr >> MJL_colShift) & (Addr)MJL_wordMask;
        Addr new_col = (addr >> MJL_rowShift) & (Addr)MJL_wordMask;
        return ((addr & ~(((Addr)MJL_wordMask << MJL_colShift) | ((Addr)MJL_wordMask << MJL_rowShift))) | (new_row << MJL_rowShift) | (new_col << MJL_colShift));
    }
    /**
     * MJL_baseAddr: starting address
     * MJL_cacheBlkDir: direction of the address
     * offset: which byte from the starting address to get the word address
     * return the address of the word
     */
    Addr MJL_addOffsetAddr(Addr MJL_baseAddr, MemCmd::MJL_DirAttribute MJL_cacheBlkDir, unsigned offset) const {
        if (MJL_cacheBlkDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
            return MJL_baseAddr + Addr(offset);
        } else if (MJL_cacheBlkDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) { // MJL_temp temporary fix for column
            return MJL_swapRowColBits(MJL_swapRowColBits(MJL_baseAddr) + Addr(offset));
        } else {
            return MJL_baseAddr + Addr(offset);
        }
    }

    /**
     * MJL_baseAddr: starting address
     * MJL_cacheBlkDir: direction of the address
     * offset: which byte from the starting address to get the word address
     * return the address of the word
     */
    Addr MJL_subOffsetAddr(Addr MJL_baseAddr, MemCmd::MJL_DirAttribute MJL_cacheBlkDir, unsigned offset) {
        if (MJL_cacheBlkDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
            return MJL_baseAddr - Addr(offset);
        } else if (MJL_cacheBlkDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) { // MJL_temp temporary fix for column
            return MJL_swapRowColBits(MJL_swapRowColBits(MJL_baseAddr) - Addr(offset));
        } else {
            return MJL_baseAddr - Addr(offset);
        }
    }

    /**
     * Updates the blocking/blocked information between targets to ensure ordering 
     * in the presence of a conflict. Should be used at each creation of a new target
     * to maintain memory access ordering on conflict.
     * @param mshr The mshr with the newly added target
     */
    void MJL_markBlockInfo(MSHR *mshr, bool new_mshr = true) {
        // Get the newly added target and packet information
        MSHR::Target* new_target = mshr->MJL_getLastTarget();
        PacketPtr pkt = new_target->pkt;
        Addr baseAddr = mshr->blkAddr;
        Addr size = pkt->getSize();
        assert(mshr->MJL_qEntryDir == pkt->MJL_getCmdDir() || size <= sizeof(uint64_t));


        MemCmd::MJL_DirAttribute crossBlkDir = pkt->MJL_getCmdDir();
        pkt->MJL_setDataDir(mshr->MJL_qEntryDir);
        Addr target_offset = pkt->getOffset(blkSize);
        if (pkt->MJL_getCmdDir() != mshr->MJL_qEntryDir) {
            pkt->MJL_setDataDir(crossBlkDir);
        }

        if (mshr->MJL_qEntryDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
            crossBlkDir = MemCmd::MJL_DirAttribute::MJL_IsColumn;
        } else if (mshr->MJL_qEntryDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
            crossBlkDir = MemCmd::MJL_DirAttribute::MJL_IsRow;
        }
        
        Addr target_cross_offset = pkt->MJL_getDirOffset(blkSize, crossBlkDir);

        // packets that are fetching blocks with stale data should have post invalidate, physically 2D cache should not have this problem since any stale data can be updated in cache
        if (pkt->MJL_hasStale()) {
            assert(!MJL_2DCache);
            new_target->MJL_postInvalidate = true;
        }
        // Search for crossing mshr entries
        for (Addr offset = 0; offset < blkSize; offset = offset + sizeof(uint64_t)) {
            Addr crossBlkAddr = MJL_blockAlign(MJL_addOffsetAddr(baseAddr, mshr->MJL_qEntryDir, offset), crossBlkDir); 
            MSHR* crossMshr =  mshrQueue.MJL_findMatch(crossBlkAddr, crossBlkDir, pkt->isSecure());
            // If an entry is found
            if (crossMshr) {
                MJL_mshrConflictCount++;
                // If the packet is a write and the crossing word is written
                if (pkt->isWrite() && (offset < target_offset + size && offset + sizeof(uint64_t) > target_offset)) {
                    /* MJL_Test */
                    if (MJL_Debug_Out && this->name().find("dcache") != std::string::npos) {
                        std::clog << "MJL_Debug: markblock " << mshr << "->" << new_target << std::endl;
                        std::clog << mshr->print() << std::endl;
                        std::clog << ", blocked by " << crossMshr << std::endl;
                        std::clog << crossMshr->print() << std::endl;
                    }
                    /* */
                    // The new target is blocked by the crossing mshr's last target
                    new_target->MJL_isBlockedBy.push_back(crossMshr->MJL_getLastTarget());
                    crossMshr->MJL_getLastTarget()->MJL_isBlocking.push_back(new_target);
                    // And the crossing mshr's last target should have post invalidate
                    crossMshr->MJL_getLastTarget()->MJL_postInvalidate = true;
                // Else if the packet is write but the crossing word is not written
                } else if (pkt->isWrite()) {
                    /* MJL_Test */
                    if (MJL_Debug_Out && this->name().find("dcache") != std::string::npos) {
                        std::clog << "MJL_Debug: markblock " << mshr << "->" << new_target << std::endl;
                        std::clog << mshr->print() << std::endl;
                        std::clog << ", blocked by " << crossMshr << std::endl;
                        std::clog << crossMshr->print() << std::endl;
                    }
                    /* */
                    // The new target is blocked by the crossing mshr's last target
                    new_target->MJL_isBlockedBy.push_back(crossMshr->MJL_getLastTarget());
                    crossMshr->MJL_getLastTarget()->MJL_isBlocking.push_back(new_target);
                    crossMshr->MJL_getLastTarget()->MJL_postWriteback = true;
                // Else if the packet is read
                // And If there is a write target in the crossing mshr
                } else if (crossMshr->MJL_getLastWriteTarget(target_cross_offset, blkSize, !new_mshr)) {
                    /* MJL_Test */
                    if (MJL_Debug_Out && this->name().find("dcache") != std::string::npos) {
                        std::clog << "MJL_Debug: markblock " << mshr << "->" << new_target << std::endl;
                        std::clog << mshr->print() << std::endl;
                        std::clog << ", blocked by " << crossMshr << std::endl;
                        std::clog << crossMshr->print() << std::endl;
                    }
                    /* */
                    // The new target is blocked by the crossing mshr's latest write target
                    new_target->MJL_isBlockedBy.push_back(crossMshr->MJL_getLastWriteTarget(target_cross_offset, blkSize, !new_mshr));
                    crossMshr->MJL_getLastWriteTarget(target_cross_offset, blkSize, !new_mshr)->MJL_isBlocking.push_back(new_target);
                    // And the crossing mshr's last write target should writeback
                    // crossMshr->MJL_getLastUnblockedTargetAfterLastWrite(target_cross_offset, blkSize)->MJL_postWriteback = true;
                    crossMshr->MJL_getLastWriteTarget(target_cross_offset, blkSize, !new_mshr)->MJL_postWriteback = true;
                // mark potential block information for when a target becomes the first of an mshr due to stale previous response and not being served
                } else if (crossMshr->MJL_getLastWriteTarget(target_cross_offset, blkSize, false)) {
                    /* MJL_Test */
                    if (MJL_Debug_Out && this->name().find("dcache") != std::string::npos) {
                        std::clog << "MJL_Debug: markblock " << mshr << "->" << new_target << std::endl;
                        std::clog << mshr->print() << std::endl;
                        std::clog << ", may be blocked by " << crossMshr << std::endl;
                        std::clog << crossMshr->print() << std::endl;
                    }
                    /* */
                    // The new target is blocked by the crossing mshr's latest write target
                    new_target->MJL_mayBeBlockedBy.push_back(crossMshr->MJL_getLastWriteTarget(target_cross_offset, blkSize, false));
                    crossMshr->MJL_getLastWriteTarget(target_cross_offset, blkSize, false)->MJL_mayBeBlocking.push_back(new_target);
                    // And the crossing mshr's last write target should writeback
                    // crossMshr->MJL_getLastUnblockedTargetAfterLastWrite(target_cross_offset, blkSize)->MJL_postWriteback = true;
                    crossMshr->MJL_getLastWriteTarget(target_cross_offset, blkSize, false)->MJL_postWriteback = true;
                // Else if the packet is write but the crossing word is not written and there is no write target in the crossing mshr
                /* Make sure that read do not pass dirty or writable if not asked for// Even read responses may pass dirty and writable, and the block was clean and unwritable at the time of the check at access, hence written back or lose writable should be checked at response.
                } else if (pkt->isWrite()) {
                    // The new target is blocked by the crossing mshr's last target
                    new_target->MJL_isBlockedBy.push_back(crossMshr->MJL_getLastTarget());
                    crossMshr->MJL_getLastTarget()->MJL_isBlocking.push_back(new_target);
                    crossMshr->MJL_getLastTarget()->MJL_postWriteback = true;*/
                // Else the crossing mshr's last target should have post lose writable
                // MJL_TODO: In physically 2D cache, the mshr entry should be blocked if there's an invalidation as well, but assuming that this never happens for now.
                } else {
                    crossMshr->MJL_getLastTarget()->MJL_postWriteback = true;
                    if (!pkt->needsWritable()) {
                        new_target->MJL_postWriteback = true;
                    }
                }

            }
        }
    }

    unsigned MJL_getRowWidth() const { return MJL_rowWidth; }
    virtual void MJL_markBlocked2D(PacketPtr pkt, MSHR * mshr) = 0;
    MJL_RowColBloomFilter * MJL_get_rowColBloomFilter() const { return MJL_rowColBloomFilter; }
    MJL_Test_RowColBloomFilters * MJL_get_Test_rowColBloomFilters() const { return MJL_Test_rowColBloomFilters; }
    /* MJL_End */

    const AddrRangeList &getAddrRanges() const { return addrRanges; }

    MSHR *allocateMissBuffer(PacketPtr pkt, Tick time, bool sched_send = true)
    {
        /* MJL_Begin */
        MSHR *mshr = nullptr;
        if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos || this->name().find("l3") != std::string::npos) {
            mshr = mshrQueue.allocate(MJL_blockAlign(pkt->getAddr(), pkt->MJL_getCmdDir()), blkSize,
                                        pkt, time, order++,
                                        allocOnFill(pkt->cmd));
        } else {
            mshr = mshrQueue.allocate(blockAlign(pkt->getAddr()), blkSize,
                                        pkt, time, order++,
                                        allocOnFill(pkt->cmd));
        }
        /* MJL_End */
        /* MJL_Comment 
        MSHR *mshr = mshrQueue.allocate(blockAlign(pkt->getAddr()), blkSize,
                                        pkt, time, order++,
                                        allocOnFill(pkt->cmd));
        */
        /* MJL_Begin */
        if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos || this->name().find("l3") != std::string::npos) {
            MJL_markBlockInfo(mshr);
        }
        if (MJL_2DCache && mshr->getTarget()->source != MSHR::Target::FromPrefetcher) { 
            MJL_markBlocked2D(pkt, mshr);
        }
        /* MJL_End */

        if (mshrQueue.isFull()) {
            setBlocked((BlockedCause)MSHRQueue_MSHRs);
        }

        if (sched_send) {
            // schedule the send
            schedMemSideSendEvent(time);
        }

        return mshr;
    }

    void allocateWriteBuffer(PacketPtr pkt, Tick time)
    {
        // should only see writes or clean evicts here
        assert(pkt->isWrite() || pkt->cmd == MemCmd::CleanEvict);

        /* MJL_Test 
        if ((this->name().find("dcache") != std::string::npos
                || this->name().find("l2") != std::string::npos)
                // || this->name().find("l3") != std::string::npos
                // && (pkt->req->hasPC() && pkt->req->getPC() >= 0x407360 && pkt->req->getPC() <=0x4074ab )
                // && ( MJL_debugOutFlag )
                && (pkt->getAddr() >= 0x38c000 && pkt->getAddr() < 0x390000)
            ) {
            std::cout << this->name() << "::MJL_Debug: allocateWriteBuffer " << pkt->print() << std::endl;
        }
         */
        /* MJL_Comment
        Addr blk_addr = blockAlign(pkt->getAddr());
        */
        /* MJL_Begin */
        Addr blk_addr = MJL_blockAlign(pkt->getAddr(), pkt->MJL_getDataDir());
        /* MJL_End */

        WriteQueueEntry *wq_entry =
        /* MJL_Comment
            writeBuffer.findMatch(blk_addr, pkt->isSecure());
        */
        /* MJL_Begin */
            writeBuffer.MJL_findMatch(blk_addr, pkt->MJL_getDataDir(), pkt->isSecure());

        uint64_t MJL_order;
        if (pkt->MJL_hasOrder) {
            MJL_order = pkt->MJL_order;
            pkt->MJL_hasOrder = false;
        } else {
            MJL_order = order++;
        }
        /* MJL_End */
        if (wq_entry && !wq_entry->inService) {
            DPRINTF(Cache, "Potential to merge writeback %s", pkt->print());
        }

        writeBuffer.allocate(blk_addr, blkSize, pkt, time, /* MJL_Begin */MJL_order/* MJL_End *//* MJL_Comment order++ */);

        if (writeBuffer.isFull()) {
            setBlocked((BlockedCause)MSHRQueue_WriteBuffer);
        }

        // schedule the send
        schedMemSideSendEvent(time);
    }

    /**
     * Returns true if the cache is blocked for accesses.
     */
    bool isBlocked() const
    {
        return blocked != 0;
    }

    /**
     * Marks the access path of the cache as blocked for the given cause. This
     * also sets the blocked flag in the slave interface.
     * @param cause The reason for the cache blocking.
     */
    void setBlocked(BlockedCause cause)
    {
        uint8_t flag = 1 << cause;
        if (blocked == 0) {
            blocked_causes[cause]++;
            blockedCycle = curCycle();
            cpuSidePort->setBlocked();
        }
        blocked |= flag;
        DPRINTF(Cache,"Blocking for cause %d, mask=%d\n", cause, blocked);
    }

    /**
     * Marks the cache as unblocked for the given cause. This also clears the
     * blocked flags in the appropriate interfaces.
     * @param cause The newly unblocked cause.
     * @warning Calling this function can cause a blocked request on the bus to
     * access the cache. The cache must be in a state to handle that request.
     */
    void clearBlocked(BlockedCause cause)
    {
        uint8_t flag = 1 << cause;
        blocked &= ~flag;
        DPRINTF(Cache,"Unblocking for cause %d, mask=%d\n", cause, blocked);
        if (blocked == 0) {
            blocked_cycles[cause] += curCycle() - blockedCycle;
            cpuSidePort->clearBlocked();
        }
    }

    /**
     * Schedule a send event for the memory-side port. If already
     * scheduled, this may reschedule the event at an earlier
     * time. When the specified time is reached, the port is free to
     * send either a response, a request, or a prefetch request.
     *
     * @param time The time when to attempt sending a packet.
     */
    void schedMemSideSendEvent(Tick time)
    {
        memSidePort->schedSendEvent(time);
    }

    virtual bool inCache(Addr addr, bool is_secure) const = 0;

    /* MJL_Begin */
    virtual bool MJL_inCache(Addr addr, MemCmd::MJL_DirAttribute MJL_cacheBlkDir, bool is_secure) const = 0;
    /* MJL_End */
    virtual bool inMissQueue(Addr addr, bool is_secure) const = 0;
    /* MJL_Begin */
    virtual bool MJL_inMissQueue(Addr addr, MemCmd::MJL_DirAttribute MJL_cacheBlkDir, bool is_secure) const = 0;
    virtual bool MJL_crossDirtyInCache(const PacketPtr &pkt) const = 0;
    virtual bool MJL_crossDirtyInMissQueue(const PacketPtr &pkt) const = 0;
    virtual bool MJL_crossDirtyInWriteBuffer(const PacketPtr &pkt) const = 0;
    virtual bool MJL_crossDirtyInCache(Addr addr, MemCmd::MJL_DirAttribute MJL_cacheBlkDir, bool is_secure) const = 0;
    virtual bool MJL_crossDirtyInMissQueue(Addr addr, MemCmd::MJL_DirAttribute MJL_cacheBlkDir, bool is_secure) const = 0;
    virtual bool MJL_crossDirtyInWriteBuffer(Addr addr, MemCmd::MJL_DirAttribute MJL_cacheBlkDir, bool is_secure) const = 0;
    virtual bool MJL_isInterestedAccess(Addr addr) const = 0;
    virtual bool MJL_isVecAccess(Addr addr) const = 0;
    /* MJL_End */

    void incMissCount(PacketPtr pkt)
    {
        assert(pkt->req->masterId() < system->maxMasters());
        misses[pkt->cmdToIndex()][pkt->req->masterId()]++;
        pkt->req->incAccessDepth();
        if (missCount) {
            --missCount;
            if (missCount == 0)
                exitSimLoop("A cache reached the maximum miss count");
        }
        /* MJL_Begin */
        if (pkt->MJL_cmdIsRow()) {
            MJL_overallRowMisses++;
            if (pkt->req->hasPC()) {
                if (MJL_isInterestedAccess(pkt->req->getPC())) {
                    MJL_overallInterestedRowMisses++;
                }
                if (MJL_isVecAccess(pkt->req->getPC())) {
                    MJL_overallRowVecMisses++;
                }
            }
        }  else if (pkt->MJL_cmdIsColumn()) {
            MJL_overallColumnMisses++;
            if (pkt->req->hasPC()) {
                if (MJL_isInterestedAccess(pkt->req->getPC())) {
                    MJL_overallInterestedColumnMisses++;
                }
                if (MJL_isVecAccess(pkt->req->getPC())) {
                    MJL_overallColVecMisses++;
                }
            }
        }
        /* MJL_End */
    }
    void incHitCount(PacketPtr pkt)
    {
        assert(pkt->req->masterId() < system->maxMasters());
        hits[pkt->cmdToIndex()][pkt->req->masterId()]++;

        /* MJL_Begin */
        if (pkt->MJL_cmdIsRow()) {
            MJL_overallRowHits++;
            if (pkt->req->hasPC()) {
                if (MJL_isInterestedAccess(pkt->req->getPC())) {
                    MJL_overallInterestedRowHits++;
                }
                if (MJL_isVecAccess(pkt->req->getPC())) {
                    MJL_overallRowVecHits++;
                }
            }
        }  else if (pkt->MJL_cmdIsColumn()) {
            MJL_overallColumnHits++;
            if (pkt->req->hasPC()) {
                if (MJL_isInterestedAccess(pkt->req->getPC())) {
                    MJL_overallInterestedColumnHits++;
                }
                if (MJL_isVecAccess(pkt->req->getPC())) {
                    MJL_overallColVecHits++;
                }
            }
        }
        /* MJL_End */
    }

};

#endif //__MEM_CACHE_BASE_HH__
