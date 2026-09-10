struct X { int n; int& get(this X& self) noexcept { return self.n; } const int& get(this const X& self) noexcept { return self.n; } }; int main(){ X x{1}; return x.get(); }
