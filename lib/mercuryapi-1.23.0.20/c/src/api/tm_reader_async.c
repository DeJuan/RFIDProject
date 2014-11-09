/**
 *  @file tm_reader_async.c
 *  @brief Mercury API - background reading implementation
 *  @author Nathan Williams
 *  @date 11/18/2009
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
#include "tm_config.h"
#ifdef TMR_ENABLE_BACKGROUND_READS

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <stdio.h>

#ifndef WIN32
#include <sys/time.h>
#endif

#include "tm_reader.h"
#include "serial_reader_imp.h"
#ifdef TMR_ENABLE_LLRP_READER
#include "llrp_reader_imp.h"
#endif
#include "osdep.h"
#include "tmr_utils.h"

static void *do_background_reads(void *arg);
static void *parse_tag_reads(void *arg);
static void process_async_response(TMR_Reader *reader);

TMR_Status
TMR_startReading(struct TMR_Reader *reader)
{
  int ret;
  bool createParser = true;

  if (TMR_READER_TYPE_SERIAL == reader->readerType)
  {
#ifdef TMR_ENABLE_SERIAL_READER
    /**
     * Currently we are not supporting stop N trigger for async read case.
     * This is also true for pseudo continuous case as well. Pop up the error
     * if user is trying to do so.
     **/
    if (TMR_READ_PLAN_TYPE_MULTI == reader->readParams.readPlan->type)
    {
      uint8_t loop = 0;
      TMR_MultiReadPlan *multi;

      multi = &reader->readParams.readPlan->u.multi;

      for (loop = 0; loop < multi->planCount; loop++)
      {
        if (multi->plans[loop]->u.simple.stopOnCount.stopNTriggerStatus)
        {
          /* Not supporting stop N trigger */
          return TMR_ERROR_UNSUPPORTED; 
        }
      }
    }
    else if (TMR_READ_PLAN_TYPE_SIMPLE == reader->readParams.readPlan->type)
    {
      if (reader->readParams.readPlan->u.simple.stopOnCount.stopNTriggerStatus)
      {
        /* Not supporting stop N trigger */
        return TMR_ERROR_UNSUPPORTED;
      }
    }
    else
    {
      /* do nothing */
    }

    /**
	  * if model is M6e and it's varient
	  * asyncOffTime == 0
	  * only then use streaming
	  */
    if (
        ((TMR_SR_MODEL_M6E == reader->u.serialReader.versionInfo.hardware[0])||
         (TMR_SR_MODEL_M6E_PRC == reader->u.serialReader.versionInfo.hardware[0]) ||
         (TMR_SR_MODEL_M6E_MICRO == reader->u.serialReader.versionInfo.hardware[0])) &&
        (reader->readParams.asyncOffTime == 0) &&
        ((TMR_READ_PLAN_TYPE_SIMPLE == reader->readParams.readPlan->type) || 
         ((TMR_READ_PLAN_TYPE_MULTI == reader->readParams.readPlan->type)  && 
          (compareAntennas(&reader->readParams.readPlan->u.multi))))
       )
    {
      /* Do nothing */
    }
    else
    {
      createParser = false;
    }
#else
    return TMR_ERROR_UNSUPPORTED;
#endif/* TMR_ENABLE_SERIAL_READER */    
  }
#ifdef TMR_ENABLE_LLRP_READER
  if (TMR_READER_TYPE_LLRP == reader->readerType)
  {
    /**
     * In case of LLRP reader and continuous reading, disable the
     * LLRP background receiver.
     **/
    TMR_LLRP_setBackgroundReceiverState(reader, false);
    /**
     * Note the keepalive start time
     * Keepalive monitoring happens only 
     * for async reads.
     **/
    reader->u.llrpReader.ka_start = tmr_gettime();
  }
#endif

  /**
   * Initialize read_started semaphore
   **/
  pthread_mutex_lock(&reader->backgroundLock);
  reader->readState = TMR_READ_STATE_STARTING;
  pthread_cond_broadcast(&reader->readCond);
  pthread_mutex_unlock(&reader->backgroundLock);

  if (true == createParser)
  {
    /** Background parser thread initialization
     *
     * Only M6e supports Streaming, and in case of other readers
     * we still use pseudo-async mechanism for continuous read.
     * To achieve continuous reading, create a parser thread
     */
    pthread_mutex_lock(&reader->parserLock);
    
    if (false == reader->parserSetup)
    {
      ret = pthread_create(&reader->backgroundParser, NULL,
                       parse_tag_reads, reader);
      if (0 != ret)
      {
        pthread_mutex_unlock(&reader->parserLock);
        return TMR_ERROR_NO_THREADS;
      }
      pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
      pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
      pthread_detach(reader->backgroundParser);
    /** Initialize semaphores only for the first time
     *  These semaphores are used only in case of streaming
     */
      reader->queue_depth = 0;
      sem_init(&reader->queue_length, 0, 0);
      sem_init(&reader->queue_slots, 0, TMR_MAX_QUEUE_SLOTS);
      reader->parserSetup = true;
    }

    reader->parserEnabled = true;


    /* Enable streaming */
    reader->continuousReading = true;
    reader->finishedReading = false;
    pthread_cond_signal(&reader->parserCond);
    pthread_mutex_unlock(&reader->parserLock);
  }

  /* Background reader thread initialization */
  pthread_mutex_lock(&reader->backgroundLock);

  if (false == reader->backgroundSetup)
  {
    ret = pthread_create(&reader->backgroundReader, NULL,
                         do_background_reads, reader);
    if (0 != ret)
    {
      pthread_mutex_unlock(&reader->backgroundLock);
      return TMR_ERROR_NO_THREADS;
    }
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    reader->backgroundSetup = true;
  }

  reader->backgroundEnabled = true;
  reader->searchStatus = true;

#ifdef TMR_ENABLE_SERIAL_READER    
  if (TMR_READER_TYPE_SERIAL == reader->readerType)
  {
    reader->u.serialReader.tagopFailureCount = 0;
    reader->u.serialReader.tagopSuccessCount = 0;
  }
#endif/* TMR_ENABLE_SERIAL_READER */    
  pthread_cond_signal(&reader->backgroundCond);
  pthread_mutex_unlock(&reader->backgroundLock);

  /* End of Background reader thread initialization */

  return TMR_SUCCESS;
}

static void
reset_continuous_reading(struct TMR_Reader *reader, bool dueToError)
{
  if (true == reader->continuousReading)
  {
#ifdef TMR_ENABLE_LLRP_READER
    if (TMR_READER_TYPE_LLRP == reader->readerType)
    {
      /**
       * In case of LLRP reader, re-enable the
       * LLRP background receiver as continuous reading is finished
       **/
      TMR_LLRP_setBackgroundReceiverState(reader, true);
    }
#endif

#ifdef TMR_ENABLE_SERIAL_READER      
    if ((false == dueToError) && (TMR_READER_TYPE_SERIAL == reader->readerType))
    {
      /**
       * Disable filtering on module
       **/
      TMR_Status ret;
      bool value = reader->u.serialReader.enableReadFiltering;
      ret = TMR_SR_cmdSetReaderConfiguration(reader, TMR_SR_CONFIGURATION_ENABLE_READ_FILTER, &value);
      if (TMR_SUCCESS != ret)
      {
        notify_exception_listeners(reader, ret);
      }
    }
#endif/* TMR_ENABLE_SERIAL_READER */      

    /* disable streaming */
    reader->continuousReading = false;
  }
}

TMR_Status
TMR_stopReading(struct TMR_Reader *reader)
{
  TMR_Status ret;

  ret = TMR_SUCCESS;

  /* Check if background setup is active */
  pthread_mutex_lock(&reader->backgroundLock);

  if (false == reader->backgroundSetup)
  {
    pthread_mutex_unlock(&reader->backgroundLock);
    return TMR_SUCCESS;
  }

  if (false == reader->searchStatus)
  {
    /**
     * searchStatus is false, i.e., reading is already
     * stopped. Returen success.
     **/
    pthread_mutex_unlock(&reader->backgroundLock);
    return TMR_SUCCESS;
  }
  /**
   * Else, read is in progress. Set
   * searchStatus to false;
   **/
  reader->searchStatus = false;
  pthread_mutex_unlock(&reader->backgroundLock);

  /**
   * Wait until the reading has started
   **/
  pthread_mutex_lock(&reader->backgroundLock);
  while (TMR_READ_STATE_STARTING == reader->readState)
  {
    pthread_cond_wait(&reader->readCond, &reader->backgroundLock);
    }
  pthread_mutex_unlock(&reader->backgroundLock);

  if ((true == reader->continuousReading) && (true == reader->trueAsyncflag))
    {
      /**
     * In case of true continuous reading, we need to send
     * stop reading message immediately.
       **/
    reader->cmdStopReading(reader);

    /**
     * Wait logic has been changed in case of continuous reading.
     * Wait while the background reader is still reading.
     **/
    pthread_mutex_lock(&reader->backgroundLock);
    while (TMR_READ_STATE_DONE != reader->readState)
    {
      pthread_cond_wait(&reader->readCond, &reader->backgroundLock);
    }
    pthread_mutex_unlock(&reader->backgroundLock);
    /**
     * By this time, reader->backgroundEnabled is
     * already set to false. i.e., background reader thread
     * is suspended.
     **/
  }

  /**
   * wait until background reader thread finishes.
   * This is needed for pseudo-async reads and also
   * worst case of continuous reading, when read isn't success
   **/
  pthread_mutex_lock(&reader->backgroundLock);
  reader->backgroundEnabled = false;
  while (true == reader->backgroundRunning)
  {
    pthread_cond_wait(&reader->backgroundCond, &reader->backgroundLock);
  }
  pthread_mutex_unlock(&reader->backgroundLock);

  /**
   * Reset continuous reading settings, so that
   * the subsequent startReading() call doesn't have
   * any surprises.
   **/
  reset_continuous_reading(reader, false);

  return TMR_SUCCESS;
}

void
notify_read_listeners(TMR_Reader *reader, TMR_TagReadData *trd)
{
  TMR_ReadListenerBlock *rlb;

  /* notify tag read to listener */
  if (NULL != reader)
  {
    pthread_mutex_lock(&reader->listenerLock);
    rlb = reader->readListeners;
    while (rlb)
    {
      rlb->listener(reader, trd, rlb->cookie);
      rlb = rlb->next;
    }
    pthread_mutex_unlock(&reader->listenerLock);
  }
}

void
notify_stats_listeners(TMR_Reader *reader, TMR_Reader_StatsValues *stats)
{
  TMR_StatsListenerBlock *slb;

  /* notify stats to the listener */
  pthread_mutex_lock(&reader->listenerLock);
  slb = reader->statsListeners;
  while (slb)
  {
    slb->listener(reader, stats, slb->cookie);
    slb = slb->next;
  }
  pthread_mutex_unlock(&reader->listenerLock);
}

/* NOTE: There is only one auth object for all the authreq listeners, so whichever listener touches it last wins.
 * For now (2012 Jul 20) we only anticipate having a single authreq listener, but there may be future cases which 
 * require multiples.  Revise this design if necessary. */
void
notify_authreq_listeners(TMR_Reader *reader, TMR_TagReadData *trd, TMR_TagAuthentication *auth)
{
  TMR_AuthReqListenerBlock *arlb;

  /* notify tag read to listener */
  pthread_mutex_lock(&reader->listenerLock);
  arlb = reader->authReqListeners;
  while (arlb)
  {
    arlb->listener(reader, trd, arlb->cookie, auth);
    arlb = arlb->next;
  }
  pthread_mutex_unlock(&reader->listenerLock);
}

TMR_Status
TMR_addReadExceptionListener(TMR_Reader *reader,
                             TMR_ReadExceptionListenerBlock *b)
{

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  b->next = reader->readExceptionListeners;
  reader->readExceptionListeners = b;

  pthread_mutex_unlock(&reader->listenerLock);

  return TMR_SUCCESS;
}

TMR_Status
TMR_removeReadExceptionListener(TMR_Reader *reader,
                                TMR_ReadExceptionListenerBlock *b)
{
  TMR_ReadExceptionListenerBlock *block, **prev;

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  prev = &reader->readExceptionListeners;
  block = reader->readExceptionListeners;
  while (NULL != block)
  {
    if (block == b)
    {
      *prev = block->next;
      break;
    }
    prev = &block->next;
    block = block->next;
  }

  pthread_mutex_unlock(&reader->listenerLock);

  if (block == NULL)
  {
    return TMR_ERROR_INVALID;
  }

  return TMR_SUCCESS;
}

void
notify_exception_listeners(TMR_Reader *reader, TMR_Status status)
{
  TMR_ReadExceptionListenerBlock *relb;

  if (NULL != reader)
  {
    pthread_mutex_lock(&reader->listenerLock);
    relb = reader->readExceptionListeners;
    while (relb)
    {
      relb->listener(reader, status, relb->cookie);
      relb = relb->next;
    }
    pthread_mutex_unlock(&reader->listenerLock);
  }
}

TMR_Queue_tagReads *
dequeue(TMR_Reader *reader)
{
  TMR_Queue_tagReads *tagRead;
  pthread_mutex_lock(&reader->queue_lock);
  tagRead = reader->tagReadQueue;
  reader->tagReadQueue = reader->tagReadQueue->next;
  reader->queue_depth --;
  pthread_mutex_unlock(&reader->queue_lock);
  return(tagRead);
}


void enqueue(TMR_Reader *reader, TMR_Queue_tagReads *tagRead)
{
  pthread_mutex_lock(&reader->queue_lock);
  tagRead->next = reader->tagReadQueue;   /* Insert at head of list */
  reader->tagReadQueue = tagRead;
  reader->queue_depth ++;
  pthread_mutex_unlock(&reader->queue_lock);
}

static void *
parse_tag_reads(void *arg)
{
  TMR_Reader *reader;
  TMR_Queue_tagReads *tagRead;
#ifdef TMR_ENABLE_SERIAL_READER          
  uint16_t flags = 0;
#endif/* TMR_ENABLE_SERIAL_READER */           

  reader = arg;  

  while (1)
  {
    pthread_mutex_lock(&reader->parserLock);
    reader->parserRunning = false;
    pthread_cond_broadcast(&reader->parserCond);
    while (false == reader->parserEnabled)
    {
      pthread_cond_wait(&reader->parserCond, &reader->parserLock);
    }

    reader->parserRunning = true;
    pthread_mutex_unlock(&reader->parserLock);

    /**
     * Wait until queue_length is more than zero,
     * i.e., Queue should have atleast one tagRead to process
     */
    sem_wait(&reader->queue_length);

    if (NULL != reader->tagReadQueue)
    {
      /**
       * At this point there is a tagEntry in the queue
       * dequeue it and parse it.
       */          
      tagRead = dequeue(reader);
      if (false == tagRead->isStatusResponse)
      {
        /* Tag Buffer stream response */

#ifdef TMR_ENABLE_SERIAL_READER          
        if (TMR_READER_TYPE_SERIAL == reader->readerType)
        {
          TMR_TagReadData trd;

          TMR_TRD_init(&trd);

          /* In case of streaming the flags always start at position 8*/
          flags = GETU16AT(tagRead->tagEntry.sMsg, 8);
          TMR_SR_parseMetadataFromMessage(reader, &trd, flags, &tagRead->bufPointer, tagRead->tagEntry.sMsg);
          TMR_SR_postprocessReaderSpecificMetadata(&trd, &reader->u.serialReader);
          
          trd.reader = reader;
          notify_read_listeners(reader, &trd);
        }
#endif/* TMR_ENABLE_SERIAL_READER */           
#ifdef TMR_ENABLE_LLRP_READER
        if (TMR_READER_TYPE_LLRP == reader->readerType)
        {
          /* Else it is LLRP message, parse it */
          LLRP_tSRO_ACCESS_REPORT *pReport;
          LLRP_tSTagReportData *pTagReportData;

          pReport = (LLRP_tSRO_ACCESS_REPORT *)tagRead->tagEntry.lMsg;

          for(pTagReportData = pReport->listTagReportData;
              NULL != pTagReportData;
              pTagReportData = (LLRP_tSTagReportData *)pTagReportData->hdr.pNextSubParameter)
          {
            TMR_TagReadData trd;

            TMR_TRD_init(&trd);
            TMR_LLRP_parseMetadataFromMessage(reader, &trd, pTagReportData);
          
            trd.reader = reader;
            notify_read_listeners(reader, &trd);
        }
        }
#endif
        }
      else
      {
       /* A status stream response */

        if (TMR_READER_TYPE_SERIAL == reader->readerType)
        {
          TMR_Reader_StatsValues stats;
          uint8_t offset, i,j;
          uint16_t flags = 0;                 

          TMR_STATS_init(&stats);
          offset = tagRead->bufPointer;

          if (NULL != reader->statusListeners && NULL== reader->statsListeners)
          {
            /* A status stream response */
            TMR_StatusListenerBlock *slb;
            uint8_t index = 0, j;
            TMR_SR_StatusReport report[TMR_SR_STATUS_MAX];


            /* Get status content flags */
            flags = GETU16(tagRead->tagEntry.sMsg, offset);

            if (0 != (flags & TMR_SR_STATUS_FREQUENCY))
            {
              report[index].type = TMR_SR_STATUS_FREQUENCY;
              report[index].u.fsr.freq = (uint32_t)(GETU24(tagRead->tagEntry.sMsg, offset));
              index ++;
            }
            if (0 != (flags & TMR_SR_STATUS_TEMPERATURE))
            {
              report[index].type = TMR_SR_STATUS_TEMPERATURE;
              report[index].u.tsr.temp = GETU8(tagRead->tagEntry.sMsg, offset);
              index ++;
            }
            if (0 != (flags & TMR_SR_STATUS_ANTENNA))
            {
              uint8_t tx, rx;
              report[index].type = TMR_SR_STATUS_ANTENNA;
              tx = GETU8(tagRead->tagEntry.sMsg, offset);
              rx = GETU8(tagRead->tagEntry.sMsg, offset);

              for (j = 0; j < reader->u.serialReader.txRxMap->len; j++)
              {
                if ((rx == reader->u.serialReader.txRxMap->list[j].rxPort) && (tx == reader->u.serialReader.txRxMap->list[j].txPort))
                {
                  report[index].u.asr.ant = reader->u.serialReader.txRxMap->list[j].antenna;
                  break;
                }
              }
              index ++;
            }

            report[index].type = TMR_SR_STATUS_NONE;
            /* notify status response to listener */
            pthread_mutex_lock(&reader->listenerLock);
            slb = reader->statusListeners;
            while (slb)
            {
              slb->listener(reader, report, slb->cookie);
              slb = slb->next;
            }
            pthread_mutex_unlock(&reader->listenerLock);

          }
          else if (NULL != reader->statsListeners && NULL== reader->statusListeners)
          {
            /* Get status content flags */
            if ((0x80) > reader->statsFlag)
            {
              offset += 1;
            }
            else
            {
              offset += 2;
            }

            /**
             * preinitialize the rf ontime and the noise floor value to zero
             * berfore getting the reader stats
             */
            for (i = 0; i < stats.perAntenna.max; i++)
            {
              stats.perAntenna.list[i].antenna = 0;
              stats.perAntenna.list[i].rfOnTime = 0;
              stats.perAntenna.list[i].noiseFloor = 0;
            }

            TMR_fillReaderStats(reader, &stats, flags, tagRead->tagEntry.sMsg, offset);

            /**
             * iterate through the per antenna values,
             * If found  any 0-antenna rows, copy the
             * later rows down to compact out the empty space.
             */
            for (i = 0; i < reader->u.serialReader.txRxMap->len; i++)
            {
              if (!stats.perAntenna.list[i].antenna)
              {
                for (j = i + 1; j < reader->u.serialReader.txRxMap->len; j++)
                {
                  if (stats.perAntenna.list[j].antenna)
                  {
                    stats.perAntenna.list[i].antenna = stats.perAntenna.list[j].antenna;
                    stats.perAntenna.list[i].rfOnTime = stats.perAntenna.list[j].rfOnTime;
                    stats.perAntenna.list[i].noiseFloor = stats.perAntenna.list[j].noiseFloor;
                    stats.perAntenna.list[j].antenna = 0;
                    stats.perAntenna.list[j].rfOnTime = 0;
                    stats.perAntenna.list[j].noiseFloor = 0;

                    stats.perAntenna.len++;
                    break;
                  }
                }
              }
              else
              {
                /* Increment the length */
                stats.perAntenna.len++;
              }
            }

            /* store the requested flags for future use */
            stats.valid = reader->statsFlag;

            /* notify status response to listener */
            notify_stats_listeners(reader, &stats);
          }
          else
          {
            /**
             * Control comes here when, user added both the listeners,
             * We should pop up error for that
             **/
            TMR_Status ret;
            ret = TMR_ERROR_UNSUPPORTED;
            notify_exception_listeners(reader, ret);
          }
        }
#ifdef TMR_ENABLE_LLRP_READER
        else
        {
          /**
           * TODO: Handle RFSurveyReports in case of
           * async read
           **/
        }
#endif
      }

      /* Free the memory */
      if (TMR_READER_TYPE_SERIAL == reader->readerType)
      {
      	free(tagRead->tagEntry.sMsg);
      }
#ifdef TMR_ENABLE_LLRP_READER
      else
      {
      	TMR_LLRP_freeMessage(tagRead->tagEntry.lMsg);
      }
#endif
      free(tagRead);

      /* Now, increment the queue_slots as we have removed one entry */
      sem_post(&reader->queue_slots);
    }
  }
  return NULL;
}


static void
process_async_response(TMR_Reader *reader)
{
  TMR_Queue_tagReads *tagRead;

  /* Decrement Queue slots */
  sem_wait(&reader->queue_slots);

  tagRead = (TMR_Queue_tagReads *) malloc(sizeof(TMR_Queue_tagReads));
  if (TMR_READER_TYPE_SERIAL == reader->readerType)
  {
    tagRead->tagEntry.sMsg = (uint8_t *) malloc(TMR_SR_MAX_PACKET_SIZE); /* size of bufResponse */
    memcpy(tagRead->tagEntry.sMsg, reader->u.serialReader.bufResponse, TMR_SR_MAX_PACKET_SIZE);
    tagRead->bufPointer = reader->u.serialReader.bufPointer;
  }
#ifdef TMR_ENABLE_LLRP_READER
  else
  {
    tagRead->tagEntry.lMsg = reader->u.llrpReader.bufResponse[0];
    reader->u.llrpReader.bufResponse[0] = NULL;
  }
#endif

  tagRead->isStatusResponse = reader->isStatusResponse;
  /* Enqueue the tagRead into Queue */
  enqueue(reader, tagRead);
  /* Increment queue_length */
  sem_post(&reader->queue_length);

  if ((false == reader->isStatusResponse) && (TMR_READER_TYPE_SERIAL == reader->readerType))
  {
    reader->u.serialReader.tagsRemainingInBuffer--;
  }
}

static void *
do_background_reads(void *arg)
{
  TMR_Status ret;
  TMR_Reader *reader;
  uint32_t onTime, offTime;
  int32_t sleepTime;
  uint64_t end, now, difftime;

  reader = arg;
  reader->trueAsyncflag = false;

  TMR_paramGet(reader, TMR_PARAM_READ_ASYNCOFFTIME, &offTime);

  while (1)
  {
    /* Wait for reads to be enabled */
    pthread_mutex_lock(&reader->backgroundLock);
    reader->backgroundRunning = false;

    pthread_cond_broadcast(&reader->backgroundCond);
    while (false == reader->backgroundEnabled)
    {
      reader->trueAsyncflag = false;
      pthread_cond_wait(&reader->backgroundCond, &reader->backgroundLock);
      if (true == reader->backgroundThreadCancel)
      {
        /**
         * thread is no more, required,
         * hence, making it terminated
         **/ 
        goto EXIT;
      }
    }

    TMR_paramGet(reader, TMR_PARAM_READ_ASYNCONTIME, &onTime);

    if (!reader->trueAsyncflag)
    {
      ret = TMR_read(reader, onTime, NULL);
      if (TMR_SUCCESS != ret)
      {
        if ((TMR_ERROR_TIMEOUT == ret) || (TMR_ERROR_CRC_ERROR == ret) ||
              (TMR_ERROR_SYSTEM_UNKNOWN_ERROR == ret) || (TMR_ERROR_TM_ASSERT_FAILED == ret))
        {
          if (TMR_READER_TYPE_SERIAL == reader->readerType)
          {
            reader->u.serialReader.transport.flush(&reader->u.serialReader.transport);
          }
          reader->backgroundEnabled = false;
        }

        /**
         * M5e and its variants hardware does not have a real PA protection.So, doing the read with out
         * antenna may cause the damage to the reader.
         *
         * it's okay to let M6e and its variants continue to operate because it has a PA protection mechanism.
         **/
        if (((TMR_ERROR_HIGH_RETURN_LOSS == ret) || (TMR_ERROR_NO_ANTENNA == ret))
            &&
            ((TMR_SR_MODEL_M6E != reader->u.serialReader.versionInfo.hardware[0]) &&
             (TMR_SR_MODEL_M6E_MICRO != reader->u.serialReader.versionInfo.hardware[0]) &&
             (TMR_SR_MODEL_M6E_PRC != reader->u.serialReader.versionInfo.hardware[0])))
        {
          reader->backgroundEnabled = false;
          reader->readState = TMR_READ_STATE_DONE;
          pthread_mutex_unlock(&reader->backgroundLock);
          notify_exception_listeners(reader, ret);
          break;
        }

        notify_exception_listeners(reader, ret);
        if(false == reader->searchStatus)
        {
          /**
           * There could be something wrong in initiating a search, continue the
           * effort to initiate a search. But meanwhile if stopReading()
           * is called, it will be blocked as the search itself is not started.
           * sem_post on read_started will unblock it.
           **/
          reader->readState = TMR_READ_STATE_STARTED;
          pthread_cond_broadcast(&reader->readCond);
          reader->backgroundEnabled = false;
        }
        pthread_mutex_unlock(&reader->backgroundLock);
        continue;
      }
      if(reader->continuousReading)
      {
        /**
         * Set this flag, In case of true async reading
         * we have to send the command only once.
         */
        reader->trueAsyncflag = true;
      }

      /**
       * Set an indication that the reading is started
       **/
      reader->readState = TMR_READ_STATE_ACTIVE;
      pthread_cond_broadcast(&reader->readCond);
    }

    reader->backgroundRunning = true;
    pthread_mutex_unlock(&reader->backgroundLock);

    if (true == reader->continuousReading)
    {
      /**  
       * Streaming is enabled only in case of M6e, 
       * read till the end of stream.
       */            
      while (true)
      {
        ret = TMR_hasMoreTags(reader);
        if (TMR_SUCCESS == ret)
        {
          /* Got a valid message, before posting it to queue
           * check whether we have slots free in the queue or
           * not. Validate this only for Serial reader.
           */
          if (TMR_READER_TYPE_SERIAL == reader->readerType)
          {
            int slotsFree = 0;
            int semret;
            /* Get the semaphore value */
            semret = sem_getvalue(&reader->queue_slots, &slotsFree);
            if (0 == semret)
            {
              if (10 > slotsFree)
              {
                tmr_sleep(20);
              }
              if (0 >= slotsFree)
              {
                /* In a normal case we should not come here.
                 * we are here means there is no place to
                 * store the tags. May be the read listener
                 * is not fast enough.
                 * In this case stop the read and exit.
                 */
                if (true == reader->searchStatus)
                {
                  ret = TMR_ERROR_BUFFER_OVERFLOW;
                  notify_exception_listeners(reader, ret);
                  reader->cmdStopReading(reader);
                  pthread_mutex_lock(&reader->backgroundLock);
                  reader->backgroundEnabled = false;
                  reader->readState = TMR_READ_STATE_DONE;
                  pthread_mutex_unlock(&reader->backgroundLock);
                  reader->searchStatus = false;
                  break;
                }
              }
            }
          }

          /* There is place to store the response. Post it */
          process_async_response(reader);
        }
        else if (TMR_ERROR_CRC_ERROR == ret)
        {
          /* Currently, just drop the corrupted packet,
           * inform the user about the error and move on.
           *
           * TODO: Fix the error by tracing the exact reason of failour
           */
          notify_exception_listeners(reader, ret);
        }
        else if (TMR_ERROR_TAG_ID_BUFFER_FULL == ret)
        {
          /* In case of buffer full error, notify the exception */
          notify_exception_listeners(reader, ret);

          /**
           * If stop read is already called, no need to resumbit the seach again.
           * Just spin in the curent loop and wait for the stop read command response.
           **/ 
          if (true == reader->searchStatus)
          {
            /**
             * Stop read is not called. Resubmit the search immediately, without user interaction
             * Resetting the trueAsyncFlag will send the continuous read command again.
             */

            ret = TMR_hasMoreTags(reader);
            reader->trueAsyncflag = false;
            break;
          }
        }
        else
        {
          if ((TMR_ERROR_TIMEOUT == ret) || (TMR_ERROR_SYSTEM_UNKNOWN_ERROR == ret) || 
            (TMR_ERROR_TM_ASSERT_FAILED == ret) || (TMR_ERROR_LLRP_READER_CONNECTION_LOST == ret))
          {
            notify_exception_listeners(reader, ret);
            /** 
             * In case of timeout error or CRC error, flush the transport buffer.
             * this avoids receiving of junk response.
             */
            if (TMR_READER_TYPE_SERIAL == reader->readerType)
            {
              /* Handling this fix for serial reader now */
		          reader->u.serialReader.transport.flush(&reader->u.serialReader.transport);
            }

            /**
             * Check if reading is finished.
             * If not, send stop command.
             **/
            if (!reader->finishedReading)
            {
              reader->cmdStopReading(reader);
            }

            pthread_mutex_lock(&reader->backgroundLock);
            reader->backgroundEnabled = false;
            reader->readState = TMR_READ_STATE_DONE;
            pthread_cond_broadcast(&reader->readCond);
            pthread_mutex_unlock(&reader->backgroundLock);


            /**
             * Forced stop 
             * Reset continuous reading settings, so that
             * the subsequent startReading() call doesn't have
             * any surprises.
             **/
            reader->searchStatus = false;
            reset_continuous_reading(reader, true);
          }
          else if (TMR_ERROR_END_OF_READING == ret)
          {
            while(0 < reader->queue_depth)
            {
              /**
               * reader->queue_depth is greater than zero. i.e.,
               * there are still some tags left in queue.
               * Give some time for the parser to parse all of them.
               * 5 ms sleep shouldn't cause much delay.
               **/
              tmr_sleep(5);
            }

            /**
             * Since the reading is finished, disable this
             * thread.
             **/
            pthread_mutex_lock(&reader->backgroundLock);
            reader->backgroundEnabled = false;
            reader->readState = TMR_READ_STATE_DONE;
            pthread_cond_broadcast(&reader->readCond);
            pthread_mutex_unlock(&reader->backgroundLock);
            break;
          }
          else if ((TMR_ERROR_NO_TAGS_FOUND != ret) && (TMR_ERROR_NO_TAGS != ret) && (TMR_ERROR_TAG_ID_BUFFER_AUTH_REQUEST != ret) && (TMR_ERROR_TOO_BIG != ret))
          {
            /* Any exception other than 0x400 should be notified */
            notify_exception_listeners(reader, ret);
          }
          break;
        }
      }
    }
    else
    {
      /** 
       * On M5e and its variants, streaming is not supported
       * So still, retain the pseudo-async mechanism
       * Also, when asyncOffTime is non-zero the API should fallback to 
       * pseudo async mode.
       */

      end = tmr_gettime();

      while (TMR_SUCCESS == TMR_hasMoreTags(reader))
      {
        TMR_TagReadData trd;
        TMR_ReadListenerBlock *rlb;

        TMR_TRD_init(&trd);

        ret = TMR_getNextTag(reader, &trd);
        if (TMR_SUCCESS != ret)
        {
          pthread_mutex_lock(&reader->backgroundLock);
          reader->backgroundEnabled = false;
          pthread_mutex_unlock(&reader->backgroundLock);
          notify_exception_listeners(reader, ret);
          break;
        }

        pthread_mutex_lock(&reader->listenerLock);
        rlb = reader->readListeners;
        while (rlb)
        { 
          rlb->listener(reader, &trd, rlb->cookie);
          rlb = rlb->next;
        }
        pthread_mutex_unlock(&reader->listenerLock);
      }

      /* Wait for the asyncOffTime duration to pass */
      now = tmr_gettime();
      difftime = now - end;

      sleepTime = offTime - (uint32_t)difftime;
      if(sleepTime > 0)
      {
        tmr_sleep(sleepTime);
      }
    }
EXIT:
    if (reader->backgroundThreadCancel)
    {
      /**
       * oops.. time to exit
       **/ 
      pthread_exit(NULL);
    }
  }
  return NULL;
}


TMR_Status
TMR_addReadListener(TMR_Reader *reader, TMR_ReadListenerBlock *b)
{

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  b->next = reader->readListeners;
  reader->readListeners = b;

  pthread_mutex_unlock(&reader->listenerLock);

  return TMR_SUCCESS;
}


TMR_Status
TMR_removeReadListener(TMR_Reader *reader, TMR_ReadListenerBlock *b)
{
  TMR_ReadListenerBlock *block, **prev;

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  prev = &reader->readListeners;
  block = reader->readListeners;
  while (NULL != block)
  {
    if (block == b)
    {
      *prev = block->next;
      break;
    }
    prev = &block->next;
    block = block->next;
  }

  pthread_mutex_unlock(&reader->listenerLock);

  if (block == NULL)
  {
    return TMR_ERROR_INVALID;
  }

  return TMR_SUCCESS;
}


TMR_Status
TMR_addAuthReqListener(TMR_Reader *reader, TMR_AuthReqListenerBlock *b)
{

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  b->next = reader->authReqListeners;
  reader->authReqListeners = b;

  pthread_mutex_unlock(&reader->listenerLock);

  return TMR_SUCCESS;
}


TMR_Status
TMR_removeAuthReqListener(TMR_Reader *reader, TMR_AuthReqListenerBlock *b)
{
  TMR_AuthReqListenerBlock *block, **prev;

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  prev = &reader->authReqListeners;
  block = reader->authReqListeners;
  while (NULL != block)
  {
    if (block == b)
    {
      *prev = block->next;
      break;
    }
    prev = &block->next;
    block = block->next;
  }

  pthread_mutex_unlock(&reader->listenerLock);

  if (block == NULL)
  {
    return TMR_ERROR_INVALID;
  }

  return TMR_SUCCESS;
}

TMR_Status
TMR_addStatusListener(TMR_Reader *reader, TMR_StatusListenerBlock *b)
{

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  b->next = reader->statusListeners;
  reader->statusListeners = b;

  /*reader->streamStats |= b->statusFlags & TMR_SR_STATUS_CONTENT_FLAGS_ALL;*/

  pthread_mutex_unlock(&reader->listenerLock);

  return TMR_SUCCESS;
}

TMR_Status
TMR_addStatsListener(TMR_Reader *reader, TMR_StatsListenerBlock *b)
{

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  b->next = reader->statsListeners;
  reader->statsListeners = b;

  /*reader->streamStats |= b->statusFlags & TMR_SR_STATUS_CONTENT_FLAGS_ALL; */

  pthread_mutex_unlock(&reader->listenerLock);

  return TMR_SUCCESS;
}


TMR_Status
TMR_removeStatsListener(TMR_Reader *reader, TMR_StatsListenerBlock *b)
{
  TMR_StatsListenerBlock *block, **prev;

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  prev = &reader->statsListeners;
  block = reader->statsListeners;
  while (NULL != block)
  {
    if (block == b)
    {
      *prev = block->next;
      break;
    }
    prev = &block->next;
    block = block->next;
  }

  /* Remove the status flags requested by this listener and reframe */
  /*reader->streamStats = TMR_SR_STATUS_CONTENT_FLAG_NONE;
  {
    TMR_StatusListenerBlock *current;
    current = reader->statusListeners;
    while (NULL != current)
    {
      reader->streamStats |= current->statusFlags;
      current = current->next;
    }    
  }*/
  
  pthread_mutex_unlock(&reader->listenerLock);

  if (block == NULL)
  {
    return TMR_ERROR_INVALID;
  }

  return TMR_SUCCESS;
}

TMR_Status
TMR_removeStatusListener(TMR_Reader *reader, TMR_StatusListenerBlock *b)
{
  TMR_StatusListenerBlock *block, **prev;

  if (0 != pthread_mutex_lock(&reader->listenerLock))
    return TMR_ERROR_TRYAGAIN;

  prev = &reader->statusListeners;
  block = reader->statusListeners;
  while (NULL != block)
  {
    if (block == b)
    {
      *prev = block->next;
      break;
    }
    prev = &block->next;
    block = block->next;
  }

  /* Remove the status flags requested by this listener and reframe */
  /*reader->streamStats = TMR_SR_STATUS_CONTENT_FLAG_NONE;
    {
    TMR_StatusListenerBlock *current;
    current = reader->statusListeners;
    while (NULL != current)
    {
    reader->streamStats |= current->statusFlags;
    current = current->next;
    }
    }*/

  pthread_mutex_unlock(&reader->listenerLock);

  if (block == NULL)
  {
    return TMR_ERROR_INVALID;
  }

  return TMR_SUCCESS;
}

void cleanup_background_threads(TMR_Reader *reader)
{
  if (NULL != reader)
  {
    pthread_mutex_lock(&reader->backgroundLock);
    pthread_mutex_lock(&reader->listenerLock);
    reader->readExceptionListeners = NULL;
    reader->statsListeners = NULL;
    if (true == reader->backgroundSetup)
    {
      /**
       * Signal for the thread exit by 
       * removing all the pthread lock dependency
       **/
      reader->backgroundThreadCancel = true;
      pthread_cond_broadcast(&reader->backgroundCond);
    }
    pthread_mutex_unlock(&reader->listenerLock);
    pthread_mutex_unlock(&reader->backgroundLock);

    if (true == reader->backgroundSetup)
    {
      /**
       * Wait for the back ground thread to exit
       **/ 
      pthread_join(reader->backgroundReader, NULL);
    }

    pthread_mutex_lock(&reader->parserLock);
    pthread_mutex_lock(&reader->listenerLock);
    reader->readListeners = NULL;
    if (true == reader->parserSetup)
    {
      pthread_cancel(reader->backgroundParser);
    }
    pthread_mutex_unlock(&reader->listenerLock);
    pthread_mutex_unlock(&reader->parserLock);
  }
}
#endif /* TMR_ENABLE_BACKGROUND_READS */
