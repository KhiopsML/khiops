add_subdirectory(DTForest)
add_subdirectory(genum)
add_subdirectory(khisto)
add_subdirectory(KDDomainKnowledge)
add_subdirectory(KhiopsNativeInterface)
add_subdirectory(KIInterpretation)
add_subdirectory(KNITransfer)
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
add_subdirectory(samples/sample3)

add_subdirectory(KWTest)

# Add khiops.jar (CMake names the target khiops_jar)
if(BUILD_JARS)
  add_khiops_jar(
    NAME
    "khiops"
    NAMESPACE
    "images"
    IMAGES
    "../../packaging/common/images/khiops_coclustering_about.gif"
    "../../packaging/common/images/khiops_coclustering.gif"
    "../../packaging/common/images/khiops_about.gif"
    "../../packaging/common/images/khiops.gif")
endif()

include(install)
