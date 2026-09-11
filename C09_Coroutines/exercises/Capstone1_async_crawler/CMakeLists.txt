coroutine_study_add_exercise(Capstone1_async_crawler
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
    STUDENT_TEST
)
target_include_directories(Capstone1_async_crawler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(TARGET Capstone1_async_crawler_reference)
    target_include_directories(Capstone1_async_crawler_reference PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()
