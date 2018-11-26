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
#include "base/random.hh"
#include <fstream>
#include <sstream>

extern std::map< Addr, uint8_t[8] > MJL_current_data;
extern std::map< Addr, bool[8] > MJL_current_valid;
extern std::map< PacketPtr, uint8_t[16] > MJL_return_data;
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
        // Test use methods
        // std::map< Addr, uint8_t[8] > MJL_current_data;
        // std::map< Addr, bool[8] > MJL_current_valid;
        // std::map< PacketPtr, uint8_t[16] > MJL_return_data;
        bool MJL_value_test;
        void MJL_regReq(PacketPtr pkt) {
            if (!MJL_value_test || pkt->cmd.isSWPrefetch()) {
                return;
            }
            // assert(pkt->getSize() <= sizeof(uint64_t));
            Addr base_addr = pkt->getAddr();
            unsigned offset = pkt->getAddr() % sizeof(uint64_t);
            assert(pkt->getSize() <= 2*sizeof(uint64_t));
            if (pkt->isWrite()) {
                if (MJL_current_data.find(base_addr/sizeof(uint64_t)) == MJL_current_data.end()) {
                    std::memset(&MJL_current_data[base_addr/sizeof(uint64_t)], 0, sizeof(uint64_t));
                    for (int i = 0; i < 8; ++i) {
                        MJL_current_valid[base_addr/sizeof(uint64_t)][i] = false;
                    }
                }
                std::memcpy(&((MJL_current_data[base_addr/sizeof(uint64_t)])[offset]), pkt->getConstPtr<uint8_t>(), std::min((Addr)pkt->getSize(), sizeof(uint64_t) - offset));
                for (int i = 0; i < std::min((Addr)pkt->getSize(), sizeof(uint64_t) - offset); ++i) {
                    MJL_current_valid[base_addr/sizeof(uint64_t)][i + offset] |= true;
                }
                if (offset + pkt->getSize() > sizeof(uint64_t)) {
                    if (MJL_current_data.find(base_addr/sizeof(uint64_t) + 1) == MJL_current_data.end()) {
                        std::memset(&MJL_current_data[base_addr/sizeof(uint64_t) + 1], 0, sizeof(uint64_t));
                        for (int i = 0; i < 8; ++i) {
                            MJL_current_valid[base_addr/sizeof(uint64_t) + 1][i] = false;
                        }
                    }
                    std::memcpy(&((MJL_current_data[base_addr/sizeof(uint64_t) + 1])[0]), pkt->getConstPtr<uint8_t>() + sizeof(uint64_t) - offset, std::min((Addr)pkt->getSize() + offset - sizeof(uint64_t), sizeof(uint64_t)));
                    for (int i = 0; i < std::min((Addr)pkt->getSize() + offset - sizeof(uint64_t), sizeof(uint64_t)); ++i) {
                        MJL_current_valid[base_addr/sizeof(uint64_t) + 1][i] |= true;
                    }
                }
                if (offset + pkt->getSize() > 2*sizeof(uint64_t)) {
                    if (MJL_current_data.find(base_addr/sizeof(uint64_t) + 2) == MJL_current_data.end()) {
                        std::memset(&MJL_current_data[base_addr/sizeof(uint64_t) + 2], 0, sizeof(uint64_t));
                        for (int i = 0; i < 8; ++i) {
                            MJL_current_valid[base_addr/sizeof(uint64_t) + 2][i] = false;
                        }
                    }
                    std::memcpy(&((MJL_current_data[base_addr/sizeof(uint64_t) + 2])[0]), pkt->getConstPtr<uint8_t>() + 2*sizeof(uint64_t) - offset, pkt->getSize() + offset - 2*sizeof(uint64_t));
                    for (int i = 0; i < pkt->getSize() + offset - sizeof(uint64_t); ++i) {
                        MJL_current_valid[base_addr/sizeof(uint64_t) + 2][i] |= true;
                    }
                }
            } else if (pkt->isRead()) {
                if (MJL_current_data.find(base_addr/sizeof(uint64_t)) != MJL_current_data.end()) {
                    bool valid = true;
                    for (int i = 0; i < std::min((Addr)pkt->getSize(), sizeof(uint64_t) - offset); ++i) {
                        valid &= MJL_current_valid[base_addr/sizeof(uint64_t)][i + offset];
                    }
                    if (offset + pkt->getSize() > sizeof(uint64_t)) {
                        if (MJL_current_data.find(base_addr/sizeof(uint64_t) + 1) == MJL_current_data.end()) {
                            valid = false;
                        }
                        for (int i = 0; i < std::min((Addr)pkt->getSize() + offset - sizeof(uint64_t), sizeof(uint64_t)); ++i) {
                            valid &= MJL_current_valid[base_addr/sizeof(uint64_t) + 1][i];
                        }
                    }
                    if (offset + pkt->getSize() > 2*sizeof(uint64_t)) {
                        if (MJL_current_data.find(base_addr/sizeof(uint64_t) + 2) == MJL_current_data.end()) {
                            valid = false;
                        }
                        for (int i = 0; i < pkt->getSize() + offset - 2*sizeof(uint64_t); ++i) {
                            valid &= MJL_current_valid[base_addr/sizeof(uint64_t) + 2][i];
                        }
                    }
                    if (valid) {
                        std::memset(&MJL_return_data[pkt], 0, 2*sizeof(uint64_t));
                        std::memcpy(&MJL_return_data[pkt], &((MJL_current_data[base_addr/sizeof(uint64_t)])[offset]), std::min((Addr)pkt->getSize(), sizeof(uint64_t) - offset));
                        if (offset + pkt->getSize() > sizeof(uint64_t)) {
                            std::memcpy(&((MJL_return_data[pkt])[sizeof(uint64_t) - offset]), &((MJL_current_data[base_addr/sizeof(uint64_t) + 1])[0]), std::min(pkt->getSize() + offset - sizeof(uint64_t), sizeof(uint64_t)));
                        }
                        if (offset + pkt->getSize() > 2*sizeof(uint64_t)) {
                            std::memcpy(&((MJL_return_data[pkt])[2*sizeof(uint64_t) - offset]), &((MJL_current_data[base_addr/sizeof(uint64_t) + 2])[0]), pkt->getSize() + offset - 2*sizeof(uint64_t));
                        }
                    }
                }
            }
        }

        void MJL_checkResp(PacketPtr pkt) {
            if (!MJL_value_test || pkt->cmd.isSWPrefetch()) {
                return;
            }
            if (MJL_return_data.find(pkt) != MJL_return_data.end()) {
                // assert(pkt->getSize() <= sizeof(uint64_t));
                assert(pkt->getSize() <= 2*sizeof(uint64_t));
                uint64_t MJL_pkt_data0 = 0;
                uint64_t MJL_pkt_data1 = 0;
                std::memcpy(&MJL_pkt_data0, pkt->getConstPtr<uint8_t>(), std::min(sizeof(uint64_t), (Addr)pkt->getSize()));
                if (pkt->getSize() > sizeof(uint64_t)) {
                    std::memcpy(&MJL_pkt_data1, pkt->getConstPtr<uint8_t>() + sizeof(uint64_t), pkt->getSize() - sizeof(uint64_t));
                }
                uint64_t MJL_reg_data0 = 0;
                uint64_t MJL_reg_data1 = 0;
                std::memcpy(&MJL_reg_data0, &(MJL_return_data[pkt]), std::min(sizeof(uint64_t), (Addr)pkt->getSize()));
                if (pkt->getSize() > sizeof(uint64_t)) {
                    std::memcpy(&MJL_reg_data1, &((MJL_return_data[pkt])[sizeof(uint64_t)]), pkt->getSize() - sizeof(uint64_t));
                }
                if (MJL_pkt_data0 != MJL_reg_data0 || MJL_pkt_data1 != MJL_reg_data1) {
                    std::clog << this->name() << "::MJL_Debug wrong read response ";
                    if (pkt->req->hasPC()) {
                        std::clog << std::hex << pkt->req->getPC() << std::dec;
                    }
                    std::clog << ": " << pkt->print() << ", " << std::hex << MJL_pkt_data0 << ":" << MJL_pkt_data1 << std::dec << ", should be " << std::hex << MJL_reg_data0 << ":" << MJL_reg_data1 << std::dec << std::endl;
                    // Addr base_addr = pkt->getAddr();
                    // unsigned offset = pkt->getAddr() % sizeof(uint64_t);
                    // std::memcpy(&((MJL_current_data[base_addr/sizeof(uint64_t)])[offset]), pkt->getConstPtr<uint8_t>(), std::min((Addr)pkt->getSize(), sizeof(uint64_t) - offset));
                    // if (offset + pkt->getSize() > sizeof(uint64_t)) {
                        // std::memcpy(&((MJL_current_data[base_addr/sizeof(uint64_t) + 1])[0]), pkt->getConstPtr<uint8_t>() + sizeof(uint64_t) - offset, pkt->getSize() + offset - sizeof(uint64_t));
                    // }
                }
                // assert(MJL_pkt_data0 == MJL_reg_data0 && MJL_pkt_data1 == MJL_reg_data1);
                MJL_return_data.erase(pkt);
            }
        }

        void MJL_functionalUpdate(PacketPtr pkt) {
            if (!MJL_value_test || pkt->cmd.isSWPrefetch()) {
                return;
            }
            if (pkt->isWrite()) {
                Addr base_addr = pkt->getAddr();
                unsigned offset = pkt->getAddr() % sizeof(uint64_t);
                if (MJL_current_data.find(base_addr/sizeof(uint64_t)) != MJL_current_data.end()) {
                    for (int i = offset; i < std::min(sizeof(uint64_t), (Addr)pkt->getSize() - offset); ++i) {
                        if (MJL_current_valid[base_addr/sizeof(uint64_t)][i]) {
                            std::memcpy(&((MJL_current_data[base_addr/sizeof(uint64_t)])[i]), pkt->getConstPtr<uint8_t>() + i, 1);
                        }
                    }
                }
                for (Addr addr = base_addr + sizeof(uint64_t) - offset; addr < base_addr + pkt->getSize(); addr += sizeof(uint64_t)) {
                    if (MJL_current_data.find(addr/sizeof(uint64_t)) != MJL_current_data.end()) {
                        for (int i = 0; i < std::min(sizeof(uint64_t), (Addr)pkt->getSize() + base_addr - addr); ++i) {
                            if (MJL_current_valid[addr/sizeof(uint64_t)][i]) {
                                std::memcpy(&((MJL_current_data[addr/sizeof(uint64_t)])[i]), pkt->getConstPtr<uint8_t>() + (addr - base_addr + i), 1);
                            }
                        }
                    }
                }
            } else {
                return;
            }
        }
        /**
         * Override the default sendTimingResp() to merge split packets at L1D$ cpu_side port
         */
        virtual bool sendTimingResp(PacketPtr pkt)
        {
            assert(pkt->isResponse());

            if (this->name().find("dcache") != std::string::npos){
                MJL_checkResp(pkt);
            } 
            /* MJL_Test */
            // if (this->name().find("dcache") != std::string::npos && pkt->req->hasPC() && pkt->req->getPC() == 0x43296c && pkt->getAddr() == 0x38c8c0 && pkt->MJL_dataIsColumn()) {
            //     MJL_debugOutFlag = false;
            // } 
            if (cache->MJL_Debug_Out && (this->name().find("dcache") != std::string::npos
                 || this->name().find("l2") != std::string::npos
                 || this->name().find("l3") != std::string::npos)
                // && (pkt->req->hasPC() && pkt->req->getPC() >= 0x407360 && pkt->req->getPC() <=0x4074ab )
                // && (pkt->getAddr() <= 0x38c8c0 && (pkt->getAddr() + pkt->getSize()) >= 0x38c8c0)
                // && MJL_debugOutFlag
                // && (pkt->getAddr() >= 0x38c000 && pkt->getAddr() < 0x390000)
                // && (pkt->getAddr() >= 0x374000 && pkt->getAddr() < 0x378000)
                // && pkt->req->hasPC() && (pkt->req->getPC() == 0x4b8526 || pkt->req->getPC() == 0x4b9e9e || pkt->req->getPC() == 0x4ba0ea || pkt->req->getPC() == 0x4ba0f4 || pkt->req->getPC() == 0x4b84db || pkt->req->getPC() == 0x4ba007)
                // && (pkt->req->hasPC() && pkt->req->getPC() >= 0x4b82d0 && pkt->req->getPC() <=0x4ba2ab )
                // && (pkt->getAddr() >= 0x8ffc000 && pkt->getAddr() < 0x9000000)
                // && (pkt->req->hasPC() && pkt->req->getPC() >= 0x44eb30 && pkt->req->getPC() <=0x44f00b )
            ) {
                std::clog << this->name() << "::MJL_Debug: sendTimingResp " << pkt->print() << std::endl; 
            }
            /* */
            /* MJL_Test: Packet information output 
            if ((this->name().find("dcache") != std::string::npos 
                  || this->name().find("l2") != std::string::npos
                  || this->name().find("l3") != std::string::npos
                )
                //&& cache->MJL_colVecHandler.MJL_ColVecList.find(pkt->req->getPC()) != cache->MJL_colVecHandler.MJL_ColVecList.end() // Debug for column vec
                //&& pkt->MJL_cmdIsColumn()
                ) { 
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
                    std::memcpy(&MJL_data, pkt->getConstPtr<uint8_t>(), std::min(sizeof(uint64_t), (Addr)pkt->getSize()));
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
                //std::cout << ", Time = " << pkt->req->time() ;
                std::cout << std::endl;
            }
             */
            if (this->name().find("dcache") != std::string::npos) {

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

        /* MJL_Begin */
        virtual bool MJL_is2DCache() {
            return cache->MJL_is2DCache();
        }
        
        virtual bool MJL_getHas2DLLC() {
            return cache->MJL_getHas2DLLC();
        }
       
        /* MJL_Test */
        bool MJL_debugOutFlag;
        /* */
        /* MJL_End */

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

        /* MJL_Begin */
        virtual bool sendTimingReq(PacketPtr pkt) {
            // if (this->name().find("l2") != std::string::npos || this->name().find("l3") != std::string::npos) {
            //     std::cout << "MJL_pfDebug: sendTimingReq " << pkt->print() << std::endl;
            // }
            return CacheMasterPort::sendTimingReq(pkt);
        }
        virtual bool MJL_is2DCache() {
            return cache->MJL_is2DCache();
        }
        virtual bool MJL_getHas2DLLC() {
            return cache->MJL_getHas2DLLC();
        }
        /* MJL_End */
    };

    /** Tag and data Storage */
    BaseTags *tags;

    /** Prefetcher */
    BasePrefetcher *prefetcher;
    /* MJL_Begin */
    class MJL_DirPredictor {
      protected:

        /** The block size of the parent cache. */
        unsigned blkSize;

        const unsigned MJL_rowWidth;
        const bool MJL_mshrPredictDir;
        const bool MJL_pfBasedPredictDir;

        const int maxConf;
        const int threshConf;
        const int minConf;
        const int startConf;

        const int pcTableAssoc;
        const int pcTableSets;

        const bool useMasterId;

        struct StrideEntry
        {
            StrideEntry() : instAddr(0), lastAddr(0), isSecure(false), stride(0),
                            confidence(0), lastPredDir(MemCmd::MJL_DirAttribute::MJL_IsRow), 
                            blkHits{false, false, false, false, false, false, false, false}, 
                            crossBlkHits{false, false, false, false, false, false, false, false}, 
                            lastRowOff(0), lastColOff(0)
            { }

            Addr instAddr;
            Addr lastAddr;
            bool isSecure;
            int stride;
            int confidence;
            MemCmd::MJL_DirAttribute lastPredDir;
            bool blkHits[8];
            bool crossBlkHits[8];
            int lastRowOff;
            int lastColOff;

            MemCmd::MJL_DirAttribute MJL_getCrossLastPredDir() {
                if (lastPredDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                    return MemCmd::MJL_DirAttribute::MJL_IsColumn;
                } else if (lastPredDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                    return MemCmd::MJL_DirAttribute::MJL_IsRow;
                } else {
                    return MemCmd::MJL_DirAttribute::MJL_IsColumn;
                }
            }

        };

        class PCTable {
          public:
            PCTable(int assoc, int sets) :
                pcTableAssoc(assoc), pcTableSets(sets) {}
            StrideEntry** operator[] (int context) {
                auto it = entries.find(context);
                if (it != entries.end())
                    return it->second;

                return allocateNewContext(context);
            }

            ~PCTable()  {
                for (auto entry : entries) {
                    for (int s = 0; s < pcTableSets; s++) {
                        delete[] entry.second[s];
                    }
                    delete[] entry.second;
                }
            }
          private:
            const int pcTableAssoc;
            const int pcTableSets;
            std::map<int, StrideEntry**> entries;

            StrideEntry** allocateNewContext(int context) {
                auto res = entries.insert(std::make_pair(context,
                                        new StrideEntry*[pcTableSets]));
                auto it = res.first;
                chatty_assert(res.second, "Allocating an already created context\n");
                assert(it->first == context);

                /* MJL_Comemnt 
                DPRINTF(HWPrefetch, "Adding context %i with stride entries at %p\n",
                        context, it->second);
                 */

                StrideEntry** entry = it->second;
                for (int s = 0; s < pcTableSets; s++) {
                    entry[s] = new StrideEntry[pcTableAssoc];
                }
                return entry;
            }
        };
        PCTable pcTable;

        struct PredictMshrEntry
        {
            PredictMshrEntry(const PacketPtr pkt, const MSHR* in_mshr, unsigned blkSize) : pc(pkt->req->getPC()), blkAddr(pkt->getBlockAddr(blkSize)), crossBlkAddr(pkt->MJL_getCrossBlockAddr(blkSize)), blkDir(pkt->MJL_getCmdDir()), crossBlkDir(pkt->MJL_getCrossCmdDir()), isSecure(pkt->isSecure()), accessAddr(pkt->getAddr()), mshr(in_mshr), masterId(pkt->req->masterId()), blkHits{false, false, false, false, false, false, false, false}, crossBlkHits{false, false, false, false, false, false, false, false}, lastRowOff(pkt->MJL_getDirOffset(blkSize, MemCmd::MJL_DirAttribute::MJL_IsRow)/sizeof(uint64_t)), lastColOff(pkt->MJL_getDirOffset(blkSize, MemCmd::MJL_DirAttribute::MJL_IsColumn)/sizeof(uint64_t))
            {
                blkHits[pkt->MJL_getDirOffset(blkSize, blkDir)/sizeof(uint64_t)] |= true;
                crossBlkHits[pkt->MJL_getDirOffset(blkSize, crossBlkDir)/sizeof(uint64_t)] |= true;
            }

            Addr pc;
            Addr blkAddr;
            Addr crossBlkAddr;
            MemCmd::MJL_DirAttribute blkDir;
            MemCmd::MJL_DirAttribute crossBlkDir;
            bool isSecure;
            Addr accessAddr;
            const MSHR* mshr;
            MasterID masterId;
            bool blkHits[8];
            bool crossBlkHits[8];
            int lastRowOff;
            int lastColOff;

            /* MJL_Test 
            std::string print() {
                std::ostringstream output;
                std::string blkDir_str = (blkDir == MemCmd::MJL_DirAttribute::MJL_IsRow) ? "r" : "c";
                std::string crossBlkDir_str = (crossBlkDir == MemCmd::MJL_DirAttribute::MJL_IsRow) ? "r" : "c";
                output << "pc " << std::hex << pc << " " << std::dec; 
                output << "blk " << blkDir_str << "/" << crossBlkDir_str << ":" << std::hex << blkAddr << "/" << crossBlkAddr << " " << std::dec;
                output << "hit/crosshit " ;
                for (int i = 0; i < 8; ++i) {
                    output << blkHits[i];
                }
                output << "/";
                for (int i = 0; i < 8; ++i) {
                    output << crossBlkHits[i];
                }
                return output.str();
            }
             */
        };
        std::list<PredictMshrEntry> copyPredictMshrQueue;

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

        Addr pcHash(Addr pc) const {
            Addr hash1 = pc >> 1;
            Addr hash2 = hash1 >> floorLog2((unsigned)pcTableSets);
            return (hash1 ^ hash2) & (Addr)(pcTableSets - 1);
        }

        bool pcTableHit(Addr pc, bool is_secure, int master_id, StrideEntry* &entry) {
            int set = pcHash(pc);
            StrideEntry* set_entries = pcTable[master_id][set];
            for (int way = 0; way < pcTableAssoc; way++) {
                // Search ways for match
                if (set_entries[way].instAddr == pc &&
                    set_entries[way].isSecure == is_secure) {
                    /* MJL_Comment 
                    DPRINTF(HWPrefetch, "Lookup hit table[%d][%d].\n", set, way);
                     */
                    entry = &set_entries[way];
                    return true;
                }
            }
            return false;
        }

        StrideEntry* pcTableVictim(Addr pc, int master_id) {
            // Rand replacement for now
            int set = pcHash(pc);
            int way = random_mt.random<int>(0, pcTableAssoc - 1);

            /* MJL_Comment 
            DPRINTF(HWPrefetch, "Victimizing lookup table[%d][%d].\n", set, way);
             */
            return &pcTable[master_id][set][way];
        }

        bool observeAccess(const PacketPtr &pkt) const {
            bool fetch = pkt->req->isInstFetch();
            bool read = pkt->isRead();
            bool inv = pkt->isInvalidate();

            if (pkt->req->isUncacheable()) return false;
            if (fetch) return false;
            if (!fetch && !read && inv) return false;
            if (pkt->cmd == MemCmd::CleanEvict) return false;
            // Do not predict the direction of a vector access
            if (pkt->getSize() > sizeof(uint64_t)) return false;

            return true;
        }

        MemCmd::MJL_DirAttribute MJL_mshrPredict(Addr pkt_addr, StrideEntry* entry, int pkt_rowOff, int pkt_colOff, bool isWrite) {
            unsigned blkHitCount = 0;
            unsigned crossBlkHitCount = 0;

            int new_stride = pkt_addr - entry->lastAddr;
            bool stride_match = (new_stride == entry->stride);

            // Adjust confidence for stride entry
            if (stride_match && new_stride != 0) {
                if (entry->confidence < maxConf)
                    entry->confidence++;
            } else {
                if (entry->confidence > minConf)
                    entry->confidence--;
                // If confidence has dropped below the threshold, train new stride
                if (entry->confidence < threshConf)
                    entry->stride = new_stride;
            }

             entry->lastAddr = pkt_addr;

            // Only generation if above confidence threshold
            MemCmd::MJL_DirAttribute selfStrideDir = MemCmd::MJL_DirAttribute::MJL_IsRow;
            int selfStrideStep = 0;
            if (entry->confidence >= threshConf) {
                int colOffset = new_stride / (MJL_rowWidth * blkSize) + entry->lastColOff;
                int rowOffset = new_stride/sizeof(uint64_t) + entry->lastRowOff;
                if (new_stride % (MJL_rowWidth * blkSize) == 0 && colOffset >= 0 && colOffset < (int) (blkSize/sizeof(uint64_t)) ) {
                    selfStrideDir = MemCmd::MJL_DirAttribute::MJL_IsColumn;
                    selfStrideStep = colOffset - entry->lastColOff;
                } else if ( rowOffset >= 0 && rowOffset < (int) (blkSize/sizeof(uint64_t)) ) {
                    selfStrideDir = MemCmd::MJL_DirAttribute::MJL_IsRow;
                    selfStrideStep = rowOffset - entry->lastRowOff;
                }
            }
            
            if (selfStrideStep != 0 && stride_match) {
                if (selfStrideDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                    for (int i = entry->lastRowOff + selfStrideStep; i >= 0 && i < (int) (blkSize/sizeof(uint64_t)); i += selfStrideStep) {
                        if (entry->lastPredDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                            entry->blkHits[i] |= true;
                        } else if (entry->lastPredDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                            entry->crossBlkHits[i] |= true;
                        }
                    }
                    /*for (int i = entry->lastRowOff - selfStrideStep; i >= 0 && i < (int) (blkSize/sizeof(uint64_t)); i -= selfStrideStep) {
                        if (entry->lastPredDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                            entry->blkHits[i] |= true;
                        } else if (entry->lastPredDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                            entry->crossBlkHits[i] |= true;
                        }
                    }*/
                } else if (selfStrideDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                    for (int i = entry->lastColOff + selfStrideStep; i >= 0 && i < (int) (blkSize/sizeof(uint64_t)); i += selfStrideStep) {
                        if (entry->lastPredDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                            entry->blkHits[i] |= true;
                        } else if (entry->lastPredDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                            entry->crossBlkHits[i] |= true;
                        }
                    }
                    /*for (int i = entry->lastColOff - selfStrideStep; i >= 0 && i < (int) (blkSize/sizeof(uint64_t)); i -= selfStrideStep) {
                        if (entry->lastPredDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                            entry->blkHits[i] |= true;
                        } else if (entry->lastPredDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                            entry->crossBlkHits[i] |= true;
                        }
                    }*/
                }
            }

            for (unsigned i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                blkHitCount += entry->blkHits[i] ? 1 : 0;
                crossBlkHitCount += entry->crossBlkHits[i] ? 1 : 0;
            }
            if (crossBlkHitCount > blkHitCount) {
                entry->lastPredDir = entry->MJL_getCrossLastPredDir();
                bool tempBlkHits[8];
                for (unsigned i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                    tempBlkHits[i] = entry->blkHits[i];
                    entry->blkHits[i] = entry->crossBlkHits[i];
                    entry->crossBlkHits[i] = tempBlkHits[i];
                }
            }
            entry->lastRowOff = pkt_rowOff;
            entry->lastColOff = pkt_colOff;
            return entry->lastPredDir;
        }

        MemCmd::MJL_DirAttribute MJL_stridePredict(Addr pkt_addr, StrideEntry* entry, MemCmd::MJL_DirAttribute pkt_dir) {
            MemCmd::MJL_DirAttribute predictedDir = pkt_dir;
            int new_stride = pkt_addr - entry->lastAddr;
            bool stride_match = (new_stride == entry->stride);

            // Adjust confidence for stride entry
            if (stride_match && new_stride != 0) {
                if (entry->confidence < maxConf)
                    entry->confidence++;
            } else {
                if (entry->confidence > minConf)
                    entry->confidence--;
                // If confidence has dropped below the threshold, train new stride
                if (entry->confidence < threshConf)
                    entry->stride = new_stride;
            }

            /* MJL_Comment 
            DPRINTF(HWPrefetch, "Hit: PC %x pkt_addr %x (%s) stride %d (%s), "
                    "conf %d\n", pc, pkt_addr, is_secure ? "s" : "ns", new_stride,
                    stride_match ? "match" : "change",
                    entry->confidence);
                */

            entry->lastAddr = pkt_addr;

            // Only generation if above confidence threshold
            if (entry->confidence >= threshConf) {
                if (new_stride % (MJL_rowWidth * blkSize) == 0) {
                    predictedDir = MemCmd::MJL_DirAttribute::MJL_IsColumn;
                    /* MJL_Test 
                    std::cout << "MJL_predDebug: MJL_predictDir " << pkt->print() << " predicted column" << std::endl;
                        */
                } else {
                    predictedDir = MemCmd::MJL_DirAttribute::MJL_IsRow;
                }
            }
            entry->lastPredDir = predictedDir;
            return predictedDir;
        }

      public:
        MJL_DirPredictor(unsigned _blkSize, unsigned _MJL_rowWidth, bool _MJL_mshrPredictDir, bool _MJL_pfBasedPredictDir)
            : blkSize(_blkSize), MJL_rowWidth(_MJL_rowWidth), MJL_mshrPredictDir(_MJL_mshrPredictDir), MJL_pfBasedPredictDir(_MJL_pfBasedPredictDir), maxConf(3), 
              threshConf(2), minConf(0), startConf(2), pcTableAssoc(4), 
              pcTableSets(8), useMasterId(true), pcTable(pcTableAssoc, pcTableSets)
            {}
        virtual ~MJL_DirPredictor() {}

        // Update counters on access
        void MJL_updatePredictMshrQueue(const PacketPtr pkt) {
            Addr pkt_blkAddr = pkt->getBlockAddr(blkSize);
            Addr pkt_crossBlkAddr = pkt->MJL_getCrossBlockAddr(blkSize);
            bool pkt_isSecure = pkt->isSecure();
            /* MJL_Test 
            std::cout << "MJL_predDebug: MJL_mshrPredictDir update " << pkt->print();
             */
            for (std::list<PredictMshrEntry>::iterator it = copyPredictMshrQueue.begin(); it != copyPredictMshrQueue.end(); it++) {
                if (pkt->getSize() > sizeof(uint64_t)) {
                    pkt_blkAddr = pkt->getBlockAddr(blkSize);
                    if (pkt_blkAddr == it->blkAddr && pkt->MJL_getCmdDir() == it->blkDir && pkt_isSecure == it->isSecure) {
                        for (unsigned i = 0; i <= pkt->getSize()/sizeof(uint64_t); ++i) {
                            it->blkHits[pkt->getOffset(blkSize)/sizeof(uint64_t) + i] |= true;
                        }
                    }
                    pkt_crossBlkAddr = pkt->getBlockAddr(blkSize);
                    if (pkt_crossBlkAddr == it->crossBlkAddr && pkt->MJL_getCmdDir() == it->crossBlkDir && pkt_isSecure == it->isSecure) {
                        for (unsigned i = 0; i <= pkt->getSize()/sizeof(uint64_t); ++i) {
                            it->crossBlkHits[pkt->getOffset(blkSize)/sizeof(uint64_t) + i] |= true;
                        }
                    }
                    /* MJL_Test 
                    if ((pkt_blkAddr == it->blkAddr && pkt->MJL_getCmdDir() == it->blkDir && pkt_isSecure == it->isSecure) || (pkt_crossBlkAddr == it->crossBlkAddr && pkt->MJL_getCmdDir() == it->crossBlkDir && pkt_isSecure == it->isSecure)) {
                        std::cout << ", " << it->print();
                    }
                     */
                } else {
                    pkt_blkAddr = pkt->MJL_getDirBlockAddr(blkSize, it->blkDir);
                    if (pkt_blkAddr == it->blkAddr && pkt_isSecure == it->isSecure) {
                        it->blkHits[pkt->MJL_getDirOffset(blkSize, it->blkDir)/sizeof(uint64_t)] |= true;
                    }
                    pkt_crossBlkAddr = pkt->MJL_getDirBlockAddr(blkSize, it->crossBlkDir);
                    if (pkt_crossBlkAddr == it->crossBlkAddr && pkt_isSecure == it->isSecure) {
                        it->crossBlkHits[pkt->MJL_getDirOffset(blkSize, it->crossBlkDir)/sizeof(uint64_t)] |= true;
                    }
                    /* MJL_Test 
                    if ((pkt_blkAddr == it->blkAddr && pkt_isSecure == it->isSecure) || (pkt_crossBlkAddr == it->crossBlkAddr && pkt_isSecure == it->isSecure)) {
                        std::cout << ", " << it->print();
                    }
                     */
                }
            }
            /* MJL_Test 
            std::cout << std::endl;
             */
        }
        // Add entry to both predict queue
        void MJL_addToPredictMshrQueue(const PacketPtr pkt, const MSHR* mshr) {
            if (pkt->req->hasPC()) {
                copyPredictMshrQueue.emplace_back(pkt, mshr, blkSize);
                /* MJL_Test 
                std::cout << "MJL_predDebug: MJL_mshrPredictDir create mshr " << pkt->print() << std::endl;
                 */
                /* for (std::list<PredictMshrEntry>::iterator it = copyPredictMshrQueue.begin(); it != copyPredictMshrQueue.end(); it++) {
                    Addr pkt_blkAddr = pkt->getBlockAddr(blkSize);
                    Addr pkt_crossBlkAddr = pkt->MJL_getCrossBlockAddr(blkSize);
                    bool pkt_isSecure = pkt->isSecure();
                    if (pkt_crossBlkAddr == it->crossBlkAddr && pkt_isSecure == it->isSecure && pkt->MJL_getCrossCmdDir() == it->crossBlkDir) {
                        copyPredictMshrQueue.back().crossBlkHits[pkt->MJL_getDirOffset(blkSize, it->crossBlkDir)/sizeof(uint64_t)] |= it->crossBlkHits[pkt->MJL_getDirOffset(blkSize, it->crossBlkDir)/sizeof(uint64_t)];
                    } else if (pkt_blkAddr == it->crossBlkAddr && pkt_isSecure == it->isSecure && pkt->MJL_getCmdDir() == it->crossBlkDir) {
                        copyPredictMshrQueue.back().blkHits[pkt->MJL_getDirOffset(blkSize, it->crossBlkDir)/sizeof(uint64_t)] |= it->crossBlkHits[pkt->MJL_getDirOffset(blkSize, it->crossBlkDir)/sizeof(uint64_t)];
                    }
                } */
            }
        }
        // Remove entry from both predict queues and get predict direction
        void MJL_removeFromPredictMshrQueue(const MSHR* mshr, bool targetHasPC) {
            // software prefetch actually happened and does not have a pc... need to bypass this case since prediction cannot be made without a pc
            if (!targetHasPC) {
                return;
            }

            std::list<PredictMshrEntry>::iterator entry_found = copyPredictMshrQueue.end();
            StrideEntry *pcTable_entry;
            MasterID master_id = 0;
            // Find the entry to be removed
            for (std::list<PredictMshrEntry>::iterator it = copyPredictMshrQueue.begin(); it != copyPredictMshrQueue.end(); it++) {
                if (mshr == it->mshr) {
                    entry_found = it;
                    assert(mshr->blkAddr == it->blkAddr && mshr->MJL_qEntryDir == it->blkDir && mshr->isSecure == it->isSecure);
                    break;
                }
            }
            assert(entry_found != copyPredictMshrQueue.end());
            // Add predict direction to pc table
            master_id = useMasterId ? entry_found->masterId : 0;
            if (pcTableHit(entry_found->pc, entry_found->isSecure, master_id, pcTable_entry)) {
                pcTable_entry->lastAddr = entry_found->accessAddr;
                // pcTable_entry->lastPredDir = entry_found->blkDir;
                if (pcTable_entry->lastPredDir == entry_found->blkDir) {
                    for (unsigned i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                        pcTable_entry->blkHits[i] = entry_found->blkHits[i];
                        pcTable_entry->crossBlkHits[i] = entry_found->crossBlkHits[i];
                    }
                } else {
                    for (unsigned i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                        pcTable_entry->blkHits[i] = entry_found->crossBlkHits[i];
                        pcTable_entry->crossBlkHits[i] = entry_found->blkHits[i];
                    }
                }
                pcTable_entry->lastRowOff = entry_found->lastRowOff;
                pcTable_entry->lastColOff = entry_found->lastColOff;
            } else {
                StrideEntry* victim_entry = pcTableVictim(entry_found->pc, master_id);
                victim_entry->instAddr = entry_found->pc;
                victim_entry->lastAddr = entry_found->accessAddr;
                victim_entry->isSecure= entry_found->isSecure;
                victim_entry->stride = 0;
                victim_entry->confidence = startConf;
                victim_entry->lastPredDir = entry_found->blkDir;
                for (unsigned i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                    victim_entry->blkHits[i] = entry_found->blkHits[i];
                    victim_entry->crossBlkHits[i] = entry_found->crossBlkHits[i];
                }
                victim_entry->lastRowOff = entry_found->lastRowOff;
                victim_entry->lastColOff = entry_found->lastColOff;
            }

            copyPredictMshrQueue.erase(entry_found);
        }
        // Update prediction for prefetch based scheme
        void MJL_updatePfPredictEntry(const PacketPtr pkt, MemCmd::MJL_DirAttribute predictedDir) {
            assert(MJL_pfBasedPredictDir);
            // Get required packet info
            Addr pc = pkt->req->getPC();
            bool is_secure = pkt->isSecure();
            MasterID master_id = useMasterId ? pkt->req->masterId() : 0;

            // Lookup pc-based information
            StrideEntry *entry;

            if (pcTableHit(pc, is_secure, master_id, entry)) {
                entry->lastPredDir = predictedDir;
            } else {
                StrideEntry* entry = pcTableVictim(pc, master_id);
                entry->instAddr = pc;
                entry->lastAddr = pkt->getAddr();
                entry->isSecure= is_secure;
                entry->stride = 0;
                entry->confidence = startConf;
                entry->lastPredDir = predictedDir;
            }
        }

        MemCmd::MJL_DirAttribute MJL_predictDir(const PacketPtr &pkt) {
            MemCmd::MJL_DirAttribute predictedDir = pkt->MJL_getCmdDir();
            if (observeAccess(pkt) && pkt->req->hasPC()) {
                // Get required packet info
                Addr pkt_addr = pkt->getAddr();
                Addr pc = pkt->req->getPC();
                bool is_secure = pkt->isSecure();
                MasterID master_id = useMasterId ? pkt->req->masterId() : 0;

                // Lookup pc-based information
                StrideEntry *entry;

                if (pcTableHit(pc, is_secure, master_id, entry)) {
                    // Hit in table
                    if (MJL_mshrPredictDir) {
                        predictedDir = MJL_mshrPredict(pkt_addr, entry, pkt->MJL_getDirOffset(blkSize, MemCmd::MJL_DirAttribute::MJL_IsRow)/sizeof(uint64_t), pkt->MJL_getDirOffset(blkSize, MemCmd::MJL_DirAttribute::MJL_IsColumn)/sizeof(uint64_t), pkt->isWrite());
                        /* MJL_Test 
                        std::cout << "MJL_predDebug: MJL_mshrPredictDir " << pkt->print() << " predicted " << predictedDir << std::endl;
                         */
                    } else if (MJL_pfBasedPredictDir) {
                        predictedDir = entry->lastPredDir;
                    } else {
                        predictedDir = MJL_stridePredict(pkt_addr, entry, pkt->MJL_getCmdDir());
                    }
                } else {
                    // Miss in table
                    /* MJL_Comment 
                    DPRINTF(HWPrefetch, "Miss: PC %x pkt_addr %x (%s)\n", pc, pkt_addr,
                            is_secure ? "s" : "ns");
                     */

                    if (!MJL_mshrPredictDir && !MJL_pfBasedPredictDir) {
                        StrideEntry* entry = pcTableVictim(pc, master_id);
                        entry->instAddr = pc;
                        entry->lastAddr = pkt_addr;
                        entry->isSecure= is_secure;
                        entry->stride = 0;
                        entry->confidence = startConf;
                        entry->lastPredDir = MemCmd::MJL_DirAttribute::MJL_IsRow;
                    }
                }
            }
            return predictedDir;
        }
    };

    MJL_DirPredictor * MJL_dirPredictor;
    bool MJL_predictDir;
    bool MJL_mshrPredictDir;
    bool MJL_pfBasedPredictDir;

    bool MJL_ignoreExtraTagCheckLatency;

    std::map < Addr, std::map < MemCmd::MJL_DirAttribute, uint64_t > > * MJL_perPCAccessCount;
    std::map < Addr, std::vector < MemCmd::MJL_DirAttribute > > * MJL_perPCAccessTrace;

    void MJL_countAccess(Addr pc, MemCmd::MJL_DirAttribute dir) {
        if ( MJL_perPCAccessCount->find(pc) == MJL_perPCAccessCount->end() || (*MJL_perPCAccessCount)[pc].find(dir) == (*MJL_perPCAccessCount)[pc].end() ) {
            (*MJL_perPCAccessCount)[pc][dir] = 0;
        }
        (*MJL_perPCAccessCount)[pc][dir]++;
        (*MJL_perPCAccessTrace)[pc].push_back(dir);
    }

    void MJL_printAccess() {
        std::cout << std::endl << "==== MJL_perPCAccessCount Begin ====" << std::endl;
        std::cout << "PC Row_Accesses Col_Accesses" << std::endl;
        for (auto it = MJL_perPCAccessCount->begin(); it != MJL_perPCAccessCount->end(); ++it) {
            if ( it->second.find(MemCmd::MJL_DirAttribute::MJL_IsRow) == it->second.end() ) {
                (it->second)[MemCmd::MJL_DirAttribute::MJL_IsRow] = 0;
            }
            if ( it->second.find(MemCmd::MJL_DirAttribute::MJL_IsColumn) == it->second.end() ) {
                (it->second)[MemCmd::MJL_DirAttribute::MJL_IsColumn] = 0;
            }
            std::cout << std::hex << it->first << std::dec << " " << (it->second)[MemCmd::MJL_DirAttribute::MJL_IsRow] << " " << (it->second)[MemCmd::MJL_DirAttribute::MJL_IsColumn] << std::endl;
        }
        std::cout << "==== MJL_perPCAccessCount End ====" << std::endl;
        std::cout << "==== MJL_perPCAccessTrace Begin ====" << std::endl;
        std::cout << "PC Trace(0 for row, 1 for column)" << std::endl;
        for (auto it = MJL_perPCAccessTrace->begin(); it != MJL_perPCAccessTrace->end(); ++it) {
            uint64_t temp_rowAccesses = 0;
            uint64_t temp_colAccesses = 0;
            uint64_t current_accesses = 0;
            if ((*MJL_perPCAccessCount)[it->first][MemCmd::MJL_DirAttribute::MJL_IsRow] == 0 || (*MJL_perPCAccessCount)[it->first][MemCmd::MJL_DirAttribute::MJL_IsColumn] == 0) { continue; }
            std::cout << std::hex << it->first << std::dec << " ";
            MemCmd::MJL_DirAttribute last_dir = MemCmd::MJL_DirAttribute::MJL_IsInvalid;
            for (auto dir : (it->second)) {
                if (dir == last_dir) {
                    current_accesses++;
                } else if (last_dir == MemCmd::MJL_DirAttribute::MJL_IsInvalid) {
                    if (dir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                        std::cout << 0 << "*";
                    } else if (dir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                        std::cout << 1 << "*";
                    };
                    current_accesses++;
                } else {
                    std::cout << current_accesses << " ";
                    current_accesses = 0;
                    if (dir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                        std::cout << 0 << "*";
                    } else if (dir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                        std::cout << 1 << "*";
                    }
                    current_accesses++;
                }
                last_dir = dir;
                if (dir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                    temp_rowAccesses++;
                } else if (dir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                    temp_colAccesses++;
                }
            }
            std::cout << current_accesses << std::endl;
            assert(temp_rowAccesses == (*MJL_perPCAccessCount)[it->first][MemCmd::MJL_DirAttribute::MJL_IsRow] && temp_colAccesses == (*MJL_perPCAccessCount)[it->first][MemCmd::MJL_DirAttribute::MJL_IsColumn]);
        }
        std::cout << "==== MJL_perPCAccessTrace End ====" << std::endl;
    }
    void MJL_printTestBloomFiltersStats () {    
        if (MJL_Test_rowColBloomFilters) {
            std::cout << "==== MJL_Test_rowColBloomFilters Stats Begin ====" << this->name() << std::endl;
            std::cout << "Name True_Neg False_Pos True_Pos" << std::endl;
            std::cout << MJL_Test_rowColBloomFilters->printStats();
            std::cout << "==== MJL_Test_rowColBloomFilters Stats End ====" << this->name() << std::endl;
        }
    }
    /* MJL_End */

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
     * The set that contains the PC number of vector instructions
     */
    std::set<Addr> MJL_VecListSet;

    /**
     * The name of the input file that contains the mapping information from PC to direction
     */
    std::string MJL_PC2DirFilename;
    /**
     * The name of the input file that contains the list of vector instructions
     */
    std::string MJL_VecListFilename;
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
     * The function that reads in the list of vector instructions from the file
     */
    void MJL_readVecList () {
        std::ifstream MJL_VecListFile;
        std::string line;
        Addr tempPC;

        assert(MJL_VecListFilename != "");
        MJL_VecListFile.open(MJL_VecListFilename);
        if (MJL_VecListFile.is_open()) {
            std::cout << this->name() << "::Reading vector instruction PC from " << MJL_VecListFilename << ":" << std::endl;
            /* MJL_Test: file information output */
            std::cout << "Vec inst PC:";
            /* */
            while (getline(MJL_VecListFile, line)) {
                std::stringstream(line) >> std::hex >> tempPC >> std::dec;
                if (MJL_VecListSet.find(tempPC) != MJL_VecListSet.end()) {
                    std::cout << "MJL_Warning: Repeated vector PC found in file!\n";
                }
                MJL_VecListSet.insert(tempPC);
                /* MJL_Test: file information output */
                if (MJL_VecListSet.find(tempPC) != MJL_VecListSet.end()) {
                    std::cout << " " << std::hex << *(MJL_VecListSet.find(tempPC)) << std::dec;
                }
                /* */
            }
            /* MJL_Test: file information output */
            std::cout << std::endl;
            /* */
        }
        else {
            std::cout << "MJL_Error: Could not open input file!\n";
            assert(MJL_VecListFile.is_open());
        }
        MJL_VecListFile.close();
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
            // And that the address accessed is not at the end of a cacheline for a packet with first position
            // Or that the address accessed is not at the begining of a cacheline for a packet with second position 
            } else if ( (MJL_ColVecList.at(pkt->req->getPC()).pos == 0 && pkt->getOffset(cache->blkSize) + 2 * pkt->getSize() <= cache->blkSize) ||
                        (MJL_ColVecList.at(pkt->req->getPC()).pos == 1 && pkt->getOffset(cache->blkSize) >= pkt->getSize()) ) {
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

    // MJL_TODO: dummy functions for now
    class MJL_FootPrint {
        public:

            class MJL_FootPrintEntry {
                public:
                    bool MJL_rowFootPrint[8];
                    bool MJL_colFootPrint[8];
                    bool MJL_rowEvictFootPrint[8];
                    bool MJL_colEvictFootPrint[8];

                    MJL_FootPrintEntry() : MJL_rowFootPrint{false, false, false, false, false, false, false, false}, MJL_colFootPrint{false, false, false, false, false, false, false, false}, MJL_rowEvictFootPrint{false, false, false, false, false, false, false, false}, MJL_colEvictFootPrint{false, false, false, false, false, false, false, false} {}
            };

            unsigned logSize;
            unsigned numSets;
            std::map<Addr, MJL_FootPrintEntry*> MJL_footPrintLog;
            std::list<Addr> MJL_orderLog;

            MJL_FootPrint(unsigned size, unsigned numsets) : logSize(size), numSets(numsets) {}

            void MJL_addFootPrint(Addr tag, int set, MemCmd::MJL_DirAttribute dir) {
                auto found = MJL_orderLog.end();
                Addr tag_set = (tag * (numSets) + set)/sizeof(uint64_t);
                auto footPrintEntry = MJL_footPrintLog.find(tag_set);
                
                int set_offset = set % sizeof(uint64_t);
                if (dir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                    if (footPrintEntry == MJL_footPrintLog.end()) {
                        MJL_footPrintLog[tag_set] = new MJL_FootPrintEntry();
                    }
                    MJL_footPrintLog[tag_set]->MJL_rowFootPrint[set_offset] = true;
                } else if (dir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                    if (footPrintEntry == MJL_footPrintLog.end()) {
                        MJL_footPrintLog[tag_set] = new MJL_FootPrintEntry();
                    }
                    MJL_footPrintLog[tag_set]->MJL_colFootPrint[set_offset] = true;
                }
                for (auto it = MJL_orderLog.begin(); it != MJL_orderLog.end(); ++it) {
                    if (*it == tag_set) {
                        found = it;
                        break;
                    }
                }
                if (found == MJL_orderLog.end()) {
                    MJL_orderLog.push_back(tag_set);
                } else {
                    MJL_orderLog.erase(found);
                    MJL_orderLog.push_back(tag_set);
                }
            }
 
            void MJL_addEvictFootPrint(Addr tag, int set, MemCmd::MJL_DirAttribute dir) {
                Addr tag_set = (tag * (numSets) + set)/sizeof(uint64_t);

                int set_offset = set % sizeof(uint64_t);
                if (dir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                    MJL_footPrintLog[tag_set] = new MJL_FootPrintEntry();
                    MJL_footPrintLog[tag_set]->MJL_rowEvictFootPrint[set_offset] = true;
                } else if (dir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                    MJL_footPrintLog[tag_set] = new MJL_FootPrintEntry();
                    MJL_footPrintLog[tag_set]->MJL_colEvictFootPrint[set_offset] = true;
                }
            }

            void MJL_clearFootPrint(Addr tag, int set) {
                Addr tag_set = (tag * (numSets) + set)/sizeof(uint64_t);
                MJL_FootPrintEntry *clearEntry = MJL_footPrintLog[tag_set];
                for (int i = 0; i < 8; ++i) {
                    clearEntry->MJL_rowFootPrint[i] = false;
                    clearEntry->MJL_colFootPrint[i] = false;
                }
            }

            bool MJL_isFullFootPrint(Addr tag, int set) {
                bool isFull = true;
                Addr tag_set = (tag * (numSets) + set)/sizeof(uint64_t);
                MJL_FootPrintEntry *entry = MJL_footPrintLog[tag_set];
                for (int i = 0; i < 8; ++i) {
                    isFull &= entry->MJL_rowFootPrint[i];
                }
                if (isFull) {
                    return isFull;
                } else {
                    isFull = true;
                    for (int i = 0; i < 8; ++i) {
                        isFull &= entry->MJL_colFootPrint[i];
                    }
                }
                return isFull;
            }

            void MJL_setFullFootPrint(Addr tag, int set) {
                Addr tag_set = (tag * (numSets) + set)/sizeof(uint64_t);
                MJL_FootPrintEntry *setEntry = MJL_footPrintLog[tag_set];
                for (int i = 0; i < 8; ++i) {
                    setEntry->MJL_rowFootPrint[i] = true;
                    setEntry->MJL_colFootPrint[i] = true;
                }
            }

            int MJL_touchedBytes(Addr tag, int set, unsigned blkSize) {
                int touchedRows = 0;
                int touchedCols = 0;
                Addr tag_set = (tag * (numSets) + set)/sizeof(uint64_t);
                MJL_FootPrintEntry *setEntry = MJL_footPrintLog[tag_set];
                for (int i = 0; i < 8; ++i) {
                    if (setEntry->MJL_rowFootPrint[i]) {
                        touchedRows++;
                    } else if (setEntry->MJL_colFootPrint[i]) {
                        touchedCols++;
                    }
                }
                return (touchedRows + touchedCols) * blkSize - touchedRows * touchedCols * sizeof(uint64_t);
            }
    };

    MJL_FootPrint *MJL_footPrint;

    void MJL_footPrintCachelines(Addr triggerAddr, MemCmd::MJL_DirAttribute triggerDir, std::list<Addr>* BlkAddrs, std::list<MemCmd::MJL_DirAttribute>* BlkDirs) {
        Addr triggerTag = tags->MJL_extractTag(triggerAddr, MemCmd::MJL_DirAttribute::MJL_IsRow);
        int triggerSet = tags->MJL_extractSet(triggerAddr, MemCmd::MJL_DirAttribute::MJL_IsRow);
        Addr triggerTag_Set = (triggerTag * (tags->getNumSets()) + triggerSet)/sizeof(uint64_t);
        for (int i = 0; i < 8; ++i) {
            Addr rowBlkAddr = tags->MJL_regenerateBlkAddr(triggerTag, MemCmd::MJL_DirAttribute::MJL_IsRow, triggerSet/sizeof(uint64_t) + i);
            Addr colBlkAddr = tags->MJL_regenerateBlkAddr(triggerTag, MemCmd::MJL_DirAttribute::MJL_IsRow, triggerSet/sizeof(uint64_t)) + i * sizeof(uint64_t);
            if (triggerSet/sizeof(uint64_t) == i) {
                if (triggerDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                    if (MJL_footPrint->MJL_footPrintLog[triggerTag_Set]->MJL_colFootPrint[i]) {
                        BlkAddrs->push_back(colBlkAddr);
                        BlkDirs->push_back(MemCmd::MJL_DirAttribute::MJL_IsColumn);
                    }
                } else if (triggerDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                    if (MJL_footPrint->MJL_footPrintLog[triggerTag_Set]->MJL_rowFootPrint[i]) {
                        BlkAddrs->push_back(rowBlkAddr);
                        BlkDirs->push_back(MemCmd::MJL_DirAttribute::MJL_IsRow);
                    }
                }
                continue;
            }
            if (MJL_footPrint->MJL_footPrintLog[triggerTag_Set]->MJL_rowFootPrint[i]) {
                BlkAddrs->push_back(rowBlkAddr);
                BlkDirs->push_back(MemCmd::MJL_DirAttribute::MJL_IsRow);
            } else if (MJL_footPrint->MJL_footPrintLog[triggerTag_Set]->MJL_colFootPrint[i]) {
                BlkAddrs->push_back(colBlkAddr);
                BlkDirs->push_back(MemCmd::MJL_DirAttribute::MJL_IsColumn);
            }
        }

        MJL_footPrint->MJL_clearFootPrint(triggerTag, triggerSet);
    }

    void MJL_fullCachelines(Addr triggerAddr, MemCmd::MJL_DirAttribute triggerDir, std::list<Addr>* BlkAddrs, std::list<MemCmd::MJL_DirAttribute>* BlkDirs) {
        Addr triggerTag = tags->MJL_extractTag(triggerAddr, MemCmd::MJL_DirAttribute::MJL_IsRow);
        int triggerSet = tags->MJL_extractSet(triggerAddr, MemCmd::MJL_DirAttribute::MJL_IsRow);
        for (int i = 0; i < 8; ++i) {
            Addr reqBlkAddr = triggerAddr;
            if (triggerDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                reqBlkAddr = tags->MJL_regenerateBlkAddr(triggerTag, MemCmd::MJL_DirAttribute::MJL_IsRow, (triggerSet/sizeof(uint64_t)) * sizeof(uint64_t) + i);
            } else if (triggerDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                reqBlkAddr = tags->MJL_regenerateBlkAddr(triggerTag, MemCmd::MJL_DirAttribute::MJL_IsRow, (triggerSet/sizeof(uint64_t)) * sizeof(uint64_t)) + i * sizeof(uint64_t);
            }
            if (tags->MJL_extractSet(triggerAddr, triggerDir)%sizeof(uint64_t) == i) {
                assert(triggerAddr == reqBlkAddr);
                continue;
            } else {
                BlkAddrs->push_back(reqBlkAddr);
                BlkDirs->push_back(triggerDir);
            }
        }

        MJL_footPrint->MJL_clearFootPrint(triggerTag, triggerSet);
    }

    virtual void MJL_markBlocked2D(PacketPtr pkt, MSHR * mshr) {
        // For physically 2D cache, block the mshr with the crossing direction ones waiting in the mshr if those combined with the cache lines for the tile in the cache make up the whole tile.
        if (MJL_2DCache) {
            bool MJL_RowsPresent[blkSize/sizeof(uint64_t)];
            bool MJL_ColsPresent[blkSize/sizeof(uint64_t)];
            for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                MJL_RowsPresent[i] = false;
                MJL_ColsPresent[i] = false;
            }
            bool MJL_wholeTileValidRow = true;
            bool MJL_wholeTileValidCol = true;
            CacheBlk * MJL_tileBlk = nullptr;
            MSHR * MJL_rowMshr = nullptr;
            MSHR * MJL_colMshr = nullptr;
            Addr MJL_rowAddr = pkt->getAddr();
            Addr MJL_colAddr = pkt->getAddr();

            for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                if (pkt->MJL_cmdIsRow()) {
                    MJL_rowAddr = pkt->MJL_getBlockAddrs(blkSize, i);
                    MJL_colAddr = pkt->MJL_getCrossBlockAddrs(blkSize, i);
                } else if (pkt->MJL_cmdIsColumn()) {
                    MJL_rowAddr = pkt->MJL_getCrossBlockAddrs(blkSize, i);
                    MJL_colAddr = pkt->MJL_getBlockAddrs(blkSize, i);
                }

                // Check for cache blocks
                MJL_tileBlk = tags->MJL_findBlock(MJL_rowAddr, MemCmd::MJL_DirAttribute::MJL_IsRow, pkt->isSecure());
                if (MJL_tileBlk) {
                    if (MJL_tileBlk->isValid()) {
                        MJL_RowsPresent[i] |= true;
                    }
                    for (int j = 0; j < blkSize/sizeof(uint64_t); ++j) {
                        MJL_ColsPresent[j] |= MJL_tileBlk->MJL_crossValid[j];
                    }
                }

                // Check for mshrs
                MJL_rowMshr = mshrQueue.MJL_findMatch(MJL_rowAddr, MemCmd::MJL_DirAttribute::MJL_IsRow, pkt->isSecure());
                MJL_colMshr = mshrQueue.MJL_findMatch(MJL_colAddr, MemCmd::MJL_DirAttribute::MJL_IsColumn, pkt->isSecure());
                if (MJL_rowMshr) {
                    MJL_RowsPresent[i] |= true;
                }
                if (MJL_colMshr) {
                    MJL_ColsPresent[i] |= true;
                }
            }

            for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                MJL_wholeTileValidRow &= MJL_RowsPresent[i];
                MJL_wholeTileValidCol &= MJL_ColsPresent[i];
            }

            if (MJL_wholeTileValidRow && pkt->MJL_cmdIsColumn()) {
                for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                    MJL_rowMshr = mshrQueue.MJL_findMatch(pkt->MJL_getCrossBlockAddrs(blkSize, i), MemCmd::MJL_DirAttribute::MJL_IsRow, pkt->isSecure());
                    MJL_tileBlk = tags->MJL_findBlock(pkt->MJL_getCrossBlockAddrs(blkSize, i), MemCmd::MJL_DirAttribute::MJL_IsRow, pkt->isSecure());
                    if (MJL_rowMshr && !(MJL_tileBlk && MJL_tileBlk->isValid())) {
                        /* MJL_Test 
                        std::cout << "MJL_Debug: Block2D row" << std::endl;
                         */
                        mshr->MJL_getLastTarget()->MJL_isBlockedBy.push_back(MJL_rowMshr->MJL_getLastTarget());
                        MJL_rowMshr->MJL_getLastTarget()->MJL_isBlocking.push_back(mshr->MJL_getLastTarget());
                    }
                }
            } else if (MJL_wholeTileValidCol && pkt->MJL_cmdIsRow()) {
                for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                    MJL_colMshr = mshrQueue.MJL_findMatch(pkt->MJL_getCrossBlockAddrs(blkSize, i), MemCmd::MJL_DirAttribute::MJL_IsColumn, pkt->isSecure());
                    MJL_tileBlk = tags->MJL_findBlock(pkt->MJL_getCrossBlockAddrs(blkSize, i), MemCmd::MJL_DirAttribute::MJL_IsRow, pkt->isSecure());
                    if (MJL_colMshr && !(MJL_tileBlk && MJL_tileBlk->MJL_crossValid[i])) {
                        /* MJL_Test 
                        std::cout << "MJL_Debug: Block2D col" << std::endl;
                         */
                        mshr->MJL_getLastTarget()->MJL_isBlockedBy.push_back(MJL_colMshr->MJL_getLastTarget());
                        MJL_colMshr->MJL_getLastTarget()->MJL_isBlocking.push_back(mshr->MJL_getLastTarget());
                    }
                }
            }
        }
    }

    void MJL_allocateFootPrintMissBuffer(PacketPtr pkt, Tick time, bool sched_send = true)
    {
        Addr triggerAddr = pkt->getAddr();
        MemCmd::MJL_DirAttribute triggerDir = pkt->MJL_getCmdDir();
        // Should only be used when the tile does not exist
        if (pkt->req->isUncacheable() || tags->MJL_tileExists(triggerAddr, pkt->isSecure())!=(int)tags->getNumWays()) {
            return;
        }

        std::list<Addr> BlkAddrs;
        std::list<MemCmd::MJL_DirAttribute> BlkDirs;
        MJL_footPrintCachelines(triggerAddr, triggerDir, &BlkAddrs, &BlkDirs);
        assert(BlkAddrs.size() == BlkDirs.size());
        // If the cache line is not in the mshr, then allocate a new mshr entry with target source footprint fill
        
        MSHR *mshr = nullptr;
        std::list<MemCmd::MJL_DirAttribute>::iterator dir_it = BlkDirs.begin();
        for (std::list<Addr>::iterator addr_it = BlkAddrs.begin(); addr_it != BlkAddrs.end(); ++addr_it, ++dir_it) {
            mshr = mshrQueue.MJL_findMatch(*addr_it, *dir_it, pkt->isSecure());
            if (mshr == nullptr) {
                Request *fp_req =
                        new Request(*addr_it, blkSize, 0, pkt->req->masterId());
                fp_req->MJL_cachelineSize = blkSize;
                fp_req->MJL_rowWidth = pkt->req->MJL_rowWidth;
                MemCmd fp_cmd = pkt->needsWritable() ? MemCmd::ReadExReq :
            (isReadOnly ? MemCmd::ReadCleanReq : MemCmd::ReadSharedReq);
                PacketPtr fp_pkt = new Packet(fp_req, fp_cmd, blkSize);
                fp_pkt->cmd.MJL_setCmdDir(pkt->MJL_getCmdDir());
                mshr = mshrQueue.MJL_allocateFootPrint(MJL_blockAlign(*addr_it, *dir_it), *dir_it, blkSize,
                                                fp_pkt, time, order++,
                                                allocOnFill(pkt->cmd));
                
                MJL_markBlockInfo(mshr);
                if (MJL_2DCache) { 
                    MJL_markBlocked2D(fp_pkt, mshr);
                }

                if (mshrQueue.isFull()) {
                    setBlocked((BlockedCause)MSHRQueue_MSHRs);
                }

                if (sched_send) {
                    // schedule the send
                    schedMemSideSendEvent(time);
                }
            }
        }

    }

    void MJL_allocateFullMissBuffer(PacketPtr pkt, Tick time, bool sched_send = true)
    {
        Addr triggerAddr = pkt->getAddr();
        MemCmd::MJL_DirAttribute triggerDir = pkt->MJL_getCmdDir();
        // Should only be used when the tile does not exist
        if (pkt->req->isUncacheable() || tags->MJL_tileExists(triggerAddr, pkt->isSecure())!=(int)tags->getNumWays()) {
            return;
        }

        std::list<Addr> BlkAddrs;
        std::list<MemCmd::MJL_DirAttribute> BlkDirs;
        MJL_fullCachelines(triggerAddr, triggerDir, &BlkAddrs, &BlkDirs);
        assert(BlkAddrs.size() == BlkDirs.size());
        // If the cache line is not in the mshr, then allocate a new mshr entry with target source footprint fill
        
        MSHR *mshr = nullptr;
        std::list<MemCmd::MJL_DirAttribute>::iterator dir_it = BlkDirs.begin();
        for (std::list<Addr>::iterator addr_it = BlkAddrs.begin(); addr_it != BlkAddrs.end(); ++addr_it, ++dir_it) {
            mshr = mshrQueue.MJL_findMatch(*addr_it, *dir_it, pkt->isSecure());
            CacheBlk * blk = tags->MJL_findBlock(*addr_it, MemCmd::MJL_DirAttribute::MJL_IsRow, pkt->isSecure());
            if (mshr == nullptr && !mshrQueue.isFull() && !(blk && ((triggerDir == MemCmd::MJL_DirAttribute::MJL_IsRow && blk->isValid()) || (triggerDir == MemCmd::MJL_DirAttribute::MJL_IsColumn && blk->MJL_crossValid[tags->MJL_extractSet(*addr_it, MemCmd::MJL_DirAttribute::MJL_IsRow)%sizeof(uint64_t)])))) {
                Request *fp_req =
                        new Request(*addr_it, blkSize, 0, pkt->req->masterId());
                fp_req->MJL_cachelineSize = blkSize;
                fp_req->MJL_rowWidth = pkt->req->MJL_rowWidth;
                MemCmd fp_cmd = pkt->needsWritable() ? MemCmd::ReadExReq :
            (isReadOnly ? MemCmd::ReadCleanReq : MemCmd::ReadSharedReq);
                PacketPtr fp_pkt = new Packet(fp_req, fp_cmd, blkSize);
                fp_pkt->cmd.MJL_setCmdDir(pkt->MJL_getCmdDir());
                mshr = mshrQueue.MJL_allocateFootPrint(MJL_blockAlign(*addr_it, *dir_it), *dir_it, blkSize,
                                                fp_pkt, time, order++,
                                                allocOnFill(pkt->cmd));
                
                MJL_markBlockInfo(mshr);
                if (MJL_2DCache) { 
                    MJL_markBlocked2D(fp_pkt, mshr);
                }

                if (mshrQueue.isFull()) {
                    setBlocked((BlockedCause)MSHRQueue_MSHRs);
                }

                if (sched_send) {
                    // schedule the send
                    schedMemSideSendEvent(time);
                }
            }
        }

    }

    // Deal with satisfying requests that were waiting on a full tile
    void MJL_satisfyWaitingCrossing(MSHR * mshr, PacketPtr pkt, CacheBlk * blk, PacketList writebacks, bool is_fill, bool MJL_unreadable) {
        MSHR::Target *initial_tgt = mshr->getTarget();
        int initial_offset = initial_tgt->pkt->getOffset(blkSize);
        bool from_cache = false;
        bool is_error = pkt->isError();
        bool is_invalidate = false;
        bool MJL_invalidate = false;
        bool wasFull = mshrQueue.isFull();
        MSHR::TargetList targets = mshr->extractServiceableTargets(pkt);
        for (auto &target: targets) {
            Packet *tgt_pkt = target.pkt;
            switch (target.source) {
            case MSHR::Target::FromCPU:
                Tick completion_time;
                // Here we charge on completion_time the delay of the xbar if the
                // packet comes from it, charged on headerDelay.
                completion_time = pkt->headerDelay;

                // Software prefetch handling for cache closest to core
                if (tgt_pkt->cmd.isSWPrefetch()) {
                    // a software prefetch would have already been ack'd
                    // immediately with dummy data so the core would be able to
                    // retire it. This request completes right here, so we
                    // deallocate it.
                    delete tgt_pkt->req;
                    delete tgt_pkt;
                    break; // skip response
                }

                // keep track of whether we have responded to another
                // cache
                from_cache = from_cache || tgt_pkt->fromCache();

                // unlike the other packet flows, where data is found in other
                // caches or memory and brought back, write-line requests always
                // have the data right away, so the above check for "is fill?"
                // cannot actually be determined until examining the stored MSHR
                // state. We "catch up" with that logic here, which is duplicated
                // from above.
                if (tgt_pkt->cmd == MemCmd::WriteLineReq) {
                    assert(!is_error);
                    // we got the block in a writable state, so promote
                    // any deferred targets if possible
                    mshr->promoteWritable();
                    // NB: we use the original packet here and not the response!
                    blk = handleFill(tgt_pkt, blk, writebacks,
                                    targets.allocOnFill);
                    assert(blk != nullptr);

                    // treat as a fill, and discard the invalidation
                    // response
                    is_fill = true;
                    is_invalidate = false;
                }

                if (is_fill) {
                    /* MJL_Begin */
                    // Should not need to invalidate anymore on writes, this is taken care of at the time of access.
                    /* MJL_End */
                    satisfyRequest(tgt_pkt, blk, true, mshr->hasPostDowngrade());

                    // How many bytes past the first request is this one
                    int transfer_offset =
                        tgt_pkt->getOffset(blkSize) - initial_offset;   
                    if (transfer_offset < 0) {
                        transfer_offset += blkSize;
                    }

                    // If not critical word (offset) return payloadDelay.
                    // responseLatency is the latency of the return path
                    // from lower level caches/memory to an upper level cache or
                    // the core.
                    completion_time += clockEdge(responseLatency) +
                        (transfer_offset ? pkt->payloadDelay : 0);

                    assert(!tgt_pkt->req->isUncacheable());

                    assert(tgt_pkt->req->masterId() < system->maxMasters());
                    missLatency[tgt_pkt->cmdToIndex()][tgt_pkt->req->masterId()] +=
                        completion_time - target.recvTime;
                } else if (pkt->cmd == MemCmd::UpgradeFailResp) {
                    // failed StoreCond upgrade
                    assert(tgt_pkt->cmd == MemCmd::StoreCondReq ||
                        tgt_pkt->cmd == MemCmd::StoreCondFailReq ||
                        tgt_pkt->cmd == MemCmd::SCUpgradeFailReq);
                    // responseLatency is the latency of the return path
                    // from lower level caches/memory to an upper level cache or
                    // the core.
                    completion_time += clockEdge(responseLatency) +
                        pkt->payloadDelay;
                    tgt_pkt->req->setExtraData(0);
                } else {
                    assert(false);
                    // We are about to send a response to a cache above
                    // that asked for an invalidation; we need to
                    // invalidate our copy immediately as the most
                    // up-to-date copy of the block will now be in the
                    // cache above. It will also prevent this cache from
                    // responding (if the block was previously dirty) to
                    // snoops as they should snoop the caches above where
                    // they will get the response from.
                    /* MJL_Begin */
                    if (MJL_2DCache && is_invalidate && blk && (blk->isValid() || blk->MJL_hasCrossValid())) {
                        MJL_invalidateTile(blk);
                    } else /* MJL_End */if (is_invalidate && blk && blk->isValid()) {
                        invalidateBlock(blk);
                    }
                    // not a cache fill, just forwarding response
                    // responseLatency is the latency of the return path
                    // from lower level cahces/memory to the core.
                    completion_time += clockEdge(responseLatency) +
                        pkt->payloadDelay;
                    if (pkt->isRead() && !is_error) {
                        // sanity check
                        assert(pkt->getAddr() == tgt_pkt->getAddr());
                        assert(pkt->getSize() >= tgt_pkt->getSize());

                        tgt_pkt->setData(pkt->getConstPtr<uint8_t>());
                    }
                }
                tgt_pkt->makeTimingResponse();
                // if this packet is an error copy that to the new packet
                if (is_error)
                    tgt_pkt->copyError(pkt);
                if (tgt_pkt->cmd == MemCmd::ReadResp &&
                    (is_invalidate || mshr->hasPostInvalidate())) {
                    // If intermediate cache got ReadRespWithInvalidate,
                    // propagate that.  Response should not have
                    // isInvalidate() set otherwise.
                    tgt_pkt->cmd = MemCmd::ReadRespWithInvalidate;
                    DPRINTF(Cache, "%s: updated cmd to %s\n", __func__,
                            tgt_pkt->print());
                }
                // Reset the bus additional time as it is now accounted for
                tgt_pkt->headerDelay = tgt_pkt->payloadDelay = 0;
                cpuSidePort->schedTimingResp(tgt_pkt, completion_time, true);
                break;

            case MSHR::Target::FromPrefetcher:
                assert(tgt_pkt->cmd == MemCmd::HardPFReq);
                if (blk)
                    blk->status |= BlkHWPrefetched;
                delete tgt_pkt->req;
                delete tgt_pkt;
                break;
            /* MJL_Begin */
            case MSHR::Target::MJL_FromFootPrintFetch:
                break;
            /* MJL_End */

            case MSHR::Target::FromSnoop:
                // I don't believe that a snoop can be in an error state
                assert(!is_error);
                // response to snoop request
                DPRINTF(Cache, "processing deferred snoop...\n");
                // If the response is invalidating, a snooping target can
                // be satisfied if it is also invalidating. If the reponse is, not
                // only invalidating, but more specifically an InvalidateResp, the
                // MSHR was created due to an InvalidateReq and a cache above is
                // waiting to satisfy a WriteLineReq. In this case even an
                // non-invalidating snoop is added as a target here since this is
                // the ordering point. When the InvalidateResp reaches this cache,
                // the snooping target will snoop further the cache above with the
                // WriteLineReq.
                assert(!(is_invalidate &&
                        pkt->cmd != MemCmd::InvalidateResp &&
                        !mshr->hasPostInvalidate()));
                handleSnoop(tgt_pkt, blk, true, true, mshr->hasPostInvalidate());
                break;

            default:
                panic("Illegal target->source enum %d\n", target.source);
            }
            /* MJL_Begin */
            target.MJL_clearBlocking();
            /* MJL_End */
        }

        maintainClusivity(from_cache, blk);

        // MJL_TODO: probably coherence handling
        /* MJL_Comment
        if (blk && blk->isValid()) {
        */
        /* MJL_Begin */
        if (blk 
            && (blk->isValid() 
                || (MJL_2DCache && blk->MJL_hasCrossValid()))) {
            /* MJL_End */
            // an invalidate response stemming from a write line request
            // should not invalidate the block, so check if the
            // invalidation should be discarded
            if (/* MJL_Begin */MJL_invalidate || /* MJL_End */is_invalidate || mshr->hasPostInvalidate()) {
                /* MJL_Begin */
                assert(false);
                if (MJL_2DCache) {
                    MJL_invalidateTile(blk);
                } else
                /* MJL_End */
                invalidateBlock(blk);
            } else if (mshr->hasPostDowngrade()) {
                /* MJL_Begin */
                assert(false);
                if (MJL_2DCache) {
                    for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                        tags->MJL_findBlockByTile(blk, i)->status &= ~BlkWritable;
                    }
                } else
                /* MJL_End */
                blk->status &= ~BlkWritable;
            }
        }

        if (mshr->promoteDeferredTargets()) {
            // avoid later read getting stale data while write miss is
            // outstanding.. see comment in timingAccess()
            if (blk/* MJL_Begin */ && !MJL_2DCache/* MJL_End */) {
                blk->status &= ~BlkReadable;
            }/* MJL_Begin */ else if (blk && MJL_2DCache) {
                MJL_unreadable |= true;
            }
            assert(!mshr->MJL_deferredAdded);
            /* MJL_End */
            mshrQueue.markPending(mshr);
            schedMemSideSendEvent(clockEdge() + pkt->payloadDelay);
        } else {
            mshrQueue.deallocate(mshr);
            if (wasFull && !mshrQueue.isFull()) {
                clearBlocked(Blocked_NoMSHRs);
            }

            // Request the bus for a prefetch if this deallocation freed enough
            // MSHRs for a prefetch to take place
            /*
            if (prefetcher && mshrQueue.canPrefetch()) {
                Tick next_pf_time = std::max(prefetcher->nextPrefetchReadyTime(),
                                            clockEdge());
                if (next_pf_time != MaxTick)
                    schedMemSideSendEvent(next_pf_time);
            }
            */
        }
    }
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
    void MJL_invalidateTile(CacheBlk *blk);
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
        // Check for bloom filter stats
        bool MJL_hasCrossBlk = false;
        for (unsigned offset = 0; offset < blkSize; offset = offset + sizeof(uint64_t)) {
            Addr MJL_writtenBlkAddr = MJL_blockAlign(MJL_written_addr, MJL_cacheBlkDir);
            CacheBlk * MJL_crossBlk = tags->MJL_findBlock(MJL_addOffsetAddr(MJL_writtenBlkAddr, MJL_cacheBlkDir, offset), MJL_diffDir, is_secure);
            MJL_hasCrossBlk |= ((MJL_crossBlk != nullptr) && MJL_crossBlk->isValid());
        }
        /* MJL_Test */
        if (MJL_Test_rowColBloomFilters) {
            MJL_Test_rowColBloomFilters->test_hasCrossStatCountBloomFilters(MJL_written_addr, MJL_cacheBlkDir, MJL_hasCrossBlk);
            MJL_Test_rowColBloomFilters->test_total(tags->MJL_get_tagsInUse());
        }
        /* */
        if (MJL_rowColBloomFilter) {
            bool MJL_bloomFilterHasCross = MJL_rowColBloomFilter->hasCross(MJL_written_addr, MJL_cacheBlkDir);
            if (MJL_bloomFilterHasCross && MJL_hasCrossBlk) {
                MJL_bloomFilterTruePositives++;
            } else if (MJL_bloomFilterHasCross && !MJL_hasCrossBlk) {
                MJL_bloomFilterFalsePositives++;
            } else if (!MJL_bloomFilterHasCross) {
                assert(!MJL_hasCrossBlk);
                MJL_bloomFilterTrueNegatives++;
            }
            assert(tags->MJL_get_tagsInUse() == MJL_rowColBloomFilter->total());
        }
        // Actual invalidation
        for (unsigned offset = 0; offset < size; offset = offset + sizeof(uint64_t)) {
            if (MJL_wordDirty[offset/sizeof(uint64_t)]) {
                MJL_writtenWord_addr = MJL_addOffsetAddr(MJL_written_addr, MJL_cacheBlkDir, offset);
                MJL_diffDir_blk = tags->MJL_findBlock(MJL_writtenWord_addr, MJL_diffDir, is_secure);
                if ((MJL_diffDir_blk != nullptr) && MJL_diffDir_blk->isValid()) {
                    // MJL_TODO: should check if there's an upgrade miss waiting on this I guess?
                    MJL_conflictWBCount4++;
                    /* MJL_Test */
                    if (MJL_Debug_Out) {
                        std::clog << this->name() << "::MJL_writebufferHitDebug: conflict blk " << std::hex << tags->MJL_regenerateBlkAddr(MJL_diffDir_blk->tag, MJL_diffDir_blk->MJL_blkDir, MJL_diffDir_blk->set) << std::dec << ", " << MJL_diffDir_blk->print() << ", invalidated by pkt_addr " << std::hex << MJL_written_addr << std::dec << std::endl;
                    }
                    /* */
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

    /**
     * Create a writeback request for the column block in physically 2D cache.
     * Need to add clear writable, reset task_id and tickInserted in caller.
     * @param blk The row block crossing with the column block to writeback.
     * @param MJL_offset The column offset of the column block (which word of the row block is crossing with the column block).
     * @param blkDirty Whether the column block is dirty.
     * @return The writeback request for the block.
     */
    PacketPtr MJL_writebackColBlk(CacheBlk *blk, unsigned MJL_offset, bool blkDirty);

    /**
     * Clear the writable status of a tile.
     * @param blk A block in the tile that needs clearing writable.
     */
    void MJL_clearTileWritable(CacheBlk *blk);

    /**
     * Reset task_id and tickInserted status of a tile.
     * @param blk A block in the tile that needs reset tile task_id and tickInserted.
     */
    void MJL_resetTileInfo(CacheBlk *blk);

    /**
     * Create a CleanEvict request for the column block in physically 2D cache.
     * Need to add reset task_id and tickInserted in caller.
     * @param blk The row block crossing with the column block to evict.
     * @param MJL_offset The column offset of the column block (which word of the row block is crossing with the column block).
     * @param blkDirty Whether the column block is dirty.
     * @return The CleanEvict request for the block.
     */
    PacketPtr MJL_cleanEvictColBlk(CacheBlk *blk, unsigned MJL_offset, bool blkDirty);
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

    bool inCache(Addr addr, bool is_secure) const override {
        return (tags->findBlock(addr, is_secure) != 0);
    }
    /* MJL_Begin */
    bool MJL_inCache(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure) const override {
        return (tags->MJL_findBlock(addr, MJL_cacheBlkDir, is_secure) != 0);
    }
    /* MJL_End */

    bool inMissQueue(Addr addr, bool is_secure) const override {
        return (mshrQueue.findMatch(addr, is_secure) != 0);
    }
    /* MJL_Begin */
    bool MJL_inMissQueue(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure) const override {
        return (mshrQueue.MJL_findMatch(addr, MJL_cacheBlkDir, is_secure) != 0);
    }
    bool MJL_crossDirtyInCache(const PacketPtr &pkt) const override {
        bool crossDirtyInCache = false;
        for ( int i = 0; i < blkSize/sizeof(uint64_t); ++i ) {
            CacheBlk * MJL_crossBlk = nullptr;
            MJL_crossBlk = tags->MJL_findBlock(pkt->MJL_getCrossBlockAddrs(blkSize, i), pkt->MJL_getCrossCmdDir(), pkt->isSecure());
            if (MJL_crossBlk && MJL_crossBlk->isValid() && MJL_crossBlk->isDirty()) {
                crossDirtyInCache |= true;
            }
        }
        return crossDirtyInCache;
    }
    bool MJL_crossDirtyInMissQueue(const PacketPtr &pkt) const override {
        bool crossDirtyInMissQueue = false;
        for ( int i = 0; i < blkSize/sizeof(uint64_t); ++i ) {
            MSHR * MJL_crossMSHR = nullptr;
            MJL_crossMSHR = mshrQueue.MJL_findMatch(pkt->MJL_getCrossBlockAddrs(blkSize, i), pkt->MJL_getCrossCmdDir(), pkt->isSecure());
            if (MJL_crossMSHR && MJL_crossMSHR->needsWritable()) {
                crossDirtyInMissQueue |= true;
            }
        }
        return crossDirtyInMissQueue;
    }
    bool MJL_crossDirtyInWriteBuffer(const PacketPtr &pkt) const override {
        bool crossDirtyInWriteBuffer = false;
        for ( int i = 0; i < blkSize/sizeof(uint64_t); ++i ) {
            WriteQueueEntry * MJL_crossWriteQueueEntry = nullptr;
            MJL_crossWriteQueueEntry = writeBuffer.MJL_findMatch(pkt->MJL_getCrossBlockAddrs(blkSize, i), pkt->MJL_getCrossCmdDir(), pkt->isSecure());
            if (MJL_crossWriteQueueEntry) {
                crossDirtyInWriteBuffer |= true;
            }
        }
        return crossDirtyInWriteBuffer;
    }
    bool MJL_crossDirtyInCache(Addr addr, MemCmd::MJL_DirAttribute MJL_cacheBlkDir, bool is_secure) const override {
        bool crossDirtyInCache = false;
        for ( int i = 0; i < blkSize; i += sizeof(uint64_t) ) {
            CacheBlk * MJL_crossBlk = nullptr;
            MemCmd::MJL_DirAttribute MJL_crossBlkDir = MJL_cacheBlkDir;
            if (MJL_cacheBlkDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                MJL_crossBlkDir = MemCmd::MJL_DirAttribute::MJL_IsColumn;
            } else if (MJL_cacheBlkDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                MJL_crossBlkDir = MemCmd::MJL_DirAttribute::MJL_IsRow;
            }
            MJL_crossBlk = tags->MJL_findBlock(MJL_addOffsetAddr(addr, MJL_crossBlkDir, i), MJL_crossBlkDir, is_secure);
            if (MJL_crossBlk && MJL_crossBlk->isValid() && MJL_crossBlk->isDirty()) {
                crossDirtyInCache |= true;
            }
        }
        return crossDirtyInCache;
    }
    bool MJL_crossDirtyInMissQueue(Addr addr, MemCmd::MJL_DirAttribute MJL_cacheBlkDir, bool is_secure) const override {
        bool crossDirtyInMissQueue = false;
        for ( int i = 0; i < blkSize; i += sizeof(uint64_t) ) {
            MSHR * MJL_crossMSHR = nullptr;
            MemCmd::MJL_DirAttribute MJL_crossBlkDir = MJL_cacheBlkDir;
            if (MJL_cacheBlkDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                MJL_crossBlkDir = MemCmd::MJL_DirAttribute::MJL_IsColumn;
            } else if (MJL_cacheBlkDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                MJL_crossBlkDir = MemCmd::MJL_DirAttribute::MJL_IsRow;
            }
            MJL_crossMSHR = mshrQueue.MJL_findMatch(MJL_addOffsetAddr(addr, MJL_crossBlkDir, i), MJL_crossBlkDir, is_secure);
            if (MJL_crossMSHR && MJL_crossMSHR->needsWritable()) {
                crossDirtyInMissQueue |= true;
            }
        }
        return crossDirtyInMissQueue;
    }
    bool MJL_crossDirtyInWriteBuffer(Addr addr, MemCmd::MJL_DirAttribute MJL_cacheBlkDir, bool is_secure) const override {
        bool crossDirtyInWriteBuffer = false;
        for ( int i = 0; i < blkSize; i += sizeof(uint64_t) ) {
            WriteQueueEntry * MJL_crossWriteQueueEntry = nullptr;
            MemCmd::MJL_DirAttribute MJL_crossBlkDir = MJL_cacheBlkDir;
            if (MJL_cacheBlkDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                MJL_crossBlkDir = MemCmd::MJL_DirAttribute::MJL_IsColumn;
            } else if (MJL_cacheBlkDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                MJL_crossBlkDir = MemCmd::MJL_DirAttribute::MJL_IsRow;
            }
            MJL_crossWriteQueueEntry = writeBuffer.MJL_findMatch(MJL_addOffsetAddr(addr, MJL_crossBlkDir, i), MJL_crossBlkDir, is_secure);
            if (MJL_crossWriteQueueEntry) {
                crossDirtyInWriteBuffer |= true;
            }
        }
        return crossDirtyInWriteBuffer;
    }
    /** Test to see if the address is in the MJL_PC2DirMap */
    virtual bool MJL_isInterestedAccess(Addr addr) const {
        return (MJL_PC2DirMap.find(addr) != MJL_PC2DirMap.end());
    }

    virtual bool MJL_isVecAccess(Addr addr) const {
        return (MJL_VecListSet.find(addr) != MJL_VecListSet.end());
    }
    /* MJL_End */

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
