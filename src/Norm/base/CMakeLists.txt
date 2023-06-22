# On utilise les exceptions seulement pour le fichier Regex.cpp
if(NOT MSVC)
  set_source_files_properties(Regex.cpp PROPERTIES COMPILE_FLAGS -fexceptions)
endif(NOT MSVC)

file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(base STATIC ${cppfiles})

set_khiops_options(base)

target_include_directories(base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(NOT MSVC)
  target_link_libraries(base PUBLIC ${CMAKE_DL_LIBS})
endif(NOT MSVC)
