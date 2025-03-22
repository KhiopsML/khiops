file(GLOB cppfiles ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_library(KWDataUtils STATIC "${cppfiles}")
set_khiops_options(KWDataUtils)

target_include_directories(KWDataUtils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KWDataUtils PUBLIC KWDRRuleLibrary)

# Note for mpi: we don't add the PLMPI target for this library. It should be the good thing to do, because it would be
# the only use of it. We prefer to add it to the binaries, because otherwise KhiopsNativeInterface depends on MPI shared
# library when it is built with gcc on rockylinux.
