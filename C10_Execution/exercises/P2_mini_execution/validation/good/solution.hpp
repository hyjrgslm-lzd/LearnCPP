#pragma once

#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <stdexec/execution.hpp>

namespace c10_p2::mini {

struct empty_env {};

struct connect_t {
  template <class S, class R>
  auto operator()(S &&s, R &&r) const
      noexcept(noexcept(std::forward<S>(s).connect(std::forward<R>(r)))) {
    return std::forward<S>(s).connect(std::forward<R>(r));
  }
};
struct start_t {
  template <class Op> void operator()(Op &op) const noexcept(noexcept(op.start())) { op.start(); }
};
struct set_value_t {
  template <class R, class... As>
  void operator()(R &&r, As &&...as) const
      noexcept(noexcept(std::forward<R>(r).set_value(std::forward<As>(as)...))) {
    std::forward<R>(r).set_value(std::forward<As>(as)...);
  }
};
struct set_error_t {
  template <class R, class E>
  void operator()(R &&r, E &&e) const
      noexcept(noexcept(std::forward<R>(r).set_error(std::forward<E>(e)))) {
    std::forward<R>(r).set_error(std::forward<E>(e));
  }
};
struct set_stopped_t {
  template <class R>
  void operator()(R &&r) const noexcept(noexcept(std::forward<R>(r).set_stopped())) {
    std::forward<R>(r).set_stopped();
  }
};
struct get_env_t {
  template <class R> auto operator()(const R &r) const noexcept {
    if constexpr (requires { r.get_env(); }) {
      return r.get_env();
    } else {
      return empty_env{};
    }
  }
};
struct schedule_t {
  template <class Sch> auto operator()(Sch sch) const noexcept(noexcept(sch.schedule())) {
    return sch.schedule();
  }
};
struct get_allocator_t {
  template <class Env> auto operator()(const Env &env) const noexcept {
    if constexpr (requires { env.get_allocator(); }) {
      return env.get_allocator();
    } else {
      return 0;
    }
  }
};

inline constexpr connect_t connect{};
inline constexpr start_t start{};
inline constexpr set_value_t set_value{};
inline constexpr set_error_t set_error{};
inline constexpr set_stopped_t set_stopped{};
inline constexpr get_env_t get_env{};
inline constexpr schedule_t schedule{};
inline constexpr get_allocator_t get_allocator{};

template <class... Sigs> struct completion_signatures {};

template <class S, class Env = empty_env>
using completion_signatures_of_t =
    typename std::remove_cvref_t<S>::template completion_signatures<Env>;

namespace meta {
struct skip {};

template <class... Sigs> struct first_value {
  using type = std::tuple<>;
};
template <class... Vs, class... Rest> struct first_value<set_value_t(Vs...), Rest...> {
  using type = std::tuple<Vs...>;
};
template <class Sig, class... Rest> struct first_value<Sig, Rest...> : first_value<Rest...> {};
template <class C> struct first_value_of;
template <class... Sigs>
struct first_value_of<completion_signatures<Sigs...>> : first_value<Sigs...> {};

template <class Sig> struct value_alt {
  using type = skip;
};
template <class... Vs> struct value_alt<set_value_t(Vs...)> {
  using type = std::tuple<Vs...>;
};
template <class Sig> struct error_alt {
  using type = skip;
};
template <class E> struct error_alt<set_error_t(E)> {
  using type = E;
};

template <class Bag, class... Ts> struct keep;
template <class... Kept> struct keep<std::tuple<Kept...>> {
  using type = std::variant<Kept...>;
};
template <> struct keep<std::tuple<>> {
  using type = std::variant<std::monostate>;
};
template <class... Kept, class T, class... Rest>
struct keep<std::tuple<Kept...>, T, Rest...> : keep<std::tuple<Kept...>, Rest...> {};
template <class... Kept, class T, class... Rest>
  requires(!std::same_as<T, skip>)
struct keep<std::tuple<Kept...>, T, Rest...> : keep<std::tuple<Kept..., T>, Rest...> {};

template <class C> struct values;
template <class... Sigs>
struct values<completion_signatures<Sigs...>>
    : keep<std::tuple<>, typename value_alt<Sigs>::type...> {};
template <class C> struct errors;
template <class... Sigs>
struct errors<completion_signatures<Sigs...>>
    : keep<std::tuple<>, typename error_alt<Sigs>::type...> {};

template <class R> struct then_value {
  using type = completion_signatures<set_value_t(R)>;
};
template <> struct then_value<void> {
  using type = completion_signatures<set_value_t()>;
};
template <class F, class Sig> struct then_one {
  using type = completion_signatures<Sig>;
};
template <class F, class... Vs>
struct then_one<F, set_value_t(Vs...)> : then_value<std::invoke_result_t<F, Vs...>> {};

template <class... Cs> struct concat;
template <> struct concat<> {
  using type = completion_signatures<>;
};
template <class... Sigs> struct concat<completion_signatures<Sigs...>> {
  using type = completion_signatures<Sigs...>;
};
template <class... A, class... B, class... Rest>
struct concat<completion_signatures<A...>, completion_signatures<B...>, Rest...>
    : concat<completion_signatures<A..., B...>, Rest...> {};
template <class F, class C> struct then_all;
template <class F, class... Sigs>
struct then_all<F, completion_signatures<Sigs...>>
    : concat<typename then_one<F, Sigs>::type...,
             completion_signatures<set_error_t(std::exception_ptr)>> {};

template <class A, class B> struct join_tuple;
template <class... A, class... B> struct join_tuple<std::tuple<A...>, std::tuple<B...>> {
  using sig = set_value_t(A..., B...);
};
} // namespace meta

template <class S, class Env = empty_env>
using value_types_of_t = typename meta::values<completion_signatures_of_t<S, Env>>::type;
template <class S, class Env = empty_env>
using error_types_of_t = typename meta::errors<completion_signatures_of_t<S, Env>>::type;

template <class R>
concept receiver = requires(R r, std::exception_ptr e) {
  mini::set_value(std::move(r));
  mini::set_error(std::move(r), e);
  mini::set_stopped(std::move(r));
};
template <class S>
concept sender = requires { typename completion_signatures_of_t<S>; };
template <class Op>
concept operation_state = requires(Op &op) { mini::start(op); } && (!std::move_constructible<Op>);

template <class... Ts> struct value_sender {
  std::tuple<Ts...> data;

  template <class Env>
  using completion_signatures = mini::completion_signatures<set_value_t(Ts...)>;

  template <class R> struct state {
    std::tuple<Ts...> data;
    R out;

    state(std::tuple<Ts...> d, R r) : data(std::move(d)), out(std::move(r)) {}
    state(const state &) = delete;
    state(state &&) = delete;
    auto operator=(const state &) -> state & = delete;
    auto operator=(state &&) -> state & = delete;

    void start() noexcept {
      auto values = std::move(data);
      auto receiver = std::move(out);
      std::apply([&](auto &&...xs) { mini::set_value(std::move(receiver), std::move(xs)...); },
                 std::move(values));
    }
  };

  template <class R> auto connect(R r) && {
    return state<std::remove_cvref_t<R>>{std::move(data), std::move(r)};
  }
};

template <class... Ts> auto just(Ts &&...xs) {
  return value_sender<std::remove_cvref_t<Ts>...>{
      std::tuple<std::remove_cvref_t<Ts>...>{std::forward<Ts>(xs)...}};
}

struct error_sender {
  std::exception_ptr error;

  template <class Env>
  using completion_signatures = mini::completion_signatures<set_error_t(std::exception_ptr)>;

  template <class R> struct state {
    std::exception_ptr error;
    R out;

    state(std::exception_ptr e, R r) : error(std::move(e)), out(std::move(r)) {}
    state(const state &) = delete;
    state(state &&) = delete;
    auto operator=(const state &) -> state & = delete;
    auto operator=(state &&) -> state & = delete;
    void start() noexcept { mini::set_error(std::move(out), std::move(error)); }
  };

  template <class R> auto connect(R r) && {
    return state<std::remove_cvref_t<R>>{std::move(error), std::move(r)};
  }
};
inline auto just_error(std::exception_ptr e) { return error_sender{std::move(e)}; }

struct stopped_sender {
  template <class Env> using completion_signatures = mini::completion_signatures<set_stopped_t()>;

  template <class R> struct state {
    R out;

    explicit state(R r) : out(std::move(r)) {}
    state(const state &) = delete;
    state(state &&) = delete;
    auto operator=(const state &) -> state & = delete;
    auto operator=(state &&) -> state & = delete;
    void start() noexcept { mini::set_stopped(std::move(out)); }
  };

  template <class R> auto connect(R r) const { return state<std::remove_cvref_t<R>>{std::move(r)}; }
};
inline auto just_stopped() { return stopped_sender{}; }

template <class R, class F> struct map_receiver {
  R out;
  F fn;

  template <class... Vs> void set_value(Vs &&...vs) noexcept {
    try {
      if constexpr (std::is_void_v<std::invoke_result_t<F, Vs...>>) {
        std::invoke(std::move(fn), std::forward<Vs>(vs)...);
        mini::set_value(std::move(out));
      } else {
        mini::set_value(std::move(out), std::invoke(std::move(fn), std::forward<Vs>(vs)...));
      }
    } catch (...) {
      mini::set_error(std::move(out), std::current_exception());
    }
  }
  void set_error(std::exception_ptr e) noexcept { mini::set_error(std::move(out), std::move(e)); }
  void set_stopped() noexcept { mini::set_stopped(std::move(out)); }
  auto get_env() const noexcept -> decltype(mini::get_env(out)) { return mini::get_env(out); }
};

template <class S, class F> struct transform_sender {
  S source;
  F fn;

  template <class Env>
  using completion_signatures =
      typename meta::then_all<F, completion_signatures_of_t<S, Env>>::type;

  template <class R> struct state {
    using inner_receiver = map_receiver<std::remove_cvref_t<R>, F>;
    using inner_state = decltype(mini::connect(std::declval<S>(), std::declval<inner_receiver>()));
    inner_state inner;

    state(S s, R r, F f)
        : inner(mini::connect(std::move(s), inner_receiver{std::move(r), std::move(f)})) {}
    state(const state &) = delete;
    state(state &&) = delete;
    auto operator=(const state &) -> state & = delete;
    auto operator=(state &&) -> state & = delete;
    void start() noexcept { mini::start(inner); }
  };

  template <class R> auto connect(R r) && {
    return state<std::remove_cvref_t<R>>{std::move(source), std::move(r), std::move(fn)};
  }
};

template <class S, class F> auto then(S &&s, F f) {
  return transform_sender<std::remove_cvref_t<S>, std::remove_cvref_t<F>>{std::forward<S>(s),
                                                                          std::move(f)};
}

template <class F> struct then_pipe {
  F fn;
  template <class S> auto operator()(S &&s) && {
    return mini::then(std::forward<S>(s), std::move(fn));
  }
};
template <class F> auto then(F f) { return then_pipe<std::remove_cvref_t<F>>{std::move(f)}; }
template <class S, class A>
auto operator|(S &&s, A &&a) -> decltype(std::forward<A>(a)(std::forward<S>(s))) {
  return std::forward<A>(a)(std::forward<S>(s));
}

template <class S>
using wait_tuple_t = typename meta::first_value_of<completion_signatures_of_t<S>>::type;

template <class T> struct wait_box {
  std::mutex m;
  std::condition_variable cv;
  std::optional<T> value;
  std::exception_ptr error;
  bool done = false;
};

template <class T> struct wait_receiver {
  wait_box<T> *box;

  template <class... Vs> void set_value(Vs &&...vs) noexcept {
    std::lock_guard lk(box->m);
    box->value.emplace(std::forward<Vs>(vs)...);
    box->done = true;
    box->cv.notify_one();
  }
  void set_error(std::exception_ptr e) noexcept {
    std::lock_guard lk(box->m);
    box->error = std::move(e);
    box->done = true;
    box->cv.notify_one();
  }
  void set_stopped() noexcept {
    std::lock_guard lk(box->m);
    box->done = true;
    box->cv.notify_one();
  }
};

template <class S> auto sync_wait(S s) -> std::optional<wait_tuple_t<S>> {
  using result_t = wait_tuple_t<S>;
  wait_box<result_t> box;
  auto op = mini::connect(std::move(s), wait_receiver<result_t>{&box});
  mini::start(op);
  {
    std::unique_lock lk(box.m);
    box.cv.wait(lk, [&] { return box.done; });
  }
  if (box.error) {
    std::rethrow_exception(box.error);
  }
  return std::move(box.value);
}

template <class L, class R> struct both_sender {
  L left;
  R right;

  template <class Env>
  using completion_signatures =
      mini::completion_signatures<typename meta::join_tuple<wait_tuple_t<L>, wait_tuple_t<R>>::sig,
                                  set_error_t(std::exception_ptr), set_stopped_t()>;

  template <class Out> struct state {
    using left_tuple = wait_tuple_t<L>;
    using right_tuple = wait_tuple_t<R>;

    struct shared {
      explicit shared(Out out) : out(std::move(out)) {}
      std::mutex m;
      Out out;
      std::optional<left_tuple> l;
      std::optional<right_tuple> r;
      std::exception_ptr error;
      bool stopped = false;
      int count = 0;

      void arrive() noexcept {
        std::unique_lock lk(m);
        if (++count < 2) {
          return;
        }
        auto out_now = std::move(out);
        auto l_now = std::move(l);
        auto r_now = std::move(r);
        auto e_now = std::move(error);
        auto stopped_now = stopped;
        lk.unlock();
        if (e_now) {
          mini::set_error(std::move(out_now), std::move(e_now));
        } else if (stopped_now || !l_now || !r_now) {
          mini::set_stopped(std::move(out_now));
        } else {
          std::apply(
              [&](auto &&...a) {
                std::apply(
                    [&](auto &&...b) {
                      mini::set_value(std::move(out_now), std::move(a)..., std::move(b)...);
                    },
                    std::move(*r_now));
              },
              std::move(*l_now));
        }
      }
    };

    template <bool IsLeft> struct child {
      std::shared_ptr<shared> p;

      template <class... Vs> void set_value(Vs &&...vs) noexcept {
        {
          std::lock_guard lk(p->m);
          if constexpr (IsLeft) {
            p->l.emplace(std::forward<Vs>(vs)...);
          } else {
            p->r.emplace(std::forward<Vs>(vs)...);
          }
        }
        p->arrive();
      }
      void set_error(std::exception_ptr e) noexcept {
        {
          std::lock_guard lk(p->m);
          if (!p->error) {
            p->error = std::move(e);
          }
        }
        p->arrive();
      }
      void set_stopped() noexcept {
        {
          std::lock_guard lk(p->m);
          p->stopped = true;
        }
        p->arrive();
      }
    };

    using left_state = decltype(mini::connect(std::declval<L>(), std::declval<child<true>>()));
    using right_state = decltype(mini::connect(std::declval<R>(), std::declval<child<false>>()));

    std::shared_ptr<shared> p;
    left_state ls;
    right_state rs;

    state(L l, R r, Out out)
        : p(std::make_shared<shared>(std::move(out))),
          ls(mini::connect(std::move(l), child<true>{p})),
          rs(mini::connect(std::move(r), child<false>{p})) {}
    state(const state &) = delete;
    state(state &&) = delete;
    auto operator=(const state &) -> state & = delete;
    auto operator=(state &&) -> state & = delete;
    void start() noexcept {
      mini::start(ls);
      mini::start(rs);
    }
  };

  template <class Out> auto connect(Out out) && {
    return state<std::remove_cvref_t<Out>>{std::move(left), std::move(right), std::move(out)};
  }
};

template <class L, class R> auto when_all(L &&l, R &&r) {
  return both_sender<std::remove_cvref_t<L>, std::remove_cvref_t<R>>{std::forward<L>(l),
                                                                     std::forward<R>(r)};
}

class run_loop {
  struct item {
    virtual void fire() noexcept = 0;
    virtual ~item() = default;
  };

  std::mutex m_;
  std::condition_variable cv_;
  std::deque<item *> work_;
  bool closed_ = false;

  enum class post_status { posted, closed, failed };

  auto post(item *p, std::exception_ptr &error) noexcept -> post_status {
    try {
      {
        std::lock_guard lk(m_);
        if (closed_) {
          return post_status::closed;
        }
        work_.push_back(p);
      }
      cv_.notify_one();
      return post_status::posted;
    } catch (...) {
      error = std::current_exception();
      return post_status::failed;
    }
  }

public:
  void close() noexcept {
    {
      std::lock_guard lk(m_);
      closed_ = true;
    }
    cv_.notify_all();
  }

  void run() noexcept {
    for (;;) {
      item *next = nullptr;
      {
        std::unique_lock lk(m_);
        cv_.wait(lk, [&] { return closed_ || !work_.empty(); });
        if (work_.empty()) {
          return;
        }
        next = work_.front();
        work_.pop_front();
      }
      next->fire();
    }
  }

  struct scheduler {
    run_loop *loop;

    struct sender {
      run_loop *loop;

      template <class Env>
      using completion_signatures =
          mini::completion_signatures<set_value_t(), set_error_t(std::exception_ptr)>;

      template <class R> struct state : item {
        run_loop *loop;
        R out;

        state(run_loop *l, R r) : loop(l), out(std::move(r)) {}
        state(const state &) = delete;
        state(state &&) = delete;
        auto operator=(const state &) -> state & = delete;
        auto operator=(state &&) -> state & = delete;

        void start() noexcept {
          std::exception_ptr error;
          switch (loop->post(this, error)) {
          case post_status::posted:
            return;
          case post_status::closed:
            mini::set_error(std::move(out), std::make_exception_ptr(std::runtime_error("closed")));
            return;
          case post_status::failed:
            mini::set_error(std::move(out), std::move(error));
            return;
          }
        }
        void fire() noexcept override { mini::set_value(std::move(out)); }
      };

      template <class R> auto connect(R r) && {
        return state<std::remove_cvref_t<R>>{loop, std::move(r)};
      }
    };

    auto schedule() const noexcept { return sender{loop}; }
  };

  auto get_scheduler() noexcept { return scheduler{this}; }
  auto schedule() noexcept { return get_scheduler().schedule(); }
};

inline auto as_stdexec(value_sender<> s) {
  (void)s;
  return stdexec::just();
}
template <class T> inline auto as_stdexec(value_sender<T> s) {
  return std::apply([](auto v) { return stdexec::just(std::move(v)); }, std::move(s.data));
}

} // namespace c10_p2::mini
