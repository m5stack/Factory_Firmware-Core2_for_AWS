/*
 * AWS IoT EduKit - Core2 for AWS IoT EduKit
 * Factory Firmware v2.3.0
 * main.c
 * 
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_freertos_hooks.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs_fat.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "core2forAWS.h"

#include "sound.h"
#include "home.h"
#include "wifi.h"
#include "mpu.h"
#include "mic.h"
#include "clock.h"
#include "power.h"
#include "touch.h"
#include "led_bar.h"
#include "crypto.h"
#include "cta.h"

static const char *TAG = "MAIN";

static void ui_start(void);
static void tab_event_cb(lv_obj_t *slider, lv_event_t event);

static lv_obj_t *tab_view;
TaskHandle_t    FFT_handle,
                led_bar_animation_handle, 
                led_bar_solid_handle,
                mic_handle,
                MPU_handle,
                touch_handle,
                wifi_handle;

LV_IMG_DECLARE( powered_by_aws_logo );

void app_main( void )
{
    ESP_LOGI( TAG, "\n***************************************************\n M5Stack Core2 for AWS IoT EduKit Factory Firmware\n***************************************************" );

    esp_log_level_set( "gpio", ESP_LOG_NONE );
    esp_log_level_set( "ILI9341", ESP_LOG_NONE );

    core2foraws_init(); // Initializes the enabled hardware drivers and calls their respective initialization functions.
    
    ui_start(); // Starts all the sensor readings and shows them on the display using the LVGL library
}

static void ui_start( void )
{
    /* Displays the Powered by AWS logo */
    xSemaphoreTake( core2foraws_display_semaphore, portMAX_DELAY );   // Takes the core2foraws_display_semaphore mutex. This blocks any other task attempting to take it before it's free'd from executing.
    lv_obj_t *opener_scr = lv_scr_act();   // Create a new LVGL "screen". Screens can be though of as a window.
    lv_obj_t *aws_img_obj = lv_img_create( opener_scr, NULL );   // Creates an LVGL image object and assigns it as a child of the opener_scr parent screen.
    lv_img_set_src( aws_img_obj, &powered_by_aws_logo );  // Sets the image object with the image data from the powered_by_aws_logo file which contains hex pixel matrix of the image.
    lv_obj_align( aws_img_obj, NULL, LV_ALIGN_CENTER, 0, 0 ); // Aligns the image object to the center of the parent screen.
    lv_obj_set_style_local_bg_color( opener_scr, LV_OBJ_PART_MAIN, 0, LV_COLOR_WHITE );   // Sets the background color of the screen to white.
    xSemaphoreGive( core2foraws_display_semaphore );  // Frees the core2foraws_display_semaphore so that another task can use it. In this case, the higher priority guiTask will take it and then read the values to then display.

    /* 
    You should release the core2foraws_display_semaphore semaphore before calling a blocking function like vTaskDelay because 
    without doing so, no other task can access it that might need it. This includes the guiTask which
    writes the objects to the display itself over SPI.
    */

    vTaskDelay( pdMS_TO_TICKS( 1500 ) ); // FreeRTOS scheduler block execution for 1.5 seconds to keep showing the Powered by AWS logo.
    
    xTaskCreatePinnedToCore( sound_task, "soundTask", 4096 * 2, NULL, 4, NULL, 1 );
    
    xSemaphoreTake( core2foraws_display_semaphore, portMAX_DELAY );   // Takes the core2foraws_display_semaphore mutex. This blocks any other task attempting to take it before it's free'd from executing.
    lv_obj_clean( opener_scr );   // Clear the aws_img_obj and remove from memory space. Currently no objects exist on the screen.
    lv_obj_t *core2forAWS_obj = lv_obj_create( NULL, NULL ); // Create an object to draw all with no parent 
    lv_scr_load_anim( core2forAWS_obj, LV_SCR_LOAD_ANIM_MOVE_LEFT, 400, 0, false );   // Animates the loading of core2forAWS_obj as a slide into view from the left
    tab_view = lv_tabview_create( core2forAWS_obj, NULL ); // Creates the tab view to display different tabs with different hardware features
    lv_obj_set_event_cb( tab_view, tab_event_cb ); // Add a callback for whenever there is an event triggered on the tab_view object (e.g. a left-to-right swipe)
    lv_tabview_set_btns_pos( tab_view, LV_TABVIEW_TAB_POS_NONE );  // Hide the tab buttons so it looks like a clean screen
    
    xSemaphoreGive( core2foraws_display_semaphore );  // Frees the core2foraws_display_semaphore so that another task can use it. In this case, the higher priority guiTask will take it and then read the values to then display.

    /*
    Below creates all the display layers for the various peripheral tabs. Some of the tabs also starts the concurrent FreeRTOS tasks 
    that read/write to the peripheral registers and displays the data from that peripheral.
    */
    display_home_tab( tab_view );
    display_clock_tab( tab_view, core2forAWS_obj );
    display_mpu_tab( tab_view );
    display_microphone_tab( tab_view );
    display_LED_bar_tab( tab_view );
    display_power_tab( tab_view, core2forAWS_obj );
    display_touch_tab( tab_view );
    display_crypto_tab( tab_view );
    display_wifi_tab( tab_view );
    display_cta_tab( tab_view );
}

static void tab_event_cb( lv_obj_t *slider, lv_event_t event )
{
    if ( event == LV_EVENT_VALUE_CHANGED )
    {
        lv_tabview_ext_t *ext = ( lv_tabview_ext_t * ) lv_obj_get_ext_attr( tab_view );
        const char *tab_name = ext->tab_name_ptr[ lv_tabview_get_tab_act( tab_view ) ];
        ESP_LOGI( TAG, "Current Active Tab: %s\n", tab_name );

        vTaskSuspend( MPU_handle );
        vTaskSuspend( mic_handle );
        vTaskSuspend( FFT_handle );
        vTaskSuspend( wifi_handle );
        vTaskSuspend( touch_handle );
        vTaskSuspend( led_bar_solid_handle );
        vTaskResume( led_bar_animation_handle );
        
        if ( strcmp( tab_name, CLOCK_TAB_NAME ) == 0 )
            update_roller_time();
        else if ( strcmp( tab_name, MPU_TAB_NAME ) == 0 )
            vTaskResume( MPU_handle );
        else if (strcmp( tab_name, MICROPHONE_TAB_NAME ) == 0 )
        {
            vTaskResume( mic_handle );
            vTaskResume( FFT_handle );
        } 
        else if ( strcmp( tab_name, LED_BAR_TAB_NAME ) == 0 )
        {
            vTaskSuspend( led_bar_animation_handle );
            vTaskResume( led_bar_solid_handle );
        }
        else if ( strcmp( tab_name, TOUCH_TAB_NAME ) == 0 )
        {
            reset_touch_bg();
            vTaskResume( touch_handle );
        }
        else if ( strcmp( tab_name, WIFI_TAB_NAME ) == 0 )
            vTaskResume( wifi_handle );
    }
}