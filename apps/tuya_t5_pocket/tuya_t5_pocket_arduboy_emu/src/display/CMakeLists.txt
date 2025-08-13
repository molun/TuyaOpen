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

# Remove simavr files and add minimal AVR emulation
list(REMOVE_ITEM UI_SRCS
    ${APP_MODULE_PATH}/ui/arduboy_avr.c
    ${APP_MODULE_PATH}/ui/sim_arduboy.h
)

# Add minimal AVR emulation files
list(APPEND UI_SRCS
    ${APP_MODULE_PATH}/ui/mini_avr_emu.c
)

list(APPEND APP_MODULE_SRCS
    ${UI_SRCS}
)

set(APP_MODULE_INC 
    ${APP_MODULE_PATH}
    ${APP_MODULE_PATH}/ui
)

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