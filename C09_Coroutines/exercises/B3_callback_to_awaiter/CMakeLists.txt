find_package(Threads REQUIRED)
coroutine_study_add_exercise(B3_callback_to_awaiter
    SOURCES main.cpp
    REFERENCE_SOURCES solution.cpp
    LIBRARIES Threads::Threads
)
