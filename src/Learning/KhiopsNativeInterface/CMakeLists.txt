file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(KhiopsNativeInterface SHARED "${cppfiles}")
target_include_directories(KhiopsNativeInterface PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KhiopsNativeInterface PUBLIC KWLearningProblem)
set_target_properties(
  KhiopsNativeInterface
  PROPERTIES PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/KhiopsNativeInterface.h
             SOVERSION ${PROJECT_VERSION_MAJOR}
             VERSION ${PROJECT_VERSION})

set_khiops_options(KhiopsNativeInterface)
