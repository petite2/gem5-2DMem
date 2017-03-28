#include <iostream>

class A{
    public:
    enum MJL_DirAttribute
    {
        // MJL_TODO: Check whether adding this would break things
        MJL_IsInvalid,  //!< Data access direction is invalid
        MJL_IsRow,      //!< Data access direction is row
        MJL_IsColumn,   //!< Data access direction is column
        MJL_NUM_COMMAND_DIRATTRIBUTES
    };
};

class B{
    public:
    typedef A::MJL_DirAttribute MJL_CacheBlkDir;
    MJL_CacheBlkDir MJL_blkDir;

};

template <class T_B>
class C{
    public:
    T_B b;
    //typedef enum T_B::MJL_CacheBlkDir MJL_Dir;
    void method1(enum T_B::MJL_CacheBlkDir dir) {b.MJL_blkDir = dir;}
};

class D{
    public:
    int d;
    void method1(B::MJL_CacheBlkDir dir) {if (dir == B::MJL_CacheBlkDir::MJL_IsRow) d = 10; else d = 20;}
};

int main() {
    B MJL_b;
    MJL_b.MJL_blkDir = B::MJL_CacheBlkDir::MJL_IsRow;
    std::cout << MJL_b.MJL_blkDir << "\n";
    MJL_b.MJL_blkDir = B::MJL_CacheBlkDir::MJL_IsColumn;
    std::cout << MJL_b.MJL_blkDir << "\n";
    std::cout << "\n";
    C<B> MJL_cB;
    MJL_cB.method1(A::MJL_IsColumn);
    std::cout << MJL_cB.b.MJL_blkDir << "\n";
    MJL_cB.method1(A::MJL_IsColumn);
    std::cout << MJL_cB.b.MJL_blkDir << "\n";
    std::cout << "\n";
    D MJL_d;
    MJL_d.method1(A::MJL_IsColumn);
    std::cout << MJL_d.d << "\n";
    MJL_d.method1(A::MJL_IsRow);
    std::cout << MJL_d.d << "\n";
    
    
    return 0;
}
