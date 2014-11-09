/**
 *  @file osdep_win32.c
 *  @brief Mercury API - Windows platform implementation of OS dependencies
*/

/*
 * Copyright (c) 2010 ThingMagic, Inc.
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

#include <windows.h>
#include <time.h>
#include "osdep.h"

uint64_t
tmr_gettime()
{
  uint64_t totalms;
  SYSTEMTIME st;
  FILETIME ft;
  LARGE_INTEGER li;    
  GetSystemTime(&st);
  SystemTimeToFileTime(&st, &ft);
  li.LowPart = ft.dwLowDateTime;
  li.HighPart = ft.dwHighDateTime;
  totalms=(((uint64_t)li.LowPart)>>32 || ((uint64_t)(li.HighPart)))/10000;
  return totalms;
}

uint32_t
tmr_gettime_low()
{
  SYSTEMTIME st;
  FILETIME ft;
  GetSystemTime(&st);
  SystemTimeToFileTime(&st, &ft);
  return ft.dwLowDateTime;
}

uint32_t
tmr_gettime_high()
{
  SYSTEMTIME st;
  FILETIME ft;
  GetSystemTime(&st);
  SystemTimeToFileTime(&st, &ft);
  return ft.dwHighDateTime;
}

void
tmr_sleep(uint32_t sleepms)
{
  Sleep(sleepms);
}

TMR_TimeStructure 
tmr_gettimestructure()
{ 
  SYSTEMTIME st;
  TMR_TimeStructure timestructure;

  GetSystemTime(&st);

  timestructure.tm_year = (uint32_t)st.wYear;
  timestructure.tm_mon = (uint32_t)st.wMonth;
  timestructure.tm_mday = (uint32_t)st.wDay;
  timestructure.tm_hour = (uint32_t)st.wHour;
  timestructure.tm_min = (uint32_t)st.wMinute;
  timestructure.tm_sec = (uint32_t)st.wSecond;
  return timestructure;  
}
