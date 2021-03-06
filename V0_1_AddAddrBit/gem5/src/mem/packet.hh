/*
 * Copyright (c) 2012-2016 ARM Limited
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
 * Copyright (c) 2006 The Regents of The University of Michigan
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
 * Authors: Ron Dreslinski
 *          Steve Reinhardt
 *          Ali Saidi
 *          Andreas Hansson
 */

/**
 * @file
 * Declaration of the Packet class.
 */

#ifndef __MEM_PACKET_HH__
#define __MEM_PACKET_HH__

#include <bitset>
#include <cassert>
#include <list>

#include "base/cast.hh"
#include "base/compiler.hh"
#include "base/flags.hh"
#include "base/misc.hh"
#include "base/printable.hh"
#include "base/types.hh"
#include "mem/request.hh"
#include "sim/core.hh"

class Packet;
typedef Packet *PacketPtr;
typedef uint8_t* PacketDataPtr;
typedef std::list<PacketPtr> PacketList;

class MemCmd
{
    friend class Packet;

  public:
    /**
     * List of all commands associated with a packet.
     */
    enum Command
    {
        InvalidCmd,
        ReadReq,
        ReadResp,
        ReadRespWithInvalidate,
        WriteReq,
        WriteResp,
        WritebackDirty,
        WritebackClean,
        CleanEvict,
        SoftPFReq,
        HardPFReq,
        SoftPFResp,
        HardPFResp,
        WriteLineReq,
        UpgradeReq,
        SCUpgradeReq,           // Special "weak" upgrade for StoreCond
        UpgradeResp,
        SCUpgradeFailReq,       // Failed SCUpgradeReq in MSHR (never sent)
        UpgradeFailResp,        // Valid for SCUpgradeReq only
        ReadExReq,
        ReadExResp,
        ReadCleanReq,
        ReadSharedReq,
        LoadLockedReq,
        StoreCondReq,
        StoreCondFailReq,       // Failed StoreCondReq in MSHR (never sent)
        StoreCondResp,
        SwapReq,
        SwapResp,
        MessageReq,
        MessageResp,
        MemFenceReq,
        MemFenceResp,
        // Error responses
        // @TODO these should be classified as responses rather than
        // requests; coding them as requests initially for backwards
        // compatibility
        InvalidDestError,  // packet dest field invalid
        BadAddressError,   // memory address invalid
        FunctionalReadError, // unable to fulfill functional read
        FunctionalWriteError, // unable to fulfill functional write
        // Fake simulator-only commands
        PrintReq,       // Print state matching address
        FlushReq,      //request for a cache flush
        InvalidateReq,   // request for address to be invalidated
        InvalidateResp,
        NUM_MEM_CMDS
    };

    /* MJL_Begin */
    /**
     * List of command direction attributes.
     */
    typedef Request::MJL_DirAttribute MJL_DirAttribute;
    // enum MJL_DirAttribute
    // {
    //     MJL_IsInvalid,  //!< Data access direction is invalid
    //     MJL_IsRow,      //!< Data access direction is row
    //     MJL_IsColumn,   //!< Data access direction is column
    //     MJL_NUM_COMMAND_DIRATTRIBUTES
    // };
    /* MJL_End */

  private:
    /**
     * List of command attributes.
     */
    enum Attribute
    {
        IsRead,         //!< Data flows from responder to requester
        IsWrite,        //!< Data flows from requester to responder
        IsUpgrade,
        IsInvalidate,
        NeedsWritable,  //!< Requires writable copy to complete in-cache
        IsRequest,      //!< Issued by requester
        IsResponse,     //!< Issue by responder
        NeedsResponse,  //!< Requester needs response from target
        IsEviction,
        IsSWPrefetch,
        IsHWPrefetch,
        IsLlsc,         //!< Alpha/MIPS LL or SC access
        HasData,        //!< There is an associated payload
        IsError,        //!< Error response
        IsPrint,        //!< Print state matching address (for debugging)
        IsFlush,        //!< Flush the address from caches
        FromCache,      //!< Request originated from a caching agent
        NUM_COMMAND_ATTRIBUTES
    };

    /**
     * Structure that defines attributes and other data associated
     * with a Command.
     */
    struct CommandInfo
    {
        /// Set of attribute flags.
        const std::bitset<NUM_COMMAND_ATTRIBUTES> attributes;
        /// Corresponding response for requests; InvalidCmd if no
        /// response is applicable.
        const Command response;
        /// String representation (for printing)
        const std::string str;
    };

    /// Array to map Command enum to associated info.
    static const CommandInfo commandInfo[];

  private:

    Command cmd;
    /* MJL_Begin */
    MJL_DirAttribute MJL_cmdDir;
    /* MJL_End */

    bool
    testCmdAttrib(MemCmd::Attribute attrib) const
    {
        return commandInfo[cmd].attributes[attrib] != 0;
    }

  public:

    bool isRead() const            { return testCmdAttrib(IsRead); }
    bool isWrite() const           { return testCmdAttrib(IsWrite); }
    bool isUpgrade() const         { return testCmdAttrib(IsUpgrade); }
    bool isRequest() const         { return testCmdAttrib(IsRequest); }
    bool isResponse() const        { return testCmdAttrib(IsResponse); }
    bool needsWritable() const     { return testCmdAttrib(NeedsWritable); }
    bool needsResponse() const     { return testCmdAttrib(NeedsResponse); }
    bool isInvalidate() const      { return testCmdAttrib(IsInvalidate); }
    bool isEviction() const        { return testCmdAttrib(IsEviction); }
    bool fromCache() const         { return testCmdAttrib(FromCache); }
    /* MJL_Begin */
    bool MJL_isRow() const         { return (MJL_cmdDir == MJL_DirAttribute::MJL_IsRow); }
    bool MJL_isColumn() const      { return (MJL_cmdDir == MJL_DirAttribute::MJL_IsColumn); }
    // MJL_Test method to get cmd
    Command MJL_getCmd()           { return cmd; }
    /* MJL_End */

    /**
     * A writeback is an eviction that carries data.
     */
    bool isWriteback() const       { return testCmdAttrib(IsEviction) &&
                                            testCmdAttrib(HasData); }

    /**
     * Check if this particular packet type carries payload data. Note
     * that this does not reflect if the data pointer of the packet is
     * valid or not.
     */
    bool hasData() const        { return testCmdAttrib(HasData); }
    bool isLLSC() const         { return testCmdAttrib(IsLlsc); }
    bool isSWPrefetch() const   { return testCmdAttrib(IsSWPrefetch); }
    bool isHWPrefetch() const   { return testCmdAttrib(IsHWPrefetch); }
    bool isPrefetch() const     { return testCmdAttrib(IsSWPrefetch) ||
                                         testCmdAttrib(IsHWPrefetch); }
    bool isError() const        { return testCmdAttrib(IsError); }
    bool isPrint() const        { return testCmdAttrib(IsPrint); }
    bool isFlush() const        { return testCmdAttrib(IsFlush); }

    Command
    responseCommand() const
    {
        return commandInfo[cmd].response;
    }

    /// Return the string to a cmd given by idx.
    const std::string &toString() const { return commandInfo[cmd].str; }
    int toInt() const { return (int)cmd; }

    /* MJL_Begin */
    /**
     * Set the command's data access direction
     */
    void MJL_setCmdDir(MJL_DirAttribute in_MJL_cmdDir) { MJL_cmdDir = in_MJL_cmdDir; }
    MJL_DirAttribute MJL_getCmdDir() const { return MJL_cmdDir; }

    /** 
     * Overload the operator=(Command _cmd) and (int _cmd) so that
     * the MemCmd(Command _cmd) and MemCmd(int _cmd) methods doesn't 
     * get called when cmd = MemCmd::xxx is used and resets the
     * MJL_cmdDir to default.
     */
    MemCmd operator=(Command _cmd) { this->cmd = _cmd; return *this; }
    MemCmd operator=(int _cmd) { this->cmd = (Command)_cmd; return *this; }
    // Implicit already covered this? MemCmd(MemCmd c2) : cmd(c2.cmd), MJL_cmdDir(c2.MJL_cmdDir) { }
    /* MJL_End */
    MemCmd(Command _cmd) : cmd(_cmd)/* MJL_Begin */, MJL_cmdDir(MJL_DirAttribute::MJL_IsRow) /* MJL_End */ { }
    MemCmd(int _cmd) : cmd((Command)_cmd)/* MJL_Begin */, MJL_cmdDir(MJL_DirAttribute::MJL_IsRow) /* MJL_End */ { }
    MemCmd() : cmd(InvalidCmd)/* MJL_Begin */, MJL_cmdDir(MJL_DirAttribute::MJL_IsRow) /* MJL_End */ { }

    /* MJL_Comment
        Not changing operator to include comparison on MJL_cmdDir due to the use of the operators in the form of cmd == MemCmd::xxx
    */
    bool operator==(MemCmd c2) const { return (cmd == c2.cmd); }
    bool operator!=(MemCmd c2) const { return (cmd != c2.cmd); }
};

/**
 * A Packet is used to encapsulate a transfer between two objects in
 * the memory system (e.g., the L1 and L2 cache).  (In contrast, a
 * single Request travels all the way from the requester to the
 * ultimate destination and back, possibly being conveyed by several
 * different Packets along the way.)
 */
class Packet : public Printable
{
  public:
    typedef uint32_t FlagsType;
    typedef ::Flags<FlagsType> Flags;

  private:

    enum : FlagsType {
        // Flags to transfer across when copying a packet
        COPY_FLAGS             = 0x0000000F,

        // Does this packet have sharers (which means it should not be
        // considered writable) or not. See setHasSharers below.
        HAS_SHARERS            = 0x00000001,

        // Special control flags
        /// Special timing-mode atomic snoop for multi-level coherence.
        EXPRESS_SNOOP          = 0x00000002,

        /// Allow a responding cache to inform the cache hierarchy
        /// that it had a writable copy before responding. See
        /// setResponderHadWritable below.
        RESPONDER_HAD_WRITABLE = 0x00000004,

        // Snoop co-ordination flag to indicate that a cache is
        // responding to a snoop. See setCacheResponding below.
        CACHE_RESPONDING       = 0x00000008,

        /// Are the 'addr' and 'size' fields valid?
        VALID_ADDR             = 0x00000100,
        VALID_SIZE             = 0x00000200,

        /// Is the data pointer set to a value that shouldn't be freed
        /// when the packet is destroyed?
        STATIC_DATA            = 0x00001000,
        /// The data pointer points to a value that should be freed when
        /// the packet is destroyed. The pointer is assumed to be pointing
        /// to an array, and delete [] is consequently called
        DYNAMIC_DATA           = 0x00002000,

        /// suppress the error if this packet encounters a functional
        /// access failure.
        SUPPRESS_FUNC_ERROR    = 0x00008000,

        // Signal block present to squash prefetch and cache evict packets
        // through express snoop flag
        BLOCK_CACHED          = 0x00010000
    };

    Flags flags;

  public:
    typedef MemCmd::Command Command;

    /// The command field of the packet.
    MemCmd cmd;

    /// A pointer to the original request.
    const RequestPtr req;

  private:
   /**
    * A pointer to the data being transfered.  It can be differnt
    * sizes at each level of the heirarchy so it belongs in the
    * packet, not request. This may or may not be populated when a
    * responder recieves the packet. If not populated it memory should
    * be allocated.
    */
    PacketDataPtr data;

    /// The address of the request.  This address could be virtual or
    /// physical, depending on the system configuration.
    Addr addr;
    /* MJL_Begin */
    MemCmd::MJL_DirAttribute MJL_dataDir;
    MemCmd::MJL_DirAttribute MJL_pfPredDir;
    /* MJL_End */

    /// True if the request targets the secure memory space.
    bool _isSecure;

    /// The size of the request or transfer.
    unsigned size;

    /**
     * Track the bytes found that satisfy a functional read.
     */
    std::vector<bool> bytesValid;
    /* MJL_Begin */
    std::vector<bool> MJL_bytesValid;
    std::vector<bool> MJL_bytesDirty;
    /* MJL_End */

  public:

    /**
     * The extra delay from seeing the packet until the header is
     * transmitted. This delay is used to communicate the crossbar
     * forwarding latency to the neighbouring object (e.g. a cache)
     * that actually makes the packet wait. As the delay is relative,
     * a 32-bit unsigned should be sufficient.
     */
    uint32_t headerDelay;

    /**
     * Keep track of the extra delay incurred by snooping upwards
     * before sending a request down the memory system. This is used
     * by the coherent crossbar to account for the additional request
     * delay.
     */
    uint32_t snoopDelay;

    /**
     * The extra pipelining delay from seeing the packet until the end of
     * payload is transmitted by the component that provided it (if
     * any). This includes the header delay. Similar to the header
     * delay, this is used to make up for the fact that the
     * crossbar does not make the packet wait. As the delay is
     * relative, a 32-bit unsigned should be sufficient.
     */
    uint32_t payloadDelay;

    /**
     * A virtual base opaque structure used to hold state associated
     * with the packet (e.g., an MSHR), specific to a MemObject that
     * sees the packet. A pointer to this state is returned in the
     * packet's response so that the MemObject in question can quickly
     * look up the state needed to process it. A specific subclass
     * would be derived from this to carry state specific to a
     * particular sending device.
     *
     * As multiple MemObjects may add their SenderState throughout the
     * memory system, the SenderStates create a stack, where a
     * MemObject can add a new Senderstate, as long as the
     * predecessing SenderState is restored when the response comes
     * back. For this reason, the predecessor should always be
     * populated with the current SenderState of a packet before
     * modifying the senderState field in the request packet.
     */
    struct SenderState
    {
        SenderState* predecessor;
        SenderState() : predecessor(NULL) {}
        virtual ~SenderState() {}
    };

    /**
     * Object used to maintain state of a PrintReq.  The senderState
     * field of a PrintReq should always be of this type.
     */
    class PrintReqState : public SenderState
    {
      private:
        /**
         * An entry in the label stack.
         */
        struct LabelStackEntry
        {
            const std::string label;
            std::string *prefix;
            bool labelPrinted;
            LabelStackEntry(const std::string &_label, std::string *_prefix);
        };

        typedef std::list<LabelStackEntry> LabelStack;
        LabelStack labelStack;

        std::string *curPrefixPtr;

      public:
        std::ostream &os;
        const int verbosity;

        PrintReqState(std::ostream &os, int verbosity = 0);
        ~PrintReqState();

        /**
         * Returns the current line prefix.
         */
        const std::string &curPrefix() { return *curPrefixPtr; }

        /**
         * Push a label onto the label stack, and prepend the given
         * prefix string onto the current prefix.  Labels will only be
         * printed if an object within the label's scope is printed.
         */
        void pushLabel(const std::string &lbl,
                       const std::string &prefix = "  ");

        /**
         * Pop a label off the label stack.
         */
        void popLabel();

        /**
         * Print all of the pending unprinted labels on the
         * stack. Called by printObj(), so normally not called by
         * users unless bypassing printObj().
         */
        void printLabels();

        /**
         * Print a Printable object to os, because it matched the
         * address on a PrintReq.
         */
        void printObj(Printable *obj);
    };

    /**
     * This packet's sender state.  Devices should use dynamic_cast<>
     * to cast to the state appropriate to the sender.  The intent of
     * this variable is to allow a device to attach extra information
     * to a request. A response packet must return the sender state
     * that was attached to the original request (even if a new packet
     * is created).
     */
    SenderState *senderState;

    /**
     * Push a new sender state to the packet and make the current
     * sender state the predecessor of the new one. This should be
     * prefered over direct manipulation of the senderState member
     * variable.
     *
     * @param sender_state SenderState to push at the top of the stack
     */
    void pushSenderState(SenderState *sender_state);

    /**
     * Pop the top of the state stack and return a pointer to it. This
     * assumes the current sender state is not NULL. This should be
     * preferred over direct manipulation of the senderState member
     * variable.
     *
     * @return The current top of the stack
     */
    SenderState *popSenderState();

    /**
     * Go through the sender state stack and return the first instance
     * that is of type T (as determined by a dynamic_cast). If there
     * is no sender state of type T, NULL is returned.
     *
     * @return The topmost state of type T
     */
    template <typename T>
    T * findNextSenderState() const
    {
        T *t = NULL;
        SenderState* sender_state = senderState;
        while (t == NULL && sender_state != NULL) {
            t = dynamic_cast<T*>(sender_state);
            sender_state = sender_state->predecessor;
        }
        return t;
    }

    /// Return the string name of the cmd field (for debugging and
    /// tracing).
    const std::string &cmdString() const { return cmd.toString(); }

    /// Return the index of this command.
    inline int cmdToIndex() const { return cmd.toInt(); }

    /* MJL_Begin */
    int MJL_testSeq; // Used to identify packet during test
    bool MJL_hasOrder;
    Counter MJL_order;
    bool MJL_wordDirty[8]; // Used to identify dirty words for cross direction checks
    bool MJL_crossBlocksCached[8]; // Used to identify whether the crossing blocks are cached in above caches in physicaly 2D L2 cache mode
    bool MJL_wordDemanded[8]; // Used to identify words demanded
    bool MJL_isStale[8]; // Used to identify whether the response data will have stale data due to modification on crossing block on miss
    bool MJL_hasSharersFlag; // Used to prevent illegal passing of writable
    void MJL_copyWordDemanded( bool in_MJL_wordDemanded[8] ) {
        for (int i = 0; i < 8; ++i) {
            MJL_wordDemanded[i] = in_MJL_wordDemanded[i];
        }
    }
    void MJL_copyCrossBlocksCached( bool in_MJL_crossBlocksCached[8] ) {
        for (int i = 0; i < 8; ++i) {
            MJL_crossBlocksCached[i] |= in_MJL_crossBlocksCached[i];
        }
    }
    void MJL_setWordDirtyFromBlk( bool MJL_blkWordDirty[8], unsigned blkSize) {
        for (int i = 0; i < getSize(); i = i + sizeof(uint64_t)) {
            MJL_wordDirty[i/sizeof(uint64_t)] = MJL_blkWordDirty[(i + getOffset(blkSize))/sizeof(uint64_t)];
        }
    }
    void MJL_copyWordDirty( bool MJL_otherWordDirty[]) {
        for (int i = 0; i < size/sizeof(uint64_t); ++i) {
            MJL_wordDirty[i] = MJL_otherWordDirty[i];
        }
    }
    void MJL_setAllDirty() {
        for (int i = 0; i < getSize(); i = i + sizeof(uint64_t)) {
            MJL_wordDirty[i/sizeof(uint64_t)] = true;
        }
    }
    bool MJL_hasStale() const {
        bool MJL_hasStaleWord = false;
        for (int i = 0; i < getSize(); i = i + sizeof(uint64_t)) {
            MJL_hasStaleWord |= MJL_isStale[i/sizeof(uint64_t)];
        }
        return MJL_hasStaleWord;
    }
    void MJL_copyIsStale( bool in_MJL_isStale[8] ) {
        for (int i = 0; i < 8; ++i) {
            MJL_isStale[i] = in_MJL_isStale[i];
        }
    }
    void MJL_setIsStaleFromResp( bool in_MJL_isStale[8], MemCmd::MJL_DirAttribute in_pkt_dir, unsigned blkSize ) {
        for (int i = 0; i < getSize(); i = i + sizeof(uint64_t)) {
            MJL_isStale[i/sizeof(uint64_t)] |= in_MJL_isStale[(i + MJL_getDirOffset(blkSize, in_pkt_dir))/sizeof(uint64_t)];
        }
    }
    bool MJL_checkIsStaleFromResp( bool in_MJL_isStale[8], MemCmd::MJL_DirAttribute in_pkt_dir, unsigned blkSize ) const {
        bool MJL_willHaveStale = false;
        for (int i = 0; i < getSize(); i = i + sizeof(uint64_t)) {
            MJL_willHaveStale |= in_MJL_isStale[(i + MJL_getDirOffset(blkSize, in_pkt_dir))/sizeof(uint64_t)];
        }
        return MJL_willHaveStale;
    }
    void MJL_setHasSharers() {
        MJL_hasSharersFlag = true;
    }
    bool MJL_hasSharers() const {
        return MJL_hasSharersFlag;
    }
    MemCmd::MJL_DirAttribute MJL_getCmdDir() const { return cmd.MJL_getCmdDir(); }
    MemCmd::MJL_DirAttribute MJL_getCrossCmdDir() const {
        if (cmd.MJL_isRow()) {
            return MemCmd::MJL_DirAttribute::MJL_IsColumn;
        } else if (cmd.MJL_isColumn()) {
            return MemCmd::MJL_DirAttribute::MJL_IsRow;
        } else {
            return MemCmd::MJL_DirAttribute::MJL_IsRow;
        }
    }
    bool MJL_cmdIsRow() const           { return cmd.MJL_isRow(); }
    bool MJL_cmdIsColumn() const        { return cmd.MJL_isColumn(); }
    MemCmd::MJL_DirAttribute MJL_getDataDir() const {return MJL_dataDir; }
    bool MJL_dataIsRow() const          { return MJL_dataDir == MemCmd::MJL_DirAttribute::MJL_IsRow; }
    bool MJL_dataIsColumn() const       { return MJL_dataDir == MemCmd::MJL_DirAttribute::MJL_IsColumn; }
    bool MJL_sameCmdDataDir() const     { return MJL_dataDir == cmd.MJL_getCmdDir(); }
    void MJL_setDataDir( MemCmd::MJL_DirAttribute in_MJL_dataDir ) { MJL_dataDir = in_MJL_dataDir; }
    MemCmd::MJL_DirAttribute MJL_getPfPredDir() const { return MJL_pfPredDir; }
    void MJL_setPfPredDir( MemCmd::MJL_DirAttribute in_MJL_pfPredDir ) { MJL_pfPredDir = in_MJL_pfPredDir; }
    bool MJL_hasDirty(int MJL_byte) {
        if (MJL_bytesDirty.empty())
            MJL_bytesDirty.resize(getSize(), false);
        return MJL_bytesDirty[MJL_byte];
    }
    /* MJL_End */
    bool isRead() const              { return cmd.isRead(); }
    bool isWrite() const             { return cmd.isWrite(); }
    bool isUpgrade()  const          { return cmd.isUpgrade(); }
    bool isRequest() const           { return cmd.isRequest(); }
    bool isResponse() const          { return cmd.isResponse(); }
    bool needsWritable() const
    {
        // we should never check if a response needsWritable, the
        // request has this flag, and for a response we should rather
        // look at the hasSharers flag (if not set, the response is to
        // be considered writable)
        assert(isRequest());
        return cmd.needsWritable();
    }
    bool needsResponse() const       { return cmd.needsResponse(); }
    bool isInvalidate() const        { return cmd.isInvalidate(); }
    bool isEviction() const          { return cmd.isEviction(); }
    bool fromCache() const           { return cmd.fromCache(); }
    bool isWriteback() const         { return cmd.isWriteback(); }
    bool hasData() const             { return cmd.hasData(); }
    bool hasRespData() const
    {
        /* MJL_Comment
            Although it uses the MemCmd(Command _cmd) constructor, this
            should not affect the correctness of the program since resp_cmd
            is a local variable, and hasData() doesn't use MJL_cmdDir.
        */
        MemCmd resp_cmd = cmd.responseCommand();
        return resp_cmd.hasData();
    }
    bool isLLSC() const              { return cmd.isLLSC(); }
    bool isError() const             { return cmd.isError(); }
    bool isPrint() const             { return cmd.isPrint(); }
    bool isFlush() const             { return cmd.isFlush(); }

    //@{
    /// Snoop flags
    /**
     * Set the cacheResponding flag. This is used by the caches to
     * signal another cache that they are responding to a request. A
     * cache will only respond to snoops if it has the line in either
     * Modified or Owned state. Note that on snoop hits we always pass
     * the line as Modified and never Owned. In the case of an Owned
     * line we proceed to invalidate all other copies.
     *
     * On a cache fill (see Cache::handleFill), we check hasSharers
     * first, ignoring the cacheResponding flag if hasSharers is set.
     * A line is consequently allocated as:
     *
     * hasSharers cacheResponding state
     * true       false           Shared
     * true       true            Shared
     * false      false           Exclusive
     * false      true            Modified
     */
    void setCacheResponding()
    {
        assert(isRequest());
        assert(!flags.isSet(CACHE_RESPONDING));
        flags.set(CACHE_RESPONDING);
    }
    bool cacheResponding() const { return flags.isSet(CACHE_RESPONDING); }
    /**
     * On fills, the hasSharers flag is used by the caches in
     * combination with the cacheResponding flag, as clarified
     * above. If the hasSharers flag is not set, the packet is passing
     * writable. Thus, a response from a memory passes the line as
     * writable by default.
     *
     * The hasSharers flag is also used by upstream caches to inform a
     * downstream cache that they have the block (by calling
     * setHasSharers on snoop request packets that hit in upstream
     * cachs tags or MSHRs). If the snoop packet has sharers, a
     * downstream cache is prevented from passing a dirty line upwards
     * if it was not explicitly asked for a writable copy. See
     * Cache::satisfyCpuSideRequest.
     *
     * The hasSharers flag is also used on writebacks, in
     * combination with the WritbackClean or WritebackDirty commands,
     * to allocate the block downstream either as:
     *
     * command        hasSharers state
     * WritebackDirty false      Modified
     * WritebackDirty true       Owned
     * WritebackClean false      Exclusive
     * WritebackClean true       Shared
     */
    void setHasSharers()    { flags.set(HAS_SHARERS); }
    bool hasSharers() const { return flags.isSet(HAS_SHARERS); }
    //@}

    /**
     * The express snoop flag is used for two purposes. Firstly, it is
     * used to bypass flow control for normal (non-snoop) requests
     * going downstream in the memory system. In cases where a cache
     * is responding to a snoop from another cache (it had a dirty
     * line), but the line is not writable (and there are possibly
     * other copies), the express snoop flag is set by the downstream
     * cache to invalidate all other copies in zero time. Secondly,
     * the express snoop flag is also set to be able to distinguish
     * snoop packets that came from a downstream cache, rather than
     * snoop packets from neighbouring caches.
     */
    void setExpressSnoop()      { flags.set(EXPRESS_SNOOP); }
    bool isExpressSnoop() const { return flags.isSet(EXPRESS_SNOOP); }

    /**
     * On responding to a snoop request (which only happens for
     * Modified or Owned lines), make sure that we can transform an
     * Owned response to a Modified one. If this flag is not set, the
     * responding cache had the line in the Owned state, and there are
     * possibly other Shared copies in the memory system. A downstream
     * cache helps in orchestrating the invalidation of these copies
     * by sending out the appropriate express snoops.
     */
    void setResponderHadWritable()
    {
        assert(cacheResponding());
        assert(!responderHadWritable());
        flags.set(RESPONDER_HAD_WRITABLE);
    }
    bool responderHadWritable() const
    { return flags.isSet(RESPONDER_HAD_WRITABLE); }

    void setSuppressFuncError()     { flags.set(SUPPRESS_FUNC_ERROR); }
    bool suppressFuncError() const  { return flags.isSet(SUPPRESS_FUNC_ERROR); }
    void setBlockCached()          { flags.set(BLOCK_CACHED); }
    bool isBlockCached() const     { return flags.isSet(BLOCK_CACHED); }
    void clearBlockCached()        { flags.clear(BLOCK_CACHED); }

    // Network error conditions... encapsulate them as methods since
    // their encoding keeps changing (from result field to command
    // field, etc.)
    void
    setBadAddress()
    {
        assert(isResponse());
        cmd = MemCmd::BadAddressError;
    }

    void copyError(Packet *pkt) { assert(pkt->isError()); cmd = pkt->cmd; }

    Addr getAddr() const { assert(flags.isSet(VALID_ADDR)); return addr; }
    /**
     * Update the address of this packet mid-transaction. This is used
     * by the address mapper to change an already set address to a new
     * one based on the system configuration. It is intended to remap
     * an existing address, so it asserts that the current address is
     * valid.
     */
    void setAddr(Addr _addr) { assert(flags.isSet(VALID_ADDR)); addr = _addr; }

    unsigned getSize() const  { assert(flags.isSet(VALID_SIZE)); return size; }

    /* MJL_Begin */
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
    uint64_t MJL_blkMaskColumn(unsigned blkSize, unsigned MJL_rowWidth) const
    {
        uint64_t MJL_byteMask = sizeof(uint64_t) - 1;
        uint64_t MJL_wordMask = blkSize/sizeof(uint64_t) - 1;
        int MJL_colShift = floorLog2(MJL_rowWidth) + floorLog2(blkSize);
        return (MJL_wordMask << MJL_colShift) | MJL_byteMask;
    }
    // Mask out the bits that are common for both row and column
    uint64_t MJL_commonMask(unsigned blkSize, unsigned MJL_rowWidth) const 
    {
        int MJL_rowShift = floorLog2(sizeof(uint64_t));
        uint64_t MJL_wordMask = blkSize/sizeof(uint64_t) - 1;
        int MJL_colShift = floorLog2(MJL_rowWidth) + floorLog2(blkSize);

        return ((Addr)MJL_wordMask << MJL_colShift) | ((Addr)MJL_wordMask << MJL_rowShift);
    }

    Addr MJL_getColOffset(unsigned int blk_size) const
    {
        return getAddr() & Addr(blk_size - 1);
    }

    Addr MJL_getRowOffset(unsigned int blk_size) const
    {
        return MJL_swapRowColBits(getAddr(), blk_size, req->MJL_rowWidth) & Addr(blk_size - 1);
    }

    Addr MJL_getDirOffset(unsigned int blk_size, MemCmd::MJL_DirAttribute blk_dir) const
    {
        /* MJL_Begin */
        /* Deals with different access directions respectively
        */
        // MJL_TODO: looks like it's been used with blkdata in this file, so changing to data direction. Need to check whether it is true over all
        if ( blk_dir == MemCmd::MJL_DirAttribute::MJL_IsRow ) {
            return getAddr() & Addr(blk_size - 1);
        } else if ( blk_dir == MemCmd::MJL_DirAttribute::MJL_IsColumn ) {
            // MJL_temp: temporary fix for column offset calculation, maybe should use data direction? check use cases
            return MJL_swapRowColBits(getAddr(), blk_size, req->MJL_rowWidth) & Addr(blk_size - 1);
        } else {
            return getAddr() & Addr(blk_size - 1);
        }
        /* MJL_End */
        /* MJL_Comment
        return getAddr() & Addr(blk_size - 1);*/
    }
    /* MJL_End */
    Addr getOffset(unsigned int blk_size) const
    {
        /* MJL_Begin */
        /* Deals with different access directions respectively
        */
        // MJL_TODO: looks like it's been used with blkdata in this file, so changing to data direction. Need to check whether it is true over all
        if ( MJL_dataIsRow() ) {
            return getAddr() & Addr(blk_size - 1);
        } else if ( MJL_dataIsColumn() ) {
            // MJL_temp: temporary fix for column offset calculation, maybe should use data direction? check use cases
            return MJL_swapRowColBits(getAddr(), blk_size, req->MJL_rowWidth) & Addr(blk_size - 1);
        } else {
            return getAddr() & Addr(blk_size - 1);
        }
        /* MJL_End */
        /* MJL_Comment
        return getAddr() & Addr(blk_size - 1);*/
    }

    Addr getBlockAddr(unsigned int blk_size) const
    {
        /* MJL_Begin */
        /* Deals with different access directions respectively
        */
        if ( MJL_cmdIsRow() ) {
            return getAddr() & ~(Addr(blk_size - 1));
        } else if ( MJL_cmdIsColumn() ) {
            // MJL_temp: temporary fix for column block address calculation
            return getAddr() & ~(Addr(MJL_blkMaskColumn(blk_size, req->MJL_rowWidth)));
        } else {
            return getAddr() & ~(Addr(blk_size - 1));
        }
        /* MJL_End */
        /* MJL_Comment
        return getAddr() & ~(Addr(blk_size - 1));*/
    }
    /* MJL_Begin */
    Addr MJL_getCrossBlockAddr(unsigned int blk_size) const
    {
        /* MJL_Begin */
        /* Deals with different access directions respectively
        */
        if ( MJL_cmdIsColumn() ) {
            return getAddr() & ~(Addr(blk_size - 1));
        } else if ( MJL_cmdIsRow() ) {
            // MJL_temp: temporary fix for column block address calculation
            return getAddr() & ~(Addr(MJL_blkMaskColumn(blk_size, req->MJL_rowWidth)));
        } else {
            return getAddr() & ~(Addr(blk_size - 1));
        }
        /* MJL_End */
        /* MJL_Comment
        return getAddr() & ~(Addr(blk_size - 1));*/
    }

    Addr MJL_getDirBlockAddr(unsigned int blk_size, MemCmd::MJL_DirAttribute blk_dir) const
    {
        /* MJL_Begin */
        /* Deals with different access directions respectively
        */
        if ( blk_dir == MemCmd::MJL_DirAttribute::MJL_IsRow ) {
            return getAddr() & ~(Addr(blk_size - 1));
        } else if ( blk_dir == MemCmd::MJL_DirAttribute::MJL_IsColumn ) {
            // MJL_temp: temporary fix for column block address calculation
            return getAddr() & ~(Addr(MJL_blkMaskColumn(blk_size, req->MJL_rowWidth)));
        } else {
            return getAddr() & ~(Addr(blk_size - 1));
        }
        /* MJL_End */
        /* MJL_Comment
        return getAddr() & ~(Addr(blk_size - 1));*/
    }

    Addr MJL_getBlockAddrs(unsigned int blk_size, int offset) const
    {
        assert(offset < blk_size/sizeof(uint64_t));
        Addr baseAddr = (getAddr() & ~(Addr(MJL_blkMaskColumn(blk_size, req->MJL_rowWidth)))) & ~(Addr(blk_size - 1));
        if ( MJL_cmdIsColumn() ) {
            return baseAddr + offset * sizeof(uint64_t);
        } else if ( MJL_cmdIsRow() ) {
            return baseAddr + offset * req->MJL_rowWidth * blk_size;
        } else {
            return getAddr() & ~(Addr(blk_size - 1));
        };
    }
    
    Addr MJL_getCrossBlockAddrs(unsigned int blk_size, int offset) const
    {
        assert(offset < blk_size/sizeof(uint64_t));
        Addr baseAddr = (getAddr() & ~(Addr(MJL_blkMaskColumn(blk_size, req->MJL_rowWidth)))) & ~(Addr(blk_size - 1));
        if ( MJL_cmdIsRow() ) {
            return baseAddr + offset * sizeof(uint64_t);
        } else if ( MJL_cmdIsColumn() ) {
            return baseAddr + offset * req->MJL_rowWidth * blk_size;
        } else {
            return getAddr() & ~(Addr(blk_size - 1));
        };
    }
    /* MJL_End */

    bool isSecure() const
    {
        assert(flags.isSet(VALID_ADDR));
        return _isSecure;
    }

    /**
     * Accessor function to atomic op.
     */
    AtomicOpFunctor *getAtomicOp() const { return req->getAtomicOpFunctor(); }
    bool isAtomicOp() const { return req->isAtomic(); }

    /**
     * It has been determined that the SC packet should successfully update
     * memory. Therefore, convert this SC packet to a normal write.
     */
    void
    convertScToWrite()
    {
        assert(isLLSC());
        assert(isWrite());
        cmd = MemCmd::WriteReq;
    }

    /**
     * When ruby is in use, Ruby will monitor the cache line and the
     * phys memory should treat LL ops as normal reads.
     */
    void
    convertLlToRead()
    {
        assert(isLLSC());
        assert(isRead());
        cmd = MemCmd::ReadReq;
    }

    /**
     * Constructor. Note that a Request object must be constructed
     * first, but the Requests's physical address and size fields need
     * not be valid. The command must be supplied.
     */
    Packet(const RequestPtr _req, MemCmd _cmd)
        :  cmd(_cmd), req(_req), data(nullptr), addr(0),/* MJL_Begin */ MJL_dataDir(_cmd.MJL_getCmdDir()), MJL_pfPredDir(MemCmd::MJL_DirAttribute::MJL_IsInvalid),/* MJL_End*/ _isSecure(false),
           size(0), headerDelay(0), snoopDelay(0), payloadDelay(0),
           senderState(NULL)/* MJL_Begin */, MJL_hasOrder(false), MJL_order(0), MJL_wordDirty{false, false, false, false, false, false, false, false}, MJL_crossBlocksCached{false, false, false, false, false, false, false, false}, MJL_wordDemanded{false, false, false, false, false, false, false, false}, MJL_isStale{false, false, false, false, false, false, false, false}, MJL_hasSharersFlag(false)/* MJL_End */
    {
        if (req->hasPaddr()) {
            addr = req->getPaddr();
            flags.set(VALID_ADDR);
            _isSecure = req->isSecure();
        }
        if (req->hasSize()) {
            size = req->getSize();
            flags.set(VALID_SIZE);
        }
    }

    /**
     * Alternate constructor if you are trying to create a packet with
     * a request that is for a whole block, not the address from the
     * req.  this allows for overriding the size/addr of the req.
     */
    Packet(const RequestPtr _req, MemCmd _cmd, int _blkSize)
        :  cmd(_cmd), req(_req), data(nullptr), addr(0),/* MJL_Begin */ MJL_dataDir(_cmd.MJL_getCmdDir()), MJL_pfPredDir(MemCmd::MJL_DirAttribute::MJL_IsInvalid),/* MJL_End*/ _isSecure(false),
           headerDelay(0), snoopDelay(0), payloadDelay(0),
           senderState(NULL)/* MJL_Begin */, MJL_hasOrder(false), MJL_order(0), MJL_wordDirty{false, false, false, false, false, false, false, false}, MJL_crossBlocksCached{false, false, false, false, false, false, false, false}, MJL_wordDemanded{false, false, false, false, false, false, false, false}, MJL_isStale{false, false, false, false, false, false, false, false}, MJL_hasSharersFlag(false)/* MJL_End */
    {
        if (req->hasPaddr()) {
            /* MJL_Begin */
            /* Deals with different access directions respectively
            */
            if ( MJL_cmdIsRow() ) {
                addr = req->getPaddr() & ~(_blkSize - 1);
            } else if ( MJL_cmdIsColumn() ) {
                // MJL_temp: temporary fix for column block address calculation
                addr = req->getPaddr() &  ~(Addr(MJL_blkMaskColumn(req->MJL_cachelineSize, req->MJL_rowWidth)));
            } else {
                addr = req->getPaddr() & ~(_blkSize - 1);
            }
            /* MJL_End */
            /* MJL_Comment
            addr = req->getPaddr() & ~(_blkSize - 1);*/
            flags.set(VALID_ADDR);
            _isSecure = req->isSecure();
        }
        size = _blkSize;
        flags.set(VALID_SIZE);
    }

    /**
     * Alternate constructor for copying a packet.  Copy all fields
     * *except* if the original packet's data was dynamic, don't copy
     * that, as we can't guarantee that the new packet's lifetime is
     * less than that of the original packet.  In this case the new
     * packet should allocate its own data.
     */
    Packet(const PacketPtr pkt, bool clear_flags, bool alloc_data)
        :  cmd(pkt->cmd), req(pkt->req),
           data(nullptr),
           addr(pkt->addr)/* MJL_Begin */, MJL_dataDir(pkt->MJL_getDataDir()), MJL_pfPredDir(pkt->MJL_getPfPredDir())/* MJL_End*/, _isSecure(pkt->_isSecure), size(pkt->size),
           bytesValid(pkt->bytesValid),/* MJL_Begin */
           MJL_bytesValid(pkt->MJL_bytesValid),
           /* MJL_End */
           headerDelay(pkt->headerDelay),
           snoopDelay(0),
           payloadDelay(pkt->payloadDelay),
           senderState(pkt->senderState)/* MJL_Begin */, MJL_hasOrder(false), MJL_order(0), MJL_wordDirty{pkt->MJL_wordDirty[0], pkt->MJL_wordDirty[1], pkt->MJL_wordDirty[2], pkt->MJL_wordDirty[3], pkt->MJL_wordDirty[4], pkt->MJL_wordDirty[5], pkt->MJL_wordDirty[6], pkt->MJL_wordDirty[7]}, MJL_crossBlocksCached{false, false, false, false, false, false, false, false}, MJL_wordDemanded{pkt->MJL_wordDemanded[0], pkt->MJL_wordDemanded[1], pkt->MJL_wordDemanded[2], pkt->MJL_wordDemanded[3], pkt->MJL_wordDemanded[4], pkt->MJL_wordDemanded[5], pkt->MJL_wordDemanded[6], pkt->MJL_wordDemanded[7]}, MJL_isStale{pkt->MJL_isStale[0], pkt->MJL_isStale[1], pkt->MJL_isStale[2], pkt->MJL_isStale[3], pkt->MJL_isStale[4], pkt->MJL_isStale[5], pkt->MJL_isStale[6], pkt->MJL_isStale[7]}, MJL_hasSharersFlag(false)/* MJL_End */
    {
        if (!clear_flags)
            flags.set(pkt->flags & COPY_FLAGS);
        /* MJL_Begin */
        if (!clear_flags) {
            MJL_copyCrossBlocksCached(pkt->MJL_crossBlocksCached);
            if (pkt->MJL_hasSharersFlag) {
            	MJL_setHasSharers();
            }
        }
        /* MJL_End */

        flags.set(pkt->flags & (VALID_ADDR|VALID_SIZE));

        // should we allocate space for data, or not, the express
        // snoops do not need to carry any data as they only serve to
        // co-ordinate state changes
        if (alloc_data) {
            // even if asked to allocate data, if the original packet
            // holds static data, then the sender will not be doing
            // any memcpy on receiving the response, thus we simply
            // carry the pointer forward
            if (pkt->flags.isSet(STATIC_DATA)) {
                data = pkt->data;
                flags.set(STATIC_DATA);
            } else {
                allocate();
            }
        }
    }

    /**
     * Generate the appropriate read MemCmd based on the Request flags.
     */
    static MemCmd
    makeReadCmd(const RequestPtr req)
    {
        /* MJL_TODO: not sure whether these lines would execute normally... */
        if (req->isLLSC())
            return MemCmd::LoadLockedReq;
        else if (req->isPrefetch())
            return MemCmd::SoftPFReq;
        else
            return MemCmd::ReadReq;
    }

    /**
     * Generate the appropriate write MemCmd based on the Request flags.
     */
    static MemCmd
    makeWriteCmd(const RequestPtr req)
    {
        /* MJL_TODO: not sure whether these lines would execute normally... */
        if (req->isLLSC())
            return MemCmd::StoreCondReq;
        else if (req->isSwap())
            return MemCmd::SwapReq;
        else
            return MemCmd::WriteReq;
    }

    /**
     * Constructor-like methods that return Packets based on Request objects.
     * Fine-tune the MemCmd type if it's not a vanilla read or write.
     */
    static PacketPtr
    createRead(const RequestPtr req)
    {
        return new Packet(req, makeReadCmd(req));
    }

    static PacketPtr
    createWrite(const RequestPtr req)
    {
        return new Packet(req, makeWriteCmd(req));
    }

    /**
     * clean up packet variables
     */
    ~Packet()
    {
        // Delete the request object if this is a request packet which
        // does not need a response, because the requester will not get
        // a chance. If the request packet needs a response then the
        // request will be deleted on receipt of the response
        // packet. We also make sure to never delete the request for
        // express snoops, even for cases when responses are not
        // needed (CleanEvict and Writeback), since the snoop packet
        // re-uses the same request.
        if (req && isRequest() && !needsResponse() &&
            !isExpressSnoop()) {
            delete req;
        }
        deleteData();
    }

    /**
     * Take a request packet and modify it in place to be suitable for
     * returning as a response to that request.
     */
    void
    makeResponse()
    {
        assert(needsResponse());
        assert(isRequest());
        cmd = cmd.responseCommand();

        // responses are never express, even if the snoop that
        // triggered them was
        flags.clear(EXPRESS_SNOOP);
    }

    void
    makeAtomicResponse()
    {
        makeResponse();
    }

    void
    makeTimingResponse()
    {
        makeResponse();
    }

    void
    setFunctionalResponseStatus(bool success)
    {
        if (!success) {
            if (isWrite()) {
                cmd = MemCmd::FunctionalWriteError;
            } else {
                cmd = MemCmd::FunctionalReadError;
            }
        }
    }

    void
    setSize(unsigned size)
    {
        assert(!flags.isSet(VALID_SIZE));

        this->size = size;
        flags.set(VALID_SIZE);
    }
    /* MJL_Begin */
    void
    MJL_setSize(unsigned size)
    {
        assert(flags.isSet(VALID_SIZE));

        this->size = size;
    }
    /* MJL_End */


  public:
    /**
     * @{
     * @name Data accessor mehtods
     */

    /**
     * Set the data pointer to the following value that should not be
     * freed. Static data allows us to do a single memcpy even if
     * multiple packets are required to get from source to destination
     * and back. In essence the pointer is set calling dataStatic on
     * the original packet, and whenever this packet is copied and
     * forwarded the same pointer is passed on. When a packet
     * eventually reaches the destination holding the data, it is
     * copied once into the location originally set. On the way back
     * to the source, no copies are necessary.
     */
    template <typename T>
    void
    dataStatic(T *p)
    {
        assert(flags.noneSet(STATIC_DATA|DYNAMIC_DATA));
        data = (PacketDataPtr)p;
        flags.set(STATIC_DATA);
    }

    /**
     * Set the data pointer to the following value that should not be
     * freed. This version of the function allows the pointer passed
     * to us to be const. To avoid issues down the line we cast the
     * constness away, the alternative would be to keep both a const
     * and non-const data pointer and cleverly choose between
     * them. Note that this is only allowed for static data.
     */
    template <typename T>
    void
    dataStaticConst(const T *p)
    {
        assert(flags.noneSet(STATIC_DATA|DYNAMIC_DATA));
        data = const_cast<PacketDataPtr>(p);
        flags.set(STATIC_DATA);
    }

    /**
     * Set the data pointer to a value that should have delete []
     * called on it. Dynamic data is local to this packet, and as the
     * packet travels from source to destination, forwarded packets
     * will allocate their own data. When a packet reaches the final
     * destination it will populate the dynamic data of that specific
     * packet, and on the way back towards the source, memcpy will be
     * invoked in every step where a new packet was created e.g. in
     * the caches. Ultimately when the response reaches the source a
     * final memcpy is needed to extract the data from the packet
     * before it is deallocated.
     */
    template <typename T>
    void
    dataDynamic(T *p)
    {
        assert(flags.noneSet(STATIC_DATA|DYNAMIC_DATA));
        data = (PacketDataPtr)p;
        flags.set(DYNAMIC_DATA);
    }

    /**
     * get a pointer to the data ptr.
     */
    template <typename T>
    T*
    getPtr()
    {
        assert(flags.isSet(STATIC_DATA|DYNAMIC_DATA));
        return (T*)data;
    }

    template <typename T>
    const T*
    getConstPtr() const
    {
        assert(flags.isSet(STATIC_DATA|DYNAMIC_DATA));
        return (const T*)data;
    }

    /**
     * Get the data in the packet byte swapped from big endian to
     * host endian.
     */
    template <typename T>
    T getBE() const;

    /**
     * Get the data in the packet byte swapped from little endian to
     * host endian.
     */
    template <typename T>
    T getLE() const;

    /**
     * Get the data in the packet byte swapped from the specified
     * endianness.
     */
    template <typename T>
    T get(ByteOrder endian) const;

    /**
     * Get the data in the packet byte swapped from guest to host
     * endian.
     */
    template <typename T>
    T get() const;

    /** Set the value in the data pointer to v as big endian. */
    template <typename T>
    void setBE(T v);

    /** Set the value in the data pointer to v as little endian. */
    template <typename T>
    void setLE(T v);

    /**
     * Set the value in the data pointer to v using the specified
     * endianness.
     */
    template <typename T>
    void set(T v, ByteOrder endian);

    /** Set the value in the data pointer to v as guest endian. */
    template <typename T>
    void set(T v);

    /**
     * Copy data into the packet from the provided pointer.
     */
    void
    setData(const uint8_t *p)
    {
        // we should never be copying data onto itself, which means we
        // must idenfity packets with static data, as they carry the
        // same pointer from source to destination and back
        assert(p != getPtr<uint8_t>() || flags.isSet(STATIC_DATA));

        if (p != getPtr<uint8_t>())
            // for packet with allocated dynamic data, we copy data from
            // one to the other, e.g. a forwarded response to a response
            std::memcpy(getPtr<uint8_t>(), p, getSize());
    }

    /**
     * Copy data into the packet from the provided block pointer,
     * which is aligned to the given block size.
     */
    void
    setDataFromBlock(const uint8_t *blk_data, int blkSize)
    {
        /* MJL_Begin */
        /* MJL_Test: read from cache information output 
	    if (req->hasPC() && !req->isInstFetch()) {
            std::cout << "setDataFromBlock";
            std::cout << ": PC(hex) = " << std::hex << req->getPC() << std::dec;
            std::cout << ", Addr(oct) = " << std::oct << getAddr() << std::dec;
            std::cout << ", CmdDir = " << MJL_getCmdDir();
            std::cout << ", SameCmdDataDir = ";
            if (MJL_sameCmdDataDir()) {
                std::cout << "1";
            } else {
                std::cout << "0";
            }
            std::cout << ", Size = " << getSize();
            
            setData(blk_data + getOffset(blkSize));
            
            std::cout << ", Data(hex) = " << std::hex;
            uint64_t MJL_data = 0;
            std::memcpy(&MJL_data, getConstPtr<uint8_t>(), std::min(sizeof(uint64_t), (Addr)getSize()));
            std::cout << "word[0] " <<  MJL_data;
            for (unsigned i = sizeof(uint64_t); i < getSize(); i = i + sizeof(uint64_t)) {
                MJL_data = 0;
                std::memcpy(&MJL_data, getConstPtr<uint8_t>() + i, std::min(sizeof(uint64_t), getSize() - (Addr)i));
                std::cout << " | word[" << i/sizeof(uint64_t) << "] " <<  MJL_data;
            }
            std::cout << std::dec << std::endl;
        } else
         */
        /* MJL_End */
        setData(blk_data + getOffset(blkSize));
    }

    /**
     * Copy data from the packet to the provided block pointer, which
     * is aligned to the given block size.
     */
    void
    writeData(uint8_t *p) const
    {
        std::memcpy(p, getConstPtr<uint8_t>(), getSize());
    }

    /**
     * Copy data from the packet to the memory at the provided pointer.
     */
    void
    writeDataToBlock(uint8_t *blk_data, int blkSize) const
    {
        /* MJL_Begin */
        /* MJL_Test: write to cache information output 
        if (req->hasPC() && !req->isInstFetch()) {
            std::cout << "writeDataToBlock:";
            std::cout << ": PC(hex) = " << std::hex << req->getPC() << std::dec;
            std::cout << ", Addr(oct) = " << std::oct << getAddr() << std::dec;
            std::cout << ", CmdDir = " << MJL_getCmdDir();
            std::cout << ", SameCmdDataDir = ";
            if (MJL_sameCmdDataDir()) {
                std::cout << "1";
            } else {
                std::cout << "0";
            }
            std::cout << ", Size = " << getSize();
            
            writeData(blk_data + getOffset(blkSize));
            
            std::cout << ", Data(hex) = " << std::hex;
            uint64_t MJL_data = 0;
            std::memcpy(&MJL_data, getConstPtr<uint8_t>(), std::min(sizeof(uint64_t), (Addr)getSize()));
            std::cout << "word[0] " <<  MJL_data;
            for (unsigned i = sizeof(uint64_t); i < getSize(); i = i + sizeof(uint64_t)) {
                MJL_data = 0;
                std::memcpy(&MJL_data, getConstPtr<uint8_t>() + i, std::min(sizeof(uint64_t), getSize() - (Addr)i));
                std::cout << " | word[" << i/sizeof(uint64_t) << "] = " <<  MJL_data;
            }
            std::cout << std::dec << std::endl;
        } else
         */
        /* MJL_End */
        writeData(blk_data + getOffset(blkSize));
    }

    /**
     * delete the data pointed to in the data pointer. Ok to call to
     * matter how data was allocted.
     */
    void
    deleteData()
    {
        if (flags.isSet(DYNAMIC_DATA))
            delete [] data;

        flags.clear(STATIC_DATA|DYNAMIC_DATA);
        data = NULL;
    }

    /** Allocate memory for the packet. */
    void
    allocate()
    {
        // if either this command or the response command has a data
        // payload, actually allocate space
        if (hasData() || hasRespData()) {
            assert(flags.noneSet(STATIC_DATA|DYNAMIC_DATA));
            flags.set(DYNAMIC_DATA);
            data = new uint8_t[getSize()];
        }
    }

    /** @} */

  private: // Private data accessor methods
    /** Get the data in the packet without byte swapping. */
    template <typename T>
    T getRaw() const;

    /** Set the value in the data pointer to v without byte swapping. */
    template <typename T>
    void setRaw(T v);

  public:
    /**
     * Check a functional request against a memory value stored in
     * another packet (i.e. an in-transit request or
     * response). Returns true if the current packet is a read, and
     * the other packet provides the data, which is then copied to the
     * current packet. If the current packet is a write, and the other
     * packet intersects this one, then we update the data
     * accordingly.
     */
    bool
    checkFunctional(PacketPtr other)
    {
        // all packets that are carrying a payload should have a valid
        // data pointer
        return checkFunctional(other, other->getAddr(), other->isSecure(),
                               other->getSize(),
                               other->hasData() ?
                               other->getPtr<uint8_t>() : NULL);
    }

    /* MJL_Begin */
    bool
    MJL_checkFunctional(PacketPtr other)
    {
        // all packets that are carrying a payload should have a valid
        // data pointer
        return MJL_checkFunctional(other, other->getAddr(), other->MJL_getDataDir(), other->isSecure(),
                               other->getSize(),
                               other->hasData() ?
                               other->getPtr<uint8_t>() : NULL);
    }
    /* MJL_End */

    /**
     * Does the request need to check for cached copies of the same block
     * in the memory hierarchy above.
     **/
    bool
    mustCheckAbove() const
    {
        return cmd == MemCmd::HardPFReq || isEviction();
    }

    /**
     * Is this packet a clean eviction, including both actual clean
     * evict packets, but also clean writebacks.
     */
    bool
    isCleanEviction() const
    {
        return cmd == MemCmd::CleanEvict || cmd == MemCmd::WritebackClean;
    }

    /**
     * Check a functional request against a memory value represented
     * by a base/size pair and an associated data array. If the
     * current packet is a read, it may be satisfied by the memory
     * value. If the current packet is a write, it may update the
     * memory value.
     */
    bool
    checkFunctional(Printable *obj, Addr base, bool is_secure, int size,
                    uint8_t *_data);

    /* MJL_Begin */
    bool
    MJL_checkFunctional(Printable *obj, Addr addr, MemCmd::MJL_DirAttribute MJL_cmdDir, bool is_secure, int size,
                        uint8_t *_data);
    /**
     * Used in the cross directional check to record if bytes are in have dirty status
     * Return true when all bytes are in have dirty status
     */
    bool MJL_setHaveDirty(Addr addr, MemCmd::MJL_DirAttribute MJL_cmdDir, bool is_secure, int size,
                        uint8_t *_data);
    /* MJL_End */

    /**
     * Push label for PrintReq (safe to call unconditionally).
     */
    void
    pushLabel(const std::string &lbl)
    {
        if (isPrint())
            safe_cast<PrintReqState*>(senderState)->pushLabel(lbl);
    }

    /**
     * Pop label for PrintReq (safe to call unconditionally).
     */
    void
    popLabel()
    {
        if (isPrint())
            safe_cast<PrintReqState*>(senderState)->popLabel();
    }

    void print(std::ostream &o, int verbosity = 0,
               const std::string &prefix = "") const;

    /**
     * A no-args wrapper of print(std::ostream...)
     * meant to be invoked from DPRINTFs
     * avoiding string overheads in fast mode
     * @return string with the request's type and start<->end addresses
     */
    std::string print() const;
};

#endif //__MEM_PACKET_HH
