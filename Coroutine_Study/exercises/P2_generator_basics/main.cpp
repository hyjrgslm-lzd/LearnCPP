// 对应讲义：00-预备知识-执行模型与标准库.md，练习 P-2。
#include <generator>
#include <iostream>
#include <vector>

struct local_lifetime {
    ~local_lifetime() { std::cout << "  producer local destroyed\n"; }
};

std::generator<int> numbers() {
    std::cout << "  producer entered\n";
    local_lifetime local;
    // TODO P2-1：依次产出 1、2、3，比较每条日志出现的时刻。
    for (int value : {1, 2, 3}) {
        std::cout << "  before yield " << value << '\n';
        co_yield value;
        std::cout << "  after yield " << value << '\n';
    }
}

int main() {
    {
        std::cout << "P2/1 construct\n";
        auto sequence = numbers();
        std::cout << "P2/1 begin\n";
        auto it = sequence.begin();
        std::cout << "read=" << *it << " read_again=" << *it << '\n';
        // TODO P2-1：逐次执行 ++it、读取当前值，最后与 sequence.end() 比较。
        ++it;
        std::cout << "read=" << *it << '\n';
        ++it;
        std::cout << "read=" << *it << '\n';
        ++it;
        std::cout << "it==end? " << (it == sequence.end()) << '\n';
    }
    {
        std::cout << "P2/2 early exit\n";
        auto sequence = numbers();
        auto it = sequence.begin();
        std::cout << "read=" << *it << '\n';
        // TODO P2-2：预测离开作用域时 after yield 和析构日志各会出现几次。
    }
    {
        auto sequence = numbers();
        std::vector<int> saved;
        for (int value : sequence) 
            saved.push_back(value);
        // TODO P2-3：遍历 saved 两次，比较结果与生产者日志次数。
        std::cout << "P2/3 saved_size=" << saved.size() << '\n';
    }
}
