/**
 * @file app_pocket.c
 * @brief Pocket application initialization for Arduboy emulator
 * @version 0.1
 * @date 2025-03-25
 */

#include "app_pocket.h"
#include "app_display.h"
#include <stdio.h>

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

/* Remove local implementation to use board's implementation */

/**
 * @brief Initialize the pocket application
 *
 * @param None
 * @return OPERATE_RET Initialization result, OPRT_OK indicates success
 */
OPERATE_RET app_pocket_init(void)
{
    OPERATE_RET rt = OPRT_OK;

    printf("Starting app_pocket_init");

    /* Initialize the display system */
    rt = app_display_init();
    if (rt != OPRT_OK) {
        printf("app_display_init failed: %d", rt);
        return rt;
    }

    printf("app_pocket_init completed successfully");
    return OPRT_OK;
}
