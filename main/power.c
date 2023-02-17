/*
 * AWS IoT Kit - Core2 for AWS IoT Kit
 * Factory Firmware v2.3.0
 * power.c
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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"

#include "core2forAWS.h"

#include "power.h"

static void led_event_handler( lv_obj_t *obj, lv_event_t event );
static void vibration_event_handler( lv_obj_t *obj, lv_event_t event );
static void brightness_event_handler( lv_obj_t *slider, lv_event_t event );

static const char *TAG = POWER_TAB_NAME;

lv_obj_t *power_tab;
TaskHandle_t power_handle;

void display_power_tab( lv_obj_t *tv, lv_obj_t *core2forAWS_screen_obj )
{
    xSemaphoreTake( core2foraws_display_semaphore, portMAX_DELAY );

    power_tab = lv_tabview_add_tab( tv, POWER_TAB_NAME );  // Create a LVGL tabview

    /* Create the main body object and set background within the tab*/
    static lv_style_t bg_style;
    lv_obj_t *power_bg = lv_obj_create( power_tab, NULL );
    lv_obj_align( power_bg, NULL, LV_ALIGN_IN_TOP_LEFT, 16, 36 );
    lv_obj_set_size( power_bg, 290, 190 );
    lv_obj_set_click( power_bg, false );
    lv_style_init( &bg_style );
    lv_style_set_bg_color( &bg_style, LV_STATE_DEFAULT, lv_color_make( 255, 97, 56 ) );
    lv_obj_add_style( power_bg, LV_OBJ_PART_MAIN, &bg_style );

    /* Create the title within the main body object */
    static lv_style_t title_style;
    lv_style_init( &title_style );
    lv_style_set_text_font( &title_style, LV_STATE_DEFAULT, LV_THEME_DEFAULT_FONT_TITLE );
    lv_style_set_text_color( &title_style, LV_STATE_DEFAULT, LV_COLOR_BLACK );
    lv_obj_t *tab_title_label = lv_label_create( power_bg, NULL );
    lv_obj_add_style( tab_title_label, LV_OBJ_PART_MAIN, &title_style );
    lv_label_set_static_text( tab_title_label, "AXP192 Power Mgmt" );
    lv_obj_align( tab_title_label, power_bg, LV_ALIGN_IN_TOP_MID, 0, 10 );

    /* Create the sensor information label object */
    lv_obj_t *body_label = lv_label_create( power_bg, NULL );
    lv_label_set_long_mode( body_label, LV_LABEL_LONG_BREAK );
    lv_label_set_static_text( body_label, "The AXP192 provides power management for the battery and on-board peripherals.\n\nTap to toggle:" );
    lv_obj_set_width( body_label, 252 );
    lv_obj_align( body_label, power_bg, LV_ALIGN_IN_TOP_LEFT, 20, 40 );

    static lv_style_t body_style;
    lv_style_init( &body_style );
    lv_style_set_text_color( &body_style, LV_STATE_DEFAULT, LV_COLOR_BLACK );
    lv_obj_add_style( body_label, LV_OBJ_PART_MAIN, &body_style );

    /* Create the sensor information label object */
    static lv_style_t btn_style;
    lv_style_init( &btn_style );
    lv_style_set_pad_top( &btn_style, LV_STATE_DEFAULT, 10 );
    lv_style_set_pad_bottom( &btn_style, LV_STATE_DEFAULT, 10 );
    lv_style_set_pad_left( &btn_style, LV_STATE_DEFAULT, 12 );
    lv_style_set_pad_right( &btn_style, LV_STATE_DEFAULT, 12 );

    lv_obj_t *pwr_led_btn = lv_btn_create( power_bg, NULL );
    lv_obj_set_event_cb( pwr_led_btn, led_event_handler );
    lv_obj_set_size( pwr_led_btn, 76, 38 );
    lv_obj_align( pwr_led_btn, power_bg, LV_ALIGN_IN_BOTTOM_LEFT, 20, -14 );
    lv_btn_set_checkable( pwr_led_btn, true );
    lv_btn_toggle( pwr_led_btn );
    lv_btn_set_state( pwr_led_btn, LV_BTN_STATE_CHECKED_PRESSED );

    lv_obj_t *led_label = lv_label_create( pwr_led_btn, NULL );
    lv_label_set_static_text( led_label, "LED" );

    lv_obj_t *vibr_btn = lv_btn_create( power_bg, pwr_led_btn );
    lv_obj_set_event_cb( vibr_btn, vibration_event_handler );
    lv_obj_align( vibr_btn, power_bg, LV_ALIGN_IN_BOTTOM_MID, 0, -14 );

    lv_obj_t *vibr_label = lv_label_create( vibr_btn, NULL );
    lv_label_set_static_text( vibr_label, "Motor" );

    lv_obj_t *scrn_btn = lv_btn_create( power_bg, pwr_led_btn );
    lv_obj_set_event_cb( scrn_btn, brightness_event_handler );
    lv_obj_align( scrn_btn, power_bg, LV_ALIGN_IN_BOTTOM_RIGHT, -20, -14 );
    lv_btn_set_state( scrn_btn, LV_BTN_STATE_CHECKED_PRESSED );

    lv_obj_t *brightness_label = lv_label_create( scrn_btn, NULL );
    lv_label_set_static_text( brightness_label, "Screen" );

    xSemaphoreGive( core2foraws_display_semaphore );

    xTaskCreatePinnedToCore( battery_task, "batteryTask", configMINIMAL_STACK_SIZE * 2, ( void * ) core2forAWS_screen_obj, 0, &power_handle, 1 );
}

static void brightness_event_handler( lv_obj_t *obj, lv_event_t event )
{
    if ( event == LV_EVENT_VALUE_CHANGED )
    {
        uint8_t value = lv_btn_get_state( obj );

        if ( value == 0 )
            core2foraws_power_backlight_set( DISPLAY_BACKLIGHT_START / 2 );
        else
            core2foraws_power_backlight_set( DISPLAY_BACKLIGHT_START );
        
        ESP_LOGI( TAG, "Screen brightness: %x", value );
    }
}

static void led_event_handler( lv_obj_t *obj, lv_event_t event )
{
    if ( event == LV_EVENT_VALUE_CHANGED )
    {
        uint8_t value = lv_btn_get_state( obj );

        core2foraws_power_led_enable( value );
        ESP_LOGI( TAG, "LED state: %x", value );
    }
}

static void vibration_event_handler( lv_obj_t *obj, lv_event_t event )
{
    if ( event == LV_EVENT_VALUE_CHANGED )
    {
        uint8_t value = lv_btn_get_state( obj );

        if ( value == 0 )
            core2foraws_power_vibration_enable( 0 );
        else
            core2foraws_power_vibration_enable( 60 );
        
        ESP_LOGI( TAG, "Vibration motor state: %x", value );
    }
}

void battery_task( void *pvParameters )
{
    xSemaphoreTake( core2foraws_display_semaphore, portMAX_DELAY );
    lv_obj_t *battery_label = lv_label_create( ( lv_obj_t * )pvParameters, NULL );
    lv_label_set_text( battery_label, LV_SYMBOL_BATTERY_FULL );
    lv_label_set_recolor( battery_label, true );
    lv_label_set_align( battery_label, LV_LABEL_ALIGN_CENTER );
    lv_obj_align( battery_label, ( lv_obj_t * )pvParameters, LV_ALIGN_IN_TOP_RIGHT, -20, 10 );
    lv_obj_t *charge_label = lv_label_create( battery_label, NULL );
    lv_label_set_recolor( charge_label, true );
    lv_label_set_text( charge_label, "" );
    lv_obj_align( charge_label, battery_label, LV_ALIGN_CENTER, -4, 0 );
    xSemaphoreGive( core2foraws_display_semaphore );

    for( ; ; )
    {
        xSemaphoreTake( core2foraws_display_semaphore, portMAX_DELAY );
        float battery_voltage;
        core2foraws_power_batt_volts_get( &battery_voltage );
        if (battery_voltage >= 4.100)
        {
            lv_label_set_text(battery_label, "#0ab300 " LV_SYMBOL_BATTERY_FULL "#");
        } 
        else if ( battery_voltage >= 3.95)
        {
            lv_label_set_text(battery_label, "#0ab300 " LV_SYMBOL_BATTERY_3 "#");
        }
        else if ( battery_voltage >= 3.80)
        {
            lv_label_set_text(battery_label, "#ff9900 " LV_SYMBOL_BATTERY_2 "#");
        }
        else if ( battery_voltage >= 3.25)
        {
            lv_label_set_text(battery_label, "#ff0000 " LV_SYMBOL_BATTERY_1 "#");
        }
        else
        {
            lv_label_set_text(battery_label, "#ff0000 " LV_SYMBOL_BATTERY_EMPTY "#");
        }

        bool charging;
        core2foraws_power_plugged_get( &charging );
        if ( charging )
        {
            lv_label_set_text( charge_label, "#0000cc " LV_SYMBOL_CHARGE "#" );
        }
        else
        {
            lv_label_set_text( charge_label, "" );
        }
        xSemaphoreGive( core2foraws_display_semaphore );
        vTaskDelay( pdMS_TO_TICKS( 200 ) );
    }

    vTaskDelete( NULL ); // Should never get to here...
}