set(a02_projection_root "${CMAKE_CURRENT_LIST_DIR}/../..")
foreach(target control subject)
    target_include_directories(${target} PRIVATE
        "${a02_projection_root}/checks"
        "${a02_projection_root}/validation/good")
endforeach()
