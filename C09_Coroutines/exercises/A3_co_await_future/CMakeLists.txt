find_package(Threads REQUIRED)
coroutine_study_add_exercise(A3_co_await_future
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
    LIBRARIES Threads::Threads
)
