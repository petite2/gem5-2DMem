#include <iostream>

void foo0(int n, int *a)
{
    for (int i = 0; i < n - 1; ++i) {
        if (i % 2 == 0) {
            a[i] = a[i+1];
        }
    }
}

int main() {
    int a[64];
    int n = 64;
    for (int i = 0; i < n; ++i) {
        a[i] = i;
    }
    foo0(n, a);
    for (int i = 0; i < n; ++i) {
        std::cout << a[i] << ", ";
    }
    std::cout << "\n";
    return 0;
}