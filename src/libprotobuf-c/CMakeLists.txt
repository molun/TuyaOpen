##
# @file CMakeLists.txt
# @brief 
#/
if(CONFIG_ENABLE_LIB_PROTOBUF_C STREQUAL "y")

# MODULE_PATH
set(MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

# MODULE_NAME
get_filename_component(MODULE_NAME ${MODULE_PATH} NAME)

# LIB_SRCS
file(GLOB_RECURSE LIB_SRCS "${MODULE_PATH}/protobuf-c/protobuf-c/*.c")
# # LIB_SRCS
# file(GLOB_RECURSE LIB_SRCS 
#     "${MODULE_PATH}/mid_protobuf/*.c"
#     "${MODULE_PATH}/mid_websocket_client/*.c"
#     "${MODULE_PATH}/tuya_voice_protoco/*.c")
# LIB_PUBLIC_INC
set(LIB_PUBLIC_INC ${MODULE_PATH}/protobuf-c)
list(APPEND  LIB_PUBLIC_INC ${MODULE_PATH}/protobuf-c/protobuf-c)

########################################
# Target Configure
########################################
add_library(${MODULE_NAME})

target_sources(${MODULE_NAME}
    PRIVATE
        ${LIB_SRCS}
    )

target_include_directories(${MODULE_NAME}
    PRIVATE
        ${LIB_PRIVATE_INC}

    PUBLIC
        ${LIB_PUBLIC_INC}

    INTERFACE

    )


########################################
# Layer Configure
########################################
list(APPEND COMPONENT_LIBS ${MODULE_NAME})
set(COMPONENT_LIBS "${COMPONENT_LIBS}" PARENT_SCOPE)
list(APPEND COMPONENT_PUBINC ${LIB_PUBLIC_INC})
set(COMPONENT_PUBINC "${COMPONENT_PUBINC}" PARENT_SCOPE)

endif()

