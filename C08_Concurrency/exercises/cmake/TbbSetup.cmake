# Compatibility entry. StudySetup owns the actual compile-and-link capability
# check; a compiler frontend name alone does not identify the parallel backend.
include("${CMAKE_CURRENT_LIST_DIR}/StudySetup.cmake")
set(CONCURRENCY_STUDY_PAR_LIBS "")
if(CS_HAS_PARALLEL_ALGORITHMS AND TARGET TBB::tbb)
    set(CONCURRENCY_STUDY_PAR_LIBS TBB::tbb)
endif()
