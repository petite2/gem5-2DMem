/*
 * Copyright (c) 2010-2012 ARM Limited
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
 * Copyright (c) 2001-2005 The Regents of The University of Michigan
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
 * Authors: Ron Dreslinski
 *          Ali Saidi
 *          Andreas Hansson
 */

#include <vector>

#include "cpu/base.hh"
#include "cpu/thread_context.hh"
#include "debug/LLSC.hh"
#include "debug/MemoryAccess.hh"
#include "mem/abstract_mem.hh"
#include "mem/packet_access.hh"
#include "sim/system.hh"

using namespace std;

AbstractMemory::AbstractMemory(const Params *p) :
    MemObject(p), range(params()->range), pmemAddr(NULL),
    confTableReported(p->conf_table_reported), inAddrMap(p->in_addr_map),
    kvmMap(p->kvm_map)/* MJL_Begin */, MJL_Debug_Out(false)/* MJL_End */, _system(NULL)
{
}

void
AbstractMemory::init()
{
    assert(system());

    if (size() % _system->getPageBytes() != 0)
        panic("Memory Size not divisible by page size\n");
}

void
AbstractMemory::setBackingStore(uint8_t* pmem_addr)
{
    pmemAddr = pmem_addr;
}

void
AbstractMemory::regStats()
{
    MemObject::regStats();

    using namespace Stats;

    assert(system());

    bytesRead
        .init(system()->maxMasters())
        .name(name() + ".bytes_read")
        .desc("Number of bytes read from this memory")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        bytesRead.subname(i, system()->getMasterName(i));
    }
    bytesInstRead
        .init(system()->maxMasters())
        .name(name() + ".bytes_inst_read")
        .desc("Number of instructions bytes read from this memory")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        bytesInstRead.subname(i, system()->getMasterName(i));
    }
    bytesWritten
        .init(system()->maxMasters())
        .name(name() + ".bytes_written")
        .desc("Number of bytes written to this memory")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        bytesWritten.subname(i, system()->getMasterName(i));
    }
    numReads
        .init(system()->maxMasters())
        .name(name() + ".num_reads")
        .desc("Number of read requests responded to by this memory")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        numReads.subname(i, system()->getMasterName(i));
    }
    numWrites
        .init(system()->maxMasters())
        .name(name() + ".num_writes")
        .desc("Number of write requests responded to by this memory")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        numWrites.subname(i, system()->getMasterName(i));
    }
    numOther
        .init(system()->maxMasters())
        .name(name() + ".num_other")
        .desc("Number of other requests responded to by this memory")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        numOther.subname(i, system()->getMasterName(i));
    }
    bwRead
        .name(name() + ".bw_read")
        .desc("Total read bandwidth from this memory (bytes/s)")
        .precision(0)
        .prereq(bytesRead)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        bwRead.subname(i, system()->getMasterName(i));
    }

    bwInstRead
        .name(name() + ".bw_inst_read")
        .desc("Instruction read bandwidth from this memory (bytes/s)")
        .precision(0)
        .prereq(bytesInstRead)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        bwInstRead.subname(i, system()->getMasterName(i));
    }
    bwWrite
        .name(name() + ".bw_write")
        .desc("Write bandwidth from this memory (bytes/s)")
        .precision(0)
        .prereq(bytesWritten)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        bwWrite.subname(i, system()->getMasterName(i));
    }
    bwTotal
        .name(name() + ".bw_total")
        .desc("Total bandwidth to/from this memory (bytes/s)")
        .precision(0)
        .prereq(bwTotal)
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        bwTotal.subname(i, system()->getMasterName(i));
    }
    bwRead = bytesRead / simSeconds;
    bwInstRead = bytesInstRead / simSeconds;
    bwWrite = bytesWritten / simSeconds;
    bwTotal = (bytesRead + bytesWritten) / simSeconds;

    /* MJL_Begin */
    MJL_bytesWrittenRow
        .init(system()->maxMasters())
        .name(name() + ".MJL_bytes_written_row")
        .desc("Number of bytes written to this memory in row")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        MJL_bytesWrittenRow.subname(i, system()->getMasterName(i));
    }
    
    MJL_bytesWrittenColumn
        .init(system()->maxMasters())
        .name(name() + ".MJL_bytes_written_column")
        .desc("Number of bytes written to this memory in column")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        MJL_bytesWrittenColumn.subname(i, system()->getMasterName(i));
    }
    
    MJL_bytesReadRow
        .init(system()->maxMasters())
        .name(name() + ".MJL_bytes_read_row")
        .desc("Number of bytes read from this memory in row")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        MJL_bytesReadRow.subname(i, system()->getMasterName(i));
    }
    
    MJL_bytesReadColumn
        .init(system()->maxMasters())
        .name(name() + ".MJL_bytes_read_column")
        .desc("Number of bytes read from this memory in column")
        .flags(total | nozero | nonan)
        ;
    for (int i = 0; i < system()->maxMasters(); i++) {
        MJL_bytesReadColumn.subname(i, system()->getMasterName(i));
    }

    MJL_bytesTotalRow
        .name(name() + ".MJL_bytes_total_row")
        .desc("Total number of bytes transferred to/from this memory in row")
        .flags(nozero)
        ;

    MJL_bytesTotalColumn
        .name(name() + ".MJL_bytes_total_column")
        .desc("Total number of bytes transferred to/from this memory in column")
        .flags(nozero)
        ;
    /* MJL_End */
}

AddrRange
AbstractMemory::getAddrRange() const
{
    return range;
}

// Add load-locked to tracking list.  Should only be called if the
// operation is a load and the LLSC flag is set.
void
AbstractMemory::trackLoadLocked(PacketPtr pkt)
{
    Request *req = pkt->req;
    Addr paddr = LockedAddr::mask(req->getPaddr());

    // first we check if we already have a locked addr for this
    // xc.  Since each xc only gets one, we just update the
    // existing record with the new address.
    list<LockedAddr>::iterator i;

    for (i = lockedAddrList.begin(); i != lockedAddrList.end(); ++i) {
        if (i->matchesContext(req)) {
            DPRINTF(LLSC, "Modifying lock record: context %d addr %#x\n",
                    req->contextId(), paddr);
            i->addr = paddr;
            return;
        }
    }

    // no record for this xc: need to allocate a new one
    DPRINTF(LLSC, "Adding lock record: context %d addr %#x\n",
            req->contextId(), paddr);
    lockedAddrList.push_front(LockedAddr(req));
}


// Called on *writes* only... both regular stores and
// store-conditional operations.  Check for conventional stores which
// conflict with locked addresses, and for success/failure of store
// conditionals.
bool
AbstractMemory::checkLockedAddrList(PacketPtr pkt)
{
    Request *req = pkt->req;
    Addr paddr = LockedAddr::mask(req->getPaddr());
    bool isLLSC = pkt->isLLSC();

    // Initialize return value.  Non-conditional stores always
    // succeed.  Assume conditional stores will fail until proven
    // otherwise.
    bool allowStore = !isLLSC;

    // Iterate over list.  Note that there could be multiple matching records,
    // as more than one context could have done a load locked to this location.
    // Only remove records when we succeed in finding a record for (xc, addr);
    // then, remove all records with this address.  Failed store-conditionals do
    // not blow unrelated reservations.
    list<LockedAddr>::iterator i = lockedAddrList.begin();

    if (isLLSC) {
        while (i != lockedAddrList.end()) {
            if (i->addr == paddr && i->matchesContext(req)) {
                // it's a store conditional, and as far as the memory system can
                // tell, the requesting context's lock is still valid.
                DPRINTF(LLSC, "StCond success: context %d addr %#x\n",
                        req->contextId(), paddr);
                allowStore = true;
                break;
            }
            // If we didn't find a match, keep searching!  Someone else may well
            // have a reservation on this line here but we may find ours in just
            // a little while.
            i++;
        }
        req->setExtraData(allowStore ? 1 : 0);
    }
    // LLSCs that succeeded AND non-LLSC stores both fall into here:
    if (allowStore) {
        // We write address paddr.  However, there may be several entries with a
        // reservation on this address (for other contextIds) and they must all
        // be removed.
        i = lockedAddrList.begin();
        while (i != lockedAddrList.end()) {
            if (i->addr == paddr) {
                DPRINTF(LLSC, "Erasing lock record: context %d addr %#x\n",
                        i->contextId, paddr);
                // For ARM, a spinlock would typically include a Wait
                // For Event (WFE) to conserve energy. The ARMv8
                // architecture specifies that an event is
                // automatically generated when clearing the exclusive
                // monitor to wake up the processor in WFE.
                ThreadContext* ctx = system()->getThreadContext(i->contextId);
                ctx->getCpuPtr()->wakeup(ctx->threadId());
                i = lockedAddrList.erase(i);
            } else {
                i++;
            }
        }
    }

    return allowStore;
}


#if TRACING_ON

#define CASE(A, T)                                                        \
  case sizeof(T):                                                         \
    DPRINTF(MemoryAccess,"%s from %s of size %i on address 0x%x data " \
            "0x%x %c\n", A, system()->getMasterName(pkt->req->masterId()),\
            pkt->getSize(), pkt->getAddr(), pkt->get<T>(),                \
            pkt->req->isUncacheable() ? 'U' : 'C');                       \
  break


#define TRACE_PACKET(A)                                                 \
    do {                                                                \
        switch (pkt->getSize()) {                                       \
          CASE(A, uint64_t);                                            \
          CASE(A, uint32_t);                                            \
          CASE(A, uint16_t);                                            \
          CASE(A, uint8_t);                                             \
          default:                                                      \
            DPRINTF(MemoryAccess, "%s from %s of size %i on address 0x%x %c\n",\
                    A, system()->getMasterName(pkt->req->masterId()),          \
                    pkt->getSize(), pkt->getAddr(),                            \
                    pkt->req->isUncacheable() ? 'U' : 'C');                    \
            DDUMP(MemoryAccess, pkt->getConstPtr<uint8_t>(), pkt->getSize());  \
        }                                                                      \
    } while (0)

#else

#define TRACE_PACKET(A)

#endif

void
AbstractMemory::access(PacketPtr pkt)
{
    if (pkt->cacheResponding()) {
        DPRINTF(MemoryAccess, "Cache responding to %#llx: not responding\n",
                pkt->getAddr());
        return;
    }

    if (pkt->cmd == MemCmd::CleanEvict || pkt->cmd == MemCmd::WritebackClean) {
        DPRINTF(MemoryAccess, "CleanEvict  on 0x%x: not responding\n",
                pkt->getAddr());
      return;
    }

    assert(AddrRange(pkt->getAddr(),
                     pkt->getAddr() + (pkt->getSize() - 1)).isSubset(range));

    uint8_t *hostAddr = pmemAddr + pkt->getAddr() - range.start();

    /* MJL_Begin */
    /* MJL_Test: Packet information output  
    if (!pkt->req->isInstFetch()
         //&& pkt->MJL_cmdIsColumn()
         ) { // Debug for column vec
        std::cout << this->name() << "::access()PreAcc";
        std::cout << ": PC(hex) = ";
        if (pkt->req->hasPC()) {
             std::cout << std::hex << pkt->req->getPC() << std::dec;
        } else {
             std::cout << "noPC";
        }
        std::cout << ", MemCmd = " << pkt->cmd.toString();
        std::cout << ", CmdDir = " << pkt->MJL_getCmdDir();
        std::cout << ", Addr(oct) = "<< std::oct << pkt->getAddr() << std::dec;
        std::cout << ", Size = " << pkt->getSize();
        if ((pkt->cmd.toString()).find("WritebackDirty") != std::string::npos) {
            std::cout << ", DataDir = " << pkt->MJL_getDataDir();
            uint64_t MJL_data = 0;
            std::cout << ", Data = " << std::hex;
            std::memcpy(&MJL_data, pkt->getConstPtr<uint8_t>(), std::min(sizeof(uint64_t), (Addr)pkt->getSize()));
            std::cout << "word[0] = " <<  MJL_data;
            for (unsigned i = sizeof(uint64_t); i < pkt->getSize(); i = i + sizeof(uint64_t)) {
                MJL_data = 0;
                std::memcpy(&MJL_data, pkt->getConstPtr<uint8_t>() + i, std::min(sizeof(uint64_t), pkt->getSize() - (Addr)i));
                std::cout << " | word[" << i/sizeof(uint64_t) << "] = " <<  MJL_data;
            }
            std::cout << std::dec;
        }
        std::cout << ", Time: " << pkt->req->time() << std::endl;
    }
     */
    if (MJL_Debug_Out) {
        std::clog << this->name() << "::access " << pkt->print() << std::endl;
    }
    /* MJL_End */
                    

    if (pkt->cmd == MemCmd::SwapReq) {
        if (pkt->isAtomicOp()) {
            if (pmemAddr) {
                // MJL_TODO: Can we just copy?
                /* MJL_Begin */
                if (pkt->MJL_getDataDir() == MemCmd::MJL_DirAttribute::MJL_IsRow){
                    memcpy(pkt->getPtr<uint8_t>(), hostAddr, pkt->getSize());
                } else if (pkt->MJL_getDataDir() == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                    Addr MJL_incColOff = (Addr)(1 << (floorLog2(pkt->req->MJL_rowWidth) + floorLog2(pkt->req->MJL_cachelineSize)));
                    Addr MJL_colOff = 0;
                    for (Addr MJL_Offset = 0; MJL_Offset < pkt->getSize(); MJL_Offset = MJL_Offset + sizeof(uint64_t)) {
                        memcpy(pkt->getPtr<uint8_t>() + MJL_Offset, hostAddr + MJL_colOff, std::min(sizeof(uint64_t), pkt->getSize() - MJL_Offset));
                        MJL_colOff = MJL_colOff + MJL_incColOff;
                    }
                } else {
                    memcpy(pkt->getPtr<uint8_t>(), hostAddr, pkt->getSize());
                }
                /* MJL_End */
                /* MJL_Comment
                memcpy(pkt->getPtr<uint8_t>(), hostAddr, pkt->getSize());
                */
                (*(pkt->getAtomicOp()))(hostAddr);
            }
        } else {
            std::vector<uint8_t> overwrite_val(pkt->getSize());
            uint64_t condition_val64;
            uint32_t condition_val32;

            if (!pmemAddr)
                panic("Swap only works if there is real memory (i.e. null=False)");

            bool overwrite_mem = true;
            // keep a copy of our possible write value, and copy what is at the
            // memory address into the packet
            std::memcpy(&overwrite_val[0], pkt->getConstPtr<uint8_t>(),
                        pkt->getSize());
            /* MJL_Begin */
            if (pkt->MJL_getDataDir() == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                std::memcpy(pkt->getPtr<uint8_t>(), hostAddr, pkt->getSize());
            } else if (pkt->MJL_getDataDir() == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                Addr MJL_incColOff = (Addr)(1 << (floorLog2(pkt->req->MJL_rowWidth) + floorLog2(pkt->req->MJL_cachelineSize)));
                Addr MJL_colOff = 0;
                for (Addr MJL_Offset = 0; MJL_Offset < pkt->getSize(); MJL_Offset = MJL_Offset + sizeof(uint64_t)) {
                    std::memcpy(pkt->getPtr<uint8_t>() + MJL_Offset, hostAddr + MJL_colOff, std::min(sizeof(uint64_t), pkt->getSize() - MJL_Offset));
                    MJL_colOff = MJL_colOff + MJL_incColOff;
                }
            } else {
                std::memcpy(pkt->getPtr<uint8_t>(), hostAddr, pkt->getSize());
            }
            /* MJL_End */
            /* MJL_Comment
            std::memcpy(pkt->getPtr<uint8_t>(), hostAddr, pkt->getSize());
            */

            if (pkt->req->isCondSwap()) {
                if (pkt->getSize() == sizeof(uint64_t)) {
                    condition_val64 = pkt->req->getExtraData();
                    overwrite_mem = !std::memcmp(&condition_val64, hostAddr,
                                                 sizeof(uint64_t));
                } else if (pkt->getSize() == sizeof(uint32_t)) {
                    condition_val32 = (uint32_t)pkt->req->getExtraData();
                    overwrite_mem = !std::memcmp(&condition_val32, hostAddr,
                                                 sizeof(uint32_t));
                } else
                    panic("Invalid size for conditional read/write\n");
            }

            if (overwrite_mem)
                /* MJL_Begin */
            {
                if (pkt->MJL_getDataDir() == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                    std::memcpy(hostAddr, &overwrite_val[0], pkt->getSize());
                } else if (pkt->MJL_getDataDir() == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                    Addr MJL_incColOff = (Addr)(1 << (floorLog2(pkt->req->MJL_rowWidth) + floorLog2(pkt->req->MJL_cachelineSize)));
                    Addr MJL_colOff = 0;
                    for (Addr MJL_Offset = 0; MJL_Offset < pkt->getSize(); MJL_Offset = MJL_Offset + sizeof(uint64_t)) {
                        std::memcpy(hostAddr + MJL_colOff, &overwrite_val[0] + MJL_Offset, std::min(sizeof(uint64_t), pkt->getSize() - MJL_Offset));
                        MJL_colOff = MJL_colOff + MJL_incColOff;
                    }
                } else {
                    std::memcpy(hostAddr, &overwrite_val[0], pkt->getSize());
                }
            }
                /* MJL_End */
                /* MJL_Comment
                std::memcpy(hostAddr, &overwrite_val[0], pkt->getSize());
                */

            assert(!pkt->req->isInstFetch());
            TRACE_PACKET("Read/Write");
            numOther[pkt->req->masterId()]++;
        }
    } else if (pkt->isRead()) {
        assert(!pkt->isWrite());
        if (pkt->isLLSC()) {
            trackLoadLocked(pkt);
        }
        if (pmemAddr)
            /* MJL_Begin */
        {
            if (pkt->MJL_getDataDir() == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                memcpy(pkt->getPtr<uint8_t>(), hostAddr, pkt->getSize());
            } else if (pkt->MJL_getDataDir() == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                Addr MJL_incColOff = (Addr)(1 << (floorLog2(pkt->req->MJL_rowWidth) + floorLog2(pkt->req->MJL_cachelineSize)));
                Addr MJL_colOff = 0;
                for (Addr MJL_Offset = 0; MJL_Offset < pkt->getSize(); MJL_Offset = MJL_Offset + sizeof(uint64_t)) {
                    memcpy(pkt->getPtr<uint8_t>() + MJL_Offset, hostAddr + MJL_colOff, std::min(sizeof(uint64_t), pkt->getSize() - MJL_Offset));
                    MJL_colOff = MJL_colOff + MJL_incColOff;
                }
            } else {
                memcpy(pkt->getPtr<uint8_t>(), hostAddr, pkt->getSize());
            }
        }
            /* MJL_End */
            /* MJL_Comment
            memcpy(pkt->getPtr<uint8_t>(), hostAddr, pkt->getSize());
            */
        TRACE_PACKET(pkt->req->isInstFetch() ? "IFetch" : "Read");
        numReads[pkt->req->masterId()]++;
        bytesRead[pkt->req->masterId()] += pkt->getSize();
        /* MJL_Begin */
        if (pkt->MJL_dataIsRow()) {
            MJL_bytesReadRow[pkt->req->masterId()] += pkt->getSize();
            MJL_bytesTotalRow += pkt->getSize();
        } else if (pkt->MJL_dataIsColumn()) {
            MJL_bytesReadColumn[pkt->req->masterId()] += pkt->getSize();
            MJL_bytesTotalColumn += pkt->getSize();
        }
        /* MJL_End */
        if (pkt->req->isInstFetch())
            bytesInstRead[pkt->req->masterId()] += pkt->getSize();
    } else if (pkt->isInvalidate()) {
        // no need to do anything
        // this clause is intentionally before the write clause: the only
        // transaction that is both a write and an invalidate is
        // WriteInvalidate, and for the sake of consistency, it does not
        // write to memory.  in a cacheless system, there are no WriteInv's
        // because the Write -> WriteInvalidate rewrite happens in the cache.
    } else if (pkt->isWrite()) {
        if (writeOK(pkt)) {
            if (pmemAddr) {
                /* MJL_Begin */
                if (pkt->MJL_getDataDir() == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                    memcpy(hostAddr, pkt->getPtr<uint8_t>(), pkt->getSize());
                } else if (pkt->MJL_getDataDir() == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                    Addr MJL_incColOff = (Addr)(1 << (floorLog2(pkt->req->MJL_rowWidth) + floorLog2(pkt->req->MJL_cachelineSize)));
                    Addr MJL_colOff = 0;
                    for (Addr MJL_Offset = 0; MJL_Offset < pkt->getSize(); MJL_Offset = MJL_Offset + sizeof(uint64_t)) {
                        memcpy(hostAddr + MJL_colOff, pkt->getPtr<uint8_t>() + MJL_Offset, std::min(sizeof(uint64_t), pkt->getSize() - MJL_Offset));
                        MJL_colOff = MJL_colOff + MJL_incColOff;
                    }
                } else {
                    memcpy(hostAddr, pkt->getPtr<uint8_t>(), pkt->getSize());
                }
                /* MJL_End */
                /* MJL_Comment
                memcpy(hostAddr, pkt->getConstPtr<uint8_t>(), pkt->getSize());
                */
                DPRINTF(MemoryAccess, "%s wrote %i bytes to address %x\n",
                        __func__, pkt->getSize(), pkt->getAddr());
            }
            assert(!pkt->req->isInstFetch());
            TRACE_PACKET("Write");
            numWrites[pkt->req->masterId()]++;
            bytesWritten[pkt->req->masterId()] += pkt->getSize();
            /* MJL_Begin */
            if (pkt->MJL_dataIsRow()) {
                MJL_bytesWrittenRow[pkt->req->masterId()] += pkt->getSize();
                MJL_bytesTotalRow += pkt->getSize();
            } else if (pkt->MJL_dataIsColumn()) {
                MJL_bytesWrittenColumn[pkt->req->masterId()] += pkt->getSize();
                MJL_bytesTotalColumn += pkt->getSize();
            }
            /* MJL_End */
        }
    } else {
        panic("unimplemented");
    }

    if (pkt->needsResponse()) {
        pkt->makeResponse();
    }
    /* MJL_Begin */
    /* MJL_Test: Packet information output  
    if (!pkt->req->isInstFetch() && pkt->isResponse()
         && pkt->req->hasPC()) { // Debug for ssyr2k column vec
        std::cout << this->name() << "::access()PostAcc";
        std::cout << ": PC(hex) = ";
        if (pkt->req->hasPC()) {
             std::cout << std::hex << pkt->req->getPC() << std::dec;
        } else {
             std::cout << "noPC";
        }
        std::cout << ", MemCmd = " << pkt->cmd.toString();
        std::cout << ", CmdDir = " << pkt->MJL_getCmdDir();
        std::cout << ", Addr(oct) = "<< std::oct << pkt->getAddr() << std::dec;
        std::cout << ", Size = " << pkt->getSize();
        if (pkt->hasData()) {
            std::cout << ", DataDir = " << pkt->MJL_getDataDir();
            uint64_t MJL_data = 0;
            std::cout << ", Data = " << std::hex;
            std::memcpy(&MJL_data, pkt->getConstPtr<uint8_t>(), std::min(sizeof(uint64_t), (Addr)pkt->getSize()));
            std::cout << "word[0] = " <<  MJL_data;
            for (unsigned i = sizeof(uint64_t); i < pkt->getSize(); i = i + sizeof(uint64_t)) {
                MJL_data = 0;
                std::memcpy(&MJL_data, pkt->getConstPtr<uint8_t>() + i, std::min(sizeof(uint64_t), pkt->getSize() - (Addr)i));
                std::cout << " | word[" << i/sizeof(uint64_t) << "] = " <<  MJL_data;
            }
            std::cout << std::dec;
        }
        std::cout << ", Time: " << pkt->req->time() << std::endl;
    }
     */
    /* MJL_End */
}

void
AbstractMemory::functionalAccess(PacketPtr pkt)
{
    /* MJL_Begin */
    /* MJL_Test: Packet information output 
    std::cout << this->name() << "::functionalAccess()";
    std::cout << ": PC(hex) = ";
    if (pkt->req->hasPC()) {
         std::cout << std::hex << pkt->req->getPC() << std::dec;
    } else {
         std::cout << "noPC";
    }
    std::cout << ", MemCmd = " << pkt->cmd.toString();
    std::cout << ", Addr(oct) = " << std::oct << pkt->getAddr() << std::dec;
    std::cout << ", Size = " << pkt->getSize();
    std::cout << ", Time: " << pkt->req->time() << std::endl;
     */
    /* MJL_End */
    assert(AddrRange(pkt->getAddr(),
                     pkt->getAddr() + pkt->getSize() - 1).isSubset(range));

    uint8_t *hostAddr = pmemAddr + pkt->getAddr() - range.start();

    if (pkt->isRead()) {
        if (pmemAddr)
        /* MJL_Begin */
        {
            if (pkt->MJL_getDataDir() == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                for (int i = 0; i < pkt->getSize(); ++i) {
                    if (!pkt->MJL_hasDirty(i)) {
                        memcpy(pkt->getPtr<uint8_t>() + i, hostAddr + i, 1);
                    }
                }
            } else if (pkt->MJL_getDataDir() == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                Addr MJL_incColOff = (Addr)(1 << (floorLog2(pkt->req->MJL_rowWidth) + floorLog2(pkt->req->MJL_cachelineSize)));
                Addr MJL_colOff = 0;
                for (Addr MJL_Offset = 0; MJL_Offset < pkt->getSize(); MJL_Offset = MJL_Offset + sizeof(uint64_t)) {
                    for (int i = 0; i < std::min(sizeof(uint64_t), pkt->getSize() - MJL_Offset); ++i) {
                        if (!pkt->MJL_hasDirty(MJL_Offset + i)) {
                            memcpy(pkt->getPtr<uint8_t>() + MJL_Offset + i, hostAddr + MJL_colOff + i, 1);
                        }
                    }
                    MJL_colOff = MJL_colOff + MJL_incColOff;
                }
            } else {
                for (int i = 0; i < pkt->getSize(); ++i) {
                    if (!pkt->MJL_hasDirty(i)) {
                        memcpy(pkt->getPtr<uint8_t>() + i, hostAddr + i, 1);
                    }
                }
            }
        }
        /* MJL_End */
        /* MJL_Comment
            memcpy(pkt->getPtr<uint8_t>(), hostAddr, pkt->getSize());
        */
        TRACE_PACKET("Read");
        pkt->makeResponse();
    } else if (pkt->isWrite()) {
        if (pmemAddr)
        /* MJL_Begin */
        {
            if (pkt->MJL_getDataDir() == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                memcpy(hostAddr, pkt->getConstPtr<uint8_t>(), pkt->getSize());
            } else if (pkt->MJL_getDataDir() == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                Addr MJL_incColOff = (Addr)(1 << (floorLog2(pkt->req->MJL_rowWidth) + floorLog2(pkt->req->MJL_cachelineSize)));
                Addr MJL_colOff = 0;
                for (Addr MJL_Offset = 0; MJL_Offset < pkt->getSize(); MJL_Offset = MJL_Offset + sizeof(uint64_t)) {
                    memcpy(hostAddr + MJL_colOff, pkt->getPtr<uint8_t>() + MJL_Offset, std::min(sizeof(uint64_t), pkt->getSize() - MJL_Offset));
                    MJL_colOff = MJL_colOff + MJL_incColOff;
                }
            } else {
                memcpy(hostAddr, pkt->getConstPtr<uint8_t>(), pkt->getSize());
            }
        }
        /* MJL_End */
        /* MJL_Comment
            memcpy(hostAddr, pkt->getConstPtr<uint8_t>(), pkt->getSize());
        */
        TRACE_PACKET("Write");
        pkt->makeResponse();
    } else if (pkt->isPrint()) {
        Packet::PrintReqState *prs =
            dynamic_cast<Packet::PrintReqState*>(pkt->senderState);
        assert(prs);
        // Need to call printLabels() explicitly since we're not going
        // through printObj().
        prs->printLabels();
        // Right now we just print the single byte at the specified address.
        ccprintf(prs->os, "%s%#x\n", prs->curPrefix(), *hostAddr);
    } else {
        panic("AbstractMemory: unimplemented functional command %s",
              pkt->cmdString());
    }
}
