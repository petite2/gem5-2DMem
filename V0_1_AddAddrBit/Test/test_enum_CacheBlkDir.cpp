#include <iostream>

enum MJL_DirAttribute
{
    // MJL_TODO: Check whether adding this would break things
    MJL_IsInvalid,  //!< Data access direction is invalid
    MJL_IsRow,      //!< Data access direction is row
    MJL_IsColumn,   //!< Data access direction is column
    MJL_NUM_COMMAND_DIRATTRIBUTES
};

enum MJL_CacheBlkDir : unsigned {
    /** Row, default */
    MJL_BlkRow =    MJL_IsRow,
    /** Column */
    MJL_BlkCol =    MJL_IsColumn,
};

int main() {
    MJL_CacheBlkDir MJL_blkDir;
    MJL_blkDir = MJL_BlkRow;
    std::cout << MJL_blkDir;
    MJL_blkDir = MJL_BlkCol;
    std::cout << MJL_blkDir;
    return 0;
}
