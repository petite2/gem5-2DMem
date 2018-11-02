// Example program
#include <iostream>
#include <string>
#include <cinttypes>
#include <vector>
#include <cassert>
#include <sstream>
#include <map>

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

class Stats {
    public:
        typedef uint64_t Scalar;
};

/**
 * Bloom filter to track the existance of row/column cache lines within a tile
 */
class MJL_RowColBloomFilter {
    public:
        std::string name;
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
            Addr hash_row = tileAddr >> floorLog2(MJL_rowWidth);
            Addr hash_col = tileAddr & ((1 << floorLog2(MJL_rowWidth)) - 1);
            switch(hash_func_id) {
                case 0: hash = tileAddr % size;
                    break;
                case 1: hash_row = hash_row >> 1;
                        hash_col = hash_col >> 1;
                        hash = ((hash_row & ((1 << (floorLog2(size) - floorLog2(size)/2)) - 1)) << floorLog2(size)/2) | (hash_col & ((1 << (floorLog2(size)/2)) - 1));
                        assert(hash < size);
                    break;
                case 2: hash_row = hash_row ^ (hash_row >> (floorLog2(size) - floorLog2(size)/2));
                        hash_col = hash_col ^ (hash_col >> floorLog2(size)/2);
                        hash = ((hash_row & ((1 << (floorLog2(size) - floorLog2(size)/2)) - 1)) << floorLog2(size)/2) | (hash_col & ((1 << (floorLog2(size)/2)) - 1));
                        assert(hash < size);
                    break;
                default: hash = tileAddr % size;
                    break;
            }
            return hash;
        }
        /** Obtain hash of the address */
        Addr addrHash(Addr addr) const {
            return tileAddrHash(getTileAddr(addr));
        }
    public:
        MJL_RowColBloomFilter(std::string in_name, unsigned in_size, unsigned in_cache_num_of_cachelines, unsigned in_MJL_rowWidth, unsigned in_blkSize, unsigned in_hash_func_id):name(in_name), size(in_size), cache_num_of_cachelines(in_cache_num_of_cachelines), MJL_rowWidth(in_MJL_rowWidth), blkSize(in_blkSize), hash_func_id(in_hash_func_id) {
            for (unsigned i = 0; i < size; ++i) {
                rol_col_BloomFilter.emplace_back(cache_num_of_cachelines);
            }
            MJL_bloomFilterFalsePositives = 0;
            MJL_bloomFilterTruePositives = 0;
            MJL_bloomFilterTrueNegatives = 0;
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
        Stats::Scalar MJL_bloomFilterFalsePositives;
        Stats::Scalar MJL_bloomFilterTruePositives;
        Stats::Scalar MJL_bloomFilterTrueNegatives;
        // void regStats() {
        //     MemObject::regStats();

        //     using namespace Stats;
        //     MJL_bloomFilterFalsePositives
        //         .name(name + ".MJL_bloomFilterFalsePositives")
        //         .desc("number of false positives in the bloom filter")
        //         .flags(nonan)
        //         ;
        //     MJL_bloomFilterTruePositives
        //         .name(name + ".MJL_bloomFilterTruePositives")
        //         .desc("number of true positives in the bloom filter")
        //         .flags(nonan)
        //         ;
        //     MJL_bloomFilterTrueNegatives
        //         .name(name + ".MJL_bloomFilterTrueNegatives")
        //         .desc("number of true negatives in the bloom filter")
        //         .flags(nonan)
        //         ;
        // }
        /** Test use */
        std::string print() const {
            std::ostringstream str;
            str << name << ": " << std::endl;
            for (unsigned i = 0; i < size; ++i) {
                if (rol_col_BloomFilter[i].hasCross(MemCmd::MJL_DirAttribute::MJL_IsRow) || rol_col_BloomFilter[i].hasCross(MemCmd::MJL_DirAttribute::MJL_IsColumn)) {
                    str << i << ": " << rol_col_BloomFilter[i].print() << std::endl;
                }
            }
            return str.str();
        }
};
/** Pointer to the Bloom Filter */
MJL_RowColBloomFilter * MJL_rowColBloomFilter;

/** For test use, explore the efficiency of multiple hash functions */
class MJL_Test_RowColBloomFilters {
    private:
        const unsigned cache_num_of_cachelines;
        const unsigned MJL_rowWidth;
        /** The block size of the parent cache. */
        const unsigned blkSize;
        std::map< unsigned, std::map<unsigned, MJL_RowColBloomFilter*> > test_row_col_BloomFilters;
        std::vector< unsigned > hash_func_ids;
        std::vector< unsigned > sizes;
    public:
        std::map< unsigned, std::map<unsigned, MJL_RowColBloomFilter*> > * getFilters() { return &test_row_col_BloomFilters; }
        MJL_Test_RowColBloomFilters(std::string in_name, unsigned in_cache_num_of_cachelines, unsigned in_MJL_rowWidth, unsigned in_blkSize, std::vector< unsigned > &in_hash_func_ids, std::vector< unsigned > &in_sizes):cache_num_of_cachelines(in_cache_num_of_cachelines), MJL_rowWidth(in_MJL_rowWidth), blkSize(in_blkSize), hash_func_ids(in_hash_func_ids), sizes(in_sizes) {
            for (auto hash_func_id : hash_func_ids) {
                for (auto size : sizes) {
                    test_row_col_BloomFilters[hash_func_id][size] = new MJL_RowColBloomFilter(in_name + ".MJL_Test_bloomfilter_" + std::to_string(hash_func_id) + "_" + std::to_string(size), size, cache_num_of_cachelines, MJL_rowWidth, blkSize, hash_func_id);
                }
            }
            std::cout << "MJL_TestBloomFilters_Configs:" << std::endl;
            std::cout << "hash_func_ids: ";
            for (auto hash_func_id : hash_func_ids) {
                std::cout << hash_func_id << " ";
            }
            std::cout << std::endl;
            std::cout << "sizes: ";
            for (auto size : sizes) {
                std::cout << size << " ";
            }
            std::cout << std::endl;
        }
        void test_add(Addr addr, MemCmd::MJL_DirAttribute blkDir) {
            for (auto hash_func_id : hash_func_ids) {
                for (auto size : sizes) {
                    test_row_col_BloomFilters[hash_func_id][size]->add(addr, blkDir);
                }
            }
        }
        void test_remove(Addr addr, MemCmd::MJL_DirAttribute blkDir) {
            for (auto hash_func_id : hash_func_ids) {
                for (auto size : sizes) {
                    test_row_col_BloomFilters[hash_func_id][size]->remove(addr, blkDir);
                }
            }
        }
        void test_hasCrossStatCountBloomFilters(Addr addr, MemCmd::MJL_DirAttribute blkDir, bool hasCross) {
            for (auto hash_func_id : hash_func_ids) {
                for (auto size : sizes) {
                    bool bloomFilterHasCross = test_row_col_BloomFilters[hash_func_id][size]->hasCross(addr, blkDir);
                    if (!bloomFilterHasCross) {
                        assert(!hasCross);
                        test_row_col_BloomFilters[hash_func_id][size]->MJL_bloomFilterTrueNegatives++;
                    }
                    if (!hasCross && bloomFilterHasCross) {
                        test_row_col_BloomFilters[hash_func_id][size]->MJL_bloomFilterFalsePositives++;
                    } else if (hasCross && bloomFilterHasCross) {
                        test_row_col_BloomFilters[hash_func_id][size]->MJL_bloomFilterTruePositives++;
                    }
                }
            }
        }
        /** For test use */
        std::string print() {
            std::ostringstream str;
            for (auto hash_func_id : hash_func_ids) {
                for (auto size : sizes) {
                    str << test_row_col_BloomFilters[hash_func_id][size]->print();
                }
            }
            return str.str();
        }
};
MJL_Test_RowColBloomFilters * MJL_Test_rowColBloomFilters;

void print_stats(MJL_RowColBloomFilter* filter) {
    std::cout << filter->name << ": f_pos " << filter->MJL_bloomFilterFalsePositives << ", t_pos " << filter->MJL_bloomFilterTruePositives << ", t_neg " << filter->MJL_bloomFilterTrueNegatives << std::endl;
}

void print_stats(MJL_Test_RowColBloomFilters * filters) {
    for (auto it = filters->getFilters()->begin(); it != filters->getFilters()->end(); ++it) {
        for (auto filter = (it->second).begin(); filter !=(it->second).end(); ++filter) {
            print_stats(filter->second);
        }
    }
}

int main()
{
    std::string name = "a";
    std::cout << "Hello, " << name << "!\n";
  
    unsigned in_size = 128;
    unsigned in_cache_num_of_cachelines = 4;
    unsigned in_MJL_rowWidth = 64;
    unsigned in_blkSize = 64;
    unsigned in_hash_func_id = 0;
    
    std::vector< unsigned > hash_func_ids;
    std::vector< unsigned > sizes;
    for (unsigned i = 0; i < 3; ++i) {
        hash_func_ids.emplace_back(i);
    }
    for (unsigned i = 1; i <= in_cache_num_of_cachelines; i = 2*i) {
        sizes.emplace_back(i);
    }
  
    MJL_rowColBloomFilter = new MJL_RowColBloomFilter("a", in_size, in_cache_num_of_cachelines, in_MJL_rowWidth, in_blkSize, in_hash_func_id);
    MJL_Test_rowColBloomFilters = new MJL_Test_RowColBloomFilters("a", in_cache_num_of_cachelines, in_MJL_rowWidth, in_blkSize, hash_func_ids, sizes);
    
    MJL_rowColBloomFilter->add(65, MemCmd::MJL_DirAttribute::MJL_IsRow);// row 0 col 1
    // MJL_rowColBloomFilter->add(131136, MemCmd::MJL_DirAttribute::MJL_IsRow);// row 4 col 1
    MJL_rowColBloomFilter->add(65, MemCmd::MJL_DirAttribute::MJL_IsColumn);// row 0 col 1
    MJL_rowColBloomFilter->add(98368, MemCmd::MJL_DirAttribute::MJL_IsRow);// row 3 col 1
    MJL_Test_rowColBloomFilters->test_add(65, MemCmd::MJL_DirAttribute::MJL_IsRow);// row 0 col 1
    // MJL_rowColBloomFilter->add(131136, MemCmd::MJL_DirAttribute::MJL_IsRow);// row 4 col 1
    MJL_Test_rowColBloomFilters->test_add(65, MemCmd::MJL_DirAttribute::MJL_IsColumn);// row 0 col 1
    MJL_Test_rowColBloomFilters->test_add(98368, MemCmd::MJL_DirAttribute::MJL_IsRow);// row 3 col 1
    std::cout << MJL_rowColBloomFilter->print() << std::endl;
    std::cout << MJL_Test_rowColBloomFilters->print() << std::endl;
    MJL_rowColBloomFilter->remove(65, MemCmd::MJL_DirAttribute::MJL_IsRow);
    MJL_Test_rowColBloomFilters->test_remove(65, MemCmd::MJL_DirAttribute::MJL_IsRow);
    std::cout << MJL_rowColBloomFilter->print() << std::endl;
    std::cout << MJL_Test_rowColBloomFilters->print() << std::endl;
    std::cout << "r,hasCross: " << MJL_rowColBloomFilter->hasCross(65600, MemCmd::MJL_DirAttribute::MJL_IsRow) << std::endl;// row 2 col 1
    std::cout << "c,hasCross: " << MJL_rowColBloomFilter->hasCross(65600, MemCmd::MJL_DirAttribute::MJL_IsColumn) << std::endl;// row 2 col 1    
    MJL_Test_rowColBloomFilters->test_hasCrossStatCountBloomFilters(65600, MemCmd::MJL_DirAttribute::MJL_IsRow, MJL_rowColBloomFilter->hasCross(65600, MemCmd::MJL_DirAttribute::MJL_IsRow));// row 2 col 1
    print_stats(MJL_Test_rowColBloomFilters);
    MJL_Test_rowColBloomFilters->test_hasCrossStatCountBloomFilters(65600, MemCmd::MJL_DirAttribute::MJL_IsColumn, MJL_rowColBloomFilter->hasCross(65600, MemCmd::MJL_DirAttribute::MJL_IsColumn));// row 2 col 1
    print_stats(MJL_Test_rowColBloomFilters);
  
    // for (unsigned i = 0; i < 2*in_cache_num_of_cachelines; ++i) {
    //     MJL_rowColBloomFilter->add(2*i*64*64*8+3*64, MemCmd::MJL_DirAttribute::MJL_IsColumn);// row 2*i col 3;
    //     std::cout << MJL_rowColBloomFilter->print() << std::endl;
    // }

    delete MJL_rowColBloomFilter;
  
}
