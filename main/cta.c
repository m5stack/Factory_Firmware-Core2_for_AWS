/*
 * AWS IoT Kit - Core2 for AWS IoT Kit
 * Factory Firmware v2.3.0
 * home.c
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
#include "freertos/semphr.h"

#include "esp_log.h"

#include "core2forAWS.h"

#include "cta.h"

static const char *TAG = CTA_TAB_NAME;

void display_cta_tab( lv_obj_t *tv )
{
    xSemaphoreTake( core2foraws_display_semaphore, portMAX_DELAY );   // Takes the core2foraws_display_semaphore mutex. This blocks any other task attempting to take it before it's free'd from executing.
    
    lv_obj_t *cta_tab = lv_tabview_add_tab( tv, CTA_TAB_NAME );   // Create a LVGL tabview

    /* Create the main body object and set background within the tab*/
    lv_obj_t *cta_bg = lv_obj_create( cta_tab, NULL );
    lv_obj_align( cta_bg, NULL, LV_ALIGN_IN_TOP_LEFT, 16, 36 );
    lv_obj_set_size( cta_bg, 290, 190 );
    lv_obj_set_click( cta_bg, false );
    
    /* Create the title within the tab */
    static lv_style_t title_style;
    lv_style_init( &title_style );
    lv_style_set_text_font( &title_style, LV_STATE_DEFAULT, LV_THEME_DEFAULT_FONT_TITLE );
    lv_style_set_text_color( &title_style, LV_STATE_DEFAULT, LV_COLOR_WHITE );
    
    lv_obj_t *tab_title_label = lv_label_create( cta_tab, NULL );
    lv_obj_add_style( tab_title_label, LV_OBJ_PART_MAIN, &title_style );
    lv_label_set_static_text( tab_title_label, "Next Steps" );
    lv_label_set_align( tab_title_label, LV_LABEL_ALIGN_CENTER );
    lv_obj_align( tab_title_label, cta_tab, LV_ALIGN_IN_TOP_MID, 0, 50 );

    /* Create the main body object and set background within the tab*/
    static lv_style_t bg_style;
    lv_style_init( &bg_style );
    lv_style_set_bg_color( &bg_style, LV_STATE_DEFAULT, lv_color_make( 35, 47, 62 ) );
    lv_obj_add_style( cta_bg, LV_OBJ_PART_MAIN, &bg_style );

    lv_obj_t *cta_label = lv_label_create( cta_tab, NULL );
    lv_label_set_long_mode( cta_label, LV_LABEL_LONG_BREAK );
    lv_label_set_text( cta_label, "Get hands-on experience building IoT solutions and learn about the AWS IoT Kit program:" );
    lv_obj_set_width( cta_label, 260 );
    lv_obj_align( cta_label, cta_tab, LV_ALIGN_CENTER, 0 , 10 );

    static lv_style_t cta_style;
    lv_style_init( &cta_style );
    lv_style_set_text_color( &cta_style, LV_STATE_DEFAULT, LV_COLOR_WHITE );
    lv_obj_add_style( cta_label, LV_OBJ_PART_MAIN, &cta_style );

    /* Create the title within the tab */
    static lv_style_t url_style;
    lv_style_init( &url_style );
    lv_style_set_text_color( &url_style, LV_STATE_DEFAULT, lv_color_make( 255, 153, 0 ) );
    lv_style_set_text_font( &url_style, LV_STATE_DEFAULT, LV_THEME_DEFAULT_FONT_TITLE );

    lv_obj_t *url_label = lv_label_create( cta_tab, NULL );
    lv_obj_add_style( url_label, LV_OBJ_PART_MAIN, &url_style );
    lv_label_set_text( url_label, "https://aws-iot-kit-docs.m5stack.com/en/" );
    lv_obj_align( url_label, cta_tab, LV_ALIGN_IN_BOTTOM_MID, 0, -40 );
    
    xSemaphoreGive( core2foraws_display_semaphore );
}