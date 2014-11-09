/* ex: set tabstop=2 shiftwidth=2 expandtab cindent: */
#ifndef _TMR_REGION_H
#define _TMR_REGION_H
/**
 *  @file tmr_region.h
 *  @brief Mercury API - Region Definitions
 *  @author Brian Fiegel
 *  @date 4/18/2009
 */

/*
 * Copyright (c) 2009 ThingMagic, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifdef  __cplusplus
extern "C" {
#endif

/**
  * RFID regulatory regions
 */
typedef enum TMR_Region
{
  /** Unspecified region */         TMR_REGION_NONE = 0,
  /** North America */              TMR_REGION_NA   = 1,
  /** European Union */             TMR_REGION_EU   = 2,
  /** Korea */                      TMR_REGION_KR   = 3,
  /** India */                      TMR_REGION_IN   = 4,
  /** Japan */                      TMR_REGION_JP   = 5,
  /** People's Republic of China */ TMR_REGION_PRC  = 6,
  /** European Union 2 */           TMR_REGION_EU2  = 7,
  /** European Union 3 */           TMR_REGION_EU3  = 8,
  /** Korea 2*/                     TMR_REGION_KR2  = 9,
  /** People's Republic of China(840MHZ)*/TMR_REGION_PRC2 = 10,
  /** Australia */                  TMR_REGION_AU   = 11,
  /** New Zealand !!EXPERIMENTAL!! */ TMR_REGION_NZ   = 12,
  /** Open */                       TMR_REGION_OPEN = 0xFF
} TMR_Region;

/** A list of TMR_Region values */
typedef struct TMR_RegionList
{
  /** Array of TMR_Region values */
  TMR_Region *list;
  /** Allocated size of the array */
  uint8_t max;
  /** Length of the list - may be larger than max, indicating truncated data */
  uint8_t len;
} TMR_RegionList;

#ifdef  __cplusplus
}
#endif

#endif /* _TMR_REGION_H_ */
