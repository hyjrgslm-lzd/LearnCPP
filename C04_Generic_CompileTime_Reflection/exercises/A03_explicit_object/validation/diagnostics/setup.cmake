set(a03_object_root "${CMAKE_CURRENT_LIST_DIR}/../..")
foreach(target control subject)
    target_include_directories(${target} PRIVATE
        "${a03_object_root}/checks"
        "${a03_object_root}/validation/good")
endforeach()
