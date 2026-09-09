#include <stdexec/execution.hpp>
#include <iostream>
#include <string>

namespace ex = stdexec;

// ── 三种轻量结果类型 ─────────────────────────────────
struct UserProfile {
    int         user_id;
    std::string name;
    int         level;
};

struct QuotaState {
    int user_id;
    int remaining;
    int total;
};

struct FeatureFlags {
    int  user_id;
    bool dark_mode;
    bool beta;
};

// ── 合流产物 ──────────────────────────────────────────
struct UserDashboard {
    UserProfile  profile;
    QuotaState   quota;
    FeatureFlags flags;
};

// ── 三条分支的函数（返回 sender） ────────────────────
// TODO [必做]: 实现 fetch_profile —— 从 just(user_id) 开始，
//   在 then 中构造 UserProfile 并返回。
//   示例返回值: UserProfile{user_id, "Alice", 42}
auto fetch_profile(int user_id) {
    return ex::just(user_id)
        | ex::then([](int uid) -> UserProfile {
            // TODO [必做]: 构造并返回 UserProfile
            return UserProfile{uid, "", 0};
        });
}

// TODO [必做]: 实现 fetch_quota —— 类似 fetch_profile
//   示例返回值: QuotaState{user_id, 75, 100}
auto fetch_quota(int user_id) {
    return ex::just(user_id)
        | ex::then([](int uid) -> QuotaState {
            // TODO [必做]: 构造并返回 QuotaState
            return QuotaState{uid, 0, 0};
        });
}

// TODO [必做]: 实现 fetch_flags —— 类似 fetch_profile
//   示例返回值: FeatureFlags{user_id, true, false}
auto fetch_flags(int user_id) {
    return ex::just(user_id)
        | ex::then([](int uid) -> FeatureFlags {
            // TODO [必做]: 构造并返回 FeatureFlags
            return FeatureFlags{uid, false, false};
        });
}

int main() {
    const int user_id = 1001;

    std::cout << "=== 扇出与合流 (user_id=" << user_id << ") ===\n\n";

    // ══════════════════════════════════════════════════════
    // TODO [必做]: 用 when_all 汇合三条分支，
    //   在最后的 then 中把三种结果拼成 UserDashboard，
    //   用 sync_wait 取出最终对象。
    //
    // auto pipeline = ex::when_all(
    //         fetch_profile(user_id),
    //         fetch_quota(user_id),
    //         fetch_flags(user_id)
    //     )
    //     | ex::then([](UserProfile profile, QuotaState quota, FeatureFlags flags)
    //                    -> UserDashboard {
    //         return UserDashboard{
    //             std::move(profile),
    //             std::move(quota),
    //             std::move(flags)
    //         };
    //     });
    //
    // auto [dashboard] = ex::sync_wait(std::move(pipeline)).value();
    // ══════════════════════════════════════════════════════

    // 临时占位，完成 TODO 后删除以下两行
    UserDashboard dashboard{
        UserProfile{user_id, "", 0},
        QuotaState{user_id, 0, 0},
        FeatureFlags{user_id, false, false}
    };

    // ── 打印最终对象 ──────────────────────────────────
    std::cout << "UserDashboard:\n";
    std::cout << "  profile : id=" << dashboard.profile.user_id
              << ", name=\"" << dashboard.profile.name
              << "\", level=" << dashboard.profile.level << "\n";
    std::cout << "  quota   : remaining=" << dashboard.quota.remaining
              << "/" << dashboard.quota.total << "\n";
    std::cout << "  flags   : dark_mode=" << dashboard.flags.dark_mode
              << ", beta=" << dashboard.flags.beta << "\n";

    // ══════════════════════════════════════════════════════
    // TODO [进阶]: 把其中一条分支改为返回 std::optional，在合流点做决策。
    // TODO [进阶]: 增加第四个分支 AuditInfo，观察合流点形状如何变化。
    // TODO [进阶]: 用 split 把某一条分支的 sender 结果同时供给两个下游，
    //   观察：如果不用 split，同一个 sender 能否被 connect 两次？
    // ══════════════════════════════════════════════════════

    return 0;
}
