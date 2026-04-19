#===============================================================================
# Alicat BASIS-2 Driver - Build Settings
# Shared variables for target name, includes, sources, and dependencies.
# Single source of truth for the driver version.
#===============================================================================

include_guard(GLOBAL)

set(HF_ALICAT_BASIS2_TARGET_NAME "hf_alicat_basis2")

#===============================================================================
# Versioning
#===============================================================================
set(HF_ALICAT_BASIS2_VERSION_MAJOR 1)
set(HF_ALICAT_BASIS2_VERSION_MINOR 0)
set(HF_ALICAT_BASIS2_VERSION_PATCH 0)
set(HF_ALICAT_BASIS2_VERSION
    "${HF_ALICAT_BASIS2_VERSION_MAJOR}.${HF_ALICAT_BASIS2_VERSION_MINOR}.${HF_ALICAT_BASIS2_VERSION_PATCH}")
set(HF_ALICAT_BASIS2_VERSION_STRING "${HF_ALICAT_BASIS2_VERSION}")

#===============================================================================
# Generated version header (build tree)
#===============================================================================
set(HF_ALICAT_BASIS2_VERSION_TEMPLATE
    "${CMAKE_CURRENT_LIST_DIR}/../inc/alicat_basis2_version.h.in")
set(HF_ALICAT_BASIS2_VERSION_HEADER_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/hf_alicat_basis2_generated")
set(HF_ALICAT_BASIS2_VERSION_HEADER
    "${HF_ALICAT_BASIS2_VERSION_HEADER_DIR}/alicat_basis2_version.h")

file(MAKE_DIRECTORY "${HF_ALICAT_BASIS2_VERSION_HEADER_DIR}")

if(EXISTS "${HF_ALICAT_BASIS2_VERSION_TEMPLATE}")
    configure_file(
        "${HF_ALICAT_BASIS2_VERSION_TEMPLATE}"
        "${HF_ALICAT_BASIS2_VERSION_HEADER}"
        @ONLY
    )
    message(STATUS
        "Alicat BASIS-2 driver v${HF_ALICAT_BASIS2_VERSION} — generated alicat_basis2_version.h in ${HF_ALICAT_BASIS2_VERSION_HEADER_DIR}")
else()
    message(WARNING "alicat_basis2_version.h.in not found at ${HF_ALICAT_BASIS2_VERSION_TEMPLATE}")
endif()

#===============================================================================
# Public include directories
#===============================================================================
set(HF_ALICAT_BASIS2_PUBLIC_INCLUDE_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../inc"
    "${HF_ALICAT_BASIS2_VERSION_HEADER_DIR}"
)

#===============================================================================
# Source files (header-only driver)
#===============================================================================
set(HF_ALICAT_BASIS2_SOURCE_FILES "")

#===============================================================================
# ESP-IDF component dependencies (header-only wrapper)
#===============================================================================
set(HF_ALICAT_BASIS2_IDF_REQUIRES driver)
