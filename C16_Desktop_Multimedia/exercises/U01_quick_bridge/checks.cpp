#include <solution.hpp>

#include <c16/check.hpp>

#include <QCoreApplication>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    return c16::run([] {
        u01::BridgeState state;
        state.add("a.wav");
        state.add("b.avi");
        c16::require(state.rowCount() == 2, "bridge exposes shared model row count");
        state.setFilter("wav");
        c16::require(state.rowCount() == 1, "filter notification changes visible rows");
        c16::require(state.select(0), "selection through visible row works");
        c16::require(state.selectedName() == "a.wav", "selected name updates binding source");
        state.setFilter({});
        c16::require(state.rowCount() == 2, "clearing filter restores rows");
    });
}
