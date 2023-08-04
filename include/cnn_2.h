/**************************************************************************************************
* Copyright (C) 2019-2021 Maxim Integrated Products, Inc. All Rights Reserved.
*
* Maxim Integrated Products, Inc. Default Copyright Notice:
* https://www.maximintegrated.com/en/aboutus/legal/copyrights.html
**************************************************************************************************/

/*
 * This header file was automatically @generated for the mobface_112_85 network from a template.
 * Please do not edit; instead, edit the template and regenerate.
 */

#ifndef __CNN_2_H__
#define __CNN_2_H__

/*
  SUMMARY OF OPS
  Hardware: 199,817,408 ops (198,051,840 macc; 1,746,752 comp; 18,816 add; 0 mul; 0 bitwise)
    Layer 0: 11,239,424 ops (10,838,016 macc; 401,408 comp; 0 add; 0 mul; 0 bitwise)
    Layer 1: 29,403,136 ops (28,901,376 macc; 501,760 comp; 0 add; 0 mul; 0 bitwise)
    Layer 2: 58,003,456 ops (57,802,752 macc; 200,704 comp; 0 add; 0 mul; 0 bitwise)
    Layer 3: 21,876,736 ops (21,676,032 macc; 200,704 comp; 0 add; 0 mul; 0 bitwise)
    Layer 4: 7,375,872 ops (7,225,344 macc; 150,528 comp; 0 add; 0 mul; 0 bitwise)
    Layer 5: 21,826,560 ops (21,676,032 macc; 150,528 comp; 0 add; 0 mul; 0 bitwise)
    Layer 6: 1,630,720 ops (1,605,632 macc; 25,088 comp; 0 add; 0 mul; 0 bitwise)
    Layer 7: 14,450,688 ops (14,450,688 macc; 0 comp; 0 add; 0 mul; 0 bitwise)
    Layer 8: 0 ops (0 macc; 0 comp; 0 add; 0 mul; 0 bitwise)
    Layer 9: 12,544 ops (0 macc; 0 comp; 12,544 add; 0 mul; 0 bitwise)
    Layer 10: 3,261,440 ops (3,211,264 macc; 50,176 comp; 0 add; 0 mul; 0 bitwise)
    Layer 11: 10,888,192 ops (10,838,016 macc; 50,176 comp; 0 add; 0 mul; 0 bitwise)
    Layer 12: 912,576 ops (903,168 macc; 9,408 comp; 0 add; 0 mul; 0 bitwise)
    Layer 13: 10,838,016 ops (10,838,016 macc; 0 comp; 0 add; 0 mul; 0 bitwise)
    Layer 14: 809,088 ops (802,816 macc; 6,272 comp; 0 add; 0 mul; 0 bitwise)
    Layer 15: 7,225,344 ops (7,225,344 macc; 0 comp; 0 add; 0 mul; 0 bitwise)
    Layer 16: 22,656 ops (16,384 macc; 0 comp; 6,272 add; 0 mul; 0 bitwise)
    Layer 17: 8,192 ops (8,192 macc; 0 comp; 0 add; 0 mul; 0 bitwise)
    Layer 18: 32,768 ops (32,768 macc; 0 comp; 0 add; 0 mul; 0 bitwise)

  RESOURCE USAGE
  Weight memory: 398,176 bytes out of 442,368 bytes total (90.0%)
  Bias memory:   1,296 bytes out of 2,048 bytes total (63.3%)
*/

/* Number of outputs for this network */
#define CNN_2_NUM_OUTPUTS 512

#define Threshold 63

/* Perform minimum accelerator initialization so it can be configured */
int cnn_2_init(void);

/* Configure accelerator for the given network */
int cnn_2_configure(void);

/* Load accelerator weights */
int cnn_2_load_weights_from_SD(void);

/* Verify accelerator weights (debug only) */
int cnn_2_verify_weights(void);

/* Load accelerator bias values (if needed) */
int cnn_2_load_bias(void);

/* Unload results from accelerator */
int cnn_2_unload(uint32_t *out_buf);

#endif // __CNN_2_H__
