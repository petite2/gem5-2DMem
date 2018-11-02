// Example program
#include <iostream>
#include <string>
#include <map>
#include <cinttypes>
#include <vector>
#include <cassert>

typedef uint64_t Addr;

typedef uint64_t Addr;

class MemCmd {
    public:
        enum MJL_DirAttribute
        {
            MJL_IsInvalid,  //!< Requested direction is invalid
            MJL_IsRow,      //!< Requested direction is row
            MJL_IsColumn,   //!< Requested direction is column
            MJL_NUM_COMMAND_DIRATTRIBUTES
        };
};


std::map < Addr, std::map < MemCmd::MJL_DirAttribute, uint64_t > > * MJL_perPCAccessCount;
std::map < Addr, std::vector < MemCmd::MJL_DirAttribute > > * MJL_perPCAccessTrace;

void MJL_countAccess(Addr pc, MemCmd::MJL_DirAttribute dir) {
    if ( MJL_perPCAccessCount->find(pc) == MJL_perPCAccessCount->end() || (*MJL_perPCAccessCount)[pc].find(dir) == (*MJL_perPCAccessCount)[pc].end() ) {
        (*MJL_perPCAccessCount)[pc][dir] = 0;
    }
    (*MJL_perPCAccessCount)[pc][dir]++;
    (*MJL_perPCAccessTrace)[pc].push_back(dir);
}

void MJL_printAccess() {
    std::cout << std::endl << "==== MJL_perPCAccessCount Begin ====" << std::endl;
    std::cout << "PC Row_Accesses Col_Accesses" << std::endl;
    for (auto it = MJL_perPCAccessCount->begin(); it != MJL_perPCAccessCount->end(); ++it) {
        if ( it->second.find(MemCmd::MJL_DirAttribute::MJL_IsRow) == it->second.end() ) {
            (it->second)[MemCmd::MJL_DirAttribute::MJL_IsRow] = 0;
        }
        if ( it->second.find(MemCmd::MJL_DirAttribute::MJL_IsColumn) == it->second.end() ) {
            (it->second)[MemCmd::MJL_DirAttribute::MJL_IsColumn] = 0;
        }
        std::cout << std::hex << it->first << std::dec << " " << (it->second)[MemCmd::MJL_DirAttribute::MJL_IsRow] << " " << (it->second)[MemCmd::MJL_DirAttribute::MJL_IsColumn] << std::endl;
    }
    std::cout << "==== MJL_perPCAccessCount End ====" << std::endl;
    std::cout << "==== MJL_perPCAccessTrace Begin ====" << std::endl;
    std::cout << "PC Trace(0 for row, 1 for column)" << std::endl;
    for (auto it = MJL_perPCAccessTrace->begin(); it != MJL_perPCAccessTrace->end(); ++it) {
        uint64_t temp_rowAccesses = 0;
        uint64_t temp_colAccesses = 0;
        if ((*MJL_perPCAccessCount)[it->first][MemCmd::MJL_DirAttribute::MJL_IsRow] + (*MJL_perPCAccessCount)[it->first][MemCmd::MJL_DirAttribute::MJL_IsColumn] < 5) { continue; }
        std::cout << std::hex << it->first << std::dec << " ";
        for (auto dir : (it->second)) {
            if (dir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                std::cout << 0 << " ";
                temp_rowAccesses++;
            } else if (dir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                std::cout << 1 << " ";
                temp_colAccesses++;
            }
        }
        std::cout << std::endl;
        assert(temp_rowAccesses == (*MJL_perPCAccessCount)[it->first][MemCmd::MJL_DirAttribute::MJL_IsRow] && temp_colAccesses == (*MJL_perPCAccessCount)[it->first][MemCmd::MJL_DirAttribute::MJL_IsColumn]);
    }
    std::cout << "==== MJL_perPCAccessTrace End ====" << std::endl;
}

int main()
{
  std::cout << "Hello, a !\n";
  
  MJL_perPCAccessCount = new std::map < Addr, std::map < MemCmd::MJL_DirAttribute, uint64_t > >();
  MJL_perPCAccessTrace = new std::map < Addr, std::vector < MemCmd::MJL_DirAttribute > > ();
  
  MJL_countAccess(1, MemCmd::MJL_DirAttribute::MJL_IsRow);
  MJL_countAccess(1, MemCmd::MJL_DirAttribute::MJL_IsRow);
  MJL_countAccess(1, MemCmd::MJL_DirAttribute::MJL_IsRow);
  MJL_countAccess(1, MemCmd::MJL_DirAttribute::MJL_IsRow);
  MJL_countAccess(1, MemCmd::MJL_DirAttribute::MJL_IsColumn);
  MJL_countAccess(17, MemCmd::MJL_DirAttribute::MJL_IsColumn);
  MJL_countAccess(17, MemCmd::MJL_DirAttribute::MJL_IsColumn);
  MJL_countAccess(17, MemCmd::MJL_DirAttribute::MJL_IsRow);
  MJL_countAccess(1, MemCmd::MJL_DirAttribute::MJL_IsRow);
  
  MJL_printAccess();
}
