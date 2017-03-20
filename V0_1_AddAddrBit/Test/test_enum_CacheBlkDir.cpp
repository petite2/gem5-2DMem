#include <iostream>

enum MJL_CacheBlkDir : unsigned {
    /** Row, default */
    MJL_BlkRow =    1,
    /** Column */
    MJL_BlkCol =    2,
};

int main() {
    MJL_CacheBlkDir MJL_blkDir;
    MJL_blkDir = MJL_BlkRow;
    std::cout << MJL_blkDir;
    MJL_blkDir = MJL_BlkCol;
    std::cout << MJL_blkDir;
    return 0;
}
