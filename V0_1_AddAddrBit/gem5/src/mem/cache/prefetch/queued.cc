/*
 * Copyright (c) 2014-2015 ARM Limited
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
 * Authors: Mitch Hayenga
 */

#include "debug/HWPrefetch.hh"
#include "mem/cache/prefetch/queued.hh"
#include "mem/cache/base.hh"

QueuedPrefetcher::QueuedPrefetcher(const QueuedPrefetcherParams *p)
    : BasePrefetcher(p), queueSize(p->queue_size), latency(p->latency),
      queueSquash(p->queue_squash), queueFilter(p->queue_filter),
      cacheSnoop(p->cache_snoop), tagPrefetch(p->tag_prefetch)
{

}

QueuedPrefetcher::~QueuedPrefetcher()
{
    // Delete the queued prefetch packets
    for (DeferredPacket &p : pfq) {
        delete p.pkt->req;
        delete p.pkt;
    }
}

Tick
QueuedPrefetcher::notify(/* MJL_Comment const */ PacketPtr &pkt)
{
    // Verify this access type is observed by prefetcher
    if (observeAccess(pkt)) {
        Addr blk_addr = pkt->getBlockAddr(blkSize);
        bool is_secure = pkt->isSecure();
        /* MJL_Begin */
        MemCmd::MJL_DirAttribute MJL_cmdDir = pkt->MJL_getCmdDir();
        /* MJL_End */

        // Squash queued prefetches if demand miss to same line
        if (queueSquash) {
            auto itr = pfq.begin();
            while (itr != pfq.end()) {
                if (itr->pkt->getAddr() == blk_addr &&
                    /* MJL_Begin */
                    itr->pkt->MJL_getCmdDir() == MJL_cmdDir &&
                    /* MJL_End */
                    itr->pkt->isSecure() == is_secure) {
                    delete itr->pkt->req;
                    delete itr->pkt;
                    itr = pfq.erase(itr);
                } else {
                    ++itr;
                }
            }
        }

        // Calculate prefetches given this access
        std::vector<AddrPriority> addresses;
        /* MJL_Comment 
        calculatePrefetch(pkt, addresses);
         */
        /* MJL_Begin */
        MemCmd::MJL_DirAttribute MJL_predCmdDir = MemCmd::MJL_DirAttribute::MJL_IsInvalid;
        /* MJL_Test 
        std::cerr << "MJL_Prefetcher::notify() trigger: " << pkt->print() << std::endl;
         */
        if (MJL_pfBasedPredictDir) {
            // Get prefetch based predicted direction
            MJL_calculatePrefetch(pkt, addresses, MJL_predCmdDir);
            // Set prefetch based predicted direction to the packet
            pkt->MJL_setPfPredDir(MJL_predCmdDir);
        } else {
            calculatePrefetch(pkt, addresses);
        }
        /* MJL_End */

        // Queue up generated prefetches
        for (AddrPriority& pf_info : addresses) {

            // Block align prefetch address
            /* MJL_Begin */
            if (MJL_colPf && pkt->MJL_cmdIsColumn()) {
                pf_info.first &= ~(Addr)(pkt->MJL_blkMaskColumn(blkSize, pkt->req->MJL_rowWidth));
            } else {
                pf_info.first &= ~(Addr)(blkSize - 1);
            }
            /* MJL_End */
            /* MJL_Test 
            std::cerr << "MJL_Prefetcher::notify() prefetch addr: " << std::hex << pf_info.first << std::dec << std::endl;
             */
            /* MJL_Comment
            pf_info.first &= ~(Addr)(blkSize - 1);
            */

            pfIdentified++;
            DPRINTF(HWPrefetch, "Found a pf candidate addr: %#x, "
                    "inserting into prefetch queue.\n", pf_info.first);

            // Create and insert the request
            /* MJL_Begin */
            PacketPtr pf_pkt;
            if (MJL_colPf) {
                // if (MJL_predictDir) {
                //     pf_pkt = MJL_insert(pf_info, MJL_predCmdDir, is_secure);
                // } else {
                    pf_pkt = MJL_insert(pf_info, MJL_cmdDir, is_secure);
                // }
            } else if (this->name().find("dcache") != std::string::npos || this->name().find("l2") != std::string::npos || this->name().find("l3") != std::string::npos) {
                pf_pkt = MJL_insert(pf_info, MemCmd::MJL_DirAttribute::MJL_IsRow, is_secure);
            } else {
                pf_pkt = insert(pf_info, is_secure);
            }
            /* MJL_End */
            /* MJL_Comment
            PacketPtr pf_pkt = insert(pf_info, is_secure);
            */

            if (pf_pkt != nullptr) {
                if (tagPrefetch && pkt->req->hasPC()) {
                    // Tag prefetch packet with  accessing pc
                    pf_pkt->req->setPC(pkt->req->getPC());
                }
                /* MJL_Test 
                std::cerr << "MJL_Prefetcher::notify() inserted pf: " << pf_pkt->print() << std::endl;
                 */
            }
        }
    }

    return pfq.empty() ? MaxTick : pfq.front().tick;
}

PacketPtr
QueuedPrefetcher::getPacket()
{
    DPRINTF(HWPrefetch, "Requesting a prefetch to issue.\n");

    if (pfq.empty()) {
        DPRINTF(HWPrefetch, "No hardware prefetches available.\n");
        return nullptr;
    }

    PacketPtr pkt = pfq.begin()->pkt;
    pfq.pop_front();

    pfIssued++;
    assert(pkt != nullptr);
    DPRINTF(HWPrefetch, "Generating prefetch for %#x.\n", pkt->getAddr());
    return pkt;
}

std::list<QueuedPrefetcher::DeferredPacket>::const_iterator
QueuedPrefetcher::inPrefetch(Addr address, bool is_secure) const
{
    for (const_iterator dp = pfq.begin(); dp != pfq.end(); dp++) {
        if ((*dp).pkt->getAddr() == address &&
            (*dp).pkt->isSecure() == is_secure) return dp;
    }

    return pfq.end();
}
/* MJL_Begin */
std::list<QueuedPrefetcher::DeferredPacket>::const_iterator
QueuedPrefetcher::MJL_inPrefetch(Addr address, MemCmd::MJL_DirAttribute MJL_cmdDir, bool is_secure) const
{
    for (const_iterator dp = pfq.begin(); dp != pfq.end(); dp++) {
        if ((*dp).pkt->getAddr() == address &&
            (*dp).pkt->MJL_getCmdDir() == MJL_cmdDir &&
            (*dp).pkt->isSecure() == is_secure) return dp;
    }

    return pfq.end();
}
/* MJL_End */

QueuedPrefetcher::iterator
QueuedPrefetcher::inPrefetch(Addr address, bool is_secure)
{
    for (iterator dp = pfq.begin(); dp != pfq.end(); dp++) {
        if (dp->pkt->getAddr() == address &&
            dp->pkt->isSecure() == is_secure) return dp;
    }

    return pfq.end();
}

/* MJL_Begin */
QueuedPrefetcher::iterator
QueuedPrefetcher::MJL_inPrefetch(Addr address, MemCmd::MJL_DirAttribute MJL_cmdDir, bool is_secure)
{
    for (iterator dp = pfq.begin(); dp != pfq.end(); dp++) {
        if (dp->pkt->getAddr() == address &&
            dp->pkt->MJL_getCmdDir() == MJL_cmdDir &&
            dp->pkt->isSecure() == is_secure) return dp;
    }

    return pfq.end();
}
/* MJL_End */

void
QueuedPrefetcher::regStats()
{
    BasePrefetcher::regStats();

    pfIdentified
        .name(name() + ".pfIdentified")
        .desc("number of prefetch candidates identified");

    pfBufferHit
        .name(name() + ".pfBufferHit")
        .desc("number of redundant prefetches already in prefetch queue");

    pfInCache
        .name(name() + ".pfInCache")
        .desc("number of redundant prefetches already in cache/mshr dropped");

    pfRemovedFull
        .name(name() + ".pfRemovedFull")
        .desc("number of prefetches dropped due to prefetch queue size");

    pfSpanPage
        .name(name() + ".pfSpanPage")
        .desc("number of prefetches not generated due to page crossing");
}

PacketPtr
QueuedPrefetcher::insert(AddrPriority &pf_info, bool is_secure)
{
    if (queueFilter) {
        iterator it = inPrefetch(pf_info.first, is_secure);
        /* If the address is already in the queue, update priority and leave */
        if (it != pfq.end()) {
            pfBufferHit++;
            if (it->priority < pf_info.second) {
                /* Update priority value and position in the queue */
                it->priority = pf_info.second;
                iterator prev = it;
                bool cont = true;
                while (cont && prev != pfq.begin()) {
                    prev--;
                    /* If the packet has higher priority, swap */
                    if (*it > *prev) {
                        std::swap(*it, *prev);
                        it = prev;
                    }
                }
                DPRINTF(HWPrefetch, "Prefetch addr already in "
                    "prefetch queue, priority updated\n");
            } else {
                DPRINTF(HWPrefetch, "Prefetch addr already in "
                    "prefetch queue\n");
            }
            return nullptr;
        }
    }

    if (cacheSnoop && (inCache(pf_info.first, is_secure) ||
                inMissQueue(pf_info.first, is_secure))) {
        pfInCache++;
        DPRINTF(HWPrefetch, "Dropping redundant in "
                "cache/MSHR prefetch addr:%#x\n", pf_info.first);
        return nullptr;
    }

    /* Create a prefetch memory request */
    Request *pf_req =
        new Request(pf_info.first, blkSize, 0, masterId);
    /* MJL_Begin */
    pf_req->MJL_cachelineSize = blkSize;
    pf_req->MJL_rowWidth = cache->MJL_getRowWidth();
    /* MJL_End */

    if (is_secure) {
        pf_req->setFlags(Request::SECURE);
    }
    pf_req->taskId(ContextSwitchTaskId::Prefetcher);
    PacketPtr pf_pkt = new Packet(pf_req, MemCmd::HardPFReq);
    pf_pkt->allocate();

    /* Verify prefetch buffer space for request */
    if (pfq.size() == queueSize) {
        pfRemovedFull++;
        /* Lowest priority packet */
        iterator it = pfq.end();
        panic_if (it == pfq.begin(), "Prefetch queue is both full and empty!");
        --it;
        /* Look for oldest in that level of priority */
        panic_if (it == pfq.begin(), "Prefetch queue is full with 1 element!");
        iterator prev = it;
        bool cont = true;
        /* While not at the head of the queue */
        while (cont && prev != pfq.begin()) {
            prev--;
            /* While at the same level of priority */
            cont = (*prev).priority == (*it).priority;
            if (cont)
                /* update pointer */
                it = prev;
        }
        DPRINTF(HWPrefetch, "Prefetch queue full, removing lowest priority "
                            "oldest packet, addr: %#x", it->pkt->getAddr());
        delete it->pkt->req;
        delete it->pkt;
        pfq.erase(it);
    }

    Tick pf_time = curTick() + clockPeriod() * latency;
    DPRINTF(HWPrefetch, "Prefetch queued. "
            "addr:%#x priority: %3d tick:%lld.\n",
            pf_info.first, pf_info.second, pf_time);

    /* Create the packet and find the spot to insert it */
    DeferredPacket dpp(pf_time, pf_pkt, pf_info.second);
    if (pfq.size() == 0) {
        pfq.emplace_back(dpp);
    } else {
        iterator it = pfq.end();
        while (it != pfq.begin() && dpp > *it)
            --it;
        /* If we reach the head, we have to see if the new element is new head
         * or not */
        if (it == pfq.begin() && dpp <= *it)
            it++;
        pfq.insert(it, dpp);
    }

    return pf_pkt;
}

/* MJL_Begin */
PacketPtr
QueuedPrefetcher::MJL_insert(AddrPriority &pf_info, MemCmd::MJL_DirAttribute MJL_cmdDir, bool is_secure)
{
    if (queueFilter) {
        iterator it = MJL_inPrefetch(pf_info.first, MJL_cmdDir, is_secure);
        /* If the address is already in the queue, update priority and leave */
        if (it != pfq.end()) {
            pfBufferHit++;
            if (it->priority < pf_info.second) {
                /* Update priority value and position in the queue */
                it->priority = pf_info.second;
                iterator prev = it;
                bool cont = true;
                while (cont && prev != pfq.begin()) {
                    prev--;
                    /* If the packet has higher priority, swap */
                    if (*it > *prev) {
                        std::swap(*it, *prev);
                        it = prev;
                    }
                }
                DPRINTF(HWPrefetch, "Prefetch addr already in "
                    "prefetch queue, priority updated\n");
            } else {
                DPRINTF(HWPrefetch, "Prefetch addr already in "
                    "prefetch queue\n");
            }
            return nullptr;
        }
    }

    if (cacheSnoop && (MJL_inCache(pf_info.first, MJL_cmdDir, is_secure) ||
                MJL_inMissQueue(pf_info.first, MJL_cmdDir, is_secure))) {
        pfInCache++;
        DPRINTF(HWPrefetch, "Dropping redundant in "
                "cache/MSHR prefetch addr:%#x\n", pf_info.first);
        return nullptr;
    }

    if ( ( (!MJL_is2DCache() && MJL_crossDirtyInCache(pf_info.first, MJL_cmdDir, is_secure)) ||
        MJL_is2DCache()) ||
        MJL_crossDirtyInMissQueue(pf_info.first, MJL_cmdDir, is_secure) || 
        MJL_crossDirtyInWriteBuffer(pf_info.first, MJL_cmdDir, is_secure) ) {
        return nullptr;
    }

    /* Create a prefetch memory request */
    Request *pf_req =
        new Request(pf_info.first, blkSize, 0, masterId);
    /* MJL_Begin */
    pf_req->MJL_cachelineSize = blkSize;
    pf_req->MJL_rowWidth = cache->MJL_getRowWidth();
    pf_req->MJL_setReqDir(MJL_cmdDir);
    /* MJL_End */

    if (is_secure) {
        pf_req->setFlags(Request::SECURE);
    }
    pf_req->taskId(ContextSwitchTaskId::Prefetcher);
    PacketPtr pf_pkt = new Packet(pf_req, MemCmd::HardPFReq);
    pf_pkt->cmd.MJL_setCmdDir(pf_req->MJL_getReqDir());
    pf_pkt->MJL_setDataDir(pf_req->MJL_getReqDir());
    pf_pkt->allocate();

    /* Verify prefetch buffer space for request */
    if (pfq.size() == queueSize) {
        pfRemovedFull++;
        /* Lowest priority packet */
        iterator it = pfq.end();
        panic_if (it == pfq.begin(), "Prefetch queue is both full and empty!");
        --it;
        /* Look for oldest in that level of priority */
        panic_if (it == pfq.begin(), "Prefetch queue is full with 1 element!");
        iterator prev = it;
        bool cont = true;
        /* While not at the head of the queue */
        while (cont && prev != pfq.begin()) {
            prev--;
            /* While at the same level of priority */
            cont = (*prev).priority == (*it).priority;
            if (cont)
                /* update pointer */
                it = prev;
        }
        DPRINTF(HWPrefetch, "Prefetch queue full, removing lowest priority "
                            "oldest packet, addr: %#x", it->pkt->getAddr());
        delete it->pkt->req;
        delete it->pkt;
        pfq.erase(it);
    }

    Tick pf_time = curTick() + clockPeriod() * latency;
    DPRINTF(HWPrefetch, "Prefetch queued. "
            "addr:%#x priority: %3d tick:%lld.\n",
            pf_info.first, pf_info.second, pf_time);

    /* Create the packet and find the spot to insert it */
    DeferredPacket dpp(pf_time, pf_pkt, pf_info.second);
    if (pfq.size() == 0) {
        pfq.emplace_back(dpp);
    } else {
        iterator it = pfq.end();
        while (it != pfq.begin() && dpp > *it)
            --it;
        /* If we reach the head, we have to see if the new element is new head
         * or not */
        if (it == pfq.begin() && dpp <= *it)
            it++;
        pfq.insert(it, dpp);
    }

    return pf_pkt;
}
/* MJL_End */
