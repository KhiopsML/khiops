project(Norm LANGUAGES CXX)

add_subdirectory(base)
add_subdirectory(khiopsdriver_file_null)
add_subdirectory(genere)

add_subdirectory(basetest)
add_subdirectory(generetest)
add_subdirectory(_khiopsgetprocnumber)

# Add norm.jar (CMake names the target norm_jar)
if(BUILD_JARS)
  file(GLOB_RECURSE norm_java_files NormGUI/*.java)
  add_khiops_jar(
    NAME
    "norm"
    JAVA_FILES
    ${norm_java_files}
    NAMESPACE
    "images"
    IMAGES
    "NormGUI/images/sample.png"
    "NormGUI/images/sample.jpg"
    "NormGUI/images/sample.gif"
    "NormGUI/images/sample2.png")
endif()
