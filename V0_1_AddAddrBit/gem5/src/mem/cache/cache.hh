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
        /**
         * Override the default sendTimingResp() to merge split packets at L1D$ cpu_side port
         */
        virtual bool sendTimingResp(PacketPtr pkt)
        {
            assert(pkt->isResponse());
            if (this->name().find("dcache") != std::string::npos) {
                /* MJL_Test: Packet information output */
                std::cout << this->name() << "::sendTimingResp";
                std::cout << ": PC(hex) = ";
                if (pkt->req->hasPC()) {
                    std::cout << std::hex << pkt->req->getPC() << std::dec;
                } else {
                    std::cout << "noPC";
                }
                std::cout << ", MemCmd = " << pkt->cmd.toString();
                std::cout << ", CmdDir = " << pkt->MJL_getCmdDir();
                std::cout << ", Addr(oct) = " << std::oct << pkt->getAddr() << std::dec;
                std::cout << ", Size = " << pkt->getSize();
                std::cout << ", Data(hex) = ";
                if (pkt->hasData()) {
                    uint64_t MJL_data = 0;
                    std::memcpy(&MJL_data, pkt->getConstPtr<uint8_t>(), pkt->getSize());
                    std::cout << "word[0] " << std::hex << MJL_data << std::dec;
                    for (unsigned i = sizeof(uint64_t); i < pkt->getSize(); i = i + sizeof(uint64_t)) {
                        MJL_data = 0;
                        std::memcpy(&MJL_data, pkt->getConstPtr<uint8_t>() + i, std::min(sizeof(uint64_t), pkt->getSize() - (Addr)i));
                        std::cout << " | word[" << i/sizeof(uint64_t) << "] " << std::hex <<  MJL_data << std::dec;
                    }       
                } else {
                    std::cout << ", noData";
                }
                std::cout << std::dec;
                std::cout << ", Time = " << pkt->req->time() ;
                std::cout << std::endl;
                /* */

                bool MJL_isUnaligned = false;
                bool MJL_isMerged = false;

                // Check whether the packet is part of a pair of split packets and handle split packets cases
                auto unaligned_PC_it = cache->MJL_unalignedPacketList.find(pkt->req->getPC());
                if (unaligned_PC_it != cache->MJL_unalignedPacketList.end()) {
                    auto unaligned_time_it = unaligned_PC_it->second.find(pkt->req->time());
                    if (unaligned_time_it != unaligned_PC_it->second.end()) {
                        auto unaligned_seq_it = unaligned_time_it->second.find(pkt->MJL_testSeq);
                        if (unaligned_seq_it != unaligned_time_it->second.end()) {

                            // The packet received was split
                            MJL_isUnaligned = true;
                            std::cout << "MJL_Merge: received a packet that was split into 2, ";

                            // Determine whether the response is to the original packet or the created second packet
                            if (pkt == std::get<0>(unaligned_seq_it->second)) {
                                std::cout << " this is the first packet\n";
                                // Register that the response to the original packet has been received
                                cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq][0] = true;
                            } else if (pkt == std::get<1>(unaligned_seq_it->second)) {
                                std::cout << " this is the second packet\n";
                                // Register that the response to the second packet has been received
                                cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq][1] = true;
                            } else {
                                // Should not have other cases
                                assert((pkt == std::get<0>(unaligned_seq_it->second)) || (pkt == std::get<1>(unaligned_seq_it->second)));
                            }

                            // Merge if both packets have been received
                            if (cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq][0] && cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq][1]) {
                                // Both packets received
                                MJL_isMerged = true;
                                std::cout << "MJL_Merge: Received both packets that were previously split, ";
                                PacketPtr MJL_origPacket = std::get<0>(unaligned_seq_it->second);
                                std::cout << ", got Original Packet, size = " << MJL_origPacket->getSize();
                                PacketPtr MJL_sndPacket = std::get<1>(unaligned_seq_it->second);
                                std::cout << ", got Second Packet, size = " << MJL_sndPacket->getSize();
                                
                                // If there is data in the response packets, merge data from both packets to the original packet
                                if (pkt->hasData()) {
                                    unsigned MJL_byteOffset = MJL_origPacket->getAddr() & (Addr)(sizeof(uint64_t) - 1);
                                    std::memcpy(MJL_origPacket->getPtr<uint8_t>() + sizeof(uint64_t) - MJL_byteOffset, MJL_sndPacket->getConstPtr<uint8_t>(), MJL_sndPacket->getSize());
                                    std::cout << ", copying data from snd to orig";
                                // Reset the request and packet size back to original size for write responses
                                } else if (pkt->isWrite()) {
                                    MJL_origPacket->MJL_setSize(MJL_origPacket->getSize() + MJL_sndPacket->getSize());
                                    MJL_origPacket->req->MJL_setSize(MJL_origPacket->getSize());
                                }

                                // If the packet arrived second is not the original packet
                                if (pkt != MJL_origPacket) {
                                    // Copy timing information from the packet to the original packet
                                    MJL_origPacket->headerDelay = pkt->headerDelay;
                                    MJL_origPacket->snoopDelay = pkt->snoopDelay;
                                    MJL_origPacket->payloadDelay = pkt->payloadDelay;
                                    // And set the packet to be passed on to be the original one
                                    pkt = MJL_origPacket;
                                }

                                // Delete the now useless second packet and split packets entry
                                delete MJL_sndPacket;
                                unaligned_time_it->second.erase(pkt->MJL_testSeq);             
                            }
                        }
                    }
                }

                // Send response to core if the packet was not split, or the split packets has merged
                if (!MJL_isUnaligned || (MJL_isUnaligned && MJL_isMerged)) {
                    // Check if it is a column vector response
                    if (cache->MJL_colVecHandler.handleResponse(pkt, true)) {
                        return CacheSlavePort::sendTimingResp(pkt);
                    }
                    return CacheSlavePort::sendTimingResp(pkt);
                // Wait for both of the split packets to be received and merged before sending response
                } else {
                    return true;
                }
            }

            // Forward response to upper level
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
     * The file format should be the "PC Dir" with each line defining the direction for different instruction
     * Note that the PC number should be in hexadecimal format, and the Dir is either "R" for row or "C" for column
     * If the instruction is part of a vector column access, then the position of the instruction and the other part of the vector access instruction should also be provided.
     * The position 0 means that it is not a vector column access, 1 means that it is the first instruction, 2 means that it is the second instruction.
     * The other intstruction's PC can be set to 0 for non column vector insturctions.
     */
    void MJL_readPC2DirMap () {
        std::ifstream MJL_PC2DirFile;
        std::string line;
        Addr tempPC;
        char tempDir;
        int tempPos;
        Addr tempOtherPC;

        MJL_PC2DirFile.open(MJL_PC2DirFilename);
        if (MJL_PC2DirFile.is_open()) {
            std::cout << this->name() << "::Reading PC to direction preference input from " << MJL_PC2DirFilename << ":" << std::endl;
            while (getline(MJL_PC2DirFile, line)) {
                std::stringstream(line) >> std::hex >> tempPC >> std::dec >> tempDir >> tempPos >> std::hex >> tempOtherPC >> std::dec;
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
                if (tempPos > 0) {
                    assert(tempPos == 1 || tempPos == 2);
                    assert( MJL_colVecHandler.MJL_ColVecList.find(tempOtherPC) == MJL_colVecHandler.MJL_ColVecList.end() || MJL_colVecHandler.MJL_ColVecList.at(tempOtherPC).pos != tempPos-1);
                    MJL_colVecHandler.MJL_ColVecList.emplace(tempPC, MJL_ColVecHandler::MJL_VecInfo(tempOtherPC, tempPos-1));
                }
                /* MJL_Test: file information output */
                if (MJL_PC2DirMap.find(tempPC) != MJL_PC2DirMap.end()) {
                    std::cout << "PC: " << std::hex << MJL_PC2DirMap.find(tempPC)->first << std::dec << ", Dir: " << MJL_PC2DirMap.find(tempPC)->second << ", ColumnVec: " << tempPos << ", OtherPC: " << std::hex << tempOtherPC << std::dec << "\n";
                }
                /* */
            }
        }
        else {
            std::cout << "MJL_Error: Could not open input file!\n";
            assert(MJL_PC2DirFile.is_open());
        }
        MJL_PC2DirFile.close();
    }

    /**
     * Track the pointers to a pair of split packets.
     */
    std::map< Addr, std::map< Tick, std::map< int , std::tuple<PacketPtr, PacketPtr> > > > MJL_unalignedPacketList;  //[PC][_time][MJL_testSeq] = <OrigPtr, SecPtr>
    /**
     * Register whether both response packets of a pair of split packets has been received.
     */
    std::map< Addr, std::map< Tick, std::map< int , std::map< int, bool > > > > MJL_unalignedPacketCount;  //[PC][_time][MJL_testSeq][PacketAddrSeq] = received
    /**
     * Track whether there is a second half of split packets waiting to be sent
     */
    bool MJL_sndPacketWaiting;
    /**
     * Pointer to the second half of split packets that is waiting
     */
    PacketPtr MJL_retrySndPacket;
    /**
     * The class that handles column vector access merging
     */
    class MJL_ColVecHandler {
        public:

        Cache* cache;

        /**
         * The class that holds the column access that is waiting for the other part of the vector access
         */
        class MJL_VecWaiting {
            public:
                // For read, the packet that came second and the corresponding data from the first response packet are recorded
                // For write, the packet that came first and the data from the second packet are recorded here.
                PacketPtr pktWaiting;
                uint8_t* dataWaiting;
                MJL_VecWaiting(): pktWaiting(nullptr), dataWaiting(nullptr) { }
                ~MJL_VecWaiting() {delete dataWaiting;}
        };

        std::map< Addr, std::map< Addr, std::list<MJL_VecWaiting*> > > MJL_VecPktWaitingList; //[PC][Addr] = MJL_VecPktWaiting

        class MJL_VecInfo {
            public:
                Addr otherPC;
                int pos;
                MJL_VecInfo(Addr in_otherPC, int in_pos): otherPC(in_otherPC), pos(in_pos) {}
        };
        std::map< Addr, MJL_VecInfo> MJL_ColVecList; //[PC][{PC of the other instruction that makes the vector access, pos}], pos=0 is the first word, pos=1 is the second.

        /**
         * Determine whether the packet should be send to the L1 data cache.
         * Also modifies the packet sent to vector packets.
         * @return if the packet should be send to cache or not 
         */
        bool isSend(PacketPtr pkt, bool isTiming) {
            bool shouldSend = true;
            bool satisfyPkt = false;

            // Try to find an entry matching the packet in the waiting list
            auto PC_it = MJL_VecPktWaitingList.find(pkt->req->getPC());
            // If the entry is found
            if (PC_it != MJL_VecPktWaitingList.end() && PC_it->second.find(pkt->getAddr()) != PC_it->second.end()) {
                auto Addr_it = PC_it->second.find(pkt->getAddr());
                auto vec_it = Addr_it->second.end();
                // And the request is a read
                if (pkt->isRead()) {
                    // An earlier request should have been sent to get the data for this request as well
                    // So we do not send this packet to the cache
                    shouldSend = false;
                    // Should not have duplicate requests for this... but it happened... so I'm using a list...
                    for (vec_it = Addr_it->second.begin(); vec_it != Addr_it->second.end(); ++vec_it) {
                        if ((*vec_it)->pktWaiting != nullptr) continue;
                        // Record this packet in the entry
                        (*vec_it)->pktWaiting = pkt;
                        // And just satisfy the packet is the data is already available
                        if ((*vec_it)->dataWaiting != nullptr) {
                            satisfyPkt = satisfy_and_respond(*vec_it, isTiming);
                            assert(satisfyPkt);
                        }
                        break;
                    }
                // If the request is a write
                } else if (pkt->isWrite()) {
                    for (vec_it = Addr_it->second.begin(); vec_it != Addr_it->second.end(); ++vec_it) {
                        // We should have the earlier packet available
                        assert((*vec_it)->pktWaiting != nullptr);
                        // Since we are using a list for duplicate case, the unused entry should not have a dataWaiting allocated
                        if ((*vec_it)->dataWaiting != nullptr) continue;
                        // Keep a copy of the data in the packet
                        int write_size = pkt->getSize();
                        (*vec_it)->dataWaiting = new uint8_t[write_size];
                        pkt->writeData((*vec_it)->dataWaiting);
                        // Modify the pkt to form a vector access
                        updatePkt(pkt);
                        // Reset data for vector access
                        pkt->deleteData();
                        pkt->allocate();
                        std::memcpy(pkt->getPtr<uint8_t>() + MJL_ColVecList.at(pkt->req->getPC()).pos * write_size, (*vec_it)->dataWaiting, write_size);
                        std::memcpy(pkt->getPtr<uint8_t>() + MJL_ColVecList.at((*vec_it)->pktWaiting->req->getPC()).pos * write_size, (*vec_it)->pktWaiting->getConstPtr<uint8_t>(), write_size);
                        break;
                    }
                }
                assert(vec_it != Addr_it->second.end());
                // If we have already done with the vector access
                if (satisfyPkt) {
                    // Erase the entry from waiting list
                    auto Addr_it = PC_it->second.find(pkt->getAddr());
                    Addr_it->second.erase(vec_it);
                    if (Addr_it->second.empty()) {
                        PC_it->second.erase(Addr_it);
                        if (PC_it->second.empty()) {
                            MJL_VecPktWaitingList.erase(PC_it);
                        }
                    }
                }

            // If the entry is not found
            } else {
                // Add the entry if it is a column vector memory access
                shouldSend = addNewVecWaiting(pkt);
            }

            return shouldSend;
        }

        /**
         * Satisfy the request of the packet waiting.
         * Make and schedule response when all information are present.
         * @return whether the request has been satisfied. Should always be true on correct use.
         */
        bool satisfy_and_respond(MJL_VecWaiting* vecWaiting, bool isTiming) {
            // The VecWaiting should have both a packet and data to be able to satisfy and respond
            assert(vecWaiting->pktWaiting != nullptr);
            assert(vecWaiting->dataWaiting != nullptr);
                
            PacketPtr pkt = vecWaiting->pktWaiting;
            // Copy the data to packet if it is a read
            if (pkt->isRead()) {
                pkt->setData(vecWaiting->dataWaiting + MJL_ColVecList.at(pkt->req->getPC()).pos * pkt->getSize());
            } else if (!pkt->isWrite()) {
                return false;
            }
            
            if (isTiming) {
                // Make response and schedule a response for the packet for as soon as possible
                pkt->makeTimingResponse();
                pkt->headerDelay = pkt->payloadDelay = 0;
                cache->cpuSidePort->schedTimingResp(pkt, curTick(), true);
            }
            
            return true;
        }

        /**
         * Add a new vecWaiting entry and populate it with available information.
         * @return whether the packet should be send to cache
         */
        bool addNewVecWaiting(PacketPtr pkt) {
            bool shouldSend = false;
            // Should send if the packet received first is a read
            if (pkt->isRead()) {
                shouldSend = true;
                // Create a new entry with the other packet's PC and access address
                Addr otherPC =  MJL_ColVecList.at(pkt->req->getPC()).otherPC;
                Addr otherAddr = pkt->getAddr();
                // If the other packet is in the first position, then it's address should be (pkt->getAddr() - pkt->getSize()) with respect to the direction preference
                if (MJL_ColVecList.at(otherPC).pos == 0) {
                    otherAddr = cache->MJL_subOffsetAddr(otherAddr, pkt->MJL_getCmdDir(), pkt->getSize());
                // Otherwise, it's (pkt->getAddr() + pkt->getSize())
                } else {
                    otherAddr = cache->MJL_addOffsetAddr(otherAddr, pkt->MJL_getCmdDir(), pkt->getSize());
                }
                MJL_VecPktWaitingList[otherPC][otherAddr].push_back(new MJL_VecWaiting());
                // Modify the packet for vector accesses
                updatePkt(pkt);
            // The packet for write is saved and not sent
            } else if (pkt->isWrite()) {
                // Create a new entry with the other packet's PC and access address
                Addr otherPC =  MJL_ColVecList.at(pkt->req->getPC()).otherPC;
                Addr otherAddr = pkt->getAddr();
                // If the other packet is in the first position, then it's address should be (pkt->getAddr() - pkt->getSize()) with respect to the direction preference
                if (MJL_ColVecList.at(otherPC).pos == 0) {
                    otherAddr = cache->MJL_subOffsetAddr(otherAddr, pkt->MJL_getCmdDir(), pkt->getSize());
                // Otherwise, it's (pkt->getAddr() + pkt->getSize())
                } else {
                    otherAddr = cache->MJL_addOffsetAddr(otherAddr, pkt->MJL_getCmdDir(), pkt->getSize());
                }
                MJL_VecPktWaitingList[otherPC][otherAddr].push_back(new MJL_VecWaiting());
                // Add self to the new entry's pktWaiting
                MJL_VecPktWaitingList[otherPC][otherAddr].back()->pktWaiting = pkt;
            }

            return shouldSend;
        }

        /**
         * Handle column vector response packets. 
         * Satisfy waiting packets and modify packets back to original state.
         * @return whether the packet is a column vector response packet
         */
        bool handleResponse(PacketPtr pkt, bool isTiming) {
            bool isVec = false;
            // See if this packet is a column vector access response
            assert(pkt->isResponse());
            if (pkt->req->MJL_isVec() && pkt->MJL_cmdIsColumn()) {
                isVec = true;
                // If this packet is a read response
                if (pkt->isRead()) {
                    // Then the PC and the address of the other packet is used as index to the vecWaiting entry
                    Addr otherPC =  MJL_ColVecList.at(pkt->req->getPC()).otherPC;
                    Addr otherAddr = pkt->getAddr();
                    // If the other packet is in the first position, then it's address should be pkt->getAddr() (we changed the address to be this)
                    // If the other packet is in the second position, then it's address should be (pkt->getAddr() + pkt->getSize()/2) (the size was changed to be double) with respect to preference direction
                    if (MJL_ColVecList.at(otherPC).pos != 0) {
                        otherAddr = cache->MJL_addOffsetAddr(otherAddr, pkt->MJL_getCmdDir(), pkt->getSize()/2);
                    }
                    // This entry should exist
                    auto PC_it = MJL_VecPktWaitingList.find(otherPC);
                    assert(PC_it != MJL_VecPktWaitingList.end() && PC_it->second.find(otherAddr) != PC_it->second.end());
                    // Copy data from the packet to dataWaiting
                    uint8_t *data = new uint8_t[pkt->getSize()];
                    MJL_VecPktWaitingList[otherPC][otherAddr].front()->dataWaiting = data;
                    pkt->writeData(data);
                    // Reset pkt to original form and get the correct portion of the data
                    resetPkt(pkt);
                    pkt->deleteData();
                    pkt->allocate();
                    int write_size = pkt->getSize();
                    std::memcpy(pkt->getPtr<uint8_t>() + MJL_ColVecList.at(pkt->req->getPC()).pos * write_size, data, write_size);
                    // Satisfy the other packet if it is waiting 
                    if (MJL_VecPktWaitingList[otherPC][otherAddr].front()->pktWaiting != nullptr) {
                        bool satisfy = false;
                        satisfy = satisfy_and_respond(MJL_VecPktWaitingList[otherPC][otherAddr].front(), isTiming);
                        assert(satisfy);
                        // Delete the entry after it has served it's purpose 
                        MJL_VecPktWaitingList[otherPC][otherAddr].pop_front();
                        if (MJL_VecPktWaitingList[otherPC][otherAddr].empty()) {
                            MJL_VecPktWaitingList[otherPC].erase(otherAddr);
                            if (MJL_VecPktWaitingList[otherPC].empty()) {
                                MJL_VecPktWaitingList.erase(otherPC);
                            }
                        }
                    }
                // Else if it is a write response
                } else if (pkt->isWrite()) {
                    resetPkt(pkt);
                    // Then the PC and Addr of this packet has been used for vecWaiting index
                    Addr thisPC = pkt->req->getPC();
                    Addr thisAddr = pkt->getAddr();
                    // This entry should exist
                    auto PC_it = MJL_VecPktWaitingList.find(thisPC);
                    assert(PC_it != MJL_VecPktWaitingList.end() && PC_it->second.find(thisAddr) != PC_it->second.end());
                    // And a packet should be present in the entry
                    assert(MJL_VecPktWaitingList[thisPC][thisAddr].front()->pktWaiting != nullptr);
                    // The write for both packets has been satisfied, respond and delete the entry
                    bool satisfy = false;
                    satisfy = satisfy_and_respond(MJL_VecPktWaitingList[thisPC][thisAddr].front(), isTiming);
                    assert(satisfy);
                    MJL_VecPktWaitingList[thisPC][thisAddr].pop_front();
                    if (MJL_VecPktWaitingList[thisPC][thisAddr].empty()) {
                        MJL_VecPktWaitingList[thisPC].erase(thisAddr);
                        if (MJL_VecPktWaitingList[thisPC].empty()) {
                            MJL_VecPktWaitingList.erase(thisPC);
                        }
                    }
                }
            }

            return isVec;
        }

        /**
         * Modify packet information to make column vector access packets
         */
        void updatePkt(PacketPtr pkt) {
            pkt->req->MJL_setVec();
            // Double the request's size (Assuming that both accesses always has the same size)
            pkt->req->MJL_setSize(pkt->getSize() + pkt->getSize()); 
            pkt->MJL_setSize(pkt->getSize() + pkt->getSize());
            if (MJL_ColVecList.at(pkt->req->getPC()).pos != 0) {
                pkt->req->setPaddr(cache->MJL_subOffsetAddr(pkt->getAddr(), pkt->MJL_getCmdDir(), pkt->getSize()/2));
                pkt->setAddr(pkt->req->getPaddr());
            }
        }

        /**
         * Reset packet information to original state from column vector access packets
         */
        void resetPkt(PacketPtr pkt) {
            // Double the request's size (Assuming that both accesses always has the same size)
            pkt->req->MJL_setSize(pkt->getSize()/2); 
            pkt->MJL_setSize(pkt->getSize()/2);
            if (MJL_ColVecList.at(pkt->req->getPC()).pos != 0) {
                pkt->req->setPaddr(cache->MJL_addOffsetAddr(pkt->getAddr(), pkt->MJL_getCmdDir(), pkt->getSize()));
                pkt->setAddr(pkt->req->getPaddr());
            }
        }

    };

    MJL_ColVecHandler MJL_colVecHandler;
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
     * Invalidate blocks that are cross direction of the addresses written
     * MJL_written_addr: starting address of the words written
     * MJL_cacheBlkDir: direction of the block written
     * size: size of bytes written, can be used to determine how many words are written and their address
     * is_secure: used in MJL_findBlk
     */
    void MJL_invalidateOtherBlocks(Addr MJL_written_addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, unsigned size, bool is_secure, PacketList& writebacks, bool MJL_wordDirty[8]) {
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
            if (MJL_wordDirty[offset/sizeof(uint64_t)]) {
                MJL_writtenWord_addr = MJL_addOffsetAddr(MJL_written_addr, MJL_cacheBlkDir, offset);
                MJL_diffDir_blk = tags->MJL_findBlock(MJL_writtenWord_addr, MJL_diffDir, is_secure);
                if ((MJL_diffDir_blk != nullptr) && MJL_diffDir_blk->isValid()) {
                    // MJL_TODO: should check if there's an upgrade miss waiting on this I guess?
                    if (MJL_diffDir_blk->isDirty()) {
                        writebacks.push_back(writebackBlk(MJL_diffDir_blk));
                    } else {
                        writebacks.push_back(cleanEvictBlk(MJL_diffDir_blk));
                    }
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
    /* MJL_Begin */
    /**
     * Create a writeback request for the given block without eviction.
     * @param blk The block to writeback.
     * @return The writeback request for the block.
     */
    PacketPtr MJL_writebackCachedBlk(CacheBlk *blk);
    /* MJL_End */

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
