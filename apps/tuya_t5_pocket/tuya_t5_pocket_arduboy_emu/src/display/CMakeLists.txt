##
# @file CMakeLists.txt
# @brief 
#/
set(APP_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

set(APP_MODULE_SRCS)
list(APPEND APP_MODULE_SRCS
        ${APP_MODULE_PATH}/app_display.c
    )

aux_source_directory(${APP_MODULE_PATH}/ui UI_SRCS)

list(APPEND APP_MODULE_SRCS
    ${UI_SRCS}
)

# Use prebuilt libelf and simavr from prebuild orchestrator output
set(PREBUILD_OUTPUT_DIR ${APP_MODULE_PATH}/ui/prebuilds/output)
set(PREBUILD_INCLUDE_DIR ${PREBUILD_OUTPUT_DIR}/include)
set(PREBUILD_LIB_DIR ${PREBUILD_OUTPUT_DIR}/lib)

set(APP_MODULE_INC
    ${APP_MODULE_PATH}
    ${APP_MODULE_PATH}/ui
    ${PREBUILD_INCLUDE_DIR}
    ${PREBUILD_INCLUDE_DIR}/simavr
    ${PREBUILD_INCLUDE_DIR}/simavr/avr
    ${PREBUILD_INCLUDE_DIR}/simavr/parts
)

# Import prebuilt static libraries directly by absolute path
set(PREBUILD_LIB_LIBELF ${PREBUILD_LIB_DIR}/libelf.a)
set(PREBUILD_LIB_LIBEU  ${PREBUILD_LIB_DIR}/libeu.a)
set(PREBUILD_LIB_SIMAVR ${PREBUILD_LIB_DIR}/libsimavr.a)

add_library(libelf STATIC IMPORTED)
set_target_properties(libelf PROPERTIES IMPORTED_LOCATION ${PREBUILD_LIB_LIBELF})

if(EXISTS ${PREBUILD_LIB_LIBEU})
    add_library(libeu STATIC IMPORTED)
    set_target_properties(libeu PROPERTIES IMPORTED_LOCATION ${PREBUILD_LIB_LIBEU})
endif()

add_library(simavr STATIC IMPORTED)
set_target_properties(simavr PROPERTIES IMPORTED_LOCATION ${PREBUILD_LIB_SIMAVR})

########################################
# Target Configure
########################################
target_sources(${EXAMPLE_LIB}
    PRIVATE
        ${APP_MODULE_SRCS}
    )

target_include_directories(${EXAMPLE_LIB}
    PRIVATE
        ${APP_MODULE_INC}
)

if(TARGET libeu)
    target_link_libraries(${EXAMPLE_LIB} simavr libelf libeu z zstd)
else()
    target_link_libraries(${EXAMPLE_LIB} simavr libelf z zstd)
endif()