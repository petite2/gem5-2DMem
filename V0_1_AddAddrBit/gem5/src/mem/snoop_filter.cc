/*
 * Copyright (c) 2013-2016 ARM Limited
 * All rights reserved
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
 * Authors: Stephan Diestelhorst
 */

/**
 * @file
 * Implementation of a snoop filter.
 */

#include "base/misc.hh"
#include "base/trace.hh"
#include "debug/SnoopFilter.hh"
#include "mem/snoop_filter.hh"
#include "sim/system.hh"

void
SnoopFilter::eraseIfNullEntry(SnoopFilterCache::iterator& sf_it)
{
    SnoopItem& sf_item = sf_it->second;
    if (!(sf_item.requested | sf_item.holder)) {
        cachedLocations.erase(sf_it);
        DPRINTF(SnoopFilter, "%s:   Removed SF entry.\n",
                __func__);
    }
}
/* MJL_Begin */
void
SnoopFilter::MJL_eraseIfNullEntry(SnoopFilterCache::iterator& sf_it, MemCmd::MJL_DirAttribute MJL_cmdDir)
{
    SnoopItem& sf_item = sf_it->second;
    if (!(sf_item.requested | sf_item.holder)) {
        MJL_cachedLocations[MJL_cmdDir].erase(sf_it);
        DPRINTF(SnoopFilter, "%s:   Removed SF entry.\n",
                __func__);
    }
}
/* MJL_End */

std::pair<SnoopFilter::SnoopList, Cycles>
SnoopFilter::lookupRequest(const Packet* cpkt, const SlavePort& slave_port)
{
    DPRINTF(SnoopFilter, "%s: src %s packet %s\n", __func__,
            slave_port.name(), cpkt->print());
    /* MJL_Test 
    std::cout << this->name() << "::MJL_Debug: At lookupRequest: " << cpkt->print() << ", dir = " << cpkt->MJL_getCmdDir();
     */

    // check if the packet came from a cache
    bool allocate = !cpkt->req->isUncacheable() && slave_port.isSnooping() &&
        cpkt->fromCache();
    Addr line_addr = cpkt->getBlockAddr(linesize);
    if (cpkt->isSecure()) {
        line_addr |= LineSecure;
    }
    SnoopMask req_port = portToMask(slave_port);
    /* MJL_Begin */
    reqLookupResult = MJL_cachedLocations[cpkt->MJL_getCmdDir()].find(line_addr);
    bool is_hit = (reqLookupResult != MJL_cachedLocations[cpkt->MJL_getCmdDir()].end());
    /* MJL_End */
    /* MJL_Comment
    reqLookupResult = cachedLocations.find(line_addr);
    bool is_hit = (reqLookupResult != cachedLocations.end());
    */

    // If the snoop filter has no entry, and we should not allocate,
    // do not create a new snoop filter entry, simply return a NULL
    // portlist.
    if (!is_hit && !allocate)
        return snoopDown(lookupLatency);

    // If no hit in snoop filter create a new element and update iterator
    if (!is_hit)
        /* MJL_Begin */
        reqLookupResult = MJL_cachedLocations[cpkt->MJL_getCmdDir()].emplace(line_addr, SnoopItem()).first;
        /* MJL_End */
        /* MJL_Comment
        reqLookupResult = cachedLocations.emplace(line_addr, SnoopItem()).first;
        */
    SnoopItem& sf_item = reqLookupResult->second;
    SnoopMask interested = sf_item.holder | sf_item.requested;

    // Store unmodified value of snoop filter item in temp storage in
    // case we need to revert because of a send retry in
    // updateRequest.
    retryItem = sf_item;
    /* MJL_Begin */
    if (slave_port.getMasterPort().MJL_is2DCache()) {
        for (int i = 0; i < linesize/sizeof(uint64_t); ++i) {
            auto MJL_temp_it = MJL_cachedLocations[cpkt->MJL_getCrossCmdDir()].find(cpkt->MJL_getCrossBlockAddrs(linesize, i));
            if (MJL_temp_it != MJL_cachedLocations[cpkt->MJL_getCrossCmdDir()].end()) {
                    SnoopItem& MJL_temp_item = MJL_temp_it->second;
                    MJL_retryItems[i] = MJL_temp_item;
                    MJL_retrySet[i] = true;
            }
        }
    } else {
        for (int i = 0; i < linesize/sizeof(uint64_t); ++i) {
            MJL_retrySet[i] = false;
        }
    } 
    /* MJL_End */
    /* MJL_Test 
    std::cout << ", old holder = " << sf_item.holder;
     */

    totRequests++;
    if (is_hit) {
        // Single bit set -> value is a power of two
        if (isPow2(interested))
            hitSingleRequests++;
        else
            hitMultiRequests++;
    }

    DPRINTF(SnoopFilter, "%s:   SF value %x.%x\n",
            __func__, sf_item.requested, sf_item.holder);

    // If we are not allocating, we are done
    if (!allocate)
        return snoopSelected(maskToPortList(interested & ~req_port),
                             lookupLatency);

    if (cpkt->needsResponse()) {
        if (!cpkt->cacheResponding()) {
            // Max one request per address per port
            panic_if(sf_item.requested & req_port, "double request :( " \
                     "SF value %x.%x\n", sf_item.requested, sf_item.holder);

            // Mark in-flight requests to distinguish later on
            sf_item.requested |= req_port;
            DPRINTF(SnoopFilter, "%s:   new SF value %x.%x\n",
                    __func__,  sf_item.requested, sf_item.holder);
        } else {
            // NOTE: The memInhibit might have been asserted by a cache closer
            // to the CPU, already -> the response will not be seen by this
            // filter -> we do not need to keep the in-flight request, but make
            // sure that we know that that cluster has a copy
            panic_if(!(sf_item.holder & req_port), "Need to hold the value!");
            DPRINTF(SnoopFilter,
                    "%s: not marking request. SF value %x.%x\n",
                    __func__,  sf_item.requested, sf_item.holder);
        }
    } else { // if (!cpkt->needsResponse())
        assert(cpkt->isEviction());
        /* MJL_Test  
        //std::cout << "MJL_Debug: " << this->name() << ":: slave_port name: " << slave_port.name() << ", slave_port.getMasterPort name: " << slave_port.getMasterPort().name() << ", is port attached to physically 2D cache ? " << slave_port.getMasterPort().MJL_is2DCache() << std::endl;

        std::cout << this->name() << "::The Evicted Packet Info: PC(hex) = ";
        if (cpkt->req->hasPC()) {
            std::cout << std::hex << cpkt->req->getPC() << std::dec;
        } else {
            std::cout << "noPC";
        }
        std::cout << ", MemCmd = " << cpkt->cmd.toString();
        std::cout << ", CmdDir = " << cpkt->MJL_getCmdDir();
        std::cout << ", Addr(oct) = " << std::oct << cpkt->getAddr() << std::dec;
        std::cout << ", Size = " << cpkt->getSize();
        std::cout << ", MJL_crossBlocksCached = blk[0] " << cpkt->MJL_crossBlocksCached[0];
        for (int i = 1; i < 8; ++i) {
            std::cout << " | blk[" << i << "] " << cpkt->MJL_crossBlocksCached[i];
        }
        std::cout << std::endl;
        // std::cout << ", Data(hex) = ";
        // if (cpkt->hasData()) {
        //     uint64_t MJL_data = 0;
        //     std::memcpy(&MJL_data, cpkt->getConstPtr<uint8_t>(), std::min(sizeof(uint64_t), (Addr)cpkt->getSize()));
        //     std::cout << "word[0] " << std::hex << MJL_data << std::dec;
        //     for (unsigned i = sizeof(uint64_t); i < cpkt->getSize(); i = i + sizeof(uint64_t)) {
        //         MJL_data = 0;
        //         std::memcpy(&MJL_data, cpkt->getConstPtr<uint8_t>() + i, std::min(sizeof(uint64_t), cpkt->getSize() - (Addr)i));
        //         std::cout << " | word[" << i/sizeof(uint64_t) << "] " << std::hex <<  MJL_data << std::dec;
        //     }       
        // } else {
        //     std::cout << ", noData";
        // }
        // std::cout << std::dec;
        //std::cout << ", Time = " << pkt->req->time() ;
        std::cout << std::endl;
         */
        
        // make sure that the sender actually had the line
        panic_if(!(sf_item.holder & req_port), "requester %x is not a " \
                 "holder :( SF value %x.%x\n", req_port,
                 sf_item.requested, sf_item.holder);
        // CleanEvicts and Writebacks -> the sender and all caches above
        // it may not have the line anymore.
        /* MJL_Begin */
        // For physically 2D cache, also need the is cached above information for the other direction as well, so that we can correctly determine whether it is still a holder for the other direction
        if (slave_port.getMasterPort().MJL_is2DCache() && !cpkt->isBlockCached()) {
            // In this case, whenever there is 1 cache line evicted, the whole tile is evicted, hence we only rely on the is block cached information on passed from the upper level cache
            bool MJL_wholeTilePresent = true;
            auto MJL_temp_it = MJL_cachedLocations[cpkt->MJL_getCmdDir()].find(line_addr);
            for (int i = 0; i < linesize/sizeof(uint64_t); ++i) {
                MJL_temp_it = MJL_cachedLocations[cpkt->MJL_getCmdDir()].find(cpkt->MJL_getBlockAddrs(linesize, i));
                if (MJL_temp_it != MJL_cachedLocations[cpkt->MJL_getCmdDir()].end()) {
                    MJL_wholeTilePresent &= (bool)(MJL_temp_it->second.holder & req_port);
                    /* MJL_Test 
                    if (cpkt->getAddr() == 1576960 || cpkt->getAddr() == 1577216 || cpkt->getAddr() == 1577472 || cpkt->getAddr() == 1577728 || cpkt->getAddr() == 1577984 || cpkt->getAddr() == 1578240 || cpkt->getAddr() == 1578496 || cpkt->getAddr() == 1578752) {
                        std::cout << this->name() << "::MJL_Debug: point D snoop_filter " << MJL_wholeTilePresent << " " << i << std::endl;
                    }
                    */
                } else {
                    MJL_wholeTilePresent = false;
                    break;
                }
            }
            if (MJL_wholeTilePresent) {
                for (int i = 0; i < linesize/sizeof(uint64_t); ++i) {
                    auto MJL_temp_it = MJL_cachedLocations[cpkt->MJL_getCrossCmdDir()].find(cpkt->MJL_getCrossBlockAddrs(linesize, i));
                    /* MJL_Test 
                    if (cpkt->getAddr() == 1576960 && MJL_temp_it != MJL_cachedLocations[cpkt->MJL_getCrossCmdDir()].end()) {
                        std::cout << this->name() << "::MJL_Debug: point A snoop_filter " << i << " " << MJL_temp_it->second.holder << std::endl;
                    }
                    */
                    if (!cpkt->MJL_crossBlocksCached[i] && MJL_temp_it != MJL_cachedLocations[cpkt->MJL_getCrossCmdDir()].end()) {
                        SnoopItem& MJL_temp_item = MJL_temp_it->second;
                        /* MJL_Test 
                        std::cout << ", old holder" << i << " = " << MJL_temp_item.holder;
                         */
                        MJL_temp_item.holder &= ~req_port;
                        /* MJL_Test 
                        std::cout << ", new holder" << i << " = " << MJL_temp_item.holder;
                         */
                        /* MJL_Test 
                        if (cpkt->getAddr() == 1576960) {
                            std::cout << this->name() << "::MJL_Debug: point C snoop_filter " << i << " " << MJL_temp_item.holder << std::endl;
                        }
                        */
                    }
                }
            }
        }
        /* MJL_End */
        if (!cpkt->isBlockCached()) {
            sf_item.holder &= ~req_port;
            DPRINTF(SnoopFilter, "%s:   new SF value %x.%x\n",
                    __func__,  sf_item.requested, sf_item.holder);
        }
    }
    /* MJL_Test 
    std::cout << ", new holder = " << sf_item.holder << std::endl;
     */

    return snoopSelected(maskToPortList(interested & ~req_port), lookupLatency);
}

void
SnoopFilter::finishRequest(bool will_retry, Addr addr, bool is_secure)
{
    if (reqLookupResult != cachedLocations.end()) {
        // since we rely on the caller, do a basic check to ensure
        // that finishRequest is being called following lookupRequest
        Addr line_addr = (addr & ~(Addr(linesize - 1)));
        if (is_secure) {
            line_addr |= LineSecure;
        }
        assert(reqLookupResult->first == line_addr);
        if (will_retry) {
            // Undo any changes made in lookupRequest to the snoop filter
            // entry if the request will come again. retryItem holds
            // the previous value of the snoopfilter entry.
            reqLookupResult->second = retryItem;

            DPRINTF(SnoopFilter, "%s:   restored SF value %x.%x\n",
                    __func__,  retryItem.requested, retryItem.holder);
        }

        eraseIfNullEntry(reqLookupResult);
    }
}
/* MJL_Begin */
void
SnoopFilter::MJL_finishRequest(bool will_retry, Addr addr, MemCmd::MJL_DirAttribute MJL_cmdDir, bool is_secure)
{
    if (reqLookupResult != MJL_cachedLocations[MJL_cmdDir].end()) {
        // since we rely on the caller, do a basic check to ensure
        // that finishRequest is being called following lookupRequest
        Addr line_addr;
        if (MJL_cmdDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
            line_addr = (addr & ~(Addr(linesize - 1)));
        } else if (MJL_cmdDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
            line_addr = (addr & ~(Addr(MJL_blkMaskColumn)));
        } else {
            line_addr = (addr & ~(Addr(linesize - 1)));
        }
        
        if (is_secure) {
            line_addr |= LineSecure;
        }
        assert(reqLookupResult->first == line_addr);
        if (will_retry) {
            // Undo any changes made in lookupRequest to the snoop filter
            // entry if the request will come again. retryItem holds
            // the previous value of the snoopfilter entry.
            reqLookupResult->second = retryItem;
            /* MJL_Test 
            //if (addr == 1576960) {
                    std::cout << this->name() << "::MJL_Debug: point retry snoop_filter " << addr << " " << MJL_retrySet << std::endl;
            //}
             */
            MemCmd::MJL_DirAttribute MJL_crossCmdDir = MJL_cmdDir;
            if (MJL_cmdDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                MJL_crossCmdDir = MemCmd::MJL_DirAttribute::MJL_IsColumn;
            } else if (MJL_cmdDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                MJL_crossCmdDir = MemCmd::MJL_DirAttribute::MJL_IsRow;
            }
            Addr MJL_crossBaseLineAddr = (addr & ~(Addr(MJL_blkMaskColumn))) & ~(Addr(linesize - 1));
            for (int i = 0; i < linesize/sizeof(uint64_t); ++i) {
                Addr MJL_crossLineAddr = MJL_crossBaseLineAddr;
                if (MJL_cmdDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                    MJL_crossLineAddr = MJL_crossBaseLineAddr + i * sizeof(uint64_t);
                } else if (MJL_cmdDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                    MJL_crossLineAddr = MJL_crossBaseLineAddr + i * MJL_rowWidth * linesize;
                }
                auto MJL_temp_it = MJL_cachedLocations[MJL_crossCmdDir].find(MJL_crossLineAddr);
                if (MJL_retrySet[i] && MJL_temp_it != MJL_cachedLocations[MJL_crossCmdDir].end()) {
                    MJL_temp_it->second = MJL_retryItems[i];

                    MJL_eraseIfNullEntry(MJL_temp_it, MJL_crossCmdDir);
                }
            }
            

            DPRINTF(SnoopFilter, "%s:   restored SF value %x.%x\n",
                    __func__,  retryItem.requested, retryItem.holder);
        }

        MJL_eraseIfNullEntry(reqLookupResult, MJL_cmdDir);
    }
}
/* MJL_End */

std::pair<SnoopFilter::SnoopList, Cycles>
SnoopFilter::lookupSnoop(const Packet* cpkt/* MJL_Begin */, bool MJL_has2DLLC/* MJL_End */)
{
    DPRINTF(SnoopFilter, "%s: packet %s\n", __func__, cpkt->print());

    assert(cpkt->isRequest());

    Addr line_addr = cpkt->getBlockAddr(linesize);
    if (cpkt->isSecure()) {
        line_addr |= LineSecure;
    }
    /* MJL_Begin */
    auto sf_it = MJL_cachedLocations[cpkt->MJL_getCmdDir()].find(line_addr);
    bool is_hit = (sf_it != MJL_cachedLocations[cpkt->MJL_getCmdDir()].end());
    // For physically 2D cache, a check on the cross direction is also needed 
    if (MJL_has2DLLC && !is_hit) {
        bool MJL_isHit = false;
        for (int i = 0; i < linesize/sizeof(uint64_t); ++i) {
            auto MJL_temp_it = MJL_cachedLocations[cpkt->MJL_getCrossCmdDir()].find(cpkt->MJL_getCrossBlockAddrs(linesize, i));
            MJL_isHit |= (MJL_temp_it != MJL_cachedLocations[cpkt->MJL_getCrossCmdDir()].end());
        }
        is_hit |= MJL_isHit;
    }
    
    size_t MJL_size = 0;
    for (auto MJL_it = MJL_cachedLocations.begin(); MJL_it != MJL_cachedLocations.end(); ++MJL_it) {
        MJL_size = MJL_size + MJL_it->second.size();
    }

    panic_if(!is_hit && (MJL_size >= maxEntryCount),
             "snoop filter exceeded capacity of %d cache blocks\n",
             maxEntryCount);
    /* MJL_End */
    /* MJL_Comment
    auto sf_it = cachedLocations.find(line_addr);
    bool is_hit = (sf_it != cachedLocations.end());

    panic_if(!is_hit && (cachedLocations.size() >= maxEntryCount),
             "snoop filter exceeded capacity of %d cache blocks\n",
             maxEntryCount);
    */

    // If the snoop filter has no entry, simply return a NULL
    // portlist, there is no point creating an entry only to remove it
    // later
    if (!is_hit)
        return snoopDown(lookupLatency);

    /* MJL_Begin */
    SnoopItem& sf_item = sf_it->second; // MJL_TODO: is this safe?
    SnoopMask interested = 0;
    if (sf_it != MJL_cachedLocations[cpkt->MJL_getCmdDir()].end()) {
        DPRINTF(SnoopFilter, "%s:   old SF value %x.%x\n",
            __func__, sf_item.requested, sf_item.holder);

        interested = (sf_item.holder | sf_item.requested);
    }
    /* MJL_End */
    /* MJL_Comment
    SnoopItem& sf_item = sf_it->second;

    DPRINTF(SnoopFilter, "%s:   old SF value %x.%x\n",
            __func__, sf_item.requested, sf_item.holder);

    SnoopMask interested = (sf_item.holder | sf_item.requested);
    */

    /* MJL_Begin */
    // For physically 2D cache, a check on the cross direction is also needed 
    if (MJL_has2DLLC) {
        /* MJL_Test 
        if (cpkt->getAddr() == 1576960) {
            std::cout << this->name() << "::MJL_Debug: point B snoop_filter " << std::endl;
        }
         */
        SnoopMask MJL_holder = 0;
        SnoopMask MJL_requested = 0;
        for (int i = 0; i < linesize/sizeof(uint64_t); ++i) {
            auto MJL_temp_it = MJL_cachedLocations[cpkt->MJL_getCrossCmdDir()].find(cpkt->MJL_getCrossBlockAddrs(linesize, i));
            if (MJL_temp_it != MJL_cachedLocations[cpkt->MJL_getCrossCmdDir()].end()) {
                SnoopItem& MJL_temp_item = MJL_temp_it->second;
                MJL_holder |= MJL_temp_item.holder;
                MJL_requested |= MJL_temp_item.requested;
            }
        }
        interested |= (MJL_holder | MJL_requested);
    }
    /* MJL_End */

    totSnoops++;
    // Single bit set -> value is a power of two
    if (isPow2(interested))
        hitSingleSnoops++;
    else
        hitMultiSnoops++;

    // ReadEx and Writes require both invalidation and exlusivity, while reads
    // require neither. Writebacks on the other hand require exclusivity but
    // not the invalidation. Previously Writebacks did not generate upward
    // snoops so this was never an aissue. Now that Writebacks generate snoops
    // we need to special case for Writebacks.
    assert(cpkt->isWriteback() || cpkt->req->isUncacheable() ||
           (cpkt->isInvalidate() == cpkt->needsWritable()));
    /* MJL_Begin */
    bool MJL_sfitemHasChanged = false;
    /* MJL_End */
    if (cpkt->isInvalidate()/* MJL_Begin */ && (sf_it != MJL_cachedLocations[cpkt->MJL_getCmdDir()].end())/* MJL_End */ && !sf_item.requested) {
        // Early clear of the holder, if no other request is currently going on
        // @todo: This should possibly be updated even though we do not filter
        // upward snoops
        sf_item.holder = 0;
        /* MJL_Begin */
        MJL_sfitemHasChanged = true;
        /* MJL_End */
    }

    /* MJL_Begin */
    if (MJL_sfitemHasChanged && sf_it != MJL_cachedLocations[cpkt->MJL_getCmdDir()].end()) {

        MJL_eraseIfNullEntry(sf_it, cpkt->MJL_getCmdDir());

        DPRINTF(SnoopFilter, "%s:   new SF value %x.%x interest: %x \n",
            __func__, sf_item.requested, sf_item.holder, interested);
    }
    /* MJL_End */
    /* MJL_Comment
    eraseIfNullEntry(sf_it);
    DPRINTF(SnoopFilter, "%s:   new SF value %x.%x interest: %x \n",
            __func__, sf_item.requested, sf_item.holder, interested);
    
    */

    return snoopSelected(maskToPortList(interested), lookupLatency);
}

void
SnoopFilter::updateSnoopResponse(const Packet* cpkt,
                                 const SlavePort& rsp_port,
                                 const SlavePort& req_port)
{
    DPRINTF(SnoopFilter, "%s: rsp %s req %s packet %s\n",
            __func__, rsp_port.name(), req_port.name(), cpkt->print());

    assert(cpkt->isResponse());
    assert(cpkt->cacheResponding());
    /* MJL_Test 
    std::cout << this->name() << ":: At updateSnoopResponse: " << cpkt->print() << ", dir = " << cpkt->MJL_getCmdDir();
    assert(this->name().find("membus") == std::string::npos);
     */

    // if this snoop response is due to an uncacheable request, or is
    // being turned into a normal response, there is nothing more to
    // do
    if (cpkt->req->isUncacheable() || !req_port.isSnooping()) {
        return;
    }

    Addr line_addr = cpkt->getBlockAddr(linesize);
    if (cpkt->isSecure()) {
        line_addr |= LineSecure;
    }
    SnoopMask rsp_mask = portToMask(rsp_port);
    SnoopMask req_mask = portToMask(req_port);
    /* MJL_Begin */
    SnoopItem& sf_item = MJL_cachedLocations[cpkt->MJL_getCmdDir()][line_addr];
    /* MJL_End */
    /* MJL_Comment 
    SnoopItem& sf_item = cachedLocations[line_addr];
    */

    DPRINTF(SnoopFilter, "%s:   old SF value %x.%x\n",
            __func__,  sf_item.requested, sf_item.holder);

    // The source should have the line
    panic_if(!(sf_item.holder & rsp_mask), "SF value %x.%x does not have "\
             "the line\n", sf_item.requested, sf_item.holder);

    // The destination should have had a request in
    panic_if(!(sf_item.requested & req_mask), "SF value %x.%x missing "\
             "the original request\n",  sf_item.requested, sf_item.holder);

    // If the snoop response has no sharers the line is passed in
    // Modified state, and we know that there are no other copies, or
    // they will all be invalidated imminently
    /* MJL_Test 
    std::cout << ", old holder = " << sf_item.holder;
     */
    if (!cpkt->hasSharers()) {
        DPRINTF(SnoopFilter,
                "%s: dropping %x because non-shared snoop "
                "response SF val: %x.%x\n", __func__,  rsp_mask,
                sf_item.requested, sf_item.holder);
        sf_item.holder = 0;
    }
    assert(!cpkt->isWriteback());
    // @todo Deal with invalidating responses
    sf_item.holder |=  req_mask;
    sf_item.requested &= ~req_mask;
    assert(sf_item.requested | sf_item.holder);
    DPRINTF(SnoopFilter, "%s:   new SF value %x.%x\n",
            __func__, sf_item.requested, sf_item.holder);
    /* MJL_Test 
    std::cout << ", new holder = " << sf_item.holder;
     */
    /* MJL_Begin 
    if (req_port.getMasterPort().MJL_is2DCache()) {
        bool MJL_wholeTilePresent = true;
        auto MJL_temp_it = MJL_cachedLocations[cpkt->MJL_getCmdDir()].find(line_addr);
        for (int i = 0; i < linesize/sizeof(uint64_t); ++i) {
            MJL_temp_it = MJL_cachedLocations[cpkt->MJL_getCmdDir()].find(cpkt->MJL_getBlockAddrs(linesize, i));
            if (MJL_temp_it != MJL_cachedLocations[cpkt->MJL_getCmdDir()].end()) {
                MJL_wholeTilePresent &= (bool)(MJL_temp_it->second.holder & req_mask);
            } else {
                MJL_wholeTilePresent = false;
                break;
            }
        }
        // MJL_Test 
        std::cout << ", wholeTilePresent? " << MJL_wholeTilePresent;
        //
        if (MJL_wholeTilePresent) {
            for (int i = 0; i < linesize/sizeof(uint64_t); ++i) {
                MJL_temp_it = MJL_cachedLocations[cpkt->MJL_getCrossCmdDir()].find(cpkt->MJL_getCrossBlockAddrs(linesize, i));
                if (MJL_temp_it == MJL_cachedLocations[cpkt->MJL_getCrossCmdDir()].end()) {
                     MJL_temp_it = MJL_cachedLocations[cpkt->MJL_getCrossCmdDir()].emplace(cpkt->MJL_getCrossBlockAddrs(linesize, i), SnoopItem()).first;
                }
                SnoopItem& MJL_temp_item = MJL_temp_it->second;
                // MJL_Test 
                std::cout << ", old holder" << i << " = " << MJL_temp_item.holder;
                //
                MJL_temp_item.holder |=  req_mask;
                // MJL_Test 
                std::cout << ", new holder" << i << " = " << MJL_temp_item.holder;
                //

            }
        }
    }
     MJL_End */
    /* MJL_Test 
    std::cout << std::endl;
     */ 
}

void
SnoopFilter::updateSnoopForward(const Packet* cpkt,
        const SlavePort& rsp_port, const MasterPort& req_port)
{
    /* MJL_Test 
    assert(false);
     */
    DPRINTF(SnoopFilter, "%s: rsp %s req %s packet %s\n",
            __func__, rsp_port.name(), req_port.name(), cpkt->print());

    assert(cpkt->isResponse());
    assert(cpkt->cacheResponding());

    Addr line_addr = cpkt->getBlockAddr(linesize);
    if (cpkt->isSecure()) {
        line_addr |= LineSecure;
    }
    /* MJL_Begin */
    auto sf_it = MJL_cachedLocations[cpkt->MJL_getCmdDir()].find(line_addr);
    bool is_hit = sf_it != MJL_cachedLocations[cpkt->MJL_getCmdDir()].end();
    /* MJL_End */
    /* MJL_Comment 
    auto sf_it = cachedLocations.find(line_addr);
    bool is_hit = sf_it != cachedLocations.end();
    */

    // Nothing to do if it is not a hit
    if (!is_hit)
        return;

    SnoopItem& sf_item = sf_it->second;

    DPRINTF(SnoopFilter, "%s:   old SF value %x.%x\n",
            __func__,  sf_item.requested, sf_item.holder);

    // If the snoop response has no sharers the line is passed in
    // Modified state, and we know that there are no other copies, or
    // they will all be invalidated imminently
    if (!cpkt->hasSharers()) {
        sf_item.holder = 0;
    }
    DPRINTF(SnoopFilter, "%s:   new SF value %x.%x\n",
            __func__, sf_item.requested, sf_item.holder);
    /* MJL_Begin */ 
    MJL_eraseIfNullEntry(sf_it, cpkt->MJL_getCmdDir());
    /* MJL_End */
    /* MJL_Comment 
    eraseIfNullEntry(sf_it);
    */

}

void
SnoopFilter::updateResponse(const Packet* cpkt, const SlavePort& slave_port)
{
    DPRINTF(SnoopFilter, "%s: src %s packet %s\n",
            __func__, slave_port.name(), cpkt->print());

    /* MJL_Test 
    std::cout << this->name() << ":: At updateResponse: " << cpkt->print() << ", dir = " << cpkt->MJL_getCmdDir();
     */
    assert(cpkt->isResponse());

    // we only allocate if the packet actually came from a cache, but
    // start by checking if the port is snooping
    if (cpkt->req->isUncacheable() || !slave_port.isSnooping())
        return;

    // next check if we actually allocated an entry
    Addr line_addr = cpkt->getBlockAddr(linesize);
    if (cpkt->isSecure()) {
        line_addr |= LineSecure;
    }
    /* MJL_Begin */
    auto sf_it = MJL_cachedLocations[cpkt->MJL_getCmdDir()].find(line_addr);
    if (sf_it == MJL_cachedLocations[cpkt->MJL_getCmdDir()].end())
    /* MJL_End */
    /* MJL_Comment
    auto sf_it = cachedLocations.find(line_addr);
    if (sf_it == cachedLocations.end())
    */
        return;

    SnoopMask slave_mask = portToMask(slave_port);
    SnoopItem& sf_item = sf_it->second;

    DPRINTF(SnoopFilter, "%s:   old SF value %x.%x\n",
            __func__,  sf_item.requested, sf_item.holder);

    // Make sure we have seen the actual request, too
    panic_if(!(sf_item.requested & slave_mask), "SF value %x.%x missing "\
             "request bit\n", sf_item.requested, sf_item.holder);
    /* MJL_Test 
    std::cout << ", old holder = " << sf_item.holder;
     */

    // Update the residency of the cache line.
    sf_item.holder |=  slave_mask;
    sf_item.requested &= ~slave_mask;
    assert(sf_item.holder | sf_item.requested);
    DPRINTF(SnoopFilter, "%s:   new SF value %x.%x\n",
            __func__, sf_item.requested, sf_item.holder);
    /* MJL_Test 
    std::cout << ", new holder = " << sf_item.holder;
     */

    /* MJL_Begin */
    // For physically 2D cache, if the cache is a holder for all rows, it is also a holder for all columns, and vice versa
    if (slave_port.getMasterPort().MJL_is2DCache()) {
        bool MJL_wholeTilePresent = true;
        auto MJL_temp_it = MJL_cachedLocations[cpkt->MJL_getCmdDir()].find(line_addr);
        for (int i = 0; i < linesize/sizeof(uint64_t); ++i) {
            MJL_temp_it = MJL_cachedLocations[cpkt->MJL_getCmdDir()].find(cpkt->MJL_getBlockAddrs(linesize, i));
            if (MJL_temp_it != MJL_cachedLocations[cpkt->MJL_getCmdDir()].end()) {
                MJL_wholeTilePresent &= (bool)(MJL_temp_it->second.holder & slave_mask);
                /* MJL_Test 
                if (cpkt->getAddr() == 1576960 || cpkt->getAddr() == 1577216 || cpkt->getAddr() == 1577472 || cpkt->getAddr() == 1577728 || cpkt->getAddr() == 1577984 || cpkt->getAddr() == 1578240 || cpkt->getAddr() == 1578496 || cpkt->getAddr() == 1578752) {
                    std::cout << this->name() << "::MJL_Debug: point D snoop_filter " << MJL_wholeTilePresent << " " << i << std::endl;
                }
                 */
            } else {
                MJL_wholeTilePresent = false;
                break;
            }
        }
        /* MJL_Test 
        std::cout << ", wholeTilePresent? " << MJL_wholeTilePresent;
         */
        if (MJL_wholeTilePresent) {
            for (int i = 0; i < linesize/sizeof(uint64_t); ++i) {
                MJL_temp_it = MJL_cachedLocations[cpkt->MJL_getCrossCmdDir()].find(cpkt->MJL_getCrossBlockAddrs(linesize, i));
                if (MJL_temp_it == MJL_cachedLocations[cpkt->MJL_getCrossCmdDir()].end()) {
                     MJL_temp_it = MJL_cachedLocations[cpkt->MJL_getCrossCmdDir()].emplace(cpkt->MJL_getCrossBlockAddrs(linesize, i), SnoopItem()).first;
                }
                SnoopItem& MJL_temp_item = MJL_temp_it->second;
                /* MJL_Test 
                std::cout << ", old holder" << i << " = " << MJL_temp_item.holder;
                 */
                MJL_temp_item.holder |=  slave_mask;
                /* MJL_Test 
                std::cout << ", new holder" << i << " = " << MJL_temp_item.holder;
                 */

                /* MJL_Test 
                if (cpkt->getAddr() == 1576960 || cpkt->getAddr() == 1577216 || cpkt->getAddr() == 1577472 || cpkt->getAddr() == 1577728 || cpkt->getAddr() == 1577984 || cpkt->getAddr() == 1578240 || cpkt->getAddr() == 1578496 || cpkt->getAddr() == 1578752) {
                    std::cout << this->name() << "::MJL_Debug: point F snoop_filter " << i << " " << MJL_temp_item.holder << std::endl;
                }
                 */
            }
        }
    }
    /* MJL_Test 
    std::cout << std::endl;
     */
    /* MJL_End */
}

void
SnoopFilter::regStats()
{
    SimObject::regStats();

    totRequests
        .name(name() + ".tot_requests")
        .desc("Total number of requests made to the snoop filter.");

    hitSingleRequests
        .name(name() + ".hit_single_requests")
        .desc("Number of requests hitting in the snoop filter with a single "\
              "holder of the requested data.");

    hitMultiRequests
        .name(name() + ".hit_multi_requests")
        .desc("Number of requests hitting in the snoop filter with multiple "\
              "(>1) holders of the requested data.");

    totSnoops
        .name(name() + ".tot_snoops")
        .desc("Total number of snoops made to the snoop filter.");

    hitSingleSnoops
        .name(name() + ".hit_single_snoops")
        .desc("Number of snoops hitting in the snoop filter with a single "\
              "holder of the requested data.");

    hitMultiSnoops
        .name(name() + ".hit_multi_snoops")
        .desc("Number of snoops hitting in the snoop filter with multiple "\
              "(>1) holders of the requested data.");
}

SnoopFilter *
SnoopFilterParams::create()
{
    return new SnoopFilter(this);
}
