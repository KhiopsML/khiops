file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
generate_gui_add_view_get_log_list(ddlogfiles)

# on fedora, binaries built with mpi must ended by _mpich suffix
add_executable(MODL_Coclustering ${cppfiles} ${ddlogfiles} MODL_Coclustering.rc)
set_khiops_options(MODL_Coclustering)
target_link_libraries(MODL_Coclustering PUBLIC KWLearningProblem)

# on fedora, binaries built with mpi must ended by _mpich or _openmpi suffix
if((IS_LINUX OR IS_MACOS) AND MPI)
  set_target_properties(MODL_Coclustering PROPERTIES OUTPUT_NAME "MODL_Coclustering${MPI_SUFFIX}")
endif()

if(MPI)
  target_link_libraries(MODL_Coclustering PUBLIC PLMPI)
endif()

add_library(MODL_Coclustering_DLL SHARED ${cppfiles} ${ddlogfiles})
target_link_libraries(MODL_Coclustering_DLL PUBLIC KWLearningProblem)

if(MPI)
  target_link_libraries(MODL_Coclustering_DLL PUBLIC PLMPI)
endif()

set_target_properties(
  MODL_Coclustering_DLL
  PROPERTIES PUBLIC_HEADER MODL_Coclustering_dll.h
             SOVERSION ${PROJECT_VERSION_MAJOR}
             VERSION ${KHIOPS_VERSION})
set_khiops_options(MODL_Coclustering_DLL)

# Generate cpp files from dd file
# cmake-format: off
 generate_gui_add_view(CCCoclusteringSpec "Coclustering parameters" CCCoclusteringSpec.dd CCCoclusteringSpec.dd.log -noarrayview)
 generate_gui_add_view(CCPostProcessingSpec "Simplification parameters" CCPostProcessingSpec.dd CCPostProcessingSpec.dd.log -noarrayview)
 generate_gui_add_view(CCPostProcessedAttribute "Coclustering variable" CCPostProcessedAttribute.dd CCPostProcessedAttribute.dd.log)
 generate_gui_add_view(CCDeploymentSpec "Deployment parameters" CCDeploymentSpec.dd CCDeploymentSpec.dd.log -noarrayview)
 generate_gui_add_view(CCAnalysisResults "Results" CCAnalysisResults.dd CCAnalysisResults.dd.log -noarrayview)
# cmake-format: on

# Add dependency to ensure that genere will be built before the gui generation
generate_gui_add_view_add_dependency(MODL_Coclustering)
