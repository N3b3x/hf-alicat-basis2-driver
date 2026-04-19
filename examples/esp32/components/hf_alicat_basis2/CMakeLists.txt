# hf_alicat_basis2 — ESP-IDF component wrapper (header-only driver)
set(HF_ALICAT_BASIS2_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." CACHE PATH
    "Path to hf-alicat-basis2-driver root")

get_filename_component(HF_ALICAT_BASIS2_ROOT "${HF_ALICAT_BASIS2_ROOT}" ABSOLUTE)
if(NOT EXISTS "${HF_ALICAT_BASIS2_ROOT}/cmake/hf_alicat_basis2_build_settings.cmake")
    message(FATAL_ERROR
        "Cannot find Alicat BASIS-2 driver root at ${HF_ALICAT_BASIS2_ROOT}")
endif()

include("${HF_ALICAT_BASIS2_ROOT}/cmake/hf_alicat_basis2_build_settings.cmake")

idf_component_register(
    INCLUDE_DIRS ${HF_ALICAT_BASIS2_PUBLIC_INCLUDE_DIRS}
    REQUIRES ${HF_ALICAT_BASIS2_IDF_REQUIRES}
)

target_compile_features(${COMPONENT_LIB} INTERFACE cxx_std_17)
