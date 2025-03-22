file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(genere ${cppfiles} "resource.h" "Version.h" "genere.rc")
set_khiops_options(genere)
target_link_libraries(genere PUBLIC base)
