# On utilise les exceptions seulement pour le fichier KWLearningProject.cpp
if(NOT MSVC)
  set_source_files_properties(main.cpp PROPERTIES COMPILE_FLAGS -fexceptions)
endif(NOT MSVC)
file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(basetest ${cppfiles})
set_khiops_options(basetest)
target_link_libraries(basetest PUBLIC base)
