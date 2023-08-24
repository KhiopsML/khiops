project(Norm LANGUAGES CXX)

add_subdirectory(base)
add_subdirectory(khiopsdriver_file_null)
add_subdirectory(genere)

add_subdirectory(basetest)
add_subdirectory(generetest)

# build norm.jar
if(BUILD_JARS)
  # we recommend to use java 1.8 to ensure that the jar is compatible with all java versions (from 1.8 to the most
  # recent ones) find_package(Java 1.8 EXACT REQUIRED COMPONENTS Development)
  find_package(Java REQUIRED COMPONENTS Development)

  include(UseJava)
  file(GLOB_RECURSE javafiles NormGUI/*.java)
  add_jar(
    norm_jar ${javafiles}
    OUTPUT_NAME norm
    ENTRY_POINT
      normGUI.engine.GUIObject
      RESOURCES
      NAMESPACE
      "images"
      NormGUI/images/sample.png
      NormGUI/images/sample.jpg
      NormGUI/images/sample.gif
      NormGUI/images/sample2.png
    OUTPUT_DIR ${CMAKE_BINARY_DIR})
  message(STATUS "build norm.jar in ${CMAKE_BINARY_DIR}")
  install_jar(
    norm_jar
    DESTINATION usr/share/khiops
    COMPONENT KHIOPS)

endif()
