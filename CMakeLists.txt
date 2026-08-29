cmake_minimum_required(VERSION 3.25)
project(MicroRuntimeFramework VERSION 0.1.0 LANGUAGES CXX)

option(VOSP_RUNTIME_BUILD_TESTS "Build runtime tests" ON)
option(VOSP_RUNTIME_BUILD_EXAMPLES "Build runtime examples" ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(vosp_runtime INTERFACE)
add_library(vosp::runtime ALIAS vosp_runtime)
target_compile_features(vosp_runtime INTERFACE cxx_std_23)
target_include_directories(vosp_runtime INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if(VOSP_RUNTIME_BUILD_TESTS)
  enable_testing()
  add_executable(vosp_runtime_tests tests/runtime_tests.cpp)
  target_link_libraries(vosp_runtime_tests PRIVATE vosp::runtime)
  add_test(NAME vosp_runtime_tests COMMAND vosp_runtime_tests)
endif()

if(VOSP_RUNTIME_BUILD_EXAMPLES)
  add_executable(vosp_runtime_basic examples/basic.cpp)
  target_link_libraries(vosp_runtime_basic PRIVATE vosp::runtime)
endif()

install(TARGETS vosp_runtime EXPORT vospRuntimeTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT vospRuntimeTargets FILE vospRuntimeTargets.cmake NAMESPACE vosp:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vosp-runtime)
configure_package_config_file(cmake/vosp-runtimeConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/vosp-runtimeConfig.cmake INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vosp-runtime)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/vosp-runtimeConfigVersion.cmake VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vosp-runtimeConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/vosp-runtimeConfigVersion.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/vosp-runtime)
