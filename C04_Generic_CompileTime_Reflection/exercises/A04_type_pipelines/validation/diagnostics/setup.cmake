set(a04_root "${CMAKE_CURRENT_LIST_DIR}/../..")
foreach(target control subject)
    target_include_directories(${target} PRIVATE "${a04_root}/validation/good")
endforeach()
