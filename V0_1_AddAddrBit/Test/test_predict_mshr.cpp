#include <list>
#include <iostream>       // std::cout
#include <cinttypes>
#include <cassert>

typedef uint64_t Addr;
typedef uint16_t MasterID;

unsigned blkSize = 64;

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
    };

    Addr addr;
    Addr blkAddr;
    Addr crossBlkAddr;
    MemCmd::MJL_DirAttribute cmdDir;
    bool is_secure;
    Request* req;

    Packet(Addr _addr, Addr _blkAddr, Addr _crossBlkAddr, MemCmd::MJL_DirAttribute _cmdDir, bool _is_secure, Addr _pc, MasterID _master_id) : addr(_addr), blkAddr(_blkAddr), crossBlkAddr(_crossBlkAddr), cmdDir(_cmdDir), is_secure(_is_secure) {req = new Request(_pc, _master_id);}
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

struct PredictMshrEntry
{
    PredictMshrEntry(Addr in_pc, Addr in_blkAddr, Addr in_crossBlkAddr, MemCmd::MJL_DirAttribute in_blkDir, MemCmd::MJL_DirAttribute in_crossBlkDir, bool in_isSecure, Addr in_accessAddr, const MSHR* in_mshr, MasterID in_masterId) : pc(in_pc), blkAddr(in_blkAddr), crossBlkAddr(in_crossBlkAddr), blkDir(in_blkDir), crossBlkDir(in_crossBlkDir), isSecure(in_isSecure), accessAddr(in_accessAddr), mshr(in_mshr), masterId(in_masterId), blkHitCounter(0), crossBlkHitCounter(0)
    { }

    Addr pc;
    Addr blkAddr;
    Addr crossBlkAddr;
    MemCmd::MJL_DirAttribute blkDir;
    MemCmd::MJL_DirAttribute crossBlkDir;
    bool isSecure;
    Addr accessAddr;
    const MSHR* mshr;
    MasterID masterId;
    uint64_t blkHitCounter;
    uint64_t crossBlkHitCounter;
};
std::list<PredictMshrEntry> copyPredictMshrQueue;

// Update counters on access
void MJL_updatePredictMshrQueue(const PacketPtr pkt) {
    Addr pkt_blkAddr = pkt->getBlockAddr(blkSize);
    Addr pkt_crossBlkAddr = pkt->MJL_getCrossBlockAddr(blkSize);
    bool pkt_isSecure = pkt->isSecure();
    for (std::list<PredictMshrEntry>::iterator it = copyPredictMshrQueue.begin(); it != copyPredictMshrQueue.end(); it++) {
        pkt_blkAddr = pkt->MJL_getDirBlockAddr(blkSize, it->blkDir);
        if (pkt_blkAddr == it->blkAddr && pkt_isSecure == it->isSecure) {
            it->blkHitCounter++;
        }
        pkt_crossBlkAddr = pkt->MJL_getDirBlockAddr(blkSize, it->crossBlkDir);
        if (pkt_crossBlkAddr == it->crossBlkAddr && pkt_isSecure == it->isSecure) {
            it->crossBlkHitCounter++;
        }
    }
}
// Add entry to both predict queue
void MJL_addToPredictMshrQueue(const PacketPtr pkt, const MSHR* mshr) {
    copyPredictMshrQueue.emplace_back(pkt->req->getPC(), pkt->getBlockAddr(blkSize), pkt->MJL_getCrossBlockAddr(blkSize), pkt->MJL_getCmdDir(), pkt->MJL_getCrossCmdDir(), pkt->isSecure(), pkt->getAddr(), mshr, pkt->req->masterId());
}
// Remove entry from both predict queues and get predict direction
MemCmd::MJL_DirAttribute MJL_removeFromPredictMshrQueue(const MSHR* mshr) {
    std::list<PredictMshrEntry>::iterator entry_found = copyPredictMshrQueue.end();
    MemCmd::MJL_DirAttribute predict_dir = mshr->MJL_qEntryDir;
    // StrideEntry *pcTable_entry;
    // MasterID master_id = 0;
    // Find the entry to be removed
    for (std::list<PredictMshrEntry>::iterator it = copyPredictMshrQueue.begin(); it != copyPredictMshrQueue.end(); it++) {
        if (mshr == it->mshr) {
            entry_found = it;
            assert(mshr->blkAddr == it->blkAddr && mshr->MJL_qEntryDir == it->blkDir && mshr->isSecure == it->isSecure);
            break;
        }
    }
    assert(entry_found != copyPredictMshrQueue.end());
    // Predict the direction
    if (entry_found->crossBlkHitCounter > entry_found->blkHitCounter) {
        predict_dir = entry_found->crossBlkDir;
    } else {
        predict_dir = entry_found->blkDir;
    }
    // Add predict direction to pc table
    // master_id = useMasterId ? entry_found->masterId : 0;
    // if (pcTableHit(entry_found->pc, entry_found->isSecure, master_id, pcTable_entry)) {
    //     pcTable_entry->lastPredDir = predict_dir;
    // } else {
    //     StrideEntry* victim_entry = pcTableVictim(entry_found->pc, master_id);
    //     victim_entry->instAddr = entry_found->pc;
    //     victim_entry->lastAddr = entry_found->accessAddr;
    //     victim_entry->isSecure= entry_found->isSecure;
    //     victim_entry->stride = 0;
    //     victim_entry->confidence = startConf;
    //     victim_entry->lastPredDir = predict_dir;
    // }

    copyPredictMshrQueue.erase(entry_found);

    return predict_dir;
}

void print_predictMshrEntry(std::list<PredictMshrEntry>::iterator entry) {
    std::cout << "pc: " << entry->pc << " ";
    std::cout << "blkAddr: " << entry->blkAddr << " ";
    std::cout << "crossBlkAddr: " << entry->crossBlkAddr << " ";
    std::cout << "blkDir: " << entry->blkDir << " ";
    std::cout << "crossBlkDir: " << entry->crossBlkDir << " ";
    std::cout << "isSecure: " << entry->isSecure << " ";
    std::cout << "accessAddr: " << entry->accessAddr << " ";
    std::cout << "mshr: " << entry->mshr << " ";
    std::cout << "masterId: " << entry->masterId << " ";
    std::cout << "blkHitCounter: " << entry->blkHitCounter << " ";
    std::cout << "crossBlkHitCounter: " << entry->crossBlkHitCounter << std::endl;
}

void print_copyPredictMshrQueue() {
    for (auto entry = copyPredictMshrQueue.begin(); entry != copyPredictMshrQueue.end(); ++entry) {
        print_predictMshrEntry(entry);
    }
}

int main () {
    // Packet(Addr _addr, Addr _blkAddr, Addr _crossBlkAddr, MemCmd::MJL_DirAttribute _cmdDir, bool _is_secure, Addr _pc, MasterID _master_id) : addr(_addr), blkAddr(_blkAddr), crossBlkAddr(_crossBlkAddr), cmdDir(_cmdDir), is_secure(_is_secure), req(_pc, _master_id)

    // Addr 3, blkAddr 4, crossBlkAddr 5, cmdDir 1(row), secure 1(true), pc 1, master_id, 1
    PacketPtr pkt1_0 = new Packet(3, 4, 5, MemCmd::MJL_DirAttribute::MJL_IsRow, true, 1, 1);
    PacketPtr pkt1_1 = new Packet(4, 5, 5, MemCmd::MJL_DirAttribute::MJL_IsRow, true, 1, 1); // same pc, same column
    PacketPtr pkt1_2 = new Packet(5, 4, 6, MemCmd::MJL_DirAttribute::MJL_IsRow, true, 1, 1); // same pc, same row
    PacketPtr pkt1_3 = new Packet(6, 5, 7, MemCmd::MJL_DirAttribute::MJL_IsColumn, true, 1, 1); // same pc, diff dir, same column
    PacketPtr pkt2_0 = new Packet(7, 4, 5, MemCmd::MJL_DirAttribute::MJL_IsRow, true, 2, 1); // diff pc, same row, same column
    PacketPtr pkt3_0 = new Packet(7, 4, 5, MemCmd::MJL_DirAttribute::MJL_IsRow, false, 3, 1); // diff pc, same row, same column, diff secure
    PacketPtr pkt4_0 = new Packet(7, 5, 4, MemCmd::MJL_DirAttribute::MJL_IsColumn, true, 4, 1); // diff pc, diff dir, same row, same column

    MSHR* mshr;
    MemCmd::MJL_DirAttribute predict_dir;
    std::cout << "create mshr: " << std::endl;
    mshr = new MSHR(pkt1_0, blkSize);
    MJL_addToPredictMshrQueue(pkt1_0, mshr);
    print_copyPredictMshrQueue();
    std::cout << "recv same column pkt: " << std::endl;
    MJL_updatePredictMshrQueue(pkt1_1);
    print_copyPredictMshrQueue();
    std::cout << "recv same row pkt: " << std::endl;
    MJL_updatePredictMshrQueue(pkt1_2);
    print_copyPredictMshrQueue();
    std::cout << "delete mshr: predict_dir ";
    predict_dir = MJL_removeFromPredictMshrQueue(mshr);
    std::cout << predict_dir << std::endl;
    delete mshr;
    print_copyPredictMshrQueue();

    std::cout << "create mshr: " << std::endl;
    mshr = new MSHR(pkt1_0, blkSize);
    MJL_addToPredictMshrQueue(pkt1_0, mshr);
    print_copyPredictMshrQueue();
    std::cout << "recv same column pkt: " << std::endl;
    MJL_updatePredictMshrQueue(pkt1_1);
    std::cout << "recv same row pkt: " << std::endl;
    MJL_updatePredictMshrQueue(pkt1_2);
    std::cout << "recv same row/column pkt: " << std::endl;
    MJL_updatePredictMshrQueue(pkt2_0);
    print_copyPredictMshrQueue();
    std::cout << "delete mshr: predict_dir ";
    predict_dir = MJL_removeFromPredictMshrQueue(mshr);
    std::cout << predict_dir << std::endl;
    delete mshr;
    print_copyPredictMshrQueue();

    std::cout << "create mshr: " << std::endl;
    mshr = new MSHR(pkt1_0, blkSize);
    MJL_addToPredictMshrQueue(pkt1_0, mshr);
    print_copyPredictMshrQueue();
    std::cout << "recv same column pkt: " << std::endl;
    MJL_updatePredictMshrQueue(pkt1_1);
    std::cout << "recv same row pkt: " << std::endl;
    MJL_updatePredictMshrQueue(pkt1_2);
    std::cout << "recv same row/column pkt: " << std::endl;
    MJL_updatePredictMshrQueue(pkt2_0);
    std::cout << "recv same column pkt: " << std::endl;
    MJL_updatePredictMshrQueue(pkt1_3);
    print_copyPredictMshrQueue();
    std::cout << "delete mshr: predict_dir ";
    predict_dir = MJL_removeFromPredictMshrQueue(mshr);
    std::cout << predict_dir << std::endl;
    delete mshr;
    print_copyPredictMshrQueue();

    MSHR* mshr2 = new MSHR(pkt2_0, blkSize);
    std::cout << "create mshr: " << std::endl;
    mshr = new MSHR(pkt1_0, blkSize);
    MJL_addToPredictMshrQueue(pkt1_0, mshr);
    print_copyPredictMshrQueue();
    std::cout << "recv same column pkt: " << std::endl;
    MJL_updatePredictMshrQueue(pkt1_1);
    std::cout << "recv same row pkt: " << std::endl;
    MJL_updatePredictMshrQueue(pkt1_2);
    std::cout << "recv same row/column pkt: " << std::endl;
    MJL_updatePredictMshrQueue(pkt2_0);
    std::cout << "create mshr2: " << std::endl;
    mshr2 = new MSHR(pkt4_0, blkSize);
    MJL_addToPredictMshrQueue(pkt4_0, mshr2);
    print_copyPredictMshrQueue();
    std::cout << "recv same column pkt: " << std::endl;
    MJL_updatePredictMshrQueue(pkt1_1);
    std::cout << "recv same row/column, diff secure pkt: " << std::endl;
    MJL_updatePredictMshrQueue(pkt3_0);
    print_copyPredictMshrQueue();
    std::cout << "delete mshr: predict_dir ";
    predict_dir = MJL_removeFromPredictMshrQueue(mshr);
    std::cout << predict_dir << std::endl;
    delete mshr;
    print_copyPredictMshrQueue();
    std::cout << "recv same row pkt: " << std::endl;
    MJL_updatePredictMshrQueue(pkt1_2);
    std::cout << "recv same row pkt: " << std::endl;
    MJL_updatePredictMshrQueue(pkt1_2);
    print_copyPredictMshrQueue();
    std::cout << "delete mshr2: predict_dir ";
    predict_dir = MJL_removeFromPredictMshrQueue(mshr2);
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