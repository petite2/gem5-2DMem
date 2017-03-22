#include <bitset>
#include <string>
#include <cstdint>
#include <iostream>
#include <cassert>

/* MJL_Comment
    Test for the addtions of MJL_cmdDir, MJL_isRead, etc. in packet.hh
*/

using namespace std;

class Packet;
typedef Packet *PacketPtr;

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
    enum MJL_DirAttribute
    {
        // MJL_TODO: Check whether adding this would break things
        MJL_IsInvalid,  //!< Data access direction is invalid
        MJL_IsRow,      //!< Data access direction is row
        MJL_IsColumn,   //!< Data access direction is column
        MJL_NUM_COMMAND_DIRATTRIBUTES
    };
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
        const bitset<NUM_COMMAND_ATTRIBUTES> attributes;
        /// Corresponding response for requests; InvalidCmd if no
        /// response is applicable.
        const Command response;
        /// String representation (for printing)
        const string str;
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
    bool MJL_isRow() const         { return (MJL_cmdDir == MJL_IsRow); }
    bool MJL_isColumn() const      { return (MJL_cmdDir == MJL_IsColumn); }
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
    const string &toString() const { return commandInfo[cmd].str; }
    int toInt() const { return (int)cmd; }

    /* MJL_Begin */
    /**
     * Set the command's data access direction
     */
    void MJL_setCmdDir(MJL_DirAttribute in_MJL_cmdDir) { MJL_cmdDir = in_MJL_cmdDir; }

    /** 
     * Overload the operator=(Command _cmd) and (int _cmd) so that
     * the MemCmd(Command _cmd) and MemCmd(int _cmd) methods doesn't 
     * get called when cmd = MemCmd::xxx is used and resets the
     * MJL_cmdDir to default.
     */
    // MJL_TODO: Check whether this breaks other things
    MemCmd operator=(Command _cmd) { this->cmd = _cmd; return *this; }
    MemCmd operator=(int _cmd) { this->cmd = (Command)_cmd; return *this; }
    /* MJL_End */
    MemCmd(Command _cmd) : cmd(_cmd)/* MJL_Begin */, MJL_cmdDir(MJL_IsRow) /* MJL_End */ { }
    MemCmd(int _cmd) : cmd((Command)_cmd)/* MJL_Begin */, MJL_cmdDir(MJL_IsRow) /* MJL_End */ { }
    MemCmd() : cmd(InvalidCmd)/* MJL_Begin */, MJL_cmdDir(MJL_IsRow) /* MJL_End */ { }

    /* MJL_Comment
        MJL_TODO: Check if changing the operators breaks anything
    */
    bool operator==(MemCmd c2) const { return (cmd == c2.cmd)/* MJL_Begin */ && (MJL_cmdDir == c2.MJL_cmdDir)/* MJL_End */; }
    bool operator!=(MemCmd c2) const { return (cmd != c2.cmd)/* MJL_Begin */ || (MJL_cmdDir != c2.MJL_cmdDir)/* MJL_End */; }

    /* MJL_Comment
        For test purpose
    */
    void print() { cout << "cmd = " << toString() << ", MJL_cmdDir = " << MJL_cmdDir ; }
};


// The one downside to bitsets is that static initializers can get ugly.
#define SET1(a1)                     (1 << (a1))
#define SET2(a1, a2)                 (SET1(a1) | SET1(a2))
#define SET3(a1, a2, a3)             (SET2(a1, a2) | SET1(a3))
#define SET4(a1, a2, a3, a4)         (SET3(a1, a2, a3) | SET1(a4))
#define SET5(a1, a2, a3, a4, a5)     (SET4(a1, a2, a3, a4) | SET1(a5))
#define SET6(a1, a2, a3, a4, a5, a6) (SET5(a1, a2, a3, a4, a5) | SET1(a6))
#define SET7(a1, a2, a3, a4, a5, a6, a7) (SET6(a1, a2, a3, a4, a5, a6) | \
                                          SET1(a7))

const MemCmd::CommandInfo
MemCmd::commandInfo[] =
{
    /* InvalidCmd */
    { 0, InvalidCmd, "InvalidCmd" },
    /* ReadReq - Read issued by a non-caching agent such as a CPU or
     * device, with no restrictions on alignment. */
    { SET3(IsRead, IsRequest, NeedsResponse), ReadResp, "ReadReq" },
    /* ReadResp */
    { SET3(IsRead, IsResponse, HasData), InvalidCmd, "ReadResp" },
    /* ReadRespWithInvalidate */
    { SET4(IsRead, IsResponse, HasData, IsInvalidate),
            InvalidCmd, "ReadRespWithInvalidate" },
    /* WriteReq */
    { SET5(IsWrite, NeedsWritable, IsRequest, NeedsResponse, HasData),
            WriteResp, "WriteReq" },
    /* WriteResp */
    { SET2(IsWrite, IsResponse), InvalidCmd, "WriteResp" },
    /* WritebackDirty */
    { SET5(IsWrite, IsRequest, IsEviction, HasData, FromCache),
            InvalidCmd, "WritebackDirty" },
    /* WritebackClean - This allows the upstream cache to writeback a
     * line to the downstream cache without it being considered
     * dirty. */
    { SET5(IsWrite, IsRequest, IsEviction, HasData, FromCache),
            InvalidCmd, "WritebackClean" },
    /* CleanEvict */
    { SET3(IsRequest, IsEviction, FromCache), InvalidCmd, "CleanEvict" },
    /* SoftPFReq */
    { SET4(IsRead, IsRequest, IsSWPrefetch, NeedsResponse),
            SoftPFResp, "SoftPFReq" },
    /* HardPFReq */
    { SET5(IsRead, IsRequest, IsHWPrefetch, NeedsResponse, FromCache),
            HardPFResp, "HardPFReq" },
    /* SoftPFResp */
    { SET4(IsRead, IsResponse, IsSWPrefetch, HasData),
            InvalidCmd, "SoftPFResp" },
    /* HardPFResp */
    { SET4(IsRead, IsResponse, IsHWPrefetch, HasData),
            InvalidCmd, "HardPFResp" },
    /* WriteLineReq */
    { SET5(IsWrite, NeedsWritable, IsRequest, NeedsResponse, HasData),
            WriteResp, "WriteLineReq" },
    /* UpgradeReq */
    { SET6(IsInvalidate, NeedsWritable, IsUpgrade, IsRequest, NeedsResponse,
            FromCache),
            UpgradeResp, "UpgradeReq" },
    /* SCUpgradeReq: response could be UpgradeResp or UpgradeFailResp */
    { SET7(IsInvalidate, NeedsWritable, IsUpgrade, IsLlsc,
           IsRequest, NeedsResponse, FromCache),
            UpgradeResp, "SCUpgradeReq" },
    /* UpgradeResp */
    { SET2(IsUpgrade, IsResponse),
            InvalidCmd, "UpgradeResp" },
    /* SCUpgradeFailReq: generates UpgradeFailResp but still gets the data */
    { SET7(IsRead, NeedsWritable, IsInvalidate,
           IsLlsc, IsRequest, NeedsResponse, FromCache),
            UpgradeFailResp, "SCUpgradeFailReq" },
    /* UpgradeFailResp - Behaves like a ReadExReq, but notifies an SC
     * that it has failed, acquires line as Dirty*/
    { SET3(IsRead, IsResponse, HasData),
            InvalidCmd, "UpgradeFailResp" },
    /* ReadExReq - Read issues by a cache, always cache-line aligned,
     * and the response is guaranteed to be writeable (exclusive or
     * even modified) */
    { SET6(IsRead, NeedsWritable, IsInvalidate, IsRequest, NeedsResponse,
            FromCache),
            ReadExResp, "ReadExReq" },
    /* ReadExResp - Response matching a read exclusive, as we check
     * the need for exclusive also on responses */
    { SET3(IsRead, IsResponse, HasData),
            InvalidCmd, "ReadExResp" },
    /* ReadCleanReq - Read issued by a cache, always cache-line
     * aligned, and the response is guaranteed to not contain dirty data
     * (exclusive or shared).*/
    { SET4(IsRead, IsRequest, NeedsResponse, FromCache),
            ReadResp, "ReadCleanReq" },
    /* ReadSharedReq - Read issued by a cache, always cache-line
     * aligned, response is shared, possibly exclusive, owned or even
     * modified. */
    { SET4(IsRead, IsRequest, NeedsResponse, FromCache),
            ReadResp, "ReadSharedReq" },
    /* LoadLockedReq: note that we use plain ReadResp as response, so that
     *                we can also use ReadRespWithInvalidate when needed */
    { SET4(IsRead, IsLlsc, IsRequest, NeedsResponse),
            ReadResp, "LoadLockedReq" },
    /* StoreCondReq */
    { SET6(IsWrite, NeedsWritable, IsLlsc,
           IsRequest, NeedsResponse, HasData),
            StoreCondResp, "StoreCondReq" },
    /* StoreCondFailReq: generates failing StoreCondResp */
    { SET6(IsWrite, NeedsWritable, IsLlsc,
           IsRequest, NeedsResponse, HasData),
            StoreCondResp, "StoreCondFailReq" },
    /* StoreCondResp */
    { SET3(IsWrite, IsLlsc, IsResponse),
            InvalidCmd, "StoreCondResp" },
    /* SwapReq -- for Swap ldstub type operations */
    { SET6(IsRead, IsWrite, NeedsWritable, IsRequest, HasData, NeedsResponse),
        SwapResp, "SwapReq" },
    /* SwapResp -- for Swap ldstub type operations */
    { SET4(IsRead, IsWrite, IsResponse, HasData),
            InvalidCmd, "SwapResp" },
    /* IntReq -- for interrupts */
    { SET4(IsWrite, IsRequest, NeedsResponse, HasData),
        MessageResp, "MessageReq" },
    /* IntResp -- for interrupts */
    { SET2(IsWrite, IsResponse), InvalidCmd, "MessageResp" },
    /* MemFenceReq -- for synchronization requests */
    {SET2(IsRequest, NeedsResponse), MemFenceResp, "MemFenceReq"},
    /* MemFenceResp -- for synchronization responses */
    {SET1(IsResponse), InvalidCmd, "MemFenceResp"},
    /* InvalidDestError  -- packet dest field invalid */
    { SET2(IsResponse, IsError), InvalidCmd, "InvalidDestError" },
    /* BadAddressError   -- memory address invalid */
    { SET2(IsResponse, IsError), InvalidCmd, "BadAddressError" },
    /* FunctionalReadError */
    { SET3(IsRead, IsResponse, IsError), InvalidCmd, "FunctionalReadError" },
    /* FunctionalWriteError */
    { SET3(IsWrite, IsResponse, IsError), InvalidCmd, "FunctionalWriteError" },
    /* PrintReq */
    { SET2(IsRequest, IsPrint), InvalidCmd, "PrintReq" },
    /* Flush Request */
    { SET3(IsRequest, IsFlush, NeedsWritable), InvalidCmd, "FlushReq" },
    /* Invalidation Request */
    { SET5(IsInvalidate, IsRequest, NeedsWritable, NeedsResponse, FromCache),
      InvalidateResp, "InvalidateReq" },
    /* Invalidation Response */
    { SET2(IsInvalidate, IsResponse),
      InvalidCmd, "InvalidateResp" }
};

class Packet
{
  public:
    typedef MemCmd::Command Command;
    typedef uint64_t Addr;

    /// The command field of the packet.
    MemCmd cmd;

  private:

    /// The address of the request.  This address could be virtual or
    /// physical, depending on the system configuration.
    Addr addr;

  public:

    bool isRead() const              { return cmd.isRead(); }
    bool isWrite() const             { return cmd.isWrite(); }
    bool MJL_isRow() const           { return cmd.MJL_isRow(); }
    bool MJL_isColumn() const        { return cmd.MJL_isColumn(); }
    bool isResponse() const        { return cmd.isResponse(); }
    bool isEviction() const          { return cmd.isEviction(); }
    bool hasRespData() const
    {
        MemCmd resp_cmd = cmd.responseCommand();
        return resp_cmd.hasData();
    }
    
    void
    setBadAddress()
    {
        assert(isResponse());
        /* MJL_TODO: not sure whether this line would execute normally... */
        cmd = MemCmd::BadAddressError;
    }

    void copyError(Packet *pkt) { cmd = pkt->cmd; }

    Addr getAddr() const { return addr; }
    /**
     * Update the address of this packet mid-transaction. This is used
     * by the address mapper to change an already set address to a new
     * one based on the system configuration. It is intended to remap
     * an existing address, so it asserts that the current address is
     * valid.
     */
    void setAddr(Addr _addr) { addr = _addr; }

    Addr getOffset() const
    {
        /* MJL_Begin */
        /* Deals with different access directions respectively
        */
        if ( MJL_isRow() ) {
            return getAddr() & Addr(16 - 1);
        }
        else if ( MJL_isColumn() ) {
            // MJL_TODO: Placeholder for column offset calculation
            return getAddr() & Addr(16 - 1);
        }
        else {
            return getAddr() & Addr(16 - 1);
        }
        /* MJL_End */
        /* MJL_Comment
        return getAddr() & Addr(blk_size - 1);*/
    }

    Addr getBlockAddr() const
    {
        /* MJL_Begin */
        /* Deals with different access directions respectively
        */
        if ( MJL_isRow() ) {
            return getAddr() & ~(Addr(16 - 1));
        }
        else if ( MJL_isColumn() ) {
            // MJL_TODO: Placeholder for column offset calculation
            return getAddr() & ~(Addr(16 - 1));
        }
        else {
            return getAddr() & ~(Addr(16 - 1));
        }
        /* MJL_End */
        /* MJL_Comment
        return getAddr() & ~(Addr(blk_size - 1));*/
    }

    void
    convertScToWrite()
    {
        /* MJL_TODO: not sure whether this line would execute normally... */
        cmd = MemCmd::WriteReq;
    }

    void
    convertLlToRead()
    {
        /* MJL_TODO: not sure whether this line would execute normally... */
        cmd = MemCmd::ReadReq;
    }

   

    /**
     * Alternate constructor if you are trying to create a packet with
     * a request that is for a whole block, not the address from the
     * req.  this allows for overriding the size/addr of the req.
     */
    Packet(MemCmd _cmd)
        :  cmd(_cmd), addr(0)
    {
            /* MJL_Begin */
            /* Deals with different access directions respectively
            */
            if ( MJL_isRow() ) {
                addr = addr & ~(16 - 1);
            }
            else if ( MJL_isColumn() ) {
                // MJL_TODO: Placeholder for column offset calculation
                addr = addr & ~(16 - 1);
            }
            else {
                addr = addr & ~(16 - 1);
            }
    }

    /**
     * Alternate constructor for copying a packet.  Copy all fields
     * *except* if the original packet's data was dynamic, don't copy
     * that, as we can't guarantee that the new packet's lifetime is
     * less than that of the original packet.  In this case the new
     * packet should allocate its own data.
     */
    Packet(const PacketPtr pkt)
        :  cmd(pkt->cmd), addr(pkt->addr)
    {
        
    }

    /**
     * Generate the appropriate read MemCmd based on the Request flags.
     */
    static MemCmd
    makeReadCmd()
    {
        /* MJL_TODO: not sure whether these lines would execute normally... */
        return MemCmd::ReadReq;
    }

    /**
     * Generate the appropriate write MemCmd based on the Request flags.
     */
    static MemCmd
    makeWriteCmd()
    {
        /* MJL_TODO: not sure whether these lines would execute normally... */
        return MemCmd::WriteReq;
    }

    /**
     * Constructor-like methods that return Packets based on Request objects.
     * Fine-tune the MemCmd type if it's not a vanilla read or write.
     */
    static PacketPtr
    createRead()
    {
        return new Packet(makeReadCmd());
    }

    static PacketPtr
    createWrite()
    {
        return new Packet(makeWriteCmd());
    }

    /**
     * clean up packet variables
     */
    ~Packet()
    {
    }

    /**
     * Take a request packet and modify it in place to be suitable for
     * returning as a response to that request.
     */
    void
    makeResponse()
    {
        cmd = cmd.responseCommand();
    }

    void
    setFunctionalResponseStatus()
    {
        /* MJL_TODO: not sure whether this line would execute normally... */
        if (isWrite()) {
            cmd = MemCmd::FunctionalWriteError;
        } else {
            cmd = MemCmd::FunctionalReadError;
        }
    }
  
 
    
    bool
    mustCheckAbove() const
    {
        /* MJL_TODO: not sure whether this line would execute normally... */
        return cmd == MemCmd::HardPFReq || isEviction();
    }

    /**
     * Is this packet a clean eviction, including both actual clean
     * evict packets, but also clean writebacks.
     */
    bool
    isCleanEviction() const
    {
        /* MJL_TODO: not sure whether this line would execute normally... */
        return cmd == MemCmd::CleanEvict || cmd == MemCmd::WritebackClean;
    }

    /* MJL_Comment 
        For test purpose
    */
    void print() { cout << "PktCmd : "; cmd.print(); cout << "; Addr = " << addr << "\n";}
};

int main()
{
    MemCmd test_cmd;
    Packet test_packet(test_cmd);
    test_packet.print();
    // MJL_Comment: Need to add more test cases in case of problem
    return 0;
}