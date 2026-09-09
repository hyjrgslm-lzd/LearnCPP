#ifndef CONCURRENCY_STUDY_RCU_HPP
#define CONCURRENCY_STUDY_RCU_HPP
// C++23 教学版。共用分代核心；不是 N5050 标准实现。
// 独立域、同线程嵌套 RAII；注册信息由域拥有，无 TLS 裸指针。
#include "concurrency_study/epoch.hpp"
namespace cs {
using rcu_domain = epoch_domain;
inline rcu_domain& rcu_default_domain() noexcept { static rcu_domain d; return d; }
inline void rcu_synchronize(rcu_domain& d = rcu_default_domain()) { d.synchronize(); }
inline void rcu_barrier(rcu_domain& d = rcu_default_domain()) { d.barrier(); }
template<class T, class D = std::default_delete<T>>
void rcu_retire(T* p, D deleter = D{}, rcu_domain& d = rcu_default_domain()) noexcept {
    d.retire(p, std::move(deleter));
}
template<class T, class D = std::default_delete<T>>
class rcu_obj_base {
public:
    void retire(D deleter = D{}, rcu_domain& d = rcu_default_domain()) noexcept {
        d.retire(static_cast<T*>(this), std::move(deleter));
    }
protected:
    rcu_obj_base() = default;
    ~rcu_obj_base() = default;
};
class rcu_reader : public epoch_guard {
public:
    explicit rcu_reader(rcu_domain& d = rcu_default_domain()) : epoch_guard(d) {}
};
} // namespace cs
#endif
