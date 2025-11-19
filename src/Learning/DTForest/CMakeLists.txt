file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(DTForest STATIC "${cppfiles}")

set_khiops_options(DTForest)

target_include_directories(DTForest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(DTForest PUBLIC KWDataPreparation KWModeling KWUserInterface)
