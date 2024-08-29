file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(_khiopsgetprocnumber ${cppfiles} _khiopsgetprocnumber.rc)
set_khiops_options(_khiopsgetprocnumber)
target_link_libraries(_khiopsgetprocnumber PUBLIC base)
