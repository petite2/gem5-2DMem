// Example program
#include <iostream>
#include <string>
#include <cinttypes>
#include <vector>
#include <cassert>
#include <sstream>

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

/**
    * Bloom filter to track the existance of row/column cache lines within a tile
    */
class MJL_RowColBloomFilter {
    protected: 
        /** Entry of the bloom filter */
        class MJL_RowColBloomFilterEntry {
            private:
                bool row_exist;
                bool col_exist;
                int row_count;
                int col_count;
                const int cache_num_of_cachelines;
                bool hasRow() const { assert((row_count != 0) == row_exist) ; return row_exist; }
                bool hasCol() const { assert((col_count != 0) == col_exist); return col_exist; }
            public:
                MJL_RowColBloomFilterEntry(int in_cache_num_of_cachelines) : 
                    row_exist(false), col_exist(false), row_count(0), col_count(0), cache_num_of_cachelines(in_cache_num_of_cachelines) {}
                ~MJL_RowColBloomFilterEntry() {}
                bool hasCross(MemCmd::MJL_DirAttribute blkDir) const {
                    bool cross_exist = false;
                    if (blkDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                        cross_exist = hasCol();
                    } else if (blkDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                        cross_exist = hasRow();
                    }
                    return cross_exist;
                }
                /** Increase the number of row/col cache lines in the entry */
                void countBlkAlloc(MemCmd::MJL_DirAttribute blkDir) {
                    assert(row_count + col_count < cache_num_of_cachelines);
                    if (blkDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                        row_count++;
                        row_exist |= true;
                    } else if (blkDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                        col_count++;
                        col_exist |= true;
                    }
                }
                /** Decrease the number of row/col cache lines in the entry */
                void countBlkEvict(MemCmd::MJL_DirAttribute blkDir) {
                    if (blkDir == MemCmd::MJL_DirAttribute::MJL_IsRow) {
                        assert(row_count > 0);
                        row_count--;
                        if (row_count == 0) {
                            row_exist &= false;
                        }
                    } else if (blkDir == MemCmd::MJL_DirAttribute::MJL_IsColumn) {
                        assert(col_count > 0);
                        col_count--;
                        if (col_count == 0) {
                            col_exist &= false;
                        }
                    }
                }
                /** Test use */
                std::string print() const {
                    std::ostringstream str;
                    str << "r_ex " << row_exist << ", c_ex " << col_exist << ", r_cnt " << row_count << ", c_cnt " << col_count;
                    return str.str();
                }
        };
        const unsigned size;
        const unsigned cache_num_of_cachelines;
        const unsigned MJL_rowWidth;
        /** The block size of the parent cache. */
        const unsigned blkSize;
        /** 
            * Which function to use for the address hash
            * 0 and default: tileAddr % size
            */
        const unsigned hash_func_id;
        std::vector<MJL_RowColBloomFilterEntry> rol_col_BloomFilter;
        int floorLog2(unsigned x) const {
            assert(x > 0);

            int y = 0;

            if (x & 0xffff0000) { y += 16; x >>= 16; }
            if (x & 0x0000ff00) { y +=  8; x >>=  8; }
            if (x & 0x000000f0) { y +=  4; x >>=  4; }
            if (x & 0x0000000c) { y +=  2; x >>=  2; }
            if (x & 0x00000002) { y +=  1; }

            return y;
        }
        /** Obtain the tile address from address */
        Addr getTileAddr(Addr addr) const {
            int MJL_wordShift = floorLog2(sizeof(uint64_t));
            int MJL_lineShift = floorLog2(blkSize);
            int MJL_colShift = floorLog2(MJL_rowWidth) + floorLog2(blkSize);
            int MJL_colTileShift = MJL_colShift + MJL_lineShift - MJL_wordShift;
            uint64_t MJL_rowMask = (1 << MJL_colShift) -1;

            Addr tile_row_Id = addr >> MJL_colTileShift;
            Addr tile_col_Id = (addr & MJL_rowMask) >> MJL_lineShift;

            return (tile_row_Id << (MJL_colShift - MJL_lineShift)) | tile_col_Id;
        }
        /** Obtain hash of the tile address */
        Addr tileAddrHash(Addr tileAddr) const {
            Addr hash = tileAddr;
            switch(hash_func_id) {
                case 0: hash = tileAddr % size;
                    break;
                default: hash = tileAddr % size;
                    break;
            }
            return hash;
        }
        /** Obtain hash of the address */
        Addr addrHash(Addr addr) const {
            std::cout << addr << "-->" << getTileAddr(addr) << ", " << tileAddrHash(getTileAddr(addr)) << std::endl;
            return tileAddrHash(getTileAddr(addr));
        }
    public:
        MJL_RowColBloomFilter(unsigned in_size, unsigned in_cache_num_of_cachelines, unsigned in_MJL_rowWidth, unsigned in_blkSize, unsigned in_hash_func_id):size(in_size), cache_num_of_cachelines(in_cache_num_of_cachelines), MJL_rowWidth(in_MJL_rowWidth), blkSize(in_blkSize), hash_func_id(in_hash_func_id) {
            for (unsigned i = 0; i < size; ++i) {
                rol_col_BloomFilter.emplace_back(cache_num_of_cachelines);
            }
        }
        ~MJL_RowColBloomFilter() {}
        /** Add the cacheline to the bloom filter */
        void add(Addr addr, MemCmd::MJL_DirAttribute blkDir) {
            rol_col_BloomFilter[addrHash(addr)].countBlkAlloc(blkDir);
        }
        /** Remove the cacheline from the bloom filter */
        void remove(Addr addr, MemCmd::MJL_DirAttribute blkDir) {
            rol_col_BloomFilter[addrHash(addr)].countBlkEvict(blkDir);
        }
        /** Check in the bloom filter if there may exist crossing cachelines in the cache */
        bool hasCross(Addr addr, MemCmd::MJL_DirAttribute blkDir) const {
            return rol_col_BloomFilter[addrHash(addr)].hasCross(blkDir);
        }
        /** Test use */
        std::string print() const {
            std::ostringstream str;
            for (unsigned i = 0; i < size; ++i) {
                if (rol_col_BloomFilter[i].hasCross(MemCmd::MJL_DirAttribute::MJL_IsRow) || rol_col_BloomFilter[i].hasCross(MemCmd::MJL_DirAttribute::MJL_IsColumn)) {
                    str << i << ": " << rol_col_BloomFilter[i].print() << std::endl;
                }
            }
            return str.str();
        }
};
MJL_RowColBloomFilter * MJL_rowColBloomFilter;

int main()
{
    std::string name = "a";
    std::cout << "Hello, " << name << "!\n";
  
    unsigned in_size = 128;
    unsigned in_cache_num_of_cachelines = 4;
    unsigned in_MJL_rowWidth = 64;
    unsigned in_blkSize = 64;
    unsigned in_hash_func_id = 0;
  
    MJL_rowColBloomFilter = new MJL_RowColBloomFilter(in_size, in_cache_num_of_cachelines, in_MJL_rowWidth, in_blkSize, in_hash_func_id);
    
    MJL_rowColBloomFilter->add(65, MemCmd::MJL_DirAttribute::MJL_IsRow);// row 0 col 1
    // MJL_rowColBloomFilter->add(131136, MemCmd::MJL_DirAttribute::MJL_IsRow);// row 4 col 1
    MJL_rowColBloomFilter->add(65, MemCmd::MJL_DirAttribute::MJL_IsColumn);// row 0 col 1
    MJL_rowColBloomFilter->add(98368, MemCmd::MJL_DirAttribute::MJL_IsRow);// row 3 col 1
    std::cout << MJL_rowColBloomFilter->print() << std::endl;
    MJL_rowColBloomFilter->remove(65, MemCmd::MJL_DirAttribute::MJL_IsRow);
    std::cout << MJL_rowColBloomFilter->print() << std::endl;
    std::cout << "r,hasCross: " << MJL_rowColBloomFilter->hasCross(65600, MemCmd::MJL_DirAttribute::MJL_IsRow) << std::endl;// row 2 col 1
    std::cout << "c,hasCross: " << MJL_rowColBloomFilter->hasCross(65600, MemCmd::MJL_DirAttribute::MJL_IsColumn) << std::endl;// row 2 col 1
  
    for (unsigned i = 0; i < 2*in_cache_num_of_cachelines; ++i) {
        MJL_rowColBloomFilter->add(2*i*64*64*8+3*64, MemCmd::MJL_DirAttribute::MJL_IsColumn);// row 2*i col 3;
        std::cout << MJL_rowColBloomFilter->print() << std::endl;
    }

    delete MJL_rowColBloomFilter;
  
}
