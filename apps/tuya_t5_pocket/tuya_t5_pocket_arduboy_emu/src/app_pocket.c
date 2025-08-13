/**
 * @file app_pocket.c
 * @brief Pocket application initialization for Arduboy emulator
 * @version 0.1
 * @date 2025-03-25
 */

#include "app_pocket.h"
#include "app_display.h"

/***********************************************************
************************macro define************************
***********************************************************/

/***********************************************************
***********************typedef define***********************
***********************************************************/

/***********************************************************
***********************variable define**********************
***********************************************************/

/***********************************************************
***********************function define**********************
***********************************************************/

/**
 * @brief Register hardware components for the pocket device
 *
 * @param None
 * @return OPERATE_RET Registration result, OPRT_OK indicates success
 */
OPERATE_RET board_register_hardware(void)
{
    /* For Arduboy emulator, we don't need to register specific hardware
       as the display and input handling is managed by the display system */
    return OPRT_OK;
}

/**
 * @brief Initialize the pocket application
 *
 * @param None
 * @return OPERATE_RET Initialization result, OPRT_OK indicates success
 */
OPERATE_RET app_pocket_init(void)
{
    OPERATE_RET rt = OPRT_OK;

    /* Initialize the display system */
    rt = app_display_init();
    if (rt != OPRT_OK) {
        return rt;
    }

    return OPRT_OK;
}
