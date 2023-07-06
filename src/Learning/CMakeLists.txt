add_subdirectory(DTForest)
add_subdirectory(genum)
add_subdirectory(khisto)

add_subdirectory(KDDomainKnowledge)
add_subdirectory(KhiopsNativeInterface)
add_subdirectory(KIInterpretation)

add_subdirectory(KMDRRuleLibrary)
if(FULL)
  add_subdirectory(KNITransfer)
endif(FULL)

add_subdirectory(KWData)
add_subdirectory(KWDataPreparation)
add_subdirectory(KWDataUtils)
add_subdirectory(KWDRRuleLibrary)
add_subdirectory(KWLearningProblem)
add_subdirectory(KWModeling)
add_subdirectory(KWUserInterface)
add_subdirectory(KWUtils)
add_subdirectory(MHHistograms)
add_subdirectory(MODL)
add_subdirectory(MODL_Coclustering)
add_subdirectory(SNBPredictor)

# build norm.jar
if(BUILD_JARS)
  include(UseJava)
  add_jar(
    khiops_jar
    OUTPUT_NAME
      khiops
      RESOURCES
      NAMESPACE
      ""
      ../../packaging/common/images/khiops_coclustering_about.gif
      ../../packaging/common/images/khiops_coclustering.gif
      ../../packaging/common/images/khiops_about.gif
      ../../packaging/common/images/khiops.gif
    OUTPUT_DIR ${CMAKE_BINARY_DIR})
  message(STATUS "build khiops.jar in ${CMAKE_BINARY_DIR}")
  install_jar(
    khiops_jar
    DESTINATION usr/share/khiops
    COMPONENT KHIOPS)
endif()

include(install)
