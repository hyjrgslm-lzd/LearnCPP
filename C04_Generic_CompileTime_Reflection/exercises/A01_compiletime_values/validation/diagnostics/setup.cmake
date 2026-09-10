set(a01_values_root "${CMAKE_CURRENT_LIST_DIR}/../..")
foreach(target control subject)
    target_include_directories(${target} PRIVATE
        "${a01_values_root}/checks"
        "${a01_values_root}/validation/good")
endforeach()
