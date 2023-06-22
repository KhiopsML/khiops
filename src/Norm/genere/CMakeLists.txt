file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(genere ${cppfiles})
set_khiops_options(genere)
target_link_libraries(genere PUBLIC base)
