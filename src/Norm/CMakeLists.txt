project(Norm LANGUAGES CXX)

add_subdirectory(base)
add_subdirectory(khiopsdriver_file_null)
add_subdirectory(genere)

add_subdirectory(basetest)
add_subdirectory(generetest)
add_subdirectory(_khiopsgetprocnumber)
add_subdirectory(_khiopslauncher)
add_subdirectory(samples/sample1)
add_subdirectory(samples/sample2)
add_subdirectory(samples/sample3)

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
    "NormGUI/theme/FlatLaf.properties"
    "NormGUI/theme/FlatArcOrangeIJTheme.properties"
    "NormGUI/theme/lucide/rocket.svg"
    "NormGUI/theme/lucide/cpu-ai.svg"
    "NormGUI/theme/lucide/database.svg"
    "NormGUI/theme/lucide/settings-2.svg"
    "NormGUI/theme/lucide/chart-column-big.svg"
    "NormGUI/fonts/Poppins-Regular.ttf"
    "NormGUI/images/logo-visualization.svg"
    "NormGUI/images/logo-covisualization.svg"
    "NormGUI/images/sample.png"
    "NormGUI/images/sample.jpg"
    "NormGUI/images/sample.gif"
    "NormGUI/images/sample2.png")
endif()
