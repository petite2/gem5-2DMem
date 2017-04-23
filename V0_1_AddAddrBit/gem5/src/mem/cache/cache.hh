/*
 * Copyright (c) 2012-2016 ARM Limited
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
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
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
 *          Dave Greene
 *          Steve Reinhardt
 *          Ron Dreslinski
 *          Andreas Hansson
 */

/**
 * @file
 * Describes a cache based on template policies.
 */

#ifndef __MEM_CACHE_CACHE_HH__
#define __MEM_CACHE_CACHE_HH__

#include "base/misc.hh" // fatal, panic, and warn
#include "enums/Clusivity.hh"
#include "mem/cache/base.hh"
#include "mem/cache/blk.hh"
#include "mem/cache/mshr.hh"
#include "mem/cache/tags/base.hh"
#include "params/Cache.hh"
#include "sim/eventq.hh"
/* MJL_Begin */
#include <fstream>
#include <sstream>
/* MJL_End */

//Forward decleration
class BasePrefetcher;

/**
 * A template-policy based cache. The behavior of the cache can be altered by
 * supplying different template policies. TagStore handles all tag and data
 * storage @sa TagStore, \ref gem5MemorySystem "gem5 Memory System"
 */
class Cache : public BaseCache
{
  public:

    /** A typedef for a list of CacheBlk pointers. */
    typedef std::list<CacheBlk*> BlkList;

  protected:

    /**
     * The CPU-side port extends the base cache slave port with access
     * functions for functional, atomic and timing requests.
     */
    class CpuSidePort : public CacheSlavePort
    {
      private:

        // a pointer to our specific cache implementation
        Cache *cache;

      protected:

        virtual bool recvTimingSnoopResp(PacketPtr pkt);

        virtual bool recvTimingReq(PacketPtr pkt);

        virtual Tick recvAtomic(PacketPtr pkt);

        virtual void recvFunctional(PacketPtr pkt);

        virtual AddrRangeList getAddrRanges() const;

      public:
        /* MJL_Begin */
        // MJL_Test (partially) to see whether cache functions work as desired, recovering original packet address, direction, and respond command, but not just for test anymore...
        virtual bool sendTimingResp(PacketPtr pkt)
        {
            assert(pkt->isResponse());
            if (this->name().find("dcache") != std::string::npos) {
                std::cout << this->name() << "::sendTimingResp: hasPC? " << pkt->req->hasPC() << ", PC = ";
                if (pkt->req->hasPC()) {
                    std::cout << pkt->req->getPC();
                } else {
                    std::cout << "NoPC";
                }
                std::cout << ", hasContextId? " << pkt->req->hasContextId() << ", contextID = ";
                if (pkt->req->hasContextId()) {
                    std::cout << pkt->req->contextId();
                } else {
                    std::cout << "NoContextId";
                }
                std::cout << ", Size = " << pkt->getSize() << ", time = " << pkt->req->time() << ", Addr = ";
                std::cout << std::oct << pkt->getAddr();
                std::cout << std::dec << ", Dir = " << pkt->MJL_getCmdDir() << ", Cmd = " << pkt->cmd.MJL_getCmd();
                if (pkt->hasData()) {
                    std::cout << ", Data = ";
                    uint64_t MJL_data = 0;
                    std::memcpy(&MJL_data, pkt->getConstPtr<uint8_t>(), pkt->getSize());
                    std::cout << std::hex << MJL_data;
                } else {
                    std::cout << ", noData";
                }
                std::cout << std::dec << "\n";
                assert(!cache->MJL_testPktOrigParamList.empty());
                auto PC_it = cache->MJL_testPktOrigParamList.find(pkt->req->getPC());
                assert(PC_it != cache->MJL_testPktOrigParamList.end());
                auto time_it = PC_it->second.find(pkt->req->time());
                assert((time_it != PC_it->second.end()) && !time_it->second.empty());
                bool MJL_isUnaligned = false;
                auto unaligned_PC_it = cache->MJL_unalignedPacketList.find(pkt->req->getPC());
                if (unaligned_PC_it != cache->MJL_unalignedPacketList.end()) {
                    auto unaligned_time_it = unaligned_PC_it->second.find(pkt->req->time());
                    if (unaligned_time_it != unaligned_PC_it->second.end()) {
                        auto unaligned_seq_it = unaligned_time_it->second.find(pkt->MJL_testSeq);
                        if (unaligned_seq_it != unaligned_time_it->second.end()) {
                            MJL_isUnaligned = true;
                            if (pkt == std::get<0>(unaligned_seq_it->second)) {
                                cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq][0] = true;
                            } else if (pkt == std::get<1>(unaligned_seq_it->second)) {
                                cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq][1] = true;
                            } else {
                                assert((pkt == std::get<0>(unaligned_seq_it->second)) || (pkt == std::get<1>(unaligned_seq_it->second)));
                            }
                            if (cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq][0] && cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq][1]) {
                                PacketPtr MJL_origPacket = std::get<0>(cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq]);
                                PacketPtr MJL_sndPacket = std::get<1>(cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq]);
                                if (pkt->isRead()) {
                                    std::memcpy(MJL_origPacket->getPtr<uint8_t>() + MJL_origPacket->getSize(), MJL_sndPacket->getConstPtr<uint8_t>(), MJL_sndPacket->getSize());
                                } else if (pkt->isWrite()) {
                                    MJL_origPacket->MJL_setSize(MJL_origPacket->getSize() + MJL_sndPacket->getSize());
                                    MJL_origPacket->req->MJL_setSize(MJL_origPacket->getSize());
                                }
                                if (pkt != MJL_origPacket) {
                                    MJL_origPacket->headerDelay = pkt->headerDelay;
                                    MJL_origPacket->snoopDelay = pkt->snoopDelay;
                                    MJL_origPacket->payloadDelay = pkt->payloadDelay;
                                    MJL_origPacket->senderState = pkt->senderState;
                                    pkt = MJL_origPacket;
                                }
                                assert(pkt->getAddr() == std::get<0>((time_it->second)[pkt->MJL_testSeq]));
                                // assert(pkt->MJL_getCmdDir() == std::get<1>((time_it->second)[pkt->MJL_testSeq]));
                                assert(pkt->cmd.MJL_getCmd() == std::get<2>((time_it->second)[pkt->MJL_testSeq]));
                                pkt->setAddr(std::get<0>((time_it->second)[pkt->MJL_testSeq]));
                                // pkt->cmd.MJL_setCmdDir(std::get<1>((time_it->second)[pkt->MJL_testSeq]));
                                // pkt->MJL_setDataDir(std::get<1>((time_it->second)[pkt->MJL_testSeq]));
                                pkt->cmd = std::get<2>((time_it->second)[pkt->MJL_testSeq]);
                                delete MJL_sndPacket;
                                unaligned_time_it->second.erase(pkt->MJL_testSeq);
                                time_it->second.erase(pkt->MJL_testSeq);
                            }
                        }
                    }
                }
                if (!MJL_isUnaligned) {
                    assert(pkt->getAddr() == std::get<0>((time_it->second)[pkt->MJL_testSeq]));
                    // assert(pkt->MJL_getCmdDir() == std::get<1>((time_it->second)[pkt->MJL_testSeq]));
                    assert(pkt->cmd.MJL_getCmd() == std::get<2>((time_it->second)[pkt->MJL_testSeq]));
                    pkt->setAddr(std::get<0>((time_it->second)[pkt->MJL_testSeq]));
                    // pkt->cmd.MJL_setCmdDir(std::get<1>((time_it->second)[pkt->MJL_testSeq]));
                    // pkt->MJL_setDataDir(std::get<1>((time_it->second)[pkt->MJL_testSeq]));
                    pkt->cmd = std::get<2>((time_it->second)[pkt->MJL_testSeq]);
                    time_it->second.erase(pkt->MJL_testSeq);
                }
            }
            return CacheSlavePort::sendTimingResp(pkt);
        }
        /* MJL_End */

        CpuSidePort(const std::string &_name, Cache *_cache,
                    const std::string &_label);

    };

    /**
     * Override the default behaviour of sendDeferredPacket to enable
     * the memory-side cache port to also send requests based on the
     * current MSHR status. This queue has a pointer to our specific
     * cache implementation and is used by the MemSidePort.
     */
    class CacheReqPacketQueue : public ReqPacketQueue
    {

      protected:

        Cache &cache;
        SnoopRespPacketQueue &snoopRespQueue;

      public:

        CacheReqPacketQueue(Cache &cache, MasterPort &port,
                            SnoopRespPacketQueue &snoop_resp_queue,
                            const std::string &label) :
            ReqPacketQueue(cache, port, label), cache(cache),
            snoopRespQueue(snoop_resp_queue) { }

        /**
         * Override the normal sendDeferredPacket and do not only
         * consider the transmit list (used for responses), but also
         * requests.
         */
        virtual void sendDeferredPacket();

        /**
         * Check if there is a conflicting snoop response about to be
         * send out, and if so simply stall any requests, and schedule
         * a send event at the same time as the next snoop response is
         * being sent out.
         */
        bool checkConflictingSnoop(Addr addr)
        {
            if (snoopRespQueue.hasAddr(addr)) {
                DPRINTF(CachePort, "Waiting for snoop response to be "
                        "sent\n");
                Tick when = snoopRespQueue.deferredPacketReadyTime();
                schedSendEvent(when);
                return true;
            }
            return false;
        }
    };

    /**
     * The memory-side port extends the base cache master port with
     * access functions for functional, atomic and timing snoops.
     */
    class MemSidePort : public CacheMasterPort
    {
      private:

        /** The cache-specific queue. */
        CacheReqPacketQueue _reqQueue;

        SnoopRespPacketQueue _snoopRespQueue;

        // a pointer to our specific cache implementation
        Cache *cache;

      protected:

        virtual void recvTimingSnoopReq(PacketPtr pkt);

        virtual bool recvTimingResp(PacketPtr pkt);

        virtual Tick recvAtomicSnoop(PacketPtr pkt);

        virtual void recvFunctionalSnoop(PacketPtr pkt);

      public:

        MemSidePort(const std::string &_name, Cache *_cache,
                    const std::string &_label);
    };

    /** Tag and data Storage */
    BaseTags *tags;

    /** Prefetcher */
    BasePrefetcher *prefetcher;

    /** Temporary cache block for occasional transitory use */
    CacheBlk *tempBlock;

    /**
     * This cache should allocate a block on a line-sized write miss.
     */
    const bool doFastWrites;

    /**
     * Turn line-sized writes into WriteInvalidate transactions.
     */
    void promoteWholeLineWrites(PacketPtr pkt);

    /**
     * Notify the prefetcher on every access, not just misses.
     */
    const bool prefetchOnAccess;

     /**
     * Clusivity with respect to the upstream cache, determining if we
     * fill into both this cache and the cache above on a miss. Note
     * that we currently do not support strict clusivity policies.
     */
    const Enums::Clusivity clusivity;

     /**
     * Determine if clean lines should be written back or not. In
     * cases where a downstream cache is mostly inclusive we likely
     * want it to act as a victim cache also for lines that have not
     * been modified. Hence, we cannot simply drop the line (or send a
     * clean evict), but rather need to send the actual data.
     */
    const bool writebackClean;

    /**
     * Upstream caches need this packet until true is returned, so
     * hold it for deletion until a subsequent call
     */
    std::unique_ptr<Packet> pendingDelete;

    /**
     * Writebacks from the tempBlock, resulting on the response path
     * in atomic mode, must happen after the call to recvAtomic has
     * finished (for the right ordering of the packets). We therefore
     * need to hold on to the packets, and have a method and an event
     * to send them.
     */
    PacketPtr tempBlockWriteback;
    /* MJL_Begin */
    /**
     * The map that contains the mapping from PC to direction
     */
    std::map<Addr, CacheBlk::MJL_CacheBlkDir> MJL_PC2DirMap;

    /**
     * The name of the input file that contains the mapping information from PC to direction
     */
    std::string MJL_PC2DirFilename;
    /**
     * The method to read the map information from file
     */
    void MJL_readPC2DirMap () {
        std::ifstream MJL_PC2DirFile;
        std::string line;
        Addr tempPC;
        char tempDir;

        MJL_PC2DirFile.open(MJL_PC2DirFilename);
        if (MJL_PC2DirFile.is_open()) {
            while (getline(MJL_PC2DirFile, line)) {
                std::stringstream(line) >> tempPC >> tempDir;
                if (MJL_PC2DirMap.find(tempPC) != MJL_PC2DirMap.end()) {
                    std::cout << "MJL_Error: Redefinition of instruction direction found!\n";
                }
                if (tempDir == 'R') {
                    MJL_PC2DirMap[tempPC] =  CacheBlk::MJL_CacheBlkDir::MJL_IsRow;
                } else if (tempDir == 'C') {
                    MJL_PC2DirMap[tempPC] =  CacheBlk::MJL_CacheBlkDir::MJL_IsColumn;
                } else {
                    std::cout << "MJL_Error: Invalid input direction annotation!\n";
                    assert((tempDir == 'R') || (tempDir == 'C'));
                }
                // MJL_Test: For test use
                if (MJL_PC2DirMap.find(tempPC) != MJL_PC2DirMap.end()) {
                    std::cout << "PC: " << MJL_PC2DirMap.find(tempPC)->first << ", Dir: " << MJL_PC2DirMap.find(tempPC)->second << "\n";
                }
            }
        }
        else {
            std::cout << "MJL_Error: Could not open input file!\n";
            assert(MJL_PC2DirFile.is_open());
        }
        MJL_PC2DirFile.close();
    }

    // MJL_Test: For test use 
    std::list< std::tuple<Addr, CacheBlk::MJL_CacheBlkDir, MemCmd::Command> > MJL_testInputList;
    // not just test use anymore
    std::map< Addr, std::map< Tick, std::map< int , std::tuple<Addr, CacheBlk::MJL_CacheBlkDir, MemCmd::Command> > > > MJL_testPktOrigParamList;// [PC][_time][MJL_testSeq] = <addr, dir, cmd>
    std::map< Addr, std::map< Tick, std::map< int , std::tuple<PacketPtr, PacketPtr> > > > MJL_unalignedPacketList;  //[PC][_time][MJL_testSeq] = <OrigPtr, SecPtr>
    std::map< Addr, std::map< Tick, std::map< int , std::map< int, bool > > > > MJL_unalignedPacketCount;  //[PC][_time][MJL_testSeq][PacketAddrSeq] = received
    bool MJL_sndPacketWaiting;
    PacketPtr MJL_retrySndPacket;

    // MJL_Test: For test use 
    void MJL_readTestInput () {
        std::ifstream MJL_testInputFile;
        std::string line;

        Addr tempAddr;
        char intempDir;
        char intempCmd;
        CacheBlk::MJL_CacheBlkDir tempDir;
        MemCmd::Command tempCmd;

        MJL_testInputFile.open("CacheTestInput.txt");
        if (MJL_testInputFile.is_open()) {
            while (getline(MJL_testInputFile, line)) {
                if (line == "End") break;
                std::stringstream(line) >> tempAddr >> intempDir >> intempCmd;
                if (intempDir == 'R') {
                    tempDir =  CacheBlk::MJL_CacheBlkDir::MJL_IsRow;
                } else if (intempDir == 'C') {
                    tempDir =  CacheBlk::MJL_CacheBlkDir::MJL_IsColumn;
                } else {
                    std::cout << "MJL_Error: Invalid input direction annotation!\n";
                    assert((intempDir == 'R') || (intempDir == 'C'));
                }
                if (intempCmd == 'R') {
                    tempCmd =  MemCmd::Command::ReadReq;
                } else if (intempCmd == 'W') {
                    tempCmd = MemCmd::Command::WriteReq;
                } else {
                    std::cout << "MJL_Error: Invalid input command annotation!\n";
                    assert((intempCmd == 'R') || (intempCmd == 'W'));
                }
                MJL_testInputList.push_back(std::tuple<Addr, CacheBlk::MJL_CacheBlkDir, MemCmd::Command> (tempAddr, tempDir, tempCmd));
            }
        } else {
            std::cout << "MJL_Error: Could not open test input file!\n";
            //assert(MJL_testInputFile.is_open());
        }
        MJL_testInputFile.close();

        std::cout << "After reading the file\n";
        for (auto it = MJL_testInputList.begin(); it != MJL_testInputList.end(); ++it) {
            std::cout << "Addr: " << std::get<0>(*it) << ", Dir: " << std::get<1>(*it) << ", Cmd: " <<  std::get<2>(*it) << "\n";
        }
    }
    /* */
    /* MJL_End */

    /**
     * Send the outstanding tempBlock writeback. To be called after
     * recvAtomic finishes in cases where the block we filled is in
     * fact the tempBlock, and now needs to be written back.
     */
    void writebackTempBlockAtomic() {
        assert(tempBlockWriteback != nullptr);
        PacketList writebacks{tempBlockWriteback};
        doWritebacksAtomic(writebacks);
        tempBlockWriteback = nullptr;
    }

    /**
     * An event to writeback the tempBlock after recvAtomic
     * finishes. To avoid other calls to recvAtomic getting in
     * between, we create this event with a higher priority.
     */
    EventWrapper<Cache, &Cache::writebackTempBlockAtomic> \
        writebackTempBlockAtomicEvent;

    /**
     * Store the outstanding requests that we are expecting snoop
     * responses from so we can determine which snoop responses we
     * generated and which ones were merely forwarded.
     */
    std::unordered_set<RequestPtr> outstandingSnoop;

    /**
     * Does all the processing necessary to perform the provided request.
     * @param pkt The memory request to perform.
     * @param blk The cache block to be updated.
     * @param lat The latency of the access.
     * @param writebacks List for any writebacks that need to be performed.
     * @return Boolean indicating whether the request was satisfied.
     */
    bool access(PacketPtr pkt, CacheBlk *&blk,
                Cycles &lat, PacketList &writebacks);

    /**
     *Handle doing the Compare and Swap function for SPARC.
     */
    void cmpAndSwap(CacheBlk *blk, PacketPtr pkt);

    /**
     * Find a block frame for new block at address addr targeting the
     * given security space, assuming that the block is not currently
     * in the cache.  Append writebacks if any to provided packet
     * list.  Return free block frame.  May return nullptr if there are
     * no replaceable blocks at the moment.
     */
    CacheBlk *allocateBlock(Addr addr, bool is_secure, PacketList &writebacks);
    /* MJL_Begin */
    CacheBlk *MJL_allocateBlock(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure, PacketList &writebacks);
    /* MJL_End */

    /**
     * Invalidate a cache block.
     *
     * @param blk Block to invalidate
     */
    void invalidateBlock(CacheBlk *blk);
    /* MJL_Begin */
    // MJL_TODO: Make this happen, but probably after the only one core version happens
    /**
     * MJL_baseAddr: starting address
     * MJL_cacheBlkDir: direction of the address
     * offset: which byte from the starting address to get the word address
     * return the address of the word
     */
    Addr MJL_addOffsetAddr(Addr MJL_baseAddr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, unsigned offset) {
        if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
            return MJL_baseAddr + Addr(offset);
        } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) { // MJL_temp temporary fix for column
            return tags->MJL_swapRowColBits(tags->MJL_swapRowColBits(MJL_baseAddr) + Addr(offset));
        } else {
            return MJL_baseAddr + Addr(offset);
        }
    }
    /**
     * Invalidate blocks that are cross direction of the addresses written
     * MJL_written_addr: starting address of the words written
     * MJL_cacheBlkDir: direction of the block written
     * size: size of bytes writte, can be used to determine how many words are written and their address
     * is_secure: used in MJL_findBlk
     */
    void MJL_invalidateOtherBlocks(Addr MJL_written_addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, unsigned size, bool is_secure) {
        CacheBlk::MJL_CacheBlkDir MJL_diffDir;
        Addr MJL_writtenWord_addr;
        CacheBlk *MJL_diffDir_blk;
        if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
            MJL_diffDir = CacheBlk::MJL_CacheBlkDir::MJL_IsColumn;
        } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) {
            MJL_diffDir = CacheBlk::MJL_CacheBlkDir::MJL_IsRow;
        } else {
            assert(MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow || MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
            MJL_diffDir = CacheBlk::MJL_CacheBlkDir::MJL_IsColumn;
        }
        for (unsigned offset = 0; offset < size; offset = offset + sizeof(uint64_t)) {
            MJL_writtenWord_addr = MJL_addOffsetAddr(MJL_written_addr, MJL_cacheBlkDir, offset);
	        MJL_diffDir_blk = tags->MJL_findBlock(MJL_writtenWord_addr, MJL_diffDir, is_secure);
            if (MJL_diffDir_blk != nullptr) {
                if (MJL_diffDir_blk->isValid()) {
                    // MJL_TODO: should check if there's an upgrade miss waiting on this I guess?
                    invalidateBlock(MJL_diffDir_blk);
                }
            }
        }
    }
    /**
     * Mark blocks that are cross direction of the addresses waiting on writable unreadable
     * MJL_upgrade_addr: starting address of the words waiting on writable
     * MJL_cacheBlkDir: direction of the block waiting on writable
     * size: size of bytes going to be written, can be used to determine how many words are going to be written and their address
     * is_secure: used in MJL_findBlk
     */
    void MJL_unreadableOtherBlocks(Addr MJL_upgrade_addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, unsigned size, bool is_secure) {
        CacheBlk::MJL_CacheBlkDir MJL_diffDir;
        Addr MJL_writtenWord_addr;
        CacheBlk *MJL_diffDir_blk;
        if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
            MJL_diffDir = CacheBlk::MJL_CacheBlkDir::MJL_IsColumn;
        } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) {
            MJL_diffDir = CacheBlk::MJL_CacheBlkDir::MJL_IsRow;
        } else {
            assert(MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow || MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
            MJL_diffDir = CacheBlk::MJL_CacheBlkDir::MJL_IsColumn;
        }
        for (unsigned offset = 0; offset < size; offset = offset + sizeof(uint64_t)) {
            MJL_writtenWord_addr = MJL_addOffsetAddr(MJL_upgrade_addr, MJL_cacheBlkDir, offset);
	        MJL_diffDir_blk = tags->MJL_findBlock(MJL_writtenWord_addr, MJL_diffDir, is_secure);
            if (MJL_diffDir_blk != nullptr) {
                if (MJL_diffDir_blk->isValid()) {
                    MJL_diffDir_blk->status &= ~BlkReadable;
                }
            }
        }
    }
    // For each cross directional address of the addresses written (MJL_written_addr <= addr < MJL_written_addr + MJL_size, invalidate the block if it exists. Should be needing writeback before invalidation. )
    // Code for handling dirty and all 
    // if (blk->isDirty() || writebackClean) {
    //     // Save writeback packet for handling by caller
    //     writebacks.push_back(writebackBlk(blk));
    // } else {
    //     writebacks.push_back(cleanEvictBlk(blk));
    // }
    // Assuming sharing only happens in L1, and L2 is unified.
    // For any pair of row and column block holding the same word
    // The statuses are MOESI, where Modified and Exclusive means this cache is the
    // only one holding the block, and Owned and Shared means there are other caches 
    // That has the block. I is Invalid.
    // The problem is when can we Invalidate the Other Blocks, should be treated as 
    // an upgrade miss and the update cannot happen. And then we need to figure out how
    // to resolve the upgrade misses. 
    // I think that the other ones should be marked not readable as well if we are 
    // waiting on an upgrade miss, and becomes invalid when we have the right to write
    // Column one is the status of the block that we want to update, Row is the others
    // Status|       M       |       O       |       E       |       S       |  I
    //   M   |               |               |               | Yes, inform S | ---- 
    //   O   |               |               |               |               | ---- 
    //   E   |               |               |               | Yes, inform S | ---- 
    //   S   |               |               |               |               | ---- 
    //   I   |               |               |               |               | ---- 
    // Apparently it is ok to invalidate if the other is shared, but need to tell the others that the shared copy is not there anymore... 
    // Apparently nothing to invalidate if the others are invalid...
    /* MJL_End */

    /**
     * Maintain the clusivity of this cache by potentially
     * invalidating a block. This method works in conjunction with
     * satisfyRequest, but is separate to allow us to handle all MSHR
     * targets before potentially dropping a block.
     *
     * @param from_cache Whether we have dealt with a packet from a cache
     * @param blk The block that should potentially be dropped
     */
    void maintainClusivity(bool from_cache, CacheBlk *blk);

    /**
     * Populates a cache block and handles all outstanding requests for the
     * satisfied fill request. This version takes two memory requests. One
     * contains the fill data, the other is an optional target to satisfy.
     * @param pkt The memory request with the fill data.
     * @param blk The cache block if it already exists.
     * @param writebacks List for any writebacks that need to be performed.
     * @param allocate Whether to allocate a block or use the temp block
     * @return Pointer to the new cache block.
     */
    CacheBlk *handleFill(PacketPtr pkt, CacheBlk *blk,
                         PacketList &writebacks, bool allocate);

    /**
     * Determine whether we should allocate on a fill or not. If this
     * cache is mostly inclusive with regards to the upstream cache(s)
     * we always allocate (for any non-forwarded and cacheable
     * requests). In the case of a mostly exclusive cache, we allocate
     * on fill if the packet did not come from a cache, thus if we:
     * are dealing with a whole-line write (the latter behaves much
     * like a writeback), the original target packet came from a
     * non-caching source, or if we are performing a prefetch or LLSC.
     *
     * @param cmd Command of the incoming requesting packet
     * @return Whether we should allocate on the fill
     */
    inline bool allocOnFill(MemCmd cmd) const override
    {
        return clusivity == Enums::mostly_incl ||
            cmd == MemCmd::WriteLineReq ||
            cmd == MemCmd::ReadReq ||
            cmd == MemCmd::WriteReq ||
            cmd.isPrefetch() ||
            cmd.isLLSC();
    }

    /**
     * Performs the access specified by the request.
     * @param pkt The request to perform.
     * @return The result of the access.
     */
    bool recvTimingReq(PacketPtr pkt);

    /**
     * Insert writebacks into the write buffer
     */
    void doWritebacks(PacketList& writebacks, Tick forward_time);

    /**
     * Send writebacks down the memory hierarchy in atomic mode
     */
    void doWritebacksAtomic(PacketList& writebacks);

    /**
     * Handling the special case of uncacheable write responses to
     * make recvTimingResp less cluttered.
     */
    void handleUncacheableWriteResp(PacketPtr pkt);

    /**
     * Handles a response (cache line fill/write ack) from the bus.
     * @param pkt The response packet
     */
    void recvTimingResp(PacketPtr pkt);

    /**
     * Snoops bus transactions to maintain coherence.
     * @param pkt The current bus transaction.
     */
    void recvTimingSnoopReq(PacketPtr pkt);

    /**
     * Handle a snoop response.
     * @param pkt Snoop response packet
     */
    void recvTimingSnoopResp(PacketPtr pkt);

    /**
     * Performs the access specified by the request.
     * @param pkt The request to perform.
     * @return The number of ticks required for the access.
     */
    Tick recvAtomic(PacketPtr pkt);

    /**
     * Snoop for the provided request in the cache and return the estimated
     * time taken.
     * @param pkt The memory request to snoop
     * @return The number of ticks required for the snoop.
     */
    Tick recvAtomicSnoop(PacketPtr pkt);

    /**
     * Performs the access specified by the request.
     * @param pkt The request to perform.
     * @param fromCpuSide from the CPU side port or the memory side port
     */
    void functionalAccess(PacketPtr pkt, bool fromCpuSide);

    /**
     * Perform any necessary updates to the block and perform any data
     * exchange between the packet and the block. The flags of the
     * packet are also set accordingly.
     *
     * @param pkt Request packet from upstream that hit a block
     * @param blk Cache block that the packet hit
     * @param deferred_response Whether this hit is to block that
     *                          originally missed
     * @param pending_downgrade Whether the writable flag is to be removed
     *
     * @return True if the block is to be invalidated
     */
    void satisfyRequest(PacketPtr pkt, CacheBlk *blk,
                        bool deferred_response = false,
                        bool pending_downgrade = false);

    void doTimingSupplyResponse(PacketPtr req_pkt, const uint8_t *blk_data,
                                bool already_copied, bool pending_inval);

    /**
     * Perform an upward snoop if needed, and update the block state
     * (possibly invalidating the block). Also create a response if required.
     *
     * @param pkt Snoop packet
     * @param blk Cache block being snooped
     * @param is_timing Timing or atomic for the response
     * @param is_deferred Is this a deferred snoop or not?
     * @param pending_inval Do we have a pending invalidation?
     *
     * @return The snoop delay incurred by the upwards snoop
     */
    uint32_t handleSnoop(PacketPtr pkt, CacheBlk *blk,
                         bool is_timing, bool is_deferred, bool pending_inval);

    /**
     * Create a writeback request for the given block.
     * @param blk The block to writeback.
     * @return The writeback request for the block.
     */
    PacketPtr writebackBlk(CacheBlk *blk);

    /**
     * Create a CleanEvict request for the given block.
     * @param blk The block to evict.
     * @return The CleanEvict request for the block.
     */
    PacketPtr cleanEvictBlk(CacheBlk *blk);


    void memWriteback() override;
    void memInvalidate() override;
    bool isDirty() const override;

    /**
     * Cache block visitor that writes back dirty cache blocks using
     * functional writes.
     *
     * \return Always returns true.
     */
    bool writebackVisitor(CacheBlk &blk);
    /**
     * Cache block visitor that invalidates all blocks in the cache.
     *
     * @warn Dirty cache lines will not be written back to memory.
     *
     * \return Always returns true.
     */
    bool invalidateVisitor(CacheBlk &blk);

    /**
     * Create an appropriate downstream bus request packet for the
     * given parameters.
     * @param cpu_pkt  The miss that needs to be satisfied.
     * @param blk The block currently in the cache corresponding to
     * cpu_pkt (nullptr if none).
     * @param needsWritable Indicates that the block must be writable
     * even if the request in cpu_pkt doesn't indicate that.
     * @return A new Packet containing the request, or nullptr if the
     * current request in cpu_pkt should just be forwarded on.
     */
    PacketPtr createMissPacket(PacketPtr cpu_pkt, CacheBlk *blk,
                               bool needsWritable) const;

    /**
     * Return the next queue entry to service, either a pending miss
     * from the MSHR queue, a buffered write from the write buffer, or
     * something from the prefetcher. This function is responsible
     * for prioritizing among those sources on the fly.
     */
    QueueEntry* getNextQueueEntry();

    /**
     * Send up a snoop request and find cached copies. If cached copies are
     * found, set the BLOCK_CACHED flag in pkt.
     */
    bool isCachedAbove(PacketPtr pkt, bool is_timing = true) const;

    /**
     * Return whether there are any outstanding misses.
     */
    bool outstandingMisses() const
    {
        return !mshrQueue.isEmpty();
    }

    CacheBlk *findBlock(Addr addr, bool is_secure) const {
        return tags->findBlock(addr, is_secure);
    }
    /* MJL_Begin */
    CacheBlk *MJL_findBlock(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure) const {
        return tags->MJL_findBlock(addr, MJL_cacheBlkDir, is_secure);
    }
    /* MJL_End */

    // MJL_TODO: seems inCache() and inMissQueue() are only used by the prefetcher, would need to change if we use prefetchers
    bool inCache(Addr addr, bool is_secure) const override {
        return (tags->findBlock(addr, is_secure) != 0);
    }

    bool inMissQueue(Addr addr, bool is_secure) const override {
        return (mshrQueue.findMatch(addr, is_secure) != 0);
    }

    /**
     * Find next request ready time from among possible sources.
     */
    Tick nextQueueReadyTime() const;

  public:
    /** Instantiates a basic cache object. */
    Cache(const CacheParams *p);

    /** Non-default destructor is needed to deallocate memory. */
    virtual ~Cache();

    void regStats() override;

    /**
     * Take an MSHR, turn it into a suitable downstream packet, and
     * send it out. This construct allows a queue entry to choose a suitable
     * approach based on its type.
     *
     * @param mshr The MSHR to turn into a packet and send
     * @return True if the port is waiting for a retry
     */
    bool sendMSHRQueuePacket(MSHR* mshr);

    /**
     * Similar to sendMSHR, but for a write-queue entry
     * instead. Create the packet, and send it, and if successful also
     * mark the entry in service.
     *
     * @param wq_entry The write-queue entry to turn into a packet and send
     * @return True if the port is waiting for a retry
     */
    bool sendWriteQueuePacket(WriteQueueEntry* wq_entry);

    /** serialize the state of the caches
     * We currently don't support checkpointing cache state, so this panics.
     */
    void serialize(CheckpointOut &cp) const override;
    void unserialize(CheckpointIn &cp) override;
};

/**
 * Wrap a method and present it as a cache block visitor.
 *
 * For example the forEachBlk method in the tag arrays expects a
 * callable object/function as their parameter. This class wraps a
 * method in an object and presents  callable object that adheres to
 * the cache block visitor protocol.
 */
class CacheBlkVisitorWrapper : public CacheBlkVisitor
{
  public:
    typedef bool (Cache::*VisitorPtr)(CacheBlk &blk);

    CacheBlkVisitorWrapper(Cache &_cache, VisitorPtr _visitor)
        : cache(_cache), visitor(_visitor) {}

    bool operator()(CacheBlk &blk) override {
        return (cache.*visitor)(blk);
    }

  private:
    Cache &cache;
    VisitorPtr visitor;
};

/**
 * Cache block visitor that determines if there are dirty blocks in a
 * cache.
 *
 * Use with the forEachBlk method in the tag array to determine if the
 * array contains dirty blocks.
 */
class CacheBlkIsDirtyVisitor : public CacheBlkVisitor
{
  public:
    CacheBlkIsDirtyVisitor()
        : _isDirty(false) {}

    bool operator()(CacheBlk &blk) override {
        if (blk.isDirty()) {
            _isDirty = true;
            return false;
        } else {
            return true;
        }
    }

    /**
     * Does the array contain a dirty line?
     *
     * \return true if yes, false otherwise.
     */
    bool isDirty() const { return _isDirty; };

  private:
    bool _isDirty;
};

#endif // __MEM_CACHE_CACHE_HH__
