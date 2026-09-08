import std;

int main() {
    std::vector<int> values{1, 2, 3, 4};
    const int total = std::accumulate(values.begin(), values.end(), 0);
    std::println("{}", total);
    return total == 10 ? 0 : 1;
}
