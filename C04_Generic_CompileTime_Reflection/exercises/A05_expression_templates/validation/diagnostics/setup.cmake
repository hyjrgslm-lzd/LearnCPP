set(a05_root "${CMAKE_CURRENT_LIST_DIR}/../..")
foreach(target control subject)
    target_include_directories(${target} PRIVATE "${a05_root}/validation/good")
endforeach()
