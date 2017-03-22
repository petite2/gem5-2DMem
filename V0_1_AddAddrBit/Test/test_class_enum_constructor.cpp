#include <iostream>
#include <bitset>

/*
    Used to understand the relationship between the MemCmd class and Packet class in src/mem/packet.hh
*/

class B;

class A {
    friend class B;
    public:
        enum enum1 {a, b, c, g};
        enum enum2 {d, e, f};
    //private:
        enum1 cheri1;
        enum2 cheri2;
        enum1 retg() {return g;}
        //A():cheri1(a), cheri2(d) {}
    public:
        A():cheri1(a),cheri2(f) {std::cout << "A():cheri1(a),cheri2(f)\n";}
        A operator=(enum1 in) { this->cheri1 = in;std::cout << "A operator=(enum1 in)\n"; return *this; }//,cheri2(d) {}
        A(enum1 in): cheri1(in), cheri2(e) {std::cout << "A(enum1 in): cheri1(in), cheri2(e)\n";}
        ~A() {}
};

class B {
    public:
        typedef A::enum1 enum1;
        A jetaime;
        void cheri() {jetaime = A::b;}
        static A cherie() { return A::c; }
        B() { std::cout << "B()\n";}
        B(A _jetaime): jetaime(_jetaime) {std::cout << "B(A _jetaime): jetaime(_jetaime)\n";}
        ~B() {}
        static B* create() { return new B(cherie()); }
        //void cherie() {jetaime = A::e}
};
 
int main()
{
    std::cout << "A::c --> " << A::c << "\n";
    std::cout << "\n";
    B cherie;
    std::cout << "B cherie --> " << cherie.jetaime.cheri1 << "," << cherie.jetaime.cheri2 << "\n";
    std::cout << "\n";
    cherie.cheri();
    std::cout << "cherie.cheri() {jetaime = A::b;} --> " << cherie.jetaime.cheri1 << "," << cherie.jetaime.cheri2 << "\n";
    std::cout << "\n";
    B cherie1(cherie.cherie());
    std::cout << "B cherie1(cherie.cherie()) A cherie() { return A::c; } --> " << cherie1.jetaime.cheri1 << "," << cherie1.jetaime.cheri2 << "\n";
    std::cout << "\n";
    B cherie2 = *(B::create());
    std::cout << "B* cherie2 = B::create() { return new B(cherie()); } --> " << cherie2.jetaime.cheri1 << "," << cherie2.jetaime.cheri2 << "\n";
    std::cout << "\n";
    A jetaime;
    jetaime = cherie.jetaime.retg();
    std::cout << "jetaime = cherie.jetaime.retg() --> " << jetaime.cheri1 << "," << jetaime.cheri2 << "\n";
    
    return 0;
    
}