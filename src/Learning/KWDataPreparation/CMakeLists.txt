file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(KWDataPreparation STATIC "${cppfiles}")
set_khiops_options(KWDataPreparation)

target_include_directories(KWDataPreparation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KWDataPreparation PUBLIC KWDataUtils)
