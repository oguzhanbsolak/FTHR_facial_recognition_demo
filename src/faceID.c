/******************************************************************************
 * Copyright (C) 2022 Maxim Integrated Products, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 *
 ******************************************************************************/
#include <string.h>

#include "board.h"
#include "mxc_device.h"
#include "mxc_delay.h"
#include "mxc.h"
#include "utils.h"
#include "camera.h"
#include "faceID.h"
#include "facedetection.h"
#include "embeddings.h"
#include "MAXCAM_Debug.h"
#include "cnn_1.h"
#include "cnn_2.h"
#include "cnn_3.h"
#ifdef BOARD_FTHR_REVA
#include "tft_ili9341.h"
#endif
#include "led.h"
#include "uart.h"
#include "math.h"
#include "post_process.h"
#define S_MODULE_NAME "faceid"

#define PRINT_TIME 1

extern area_t area;
/************************************ VARIABLES ******************************/
extern volatile char names[1024][7];
extern volatile uint32_t cnn_time; // Stopwatch

static void run_cnn_2(void);

#ifdef TFT_ENABLE
#ifdef BOARD_FTHR_REVA
static int font = (int)&SansSerif16x16[0];
#endif
#endif //#ifdef TFT_ENABLE


extern uint8_t box[4]; // x1, y1, x2, y2
volatile int32_t output_buffer[16];
char* name;
static int32_t ml_3_data32[(CNN_3_NUM_OUTPUTS + 3) / 4];

int face_id(void)
{
#if (PRINT_TIME == 1)
    /* Get current time */
    uint32_t process_time = utils_get_time_ms();
    uint32_t total_time = utils_get_time_ms();
    uint32_t weight_load_time = 0;
#endif

    /* Check for received image */
    if (camera_is_image_rcv()) {
#if (PRINT_TIME == 1)
        process_time = utils_get_time_ms();
#endif

        // Enable CNN peripheral, enable CNN interrupt, turn on CNN clock
        // CNN clock: 50 MHz div 1
        cnn_enable(MXC_S_GCR_PCLKDIV_CNNCLKSEL_PCLK, MXC_S_GCR_PCLKDIV_CNNCLKDIV_DIV1);
        weight_load_time = utils_get_time_ms();
        /* Configure CNN 2 to recognize a face */
        cnn_2_init(); // Bring state machine into consistent state
        cnn_2_load_weights_from_SD(); // Load CNN kernels from SD card
        PR_DEBUG("CNN weight load time: %dms", utils_get_time_ms() - weight_load_time);
        cnn_2_load_bias(); // Reload CNN bias
        PR_DEBUG("CNN bias load time");
        cnn_2_configure(); // Configure state machine
        PR_DEBUG("CNN configure");

        /* Run CNN */
        run_cnn_2();
        PR_DEBUG("CNN RUN");

#if (PRINT_TIME == 1)
        PR_INFO("Process Time Total : %dms", utils_get_time_ms() - process_time);
#endif

#if (PRINT_TIME == 1)
        PR_INFO("Capture Time : %dms", process_time - total_time);
        PR_INFO("Total Time : %dms", utils_get_time_ms() - total_time);
        total_time = utils_get_time_ms();
#endif
    }

    return 0;
}

static void run_cnn_2(void)
{
    uint32_t imgLen;
    uint32_t w, h;
    /* Get current time */
    uint32_t pass_time = 0;
    uint8_t *raw;
    float y_prime;
    float x_prime;
    uint8_t x_loc;
    uint8_t y_loc;
    uint8_t x1 =  MAX(box[1], 0); 
    uint8_t y1 =  MAX(box[0], 0); 
    float sum = 0;
    float b;
    //uint8_t send_package[3];
    //uint8_t* snd_ptr = send_package;

    uint8_t box_width = box[2] - box[0];
    uint8_t box_height = box[3] - box[1];
    // Get the details of the image from the camera driver.
    camera_get_image(&raw, &imgLen, &w, &h);

    pass_time = utils_get_time_ms();
    cnn_start();

    //PR_INFO("CNN initialization time : %d", utils_get_time_ms() - pass_time);

    uint8_t *data = raw;

    //pass_time = utils_get_time_ms();

    PR_DEBUG("x1: %d, y1: %d", x1, y1);

    // Resize image inside facedetection box to 160x120 and load to CNN memory
    for (int i = 0; i < HEIGHT_ID; i++) {
        y_prime = ((float)(i) / HEIGHT_ID) * box_height;
        y_loc = (uint8_t)(MIN(round(y_prime), box_height - 1)); // + y1;
        data = raw + y1 * IMAGE_W * BYTE_PER_PIXEL + y_loc * BYTE_PER_PIXEL;
        data += x1 * BYTE_PER_PIXEL;
        for (int j = 0; j < WIDTH_ID; j++) {
            uint8_t ur, ug, ub;
            int8_t r, g, b;
            uint32_t number;
            x_prime = ((float)(j) / WIDTH_ID) * box_width;
            x_loc = (uint8_t)(MIN(round(x_prime), box_width - 1));

            ub = (uint8_t)(data[x_loc * BYTE_PER_PIXEL * IMAGE_W + 1] << 3);
            ug = (uint8_t)((data[x_loc * BYTE_PER_PIXEL * IMAGE_W] << 5) |
                           ((data[x_loc * BYTE_PER_PIXEL * IMAGE_W + 1] & 0xE0) >> 3));
            ur = (uint8_t)(data[x_loc * BYTE_PER_PIXEL * IMAGE_W] & 0xF8);

            b = ub - 128;
            g = ug - 128;
            r = ur - 128;

            number = 0x00FFFFFF & ((((uint8_t)b) << 16) | (((uint8_t)g) << 8) | ((uint8_t)r));
            // Loading data into the CNN fifo
            // Remove the following line if there is no risk that the source would overrun the FIFO:
            //PR_DEBUG("number: %d", i);
            
            while (((*((volatile uint32_t *)0x50000004) & 1)) != 0)
                ; // Wait for FIFO 0

            *((volatile uint32_t *)0x50000008) = number; // Write FIFO 0
        }
    }

    int cnn_load_time = utils_get_time_ms() - pass_time;
    PR_DEBUG("CNN load data time : %d", cnn_load_time);

    pass_time = utils_get_time_ms();

    // Disable Deep Sleep mode
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    // CNN interrupt wakes up CPU from sleep mode
    while (cnn_time == 0) {
        asm volatile("wfi"); // Sleep and wait for CNN interrupt
    }

    PR_DEBUG("CNN wait time : %d", utils_get_time_ms() - pass_time);

    pass_time = utils_get_time_ms();

    cnn_2_unload((uint32_t *)(output_buffer));

    cnn_disable();

    uint32_t value;
    int8_t pr_value;
    float n1, n2, n3, n4;
    //Calculate vector sum
    for (int i =0; i < 16; i++) {
        value = output_buffer[i];
        pr_value = (int8_t)(value & 0xff);
        sum += pr_value * pr_value;
        pr_value = (int8_t)((value >> 8) & 0xff);
        sum += pr_value * pr_value;
        pr_value = (int8_t)((value >> 16) & 0xff);
        sum += pr_value * pr_value;
        pr_value = (int8_t)((value >> 24) & 0xff);
        sum += pr_value * pr_value;
    }
    b = 1/sqrt(sum);

    for (int i =0; i < 16; i++) {
        value = output_buffer[i];
        pr_value = (int8_t)(value & 0xff);
        n1 = 128 * pr_value * b;
        if (n1 < 0) {
            n1 = 256 + n1;           
        }

        pr_value = (int8_t)((value >> 8) & 0xff);
        n2 = 128 * pr_value * b;
        if (n2 < 0) {
            n2 = 256 + n2;           
        }

        pr_value = (int8_t)((value >> 16) & 0xff);
        n3 = 128 * pr_value * b;
        if (n3 < 0) {
            n3 = 256 + n3;           
        }
        pr_value = (int8_t)((value >> 24) & 0xff);
        n4 = 128 * pr_value * b;
        if (n4 < 0) {
            n4 = 256 + n4;           
        }
        #ifdef UNNORMALIZE_RECORD
            norm_output_buffer[i] = (((uint8_t)n4) << 24) | (((uint8_t)n3) << 16) | (((uint8_t)n2) << 8) | ((uint8_t)n1);
        #else
            output_buffer[i] = (((uint8_t)n4) << 24) | (((uint8_t)n3) << 16) | (((uint8_t)n2) << 8) | ((uint8_t)n1);
        #endif
    }
    cnn_enable(MXC_S_GCR_PCLKDIV_CNNCLKSEL_PCLK, MXC_S_GCR_PCLKDIV_CNNCLKDIV_DIV1);
    cnn_3_init(); // Bring CNN state machine into consistent state
    cnn_3_load_weights(); // Load CNN kernels
    cnn_3_load_bias(); // Load CNN bias
    cnn_3_configure();
    //load input
    #ifdef UNNORMALIZE_RECORD
        uint32_t *out_point = (uint32_t *)norm_output_buffer; // For unnormalized emb. experiment
    #else
        uint32_t *out_point = (uint32_t *)output_buffer;
    #endif

    memcpy32((uint32_t *) 0x50400000, out_point, 1);
    out_point++;
    memcpy32((uint32_t *) 0x50408000, out_point, 1);
    out_point++;
    memcpy32((uint32_t *) 0x50410000, out_point, 1);
    out_point++;
    memcpy32((uint32_t *) 0x50418000, out_point, 1);
    out_point++;
    memcpy32((uint32_t *) 0x50800000, out_point, 1);
    out_point++;
    memcpy32((uint32_t *) 0x50808000, out_point, 1);
    out_point++;
    memcpy32((uint32_t *) 0x50810000, out_point, 1);
    out_point++;
    memcpy32((uint32_t *) 0x50818000, out_point, 1);
    out_point++;
    memcpy32((uint32_t *) 0x50c00000, out_point, 1);
    out_point++;
    memcpy32((uint32_t *) 0x50c08000, out_point, 1);
    out_point++;
    memcpy32((uint32_t *) 0x50c10000, out_point, 1);
    out_point++;
    memcpy32((uint32_t *) 0x50c18000, out_point, 1);
    out_point++;
    memcpy32((uint32_t *) 0x51000000, out_point, 1);
    out_point++;
    memcpy32((uint32_t *) 0x51008000, out_point, 1);
    out_point++;
    memcpy32((uint32_t *) 0x51010000, out_point, 1);
    out_point++;
    memcpy32((uint32_t *) 0x51018000, out_point, 1);
    out_point++;
    cnn_3_start(); // Start CNN_3
    while (cnn_time == 0);
        //    MXC_LP_EnterSleepMode(); // Wait for CNN_3, but not sleep, ISR can't catch it otherwise
    cnn_3_unload((uint32_t *) ml_3_data32);
    cnn_disable();

    PR_DEBUG("CNN unload time : %d", utils_get_time_ms() - pass_time);

    pass_time = utils_get_time_ms();


    uint32_t *ml_point =  ml_3_data32;
    int8_t max_emb = 0;
    int32_t max_emb_index = 0;
    char* name;

     for (int i = 0; i < DEFAULT_EMBS_NUM; i++)
    {   
        value = *ml_point;
        pr_value = value & 0xff;
        if ((int8_t)pr_value > max_emb)
        {
            max_emb = (int8_t)pr_value;
            max_emb_index = i*4;
        }
        pr_value = (value >> 8) & 0xff;
        if ((int8_t)pr_value > max_emb)
        {
            max_emb = (int8_t)pr_value;
            max_emb_index = (i*4)+1;
                }
        pr_value = (value >> 16) & 0xff;
        if ((int8_t)pr_value > max_emb)
        {
            max_emb = (int8_t)pr_value;
            max_emb_index = (i*4)+2;
            }
        pr_value = (value >> 24) & 0xff;
        if ((int8_t)pr_value > max_emb)
        {
            max_emb = (int8_t)pr_value;
            max_emb_index = (i*4)+3;
        }
        ml_point++;
    }
    PR_DEBUG("FaceID inference time: %d ms\n", utils_get_time_ms() - pass_time);
    PR_DEBUG("CNN_3 max value: %d \n", max_emb);
    PR_DEBUG("CNN_3 max value index: %d \n", max_emb_index);
    if (max_emb > Threshold)
    {
        PR_DEBUG("subject id: %d \n", max_emb_index);
        name =  (char*)names[max_emb_index];
        PR_DEBUG("subject name: %s \n", name);
            }
    else
    {
        PR_DEBUG("Unknown subject \n");
        name =  "Unknown";
            }

    

#ifdef TFT_ENABLE
        text_t printResult;
        printResult.data = name;
        printResult.len = strlen(name);
        //Rotate before print text
        MXC_TFT_SetRotation(ROTATE_180);
        MXC_TFT_ClearArea(&area, 4);        
        MXC_TFT_PrintFont(CAPTURE_X, CAPTURE_Y, font, &printResult, NULL);
        MXC_TFT_SetRotation(ROTATE_270);

#endif
    
}
