#ifndef CS_HAS_STD_RCU
#define CS_HAS_STD_RCU 0
#endif

#include <iostream>

#if CS_HAS_STD_RCU
#include <mutex>
#include <rcu>
#endif

int main() try {
#if CS_HAS_STD_RCU
    auto& domain = std::rcu_default_domain();
    {
        std::scoped_lock<std::rcu_domain> reader(domain);
    }
    std::rcu_synchronize(domain);
    std::rcu_barrier(domain);
    std::cout << "F03 native std rcu OK\n";
    return 0;
#else
    std::cerr << "SKIP: CS_HAS_STD_RCU=0; native standard rcu unavailable\n";
    return 77;
#endif
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
