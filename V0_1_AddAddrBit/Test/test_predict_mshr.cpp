#include <list>
#include <map>
#include <iostream>       // std::cout
#include <sstream>        // std::ostringstream
#include <cinttypes>
#include <cassert>

typedef uint64_t Addr;
typedef uint16_t MasterID;

unsigned blkSize = 64;

unsigned MJL_rowWidth = 4;
bool MJL_mshrPredictDir = true;

int maxConf = 7;
int threshConf = 3;
int minConf = 0;
int startConf = 4;

int pcTableAssoc = 4;
int pcTableSets = 16;

bool useMasterId = true;

class MemCmd
{
    public:
        enum MJL_DirAttribute
        {
            MJL_IsInvalid,  //!< Data access direction is invalid
            MJL_IsRow,      //!< Data access direction is row
            MJL_IsColumn,   //!< Data access direction is column
            MJL_NUM_COMMAND_DIRATTRIBUTES
        };
};

class Packet
{
    public:
    class Request {
        public:
        Addr pc;
        MasterID master_id;

        Request(Addr _pc, MasterID _master_id) : pc(_pc), master_id(_master_id) {}
        Addr getPC() {return pc;}
        MasterID masterId() {return master_id;}
        bool hasPC() {return true;}
    };

    Addr addr;
    Addr blkAddr;
    Addr crossBlkAddr;
    MemCmd::MJL_DirAttribute cmdDir;
    bool is_secure;
    Request* req;
    Addr blkOffset;
    Addr crossBlkOffset;

    Packet(Addr _addr, Addr _blkAddr, Addr _crossBlkAddr, MemCmd::MJL_DirAttribute _cmdDir, bool _is_secure, Addr _pc, MasterID _master_id, Addr _blkOffset, Addr _crossBlkOffset) : addr(_addr), blkAddr(_blkAddr), crossBlkAddr(_crossBlkAddr), cmdDir(_cmdDir), is_secure(_is_secure), blkOffset(_blkOffset), crossBlkOffset(_crossBlkOffset) {req = new Request(_pc, _master_id);}
    Addr getAddr() {return addr;}
    Addr getBlockAddr(unsigned blkSize) {return blkAddr;}
    Addr MJL_getCrossBlockAddr(unsigned blkSize) {return crossBlkAddr;}
    bool isSecure() {return is_secure;}
    Addr MJL_getDirBlockAddr(unsigned blkSize, MemCmd::MJL_DirAttribute blkDir) {
        if (blkDir == cmdDir) {
            return getBlockAddr(blkSize);
        } else {
            return MJL_getCrossBlockAddr(blkSize);
        }
    }
    MemCmd::MJL_DirAttribute MJL_getCmdDir() {return cmdDir;}
    MemCmd::MJL_DirAttribute MJL_getCrossCmdDir() {
        if (cmdDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
            return MemCmd::MJL_DirAttribute::MJL_IsColumn;
        } else {
            return MemCmd::MJL_DirAttribute::MJL_IsRow;
        }
    }
    Addr getOffset(unsigned blkSize) {return blkOffset;}
    Addr MJL_getDirOffset(unsigned blkSize, MemCmd::MJL_DirAttribute blkDir) {
        if (blkDir == cmdDir) {
            return getOffset(blkSize);
        } else {
            return crossBlkOffset;
        }
    }
    unsigned getSize() {
        return blkSize/sizeof(uint64_t);
    }
};
typedef Packet* PacketPtr;

class MSHR
{
    public:
    Addr blkAddr;
    MemCmd::MJL_DirAttribute MJL_qEntryDir;
    bool isSecure;

    MSHR(const PacketPtr pkt, unsigned blkSize) : blkAddr(pkt->getBlockAddr(blkSize)), MJL_qEntryDir(pkt->MJL_getCmdDir()), isSecure(pkt->isSecure()) {}
};

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
    
    void reset_entry() {
        instAddr = 0; 
        lastAddr = 0; 
        isSecure = false;
        stride = 0;
        confidence = 0;
        lastPredDir = MemCmd::MJL_DirAttribute::MJL_IsRow;
        for (unsigned i = 0; i < 8; ++i) {
            blkHits[i] = false;
            crossBlkHits[i] = false;
        }
        lastRowOff = 0;
        lastColOff = 0;
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
        // chatty_assert(res.second, "Allocating an already created context\n");
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
PCTable pcTable(pcTableAssoc, pcTableSets);

struct PredictMshrEntry
{
    PredictMshrEntry(const PacketPtr pkt, const MSHR* in_mshr) : pc(pkt->req->getPC()), blkAddr(pkt->getBlockAddr(blkSize)), crossBlkAddr(pkt->MJL_getCrossBlockAddr(blkSize)), blkDir(pkt->MJL_getCmdDir()), crossBlkDir(pkt->MJL_getCrossCmdDir()), isSecure(pkt->isSecure()), accessAddr(pkt->getAddr()), mshr(in_mshr), masterId(pkt->req->masterId()), blkHits{false, false, false, false, false, false, false, false}, crossBlkHits{false, false, false, false, false, false, false, false}, lastRowOff(pkt->MJL_getDirOffset(blkSize, MemCmd::MJL_DirAttribute::MJL_IsRow)/sizeof(uint64_t)), lastColOff(pkt->MJL_getDirOffset(blkSize, MemCmd::MJL_DirAttribute::MJL_IsColumn)/sizeof(uint64_t))
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
};
std::list<PredictMshrEntry> copyPredictMshrQueue;

int floorLog2(unsigned x) {
    assert(x > 0);

    int y = 0;

    if (x & 0xffff0000) { y += 16; x >>= 16; }
    if (x & 0x0000ff00) { y +=  8; x >>=  8; }
    if (x & 0x000000f0) { y +=  4; x >>=  4; }
    if (x & 0x0000000c) { y +=  2; x >>=  2; }
    if (x & 0x00000002) { y +=  1; }

    return y;
}

Addr pcHash(Addr pc) {
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
    int way = pcTableAssoc - 1;

    /* MJL_Comment 
    DPRINTF(HWPrefetch, "Victimizing lookup table[%d][%d].\n", set, way);
        */
    return &pcTable[master_id][set][way];
}

MemCmd::MJL_DirAttribute MJL_mshrPredict(Addr pkt_addr, StrideEntry* entry) {
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
        int rowOffset = new_stride / sizeof(uint64_t) + entry->lastRowOff;
        if (new_stride % (MJL_rowWidth * blkSize) == 0 && colOffset >= 0 && colOffset < (int) (blkSize/sizeof(uint64_t)) ) {
            selfStrideDir = MemCmd::MJL_DirAttribute::MJL_IsColumn;
            selfStrideStep = colOffset - entry->lastColOff;
        } else if ( rowOffset >= 0 && rowOffset < (int) (blkSize/sizeof(uint64_t)) ) {
            selfStrideDir = MemCmd::MJL_DirAttribute::MJL_IsRow;
            selfStrideStep = rowOffset - entry->lastRowOff;
        }
    }
    std::cout << "selfStrideDir " << selfStrideDir << ", selfStrideStep " << selfStrideStep << ", lastRowOff " << entry->lastRowOff << ", lastColOff " << entry->lastColOff << std::endl; 

    if (selfStrideStep != 0) {
        if (selfStrideDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
            for (int i = entry->lastRowOff + selfStrideStep; i < (int) (blkSize/sizeof(uint64_t)) && i >= 0; i += selfStrideStep) {
                if (entry->lastPredDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                    entry->blkHits[i] |= true;
                } else if (entry->lastPredDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                    entry->crossBlkHits[i] |= true;
                }
            }
        } else if (selfStrideDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
            std::cout << "lastColOff " << entry->lastColOff << ", lastColOffset + selfStrideStep " << entry->lastColOff + selfStrideStep << std::endl;
            for (int i = entry->lastColOff + selfStrideStep; i < (int) (blkSize/sizeof(uint64_t)) && i >= 0; i += selfStrideStep) {
                std::cout << "i " << i << std::endl; 
                if (entry->lastPredDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                    entry->blkHits[i] |= true;
                } else if (entry->lastPredDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                    entry->crossBlkHits[i] |= true;
                }
            }
        }
    }
    
    std::ostringstream output;
    output << "hit/crosshit " ;
    for (int i = 0; i < 8; ++i) {
        output << entry->blkHits[i];
    }
    output << "/";
    for (int i = 0; i < 8; ++i) {
        output << entry->crossBlkHits[i];
    }
    std::cout << output.str() << std::endl;

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
    
    return entry->lastPredDir;
}

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
        copyPredictMshrQueue.emplace_back(pkt, mshr);
        /* MJL_Test 
        std::cout << "MJL_predDebug: MJL_mshrPredictDir create mshr " << pkt->print() << std::endl;
            */
    }
}
// Remove entry from both predict queues and get predict direction
void MJL_removeFromPredictMshrQueue(const MSHR* mshr) {
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
        pcTable_entry->lastPredDir = entry_found->blkDir;
        for (unsigned i = 0; i < blkSize/sizeof(uint64_t); ++i) {
            pcTable_entry->blkHits[i] = entry_found->blkHits[i];
            pcTable_entry->crossBlkHits[i] = entry_found->crossBlkHits[i];
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

void print_predictMshrEntry(std::list<PredictMshrEntry>::iterator entry) {
    std::cout << entry->print() << std::endl;
}

void print_copyPredictMshrQueue() {
    for (auto entry = copyPredictMshrQueue.begin(); entry != copyPredictMshrQueue.end(); ++entry) {
        print_predictMshrEntry(entry);
    }
}

int main () {
    // Packet(Addr _addr, Addr _blkAddr, Addr _crossBlkAddr, MemCmd::MJL_DirAttribute _cmdDir, bool _is_secure, Addr _pc, MasterID _master_id) : addr(_addr), blkAddr(_blkAddr), crossBlkAddr(_crossBlkAddr), cmdDir(_cmdDir), is_secure(_is_secure), req(_pc, _master_id)

    // Addr 3, blkAddr 4, crossBlkAddr 5, cmdDir 1(row), secure 1(true), pc 1, master_id, 1
    PacketPtr pkt1_0 = new Packet(1064, 4, 5, MemCmd::MJL_DirAttribute::MJL_IsRow, true, 1, 1, 40, 32);
    PacketPtr pkt1_1 = new Packet(1320, 5, 5, MemCmd::MJL_DirAttribute::MJL_IsRow, true, 1, 1, 40, 40); // same pc, same column
    PacketPtr pkt1_2 = new Packet(1072, 4, 6, MemCmd::MJL_DirAttribute::MJL_IsRow, true, 1, 1, 48, 32); // same pc, same row
    PacketPtr pkt1_3 = new Packet(1832, 5, 7, MemCmd::MJL_DirAttribute::MJL_IsColumn, true, 1, 1, 56, 40); // same pc, diff dir, same column
    PacketPtr pkt2_0 = new Packet(1064, 4, 5, MemCmd::MJL_DirAttribute::MJL_IsRow, true, 2, 1, 40, 32); // diff pc, same row, same column
    PacketPtr pkt3_0 = new Packet(1064, 4, 5, MemCmd::MJL_DirAttribute::MJL_IsRow, false, 3, 1, 40, 32); // diff pc, same row, same column, diff secure
    PacketPtr pkt4_0 = new Packet(1064, 5, 4, MemCmd::MJL_DirAttribute::MJL_IsColumn, true, 4, 1, 32, 40); // diff pc, diff dir, same row, same column

    MSHR* mshr;
    StrideEntry *entry;
    MemCmd::MJL_DirAttribute predict_dir;
    std::cout << "create mshr: pkt1_0" << std::endl;
    mshr = new MSHR(pkt1_0, blkSize);
    MJL_addToPredictMshrQueue(pkt1_0, mshr);
    print_copyPredictMshrQueue();
    std::cout << "recv same column pkt: pkt1_1" << std::endl;
    MJL_updatePredictMshrQueue(pkt1_1);
    print_copyPredictMshrQueue();
    std::cout << "recv same row pkt: pkt1_2" << std::endl;
    MJL_updatePredictMshrQueue(pkt1_2);
    print_copyPredictMshrQueue();
    std::cout << "delete mshr: " << std::endl;
    MJL_removeFromPredictMshrQueue(mshr);
    std::cout << "recv same row pkt: pkt1_2" << std::endl;
    pcTableHit(pkt1_2->req->getPC(), pkt1_2->isSecure(), pkt1_2->req->masterId(), entry);
    predict_dir = MJL_mshrPredict(pkt1_2->getAddr(), entry);
    std::cout << predict_dir << std::endl;
    delete mshr;
    print_copyPredictMshrQueue();

    entry->reset_entry();
    std::cout << "create mshr: pkt1_0" << std::endl;
    mshr = new MSHR(pkt1_0, blkSize);
    MJL_addToPredictMshrQueue(pkt1_0, mshr);
    print_copyPredictMshrQueue();
    std::cout << "recv same column pkt: pkt1_1" << std::endl;
    MJL_updatePredictMshrQueue(pkt1_1);
    std::cout << "recv same row pkt: pkt1_2" << std::endl;
    MJL_updatePredictMshrQueue(pkt1_2);
    std::cout << "recv same row/column pkt: pkt2_0" << std::endl;
    MJL_updatePredictMshrQueue(pkt2_0);
    print_copyPredictMshrQueue();
    std::cout << "delete mshr: " << std::endl;
    MJL_removeFromPredictMshrQueue(mshr);
    std::cout << "recv same column pkt: pkt1_3" << std::endl;
    pcTableHit(pkt1_3->req->getPC(), pkt1_3->isSecure(), pkt1_3->req->masterId(), entry);
    predict_dir = MJL_mshrPredict(pkt1_3->getAddr(), entry);
    std::cout << predict_dir << std::endl;
    delete mshr;
    print_copyPredictMshrQueue();

    entry->reset_entry();
    std::cout << "create mshr: pkt1_0" << std::endl;
    mshr = new MSHR(pkt1_0, blkSize);
    MJL_addToPredictMshrQueue(pkt1_0, mshr);
    print_copyPredictMshrQueue();
    std::cout << "recv same column pkt: pkt1_1" << std::endl;
    MJL_updatePredictMshrQueue(pkt1_1);
    std::cout << "recv same row pkt: pkt1_2" << std::endl;
    MJL_updatePredictMshrQueue(pkt1_2);
    std::cout << "recv same row/column pkt: pkt2_0" << std::endl;
    MJL_updatePredictMshrQueue(pkt2_0);
    std::cout << "recv same column pkt: pkt1_3" << std::endl;
    MJL_updatePredictMshrQueue(pkt1_3);
    print_copyPredictMshrQueue();
    std::cout << "delete mshr: " << std::endl;
    MJL_removeFromPredictMshrQueue(mshr);
    std::cout << "recv same row pkt: pkt1_2" << std::endl;
    pcTableHit(pkt1_2->req->getPC(), pkt1_2->isSecure(), pkt1_2->req->masterId(), entry);
    predict_dir = MJL_mshrPredict(pkt1_2->getAddr(), entry);
    std::cout << predict_dir << std::endl;
    delete mshr;
    print_copyPredictMshrQueue();

    entry->reset_entry();
    MSHR* mshr2;
    std::cout << "create mshr: pkt1_0" << std::endl;
    mshr = new MSHR(pkt1_0, blkSize);
    MJL_addToPredictMshrQueue(pkt1_0, mshr);
    print_copyPredictMshrQueue();
    std::cout << "recv same column pkt: pkt1_1" << std::endl;
    MJL_updatePredictMshrQueue(pkt1_1);
    std::cout << "recv same row pkt: pkt1_2" << std::endl;
    MJL_updatePredictMshrQueue(pkt1_2);
    std::cout << "recv same row/column pkt: pkt2_0" << std::endl;
    MJL_updatePredictMshrQueue(pkt2_0);
    std::cout << "create mshr2: pkt4_0" << std::endl;
    mshr2 = new MSHR(pkt4_0, blkSize);
    MJL_addToPredictMshrQueue(pkt4_0, mshr2);
    print_copyPredictMshrQueue();
    std::cout << "recv same column pkt: pkt1_1" << std::endl;
    MJL_updatePredictMshrQueue(pkt1_1);
    std::cout << "recv same row/column, diff secure pkt: pkt3_0" << std::endl;
    MJL_updatePredictMshrQueue(pkt3_0);
    print_copyPredictMshrQueue();
    std::cout << "delete mshr: " << std::endl;
    MJL_removeFromPredictMshrQueue(mshr);
    std::cout << "recv same row pkt: pkt1_2" << std::endl;
    pcTableHit(pkt1_2->req->getPC(), pkt1_2->isSecure(), pkt1_2->req->masterId(), entry);
    predict_dir = MJL_mshrPredict(pkt1_2->getAddr(), entry);
    std::cout << predict_dir << std::endl;
    delete mshr;
    print_copyPredictMshrQueue();
    std::cout << "recv same row pkt: pkt1_2" << std::endl;
    MJL_updatePredictMshrQueue(pkt1_2);
    std::cout << "recv same row pkt: pkt1_2" << std::endl;
    MJL_updatePredictMshrQueue(pkt1_2);
    print_copyPredictMshrQueue();
    std::cout << "delete mshr2: " << std::endl;
    MJL_removeFromPredictMshrQueue(mshr2);
    std::cout << "recv same row pkt: pkt1_2" << std::endl;
    pcTableHit(pkt1_2->req->getPC(), pkt1_2->isSecure(), pkt1_2->req->masterId(), entry);
    predict_dir = MJL_mshrPredict(pkt1_2->getAddr(), entry);
    std::cout << predict_dir << std::endl;
    std::cout << "recv same row/column pkt: pkt4_0" << std::endl;
    pcTableHit(pkt4_0->req->getPC(), pkt4_0->isSecure(), pkt4_0->req->masterId(), entry);
    predict_dir = MJL_mshrPredict(pkt4_0->getAddr(), entry);
    std::cout << predict_dir << std::endl;
    delete mshr2;

    delete pkt1_0;
    delete pkt1_1;
    delete pkt1_2;
    delete pkt1_3;
    delete pkt2_0;
    delete pkt3_0;
    delete pkt4_0;
    
    return 0;

}
