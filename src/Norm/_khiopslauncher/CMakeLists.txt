file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(_khiopslauncher ${cppfiles} _khiopslauncher.rc)
set_target_properties(_khiopslauncher PROPERTIES WIN32_EXECUTABLE TRUE)
set_khiops_options(_khiopslauncher)
