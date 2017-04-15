#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <cinttypes>
#include <cassert>
#include <iostream>

typedef uint64_t Addr;

class CacheBlk {
    public:
        enum MJL_CacheBlkDir
        {
            // MJL_TODO: Check whether adding this would break things
            MJL_IsInvalid,  //!< Requested direction is invalid
            MJL_IsRow,      //!< Requested direction is row
            MJL_IsColumn,   //!< Requested direction is column
            MJL_NUM_COMMAND_DIRATTRIBUTES
        };
};

int main (int argc , char *argv[]) {
    std::map<Addr, CacheBlk::MJL_CacheBlkDir> MJL_PC2DirMap;
    std::string MJL_PC2DirFilename(argv[1]);

    // Function MJL_readPC2DirMap in cache.hh
    std::ifstream MJL_PC2DirFile;
    std::string line;
    Addr tempPC;
    char tempDir;

    MJL_PC2DirFile.open(MJL_PC2DirFilename);
    if (MJL_PC2DirFile.is_open()) {
        while (getline(MJL_PC2DirFile, line)) {
            std::stringstream(line) >> tempPC >> tempDir;
            if (MJL_PC2DirMap.find(tempPC) != MJL_PC2DirMap.end()) {
                std::cout << "MJL_Error: Redefinition of instruction direction found!\n";
            }
            if (tempDir == 'R') {
                MJL_PC2DirMap[tempPC] =  CacheBlk::MJL_CacheBlkDir::MJL_IsRow;
            } else if (tempDir == 'C') {
                MJL_PC2DirMap[tempPC] =  CacheBlk::MJL_CacheBlkDir::MJL_IsColumn;
            } else {
                std::cout << "MJL_Error: Invalid input direction annotation!\n";
                assert((tempDir == 'R') || (tempDir == 'C'));
            }
            // MJL_Test: For test use
            if (MJL_PC2DirMap.find(tempPC) != MJL_PC2DirMap.end()) {
                std::cout << "PC: " << MJL_PC2DirMap.find(tempPC)->first << ", Dir: " << MJL_PC2DirMap.find(tempPC)->second << "\n";
            }
        }
    }
    else {
        std::cout << "MJL_Error: Could not open input file!\n";
        assert(MJL_PC2DirFile.is_open());
    }
    MJL_PC2DirFile.close();

    std::cout << "After reading all the file\n";
    for (auto it = MJL_PC2DirMap.begin(); it != MJL_PC2DirMap.end(); ++it) {
        std::cout << "PC: " << it->first << ", Dir: " << it->second << "\n";
    }

    return 0;
}
