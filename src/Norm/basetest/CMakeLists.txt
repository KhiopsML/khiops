# On utilise les exceptions seulement pour le fichier KWLearningProject.cpp
if(NOT MSVC)
  set_source_files_properties(main.cpp PROPERTIES COMPILE_FLAGS -fexceptions)
endif(NOT MSVC)
file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
generate_gui_add_view_get_log_list(ddlogfiles)

add_executable(basetest ${cppfiles} ${ddlogfiles})
set_khiops_options(basetest)
target_link_libraries(basetest PUBLIC base)

# Generate cpp files from dd file
# cmake-format: off
 generate_gui_add_view(UITestObject "Test object" UITestObject.dd UITestObject.dd.log -noarrayview)
 generate_gui_add_view(UITestSubObject "Test sub-object" UITestSubObject.dd UITestSubObject.dd.log -noarrayview)
 generate_gui_add_view(UITestActionSubObject "Test sub-object with action" UITestActionSubObject.dd UITestActionSubObject.dd.log -noarrayview)
# cmake-format: on

# Add dependency to ensure that genere will be built before the gui generation
generate_gui_add_view_add_dependency(basetest)
