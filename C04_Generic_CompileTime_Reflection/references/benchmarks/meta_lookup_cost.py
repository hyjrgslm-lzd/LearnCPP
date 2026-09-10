"""Generate B01 meta-map lookup benchmark translation units."""
from __future__ import annotations

from pathlib import Path
import re
import textwrap


EXPECTED_MP11_COMMIT = "b94b089d4ec83cd397f20958f34edf25bc3e06f4"
MP11_DEP_PREFIX = Path("exercises/build/_deps/mp11-boost-1.91.0")
COUNTS = (32, 128, 256)
KINDS = ("manual", "mp11")
TARGETS = ("last", "missing")


def mp11_marker_path(course: Path) -> Path:
    return course / MP11_DEP_PREFIX / ".learncpp-dependency.cmake"


def read_dependency_marker(course: Path) -> dict[str, str]:
    marker = mp11_marker_path(course)
    if not marker.exists():
        return {}
    text = marker.read_text(encoding="utf-8")
    return dict(re.findall(r'set\((LEARNCPP_DEP_[A-Z0-9_]+)\s+"([^"]*)"\)', text))


def mp11_source_root(course: Path) -> Path:
    marker = read_dependency_marker(course)
    source = marker.get("LEARNCPP_DEP_SOURCE_DIR")
    if source:
        return Path(source)
    return course / MP11_DEP_PREFIX / "source"


def mp11_include_dir(course: Path) -> Path:
    return mp11_source_root(course) / "include"


def variant_name(kind: str, count: int, target: str) -> str:
    return f"{kind}_{count}_{target}"


def generate_meta_lookup(kind: str, count: int, target: str) -> str:
    if kind not in KINDS:
        raise ValueError(f"unknown meta lookup kind: {kind}")
    if count not in COUNTS:
        raise ValueError(f"unsupported meta lookup count: {count}")
    if target not in TARGETS:
        raise ValueError(f"unknown meta lookup target: {target}")

    query_index = count - 1 if target == "last" else count
    expected = f"value<{count - 1}>" if target == "last" else "void"
    result_body = "return -1;" if target == "missing" else "return answer::id;"
    entries = ",\n    ".join(f"boost::mp11::mp_list<key<{i}>, value<{i}>>" for i in range(count))
    common = textwrap.dedent(f"""
        #include <type_traits>
        #include <boost/mp11/map.hpp>
        #include <boost/mp11/list.hpp>
        template<int I> struct key {{}};
        template<int I> struct value {{ static constexpr int id = I; }};

        using source_map = boost::mp11::mp_list<
            {entries}>;

        template<class T>
        struct entry_key;

        template<template<class...> class L, class K, class V, class... Rest>
        struct entry_key<L<K, V, Rest...>> : std::type_identity<K> {{}};

        template<class T>
        struct entry_value : std::type_identity<void> {{}};

        template<template<class...> class L, class K, class V, class... Rest>
        struct entry_value<L<K, V, Rest...>> : std::type_identity<V> {{}};

        template<class M, class K>
        struct manual_find;

        template<class K>
        struct manual_find<boost::mp11::mp_list<>, K> : std::type_identity<void> {{}};

        template<class Head, class... Tail, class K>
        struct manual_find<boost::mp11::mp_list<Head, Tail...>, K>
            : std::conditional_t<std::is_same_v<typename entry_key<Head>::type, K>,
                                 std::type_identity<Head>,
                                 manual_find<boost::mp11::mp_list<Tail...>, K>> {{}};
    """)
    if kind == "manual":
        body = f"using entry = typename manual_find<source_map, key<{query_index}>>::type;\n"
    else:
        body = f"using entry = boost::mp11::mp_map_find<source_map, key<{query_index}>>;\n"
    return textwrap.dedent(f"""
        {common}
        {body}
        using answer = typename entry_value<entry>::type;
        static_assert(std::is_same_v<answer, {expected}>);
        extern "C" __declspec(dllexport) int result() {{
            {result_body}
        }}
        int main() {{
            return result() == {'-1' if target == 'missing' else str(count - 1)} ? 0 : 1;
        }}
    """).strip() + "\n"
