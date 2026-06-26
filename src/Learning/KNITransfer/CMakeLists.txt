file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(KNITransfer ${cppfiles})
set_khiops_options(KNITransfer)

target_link_libraries(KNITransfer PUBLIC KhiopsNativeInterface KWLearningProblem DTForest PLParallelTask)

# On Pip installations the KNI library is installed into `bin/`, in the same directory as KNITransfer, not into `lib/`.
# Hence, append the binary's own directory to the rpath so that it is found at runtime on MacOS and Linux.
if(IS_PIP AND NOT MSVC)
  if(APPLE)
    set_property(
      TARGET KNITransfer
      APPEND
      PROPERTY INSTALL_RPATH "@loader_path")
  else()
    set_property(
      TARGET KNITransfer
      APPEND
      PROPERTY INSTALL_RPATH "$ORIGIN")
  endif()
endif()
