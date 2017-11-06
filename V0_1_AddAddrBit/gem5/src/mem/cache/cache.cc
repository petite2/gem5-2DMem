/*
 * Copyright (c) 2010-2016 ARM Limited
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
 * Copyright (c) 2010,2015 Advanced Micro Devices, Inc.
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
 *          Nathan Binkert
 *          Steve Reinhardt
 *          Ron Dreslinski
 *          Andreas Sandberg
 */

/**
 * @file
 * Cache definitions.
 */

#include "mem/cache/cache.hh"

#include "base/misc.hh"
#include "base/types.hh"
#include "debug/Cache.hh"
#include "debug/CachePort.hh"
#include "debug/CacheTags.hh"
#include "debug/CacheVerbose.hh"
#include "mem/cache/blk.hh"
#include "mem/cache/mshr.hh"
#include "mem/cache/prefetch/base.hh"
#include "sim/sim_exit.hh"

Cache::Cache(const CacheParams *p)
    : BaseCache(p, p->system->cacheLineSize()),
      tags(p->tags),
      prefetcher(p->prefetcher),
      doFastWrites(true),
      prefetchOnAccess(p->prefetch_on_access),
      clusivity(p->clusivity),
      writebackClean(p->writeback_clean),
      tempBlockWriteback(nullptr),/* MJL_Begin */
      MJL_PC2DirFilename(p->MJL_PC2DirFile),
      MJL_VecListFilename(p->MJL_VecListFile), /* MJL_End */
      writebackTempBlockAtomicEvent(this, false,
                                    EventBase::Delayed_Writeback_Pri)
{
    tempBlock = new CacheBlk();
    tempBlock->data = new uint8_t[blkSize];

    cpuSidePort = new CpuSidePort(p->name + ".cpu_side", this,
                                  "CpuSidePort");
    memSidePort = new MemSidePort(p->name + ".mem_side", this,
                                  "MemSidePort");

    tags->setCache(this);
    if (prefetcher)
        prefetcher->setCache(this);

    /* MJL_Begin */
    if (this->name().find("dcache") != std::string::npos) {
        MJL_readPC2DirMap();
        if (MJL_VecListFilename != "") {
            MJL_readVecList();
        }
    }
    MJL_sndPacketWaiting = false;
    MJL_colVecHandler.cache = this;

    if (MJL_2DCache) {
        MJL_footPrint = new MJL_FootPrint(tags->getNumSets());
    }
    /* MJL_End */
}

Cache::~Cache()
{
    delete [] tempBlock->data;
    delete tempBlock;

    delete cpuSidePort;
    delete memSidePort;
}

void
Cache::regStats()
{
    BaseCache::regStats();
}

void
Cache::cmpAndSwap(CacheBlk *blk, PacketPtr pkt)
{
    assert(pkt->isRequest());

    uint64_t overwrite_val;
    bool overwrite_mem;
    uint64_t condition_val64;
    uint32_t condition_val32;

    /* MJL_Begin */
    // Should be the offset of the block since it operates on data of blk
    int offset = tags->MJL_extractBlkOffset(pkt->getAddr(), blk->MJL_blkDir);
    /* MJL_End */
    /* MJL_Comment 
    int offset = tags->extractBlkOffset(pkt->getAddr());
    */
    uint8_t *blk_data = blk->data + offset;

    assert(sizeof(uint64_t) >= pkt->getSize());

    overwrite_mem = true;
    // keep a copy of our possible write value, and copy what is at the
    // memory address into the packet
    pkt->writeData((uint8_t *)&overwrite_val);
    pkt->setData(blk_data);

    if (pkt->req->isCondSwap()) {
        if (pkt->getSize() == sizeof(uint64_t)) {
            condition_val64 = pkt->req->getExtraData();
            overwrite_mem = !std::memcmp(&condition_val64, blk_data,
                                         sizeof(uint64_t));
        } else if (pkt->getSize() == sizeof(uint32_t)) {
            condition_val32 = (uint32_t)pkt->req->getExtraData();
            overwrite_mem = !std::memcmp(&condition_val32, blk_data,
                                         sizeof(uint32_t));
        } else
            panic("Invalid size for conditional read/write\n");
    }

    if (overwrite_mem) {
        std::memcpy(blk_data, &overwrite_val, pkt->getSize());
        /* MJL_Begin */
        blk->MJL_setWordDirty(offset/sizeof(uint64_t));
        // Should be for SPARC so should not be a problem...
        //MJL_invalidateOtherBlocks(pkt->getAddr(), blk->MJL_blkDir, pkt->getSize(), pkt->isSecure());
        /* MJL_End */
        blk->status |= BlkDirty;
    }
}


void
Cache::satisfyRequest(PacketPtr pkt, CacheBlk *blk,
                      bool deferred_response, bool pending_downgrade)
{
    assert(pkt->isRequest());

    /* MJL_Begin */
    // With physically 2D cache, the blk can be only partially valid on the accessed column
    assert(blk && ((MJL_2DCache && pkt->MJL_cmdIsColumn() && blk->MJL_crossValid[pkt->MJL_getColOffset(blkSize)/sizeof(uint64_t)]) || blk->isValid()));
    /* MJL_End */
    /* MJL_Comment 
    assert(blk && blk->isValid());
    */
    // Occasionally this is not true... if we are a lower-level cache
    // satisfying a string of Read and ReadEx requests from
    // upper-level caches, a Read will mark the block as shared but we
    // can satisfy a following ReadEx anyway since we can rely on the
    // Read requester(s) to have buffered the ReadEx snoop and to
    // invalidate their blocks after receiving them.
    // assert(!pkt->needsWritable() || blk->isWritable());
    /* MJL_Begin */
    // Packet's requested and Block data's direction should be the same, unless the requested size is less than a word sizeof(uint64_t)
    // In physically 2D cache case, the direction can be different as well 
    assert( (pkt->MJL_getCmdDir() == blk->MJL_blkDir) || (pkt->getSize() <= sizeof(uint64_t)) 
            || MJL_2DCache);
    // Test for L1D$
    if (this->name().find("dcache") != std::string::npos) {
       assert ((pkt->req->MJL_isVec()) || (pkt->getSize() <= sizeof(uint64_t)));
    }
    // MJL_TODO: a use case of getOffset, I think the direction of the cmd should be used, the requested size should not exceed the blksize anyway. And different direction should only happen when the size is smaller than a word.
    // To avoid that the assertion fail and setting the DataDir before actually having data
    CacheBlk::MJL_CacheBlkDir MJL_origDataDir = pkt->MJL_getDataDir();
    pkt->MJL_setDataDir(blk->MJL_blkDir);
    // if (pkt->isWrite() && pkt->getOffset(blkSize) + pkt->getSize() > blkSize) {
    //     std::cout << "MJL_AssertFailure: pkt->getOffset(blkSize) + pkt->getSize() <= blkSize, addr = ";
    //     std::cout << std::oct << pkt->getAddr();
    //     std::cout << std::dec  << ", dataDir = " << pkt->MJL_getDataDir() << ", cmdDir = " << pkt->MJL_getCmdDir() << ", Size = " << pkt->getSize() << "\n";
    // }
    assert(!pkt->isWrite() || pkt->getOffset(blkSize) + pkt->getSize() <= blkSize);
    pkt->MJL_setDataDir(MJL_origDataDir);
    /* MJL_End */
    /* MJL_Comment
    assert(pkt->getOffset(blkSize) + pkt->getSize() <= blkSize);
    */

    // Check RMW operations first since both isRead() and
    // isWrite() will be true for them
    if (pkt->cmd == MemCmd::SwapReq) {
        cmpAndSwap(blk, pkt);
    } else if (pkt->isWrite()) {
        // we have the block in a writable state and can go ahead,
        // note that the line may be also be considered writable in
        // downstream caches along the path to memory, but always
        // Exclusive, and never Modified
        assert(blk->isWritable());
        // Write or WriteLine at the first cache with block in writable state
        if (blk->checkWrite(pkt)) {
            /* MJL_Begin */
            if (MJL_2DCache && pkt->MJL_cmdIsColumn()) {
                // Write to intermediate data structure and then put into corresponding blocks on cross directional write;
                uint8_t MJL_tempData[blkSize];
                int MJL_offset = (pkt->MJL_getColOffset(blkSize)/sizeof(uint64_t))*sizeof(uint64_t);
                pkt->writeDataToBlock(MJL_tempData, blkSize);
                for (int i = pkt->MJL_getRowOffset(blkSize); i < pkt->MJL_getRowOffset(blkSize) + pkt->getSize(); i = i + (sizeof(uint64_t) - i%sizeof(uint64_t))) {
                    memcpy(tags->MJL_findBlockByTile(blk, i/sizeof(uint64_t))->data + MJL_offset + i%sizeof(uint64_t), &MJL_tempData[i * sizeof(uint64_t)], sizeof(uint64_t) - i%sizeof(uint64_t));
                }
            } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
                // Setting the direction to make sure that offset is calculated correctly. Maybe can also be used to collect the statistics on different directional hit?
                pkt->MJL_setDataDir(blk->MJL_blkDir);
                /* MJL_Test 
                std::cout << "MJL_writeToBlock: set " << blk->set << std::endl;
                    */
                pkt->writeDataToBlock(blk->data, blkSize);
            } else {
                pkt->writeDataToBlock(blk->data, blkSize);
            }
            /* MJL_End */
            /* MJL_Comment
            pkt->writeDataToBlock(blk->data, blkSize);
            */
        }
        // Always mark the line as dirty (and thus transition to the
        // Modified state) even if we are a failed StoreCond so we
        // supply data to any snoops that have appended themselves to
        // this cache before knowing the store will fail.
        /* MJL_Begin */
        // If this is a physically 2D cache and the access is column
        if (MJL_2DCache && pkt->MJL_cmdIsColumn()) {
            // Set the column word dirty for each row 
            for (int i = pkt->getOffset(blkSize); i < pkt->getOffset(blkSize) + pkt->getSize(); i = i + sizeof(uint64_t) - i%sizeof(uint64_t)) {
                tags->MJL_findBlockByTile(blk, i/sizeof(uint64_t))->MJL_setWordDirty(pkt->MJL_getColOffset(blkSize)/sizeof(uint64_t));
                tags->MJL_findBlockByTile(blk, i/sizeof(uint64_t))->status |= BlkDirty;
            }
        // If the access is row or this is a physically 1D, logically 2D cache
        } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
            for (int i = pkt->getOffset(blkSize); i < pkt->getOffset(blkSize) + pkt->getSize(); i = i + sizeof(uint64_t)) {
                blk->MJL_setWordDirty(i/sizeof(uint64_t));
            }
            blk->status |= BlkDirty;
        } else {
            blk->status |= BlkDirty;
        }
        /* MJL_End */
        /* MJL_Comment
        blk->status |= BlkDirty;
        */
        DPRINTF(CacheVerbose, "%s for %s (write)\n", __func__, pkt->print());
    } else if (pkt->isRead()) {
        if (pkt->isLLSC()) {
            blk->trackLoadLocked(pkt);
        }

        // all read responses have a data payload
        assert(pkt->hasRespData());
        /* MJL_Begin */
        // If this is a physically 2D cache and the access is column
        if (MJL_2DCache && pkt->MJL_cmdIsColumn()) {
            // In physically 2D cache, the cmd and the data direction should always align
            assert(pkt->MJL_sameCmdDataDir());
            // Construct the word dirty from the blocks in sets forming a column, and pass the information to pkt
            bool MJL_crossBlkWordDirty[blkSize/sizeof(uint64_t)];
            for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                MJL_crossBlkWordDirty[i] = false;
            }
            for (int i = pkt->getOffset(blkSize); i < pkt->getOffset(blkSize) + pkt->getSize(); i = i + sizeof(uint64_t) - i%sizeof(uint64_t)) {
                MJL_crossBlkWordDirty[i/sizeof(uint64_t)] = tags->MJL_findBlockByTile(blk, i/sizeof(uint64_t))->MJL_wordDirty[pkt->MJL_getColOffset(blkSize)/sizeof(uint64_t)];
            }
            pkt->MJL_copyWordDirty(MJL_crossBlkWordDirty);
        // If the access is row or this is a physically 1D, logically 2D cache
        } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
            pkt->MJL_setDataDir(blk->MJL_blkDir);
            pkt->MJL_setWordDirtyFromBlk(blk->MJL_wordDirty, blkSize);
        }
        /* MJL_Test 
        std::cout << "MJL_setFromBlock: set " << blk->set << std::endl;
         */
        // If this is a physically 2D cache and the access is column, gather data from blocks into an intermediate structure and then copy to packet
        if (MJL_2DCache && pkt->MJL_cmdIsColumn()) {
            // MJL_Test: To see if the cross direction line/column access is correct.
            std::cout << "MJL_Test: column physical 2D cache read. Packet info: Addr(oct) " << std::oct << pkt->getAddr() << std::dec << ", Size " << pkt->getSize() << std::endl;
            uint8_t MJL_tempData[blkSize];
            int MJL_offset = (pkt->MJL_getColOffset(blkSize)/sizeof(uint64_t))*sizeof(uint64_t);
            
            for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                // MJL_Test: See if this gets the rows correctly
                std::cout << "MJL_Test: write Set(oct) " << std::oct << tags->MJL_findBlockByTile(blk, i)->set << std::dec << ", Way " << tags->MJL_findBlockByTile(blk, i)->way << ", Tag(oct) " << std::oct << tags->MJL_findBlockByTile(blk, i)->tag << std::dec << std::endl;
                // MJL_TODO: Verify if this will get the correct blk in the consecutive sets.
                memcpy(&MJL_tempData[i * sizeof(uint64_t)], tags->MJL_findBlockByTile(blk, i)->data + MJL_offset, sizeof(uint64_t));
            }
            pkt->setDataFromBlock(MJL_tempData, blkSize);
        } else {
            pkt->setDataFromBlock(blk->data, blkSize);
        }
        /* MJL_End */
        /* MJL_Comment
        pkt->setDataFromBlock(blk->data, blkSize);
        */

        // MJL_TODO: coherence things
        // determine if this read is from a (coherent) cache or not
        if (pkt->fromCache()) {
            assert(pkt->getSize() == blkSize);
            // special handling for coherent block requests from
            // upper-level caches
            if (pkt->needsWritable()) {
                // sanity check
                assert(pkt->cmd == MemCmd::ReadExReq ||
                       pkt->cmd == MemCmd::SCUpgradeFailReq);
                assert(!pkt->hasSharers());

                // if we have a dirty copy, make sure the recipient
                // keeps it marked dirty (in the modified state)
                if (blk->isDirty()) {
                    pkt->setCacheResponding();
                    /* MJL_Begin */
                    if (MJL_2DCache && pkt->MJL_cmdIsColumn()) {
                        for (int i = pkt->getOffset(blkSize); i < pkt->getOffset(blkSize) + pkt->getSize(); i = i + sizeof(uint64_t) - i%sizeof(uint64_t)) {
                            tags->MJL_findBlockByTile(blk, i/sizeof(uint64_t))->MJL_clearWordDirty(pkt->MJL_getColOffset(blkSize)/sizeof(uint64_t));
                            tags->MJL_findBlockByTile(blk, i/sizeof(uint64_t))->MJL_updateDirty();
                        }
                    } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
                        blk->MJL_clearAllDirty();
                        blk->status &= ~BlkDirty;
                    } else {
                        blk->status &= ~BlkDirty;
                    }
                    /* MJL_End */
                    /* MJL_Comment
                    blk->status &= ~BlkDirty;
                    */
                }
            } else if (blk->isWritable() && !pending_downgrade &&
                       !pkt->hasSharers() &&
                       pkt->cmd != MemCmd::ReadCleanReq) {
                // we can give the requester a writable copy on a read
                // request if:
                // - we have a writable copy at this level (& below)
                // - we don't have a pending snoop from below
                //   signaling another read request
                // - no other cache above has a copy (otherwise it
                //   would have set hasSharers flag when
                //   snooping the packet)
                // - the read has explicitly asked for a clean
                //   copy of the line
                if (blk->isDirty()) {
                    // special considerations if we're owner:
                    if (!deferred_response) {
                        // respond with the line in Modified state
                        // (cacheResponding set, hasSharers not set)
                        pkt->setCacheResponding();

                        // if this cache is mostly inclusive, we
                        // keep the block in the Exclusive state,
                        // and pass it upwards as Modified
                        // (writable and dirty), hence we have
                        // multiple caches, all on the same path
                        // towards memory, all considering the
                        // same block writable, but only one
                        // considering it Modified

                        // we get away with multiple caches (on
                        // the same path to memory) considering
                        // the block writeable as we always enter
                        // the cache hierarchy through a cache,
                        // and first snoop upwards in all other
                        // branches
                        /* MJL_Begin */
                        if (MJL_2DCache && pkt->MJL_cmdIsColumn()) {
                            for (int i = pkt->getOffset(blkSize); i < pkt->getOffset(blkSize) + pkt->getSize(); i = i + sizeof(uint64_t) - i%sizeof(uint64_t)) {
                                // MJL_TODO: Verify if this will get the correct blk in the consecutive sets.
                                tags->MJL_findBlockByTile(blk, i/sizeof(uint64_t))->MJL_clearWordDirty(pkt->MJL_getColOffset(blkSize)/sizeof(uint64_t));
                                tags->MJL_findBlockByTile(blk, i/sizeof(uint64_t))->MJL_updateDirty();
                            }
                        } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
                            blk->MJL_clearAllDirty();
                            blk->status &= ~BlkDirty;
                        } else {
                            blk->status &= ~BlkDirty;
                        }
                        /* MJL_End */
                        /* MJL_Comment
                        blk->status &= ~BlkDirty;
                        */
                    } else {
                        // if we're responding after our own miss,
                        // there's a window where the recipient didn't
                        // know it was getting ownership and may not
                        // have responded to snoops correctly, so we
                        // have to respond with a shared line
                        pkt->setHasSharers();
                    }
                }
            } else {
                // otherwise only respond with a shared copy
                pkt->setHasSharers();
            }
        }
    } else if (pkt->isUpgrade()) {
        // sanity check
        // MJL_TODO: would need to comment this if cross directional block existence also mark sharers (cannot respond even if it is a sharer). Or is it that they should have invalidated theirs anyway?
        assert(!pkt->hasSharers());

        if (blk->isDirty()) {
            // we were in the Owned state, and a cache above us that
            // has the line in Shared state needs to be made aware
            // that the data it already has is in fact dirty
            pkt->setCacheResponding();
            /* MJL_Begin */
            if (MJL_2DCache && pkt->MJL_cmdIsColumn()) {
                bool MJL_tempWordDirty[blkSize/sizeof(uint64_t)];
                for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                    MJL_tempWordDirty[i] = false;
                }
                for (int i = pkt->getOffset(blkSize); i < pkt->getOffset(blkSize) + pkt->getSize(); i = i + sizeof(uint64_t) - i%sizeof(uint64_t)) {
                    MJL_tempWordDirty[i/sizeof(uint64_t)] = tags->MJL_findBlockByTile(blk, i/sizeof(uint64_t))->MJL_wordDirty[pkt->MJL_getColOffset(blkSize)/sizeof(uint64_t)];
                    // MJL_TODO: Verify if this will get the correct blk in the consecutive sets.
                    tags->MJL_findBlockByTile(blk, i/sizeof(uint64_t))->MJL_clearWordDirty(pkt->MJL_getColOffset(blkSize)/sizeof(uint64_t));
                    tags->MJL_findBlockByTile(blk, i/sizeof(uint64_t))->MJL_updateDirty();
                }
                pkt->MJL_setWordDirtyFromBlk(MJL_tempWordDirty, blkSize);
            } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
                pkt->MJL_setWordDirtyFromBlk(blk->MJL_wordDirty, blkSize);
                blk->MJL_clearAllDirty();
                blk->status &= ~BlkDirty;
            } else {
                blk->status &= ~BlkDirty;
            }
            /* MJL_End */
            /* MJL_Comment
            blk->status &= ~BlkDirty;
            */
        }
    } else {
        assert(pkt->isInvalidate());
        /* MJL_Begin */
        if (MJL_2DCache) {
            MJL_invalidateTile(blk);
        } else {
            invalidateBlock(blk);
        }
        /* MJL_End */
        /* MJL_Comment
        invalidateBlock(blk);
        */
        DPRINTF(CacheVerbose, "%s for %s (invalidation)\n", __func__,
                pkt->print());
    }
}

/////////////////////////////////////////////////////
//
// Access path: requests coming in from the CPU side
//
/////////////////////////////////////////////////////

bool
Cache::access(PacketPtr pkt, CacheBlk *&blk, Cycles &lat,
              PacketList &writebacks)
{
    // sanity check
    assert(pkt->isRequest());

    chatty_assert(!(isReadOnly && pkt->isWrite()),
                  "Should never see a write in a read-only cache %s\n",
                  name());

    DPRINTF(CacheVerbose, "%s for %s\n", __func__, pkt->print());

    if (pkt->req->isUncacheable()) {
        DPRINTF(Cache, "uncacheable: %s\n", pkt->print());

        // flush and invalidate any existing block
        /* MJL_Begin */
        CacheBlk *old_blk(tags->MJL_findBlock(pkt->getAddr(), pkt->MJL_getCmdDir(), pkt->isSecure()));
        /* MJL_End */
        /* MJL_Comment
        CacheBlk *old_blk(tags->findBlock(pkt->getAddr(), pkt->isSecure()));
        */
        if (old_blk && old_blk->isValid()) {
            if (old_blk->isDirty() || writebackClean)
                writebacks.push_back(writebackBlk(old_blk));
            else
                writebacks.push_back(cleanEvictBlk(old_blk));
            tags->invalidate(old_blk);
            old_blk->invalidate();
        }

        blk = nullptr;
        // lookupLatency is the latency in case the request is uncacheable.
        lat = lookupLatency;
        return false;
    }

    ContextID id = pkt->req->hasContextId() ?
        pkt->req->contextId() : InvalidContextID;
    // Here lat is the value passed as parameter to accessBlock() function
    // that can modify its value.
    /* MJL_Begin */
    if ( pkt->getSize() <= sizeof(uint64_t) && (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos)) { // Less than a word, cross direction possible
        blk = tags->MJL_accessBlockOneWord(pkt->getAddr(), pkt->MJL_getCmdDir(), pkt->isSecure(), lat, id);
    } else if (MJL_2DCache && pkt->MJL_cmdIsColumn()) {
        blk = tags->MJL_accessCrossBlock(pkt->getAddr(), CacheBlk::MJL_CacheBlkDir::MJL_IsRow, pkt->isSecure(), lat, id, pkt->MJL_getColOffset(blkSize));
    } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
        blk = tags->MJL_accessBlock(pkt->getAddr(), pkt->MJL_getCmdDir(), pkt->isSecure(), lat, id);
    } else {
        blk = tags->accessBlock(pkt->getAddr(), pkt->isSecure(), lat, id);
    }
    // MJL_TODO: may need to change lat if vector load/store is not possible
    /* MJL_End */
    /* MJL_Comment
    blk = tags->accessBlock(pkt->getAddr(), pkt->isSecure(), lat, id);
    */

    DPRINTF(Cache, "%s %s\n", pkt->print(),
            blk ? "hit " + blk->print() : "miss");


    if (pkt->isEviction()) {
        // We check for presence of block in above caches before issuing
        // Writeback or CleanEvict to write buffer. Therefore the only
        // possible cases can be of a CleanEvict packet coming from above
        // encountering a Writeback generated in this cache peer cache and
        // waiting in the write buffer. Cases of upper level peer caches
        // generating CleanEvict and Writeback or simply CleanEvict and
        // CleanEvict almost simultaneously will be caught by snoops sent out
        // by crossbar.
        /* MJL_Begin */
        // MJL_TODO: Eviction should be from cache, so should be cacheline size requests. Writeback should be in order. Should we check the other direction as well for clean eviction?
        WriteQueueEntry *wb_entry = nullptr;
        if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
            wb_entry = writeBuffer.MJL_findMatch(pkt->getAddr(),
                                                                pkt->MJL_getCmdDir(),
                                                                pkt->isSecure());
        } else {
            wb_entry = writeBuffer.findMatch(pkt->getAddr(),
                                                          pkt->isSecure());
        }
        /* MJL_End */
        /* MJL_Comment
        WriteQueueEntry *wb_entry = writeBuffer.findMatch(pkt->getAddr(),
                                                          pkt->isSecure());
        */
        if (wb_entry) {
            assert(wb_entry->getNumTargets() == 1);
            PacketPtr wbPkt = wb_entry->getTarget()->pkt;
            assert(wbPkt->isWriteback());

            if (pkt->isCleanEviction()) {
                // The CleanEvict and WritebackClean snoops into other
                // peer caches of the same level while traversing the
                // crossbar. If a copy of the block is found, the
                // packet is deleted in the crossbar. Hence, none of
                // the other upper level caches connected to this
                // cache have the block, so we can clear the
                // BLOCK_CACHED flag in the Writeback if set and
                // discard the CleanEvict by returning true.
                wbPkt->clearBlockCached();
                return true;
            } else {
                assert(pkt->cmd == MemCmd::WritebackDirty);
                // Dirty writeback from above trumps our clean
                // writeback... discard here
                // Note: markInService will remove entry from writeback buffer.
                markInService(wb_entry);
                delete wbPkt;
            }
        }
    }

    // Writeback handling is special case.  We can write the block into
    // the cache without having a writeable copy (or any copy at all).
    if (pkt->isWriteback()) {
        assert(blkSize == pkt->getSize());

        // we could get a clean writeback while we are having
        // outstanding accesses to a block, do the simple thing for
        // now and drop the clean writeback so that we do not upset
        // any ordering/decisions about ownership already taken
        if (pkt->cmd == MemCmd::WritebackClean &&
        /* MJL_Begin */
            (((this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) 
                && mshrQueue.MJL_findMatch(pkt->getAddr(), pkt->MJL_getCmdDir(), pkt->isSecure())) 
            || ((this->name().find("dcache") == std::string::npos && this->name().find("l2") == std::string::npos) 
                && mshrQueue.findMatch(pkt->getAddr(), pkt->isSecure()))) ) {
        /* MJL_End */
        /* MJL_Comment
            mshrQueue.findMatch(pkt->getAddr(), pkt->isSecure())) {
        */
            DPRINTF(Cache, "Clean writeback %#llx to block with MSHR, "
                    "dropping\n", pkt->getAddr());
            return true;
        }

        /* MJL_Begin */
        // For physically 2D Cache, we should check if the tile exists, and if it does, the blk can be used directly for write back and should not do a replacement.
        if (MJL_2DCache && blk == nullptr) {
            // Set the direction to get the cross direction offset
            int MJL_offset = pkt->getOffset(blkSize);
            if (pkt->MJL_cmdIsRow()) {
                MJL_offset = pkt->MJL_getRowOffset(blkSize);
            } else if (pkt->MJL_cmdIsColumn()) {
                MJL_offset = pkt->MJL_getColOffset(blkSize);
            }
            blk = tags->MJL_findWritebackBlk(pkt->getAddr(), pkt->MJL_getCmdDir(), pkt->isSecure(), MJL_offset);
        }
        /* MJL_End */

        if (blk == nullptr) {
            // need to do a replacement
            /* MJL_Begin */
            if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
                assert(pkt->MJL_getCmdDir() == pkt->MJL_getDataDir());
                blk = MJL_allocateBlock(pkt->getAddr(), pkt->MJL_getDataDir(), pkt->isSecure(), writebacks);
            } else {
                blk = allocateBlock(pkt->getAddr(), pkt->isSecure(), writebacks);
            }
            /* MJL_End */
            /* MJL_Comment
            blk = allocateBlock(pkt->getAddr(), pkt->isSecure(), writebacks);
            */
            if (blk == nullptr) {
                // no replaceable block available: give up, fwd to next level.
                incMissCount(pkt);
                /* MJL_Begin */
                if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
                    if (pkt->MJL_cmdIsRow()) {
                        MJL_overallRowMisses++;
                        if (pkt->req->hasPC() && MJL_VecListSet.find(pkt->req->getPC()) != MJL_VecListSet.end()) {
                            MJL_overallRowVecMisses++;
                        }
                    } else if (pkt->MJL_cmdIsColumn()) {
                        MJL_overallColumnMisses++;
                        if (pkt->req->hasPC() && MJL_VecListSet.find(pkt->req->getPC()) != MJL_VecListSet.end()) {
                            MJL_overallColVecMisses++;
                        }
                    }
                }
                /* MJL_End */
                return false;
            }

            tags->insertBlock(pkt, blk);
            /* MJL_Begin */
            // Physically 2D Cache should handle insert column block and the block status changes.
            if (MJL_2DCache) {
                bool MJL_readable = true;
                bool MJL_secure = pkt->isSecure();
                for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                    CacheBlk* tile_blk = tags->MJL_findBlockByTile(blk, i);
                    if ((tile_blk->isValid() || tile_blk->MJL_hasCrossValid()) && !tile_blk->isReadable()) {
                        MJL_readable = false;
                    }
                    if ((tile_blk->isValid() || tile_blk->MJL_hasCrossValid()) && MJL_secure != tile_blk->isSecure()) {
                        MJL_secure = tile_blk->isSecure();
                    }
                }
                for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                    CacheBlk* tile_blk = tags->MJL_findBlockByTile(blk, i);
                    if (MJL_readable) {
                        tile_blk->status |= BlkReadable;
                    } else {
                        tile_blk->status &= ~BlkReadable;
                    }
                    assert(pkt->isSecure() == MJL_secure);
                    if (pkt->isSecure()) {
                        tile_blk->status |= BlkSecure;
                    }
                }
                if (pkt->MJL_cmdIsRow()) {
                    blk->status |= BlkValid;
                } else if (pkt->MJL_cmdIsColumn()) {
                    int MJL_offset = pkt->MJL_getColOffset(blkSize);
                    for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                        tags->MJL_findBlockByTile(blk, i)->MJL_crossValid[MJL_offset/sizeof(uint64_t)] = true;
                    }
                }
            } else {

                blk->status = (BlkValid | BlkReadable);
                if (pkt->isSecure()) {
                    blk->status |= BlkSecure;
                }
            }
            /* MJL_End */
            /* MJL_Comment
    
            blk->status = (BlkValid | BlkReadable);
            if (pkt->isSecure()) {
                blk->status |= BlkSecure;
            }
            */
        }
        // only mark the block dirty if we got a writeback command,
        // and leave it as is for a clean writeback
        if (pkt->cmd == MemCmd::WritebackDirty) {
            /* MJL_Begin */
            // No need for invalidation for physically 2D Cache, but the setting dirty status should change
            if (MJL_2DCache) {
                if (pkt->MJL_cmdIsColumn()) {
                    unsigned MJL_offset = pkt->MJL_getColOffset(blkSize);
                    for (unsigned i = 0; i < pkt->getSize()/sizeof(uint64_t); ++i) { // For writeback, pkt->getSize() == blkSize
                        CacheBlk* tile_blk = tags->MJL_findBlockByTile(blk, i);
                        tile_blk->MJL_setWordDirty(MJL_offset/sizeof(uint64_t));
                        tile_blk->MJL_updateDirty();
                    }
                } else {
                    blk->MJL_setWordDirtyPkt(pkt, blkSize);
                }
            } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
                assert (pkt->MJL_getCmdDir() == blk->MJL_blkDir);
                // Taking the additional tag check latency into account
                for (unsigned offset = 0; offset < pkt->getSize(); offset = offset + sizeof(uint64_t)) {
                    if (pkt->MJL_wordDirty[offset/sizeof(uint64_t)]) {
                        lat += lookupLatency;
                    }
                }
                MJL_invalidateOtherBlocks(pkt->getAddr(), blk->MJL_blkDir, pkt->getSize(), pkt->isSecure(), writebacks, pkt->MJL_wordDirty);
                //blk->MJL_clearAllDirty();
                blk->MJL_setWordDirtyPkt(pkt, blkSize);
            }
            /* MJL_End */
            blk->status |= BlkDirty;
        }
        // if the packet does not have sharers, it is passing
        // writable, and we got the writeback in Modified or Exclusive
        // state, if not we are in the Owned or Shared state
        /* MJL_Begin */
        if (MJL_2DCache) {
            // In physically 2D cache, Writable should propagate to other blocks of the same tile as well
            // MJL_TODO: although without other cores, there should be no sharers so this should work, but actually should change to check tile wise when the has sharers is set.
            if (!pkt->hasSharers()) {
                for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                    CacheBlk* tile_blk = tags->MJL_findBlockByTile(blk, i);
                    tile_blk->status |= BlkWritable;
                }
            }
        } else {
            if (!pkt->hasSharers()) {
                blk->status |= BlkWritable;
            }
        }
        /* MJL_End */
        /* MJL_Comment
        if (!pkt->hasSharers()) {
            blk->status |= BlkWritable;
        }
        */
        // nothing else to do; writeback doesn't expect response
        assert(!pkt->needsResponse());
        /* MJL_Begin */
        // MJL_TODO: The data copy should be changed for physically 2D cache column access
        if (MJL_2DCache && pkt->MJL_cmdIsColumn()) {
            unsigned MJL_offset = pkt->MJL_getColOffset(blkSize);
            for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) { // For writeback, pkt->getSize() == blkSize
                std::memcpy(tags->MJL_findBlockByTile(blk, i)->data + MJL_offset, pkt->getConstPtr<uint8_t>() + i * sizeof(uint64_t), sizeof(uint64_t));
            }
        } else {
            std::memcpy(blk->data, pkt->getConstPtr<uint8_t>(), blkSize);
        }
        /* MJL_End */
        /* MJL_Comment
        std::memcpy(blk->data, pkt->getConstPtr<uint8_t>(), blkSize);
        */
        DPRINTF(Cache, "%s new state is %s\n", __func__, blk->print());
        incHitCount(pkt);
        /* MJL_Begin */
        if (pkt->MJL_cmdIsRow()) {
            MJL_overallRowHits++;
            if (pkt->req->hasPC() && MJL_VecListSet.find(pkt->req->getPC()) != MJL_VecListSet.end()) {
                MJL_overallRowVecHits++;
            }
        } else if (pkt->MJL_cmdIsColumn()) {
            MJL_overallColumnHits++;
            if (pkt->req->hasPC() && MJL_VecListSet.find(pkt->req->getPC()) != MJL_VecListSet.end()) {
                MJL_overallColVecHits++;
            }
        }
        /* MJL_End */
        return true;
    } else if (pkt->cmd == MemCmd::CleanEvict) {
        if (blk != nullptr) {
            // Found the block in the tags, need to stop CleanEvict from
            // propagating further down the hierarchy. Returning true will
            // treat the CleanEvict like a satisfied write request and delete
            // it.
            return true;
        }
        // We didn't find the block here, propagate the CleanEvict further
        // down the memory hierarchy. Returning false will treat the CleanEvict
        // like a Writeback which could not find a replaceable block so has to
        // go to next level.
        return false;
    } else if (blk && (pkt->needsWritable() ? blk->isWritable() :
                       blk->isReadable())) {
        // OK to satisfy access
        incHitCount(pkt);
        /* MJL_Begin */
        if (pkt->MJL_cmdIsRow()) {
            MJL_overallRowHits++;
            if (pkt->req->hasPC() && MJL_VecListSet.find(pkt->req->getPC()) != MJL_VecListSet.end()) {
                MJL_overallRowVecHits++;
            }
        } else if (pkt->MJL_cmdIsColumn()) {
            MJL_overallColumnHits++;
            if (pkt->req->hasPC() && MJL_VecListSet.find(pkt->req->getPC()) != MJL_VecListSet.end()) {
                MJL_overallColVecHits++;
            }
        }

        if (pkt->isWrite() && (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) && !MJL_2DCache ) {
            // Taking the additional tag check into account
            for (unsigned offset = 0; offset < pkt->getSize(); offset = offset + sizeof(uint64_t)) {
                if (pkt->MJL_wordDirty[offset/sizeof(uint64_t)]) {
                    lat += lookupLatency;
                }
            }
            MJL_invalidateOtherBlocks(pkt->getAddr(), blk->MJL_blkDir, pkt->getSize(), pkt->isSecure(), writebacks, pkt->MJL_wordDirty);
        }
        /* MJL_End */
        satisfyRequest(pkt, blk);
        maintainClusivity(pkt->fromCache(), blk);

        return true;
    }

    // Can't satisfy access normally... either no block (blk == nullptr)
    // or have block but need writable

    incMissCount(pkt);
    /* MJL_Begin */
    if (pkt->MJL_cmdIsRow()) {
        MJL_overallRowMisses++;
        if (pkt->req->hasPC() && MJL_VecListSet.find(pkt->req->getPC()) != MJL_VecListSet.end()) {
            MJL_overallRowVecMisses++;
        }
    } else if (pkt->MJL_cmdIsColumn()) {
        MJL_overallColumnMisses++;
        if (pkt->req->hasPC() && MJL_VecListSet.find(pkt->req->getPC()) != MJL_VecListSet.end()) {
            MJL_overallColVecMisses++;
        }
    }
    /* MJL_End */

    if (blk == nullptr && pkt->isLLSC() && pkt->isWrite()) {
        // complete miss on store conditional... just give up now
        pkt->req->setExtraData(0);
        return true;
    }
    /* MJL_Begin */
    // We are going to bring in a cache line, crossing lines with dirty data at the crossing needs to be written back
    // And if the access were a write, then the crossing lines to the write section needs to be invalidated as well
    // If the cache is physically 2D, then there's no need for all this
    if (blk == nullptr && (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) && !MJL_2DCache ) {
        CacheBlk *MJL_crossBlk = nullptr;
        Addr MJL_crossBlkAddr;
        Cycles templat = lat;
        for (unsigned MJL_offset = 0; MJL_offset < blkSize; MJL_offset = MJL_offset + sizeof(uint64_t)) {
            // Get the address of each word in the cache line
            MJL_crossBlkAddr = MJL_addOffsetAddr(MJL_blockAlign(pkt->getAddr(), pkt->MJL_getCmdDir()), pkt->MJL_getCmdDir(), MJL_offset);
            Addr MJL_crossBlkOffset = MJL_offset;
            // Search for the crossing cache line 
            if ( pkt->MJL_getCmdDir() == CacheBlk::MJL_CacheBlkDir::MJL_IsRow ) {
                MJL_crossBlk = tags->MJL_accessBlock(MJL_crossBlkAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn, pkt->isSecure(), lat, id);
                MJL_crossBlkOffset = tags->MJL_swapRowColBits(MJL_crossBlkAddr) & Addr(blkSize - 1);
            } else if ( pkt->MJL_getCmdDir() == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn ) {
                MJL_crossBlk = tags->MJL_accessBlock(MJL_crossBlkAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow, pkt->isSecure(), lat, id);
                MJL_crossBlkOffset = MJL_crossBlkAddr & Addr(blkSize - 1);
            }
            templat = templat + lat;
            // If the crossing line exists
            if (MJL_crossBlk && MJL_crossBlk->isValid()) {
                // Invalidate for the written section of the write request
                if (pkt->isWrite() && (MJL_offset >= pkt->getOffset(blkSize) && MJL_offset < pkt->getOffset(blkSize) + pkt->getSize())) {
                    MJL_conflictWBCount1++;
                    if (MJL_crossBlk->isDirty() || writebackClean) {
                        writebacks.push_back(writebackBlk(MJL_crossBlk));
                    } else {
                        writebacks.push_back(cleanEvictBlk(MJL_crossBlk));
                    }
                    invalidateBlock(MJL_crossBlk);
                // Write back for read requests and non written sections of the write request if the crossing word is dirty
                } else if (MJL_crossBlk->isDirty() && MJL_crossBlk->MJL_wordDirty[MJL_crossBlkOffset/sizeof(uint64_t)]){
                    MJL_conflictWBCount2++;
                    writebacks.push_back(MJL_writebackCachedBlk(MJL_crossBlk));
                // Otherwise, just revoke writable
                } else {
                    MJL_crossBlk->status &= ~BlkWritable;
                }
            }
        }
        lat = templat;
    }
    /* MJL_End */

    return false;
}

void
Cache::maintainClusivity(bool from_cache, CacheBlk *blk)
{
    if (from_cache && blk && blk->isValid() && !blk->isDirty() &&
        clusivity == Enums::mostly_excl) {
        // if we have responded to a cache, and our block is still
        // valid, but not dirty, and this cache is mostly exclusive
        // with respect to the cache above, drop the block
        invalidateBlock(blk);
    }
}

void
Cache::doWritebacks(PacketList& writebacks, Tick forward_time)
{
    while (!writebacks.empty()) {
        PacketPtr wbPkt = writebacks.front();
        // We use forwardLatency here because we are copying writebacks to
        // write buffer.  Call isCachedAbove for both Writebacks and
        // CleanEvicts. If isCachedAbove returns true we set BLOCK_CACHED flag
        // in Writebacks and discard CleanEvicts.
        if (isCachedAbove(wbPkt)) {
            if (wbPkt->cmd == MemCmd::CleanEvict) {
                // Delete CleanEvict because cached copies exist above. The
                // packet destructor will delete the request object because
                // this is a non-snoop request packet which does not require a
                // response.
                delete wbPkt;
            } else if (wbPkt->cmd == MemCmd::WritebackClean) {
                // clean writeback, do not send since the block is
                // still cached above
                assert(writebackClean);
                delete wbPkt;
            } else {
                assert(wbPkt->cmd == MemCmd::WritebackDirty);
                // Set BLOCK_CACHED flag in Writeback and send below, so that
                // the Writeback does not reset the bit corresponding to this
                // address in the snoop filter below.
                wbPkt->setBlockCached();
                allocateWriteBuffer(wbPkt, forward_time);
            }
        } else {
            // If the block is not cached above, send packet below. Both
            // CleanEvict and Writeback with BLOCK_CACHED flag cleared will
            // reset the bit corresponding to this address in the snoop filter
            // below.
            allocateWriteBuffer(wbPkt, forward_time);
        }
        writebacks.pop_front();
    }
}

void
Cache::doWritebacksAtomic(PacketList& writebacks)
{
    while (!writebacks.empty()) {
        PacketPtr wbPkt = writebacks.front();
        // Call isCachedAbove for both Writebacks and CleanEvicts. If
        // isCachedAbove returns true we set BLOCK_CACHED flag in Writebacks
        // and discard CleanEvicts.
        if (isCachedAbove(wbPkt, false)) {
            if (wbPkt->cmd == MemCmd::WritebackDirty) {
                // Set BLOCK_CACHED flag in Writeback and send below,
                // so that the Writeback does not reset the bit
                // corresponding to this address in the snoop filter
                // below. We can discard CleanEvicts because cached
                // copies exist above. Atomic mode isCachedAbove
                // modifies packet to set BLOCK_CACHED flag
                memSidePort->sendAtomic(wbPkt);
            }
        } else {
            // If the block is not cached above, send packet below. Both
            // CleanEvict and Writeback with BLOCK_CACHED flag cleared will
            // reset the bit corresponding to this address in the snoop filter
            // below.
            memSidePort->sendAtomic(wbPkt);
        }
        writebacks.pop_front();
        // In case of CleanEvicts, the packet destructor will delete the
        // request object because this is a non-snoop request packet which
        // does not require a response.
        delete wbPkt;
    }
}


void
Cache::recvTimingSnoopResp(PacketPtr pkt)
{
    DPRINTF(Cache, "%s for %s\n", __func__, pkt->print());

    assert(pkt->isResponse());
    assert(!system->bypassCaches());

    // determine if the response is from a snoop request we created
    // (in which case it should be in the outstandingSnoop), or if we
    // merely forwarded someone else's snoop request
    const bool forwardAsSnoop = outstandingSnoop.find(pkt->req) ==
        outstandingSnoop.end();

    if (!forwardAsSnoop) {
        // the packet came from this cache, so sink it here and do not
        // forward it
        assert(pkt->cmd == MemCmd::HardPFResp);

        outstandingSnoop.erase(pkt->req);

        DPRINTF(Cache, "Got prefetch response from above for addr "
                "%#llx (%s)\n", pkt->getAddr(), pkt->isSecure() ? "s" : "ns");
        recvTimingResp(pkt);
        return;
    }

    // forwardLatency is set here because there is a response from an
    // upper level cache.
    // To pay the delay that occurs if the packet comes from the bus,
    // we charge also headerDelay.
    Tick snoop_resp_time = clockEdge(forwardLatency) + pkt->headerDelay;
    // Reset the timing of the packet.
    pkt->headerDelay = pkt->payloadDelay = 0;
    memSidePort->schedTimingSnoopResp(pkt, snoop_resp_time);
}

void
Cache::promoteWholeLineWrites(PacketPtr pkt)
{
    // Cache line clearing instructions
    /* MJL_Begin */
    if (pkt->cmd == MemCmd::WriteReq && (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos)) {
        // For writes, dataDir should be the same as cmdDir, and getOffset() works on MJL_dataDir
        assert(pkt->MJL_sameCmdDataDir());
    }
    /* MJL_End */
    if (doFastWrites && (pkt->cmd == MemCmd::WriteReq) &&
        (pkt->getSize() == blkSize) && (pkt->getOffset(blkSize) == 0)) {
        pkt->cmd = MemCmd::WriteLineReq;
        DPRINTF(Cache, "packet promoted from Write to WriteLineReq\n");
    }
}

bool
Cache::recvTimingReq(PacketPtr pkt)
{
    DPRINTF(CacheTags, "%s tags: %s\n", __func__, tags->print());

    assert(pkt->isRequest());

    // Just forward the packet if caches are disabled.
    if (system->bypassCaches()) {
        // @todo This should really enqueue the packet rather
        bool M5_VAR_USED success = memSidePort->sendTimingReq(pkt);
        assert(success);
        return true;
    }

    promoteWholeLineWrites(pkt);

    if (pkt->cacheResponding()) {
        // a cache above us (but not where the packet came from) is
        // responding to the request, in other words it has the line
        // in Modified or Owned state
        DPRINTF(Cache, "Cache above responding to %s: not responding\n",
                pkt->print());

        // if the packet needs the block to be writable, and the cache
        // that has promised to respond (setting the cache responding
        // flag) is not providing writable (it is in Owned rather than
        // the Modified state), we know that there may be other Shared
        // copies in the system; go out and invalidate them all
        assert(pkt->needsWritable() && !pkt->responderHadWritable());

        // an upstream cache that had the line in Owned state
        // (dirty, but not writable), is responding and thus
        // transferring the dirty line from one branch of the
        // cache hierarchy to another

        // send out an express snoop and invalidate all other
        // copies (snooping a packet that needs writable is the
        // same as an invalidation), thus turning the Owned line
        // into a Modified line, note that we don't invalidate the
        // block in the current cache or any other cache on the
        // path to memory

        // create a downstream express snoop with cleared packet
        // flags, there is no need to allocate any data as the
        // packet is merely used to co-ordinate state transitions
        Packet *snoop_pkt = new Packet(pkt, true, false);

        // also reset the bus time that the original packet has
        // not yet paid for
        snoop_pkt->headerDelay = snoop_pkt->payloadDelay = 0;

        // make this an instantaneous express snoop, and let the
        // other caches in the system know that the another cache
        // is responding, because we have found the authorative
        // copy (Modified or Owned) that will supply the right
        // data
        snoop_pkt->setExpressSnoop();
        snoop_pkt->setCacheResponding();

        // this express snoop travels towards the memory, and at
        // every crossbar it is snooped upwards thus reaching
        // every cache in the system
        bool M5_VAR_USED success = memSidePort->sendTimingReq(snoop_pkt);
        // express snoops always succeed
        assert(success);

        // main memory will delete the snoop packet

        // queue for deletion, as opposed to immediate deletion, as
        // the sending cache is still relying on the packet
        pendingDelete.reset(pkt);

        // no need to take any further action in this particular cache
        // as an upstram cache has already committed to responding,
        // and we have already sent out any express snoops in the
        // section above to ensure all other copies in the system are
        // invalidated
        return true;
    }

    // anything that is merely forwarded pays for the forward latency and
    // the delay provided by the crossbar
    Tick forward_time = clockEdge(forwardLatency) + pkt->headerDelay;

    // We use lookupLatency here because it is used to specify the latency
    // to access.
    Cycles lat = lookupLatency;
    CacheBlk *blk = nullptr;
    bool satisfied = false;
    {
        PacketList writebacks;
        // Note that lat is passed by reference here. The function
        // access() calls accessBlock() which can modify lat value.
        satisfied = access(pkt, blk, lat, writebacks);
        /* MJL_Begin */
        // Add the additional tag check latency for misses
        if (!MJL_2DCache && (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) && 
            ((!satisfied && !pkt->req->isUncacheable() && pkt->cmd != MemCmd::CleanEvict && !pkt->isWriteback())
            || (satisfied && pkt->cmd == MemCmd::WritebackDirty))) {
            forward_time = clockEdge(Cycles( blkSize/sizeof(uint64_t) * forwardLatency)) + pkt->headerDelay;
        }
        /* MJL_End */

        // copy writebacks to write buffer here to ensure they logically
        // proceed anything happening below
        doWritebacks(writebacks, forward_time);
    }

    // Here we charge the headerDelay that takes into account the latencies
    // of the bus, if the packet comes from it.
    // The latency charged it is just lat that is the value of lookupLatency
    // modified by access() function, or if not just lookupLatency.
    // In case of a hit we are neglecting response latency.
    // In case of a miss we are neglecting forward latency.
    Tick request_time = clockEdge(lat) + pkt->headerDelay;
    // Here we reset the timing of the packet.
    pkt->headerDelay = pkt->payloadDelay = 0;

    // track time of availability of next prefetch, if any
    Tick next_pf_time = MaxTick;

    bool needsResponse = pkt->needsResponse();

    if (satisfied) {
        // should never be satisfying an uncacheable access as we
        // flush and invalidate any existing block as part of the
        // lookup
        assert(!pkt->req->isUncacheable());

        // hit (for all other request types)

        if (prefetcher && (prefetchOnAccess ||
                           (blk && blk->wasPrefetched()))) {
            if (blk)
                blk->status &= ~BlkHWPrefetched;

            // Don't notify on SWPrefetch
            if (!pkt->cmd.isSWPrefetch())
                next_pf_time = prefetcher->notify(pkt);
        }

        if (needsResponse) {
            pkt->makeTimingResponse();
            // @todo: Make someone pay for this
            pkt->headerDelay = pkt->payloadDelay = 0;

            // In this case we are considering request_time that takes
            // into account the delay of the xbar, if any, and just
            // lat, neglecting responseLatency, modelling hit latency
            // just as lookupLatency or or the value of lat overriden
            // by access(), that calls accessBlock() function.
            cpuSidePort->schedTimingResp(pkt, request_time, true);
        } else {
            DPRINTF(Cache, "%s satisfied %s, no response needed\n", __func__,
                    pkt->print());

            // queue the packet for deletion, as the sending cache is
            // still relying on it; if the block is found in access(),
            // CleanEvict and Writeback messages will be deleted
            // here as well
            pendingDelete.reset(pkt);
        }
    } else {
        // miss

        /* MJL_Begin */
        Addr blk_addr;
        MSHR *mshr = nullptr;
        if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
            blk_addr = MJL_blockAlign(pkt->getAddr(), pkt->MJL_getCmdDir());
            mshr = pkt->req->isUncacheable() ? nullptr :
                mshrQueue.MJL_findMatch(blk_addr, pkt->MJL_getCmdDir(), pkt->isSecure());

            if ( (!mshr) && (pkt->getSize() <= sizeof(uint64_t)) ) {
                if (pkt->MJL_cmdIsRow()) {
                    blk_addr = MJL_blockAlign(pkt->getAddr(), MemCmd::MJL_DirAttribute::MJL_IsColumn);
                    mshr = pkt->req->isUncacheable() ? nullptr :
                            mshrQueue.MJL_findMatch(blk_addr, MemCmd::MJL_DirAttribute::MJL_IsColumn, pkt->isSecure());
                } else if (pkt->MJL_cmdIsColumn()) {
                    blk_addr = MJL_blockAlign(pkt->getAddr(), MemCmd::MJL_DirAttribute::MJL_IsRow);
                    mshr = pkt->req->isUncacheable() ? nullptr :
                            mshrQueue.MJL_findMatch(blk_addr, MemCmd::MJL_DirAttribute::MJL_IsRow, pkt->isSecure());
                } else {
                    assert( pkt->MJL_cmdIsRow() || pkt->MJL_cmdIsColumn() );
                }
            }
        } else {
            blk_addr = blockAlign(pkt->getAddr());
            mshr = pkt->req->isUncacheable() ? nullptr :
                mshrQueue.findMatch(blk_addr, pkt->isSecure());
        }
        /* MJL_End */
        /* MJL_Comment
        Addr blk_addr = blockAlign(pkt->getAddr());
        */

        // ignore any existing MSHR if we are dealing with an
        // uncacheable request
        /* MJL_Comment 
        MSHR *mshr = pkt->req->isUncacheable() ? nullptr :
            mshrQueue.findMatch(blk_addr, pkt->isSecure());
        */

        // Software prefetch handling:
        // To keep the core from waiting on data it won't look at
        // anyway, send back a response with dummy data. Miss handling
        // will continue asynchronously. Unfortunately, the core will
        // insist upon freeing original Packet/Request, so we have to
        // create a new pair with a different lifecycle. Note that this
        // processing happens before any MSHR munging on the behalf of
        // this request because this new Request will be the one stored
        // into the MSHRs, not the original.
        if (pkt->cmd.isSWPrefetch()) {
            assert(needsResponse);
            assert(pkt->req->hasPaddr());
            assert(!pkt->req->isUncacheable());

            // There's no reason to add a prefetch as an additional target
            // to an existing MSHR. If an outstanding request is already
            // in progress, there is nothing for the prefetch to do.
            // If this is the case, we don't even create a request at all.
            PacketPtr pf = nullptr;

            if (!mshr) {
                // copy the request and create a new SoftPFReq packet
                RequestPtr req = new Request(pkt->req->getPaddr(),
                                             pkt->req->getSize(),
                                             pkt->req->getFlags(),
                                             pkt->req->masterId());
                
                /* MJL_Begin */
                if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
                    req->MJL_setReqDir(pkt->req->MJL_getReqDir());
                }
                req->MJL_cachelineSize = blkSize;
                req->MJL_rowWidth = MJL_rowWidth;
                // MJL_TODO: not sure whether this would cause problem, but I would assume that reqDir should be the same as cmdDir
                //assert(pkt->req->MJL_getReqDir() == pkt->MJL_getCmdDir());
                /* MJL_End */
                pf = new Packet(req, pkt->cmd);
                pf->allocate();
                assert(pf->getAddr() == pkt->getAddr());
                assert(pf->getSize() == pkt->getSize());
            }

            pkt->makeTimingResponse();

            // request_time is used here, taking into account lat and the delay
            // charged if the packet comes from the xbar.
            cpuSidePort->schedTimingResp(pkt, request_time, true);

            // If an outstanding request is in progress (we found an
            // MSHR) this is set to null
            pkt = pf;
        }

        if (mshr) {
            /// MSHR hit
            /// @note writebacks will be checked in getNextMSHR()
            /// for any conflicting requests to the same block

            //@todo remove hw_pf here

            // Coalesce unless it was a software prefetch (see above).
            if (pkt) {
                assert(!pkt->isWriteback());
                // CleanEvicts corresponding to blocks which have
                // outstanding requests in MSHRs are simply sunk here
                if (pkt->cmd == MemCmd::CleanEvict) {
                    pendingDelete.reset(pkt);
                } else {
                    DPRINTF(Cache, "%s coalescing MSHR for %s\n", __func__,
                            pkt->print());

                    assert(pkt->req->masterId() < system->maxMasters());
                    mshr_hits[pkt->cmdToIndex()][pkt->req->masterId()]++;
                    // We use forward_time here because it is the same
                    // considering new targets. We have multiple
                    // requests for the same address here. It
                    // specifies the latency to allocate an internal
                    // buffer and to schedule an event to the queued
                    // port and also takes into account the additional
                    // delay of the xbar.
                    mshr->allocateTarget(pkt, forward_time, order++,
                                         allocOnFill(pkt->cmd));
                    /* MJL_Begin */
                    if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
                        MJL_markBlockInfo(mshr);
                    }
                    /* MJL_End */
                    if (mshr->getNumTargets() == numTarget) {
                        noTargetMSHR = mshr;
                        setBlocked(Blocked_NoTargets);
                        // need to be careful with this... if this mshr isn't
                        // ready yet (i.e. time > curTick()), we don't want to
                        // move it ahead of mshrs that are ready
                        // mshrQueue.moveToFront(mshr);
                    }
                }
                // We should call the prefetcher reguardless if the request is
                // satisfied or not, reguardless if the request is in the MSHR
                // or not.  The request could be a ReadReq hit, but still not
                // satisfied (potentially because of a prior write to the same
                // cache line.  So, even when not satisfied, tehre is an MSHR
                // already allocated for this, we need to let the prefetcher
                // know about the request
                if (prefetcher) {
                    // Don't notify on SWPrefetch
                    if (!pkt->cmd.isSWPrefetch())
                        next_pf_time = prefetcher->notify(pkt);
                }
            }
        } else {
            // no MSHR
            assert(pkt->req->masterId() < system->maxMasters());
            if (pkt->req->isUncacheable()) {
                mshr_uncacheable[pkt->cmdToIndex()][pkt->req->masterId()]++;
            } else {
                mshr_misses[pkt->cmdToIndex()][pkt->req->masterId()]++;
            }

            if (pkt->isEviction() ||
                (pkt->req->isUncacheable() && pkt->isWrite())) {
                // We use forward_time here because there is an
                // uncached memory write, forwarded to WriteBuffer.
                allocateWriteBuffer(pkt, forward_time);
            } else {
                /* MJL_Begin */ 
                if (MJL_2DCache && blk) {
                    // In a physically 2D cache, a write miss to a valid tile would need non-readable marked only when there is valid data in the write interval
                    bool MJL_blkValid = blk->isValid();
                    if (pkt->MJL_cmdIsColumn()) {
                        MJL_blkValid = blk->MJL_crossValid[pkt->MJL_getColOffset(blkSize)/sizeof(uint64_t)];
                        for (int i = pkt->getOffset(blkSize); i < pkt->getOffset(blkSize) + pkt->getSize(); i = i + sizeof(uint64_t) - i%sizeof(uint64_t)) {
                            MJL_blkValid |= tags->MJL_findBlockByTile(blk, i/sizeof(uint64_t))->isValid();
                        }
                    } else if (pkt->MJL_cmdIsRow()) {
                        for (int i = pkt->getOffset(blkSize); i < pkt->getOffset(blkSize) + pkt->getSize(); i = i + sizeof(uint64_t) - i%sizeof(uint64_t)) {
                            MJL_blkValid |= blk->MJL_crossValid[i/sizeof(uint64_t)];
                        }
                    }
                    if (MJL_blkValid) {
                        assert(!pkt->req->isUncacheable());
                        assert(pkt->needsWritable());
                        assert(!blk->isWritable());
                        for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                            tags->MJL_findBlockByTile(blk, i)->status &= ~BlkReadable;
                        }
                    }
                } else /* MJL_End */if (blk && blk->isValid()) {
                    // should have flushed and have no valid block
                    assert(!pkt->req->isUncacheable());

                    // If we have a write miss to a valid block, we
                    // need to mark the block non-readable.  Otherwise
                    // if we allow reads while there's an outstanding
                    // write miss, the read could return stale data
                    // out of the cache block... a more aggressive
                    // system could detect the overlap (if any) and
                    // forward data out of the MSHRs, but we don't do
                    // that yet.  Note that we do need to leave the
                    // block valid so that it stays in the cache, in
                    // case we get an upgrade response (and hence no
                    // new data) when the write miss completes.
                    // As long as CPUs do proper store/load forwarding
                    // internally, and have a sufficiently weak memory
                    // model, this is probably unnecessary, but at some
                    // point it must have seemed like we needed it...
                    assert(pkt->needsWritable());
                    assert(!blk->isWritable());
                    blk->status &= ~BlkReadable;
                    /* MJL_Begin */
                    // Write miss, crossing blocks should have been invalidated
                    // MJL_unreadableOtherBlocks(pkt->getAddr(), blk->MJL_blkDir, pkt->getSize(), pkt->isSecure());
                    /* MJL_End */
                }
                // Here we are using forward_time, modelling the latency of
                // a miss (outbound) just as forwardLatency, neglecting the
                // lookupLatency component.
                allocateMissBuffer(pkt, forward_time);
            }

            if (prefetcher) {
                // Don't notify on SWPrefetch
                if (!pkt->cmd.isSWPrefetch())
                    next_pf_time = prefetcher->notify(pkt);
            }
        }
    }

    if (next_pf_time != MaxTick)
        schedMemSideSendEvent(next_pf_time);

    return true;
}

PacketPtr
Cache::createMissPacket(PacketPtr cpu_pkt, CacheBlk *blk,
                        bool needsWritable) const
{
    // should never see evictions here
    assert(!cpu_pkt->isEviction());
    /* MJL_Begin */
    bool blkValid = (blk != nullptr);
    if (!MJL_2DCache || cpu_pkt->MJL_getCmdDir() == MemCmd::MJL_DirAttribute::MJL_IsRow) {
        blkValid = blkValid && blk->isValid();
    } else if (cpu_pkt->MJL_getCmdDir() == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
        blkValid = blkValid && blk->MJL_crossValid[cpu_pkt->MJL_getColOffset(blkSize)/sizeof(uint64_t)];
    }
    /* MJL_End */
    /* MJL_Comment
    bool blkValid = blk && blk->isValid();
    */

    if (cpu_pkt->req->isUncacheable() ||
        (!blkValid && cpu_pkt->isUpgrade()) ||
        cpu_pkt->cmd == MemCmd::InvalidateReq) {
        // uncacheable requests and upgrades from upper-level caches
        // that missed completely just go through as is
        return nullptr;
    }

    assert(cpu_pkt->needsResponse());

    MemCmd cmd;
    // @TODO make useUpgrades a parameter.
    // Note that ownership protocols require upgrade, otherwise a
    // write miss on a shared owned block will generate a ReadExcl,
    // which will clobber the owned copy.
    const bool useUpgrades = true;
    if (cpu_pkt->cmd == MemCmd::WriteLineReq) {
        assert(!blkValid || !blk->isWritable());
        // forward as invalidate to all other caches, this gives us
        // the line in Exclusive state, and invalidates all other
        // copies
        // MJL_TODO: coherence related
        cmd = MemCmd::InvalidateReq;
        /* MJL_Begin */
        if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
            cmd.MJL_setCmdDir(cpu_pkt->MJL_getCmdDir());
        }
        /* MJL_End */
    } else if (blkValid && useUpgrades) {
        // only reason to be here is that blk is read only and we need
        // it to be writable
        assert(needsWritable);
        assert(!blk->isWritable());
        cmd = cpu_pkt->isLLSC() ? MemCmd::SCUpgradeReq : MemCmd::UpgradeReq;
        /* MJL_Begin */
        if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
            cmd.MJL_setCmdDir(blk->MJL_blkDir);
        }
        /* MJL_End */
    } else if (cpu_pkt->cmd == MemCmd::SCUpgradeFailReq ||
               cpu_pkt->cmd == MemCmd::StoreCondFailReq) {
        // Even though this SC will fail, we still need to send out the
        // request and get the data to supply it to other snoopers in the case
        // where the determination the StoreCond fails is delayed due to
        // all caches not being on the same local bus.
        cmd = MemCmd::SCUpgradeFailReq;
        /* MJL_Begin */
        if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
            cmd.MJL_setCmdDir(cpu_pkt->MJL_getCmdDir());
        }
        /* MJL_End */
    } else {
        // block is invalid
        cmd = needsWritable ? MemCmd::ReadExReq :
            (isReadOnly ? MemCmd::ReadCleanReq : MemCmd::ReadSharedReq);
        /* MJL_Begin */
        if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
            cmd.MJL_setCmdDir(cpu_pkt->MJL_getCmdDir());
        }
        /* MJL_End */
    }
    PacketPtr pkt = new Packet(cpu_pkt->req, cmd, blkSize);

    // if there are upstream caches that have already marked the
    // packet as having sharers (not passing writable), pass that info
    // downstream
    if (cpu_pkt->hasSharers() && !needsWritable) {
        // note that cpu_pkt may have spent a considerable time in the
        // MSHR queue and that the information could possibly be out
        // of date, however, there is no harm in conservatively
        // assuming the block has sharers
        pkt->setHasSharers();
        DPRINTF(Cache, "%s: passing hasSharers from %s to %s\n",
                __func__, cpu_pkt->print(), pkt->print());
    }

    // the packet should be block aligned
    /* MJL_Begin */
    if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
        assert(pkt->getAddr() == MJL_blockAlign(pkt->getAddr(), pkt->MJL_getCmdDir()));
    } else {
        assert(pkt->getAddr() == blockAlign(pkt->getAddr()));
    }
    /* MJL_End */
    /* MJL_Comment
    assert(pkt->getAddr() == blockAlign(pkt->getAddr()));
    */

    pkt->allocate();
    DPRINTF(Cache, "%s: created %s from %s\n", __func__, pkt->print(),
            cpu_pkt->print());
    return pkt;
}


Tick
Cache::recvAtomic(PacketPtr pkt)
{
    // We are in atomic mode so we pay just for lookupLatency here.
    Cycles lat = lookupLatency;

    // Forward the request if the system is in cache bypass mode.
    if (system->bypassCaches())
        return ticksToCycles(memSidePort->sendAtomic(pkt));

    promoteWholeLineWrites(pkt);

    // follow the same flow as in recvTimingReq, and check if a cache
    // above us is responding
    if (pkt->cacheResponding()) {
        DPRINTF(Cache, "Cache above responding to %s: not responding\n",
                pkt->print());

        // if a cache is responding, and it had the line in Owned
        // rather than Modified state, we need to invalidate any
        // copies that are not on the same path to memory
        assert(pkt->needsWritable() && !pkt->responderHadWritable());
        lat += ticksToCycles(memSidePort->sendAtomic(pkt));

        return lat * clockPeriod();
    }

    // should assert here that there are no outstanding MSHRs or
    // writebacks... that would mean that someone used an atomic
    // access in timing mode

    CacheBlk *blk = nullptr;
    PacketList writebacks;
    bool satisfied = access(pkt, blk, lat, writebacks);

    // handle writebacks resulting from the access here to ensure they
    // logically proceed anything happening below
    doWritebacksAtomic(writebacks);

    if (!satisfied) {
        // MISS

        // deal with the packets that go through the write path of
        // the cache, i.e. any evictions and uncacheable writes
        if (pkt->isEviction() ||
            (pkt->req->isUncacheable() && pkt->isWrite())) {
            lat += ticksToCycles(memSidePort->sendAtomic(pkt));
            return lat * clockPeriod();
        }
        // only misses left

        PacketPtr bus_pkt = createMissPacket(pkt, blk, pkt->needsWritable());

        bool is_forward = (bus_pkt == nullptr);

        if (is_forward) {
            // just forwarding the same request to the next level
            // no local cache operation involved
            bus_pkt = pkt;
        }

        DPRINTF(Cache, "%s: Sending an atomic %s\n", __func__,
                bus_pkt->print());

#if TRACING_ON
        CacheBlk::State old_state = blk ? blk->status : 0;
#endif

        lat += ticksToCycles(memSidePort->sendAtomic(bus_pkt));

        bool is_invalidate = bus_pkt->isInvalidate();

        // We are now dealing with the response handling
        DPRINTF(Cache, "%s: Receive response: %s in state %i\n", __func__,
                bus_pkt->print(), old_state);

        // If packet was a forward, the response (if any) is already
        // in place in the bus_pkt == pkt structure, so we don't need
        // to do anything.  Otherwise, use the separate bus_pkt to
        // generate response to pkt and then delete it.
        if (!is_forward) {
            if (pkt->needsResponse()) {
                assert(bus_pkt->isResponse());
                if (bus_pkt->isError()) {
                    pkt->makeAtomicResponse();
                    pkt->copyError(bus_pkt);
                } else if (pkt->cmd == MemCmd::WriteLineReq) {
                    // note the use of pkt, not bus_pkt here.

                    // write-line request to the cache that promoted
                    // the write to a whole line
                    blk = handleFill(pkt, blk, writebacks,
                                     allocOnFill(pkt->cmd));
                    assert(blk != NULL);
                    is_invalidate = false;
                    /* MJL_Begin */
                    // Should not need the invalidation, things should have been taken care of at the access
                    //MJL_invalidateOtherBlocks(pkt->getAddr(), pkt->MJL_getDataDir(), pkt->getSize(), pkt->isSecure(), writebacks);
                    /* MJL_End */
                    satisfyRequest(pkt, blk);
                } else if (bus_pkt->isRead() ||
                           bus_pkt->cmd == MemCmd::UpgradeResp) {
                    // we're updating cache state to allow us to
                    // satisfy the upstream request from the cache
                    blk = handleFill(bus_pkt, blk, writebacks,
                                     allocOnFill(pkt->cmd));
                    satisfyRequest(pkt, blk);
                    maintainClusivity(pkt->fromCache(), blk);
                } else {
                    // we're satisfying the upstream request without
                    // modifying cache state, e.g., a write-through
                    pkt->makeAtomicResponse();
                }
            }
            delete bus_pkt;
        }

        /* MJL_Begin */
        if (is_invalidate && MJL_2DCache && blk && (blk != tempBlock)) {
            if (blk->isValid() || blk->MJL_hasCrossValid()) {
                MJL_invalidateTile(blk);
            }
        } else /* MJL_End */if (is_invalidate && blk && blk->isValid()) {
            invalidateBlock(blk);
        }
    }

    // Note that we don't invoke the prefetcher at all in atomic mode.
    // It's not clear how to do it properly, particularly for
    // prefetchers that aggressively generate prefetch candidates and
    // rely on bandwidth contention to throttle them; these will tend
    // to pollute the cache in atomic mode since there is no bandwidth
    // contention.  If we ever do want to enable prefetching in atomic
    // mode, though, this is the place to do it... see timingAccess()
    // for an example (though we'd want to issue the prefetch(es)
    // immediately rather than calling requestMemSideBus() as we do
    // there).

    // do any writebacks resulting from the response handling
    doWritebacksAtomic(writebacks);

    // if we used temp block, check to see if its valid and if so
    // clear it out, but only do so after the call to recvAtomic is
    // finished so that any downstream observers (such as a snoop
    // filter), first see the fill, and only then see the eviction
    if (blk == tempBlock && tempBlock->isValid()) {
        // the atomic CPU calls recvAtomic for fetch and load/store
        // sequentuially, and we may already have a tempBlock
        // writeback from the fetch that we have not yet sent
        if (tempBlockWriteback) {
            // if that is the case, write the prevoius one back, and
            // do not schedule any new event
            writebackTempBlockAtomic();
        } else {
            // the writeback/clean eviction happens after the call to
            // recvAtomic has finished (but before any successive
            // calls), so that the response handling from the fill is
            // allowed to happen first
            schedule(writebackTempBlockAtomicEvent, curTick());
        }

        tempBlockWriteback = (blk->isDirty() || writebackClean) ?
            writebackBlk(blk) : cleanEvictBlk(blk);
        blk->invalidate();
    }

    if (pkt->needsResponse()) {
        pkt->makeAtomicResponse();
    }

    return lat * clockPeriod();
}


void
Cache::functionalAccess(PacketPtr pkt, bool fromCpuSide)
{
    if (system->bypassCaches()) {
        // Packets from the memory side are snoop request and
        // shouldn't happen in bypass mode.
        assert(fromCpuSide);

        // The cache should be flushed if we are in cache bypass mode,
        // so we don't need to check if we need to update anything.
        memSidePort->sendFunctional(pkt);
        return;
    }

    /* MJL_Begin */ 
    assert(pkt->MJL_getCmdDir() == CacheBlk::MJL_CacheBlkDir::MJL_IsRow);
    // functional access for same direction blocks
    Addr blk_addr = MJL_blockAlign(pkt->getAddr(), pkt->MJL_getCmdDir());
    bool is_secure = pkt->isSecure();
    CacheBlk *blk = tags->MJL_findBlock(pkt->getAddr(), pkt->MJL_getCmdDir(), is_secure);
    MSHR *mshr = mshrQueue.MJL_findMatch(blk_addr, pkt->MJL_getCmdDir(), is_secure);

    pkt->pushLabel(name());

    CacheBlkPrintWrapper cbpw(blk);
    bool have_data = blk && blk->isValid()
        && pkt->MJL_checkFunctional(&cbpw, blk_addr, pkt->MJL_getCmdDir(), is_secure, blkSize,
                                blk->data);
    bool have_dirty =
        have_data && (blk->isDirty() ||
                    (mshr && mshr->inService && mshr->isPendingModified()));

    bool done = have_dirty
        || cpuSidePort->MJL_checkFunctional(pkt)
        || mshrQueue.MJL_checkFunctional(pkt, blk_addr, pkt->MJL_getDataDir())
        || writeBuffer.MJL_checkFunctional(pkt, blk_addr, pkt->MJL_getDataDir())
        || memSidePort->MJL_checkFunctional(pkt);

    DPRINTF(CacheVerbose, "%s: %s %s%s%s\n", __func__,  pkt->print(),
            (blk && blk->isValid()) ? "valid " : "",
            have_data ? "data " : "", done ? "done " : "");

    // functional access for different direction blocks
    bool diff_have_data = false;
    bool diff_have_dirty = false;
    bool diff_done = false;
    for (Addr MJL_wordAddr = pkt->getAddr(); MJL_wordAddr < (pkt->getAddr() + pkt->getSize()); MJL_wordAddr = MJL_wordAddr + sizeof(uint64_t)) {
        blk_addr = MJL_blockAlign(MJL_wordAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
        blk = tags->MJL_findBlock(MJL_wordAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn, is_secure);
        mshr = mshrQueue.MJL_findMatch(blk_addr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn, is_secure);

        CacheBlkPrintWrapper cbpw(blk);
        diff_have_data = diff_have_data || (blk && blk->isValid()
            && pkt->MJL_checkFunctional(&cbpw, blk_addr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn, is_secure, blkSize,
                                    blk->data));

        if (blk && blk->isValid() && (blk->isDirty() ||
                        (mshr && mshr->inService && mshr->isPendingModified()))) {
            diff_have_dirty = diff_have_dirty || pkt->MJL_setHaveDirty(blk_addr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn, is_secure, blkSize,
                        blk->data);
        }

        diff_done = diff_done || (diff_have_dirty
            || cpuSidePort->MJL_checkFunctional(pkt)
            || mshrQueue.MJL_checkFunctional(pkt, blk_addr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn)
            || writeBuffer.MJL_checkFunctional(pkt, blk_addr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn)
            || memSidePort->MJL_checkFunctional(pkt));

        DPRINTF(CacheVerbose, "%s: %s %s%s%s\n", __func__,  pkt->print(),
                (blk && blk->isValid()) ? "valid " : "",
                have_data ? "data " : "", done ? "done " : "");

    }
    done = done || diff_done;
    /* MJL_End */
    /* MJL_Comment 
    Addr blk_addr = blockAlign(pkt->getAddr());
    bool is_secure = pkt->isSecure();
    CacheBlk *blk = tags->findBlock(pkt->getAddr(), is_secure);
    MSHR *mshr = mshrQueue.findMatch(blk_addr, is_secure);
    
    

    pkt->pushLabel(name());

    CacheBlkPrintWrapper cbpw(blk);

    // Note that just because an L2/L3 has valid data doesn't mean an
    // L1 doesn't have a more up-to-date modified copy that still
    // needs to be found.  As a result we always update the request if
    // we have it, but only declare it satisfied if we are the owner.

    // see if we have data at all (owned or otherwise)
    bool have_data = blk && blk->isValid()
        && pkt->checkFunctional(&cbpw, blk_addr, is_secure, blkSize,
                                blk->data);

    // data we have is dirty if marked as such or if we have an
    // in-service MSHR that is pending a modified line
    bool have_dirty =
        have_data && (blk->isDirty() ||
                      (mshr && mshr->inService && mshr->isPendingModified()));

    bool done = have_dirty
        || cpuSidePort->checkFunctional(pkt)
        || mshrQueue.checkFunctional(pkt, blk_addr)
        || writeBuffer.checkFunctional(pkt, blk_addr)
        || memSidePort->checkFunctional(pkt);

    DPRINTF(CacheVerbose, "%s: %s %s%s%s\n", __func__,  pkt->print(),
            (blk && blk->isValid()) ? "valid " : "",
            have_data ? "data " : "", done ? "done " : "");
    */

    // We're leaving the cache, so pop cache->name() label
    pkt->popLabel();

    if (done) {
        pkt->makeResponse();
    } else {
        // if it came as a request from the CPU side then make sure it
        // continues towards the memory side
        if (fromCpuSide) {
            memSidePort->sendFunctional(pkt);
        } else if (cpuSidePort->isSnooping()) {
            // if it came from the memory side, it must be a snoop request
            // and we should only forward it if we are forwarding snoops
            cpuSidePort->sendFunctionalSnoop(pkt);
        }
    }
}


/////////////////////////////////////////////////////
//
// Response handling: responses from the memory side
//
/////////////////////////////////////////////////////


void
Cache::handleUncacheableWriteResp(PacketPtr pkt)
{
    Tick completion_time = clockEdge(responseLatency) +
        pkt->headerDelay + pkt->payloadDelay;

    // Reset the bus additional time as it is now accounted for
    pkt->headerDelay = pkt->payloadDelay = 0;

    cpuSidePort->schedTimingResp(pkt, completion_time, true);
}

void
Cache::recvTimingResp(PacketPtr pkt)
{
    assert(pkt->isResponse());

    // all header delay should be paid for by the crossbar, unless
    // this is a prefetch response from above
    panic_if(pkt->headerDelay != 0 && pkt->cmd != MemCmd::HardPFResp,
             "%s saw a non-zero packet delay\n", name());

    bool is_error = pkt->isError();

    if (is_error) {
        DPRINTF(Cache, "%s: Cache received %s with error\n", __func__,
                pkt->print());
    }

    DPRINTF(Cache, "%s: Handling response %s\n", __func__,
            pkt->print());

    // if this is a write, we should be looking at an uncacheable
    // write
    if (pkt->isWrite()) {
        assert(pkt->req->isUncacheable());
        handleUncacheableWriteResp(pkt);
        return;
    }

    // we have dealt with any (uncacheable) writes above, from here on
    // we know we are dealing with an MSHR due to a miss or a prefetch
    MSHR *mshr = dynamic_cast<MSHR*>(pkt->popSenderState());
    assert(mshr);

    if (mshr == noTargetMSHR) {
        // we always clear at least one target
        clearBlocked(Blocked_NoTargets);
        noTargetMSHR = nullptr;
    }

    // Initial target is used just for stats
    MSHR::Target *initial_tgt = mshr->getTarget();
    int stats_cmd_idx = initial_tgt->pkt->cmdToIndex();
    Tick miss_latency = curTick() - initial_tgt->recvTime;

    if (pkt->req->isUncacheable()) {
        assert(pkt->req->masterId() < system->maxMasters());
        mshr_uncacheable_lat[stats_cmd_idx][pkt->req->masterId()] +=
            miss_latency;
    } else {
        assert(pkt->req->masterId() < system->maxMasters());
        mshr_miss_latency[stats_cmd_idx][pkt->req->masterId()] +=
            miss_latency;
    }

    bool wasFull = mshrQueue.isFull();

    PacketList writebacks;

    Tick forward_time = clockEdge(forwardLatency) + pkt->headerDelay;

    // upgrade deferred targets if the response has no sharers, and is
    // thus passing writable
    if (!pkt->hasSharers()) {
        mshr->promoteWritable();
    }

    bool is_fill = !mshr->isForward &&
        (pkt->isRead() || pkt->cmd == MemCmd::UpgradeResp);

    /* MJL_Begin */
    CacheBlk *blk = nullptr;
    if (MJL_2DCache && pkt->MJL_dataIsColumn()) {
        assert(pkt->MJL_sameCmdDataDir());
        blk = tags->MJL_findCrossBlock(pkt->getAddr(), CacheBlk::MJL_CacheBlkDir::MJL_IsRow, pkt->isSecure(), pkt->MJL_getColOffset(blkSize));
    } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
        blk = tags->MJL_findBlock(pkt->getAddr(), pkt->MJL_getCmdDir(), pkt->isSecure());
    } else {
        blk = tags->findBlock(pkt->getAddr(), pkt->isSecure());
    }
    /* MJL_End */
    /* MJL_Comment
    CacheBlk *blk = tags->findBlock(pkt->getAddr(), pkt->isSecure());
    */

    if (is_fill && !is_error) {
        DPRINTF(Cache, "Block for addr %#llx being updated in Cache\n",
                pkt->getAddr());

        blk = handleFill(pkt, blk, writebacks, mshr->allocOnFill());
        assert(blk != nullptr);
    }

    // allow invalidation responses originating from write-line
    // requests to be discarded
    bool is_invalidate = pkt->isInvalidate();

    // First offset for critical word first calculations
    int initial_offset = initial_tgt->pkt->getOffset(blkSize);

    bool from_cache = false;
    /* MJL_Begin */
    bool MJL_writeback = false;
    bool MJL_invalidate = false;
    Counter MJL_order = initial_tgt->order;
    /* MJL_End */
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
        if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
            MJL_writeback |= target.MJL_postWriteback;
            MJL_invalidate |= target.MJL_postInvalidate;
            assert(!target.MJL_postInvalidate || (&target == &targets.back())); 
            if (target.MJL_postWriteback) {
                MJL_order = target.order;
            } else if (!MJL_writeback && target.MJL_postInvalidate) {
                MJL_order = target.order;
            }
            target.MJL_clearBlocking();
        }
        /* MJL_End */
    }

    maintainClusivity(from_cache, blk);

    // MJL_TODO: probably coherence handling
    if (blk && blk->isValid()) {
        /* MJL_Begin */
        if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
            PacketPtr MJL_postPkt = nullptr;
            if (MJL_invalidate && (blk->isDirty() || writebackClean)) {
                MJL_postPkt = writebackBlk(blk);
            } else if (MJL_invalidate) {
                MJL_postPkt = cleanEvictBlk(blk);
            } else if (MJL_writeback && blk->isDirty()) {
                MJL_postPkt = MJL_writebackCachedBlk(blk);
            } else if (MJL_writeback) {
                blk->status &= ~BlkWritable;
            }
            
            if (MJL_postPkt) {
                // physically 2D cache should not have this problem
                assert(!MJL_2DCache);
                MJL_conflictWBCount3++;
                MJL_postPkt->MJL_hasOrder = true;
                MJL_postPkt->MJL_order = MJL_order;
                writebacks.push_back(MJL_postPkt);
            }
        }
        /* MJL_End */
        // an invalidate response stemming from a write line request
        // should not invalidate the block, so check if the
        // invalidation should be discarded
        if (/* MJL_Begin */MJL_invalidate || /* MJL_End */is_invalidate || mshr->hasPostInvalidate()) {
            invalidateBlock(blk);
        } else if (mshr->hasPostDowngrade()) {
            blk->status &= ~BlkWritable;
        }
    }

    if (mshr->promoteDeferredTargets()) {
        // avoid later read getting stale data while write miss is
        // outstanding.. see comment in timingAccess()
        if (blk) {
            blk->status &= ~BlkReadable;
        }
        mshrQueue.markPending(mshr);
        schedMemSideSendEvent(clockEdge() + pkt->payloadDelay);
    } else {
        mshrQueue.deallocate(mshr);
        if (wasFull && !mshrQueue.isFull()) {
            clearBlocked(Blocked_NoMSHRs);
        }

        // Request the bus for a prefetch if this deallocation freed enough
        // MSHRs for a prefetch to take place
        if (prefetcher && mshrQueue.canPrefetch()) {
            Tick next_pf_time = std::max(prefetcher->nextPrefetchReadyTime(),
                                         clockEdge());
            if (next_pf_time != MaxTick)
                schedMemSideSendEvent(next_pf_time);
        }
    }
    // reset the xbar additional timinig  as it is now accounted for
    pkt->headerDelay = pkt->payloadDelay = 0;

    // copy writebacks to write buffer
    doWritebacks(writebacks, forward_time);

    // if we used temp block, check to see if its valid and then clear it out
    if (blk == tempBlock && tempBlock->isValid()) {
        // We use forwardLatency here because we are copying
        // Writebacks/CleanEvicts to write buffer. It specifies the latency to
        // allocate an internal buffer and to schedule an event to the
        // queued port.
        if (blk->isDirty() || writebackClean) {
            PacketPtr wbPkt = writebackBlk(blk);
            allocateWriteBuffer(wbPkt, forward_time);
            // Set BLOCK_CACHED flag if cached above.
            if (isCachedAbove(wbPkt))
                wbPkt->setBlockCached();
        } else {
            PacketPtr wcPkt = cleanEvictBlk(blk);
            // Check to see if block is cached above. If not allocate
            // write buffer
            if (isCachedAbove(wcPkt))
                delete wcPkt;
            else
                allocateWriteBuffer(wcPkt, forward_time);
        }
        blk->invalidate();
    }

    DPRINTF(CacheVerbose, "%s: Leaving with %s\n", __func__, pkt->print());
    delete pkt;
}

PacketPtr
Cache::writebackBlk(CacheBlk *blk)
{
    chatty_assert(!isReadOnly || writebackClean,
                  "Writeback from read-only cache");
    assert(blk && blk->isValid() && (blk->isDirty() || writebackClean));

    writebacks[Request::wbMasterId]++;
    /* MJL_Begin */
    Request *req = nullptr;
    if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
        req = new Request(tags->MJL_regenerateBlkAddr(blk->tag, blk->MJL_blkDir, blk->set),
                               blkSize, 0, Request::wbMasterId);
        req->MJL_setReqDir(blk->MJL_blkDir);
    } else {
        req = new Request(tags->regenerateBlkAddr(blk->tag, blk->set),
                               blkSize, 0, Request::wbMasterId);
    }
    req->MJL_cachelineSize = blkSize;
    req->MJL_rowWidth = MJL_rowWidth;
    /* MJL_End */
    /* MJL_Comment
    Request *req = new Request(tags->regenerateBlkAddr(blk->tag, blk->set),
                               blkSize, 0, Request::wbMasterId);
    */
    if (blk->isSecure())
        req->setFlags(Request::SECURE);

    req->taskId(blk->task_id);
    blk->task_id= ContextSwitchTaskId::Unknown;
    blk->tickInserted = curTick();

    PacketPtr pkt =
        new Packet(req, blk->isDirty() ?
                   MemCmd::WritebackDirty : MemCmd::WritebackClean);
    /* MJL_Begin */
    if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
        pkt->cmd.MJL_setCmdDir(req->MJL_getReqDir());
        pkt->MJL_setDataDir(req->MJL_getReqDir());
        pkt->MJL_setWordDirtyFromBlk(blk->MJL_wordDirty, blkSize);
    }
    /* MJL_End */

    DPRINTF(Cache, "Create Writeback %s writable: %d, dirty: %d\n",
            pkt->print(), blk->isWritable(), blk->isDirty());

    if (blk->isWritable()) {
        // not asserting shared means we pass the block in modified
        // state, mark our own block non-writeable
        blk->status &= ~BlkWritable;
    } else {
        // we are in the Owned state, tell the receiver
        pkt->setHasSharers();
    }

    // make sure the block is not marked dirty
    /* MJL_Begin */
    if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
        blk->MJL_clearAllDirty();
    }
    /* MJL_End */
    blk->status &= ~BlkDirty;

    pkt->allocate();
    std::memcpy(pkt->getPtr<uint8_t>(), blk->data, blkSize);
    /* MJL_Begin */
    if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
        pkt->MJL_setDataDir(blk->MJL_blkDir);
    }
    /* MJL_End */

    return pkt;
}
/* MJL_Begin */
PacketPtr
Cache::MJL_writebackCachedBlk(CacheBlk *blk) {
    PacketPtr pkt = writebackBlk(blk);
    pkt->setBlockCached();
    return pkt;
}

PacketPtr
Cache::MJL_writebackColBlk(CacheBlk *blk, unsigned MJL_offset, bool blkDirty)
{
    chatty_assert(!isReadOnly || writebackClean,
                  "Writeback from read-only cache");
    assert(blk && blk->MJL_crossValid[MJL_offset/sizeof(uint64_t)] && (blkDirty|| writebackClean));
    assert(MJL_2DCache);

    writebacks[Request::wbMasterId]++;
    // Get the column block address
    Addr wbAddr = tags->MJL_regenerateBlkAddr(blk->tag, MemCmd::MJL_DirAttribute::MJL_IsRow, blk->set) + MJL_offset;
    wbAddr = tags->MJL_blkAlign(wbAddr, MemCmd::MJL_DirAttribute::MJL_IsColumn);

    Request *req = new Request(wbAddr, blkSize, 0, Request::wbMasterId);
    req->MJL_setReqDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);
    req->MJL_cachelineSize = blkSize;
    req->MJL_rowWidth = MJL_rowWidth;
    
    if (blk->isSecure())
        req->setFlags(Request::SECURE);

    req->taskId(blk->task_id);

    PacketPtr pkt =
        new Packet(req, blkDirty ?
                   MemCmd::WritebackDirty : MemCmd::WritebackClean);
                   
    pkt->cmd.MJL_setCmdDir(req->MJL_getReqDir());
    pkt->MJL_setDataDir(req->MJL_getReqDir());

    DPRINTF(Cache, "Create Writeback %s writable: %d, dirty: %d\n",
            pkt->print(), blk->isWritable(), blk->isDirty());

    if (blk->isWritable()) {
        // MJL_Note: Not clearing the BlkWritable here, clearing it later in caller.
        // not asserting shared means we pass the block in modified
        // state, mark our own block non-writeable
        // blk->status &= ~BlkWritable;
    } else {
        // we are in the Owned state, tell the receiver
        pkt->setHasSharers();
    }

    // Gather word dirty, data information from blocks
    bool MJL_tempWordDirty[blkSize/sizeof(uint64_t)];
    for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
        MJL_tempWordDirty[i] = false;
    }
    uint8_t MJL_tempData[blkSize];
    for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
        // Get pointer to blks
        CacheBlk *tile_blk = tags->MJL_findBlockByTile(blk, i);
        // Gather word dirty information from blks
        MJL_tempWordDirty[i] = tile_blk->MJL_wordDirty[MJL_offset/sizeof(uint64_t)];
        // make sure the block is not marked dirty
        tile_blk->MJL_clearWordDirty(MJL_offset/sizeof(uint64_t));
        tile_blk->MJL_updateDirty();
        // Gather data from blks
        memcpy(&MJL_tempData[i * sizeof(uint64_t)], tile_blk->data + MJL_offset, sizeof(uint64_t));
    }

    pkt->MJL_setWordDirtyFromBlk(MJL_tempWordDirty, blkSize);

    pkt->allocate();
    std::memcpy(pkt->getPtr<uint8_t>(), MJL_tempData, blkSize);
    pkt->MJL_setDataDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);

    return pkt;
}

void
Cache::MJL_clearTileWritable(CacheBlk *blk)
{
    for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
        // Get pointer to blks
        CacheBlk *tile_blk =  tags->MJL_findBlockByTile(blk, i);
        if (tile_blk->isWritable()) {
            // mark our own block non-writeable
            tile_blk->status &= ~BlkWritable;
        }
    }
}

void
Cache::MJL_resetTileInfo(CacheBlk *blk)
{
    for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
        // Get pointer to blks
        CacheBlk *tile_blk =  tags->MJL_findBlockByTile(blk, i);
        tile_blk->task_id = ContextSwitchTaskId::Unknown;
        tile_blk->tickInserted = curTick();
    }
}

PacketPtr
Cache::MJL_cleanEvictColBlk(CacheBlk *blk, unsigned MJL_offset, bool blkDirty)
{
    assert(!writebackClean);
    assert(blk && blk->MJL_crossValid[MJL_offset/sizeof(uint64_t)] && !blkDirty);

    // Get the column block address
    Addr wbAddr = tags->MJL_regenerateBlkAddr(blk->tag, MemCmd::MJL_DirAttribute::MJL_IsRow, blk->set) + MJL_offset;
    wbAddr = tags->MJL_blkAlign(col_repl_addr, MemCmd::MJL_DirAttribute::MJL_IsColumn);
    // Creating a zero sized write, a message to the snoop filter
    Request *req =
        new Request(wbAddr, blkSize, 0, Request::wbMasterId);

    req->MJL_setReqDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);
    req->MJL_cachelineSize = blkSize;
    req->MJL_rowWidth = MJL_rowWidth;
    
    if (blk->isSecure())
        req->setFlags(Request::SECURE);

    req->taskId(blk->task_id);
    // blk->task_id = ContextSwitchTaskId::Unknown;
    // blk->tickInserted = curTick();

    PacketPtr pkt = new Packet(req, MemCmd::CleanEvict);
    
    pkt->cmd.MJL_setCmdDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);
    pkt->MJL_setDataDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);
    
    pkt->allocate();
    DPRINTF(Cache, "Create CleanEvict %s\n", pkt->print());

    return pkt;
}
/* MJL_End */

PacketPtr
Cache::cleanEvictBlk(CacheBlk *blk)
{
    assert(!writebackClean);
    assert(blk && blk->isValid() && !blk->isDirty());
    // Creating a zero sized write, a message to the snoop filter
    Request *req =
    /* MJL_Begin */
        nullptr;
    if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
        req = new Request(tags->MJL_regenerateBlkAddr(blk->tag, blk->MJL_blkDir, blk->set), blkSize, 0,
                        Request::wbMasterId);
    req->MJL_setReqDir(blk->MJL_blkDir);
    } else {
        req = new Request(tags->regenerateBlkAddr(blk->tag, blk->set), blkSize, 0,
                        Request::wbMasterId);
    }
    req->MJL_cachelineSize = blkSize;
    req->MJL_rowWidth = MJL_rowWidth;
    /* MJL_End */
    /* MJL_Comment 
        new Request(tags->regenerateBlkAddr(blk->tag, blk->set), blkSize, 0,
                        Request::wbMasterId);
    */
    if (blk->isSecure())
        req->setFlags(Request::SECURE);

    req->taskId(blk->task_id);
    blk->task_id = ContextSwitchTaskId::Unknown;
    blk->tickInserted = curTick();

    PacketPtr pkt = new Packet(req, MemCmd::CleanEvict);
    /* MJL_Begin */
    if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
        pkt->cmd.MJL_setCmdDir(blk->MJL_blkDir);
        pkt->MJL_setDataDir(blk->MJL_blkDir);
    }
    /* MJL_End */
    pkt->allocate();
    DPRINTF(Cache, "Create CleanEvict %s\n", pkt->print());

    return pkt;
}

void
Cache::memWriteback()
{
    CacheBlkVisitorWrapper visitor(*this, &Cache::writebackVisitor);
    tags->forEachBlk(visitor);
}

void
Cache::memInvalidate()
{
    CacheBlkVisitorWrapper visitor(*this, &Cache::invalidateVisitor);
    tags->forEachBlk(visitor);
}

bool
Cache::isDirty() const
{
    CacheBlkIsDirtyVisitor visitor;
    tags->forEachBlk(visitor);

    return visitor.isDirty();
}

bool
Cache::writebackVisitor(CacheBlk &blk)
{
    if (blk.isDirty()) {
        assert(blk.isValid());

        /* MJL_Begin */
        Request request(tags->MJL_regenerateBlkAddr(blk.tag, blk.MJL_blkDir, blk.set),
                        blkSize, 0, Request::funcMasterId);
        request.MJL_setReqDir(blk.MJL_blkDir);
        request.MJL_cachelineSize = blkSize;
        request.MJL_rowWidth = MJL_rowWidth;
        /* MJL_End */
        /* MJL_Comment 
        Request request(tags->regenerateBlkAddr(blk.tag, blk.set),
                        blkSize, 0, Request::funcMasterId);
        req->MJL_setReqDir(blk->MJL_blkDir);
        */
        request.taskId(blk.task_id);

        Packet packet(&request, MemCmd::WriteReq);
        /* MJL_Begin */
        packet.cmd.MJL_setCmdDir(blk.MJL_blkDir);
        packet.MJL_setDataDir(blk.MJL_blkDir);
        packet.MJL_setWordDirtyFromBlk(blk.MJL_wordDirty, blkSize);
        /* MJL_End */
        packet.dataStatic(blk.data);

        memSidePort->sendFunctional(&packet);

        /* MJL_Begin */
        blk.MJL_clearAllDirty();
        /* MJL_End */
        blk.status &= ~BlkDirty;
    }

    return true;
}

bool
Cache::invalidateVisitor(CacheBlk &blk)
{

    if (blk.isDirty())
        warn_once("Invalidating dirty cache lines. Expect things to break.\n");

    if (blk.isValid()) {
        assert(!blk.isDirty());
        tags->invalidate(&blk);
        blk.invalidate();
    }

    return true;
}

CacheBlk*
Cache::allocateBlock(Addr addr, bool is_secure, PacketList &writebacks)
{
    CacheBlk *blk = tags->findVictim(addr);

    // It is valid to return nullptr if there is no victim
    if (!blk)
        return nullptr;

    if (blk->isValid()) {
        Addr repl_addr = tags->regenerateBlkAddr(blk->tag, blk->set);
        MSHR *repl_mshr = mshrQueue.findMatch(repl_addr, blk->isSecure());
        if (repl_mshr) {
            // must be an outstanding upgrade request
            // on a block we're about to replace...
            assert(!blk->isWritable() || blk->isDirty());
            assert(repl_mshr->needsWritable());
            // too hard to replace block with transient state
            // allocation failed, block not inserted
            return nullptr;
        } else {
            DPRINTF(Cache, "replacement: replacing %#llx (%s) with %#llx "
                    "(%s): %s\n", repl_addr, blk->isSecure() ? "s" : "ns",
                    addr, is_secure ? "s" : "ns",
                    blk->isDirty() ? "writeback" : "clean");

            if (blk->wasPrefetched()) {
                unusedPrefetches++;
            }
            // Will send up Writeback/CleanEvict snoops via isCachedAbove
            // when pushing this writeback list into the write buffer.
            if (blk->isDirty() || writebackClean) {
                // Save writeback packet for handling by caller
                writebacks.push_back(writebackBlk(blk));
            } else {
                writebacks.push_back(cleanEvictBlk(blk));
            }
        }
    }

    return blk;
}
/* MJL_Begin */
CacheBlk*
Cache::MJL_allocateBlock(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, bool is_secure, PacketList &writebacks)
{
    /* MJL_Begin */
    CacheBlk *blk = nullptr;
    if (MJL_2DCache) {
        blk = tags->MJL_findVictim(addr, MemCmd::MJL_DirAttribute::MJL_IsRow);
    } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
        blk = tags->MJL_findVictim(addr, MJL_cacheBlkDir);
    } else {
        return allocateBlock(addr, is_secure, writebacks);
    }
    /* MJL_End */
    /* MJL_Comment
    CacheBlk *blk = tags->MJL_findVictim(addr, MJL_cacheBlkDir);
    */

    // It is valid to return nullptr if there is no victim
    if (!blk)
        return nullptr;

    // If the cache is a physically 2D cache
    if (MJL_2DCache) {
        // Get the dirty status of each column
        bool MJL_colDirty[blkSize/sizeof(uint64_t)];
        for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
            MJL_colDirty[i] = false;
        }
        // Get if the whole tile is valid
        bool MJL_tileValid = true;

        // For each set(row) in this tile
        for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
            // Get the row block's pointer
            CacheBlk *tile_blk = tags->MJL_findBlockByTile(blk, i);
            for (int j = 0; j < blkSize/sizeof(uint64_t); ++j) {
                MJL_colDirty[j] |= tile_blk->MJL_wordDirty[j];
            }
            MJL_tileValid &= tile_blk->isValid();
            // If the row block is valid
            if (tile_blk->isValid()) {
                // Regenerate the block address of the row block and check if it is waiting on an upgrade request. If it is, just return nullptr
                Addr repl_addr = tags->MJL_regenerateBlkAddr(tile_blk->tag, tile_blk->MJL_blkDir, tile_blk->set);
                MSHR *repl_mshr = mshrQueue.MJL_findMatch(repl_addr, tile_blk->MJL_blkDir, tile_blk->isSecure());
                if (repl_mshr) {
                    // must be an outstanding upgrade request
                    // on a block we're about to replace...
                    // MJL_Note: Ignoring writable status for column, assuming it's the same as the row (actually the whole tile should have the same...)
                    assert(!tile_blk->isWritable() || tile_blk->isDirty());
                    assert(repl_mshr->needsWritable());
                    // too hard to replace block with transient state
                    // allocation failed, block not inserted
                    return nullptr;
                }
            }
        }
        // For each column in this tile
        for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
            // if the column is valid
            if (blk->MJL_crossValid[i]) {
                // Regenerate the column block address of this tile, and and check if it is waiting on an upgrade request. If it is, just return nullptr
                Addr col_repl_addr = tags->MJL_regenerateBlkAddr(blk->tag, blk->MJL_blkDir, blk->set) + i*sizeof(uint64_t);
                col_repl_addr = tags->MJL_blkAlign(col_repl_addr, MemCmd::MJL_DirAttribute::MJL_IsColumn);
                MSHR *repl_mshr = mshrQueue.MJL_findMatch(col_repl_addr, MemCmd::MJL_DirAttribute::MJL_IsColumn, blk->isSecure());
                if (repl_mshr) {
                    // must be an outstanding upgrade request
                    // on a block we're about to replace...
                    assert(!blk->isWritable() || MJL_colDirty[i]);
                    assert(repl_mshr->needsWritable());
                    // too hard to replace block with transient state
                    // allocation failed, block not inserted
                    return nullptr;
                }
            }
        }
        // Now no row or column of the tile should be waiting on a upgrade, the blks can be evicted
        // If the whole tile is valid, then favorize row treatments, otherwise treat columns first
        if (!MJL_tileValid) {
            // For each column in this tile
            for (int i = 0; i < 8; ++i) {
                // If the column is valid
                if (blk->MJL_crossValid[i]) {
                    // MJL_Note: Ignoring prefetch stat calculation (no additional prefetch status bits)
                    if (MJL_colDirty[i] || writebackClean) {
                        writebacks.push_back(MJL_writebackColBlk(blk, i, MJL_colDirty[i]));
                    } else {
                        writebacks.push_back(MJL_cleanEvictColBlk(blk, i, MJL_colDirty[i]));
                    }
                }
            }
        }
        // For each row in this tile
        for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
            CacheBlk *tile_blk = tags->MJL_findBlockByTile(blk, i);
            if (tile_blk->isValid()) {
                if (tile_blk->wasPrefetched()) {
                    unusedPrefetches++;
                }
                // Will send up Writeback/CleanEvict snoops via isCachedAbove
                // when pushing this writeback list into the write buffer.
                if (tile_blk->isDirty() || writebackClean) {
                    // Save writeback packet for handling by caller
                    writebacks.push_back(writebackBlk(tile_blk));
                } else {
                    writebacks.push_back(cleanEvictBlk(tile_blk));
                }
            }
        }
        MJL_clearTileWritable(blk);
        MJL_resetTileInfo(blk);
    } else {
        if (blk->isValid()) {
            Addr repl_addr = tags->MJL_regenerateBlkAddr(blk->tag, blk->MJL_blkDir, blk->set);
            MSHR *repl_mshr = mshrQueue.MJL_findMatch(repl_addr, blk->MJL_blkDir, blk->isSecure());
            if (repl_mshr) {
                // must be an outstanding upgrade request
                // on a block we're about to replace...
                assert(!blk->isWritable() || blk->isDirty());
                assert(repl_mshr->needsWritable());
                // too hard to replace block with transient state
                // allocation failed, block not inserted
                return nullptr;
            } else {
                DPRINTF(Cache, "replacement: replacing %#llx (%s) with %#llx "
                        "(%s): %s\n", repl_addr, blk->isSecure() ? "s" : "ns",
                        addr, is_secure ? "s" : "ns",
                        blk->isDirty() ? "writeback" : "clean");

                if (blk->wasPrefetched()) {
                    unusedPrefetches++;
                }
                // Will send up Writeback/CleanEvict snoops via isCachedAbove
                // when pushing this writeback list into the write buffer.
                if (blk->isDirty() || writebackClean) {
                    // Save writeback packet for handling by caller
                    writebacks.push_back(writebackBlk(blk));
                } else {
                    writebacks.push_back(cleanEvictBlk(blk));
                }
                /* MJL_Test: what evict caused problem? 
                std::cout << this->name() << "::MJL_allocateBlock()Evict";
                std::cout << ": Addr(oct) = " << std::oct << repl_addr << std::dec;
                std::cout << ", BlkDir = " << blk->MJL_blkDir;
                std::cout << ", IsSecure = " << blk->isSecure();
                std::cout << ", IsDirty = " << blk->isDirty();
                std::cout << std::endl;
                */ 
            }
        }
    }

    return blk;
}
/* MJL_End */

void
Cache::invalidateBlock(CacheBlk *blk)
{
    if (blk != tempBlock)
        tags->invalidate(blk);
    blk->invalidate();
}
/* MJL_Begin */
void
Cache::MJL_invalidateTile(CacheBlk *blk)
{
    assert (blk != tempBlock);
    CacheBlk * tile_blk = nullptr;
    for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
        tile_blk = tags->MJL_findBlockByTile(blk, i);
        tags->invalidate(tile_blk);
        tile_blk->MJL_clearAllDirty();
        tile_blk->MJL_clearCrossValid();
        tile_blk->invalidate();
    }
}
/* MJL_End */

// Note that the reason we return a list of writebacks rather than
// inserting them directly in the write buffer is that this function
// is called by both atomic and timing-mode accesses, and in atomic
// mode we don't mess with the write buffer (we just perform the
// writebacks atomically once the original request is complete).
CacheBlk*
Cache::handleFill(PacketPtr pkt, CacheBlk *blk, PacketList &writebacks,
                  bool allocate)
{
    assert(pkt->isResponse() || pkt->cmd == MemCmd::WriteLineReq);
    Addr addr = pkt->getAddr();
    bool is_secure = pkt->isSecure();
#if TRACING_ON
    CacheBlk::State old_state = blk ? blk->status : 0;
#endif

    // When handling a fill, we should have no writes to this line.
    /* MJL_Begin */
    // MJL_TODO: should be packets operating on cachelines, and should have the same direction for cmd and data, need verification
    assert(pkt->MJL_sameCmdDataDir());
    assert(addr == MJL_blockAlign(addr, pkt->MJL_getCmdDir()));
    assert(!writeBuffer.MJL_findMatch(addr, pkt->MJL_getCmdDir(), is_secure));
    /* MJL_End */
    /* MJL_Comment
    assert(addr == blockAlign(addr));
    assert(!writeBuffer.findMatch(addr, is_secure));
    */

    if (blk == nullptr) {
        // better have read new data...
        assert(pkt->hasData());

        // only read responses and write-line requests have data;
        // note that we don't write the data here for write-line - that
        // happens in the subsequent call to satisfyRequest
        assert(pkt->isRead() || pkt->cmd == MemCmd::WriteLineReq);

        // need to do a replacement if allocating, otherwise we stick
        // with the temporary storage
        /* MJL_Begin */
        blk = allocate ? MJL_allocateBlock(addr, pkt->MJL_getDataDir(), is_secure, writebacks) : nullptr;
        /* MJL_End */
        /* MJL_Comment 
        blk = allocate ? allocateBlock(addr, is_secure, writebacks) : nullptr;
         */

        if (blk == nullptr) {
            // No replaceable block or a mostly exclusive
            // cache... just use temporary storage to complete the
            // current request and then get rid of it
            assert(!tempBlock->isValid());
            blk = tempBlock;
            /* MJL_Begin */
            if (MJL_2DCache) {
                tempBlock->set = tags->MJL_extractSet(addr, MemCmd::MJL_DirAttribute::MJL_IsRow);
                tempBlock->tag = tags->MJL_extractTag(addr, MemCmd::MJL_DirAttribute::MJL_IsRow);
            } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
                tempBlock->set = tags->MJL_extractSet(addr, pkt->MJL_getDataDir());
                tempBlock->tag = tags->MJL_extractTag(addr, pkt->MJL_getDataDir());
            } else {
                tempBlock->set = tags->extractSet(addr);
                tempBlock->tag = tags->extractTag(addr);
            }
            /* MJL_End */
            /* MJL_Comment 
            tempBlock->set = tags->extractSet(addr);
            tempBlock->tag = tags->extractTag(addr);
            */
            // @todo: set security state as well...
            DPRINTF(Cache, "using temp block for %#llx (%s)\n", addr,
                    is_secure ? "s" : "ns");
        } else {
            tags->insertBlock(pkt, blk);
        }

        // we should never be overwriting a valid block
        /* MJL_Begin */
        if (MJL_2DCache) {
            bool MJL_tileValid = false;
            for (int i = 0; i < 8; ++i) {
                MJL_tileValid |= blk->MJL_crossValid[i];
            }
            for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                MJL_tileValid |= (tags->MJL_findBlockByTile(blk, i))->isValid();
            }
            assert(!MJL_tileValid);
        } else {
            assert(!blk->isValid());
        }
        /* MJL_End */
        /* MJL_Comment
        assert(!blk->isValid());
        */
    } else {
        // existing block... probably an upgrade
        /* MJL_Begin */
        if (MJL_2DCache) {
            assert((blk->MJL_blkDir == MemCmd::MJL_DirAttribute::MJL_IsRow) && (blk->tag == tags->MJL_extractTag(addr, MemCmd::MJL_DirAttribute::MJL_IsRow)));
        } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
            assert((blk->MJL_blkDir == pkt->MJL_getCmdDir()) && (blk->tag == tags->MJL_extractTag(addr, pkt->MJL_getCmdDir())));
        } else {
            assert(blk->tag == tags->extractTag(addr));
        }
        /* MJL_End */
        /* MJL_Comment
        assert(blk->tag == tags->extractTag(addr));
        */
        // either we're getting new data or the block should already be valid
        assert(pkt->hasData() || blk->isValid());
        // don't clear block status... if block is already dirty we
        // don't want to lose that
    }

    /* MJL_Begin */
    if (MJL_2DCache) {
        if (is_secure) {
            for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                (tags->MJL_findBlockByTile(blk, i))->status |= BlkSecure;
            }
        }
    } else {
        if (is_secure)
            blk->status |= BlkSecure;
    }
    /* MJL_End */
    /* MJL_Comment
    if (is_secure)
        blk->status |= BlkSecure;
    */
    blk->status |= BlkValid | BlkReadable;

    // sanity check for whole-line writes, which should always be
    // marked as writable as part of the fill, and then later marked
    // dirty as part of satisfyRequest
    if (pkt->cmd == MemCmd::WriteLineReq) {
        assert(!pkt->hasSharers());
    }

    // here we deal with setting the appropriate state of the line,
    // and we start by looking at the hasSharers flag, and ignore the
    // cacheResponding flag (normally signalling dirty data) if the
    // packet has sharers, thus the line is never allocated as Owned
    // (dirty but not writable), and always ends up being either
    // Shared, Exclusive or Modified, see Packet::setCacheResponding
    // for more details
    if (!pkt->hasSharers()) {
        /* MJL_Begin */
        if (MJL_2DCache) {
            unsigned MJL_offset = pkt->MJL_getColOffset(blkSize);
            for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                (tags->MJL_findBlockByTile(blk, i))->status |= BlkWritable;
            }
            if (pkt->cacheResponding()) {
                if (pkt->MJL_dataIsRow()) {
                    blk->MJL_setWordDirtyPkt(pkt, blkSize);
                    blk->MJL_updateDirty();
                } else if (pkt->MJL_dataIsColumn()) {
                    for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                        (tags->MJL_findBlockByTile(blk, i))->MJL_wordDirty[MJL_offset/sizeof(uint64_t)] = pkt->MJL_wordDirty[i];
                    }
                }
            }
        } else {
        /* MJL_End */
        // we could get a writable line from memory (rather than a
        // cache) even in a read-only cache, note that we set this bit
        // even for a read-only cache, possibly revisit this decision
        blk->status |= BlkWritable;

        // check if we got this via cache-to-cache transfer (i.e., from a
        // cache that had the block in Modified or Owned state)
        if (pkt->cacheResponding()) {
            // we got the block in Modified state, and invalidated the
            // owners copy
            blk->status |= BlkDirty;
            /* MJL_Begin */
            if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
                blk->MJL_clearAllDirty();
                blk->MJL_setWordDirtyPkt(pkt, blkSize);
            }
            /* MJL_End */

            chatty_assert(!isReadOnly, "Should never see dirty snoop response "
                          "in read-only cache %s\n", name());
        }
        /* MJL_Begin */
        }
        /* MJL_End */
    }

    DPRINTF(Cache, "Block addr %#llx (%s) moving from state %x to %s\n",
            addr, is_secure ? "s" : "ns", old_state, blk->print());

    // if we got new data, copy it in (checking for a read response
    // and a response that has data is the same in the end)
    if (pkt->isRead()) {
        // sanity checks
        assert(pkt->hasData());
        assert(pkt->getSize() == blkSize);
        /* MJL_Begin */
        if (MJL_2DCache && pkt->MJL_dataIsColumn()) {
            unsigned MJL_offset = pkt->MJL_getColOffset(blkSize);
            for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                CacheBlk * tile_blk = tags->MJL_findBlockByTile(blk, i);
                pkt->MJL_wordDirty[i] = tile_blk->MJL_wordDirty[MJL_offset/sizeof(uint64_t)];
                std::memcpy(tile_blk->data + MJL_offset, pkt->getConstPtr<uint8_t>() + i*sizeof(uint64_t), sizeof(uint64_t));
            }
        } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
            assert(pkt->MJL_getDataDir() == blk->MJL_blkDir);
            pkt->MJL_setWordDirtyFromBlk(blk->MJL_wordDirty, blkSize);
            std::memcpy(blk->data, pkt->getConstPtr<uint8_t>(), blkSize);
        } else {
            std::memcpy(blk->data, pkt->getConstPtr<uint8_t>(), blkSize);
        }
        // MJL_TODO: needs to be changed for 2D Cache
        assert(pkt->MJL_getDataDir() == blk->MJL_blkDir);
        /* MJL_End */

        /* MJL_Comment
        std::memcpy(blk->data, pkt->getConstPtr<uint8_t>(), blkSize);
        */
    }
    // We pay for fillLatency here.
    blk->whenReady = clockEdge() + fillLatency * clockPeriod() +
        pkt->payloadDelay;

    return blk;
}


/////////////////////////////////////////////////////
//
// Snoop path: requests coming in from the memory side
//
/////////////////////////////////////////////////////

void
Cache::doTimingSupplyResponse(PacketPtr req_pkt, const uint8_t *blk_data,
                              bool already_copied, bool pending_inval)
{
    // sanity check
    assert(req_pkt->isRequest());
    assert(req_pkt->needsResponse());

    DPRINTF(Cache, "%s: for %s\n", __func__, req_pkt->print());
    // timing-mode snoop responses require a new packet, unless we
    // already made a copy...
    PacketPtr pkt = req_pkt;
    if (!already_copied)
        // do not clear flags, and allocate space for data if the
        // packet needs it (the only packets that carry data are read
        // responses)
        pkt = new Packet(req_pkt, false, req_pkt->isRead());

    assert(req_pkt->req->isUncacheable() || req_pkt->isInvalidate() ||
           pkt->hasSharers());
    pkt->makeTimingResponse();
    if (pkt->isRead()) {
        pkt->setDataFromBlock(blk_data, blkSize);
    }
    if (pkt->cmd == MemCmd::ReadResp && pending_inval) {
        // Assume we defer a response to a read from a far-away cache
        // A, then later defer a ReadExcl from a cache B on the same
        // bus as us. We'll assert cacheResponding in both cases, but
        // in the latter case cacheResponding will keep the
        // invalidation from reaching cache A. This special response
        // tells cache A that it gets the block to satisfy its read,
        // but must immediately invalidate it.
        pkt->cmd = MemCmd::ReadRespWithInvalidate;
    }
    // Here we consider forward_time, paying for just forward latency and
    // also charging the delay provided by the xbar.
    // forward_time is used as send_time in next allocateWriteBuffer().
    Tick forward_time = clockEdge(forwardLatency) + pkt->headerDelay;
    // Here we reset the timing of the packet.
    pkt->headerDelay = pkt->payloadDelay = 0;
    DPRINTF(CacheVerbose, "%s: created response: %s tick: %lu\n", __func__,
            pkt->print(), forward_time);
    memSidePort->schedTimingSnoopResp(pkt, forward_time, true);
}

uint32_t
Cache::handleSnoop(PacketPtr pkt, CacheBlk *blk, bool is_timing,
                   bool is_deferred, bool pending_inval)
{
    DPRINTF(CacheVerbose, "%s: for %s\n", __func__, pkt->print());
    // deferred snoops can only happen in timing mode
    assert(!(is_deferred && !is_timing));
    // pending_inval only makes sense on deferred snoops
    assert(!(pending_inval && !is_deferred));
    assert(pkt->isRequest());

    // the packet may get modified if we or a forwarded snooper
    // responds in atomic mode, so remember a few things about the
    // original packet up front
    bool invalidate = pkt->isInvalidate();
    bool M5_VAR_USED needs_writable = pkt->needsWritable();

    // at the moment we could get an uncacheable write which does not
    // have the invalidate flag, and we need a suitable way of dealing
    // with this case
    panic_if(invalidate && pkt->req->isUncacheable(),
             "%s got an invalidating uncacheable snoop request %s",
             name(), pkt->print());

    uint32_t snoop_delay = 0;

    if (forwardSnoops) {
        // first propagate snoop upward to see if anyone above us wants to
        // handle it.  save & restore packet src since it will get
        // rewritten to be relative to cpu-side bus (if any)
        bool alreadyResponded = pkt->cacheResponding();
        if (is_timing) {
            // copy the packet so that we can clear any flags before
            // forwarding it upwards, we also allocate data (passing
            // the pointer along in case of static data), in case
            // there is a snoop hit in upper levels
            Packet snoopPkt(pkt, true, true);
            snoopPkt.setExpressSnoop();
            // the snoop packet does not need to wait any additional
            // time
            snoopPkt.headerDelay = snoopPkt.payloadDelay = 0;
            cpuSidePort->sendTimingSnoopReq(&snoopPkt);

            // add the header delay (including crossbar and snoop
            // delays) of the upward snoop to the snoop delay for this
            // cache
            snoop_delay += snoopPkt.headerDelay;

            if (snoopPkt.cacheResponding()) {
                // cache-to-cache response from some upper cache
                assert(!alreadyResponded);
                pkt->setCacheResponding();
            }
            // upstream cache has the block, or has an outstanding
            // MSHR, pass the flag on
            if (snoopPkt.hasSharers()) {
                pkt->setHasSharers();
            }
            // If this request is a prefetch or clean evict and an upper level
            // signals block present, make sure to propagate the block
            // presence to the requester.
            if (snoopPkt.isBlockCached()) {
                pkt->setBlockCached();
            }
        } else {
            cpuSidePort->sendAtomicSnoop(pkt);
            if (!alreadyResponded && pkt->cacheResponding()) {
                // cache-to-cache response from some upper cache:
                // forward response to original requester
                assert(pkt->isResponse());
            }
        }
    }

    /* MJL_Begin */
    if (MJL_2DCache && (!blk || (!blk->isValid() && !blk->MJL_hasCrossValid()))) {
        DPRINTF(CacheVerbose, "%s: snoop miss for %s\n", __func__,
                pkt->print());
        if (is_deferred) {
            // we no longer have the block, and will not respond, but a
            // packet was allocated in MSHR::handleSnoop and we have
            // to delete it
            assert(pkt->needsResponse());

            // we have passed the block to a cache upstream, that
            // cache should be responding
            assert(pkt->cacheResponding());

            delete pkt;
        }
        return snoop_delay;
    } else /* MJL_End */if (!blk || !blk->isValid()) {
        DPRINTF(CacheVerbose, "%s: snoop miss for %s\n", __func__,
                pkt->print());
        if (is_deferred) {
            // we no longer have the block, and will not respond, but a
            // packet was allocated in MSHR::handleSnoop and we have
            // to delete it
            assert(pkt->needsResponse());

            // we have passed the block to a cache upstream, that
            // cache should be responding
            assert(pkt->cacheResponding());

            delete pkt;
        }
        return snoop_delay;
    } else {
        DPRINTF(Cache, "%s: snoop hit for %s, old state is %s\n", __func__,
                pkt->print(), blk->print());
    }

    chatty_assert(!(isReadOnly && blk->isDirty()),
                  "Should never have a dirty block in a read-only cache %s\n",
                  name());

    // We may end up modifying both the block state and the packet (if
    // we respond in atomic mode), so just figure out what to do now
    // and then do it later. We respond to all snoops that need
    // responses provided we have the block in dirty state. The
    // invalidation itself is taken care of below.
    /* MJL_Begin */
    bool respond = false;
    if (MJL_2DCache && pkt->MJL_cmdIsColumn()) {
        bool MJL_colDirty = false;
        unsigned MJL_offset = pkt->MJL_getColOffset(blkSize);
        for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
            MJL_colDirty |= (tags->MJL_findBlockByTile(blk, i))->MJL_wordDirty[MJL_offset/sizeof(uint64_t)];
        }
        respond = MJL_colDirty && pkt->needsResponse();
    } else {
        respond = blk->isDirty() && pkt->needsResponse();
    }
    /* MJL_End */
    /* MJL_Comment
    bool respond = blk->isDirty() && pkt->needsResponse();
    */
    bool have_writable = blk->isWritable();

    // Invalidate any prefetch's from below that would strip write permissions
    // MemCmd::HardPFReq is only observed by upstream caches.  After missing
    // above and in it's own cache, a new MemCmd::ReadReq is created that
    // downstream caches observe.
    if (pkt->mustCheckAbove()) {
        DPRINTF(Cache, "Found addr %#llx in upper level cache for snoop %s "
                "from lower cache\n", pkt->getAddr(), pkt->print());
        pkt->setBlockCached();
        return snoop_delay;
    }

    if (pkt->isRead() && !invalidate) {
        // reading without requiring the line in a writable state
        assert(!needs_writable);
        pkt->setHasSharers();

        // if the requesting packet is uncacheable, retain the line in
        // the current state, otherwhise unset the writable flag,
        // which means we go from Modified to Owned (and will respond
        // below), remain in Owned (and will respond below), from
        // Exclusive to Shared, or remain in Shared
        if (!pkt->req->isUncacheable())/* MJL_Begin */ {
            if (MJL_2DCache) {
                for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                    (tags->MJL_findBlockByTile(blk, i))->status &= ~BlkWritable;
                }
            } else {
                blk->status &= ~BlkWritable;
            }
        }/* MJL_End */
            /* MJL_Comment
            blk->status &= ~BlkWritable;
            */
    }

    if (respond) {
        // prevent anyone else from responding, cache as well as
        // memory, and also prevent any memory from even seeing the
        // request
        pkt->setCacheResponding();
        if (have_writable) {
            // inform the cache hierarchy that this cache had the line
            // in the Modified state so that we avoid unnecessary
            // invalidations (see Packet::setResponderHadWritable)
            pkt->setResponderHadWritable();

            // in the case of an uncacheable request there is no point
            // in setting the responderHadWritable flag, but since the
            // recipient does not care there is no harm in doing so
        } else {
            // if the packet has needsWritable set we invalidate our
            // copy below and all other copies will be invalidates
            // through express snoops, and if needsWritable is not set
            // we already called setHasSharers above
        }

        // if we are returning a writable and dirty (Modified) line,
        // we should be invalidating the line
        panic_if(!invalidate && !pkt->hasSharers(),
                 "%s is passing a Modified line through %s, "
                 "but keeping the block", name(), pkt->print());

        if (is_timing) {
            /* MJL_Begin */
            if (MJL_2DCache && pkt->MJL_cmdIsColumn()) {
                uint8_t MJL_tempData[blkSize];
                unsigned MJL_offset = (pkt->MJL_getColOffset(blkSize)/sizeof(uint64_t))*sizeof(uint64_t);
                for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                    std::memcpy(&MJL_tempData[i * sizeof(uint64_t)], (tags->MJL_findBlockByTile(blk, i))->data + MJL_offset, sizeof(uint64_t));
                }
                doTimingSupplyResponse(pkt, MJL_tempData, is_deferred, pending_inval);
            } else {
                doTimingSupplyResponse(pkt, blk->data, is_deferred, pending_inval);
            }
            /* MJL_End */
            /* MJL_Comment
            doTimingSupplyResponse(pkt, blk->data, is_deferred, pending_inval);
            */
        } else {
            pkt->makeAtomicResponse();
            // packets such as upgrades do not actually have any data
            // payload
            if (pkt->hasData())
            /* MJL_Begin */
            {
                /* MJL_Test 
                std::cout << "MJL_setFromBlock: set " << blk->set << std::endl;
                 */
                if (MJL_2DCache && pkt->MJL_cmdIsColumn()) {
                    uint8_t MJL_tempData[blkSize];
                    bool MJL_tempWordDirty[blkSize/sizeof(uint64_t)];
                    unsigned MJL_offset = (pkt->MJL_getColOffset(blkSize)/sizeof(uint64_t))*sizeof(uint64_t);
                    for (int i = 0; i < blkSize/sizeof(uint64_t); ++i) {
                        std::memcpy(&MJL_tempData[i * sizeof(uint64_t)], (tags->MJL_findBlockByTile(blk, i))->data + MJL_offset, sizeof(uint64_t));
                        MJL_tempWordDirty[i] = (tags->MJL_findBlockByTile(blk, i))->MJL_wordDirty[MJL_offset/sizeof(uint64_t)];
                    }
                    pkt->setDataFromBlock(MJL_tempData, blkSize);
                    pkt->MJL_setWordDirtyFromBlk(MJL_tempWordDirty,blkSize);
                } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
                    pkt->setDataFromBlock(blk->data, blkSize);
                    pkt->MJL_setWordDirtyFromBlk(blk->MJL_wordDirty,blkSize);
                } else {
                    pkt->setDataFromBlock(blk->data, blkSize);
                }
            }
            /* MJL_End */
            /* MJL_Comment 
                pkt->setDataFromBlock(blk->data, blkSize);
            */
        }
    }

    if (!respond && is_deferred) {
        assert(pkt->needsResponse());

        // if we copied the deferred packet with the intention to
        // respond, but are not responding, then a cache above us must
        // be, and we can use this as the indication of whether this
        // is a packet where we created a copy of the request or not
        if (!pkt->cacheResponding()) {
            delete pkt->req;
        }

        delete pkt;
    }

    // Do this last in case it deallocates block data or something
    // like that
    if (invalidate) {
        /* MJL_Begin */
        if (MJL_2DCache && (blk != tempBlock)) {
            MJL_invalidateTile(blk);
        } else 
        /* MJL_End */
        invalidateBlock(blk);
    }

    DPRINTF(Cache, "new state is %s\n", blk->print());

    return snoop_delay;
}


void
Cache::recvTimingSnoopReq(PacketPtr pkt)
{
    DPRINTF(CacheVerbose, "%s: for %s\n", __func__, pkt->print());

    // Snoops shouldn't happen when bypassing caches
    assert(!system->bypassCaches());

    // no need to snoop requests that are not in range
    if (!inRange(pkt->getAddr())) {
        return;
    }

    bool is_secure = pkt->isSecure();
    /* MJL_Begin */
    CacheBlk *blk = nullptr;
    Addr blk_addr;
    MSHR *mshr = nullptr;
    if (MJL_2DCache && pkt->MJL_cmdIsColumn()) {
        blk = tags->MJL_findCrossBlock(pkt->getAddr(),CacheBlk::MJL_CacheBlkDir::MJL_IsRow, is_secure, pkt->MJL_getColOffset(blkSize));

        blk_addr = MJL_blockAlign(pkt->getAddr(), pkt->MJL_getCmdDir());
        mshr = mshrQueue.MJL_findMatch(blk_addr, pkt->MJL_getCmdDir(), is_secure);
    } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
        blk = tags->MJL_findBlock(pkt->getAddr(), pkt->MJL_getCmdDir(), is_secure);

        blk_addr = MJL_blockAlign(pkt->getAddr(), pkt->MJL_getCmdDir());
        mshr = mshrQueue.MJL_findMatch(blk_addr, pkt->MJL_getCmdDir(), is_secure);
    } else {
        blk = tags->findBlock(pkt->getAddr(), is_secure);

        blk_addr = blockAlign(pkt->getAddr());
        mshr = mshrQueue.findMatch(blk_addr, is_secure);
    }
    /* MJL_End */
    /* MJL_Comment 
    CacheBlk *blk = tags->findBlock(pkt->getAddr(), is_secure);

    Addr blk_addr = blockAlign(pkt->getAddr());
    MSHR *mshr = mshrQueue.findMatch(blk_addr, is_secure);
    */

    // Update the latency cost of the snoop so that the crossbar can
    // account for it. Do not overwrite what other neighbouring caches
    // have already done, rather take the maximum. The update is
    // tentative, for cases where we return before an upward snoop
    // happens below.
    pkt->snoopDelay = std::max<uint32_t>(pkt->snoopDelay,
                                         lookupLatency * clockPeriod());

    // Inform request(Prefetch, CleanEvict or Writeback) from below of
    // MSHR hit, set setBlockCached.
    if (mshr && pkt->mustCheckAbove()) {
        DPRINTF(Cache, "Setting block cached for %s from lower cache on "
                "mshr hit\n", pkt->print());
        pkt->setBlockCached();
        return;
    }

    // Let the MSHR itself track the snoop and decide whether we want
    // to go ahead and do the regular cache snoop
    if (mshr && mshr->handleSnoop(pkt, order++)) {
        DPRINTF(Cache, "Deferring snoop on in-service MSHR to blk %#llx (%s)."
                "mshrs: %s\n", blk_addr, is_secure ? "s" : "ns",
                mshr->print());

        if (mshr->getNumTargets() > numTarget)
            warn("allocating bonus target for snoop"); //handle later
        return;
    }

    //We also need to check the writeback buffers and handle those
    /* MJL_Begin */
    WriteQueueEntry *wb_entry = writeBuffer.MJL_findMatch(blk_addr, pkt->MJL_getCmdDir(), is_secure);
    /* MJL_End */
    /* MJL_Comment
    WriteQueueEntry *wb_entry = writeBuffer.findMatch(blk_addr, is_secure);
    */
    if (wb_entry) {
        DPRINTF(Cache, "Snoop hit in writeback to addr %#llx (%s)\n",
                pkt->getAddr(), is_secure ? "s" : "ns");
        // Expect to see only Writebacks and/or CleanEvicts here, both of
        // which should not be generated for uncacheable data.
        assert(!wb_entry->isUncacheable());
        // There should only be a single request responsible for generating
        // Writebacks/CleanEvicts.
        assert(wb_entry->getNumTargets() == 1);
        PacketPtr wb_pkt = wb_entry->getTarget()->pkt;
        assert(wb_pkt->isEviction());

        if (pkt->isEviction()) {
            // if the block is found in the write queue, set the BLOCK_CACHED
            // flag for Writeback/CleanEvict snoop. On return the snoop will
            // propagate the BLOCK_CACHED flag in Writeback packets and prevent
            // any CleanEvicts from travelling down the memory hierarchy.
            pkt->setBlockCached();
            DPRINTF(Cache, "%s: Squashing %s from lower cache on writequeue "
                    "hit\n", __func__, pkt->print());
            return;
        }

        // conceptually writebacks are no different to other blocks in
        // this cache, so the behaviour is modelled after handleSnoop,
        // the difference being that instead of querying the block
        // state to determine if it is dirty and writable, we use the
        // command and fields of the writeback packet
        // MJL_TODO: should check other direction as well for coherency
        bool respond = wb_pkt->cmd == MemCmd::WritebackDirty &&
            pkt->needsResponse();
        bool have_writable = !wb_pkt->hasSharers();
        bool invalidate = pkt->isInvalidate();

        if (!pkt->req->isUncacheable() && pkt->isRead() && !invalidate) {
            assert(!pkt->needsWritable());
            pkt->setHasSharers();
            wb_pkt->setHasSharers();
        }

        if (respond) {
            pkt->setCacheResponding();

            if (have_writable) {
                pkt->setResponderHadWritable();
            }

            doTimingSupplyResponse(pkt, wb_pkt->getConstPtr<uint8_t>(),
                                   false, false);
        }

        if (invalidate) {
            // Invalidation trumps our writeback... discard here
            // Note: markInService will remove entry from writeback buffer.
            markInService(wb_entry);
            delete wb_pkt;
        }
    }

    // If this was a shared writeback, there may still be
    // other shared copies above that require invalidation.
    // We could be more selective and return here if the
    // request is non-exclusive or if the writeback is
    // exclusive.
    uint32_t snoop_delay = handleSnoop(pkt, blk, true, false, false);

    // Override what we did when we first saw the snoop, as we now
    // also have the cost of the upwards snoops to account for
    pkt->snoopDelay = std::max<uint32_t>(pkt->snoopDelay, snoop_delay +
                                         lookupLatency * clockPeriod());
}

bool
Cache::CpuSidePort::recvTimingSnoopResp(PacketPtr pkt)
{
    // Express snoop responses from master to slave, e.g., from L1 to L2
    cache->recvTimingSnoopResp(pkt);
    return true;
}

Tick
Cache::recvAtomicSnoop(PacketPtr pkt)
{
    // Snoops shouldn't happen when bypassing caches
    assert(!system->bypassCaches());

    // no need to snoop requests that are not in range.
    if (!inRange(pkt->getAddr())) {
        return 0;
    }

    /* MJL_Begin */
    // MJL_TODO: needs to be changed for 2D Cache
    CacheBlk *blk = nullptr;
    if (MJL_2DCache && pkt->MJL_cmdIsColumn()) {
        blk = tags->MJL_findCrossBlock(pkt->getAddr(), CacheBlk::MJL_CacheBlkDir::MJL_IsRow, pkt->isSecure(), pkt->MJL_getColOffset(blkSize));
    } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
        blk = tags->MJL_findBlock(pkt->getAddr(), pkt->MJL_getCmdDir(), pkt->isSecure());
    } else {
        blk = tags->findBlock(pkt->getAddr(), pkt->isSecure());
    }
    /* MJL_End */
    /* MJL_Comment
    CacheBlk *blk = tags->findBlock(pkt->getAddr(), pkt->isSecure());
    */
    uint32_t snoop_delay = handleSnoop(pkt, blk, false, false, false);
    return snoop_delay + lookupLatency * clockPeriod();
}


QueueEntry*
Cache::getNextQueueEntry()
{
    // Check both MSHR queue and write buffer for potential requests,
    // note that null does not mean there is no request, it could
    // simply be that it is not ready
    MSHR *miss_mshr  = mshrQueue.getNext();
    WriteQueueEntry *wq_entry = writeBuffer.getNext();

    // If we got a write buffer request ready, first priority is a
    // full write buffer, otherwise we favour the miss requests
    if (wq_entry && (writeBuffer.isFull() || !miss_mshr)) {
        // need to search MSHR queue for conflicting earlier miss.
        MSHR *conflict_mshr =
        /* MJL_Begin */
            nullptr;
        if (!MJL_2DCache && (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos)) {
            conflict_mshr = mshrQueue.MJL_findPending(wq_entry->blkAddr, wq_entry->MJL_qEntryDir, 
                                  wq_entry->isSecure);

            // Check other direction conflict
            Addr MJL_crossBlkAddr = wq_entry->blkAddr;
            MSHR *temp_conflict_mshr = conflict_mshr;
            for (Addr MJL_offset = 0; MJL_offset < blkSize; MJL_offset = MJL_offset + sizeof(uint64_t)) {
                MJL_crossBlkAddr = MJL_addOffsetAddr(wq_entry->blkAddr, wq_entry->MJL_qEntryDir, MJL_offset);
                if (wq_entry->MJL_qEntryDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
                    MJL_crossBlkAddr = MJL_blockAlign(MJL_crossBlkAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
                    temp_conflict_mshr = mshrQueue.MJL_findPending(MJL_crossBlkAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn, 
                                        wq_entry->isSecure);
                } else if (wq_entry->MJL_qEntryDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) {
                    MJL_crossBlkAddr = MJL_blockAlign(MJL_crossBlkAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow);
                    temp_conflict_mshr = mshrQueue.MJL_findPending(MJL_crossBlkAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow, 
                                        wq_entry->isSecure);
                }
                if (conflict_mshr) {
                    if (temp_conflict_mshr && (temp_conflict_mshr->order < conflict_mshr->order) && (temp_conflict_mshr->order < wq_entry->order)) {
                        conflict_mshr = temp_conflict_mshr;
                    }
                } else {
                    if (temp_conflict_mshr && (temp_conflict_mshr->order < wq_entry->order)) {
                        conflict_mshr = temp_conflict_mshr;
                    }
                }
            }
        } else {
            conflict_mshr = mshrQueue.findPending(wq_entry->blkAddr,
                                  wq_entry->isSecure);
        }
        /* MJL_End */
        /* MJL_Comment
            mshrQueue.findPending(wq_entry->blkAddr,
                                  wq_entry->isSecure);
        */

        if (conflict_mshr && conflict_mshr->order < wq_entry->order) {
            // Service misses in order until conflict is cleared.
            return conflict_mshr;

            // @todo Note that we ignore the ready time of the conflict here
        }

        // No conflicts; issue write
        return wq_entry;
    } else if (miss_mshr) {
        // need to check for conflicting earlier writeback
        WriteQueueEntry *conflict_mshr =
        /* MJL_Begin */
            nullptr;
        if (!MJL_2DCache && (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos)) {
            conflict_mshr = writeBuffer.MJL_findPending(miss_mshr->blkAddr, miss_mshr->MJL_qEntryDir, 
                                    miss_mshr->isSecure);

            // Check other direction conflict
            Addr MJL_crossBlkAddr = miss_mshr->blkAddr;
            WriteQueueEntry *temp_conflict_mshr = conflict_mshr;
            for (Addr MJL_offset = 0; MJL_offset < blkSize; MJL_offset = MJL_offset + sizeof(uint64_t)) {
                MJL_crossBlkAddr = MJL_addOffsetAddr(miss_mshr->blkAddr, miss_mshr->MJL_qEntryDir, MJL_offset);
                if (miss_mshr->MJL_qEntryDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
                    MJL_crossBlkAddr = MJL_blockAlign(MJL_crossBlkAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn);
                    temp_conflict_mshr = writeBuffer.MJL_findPending(MJL_crossBlkAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsColumn, 
                                        miss_mshr->isSecure);
                } else if (miss_mshr->MJL_qEntryDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) {
                    MJL_crossBlkAddr = MJL_blockAlign(MJL_crossBlkAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow);
                    temp_conflict_mshr = writeBuffer.MJL_findPending(MJL_crossBlkAddr, CacheBlk::MJL_CacheBlkDir::MJL_IsRow, 
                                        miss_mshr->isSecure);
                }
                if (conflict_mshr) {
                    if (temp_conflict_mshr && (temp_conflict_mshr->order < conflict_mshr->order) && (temp_conflict_mshr->order < miss_mshr->order)) {
                        conflict_mshr = temp_conflict_mshr;
                    }
                } else {
                    if (temp_conflict_mshr && (temp_conflict_mshr->order < miss_mshr->order)) {
                        conflict_mshr = temp_conflict_mshr;
                    }
                }
            }
        } else {
            conflict_mshr = writeBuffer.findPending(miss_mshr->blkAddr,
                                    miss_mshr->isSecure);
        }
        /* MJL_End */
        /* MJL_Comment
            writeBuffer.findPending(miss_mshr->blkAddr,
                                    miss_mshr->isSecure);
        */
        if (conflict_mshr) {
            // not sure why we don't check order here... it was in the
            // original code but commented out.

            // The only way this happens is if we are
            // doing a write and we didn't have permissions
            // then subsequently saw a writeback (owned got evicted)
            // We need to make sure to perform the writeback first
            // To preserve the dirty data, then we can issue the write

            // should we return wq_entry here instead?  I.e. do we
            // have to flush writes in order?  I don't think so... not
            // for Alpha anyway.  Maybe for x86?
            return conflict_mshr;

            // @todo Note that we ignore the ready time of the conflict here
        }

        // No conflicts; issue read
        return miss_mshr;
    }

    // fall through... no pending requests.  Try a prefetch.
    assert(!miss_mshr && !wq_entry);
    if (prefetcher && mshrQueue.canPrefetch()) {
        // If we have a miss queue slot, we can try a prefetch
        PacketPtr pkt = prefetcher->getPacket();
        if (pkt) {
            /* MJL_Begin */
            Addr pf_addr;
            if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
                pf_addr = MJL_blockAlign(pkt->getAddr(), pkt->MJL_getCmdDir());
            } else {
                pf_addr = blockAlign(pkt->getAddr());
            }
            if (((this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) && !tags->MJL_findBlock(pf_addr, pkt->MJL_getCmdDir(), pkt->isSecure()) &&
                !mshrQueue.MJL_findMatch(pf_addr, pkt->MJL_getCmdDir(), pkt->isSecure()) &&
                !writeBuffer.MJL_findMatch(pf_addr, pkt->MJL_getCmdDir(), pkt->isSecure())) || ((this->name().find("dcache") == std::string::npos && this->name().find("l2") == std::string::npos) && (!tags->findBlock(pf_addr, pkt->isSecure()) &&
                !mshrQueue.findMatch(pf_addr, pkt->isSecure()) &&
                !writeBuffer.findMatch(pf_addr, pkt->isSecure())))) {
            /* MJL_End */
            /* MJL_Comment
            Addr pf_addr = blockAlign(pkt->getAddr());
            if (!tags->findBlock(pf_addr, pkt->isSecure()) &&
                !mshrQueue.findMatch(pf_addr, pkt->isSecure()) &&
                !writeBuffer.findMatch(pf_addr, pkt->isSecure())) {
            */
                // Update statistic on number of prefetches issued
                // (hwpf_mshr_misses)
                assert(pkt->req->masterId() < system->maxMasters());
                mshr_misses[pkt->cmdToIndex()][pkt->req->masterId()]++;

                // allocate an MSHR and return it, note
                // that we send the packet straight away, so do not
                // schedule the send
                return allocateMissBuffer(pkt, curTick(), false);
            } else {
                // free the request and packet
                delete pkt->req;
                delete pkt;
            }
        }
    }

    return nullptr;
}

bool
Cache::isCachedAbove(PacketPtr pkt, bool is_timing) const
{
    if (!forwardSnoops)
        return false;
    // Mirroring the flow of HardPFReqs, the cache sends CleanEvict and
    // Writeback snoops into upper level caches to check for copies of the
    // same block. Using the BLOCK_CACHED flag with the Writeback/CleanEvict
    // packet, the cache can inform the crossbar below of presence or absence
    // of the block.
    if (is_timing) {
        Packet snoop_pkt(pkt, true, false);
        snoop_pkt.setExpressSnoop();
        // Assert that packet is either Writeback or CleanEvict and not a
        // prefetch request because prefetch requests need an MSHR and may
        // generate a snoop response.
        assert(pkt->isEviction());
        snoop_pkt.senderState = nullptr;
        cpuSidePort->sendTimingSnoopReq(&snoop_pkt);
        // Writeback/CleanEvict snoops do not generate a snoop response.
        assert(!(snoop_pkt.cacheResponding()));
        return snoop_pkt.isBlockCached();
    } else {
        cpuSidePort->sendAtomicSnoop(pkt);
        return pkt->isBlockCached();
    }
}

Tick
Cache::nextQueueReadyTime() const
{
    Tick nextReady = std::min(mshrQueue.nextReadyTime(),
                              writeBuffer.nextReadyTime());

    // Don't signal prefetch ready time if no MSHRs available
    // Will signal once enoguh MSHRs are deallocated
    if (prefetcher && mshrQueue.canPrefetch()) {
        nextReady = std::min(nextReady,
                             prefetcher->nextPrefetchReadyTime());
    }

    return nextReady;
}

bool
Cache::sendMSHRQueuePacket(MSHR* mshr)
{
    assert(mshr);

    // use request from 1st target
    PacketPtr tgt_pkt = mshr->getTarget()->pkt;

    DPRINTF(Cache, "%s: MSHR %s\n", __func__, tgt_pkt->print());

    /* MJL_Begin */
    CacheBlk *blk = nullptr;
    if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos) {
        blk = tags->MJL_findBlock(mshr->blkAddr, mshr->MJL_qEntryDir, mshr->isSecure);
    } else {
        blk = tags->findBlock(mshr->blkAddr, mshr->isSecure);
    }
    /* MJL_End */
    /* MJL_Comment
    CacheBlk *blk = tags->findBlock(mshr->blkAddr, mshr->isSecure);
    */

    if (tgt_pkt->cmd == MemCmd::HardPFReq && forwardSnoops) {
        // we should never have hardware prefetches to allocated
        // blocks
        assert(blk == nullptr);

        // We need to check the caches above us to verify that
        // they don't have a copy of this block in the dirty state
        // at the moment. Without this check we could get a stale
        // copy from memory that might get used in place of the
        // dirty one.
        Packet snoop_pkt(tgt_pkt, true, false);
        snoop_pkt.setExpressSnoop();
        // We are sending this packet upwards, but if it hits we will
        // get a snoop response that we end up treating just like a
        // normal response, hence it needs the MSHR as its sender
        // state
        snoop_pkt.senderState = mshr;
        cpuSidePort->sendTimingSnoopReq(&snoop_pkt);

        // Check to see if the prefetch was squashed by an upper cache (to
        // prevent us from grabbing the line) or if a Check to see if a
        // writeback arrived between the time the prefetch was placed in
        // the MSHRs and when it was selected to be sent or if the
        // prefetch was squashed by an upper cache.

        // It is important to check cacheResponding before
        // prefetchSquashed. If another cache has committed to
        // responding, it will be sending a dirty response which will
        // arrive at the MSHR allocated for this request. Checking the
        // prefetchSquash first may result in the MSHR being
        // prematurely deallocated.
        if (snoop_pkt.cacheResponding()) {
            auto M5_VAR_USED r = outstandingSnoop.insert(snoop_pkt.req);
            assert(r.second);

            // if we are getting a snoop response with no sharers it
            // will be allocated as Modified
            bool pending_modified_resp = !snoop_pkt.hasSharers();
            markInService(mshr, pending_modified_resp);

            DPRINTF(Cache, "Upward snoop of prefetch for addr"
                    " %#x (%s) hit\n",
                    tgt_pkt->getAddr(), tgt_pkt->isSecure()? "s": "ns");
            return false;
        }

        if (snoop_pkt.isBlockCached()) {
            DPRINTF(Cache, "Block present, prefetch squashed by cache.  "
                    "Deallocating mshr target %#x.\n",
                    mshr->blkAddr);

            // Deallocate the mshr target
            if (mshrQueue.forceDeallocateTarget(mshr)) {
                // Clear block if this deallocation resulted freed an
                // mshr when all had previously been utilized
                clearBlocked(Blocked_NoMSHRs);
            }
            return false;
        }
    }

    // either a prefetch that is not present upstream, or a normal
    // MSHR request, proceed to get the packet to send downstream
    PacketPtr pkt = createMissPacket(tgt_pkt, blk, mshr->needsWritable());

    mshr->isForward = (pkt == nullptr);

    if (mshr->isForward) {
        // not a cache block request, but a response is expected
        // make copy of current packet to forward, keep current
        // copy for response handling
        pkt = new Packet(tgt_pkt, false, true);
        assert(!pkt->isWrite());
    }

    // play it safe and append (rather than set) the sender state,
    // as forwarded packets may already have existing state
    pkt->pushSenderState(mshr);

    if (!memSidePort->sendTimingReq(pkt)) {
        // we are awaiting a retry, but we
        // delete the packet and will be creating a new packet
        // when we get the opportunity
        delete pkt;

        // note that we have now masked any requestBus and
        // schedSendEvent (we will wait for a retry before
        // doing anything), and this is so even if we do not
        // care about this packet and might override it before
        // it gets retried
        return true;
    } else {
        // As part of the call to sendTimingReq the packet is
        // forwarded to all neighbouring caches (and any caches
        // above them) as a snoop. Thus at this point we know if
        // any of the neighbouring caches are responding, and if
        // so, we know it is dirty, and we can determine if it is
        // being passed as Modified, making our MSHR the ordering
        // point
        bool pending_modified_resp = !pkt->hasSharers() &&
            pkt->cacheResponding();
        markInService(mshr, pending_modified_resp);
        return false;
    }
}

bool
Cache::sendWriteQueuePacket(WriteQueueEntry* wq_entry)
{
    assert(wq_entry);

    // always a single target for write queue entries
    PacketPtr tgt_pkt = wq_entry->getTarget()->pkt;

    DPRINTF(Cache, "%s: write %s\n", __func__, tgt_pkt->print());

    // forward as is, both for evictions and uncacheable writes
    if (!memSidePort->sendTimingReq(tgt_pkt)) {
        // note that we have now masked any requestBus and
        // schedSendEvent (we will wait for a retry before
        // doing anything), and this is so even if we do not
        // care about this packet and might override it before
        // it gets retried
        return true;
    } else {
        markInService(wq_entry);
        return false;
    }
}

void
Cache::serialize(CheckpointOut &cp) const
{
    bool dirty(isDirty());

    if (dirty) {
        warn("*** The cache still contains dirty data. ***\n");
        warn("    Make sure to drain the system using the correct flags.\n");
        warn("    This checkpoint will not restore correctly and dirty data "
             "    in the cache will be lost!\n");
    }

    // Since we don't checkpoint the data in the cache, any dirty data
    // will be lost when restoring from a checkpoint of a system that
    // wasn't drained properly. Flag the checkpoint as invalid if the
    // cache contains dirty data.
    bool bad_checkpoint(dirty);
    SERIALIZE_SCALAR(bad_checkpoint);
}

void
Cache::unserialize(CheckpointIn &cp)
{
    bool bad_checkpoint;
    UNSERIALIZE_SCALAR(bad_checkpoint);
    if (bad_checkpoint) {
        fatal("Restoring from checkpoints with dirty caches is not supported "
              "in the classic memory system. Please remove any caches or "
              " drain them properly before taking checkpoints.\n");
    }
}

///////////////
//
// CpuSidePort
//
///////////////

AddrRangeList
Cache::CpuSidePort::getAddrRanges() const
{
    return cache->getAddrRanges();
}

bool
Cache::CpuSidePort::recvTimingReq(PacketPtr pkt)
{
    /* MJL_Begin */ 
    // Set common system information to propagate the information everywhere
    pkt->req->MJL_cachelineSize = cache->blkSize;
    pkt->req->MJL_rowWidth = cache->MJL_rowWidth;

    // Assign direction preference to packet based on PC at L1D$
    if ((pkt->req->hasPC())
        && (this->name().find("dcache") != std::string::npos)
        && (cache->MJL_PC2DirMap.find(pkt->req->getPC()) != cache->MJL_PC2DirMap.end())) {
        CacheBlk::MJL_CacheBlkDir InputDir = cache->MJL_PC2DirMap.find(pkt->req->getPC())->second;
        pkt->cmd.MJL_setCmdDir(InputDir);
        pkt->req->MJL_setReqDir(InputDir);
        pkt->MJL_setDataDir(InputDir);
    }

    // Assign dirty bits for write requests at L1D$
    if ((this->name().find("dcache") != std::string::npos) && pkt->isWrite()) {
        pkt->MJL_setAllDirty();
    }

    /* MJL_Test: Packet information output */
    if ((this->name().find("dcache") != std::string::npos) && !blocked && !mustSendRetry
         && cache->MJL_colVecHandler.MJL_ColVecList.find(pkt->req->getPC()) != cache->MJL_colVecHandler.MJL_ColVecList.end() && pkt->MJL_cmdIsColumn()) { // Debug for column vec
        std::cout << this->name() << "::recvTimingReq";
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
        std::cout << ", Data(hex) = " << std::hex;
        if (pkt->hasData()) {
            uint64_t MJL_data = 0;
            std::memcpy(&MJL_data, pkt->getConstPtr<uint8_t>(), pkt->getSize());
            std::cout << "word[0] " << std::hex << MJL_data << std::dec;
            for (unsigned i = sizeof(uint64_t); i < pkt->getSize(); i = i + sizeof(uint64_t)) {
                MJL_data = 0;
                std::memcpy(&MJL_data, pkt->getConstPtr<uint8_t>() + i, std::min(sizeof(uint64_t), pkt->getSize() - (Addr)i));
                std::cout << " | word[" << i/sizeof(uint64_t) << "] " <<  MJL_data;
            }         
        } else {
            std::cout << "noData";
        }
        std::cout << std::dec;
        std::cout << ", Time = " << pkt->req->time();
        std::cout << std::endl;
    }
    /* */
    
    // Column vector access handler
    if ((pkt->req->hasPC())
        && (this->name().find("dcache") != std::string::npos) && !blocked && !mustSendRetry
        && (cache->MJL_colVecHandler.MJL_ColVecList.find(pkt->req->getPC()) != cache->MJL_colVecHandler.MJL_ColVecList.end() && !cache->MJL_colVecHandler.isSend(pkt, true))) {
        return true;
    }

    // MJL_Test: see if the input is correct on vector operations
    if ((this->name().find("dcache") != std::string::npos) && pkt->req->hasPC() && !blocked && !mustSendRetry && cache->MJL_VecListSet.find(pkt->req->getPC()) != cache->MJL_VecListSet.end()) {
        if (pkt->getSize() <= sizeof(uint64_t)) {
            std::cout << "NonVec: " << std::hex << pkt->req->getPC() << std::dec << "[" << pkt->getSize() << "], Addr(oct) " << std::oct << pkt->getAddr() << std::dec << std::endl;
        }
        assert(pkt->getSize() > sizeof(uint64_t));
        //std::cout << "Vec: " << std::hex << pkt->req->getPC() << std::dec << "[" << pkt->getSize() << "], Addr(oct) " << std::oct << pkt->getAddr() << std::dec << std::endl;
    }


    // Split packet if the access is not word aligned (despite changes in "splitRequest()")
    bool MJL_split = false;
    PacketPtr MJL_sndPkt = pkt;

    // Extract the byte offset of the access
    Addr MJL_baseAddr = pkt->getAddr();
    unsigned MJL_byteOffset = MJL_baseAddr & (Addr)(sizeof(uint64_t) - 1);

    if ((this->name().find("dcache") != std::string::npos) // Only split for L1D$
        && !blocked // When the cache is not blocked
        && !mustSendRetry // Or committed to a retry
        && pkt->needsResponse() // Read and Writes needs response, otherwise don't care (should not recv writeback or evict requests at this point either)
        && !cache->MJL_sndPacketWaiting // Do not assign new sequence number to the second half of a split packet
        && !pkt->req->MJL_isVec() // Only split on non-vector accesses
        && (MJL_byteOffset + pkt->getSize() > sizeof(uint64_t))) { // that are not word aligned

        MJL_split = true;

        // Extract basic information about the packet
        CacheBlk::MJL_CacheBlkDir pktOrigDir = pkt->MJL_getCmdDir();
        assert(pkt->req->hasPC());

        // Set a unique sequence number to each packet that has the same PC and request time to identify the packet
        int MJL_testSeq = 0;
        while (cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()].find(MJL_testSeq) != cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()].end()) {
            // Find the next sequence number
            MJL_testSeq++;
        }
        // Assign the sequence number
        pkt->MJL_testSeq = MJL_testSeq;
        
        // Accesses designed to be column direction should never be unaligned
        assert(pktOrigDir == MemCmd::MJL_DirAttribute::MJL_IsRow);

        std::cout << "MJL_Split: splitting one packet into 2, ";
        RequestPtr MJL_sndReq = new Request(*pkt->req);
        std::cout << "New Request created, ";
        MJL_sndReq->MJL_setSize(MJL_byteOffset + pkt->getSize() - sizeof(uint64_t));
        std::cout << "New Request size set, ";
        MJL_sndReq->setPaddr(pkt->getAddr() + sizeof(uint64_t) - MJL_byteOffset);
        std::cout << "New Request address set, ";
        MJL_sndPkt = new Packet(MJL_sndReq, pkt->cmd);
        std::cout << "New Packet created." << std::endl;
        MJL_sndPkt->MJL_testSeq = MJL_testSeq;
        MJL_sndPkt->allocate();

        // Register the split packet pair 
        cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()][MJL_testSeq] = std::tuple<PacketPtr, PacketPtr> (pkt, MJL_sndPkt);
        cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][MJL_testSeq][0] = false;
        cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][MJL_testSeq][1] = false;
        if (pkt->hasData()) {
            memcpy(MJL_sndPkt->getPtr<uint8_t>(), pkt->getConstPtr<uint8_t>() + sizeof(uint64_t) - MJL_byteOffset, MJL_sndPkt->getSize());
        }
        if (pkt->isWrite()) {
            pkt->req->MJL_setSize(sizeof(uint64_t) - MJL_byteOffset);
            pkt->MJL_setSize(sizeof(uint64_t) - MJL_byteOffset);
        }
    }

    // Assign column preference for default column access
    if ((this->name().find("dcache") != std::string::npos) && cache->MJL_defaultColumn && !blocked && !mustSendRetry) {
        pkt->cmd.MJL_setCmdDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);
        pkt->req->MJL_setReqDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);
        pkt->MJL_setDataDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);
        if (MJL_split) {
            MJL_sndPkt->cmd.MJL_setCmdDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);
            MJL_sndPkt->req->MJL_setReqDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);
            MJL_sndPkt->MJL_setDataDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);
        }
    }

    // Set common system information to propagate the information everywhere
    if (MJL_split) {
        MJL_sndPkt->req->MJL_cachelineSize = cache->blkSize;
        MJL_sndPkt->req->MJL_rowWidth = cache->MJL_rowWidth;
    }
    /* MJL_End */
    assert(!cache->system->bypassCaches());

    bool success = false;

    // always let express snoop packets through if even if blocked
    if (pkt->isExpressSnoop()) {
        // do not change the current retry state
        bool M5_VAR_USED bypass_success = cache->recvTimingReq(pkt);
        assert(bypass_success);
        return true;
    } else if (blocked || mustSendRetry) {
        // either already committed to send a retry, or blocked
        success = false;
    /* MJL_Begin */
    // Sending mechanism on the second half of the split packets
    } else if (this->name().find("dcache") && cache->MJL_sndPacketWaiting) {
        // Wait till the second half of the split packets is sent to accept new packets
        if (pkt->req->contextId() == cache->MJL_retrySndPacket->req->contextId()) {
            assert(MJL_split);
            MJL_split = false;
            success = cache->recvTimingReq(cache->MJL_retrySndPacket);
            if (success) {
                cache->MJL_sndPacketWaiting = false;
            }
            // Should pass (it seems that the function only returns true...)
            assert(success == true);
        } else {
            success = false;
        }
    /* MJL_End */
    } else {
        // pass it on to the cache, and let the cache decide if we
        // have to retry or not
        success = cache->recvTimingReq(pkt);
    }

    /* MJL_Begin */
    if (MJL_split) {
        // Only wait when the first packet is not sent successfully
        cache->MJL_sndPacketWaiting = !success;
        // Try sending the second packet if the first was sent successfully
        if (success) {
            success = cache->recvTimingReq(MJL_sndPkt);
            // Should pass as well (it seems that the function only returns true...)
            assert(success == true);
        // The second packet needs to wait for the first packet's retry'
        } else {
            cache->MJL_retrySndPacket = MJL_sndPkt;
        }
    } 
    /* MJL_End */
    // remember if we have to retry
    mustSendRetry = !success;
    return success;
}

Tick
Cache::CpuSidePort::recvAtomic(PacketPtr pkt)
{
    /* MJL_Begin */
    // Assign direction preference to packet based on PC at L1D$
    if ((pkt->req->hasPC())
        && (this->name().find("dcache") != std::string::npos)
        && (cache->MJL_PC2DirMap.find(pkt->req->getPC()) != cache->MJL_PC2DirMap.end())) {
        CacheBlk::MJL_CacheBlkDir InputDir = cache->MJL_PC2DirMap.find(pkt->req->getPC())->second;
        pkt->cmd.MJL_setCmdDir(InputDir);
        pkt->req->MJL_setReqDir(InputDir);
        pkt->MJL_setDataDir(InputDir);
    }

    // Assign dirty bits for write requests at L1D$
    if ((this->name().find("dcache") != std::string::npos) && pkt->isWrite()) {
        pkt->MJL_setAllDirty();
    }

    /* MJL_Test: Request packet information output  
    if (this->name().find("dcache") != std::string::npos) {
        std::cout << this->name() << "::recvAtomicPreAcc";
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
        std::cout << ", Data(hex) = " << std::hex;
        if (pkt->hasData()) {
            uint64_t MJL_data = 0;
            std::memcpy(&MJL_data, pkt->getConstPtr<uint8_t>(), pkt->getSize());
            std::cout << "word[0] " << std::hex << MJL_data << std::dec;
            for (unsigned i = sizeof(uint64_t); i < pkt->getSize(); i = i + sizeof(uint64_t)) {
                MJL_data = 0;
                std::memcpy(&MJL_data, pkt->getConstPtr<uint8_t>() + i, std::min(sizeof(uint64_t), pkt->getSize() - (Addr)i));
                std::cout << " | word[" << i/sizeof(uint64_t) << "] " <<  MJL_data;
            }         
        } else {
            std::cout << "noData";
        }
        std::cout << std::dec;
        std::cout << ", Time = " << pkt->req->time();
        std::cout << std::endl;
    }
     */

    // Column vector access handler
    if ((pkt->req->hasPC())
        && (this->name().find("dcache") != std::string::npos)
        && (cache->MJL_colVecHandler.MJL_ColVecList.find(pkt->req->getPC()) != cache->MJL_colVecHandler.MJL_ColVecList.end() && !cache->MJL_colVecHandler.isSend(pkt, false))) {
        if (pkt->isRead()) return 0;
        // Cannot do vector write work around since there one packet cannot wait on another
    }

    // Split packet if the access is not word aligned (despite changes in "splitRequest()"), see recvTimingReq for detail
    bool MJL_split = false;
    PacketPtr MJL_sndPkt = pkt;

    Addr MJL_baseAddr = pkt->getAddr();
    unsigned MJL_byteOffset = MJL_baseAddr & (Addr)(sizeof(uint64_t) - 1);

    if ((this->name().find("dcache") != std::string::npos)
        && pkt->needsResponse()
        && !cache->MJL_sndPacketWaiting
        && !pkt->req->MJL_isVec()
        && (MJL_byteOffset + pkt->getSize() > sizeof(uint64_t))) {

        MJL_split = true;

        CacheBlk::MJL_CacheBlkDir pktOrigDir = pkt->MJL_getCmdDir();
        assert(pkt->req->hasPC());

        int MJL_testSeq = 0;
            
        while (cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()].find(MJL_testSeq) != cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()].end()) {
            MJL_testSeq++;
        }

        pkt->MJL_testSeq = MJL_testSeq;

        assert(pktOrigDir == MemCmd::MJL_DirAttribute::MJL_IsRow);

        std::cout << "MJL_Split: splitting one packet into 2, ";
        RequestPtr MJL_sndReq = new Request(*pkt->req);
        std::cout << "New Request created, ";
        MJL_sndReq->MJL_setSize(MJL_byteOffset + pkt->getSize() - sizeof(uint64_t));
        std::cout << "New Request size set, ";
        MJL_sndReq->setPaddr(pkt->getAddr() + sizeof(uint64_t) - MJL_byteOffset);
        std::cout << "New Request address set, ";
        MJL_sndPkt = new Packet(MJL_sndReq, pkt->cmd);
        std::cout << "New Packet created." << std::endl;
        MJL_sndPkt->MJL_testSeq = MJL_testSeq;
        MJL_sndPkt->allocate();
                
        cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()][MJL_testSeq] = std::tuple<PacketPtr, PacketPtr> (pkt, MJL_sndPkt);
        cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][MJL_testSeq][0] = false;
        cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][MJL_testSeq][1] = false;
        if (pkt->hasData()) {
            memcpy(MJL_sndPkt->getPtr<uint8_t>(), pkt->getConstPtr<uint8_t>() + sizeof(uint64_t) - MJL_byteOffset, MJL_sndPkt->getSize());
        }
        if (pkt->isWrite()) {
            pkt->req->MJL_setSize(sizeof(uint64_t) - MJL_byteOffset);
            pkt->MJL_setSize(sizeof(uint64_t) - MJL_byteOffset);
        }
    }

    // Assign column preference for default column access
    if (this->name().find("dcache") != std::string::npos && cache->MJL_defaultColumn) {
        pkt->cmd.MJL_setCmdDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);
        pkt->req->MJL_setReqDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);
        pkt->MJL_setDataDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);
        if (MJL_split) {
            MJL_sndPkt->cmd.MJL_setCmdDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);
            MJL_sndPkt->req->MJL_setReqDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);
            MJL_sndPkt->MJL_setDataDir(MemCmd::MJL_DirAttribute::MJL_IsColumn);
        }
    }

    // Set common system information to propagate the information everywhere
    pkt->req->MJL_cachelineSize = cache->blkSize;
    pkt->req->MJL_rowWidth = cache->MJL_rowWidth;
    if (MJL_split) {
        MJL_sndPkt->req->MJL_cachelineSize = cache->blkSize;
        MJL_sndPkt->req->MJL_rowWidth = cache->MJL_rowWidth;
    }
    
    // The actual access
    Tick time = cache->recvAtomic(pkt);
    
    if (this->name().find("dcache") != std::string::npos && pkt->isResponse()) {
        /* MJL_Test: Respnse packet information output 
        std::cout << this->name() << "::recvAtomicPostAcc";
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
                std::cout << " | word[" << i/sizeof(uint64_t) << "] " <<  MJL_data;
            }       
        } else {
            std::cout << ", noData";
        }
        std::cout << std::dec;
        std::cout << ", Time = " << pkt->req->time() ;
        std::cout << std::endl;
         */
        
        // Handle split packet's response, see sendTimingResp() in cache.hh for detail
        bool MJL_isUnaligned = MJL_split;
        if (MJL_isUnaligned) {
            std::cout << "MJL_Merge: Received a packet that was split\n";

            if (pkt == std::get<0>(cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq])) {
                cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq][0] = true;
            } else if (pkt == std::get<1>(cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq])) {
                cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq][1] = true;
            } else {
                assert((pkt == std::get<0>(cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq])) || (pkt == std::get<1>(cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq])));
            }
        }
    }

    // Access for the second half of the split packet
    if (MJL_split) {
        pkt = MJL_sndPkt;
        time = time + cache->recvAtomic(pkt);
        if (this->name().find("dcache") != std::string::npos && pkt->isResponse() && MJL_sndPkt->isResponse()) {
            /* MJL_Test: Respnse packet information output 
            std::cout << this->name() << "::recvAtomicPostAcc";
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
                    std::cout << " | word[" << i/sizeof(uint64_t) << "] " <<  MJL_data;
                }       
            } else {
                std::cout << ", noData";
            }
            std::cout << std::dec;
            std::cout << ", Time = " << pkt->req->time() ;
            std::cout << std::endl;
            */

            std::cout << "MJL_Merge: Received a packet that was split\n";
            if (pkt == std::get<0>(cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq])) {
                cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq][0] = true;
            } else if (pkt == std::get<1>(cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq])) {
                cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq][1] = true;
            } else {
                assert((pkt == std::get<0>(cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq])) || (pkt == std::get<1>(cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq])));
            }

            if (cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq][0] && cache->MJL_unalignedPacketCount[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq][1]) {
                std::cout << "MJL_Merge: Both packet from split received, ";
                PacketPtr MJL_origPacket = std::get<0>(cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq]);
                PacketPtr MJL_sndPacket = std::get<1>(cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()][pkt->MJL_testSeq]);

                if (pkt->isRead()) {
                    unsigned MJL_byteOffset = MJL_origPacket->getAddr() & (Addr)(sizeof(uint64_t) - 1);
                    std::memcpy(MJL_origPacket->getPtr<uint8_t>() + sizeof(uint64_t) - MJL_byteOffset, MJL_sndPacket->getConstPtr<uint8_t>(), MJL_sndPacket->getSize());
                    std::cout << "Merged read results, data = ";
                    uint64_t MJL_Data = 0;
                    std::memcpy(&MJL_Data, MJL_origPacket->getConstPtr<uint8_t>(), MJL_origPacket->getSize());
                    std::cout << std::hex << MJL_Data << std::dec << ", ";
                } else if (pkt->isWrite()) {
                    MJL_origPacket->MJL_setSize(MJL_origPacket->getSize() + MJL_sndPacket->getSize());
                    MJL_origPacket->req->MJL_setSize(MJL_origPacket->getSize());
                    std::cout << "Recovering write size for original packet, ";
                }

                if (pkt != MJL_origPacket) {
                    MJL_origPacket->headerDelay = pkt->headerDelay;
                    MJL_origPacket->snoopDelay = pkt->snoopDelay;
                    MJL_origPacket->payloadDelay = pkt->payloadDelay;
                    pkt = MJL_origPacket;
                    std::cout << "Setting original packet variables, ";
                }
                
                delete MJL_sndPacket;
                std::cout << "Deleted created packet\n";
                cache->MJL_unalignedPacketList[pkt->req->getPC()][pkt->req->time()].erase(pkt->MJL_testSeq);
            }
        }
    }
    
    // Column vector access handler
    if ((pkt->req->hasPC())
        && (this->name().find("dcache") != std::string::npos)) {
        cache->MJL_colVecHandler.handleResponse(pkt, false);
    }

    return time;
    /* MJL_End */
    /* MJL_Comment 
    return cache->recvAtomic(pkt);
    */
}

void
Cache::CpuSidePort::recvFunctional(PacketPtr pkt)
{
    /* MJL_Begin */
    /* MJL_Test request packet information output 
    if (this->name().find("dcache") != std::string::npos) {
        std::cout << this->name() << "::recvFunctionalPreAcc";
        std::cout << ": MemCmd = " << pkt->cmd.toString();
        std::cout << ", Addr(oct) = " << std::oct << pkt->getAddr() << std::dec;
        std::cout << ", Size = " << pkt->getSize();
        std::cout << ", Data = " << std::hex;
        if (pkt->hasData()) {
            uint64_t MJL_data = 0;
            std::memcpy(&MJL_data, pkt->getConstPtr<uint8_t>(), std::min(sizeof(uint64_t), pkt->getSize()));
            std::cout << "word[0] "<< MJL_data;
            for (int i = 1; i < pkt->getSize()/sizeof(uint64_t); ++i) {
                MJL_data = 0;
                std::memcpy(&MJL_data, pkt->getConstPtr<uint8_t>() + i*sizeof(uint64_t), std::min(sizeof(uint64_t), pkt->getSize() - i*sizeof(uint64_t)));
                std::cout << "| word[" << i << "] "<< MJL_data;
            }
        } else {
            std::cout << "noData";
        }
        std::cout << std::dec << std::endl;
    }
     */

    // Functional accesses are all in row direction
    assert(pkt->MJL_getCmdDir() == MemCmd::MJL_DirAttribute::MJL_IsRow);

    // Set common system information to propagate the information everywhere
    pkt->req->MJL_cachelineSize = cache->blkSize;
    pkt->req->MJL_rowWidth = cache->MJL_rowWidth;
    /* MJL_End */
    // functional request
    cache->functionalAccess(pkt, true);
    /* MJL_Begin */
    /* MJL_Test response packet information output 
    if ((this->name().find("dcache") != std::string::npos) && pkt->isResponse()) {
        std::cout << this->name() << "::recvFunctionalPostAcc";
        std::cout << ": MemCmd = " << pkt->cmd.toString();
        std::cout << ", Addr(oct) = " << std::oct << pkt->getAddr() << std::dec;
        std::cout << ", Size = " << pkt->getSize();
        std::cout << ", Data = " << std::hex;
        if (pkt->hasData()) {
            uint64_t MJL_data = 0;
            std::memcpy(&MJL_data, pkt->getConstPtr<uint8_t>(), std::min(sizeof(uint64_t), pkt->getSize()));
            std::cout << "word[0] = "<< MJL_data;
            for (int i = 1; i < pkt->getSize()/sizeof(uint64_t); ++i) {
                MJL_data = 0;
                std::memcpy(&MJL_data, pkt->getConstPtr<uint8_t>() + i*sizeof(uint64_t), std::min(sizeof(uint64_t), pkt->getSize() - i*sizeof(uint64_t)));
                std::cout << " | word[" << i << "] = "<< MJL_data;
            }
        } else {
            std::cout << "noData";
        }
        std::cout << std::dec << std::endl;
    }
     */
    /* MJL_End */
}

Cache::
CpuSidePort::CpuSidePort(const std::string &_name, Cache *_cache,
                         const std::string &_label)
    : BaseCache::CacheSlavePort(_name, _cache, _label), cache(_cache)
{
}

Cache*
CacheParams::create()
{
    assert(tags);

    return new Cache(this);
}
///////////////
//
// MemSidePort
//
///////////////

bool
Cache::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    cache->recvTimingResp(pkt);
    return true;
}

// Express snooping requests to memside port
void
Cache::MemSidePort::recvTimingSnoopReq(PacketPtr pkt)
{
    // handle snooping requests
    cache->recvTimingSnoopReq(pkt);
}

Tick
Cache::MemSidePort::recvAtomicSnoop(PacketPtr pkt)
{
    return cache->recvAtomicSnoop(pkt);
}

void
Cache::MemSidePort::recvFunctionalSnoop(PacketPtr pkt)
{
    // functional snoop (note that in contrast to atomic we don't have
    // a specific functionalSnoop method, as they have the same
    // behaviour regardless)
    /* MJL_Begin */
    // MJL_TODO: to check whether there are column accesses for functional
    assert(pkt->MJL_getCmdDir() == MemCmd::MJL_DirAttribute::MJL_IsRow);
    /* MJL_End */
    
    cache->functionalAccess(pkt, false);
}

void
Cache::CacheReqPacketQueue::sendDeferredPacket()
{
    // sanity check
    assert(!waitingOnRetry);

    // there should never be any deferred request packets in the
    // queue, instead we resly on the cache to provide the packets
    // from the MSHR queue or write queue
    assert(deferredPacketReadyTime() == MaxTick);

    // check for request packets (requests & writebacks)
    QueueEntry* entry = cache.getNextQueueEntry();

    if (!entry) {
        // can happen if e.g. we attempt a writeback and fail, but
        // before the retry, the writeback is eliminated because
        // we snoop another cache's ReadEx.
    } else {
        // let our snoop responses go first if there are responses to
        // the same addresses
        if (checkConflictingSnoop(entry->blkAddr)) {
            return;
        }
        waitingOnRetry = entry->sendPacket(cache);
    }

    // if we succeeded and are not waiting for a retry, schedule the
    // next send considering when the next queue is ready, note that
    // snoop responses have their own packet queue and thus schedule
    // their own events
    if (!waitingOnRetry) {
        schedSendEvent(cache.nextQueueReadyTime());
    }
}

Cache::
MemSidePort::MemSidePort(const std::string &_name, Cache *_cache,
                         const std::string &_label)
    : BaseCache::CacheMasterPort(_name, _cache, _reqQueue, _snoopRespQueue),
      _reqQueue(*_cache, *this, _snoopRespQueue, _label),
      _snoopRespQueue(*_cache, *this, _label), cache(_cache)
{
}
