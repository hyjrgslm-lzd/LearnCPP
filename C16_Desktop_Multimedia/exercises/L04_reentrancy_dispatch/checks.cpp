#include <solution.hpp>
#include <c16/check.hpp>

#include <QCoreApplication>
#include <QStringList>

namespace {
void check_fifo()
{
    c16_l04::DispatchQueue queue;
    QStringList seen;
    queue.post("a", [&] { seen << "a"; });
    queue.post("b", [&] { seen << "b"; });
    queue.drain();
    c16::require(seen == QStringList({"a", "b"}), "dispatch preserves FIFO order");
}

void check_reentrant_drain_is_deferred()
{
    c16_l04::DispatchQueue queue;
    QStringList seen;
    queue.post("outer", [&] {
        seen << "outer";
        queue.post("inner", [&] { seen << "inner"; });
        queue.drain();
        seen << "after-nested-drain";
    });
    queue.drain();
    c16::require(seen == QStringList({"outer", "after-nested-drain", "inner"}),
        "nested drain cannot reenter the active handler");
}

void check_clear_removes_pending()
{
    c16_l04::DispatchQueue queue;
    QStringList seen;
    queue.post("a", [&] { seen << "a"; queue.clear(); });
    queue.post("b", [&] { seen << "b"; });
    queue.drain();
    c16::require(seen == QStringList({"a"}), "clear removes pending events without aborting active event");
}
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    return c16::run([] {
        check_fifo();
        check_reentrant_drain_is_deferred();
        check_clear_removes_pending();
    });
}
