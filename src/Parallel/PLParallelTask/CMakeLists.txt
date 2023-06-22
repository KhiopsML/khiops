file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(PLParallelTask STATIC ${cppfiles})

set_khiops_options(PLParallelTask)

target_include_directories(PLParallelTask PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(PLParallelTask PUBLIC RMResourceManager PLMPI) # TODO remove PLMPI in next release
