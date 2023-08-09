file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# on fedora, binaries built with mpi must ended by _mpich suffix
add_executable(MODL_Coclustering${MPI_SUFFIX} ${cppfiles})
set_khiops_options(MODL_Coclustering${MPI_SUFFIX})
target_link_libraries(MODL_Coclustering${MPI_SUFFIX} PUBLIC KWLearningProblem DTForest KMDRRuleLibrary)

if(MSVC)
  target_link_options(MODL_Coclustering${MPI_SUFFIX} PUBLIC "/INCREMENTAL")
endif()

add_library(MODL_Coclustering_SO SHARED ${cppfiles})
# Add view generation cpp files from dd files
# cmake-format: off
add_view_generation(MODL_Coclustering${MPI_SUFFIX} CCVarPartCoclusteringSpec "Instances Variables coclustering parameters" CCVarPartCoclusteringSpec.dd CCVarPartCoclusteringSpec.dd.log -noarrayview)
add_view_generation(MODL_Coclustering${MPI_SUFFIX} CCCoclusteringSpec "Coclustering parameters" CCCoclusteringSpec.dd CCCoclusteringSpec.dd.log -noarrayview)
add_view_generation(MODL_Coclustering${MPI_SUFFIX} CCPostProcessingSpec "Simplification parameters" CCPostProcessingSpec.dd CCPostProcessingSpec.dd.log -noarrayview)
add_view_generation(MODL_Coclustering${MPI_SUFFIX} CCPostProcessedAttribute "Coclustering variable" CCPostProcessedAttribute.dd CCPostProcessedAttribute.dd.log)
add_view_generation(MODL_Coclustering${MPI_SUFFIX} CCDeploymentSpec "Deployment parameters" CCDeploymentSpec.dd CCDeploymentSpec.dd.log -noarrayview)
add_view_generation(MODL_Coclustering${MPI_SUFFIX} CCAnalysisResults "Results" CCAnalysisResults.dd CCAnalysisResults.dd.log -noarrayview)
# cmake-format: on
target_link_libraries(MODL_Coclustering_SO PUBLIC KWLearningProblem DTForest KMDRRuleLibrary PLParallelTask)
set_target_properties(
  MODL_Coclustering_SO
  PROPERTIES PUBLIC_HEADER MODL_Coclustering_dll.h
             SOVERSION ${PROJECT_VERSION_MAJOR}
             VERSION ${PROJECT_VERSION})
set_khiops_options(MODL_Coclustering_SO)
