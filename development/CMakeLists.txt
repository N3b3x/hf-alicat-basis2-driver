# =============================================================================
# hf-alicat-basis2-driver — Root CMakeLists.txt
# =============================================================================
# Hardware-agnostic header-only driver for Alicat BASIS-2 mass-flow meters
# and controllers (B-Series / BC-Series). Provides a Modbus-RTU + ASCII
# serial command interface bridged to any byte-level transport via a CRTP
# adapter (see inc/alicat_basis2_uart_interface.hpp).
# For ESP-IDF integration, register the driver as a component using the
# pattern under examples/esp32 or via cmake/hf_alicat_basis2_build_settings.cmake.
# =============================================================================
cmake_minimum_required(VERSION 3.16)

project(hf_alicat_basis2
  VERSION 1.0.0
  LANGUAGES CXX
)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/hf_alicat_basis2_build_settings.cmake)

add_library(${HF_ALICAT_BASIS2_TARGET_NAME} INTERFACE)
target_include_directories(${HF_ALICAT_BASIS2_TARGET_NAME}
    INTERFACE ${HF_ALICAT_BASIS2_PUBLIC_INCLUDE_DIRS})
target_compile_features(${HF_ALICAT_BASIS2_TARGET_NAME} INTERFACE cxx_std_17)

add_library(hf::alicat_basis2 ALIAS ${HF_ALICAT_BASIS2_TARGET_NAME})

option(HF_ALICAT_BASIS2_ENABLE_WARNINGS "Enable compiler warnings for BASIS-2 consumers" OFF)
if(HF_ALICAT_BASIS2_ENABLE_WARNINGS)
  target_compile_options(${HF_ALICAT_BASIS2_TARGET_NAME}
      INTERFACE -Wall -Wextra -Wpedantic)
endif()

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/hf_alicat_basis2ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/hf_alicat_basis2Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/hf_alicat_basis2Config.cmake"
  INSTALL_DESTINATION lib/cmake/hf_alicat_basis2
)
