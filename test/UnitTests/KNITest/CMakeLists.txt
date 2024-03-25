file(GLOB cppfiles *.cpp)
add_executable(KNITest ${cppfiles})

set_khiops_options(KNITest)

target_link_libraries(KNITest GTest::gtest_main testutils KhiopsNativeInterface)
target_compile_options(KNITest PUBLIC ${GTEST_CFLAGS})
include(GoogleTest)
gtest_discover_tests(KNITest)
