include_guard(GLOBAL)

option(RANGES_ENABLE_WARNINGS "Enable strict warnings"          OFF)
option(RANGES_ENABLE_ASAN     "Enable AddressSanitizer"         OFF)
option(RANGES_ENABLE_UBSAN    "Enable UndefinedBehaviorSanitizer" OFF)

if(RANGES_ENABLE_WARNINGS)
    if(MSVC)
        add_compile_options(/W4)
    else()
        add_compile_options(-Wall -Wextra -Wpedantic)
    endif()
endif()

if(RANGES_ENABLE_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

if(RANGES_ENABLE_UBSAN AND NOT MSVC)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif()

message(STATUS
    "RangesSetup loaded "
    "(warnings=${RANGES_ENABLE_WARNINGS} "
    "asan=${RANGES_ENABLE_ASAN} "
    "ubsan=${RANGES_ENABLE_UBSAN})")
