#include <ranges>
#include <vector>
int main(){ std::vector<int> v{1}; auto e = std::views::enumerate(v); (void)e; }
