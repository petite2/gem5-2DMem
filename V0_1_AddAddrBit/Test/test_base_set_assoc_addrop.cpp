#include <iostream>       // std::cout
#include <bitset>         // std::bitset
#include <cinttypes>
#include <cassert>

typedef uint64_t Addr;
typedef unsigned long long ULL;

enum MJL_DirAttribute
{
    MJL_IsInvalid,  //!< Data access direction is invalid
    MJL_IsRow,      //!< Data access direction is row
    MJL_IsColumn,   //!< Data access direction is column
    MJL_NUM_COMMAND_DIRATTRIBUTES
};
class CacheBlk {
    public:
    typedef MJL_DirAttribute MJL_CacheBlkDir;
};

int numSets = 1024;
int blkSize = 64;
int MJL_rowWidth = 512;

int setShift;
/** The amount to shift the address to get the tag. */
int tagShift;
/** Mask out all bits that aren't part of the set index. */
unsigned setMask;
/** Mask out all bits that aren't part of the block offset. */
unsigned blkMask;
// Mask first and then shift to get the value
/** Mask out all bits that aren't part of the column block offset. */
uint64_t MJL_blkMaskColumn;
/** Mask out all bits that aren't part of the byte offset. */
uint64_t MJL_byteMask;
/** Mask out all bits that aren't part of the word offset. */
uint64_t MJL_wordMask;
/** Mask out all bits that aren't part of the same for both row and column (the higher part). */
uint64_t MJL_commonHighMask;
/** Mask out all bits that aren't part of the same for both row and column (the lower part). */
uint64_t MJL_commonLowMask;
/** The amount to shift the address to get the column. */
int MJL_colShift;
/** The amount to shift the address to get the row. */
int MJL_rowShift;

Addr MJL_swapRowColBits(Addr addr)  
{
    Addr new_row = (addr >> MJL_colShift) & (Addr)MJL_wordMask;
    Addr new_col = (addr >> MJL_rowShift) & (Addr)MJL_wordMask;
    return ((addr & ~(((Addr)MJL_wordMask << MJL_colShift) | ((Addr)MJL_wordMask << MJL_rowShift))) | (new_row << MJL_rowShift) | (new_col << MJL_colShift));
}
Addr MJL_movColRight(Addr addr)  
{
    Addr col = (addr >> MJL_colShift) & (Addr)MJL_wordMask;
    Addr common_high = addr & MJL_commonHighMask;
    Addr common_low = addr & MJL_commonLowMask;
    Addr blk_offset = addr & blkMask;
    return (common_high | (common_low << (setShift - MJL_rowShift)) | (col << setShift) | blk_offset);
}
Addr MJL_movColLeft(Addr addr)  
{
    Addr col = (addr >> setShift) & (Addr)MJL_wordMask;
    Addr common_high = addr & MJL_commonHighMask;
    Addr common_low = addr & (MJL_commonLowMask << (setShift - MJL_rowShift));
    Addr blk_offset = addr & blkMask;
    return (common_high | (common_low >> (setShift - MJL_rowShift)) | (col << MJL_colShift) | blk_offset);
}

Addr MJL_extractTag(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir)
{
    if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
        return (MJL_movColRight(addr) >> tagShift);
    } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) { // MJL_TODO: Placeholder for column 
        return (MJL_movColRight(MJL_swapRowColBits(addr)) >> tagShift);
    } else {
        return (addr >> tagShift);
    }
}

int MJL_extractSet(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir)
{
    if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
        return ((MJL_movColRight(addr) >> setShift) & setMask);
    } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) { // MJL_TODO: Placeholder for column 
        return ((MJL_movColRight(MJL_swapRowColBits(addr)) >> setShift) & setMask);
    } else {
        return ((addr >> setShift) & setMask);
    }
}

Addr MJL_blkAlign(Addr addr, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir) 
{
    if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
        return (addr & ~(Addr)blkMask);
    } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) {
        return (addr & ~(Addr)MJL_blkMaskColumn);
    } else {
        return (addr & ~(Addr)blkMask);
    }
}

Addr MJL_regenerateBlkAddr(Addr tag, CacheBlk::MJL_CacheBlkDir MJL_cacheBlkDir, unsigned set) 
{
    if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsRow) {
        return MJL_movColLeft(((tag << tagShift) | ((Addr)set << setShift)));
    } else if (MJL_cacheBlkDir == CacheBlk::MJL_CacheBlkDir::MJL_IsColumn) { // MJL_TODO: Placeholder for column
        return MJL_swapRowColBits(MJL_movColLeft(((tag << tagShift) | ((Addr)set << setShift))));
    } else {
        return ((tag << tagShift) | ((Addr)set << setShift));
    }
}

int
floorLog2(unsigned x)
{
    assert(x > 0);

    int y = 0;

    if (x & 0xffff0000) { y += 16; x >>= 16; }
    if (x & 0x0000ff00) { y +=  8; x >>=  8; }
    if (x & 0x000000f0) { y +=  4; x >>=  4; }
    if (x & 0x0000000c) { y +=  2; x >>=  2; }
    if (x & 0x00000002) { y +=  1; }

    return y;
}

int
floorLog2(unsigned long x)
{
    assert(x > 0);

    int y = 0;


    if (x & 0xffff0000) { y += 16; x >>= 16; }
    if (x & 0x0000ff00) { y +=  8; x >>=  8; }
    if (x & 0x000000f0) { y +=  4; x >>=  4; }
    if (x & 0x0000000c) { y +=  2; x >>=  2; }
    if (x & 0x00000002) { y +=  1; }

    return y;
}

int
floorLog2(unsigned long long x)
{
    assert(x > 0);

    int y = 0;

    if (x & ULL(0xffffffff00000000)) { y += 32; x >>= 32; }
    if (x & ULL(0x00000000ffff0000)) { y += 16; x >>= 16; }
    if (x & ULL(0x000000000000ff00)) { y +=  8; x >>=  8; }
    if (x & ULL(0x00000000000000f0)) { y +=  4; x >>=  4; }
    if (x & ULL(0x000000000000000c)) { y +=  2; x >>=  2; }
    if (x & ULL(0x0000000000000002)) { y +=  1; }

    return y;
}

inline int
floorLog2(int x)
{
    assert(x > 0);
    return floorLog2((unsigned)x);
}

inline int
floorLog2(long x)
{
    assert(x > 0);
    return floorLog2((unsigned long)x);
}

inline int
floorLog2(long long x)
{
    assert(x > 0);
    return floorLog2((unsigned long long)x);
}

void show(std::bitset<64> addr, int msb, int lsb) {
    for (int i = msb; i >= lsb; i--) {
        std::cout << addr[i];
    }
}

void sep(Addr addr) {
  std::cout << "H:C:W:R:B: ";
  show(std::bitset<64>(addr), 63, floorLog2(MJL_rowWidth * blkSize * blkSize/sizeof(uint64_t))); std::cout << ":";
  show(std::bitset<64>(addr), floorLog2(MJL_rowWidth * blkSize * blkSize/sizeof(uint64_t)) - 1, floorLog2(MJL_rowWidth * blkSize)); std::cout << ":";
  show(std::bitset<64>(addr), floorLog2(MJL_rowWidth * blkSize) - 1, floorLog2(blkSize)); std::cout << ":";
  show(std::bitset<64>(addr), floorLog2(blkSize) - 1, floorLog2(sizeof(uint64_t))); std::cout << ":";
  show(std::bitset<64>(addr), floorLog2(sizeof(uint64_t)) - 1, 0); std::cout << "\n";
}


int main ()
{
    Addr addr = 123456789; 

    blkMask = blkSize - 1;
    MJL_byteMask = sizeof(uint64_t) - 1;
    MJL_rowShift = floorLog2(sizeof(uint64_t));
    MJL_wordMask = blkSize/sizeof(uint64_t) - 1;
    MJL_colShift = floorLog2(MJL_rowWidth) + floorLog2(blkSize);
    MJL_blkMaskColumn = (MJL_wordMask << MJL_colShift) | MJL_byteMask;
    MJL_commonHighMask = ~(MJL_rowWidth * blkSize * blkSize/sizeof(uint64_t) - 1);
    MJL_commonLowMask = (MJL_rowWidth - 1) << floorLog2(blkSize);
    /* MJL_End */
    setShift = floorLog2(blkSize);
    setMask = numSets - 1;
    tagShift = setShift + floorLog2(numSets);

    Addr tagR = MJL_extractTag(addr, MJL_DirAttribute::MJL_IsRow);
    Addr tagC = MJL_extractTag(addr, MJL_DirAttribute::MJL_IsColumn);
    Addr setR = MJL_extractSet(addr, MJL_DirAttribute::MJL_IsRow);
    Addr setC = MJL_extractSet(addr, MJL_DirAttribute::MJL_IsColumn);
    Addr blkR = MJL_blkAlign(addr, MJL_DirAttribute::MJL_IsRow);
    Addr blkC = MJL_blkAlign(addr, MJL_DirAttribute::MJL_IsColumn);
    Addr regR = MJL_regenerateBlkAddr(tagR, MJL_DirAttribute::MJL_IsRow, setR);
    Addr regC = MJL_regenerateBlkAddr(tagC, MJL_DirAttribute::MJL_IsColumn, setC);


    std::cout << "addr:      " << std::bitset<64>(addr) << '\n';
    sep(addr);
    std::cout << "swap:     " << std::bitset<64>(MJL_swapRowColBits(addr)) << '\n';
    sep(MJL_swapRowColBits(addr));
    std::cout << "movR:     " << std::bitset<64>(MJL_movColRight(addr)) << '\n';
    sep(MJL_movColRight(addr));
    std::cout << "movL:     " << std::bitset<64>(MJL_movColLeft(addr)) << '\n';
    sep(MJL_movColLeft(addr));
    std::cout << "swapmovR:     " << std::bitset<64>(MJL_movColRight(MJL_swapRowColBits(addr))) << '\n';
    sep(MJL_movColRight(MJL_swapRowColBits(addr)));
    std::cout << "tagR:      " << std::bitset<64>(tagR)  << "\n";
    std::cout << "tagC:      " << std::bitset<64>(tagC)  << "\n";
    std::cout << "setR:      " << std::bitset<64>(setR)  << "\n";
    std::cout << "setC:      " << std::bitset<64>(setC)  << "\n";
    std::cout << "blkR:      " << std::bitset<64>(blkR)  << "\n";
    std::cout << "blkC:      " << std::bitset<64>(blkC)  << "\n";
    std::cout << "regR:      " << std::bitset<64>(regR)  << "\n";
    std::cout << "regC:      " << std::bitset<64>(regC)  << "\n";
    std::cout << "addr:      " << std::bitset<64>(addr) << '\n';
    return 0;
}