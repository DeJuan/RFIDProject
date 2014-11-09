/* ex: set tabstop=2 shiftwidth=2 expandtab cindent: */
#ifndef _TMR_READ_PLAN_H
#define _TMR_READ_PLAN_H
/** 
 *  @file tmr_read_plan.h
 *  @brief Mercury API - Read Plan Definitions
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

#include "tmr_tag_protocol.h"
#include "tmr_filter.h"
#include "tmr_tagop.h"

#ifdef  __cplusplus
extern "C" {
#endif


/**
 * @defgroup readplan Read plans
 *
 * A read plan specifies the antennas, protocols, and filters to use
 * for a search (read). Each ReadPlan structure has a numeric weight
 * that controls what fraction of a search is used by that plan when
 * combined in a MultiReadPlan (see below). Read plans are specified
 * for the reader in the @c /reader/read/plan parameter.
 *
 * @{
 */

typedef struct TMR_ReadPlan TMR_ReadPlan;
typedef struct TMR_SimpleReadPlan TMR_SimpleReadPlan;
typedef struct TMR_MultiReadPlan TMR_MultiReadPlan;
typedef struct TMR_StopOnTagCount TMR_StopOnTagCount;
typedef struct TMR_TagObservationTrigger TMR_TagObservationTrigger;
typedef struct TMR_StopTrigger TMR_StopTrigger;

/**
 * TODO: to be used later.
 **/ 
/*struct TMR_TagObservationTrigger
{
};

struct TMR_StopTrigger
{
  TMR_TagObservationTrigger stopTrigger; 
};*/

/**
 * A StopOnTagCount will be used in case of stop N trigger option.
 * It contains a flag to specify that user is requesting for stop
 * N trigger and a count to specify the number of tags user is 
 * requesting to read.
 **/
struct TMR_StopOnTagCount
{
  /* option for stop N trigger */
  bool stopNTriggerStatus;

  /* Number of tags to be read */
  uint32_t noOfTags;
};

/**
 * A SimpleReadPlan contains a protocol, a list of antennas, and an
 * optional filter. The list of antennas may be an empty list, in
 * which case the reader will use all antennas in the antenna map (see
 * @c /reader/antenna/txRxMap) where the reader has detected an
 * antenna present. The filter describes any selection or filtering
 * operation to perform in the RFID protocol during the search. The
 * filter may be @c NULL, in which case no selection or filtering is
 * performed. Invalid combinations of protocols and filters (a Gen2
 * select on ISO180006B, for example) will produce an error at read
 * time.
 */
struct TMR_SimpleReadPlan
{
  /** The list of antennas to read on */
  TMR_uint8List antennas;
  /** The protocol to use for reading */
  TMR_TagProtocol protocol;
  /** The filter to apply to reading, or NULL */
  TMR_TagFilter *filter;
  /** The tag operation to apply to each read tag, or NULL */
  TMR_TagOp *tagop;
  /** Option to use the FastSearch */
  bool useFastSearch;
  /** The stop N trigger */
  TMR_StopOnTagCount stopOnCount;
};

/**
 * A MultiReadPlan contains a list of other ReadPlan objects. The
 * relative weights of each of the included sub-plans are used
 * determine what fraction of the total read time to allot to that
 * sub-plan (for example, if the first plan has a weight of 20 and the
 * second has a weight of 10, the first 2/3 of any read will use the
 * first plan, and the remaining 1/3 will use the second
 * plan). MultiReadPlan is useful for specifying searches over
 * multiple protocols, for using different filters on different
 * antennas, and other combinations.
 */
struct TMR_MultiReadPlan
{
  /** Array of pointers to the subsidiary read plans */
  TMR_ReadPlan **plans;
  uint32_t totalWeight; /** Internal value - initialize to 0 */
  /** Number of elements in the array of read plans */
  uint8_t planCount;
};

/** The type of a read plan */
typedef enum TMR_ReadPlanType
{
  TMR_READ_PLAN_TYPE_INVALID,
  /** Simple read plan - one protocol, a set of antennas, an optional
   * tag filter, and an optional tag operation.
   */
  TMR_READ_PLAN_TYPE_SIMPLE,
  /** Multi-read plan - a list of read plans (simple or multi). */
  TMR_READ_PLAN_TYPE_MULTI
} TMR_ReadPlanType;

/**
 * A ReadPlan structure specifies the antennas, protocols, and filters
 * to use for a search (read).
 */
struct TMR_ReadPlan
{
  /** The type of the read plan and the type of the union that is populated */
  TMR_ReadPlanType type;
  /** The relative weight of this read plan */
  uint32_t weight;
  union
  {
    /** SimpleReadPlan contents */
    TMR_SimpleReadPlan simple;
    /** MultiReadPlan contents */
    TMR_MultiReadPlan multi;
  } u;
};

TMR_Status TMR_RP_init_simple(TMR_ReadPlan *plan, uint8_t antennaCount,
                              uint8_t *antennaList, TMR_TagProtocol protocol,
                              uint32_t weight);

TMR_Status TMR_RP_init_multi(TMR_ReadPlan *plan, TMR_ReadPlan **plans,
                             uint8_t planCount, uint32_t weight);

TMR_Status TMR_RP_set_filter(TMR_ReadPlan *plan, TMR_TagFilter *filter);

TMR_Status TMR_RP_set_tagop(TMR_ReadPlan *plan, TMR_TagOp *tagop);

TMR_Status TMR_RP_set_useFastSearch(TMR_ReadPlan *plan, bool useFastSearch);
TMR_Status TMR_RP_set_stopTrigger(TMR_ReadPlan *plan, uint32_t count);
/**
 * @}
 */

#ifdef  __cplusplus
}
#endif

#endif /* _TMR_READ_PLAN_H_ */
