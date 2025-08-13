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

set(APP_MODULE_INC 
    ${APP_MODULE_PATH}
    ${APP_MODULE_PATH}/ui
    ${APP_MODULE_PATH}/ui/simavr/simavr/sim
    ${APP_MODULE_PATH}/ui/simavr/examples/parts
)

# Build simavr static library via its makefile and link it
execute_process(
    COMMAND ${CMAKE_C_COMPILER} -dumpmachine
    OUTPUT_VARIABLE GCC_DUMP_MACHINE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(SIMAVR_DIR ${APP_MODULE_PATH}/ui/simavr)
set(SIMAVR_BIN_DIR ${SIMAVR_DIR}/simavr)
set(SIMAVR_OBJ_DIR ${SIMAVR_BIN_DIR}/obj-${GCC_DUMP_MACHINE})
set(SIMAVR_LIB ${SIMAVR_OBJ_DIR}/libsimavr.a)

add_custom_command(
    OUTPUT ${SIMAVR_LIB}
    COMMAND make -C ${SIMAVR_BIN_DIR} libsimavr
    WORKING_DIRECTORY ${SIMAVR_BIN_DIR}
    COMMENT "Building simavr static library"
)

add_custom_target(simavr_build DEPENDS ${SIMAVR_LIB})

add_library(simavr STATIC IMPORTED)
set_target_properties(simavr PROPERTIES IMPORTED_LOCATION ${SIMAVR_LIB})
add_dependencies(simavr simavr_build)

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

target_link_libraries(${EXAMPLE_LIB} simavr elf)