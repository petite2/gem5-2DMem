/*
 * Copyright (c) 2012-2015 ARM Limited
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
 *          Andreas Sandberg
 */

/** @file
 * Definitions of a simple cache block class.
 */

#ifndef __MEM_CACHE_BLK_HH__
#define __MEM_CACHE_BLK_HH__

#include <list>

#include "base/printable.hh"
#include "mem/packet.hh"
#include "mem/request.hh"

/**
 * Cache block status bit assignments
 */
enum CacheBlkStatusBits : unsigned {
    /** valid, readable */
    BlkValid =          0x01,
    /** write permission */
    BlkWritable =       0x02,
    /** read permission (yes, block can be valid but not readable) */
    BlkReadable =       0x04,
    /** dirty (modified) */
    BlkDirty =          0x08,
    /** block was a hardware prefetch yet unaccessed*/
    BlkHWPrefetched =   0x20,
    /** block holds data from the secure memory space */
    BlkSecure =         0x40,
};


/**
 * A Basic Cache block.
 * Contains the tag, status, and a pointer to data.
 */
class CacheBlk
{
  public:
    /** Task Id associated with this block */
    uint32_t task_id;

    /** The address space ID of this block. */
    int asid;
    /** Data block tag value. */
    Addr tag;
    /**
     * Contains a copy of the data in this block for easy access. This is used
     * for efficient execution when the data could be actually stored in
     * another format (COW, compressed, sub-blocked, etc). In all cases the
     * data stored here should be kept consistant with the actual data
     * referenced by this block.
     */
    uint8_t *data;
    /** the number of bytes stored in this block. */
    unsigned size;

    /** block state: OR of CacheBlkStatusBit */
    typedef unsigned State;

    /** The current status of this block. @sa CacheBlockStatusBits */
    State status;

    /* MJL_Begin */
    /**
     * Cache block direction assignments. 
     */
    typedef MemCmd::MJL_DirAttribute MJL_CacheBlkDir;
    /*
    Invalid:    MJL_CacheBlkDir::MJL_IsInvalid
    Row:        MJL_CacheBlkDir::MJL_IsRow
    Column:     MJL_CacheBlkDir::MJL_IsColumn
    */
    
    /** block data direction */
    MJL_CacheBlkDir MJL_blkDir;
    /* MJL_End */

    /** Which curTick() will this block be accessable */
    Tick whenReady;

    /**
     * The set and way this block belongs to.
     * @todo Move this into subclasses when we fix CacheTags to use them.
     */
    int set, way;

    /** whether this block has been touched */
    bool isTouched;

    /** Number of references to this block since it was brought in. */
    unsigned refCount;

    /** holds the source requestor ID for this block. */
    int srcMasterId;

    Tick tickInserted;

  protected:
    /**
     * Represents that the indicated thread context has a "lock" on
     * the block, in the LL/SC sense.
     */
    class Lock {
      public:
        ContextID contextId;     // locking context
        Addr lowAddr;      // low address of lock range
        Addr highAddr;     // high address of lock range
        /* MJL_Begin */
        Request::MJL_DirAttribute MJL_reqDir;
        inline int
        floorLog2(unsigned long x) const
        {
            assert(x > 0);

            int y = 0;

        #if defined(__LP64__)
            if (x & ULL(0xffffffff00000000)) { y += 32; x >>= 32; }
        #endif
            if (x & 0xffff0000) { y += 16; x >>= 16; }
            if (x & 0x0000ff00) { y +=  8; x >>=  8; }
            if (x & 0x000000f0) { y +=  4; x >>=  4; }
            if (x & 0x0000000c) { y +=  2; x >>=  2; }
            if (x & 0x00000002) { y +=  1; }

            return y;
        }
        Addr MJL_swapRowColBits(Addr addr, unsigned blkSize, unsigned MJL_rowWidth) const 
        {
            int MJL_rowShift = floorLog2(sizeof(uint64_t));
            uint64_t MJL_wordMask = blkSize/sizeof(uint64_t) - 1;
            int MJL_colShift = floorLog2(MJL_rowWidth) + floorLog2(blkSize);

            Addr new_row = (addr >> MJL_colShift) & (Addr)MJL_wordMask;
            Addr new_col = (addr >> MJL_rowShift) & (Addr)MJL_wordMask;
            return ((addr & ~(((Addr)MJL_wordMask << MJL_colShift) | ((Addr)MJL_wordMask << MJL_rowShift))) | (new_row << MJL_rowShift) | (new_col << MJL_colShift));
        }
        Addr MJL_commonMask(Addr addr, unsigned blkSize, unsigned MJL_rowWidth) const 
        {
            int MJL_rowShift = floorLog2(sizeof(uint64_t));
            uint64_t MJL_wordMask = blkSize/sizeof(uint64_t) - 1;
            int MJL_colShift = floorLog2(MJL_rowWidth) + floorLog2(blkSize);

            return (addr & ~(((Addr)MJL_wordMask << MJL_colShift) | ((Addr)MJL_wordMask << MJL_rowShift)));
        }
        /* MJL_End */

        // check for matching execution context, and an address that
        // is within the lock
        bool matches(const RequestPtr req) const
        {
            Addr req_low = req->getPaddr();
            Addr req_high;
            /* MJL_Begin */
            if ( req->MJL_reqIsRow() ) {
                req_high = req_low + req->getSize() -1;
            } else if ( req->MJL_reqIsColumn() ) { //MJL_temp temporary fix for column
                req_high = MJL_swapRowColBits(MJL_swapRowColBits(req_low, req->MJL_cachelineSize, req->MJL_rowWidth) + req->getSize() -1, req->MJL_cachelineSize, req->MJL_rowWidth);
            } else {
                req_high = req_low + req->getSize() -1;
            }
            if ( req->getSize() <= sizeof(uint64_t) ) {
                return (contextId == req->contextId()) &&
                   (req_low >= lowAddr) && (req_high <= highAddr);
            }
            return (contextId == req->contextId()) && (MJL_reqDir == req->MJL_getReqDir()) &&
                   (req_low >= lowAddr) && (req_high <= highAddr);
            /* MJL_End */
            /* MJL_Comment 
            Addr req_high = req_low + req->getSize() -1;
            return (contextId == req->contextId()) &&
                   (req_low >= lowAddr) && (req_high <= highAddr);
            */
        }

        // check if a request is intersecting and thus invalidating the lock
        bool intersects(const RequestPtr req) const
        {
            Addr req_low = req->getPaddr();
            Addr req_high;
            /* MJL_Begin */
            if ( req->MJL_reqIsRow() ) {
                req_high = req_low + req->getSize() -1;
            } else if ( req->MJL_reqIsColumn() ) { //MJL_temp temporary fix for column
                req_high = MJL_swapRowColBits(MJL_swapRowColBits(req_low, req->MJL_cachelineSize, req->MJL_rowWidth) + req->getSize() -1, req->MJL_cachelineSize, req->MJL_rowWidth);
            } else {
                req_high = req_low + req->getSize() -1;
            }
            if ( (req->getSize() <= sizeof(uint64_t)) || (MJL_reqDir == req->MJL_getReqDir()) ) {
                return (req_low <= highAddr) && (req_high >= lowAddr);
            } else {
                //MJL_temp temporary fix for column, assuming that req->getSize() is smaller cacheline size
                assert(req->getSize() <= req->MJL_cachelineSize);
                return (MJL_commonMask(req_low, req->MJL_cachelineSize, req->MJL_rowWidth) == MJL_commonMask(lowAddr, req->MJL_cachelineSize, req->MJL_rowWidth));
            }
            /* MJL_End */
            /* MJL_Comment
            Addr req_high = req_low + req->getSize() - 1;

            return (req_low <= highAddr) && (req_high >= lowAddr);
            */
        }

        Lock(const RequestPtr req)
            : contextId(req->contextId()),
              lowAddr(req->getPaddr()),
              highAddr(lowAddr + req->getSize() - 1)
              /* MJL_Begin */
              , MJL_reqDir(req->MJL_getReqDir())
              /* MJL_End */
        {
            /* MJL_Begin */
            highAddr = MJL_swapRowColBits(MJL_swapRowColBits(lowAddr, req->MJL_cachelineSize, req->MJL_rowWidth) + req->getSize() -1, req->MJL_cachelineSize, req->MJL_rowWidth);
            /* MJL_End */
        }
    };

    /** List of thread contexts that have performed a load-locked (LL)
     * on the block since the last store. */
    std::list<Lock> lockList;

  public:

    CacheBlk()
        : task_id(ContextSwitchTaskId::Unknown),
          asid(-1), tag(0), data(0) ,size(0), status(0),
          /* MJL_Begin */ 
          MJL_blkDir(MJL_CacheBlkDir::MJL_IsRow),
          /* MJL_End */
           whenReady(0),
          set(-1), way(-1), isTouched(false), refCount(0),
          srcMasterId(Request::invldMasterId),
          tickInserted(0)
    {}

    CacheBlk(const CacheBlk&) = delete;
    CacheBlk& operator=(const CacheBlk&) = delete;

    /**
     * Checks the write permissions of this block.
     * @return True if the block is writable.
     */
    bool isWritable() const
    {
        const State needed_bits = BlkWritable | BlkValid;
        return (status & needed_bits) == needed_bits;
    }

    /**
     * Checks the read permissions of this block.  Note that a block
     * can be valid but not readable if there is an outstanding write
     * upgrade miss.
     * @return True if the block is readable.
     */
    bool isReadable() const
    {
        const State needed_bits = BlkReadable | BlkValid;
        return (status & needed_bits) == needed_bits;
    }

    /**
     * Checks that a block is valid.
     * @return True if the block is valid.
     */
    bool isValid() const
    {
        return (status & BlkValid) != 0;
    }

    /**
     * Invalidate the block and clear all state.
     */
    void invalidate()
    {
        status = 0;
        isTouched = false;
        lockList.clear();
    }

    /**
     * Check to see if a block has been written.
     * @return True if the block is dirty.
     */
    bool isDirty() const
    {
        return (status & BlkDirty) != 0;
    }

    /**
     * Check if this block was the result of a hardware prefetch, yet to
     * be touched.
     * @return True if the block was a hardware prefetch, unaccesed.
     */
    bool wasPrefetched() const
    {
        return (status & BlkHWPrefetched) != 0;
    }

    /**
     * Check if this block holds data from the secure memory space.
     * @return True if the block holds data from the secure memory space.
     */
    bool isSecure() const
    {
        return (status & BlkSecure) != 0;
    }

    /* MJL_Begin */
    /**
     * Check if this block holds data in row direction
     * @return Truc if the block holds data in row direction
     */
    bool MJL_isRow() const
    {
        return MJL_blkDir == MJL_CacheBlkDir::MJL_IsRow;
    }
    
    /**
     * Check if this block holds data in column direction
     * @return Truc if the block holds data in column direction
     */
    bool MJL_isColumn() const
    {
        return MJL_blkDir == MJL_CacheBlkDir::MJL_IsColumn;
    }
    /* MJL_End */

    /**
     * Track the fact that a local locked was issued to the
     * block. Invalidate any previous LL to the same address.
     */
    void trackLoadLocked(PacketPtr pkt)
    {
        assert(pkt->isLLSC());
        auto l = lockList.begin();
        while (l != lockList.end()) {
            if (l->intersects(pkt->req))
                l = lockList.erase(l);
            else
                ++l;
        }

        lockList.emplace_front(pkt->req);
    }

    /**
     * Clear the any load lock that intersect the request, and is from
     * a different context.
     */
    void clearLoadLocks(RequestPtr req)
    {
        auto l = lockList.begin();
        while (l != lockList.end()) {
            if (l->intersects(req) && l->contextId != req->contextId()) {
                l = lockList.erase(l);
            } else {
                ++l;
            }
        }
    }

    /**
     * Pretty-print a tag, and interpret state bits to readable form
     * including mapping to a MOESI state.
     *
     * @return string with basic state information
     */
    std::string print() const
    {
        /**
         *  state       M   O   E   S   I
         *  writable    1   0   1   0   0
         *  dirty       1   1   0   0   0
         *  valid       1   1   1   1   0
         *
         *  state   writable    dirty   valid
         *  M       1           1       1
         *  O       0           1       1
         *  E       1           0       1
         *  S       0           0       1
         *  I       0           0       0
         *
         * Note that only one cache ever has a block in Modified or
         * Owned state, i.e., only one cache owns the block, or
         * equivalently has the BlkDirty bit set. However, multiple
         * caches on the same path to memory can have a block in the
         * Exclusive state (despite the name). Exclusive means this
         * cache has the only copy at this level of the hierarchy,
         * i.e., there may be copies in caches above this cache (in
         * various states), but there are no peers that have copies on
         * this branch of the hierarchy, and no caches at or above
         * this level on any other branch have copies either.
         **/
        unsigned state = isWritable() << 2 | isDirty() << 1 | isValid();
        char s = '?';
        switch (state) {
          case 0b111: s = 'M'; break;
          case 0b011: s = 'O'; break;
          case 0b101: s = 'E'; break;
          case 0b001: s = 'S'; break;
          case 0b000: s = 'I'; break;
          default:    s = 'T'; break; // @TODO add other types
        }
        return csprintf("state: %x (%c) valid: %d writable: %d readable: %d "
                        "dirty: %d tag: %x", status, s, isValid(),
                        isWritable(), isReadable(), isDirty(), tag);
    }

    /**
     * Handle interaction of load-locked operations and stores.
     * @return True if write should proceed, false otherwise.  Returns
     * false only in the case of a failed store conditional.
     */
    bool checkWrite(PacketPtr pkt)
    {
        assert(pkt->isWrite());

        // common case
        if (!pkt->isLLSC() && lockList.empty())
            return true;

        RequestPtr req = pkt->req;

        if (pkt->isLLSC()) {
            // it's a store conditional... have to check for matching
            // load locked.
            bool success = false;

            auto l = lockList.begin();
            while (!success && l != lockList.end()) {
                if (l->matches(pkt->req)) {
                    // it's a store conditional, and as far as the
                    // memory system can tell, the requesting
                    // context's lock is still valid.
                    success = true;
                    lockList.erase(l);
                } else {
                    ++l;
                }
            }

            req->setExtraData(success ? 1 : 0);
            // clear any intersected locks from other contexts (our LL
            // should already have cleared them)
            clearLoadLocks(req);
            return success;
        } else {
            // a normal write, if there is any lock not from this
            // context we clear the list, thus for a private cache we
            // never clear locks on normal writes
            clearLoadLocks(req);
            return true;
        }
    }
};

/**
 * Simple class to provide virtual print() method on cache blocks
 * without allocating a vtable pointer for every single cache block.
 * Just wrap the CacheBlk object in an instance of this before passing
 * to a function that requires a Printable object.
 */
class CacheBlkPrintWrapper : public Printable
{
    CacheBlk *blk;
  public:
    CacheBlkPrintWrapper(CacheBlk *_blk) : blk(_blk) {}
    virtual ~CacheBlkPrintWrapper() {}
    void print(std::ostream &o, int verbosity = 0,
               const std::string &prefix = "") const;
};

/**
 * Base class for cache block visitor, operating on the cache block
 * base class (later subclassed for the various tag classes). This
 * visitor class is used as part of the forEachBlk interface in the
 * tag classes.
 */
class CacheBlkVisitor
{
  public:

    CacheBlkVisitor() {}
    virtual ~CacheBlkVisitor() {}

    virtual bool operator()(CacheBlk &blk) = 0;
};

#endif //__MEM_CACHE_BLK_HH__
