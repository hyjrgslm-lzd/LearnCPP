#include <cstdlib>
#include <new>
#include <iostream>
void* operator new(std::size_t n) { if (void* p = std::malloc(n ? n : 1)) return p; throw std::bad_alloc(); }
void operator delete(void* p) noexcept { std::free(p); }
int main(){ int* p = new int(7); std::cout << *p << "\n"; delete p; }
