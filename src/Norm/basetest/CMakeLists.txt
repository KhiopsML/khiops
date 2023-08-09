# On utilise les exceptions seulement pour le fichier KWLearningProject.cpp
if(NOT MSVC)
  set_source_files_properties(main.cpp PROPERTIES COMPILE_FLAGS -fexceptions)
endif(NOT MSVC)

file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(basetest ${cppfiles})
set_khiops_options(basetest)
# cmake-format: off
add_view_generation(basetest UITestObject "Test object" UITestObject.dd UITestObject.dd.log -noarrayview)
add_view_generation(basetest UITestSubObject "Test sub-object" UITestSubObject.dd UITestSubObject.dd.log -noarrayview)
add_view_generation(basetest UITestActionSubObject "Test sub-object with action" UITestActionSubObject.dd UITestActionSubObject.dd.log
           -noarrayview)
# cmake-format: on
target_link_libraries(basetest PUBLIC base)
