/******************************************************************************
*
* Copyright (C) 2023 - 2028 KETI, All rights reserved.
*                           (Korea Electronics Technology Institute)
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
* Use of the Software is limited solely to applications:
* (a) running for Korean Government Project, or
* (b) that interact with KETI project/platform.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* KETI BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the KETI shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from KETI.
*
******************************************************************************/
/******************************************************************************/
/**
*
* @file time_manager.c
*
* This file contains a data format design
*
* @note
*
* V2X Data Format Source File
*
* MODIFICATION HISTORY:
* Ver   Who  Date     Changes
* ----- ---- -------- ----------------------------------------------------
* 1.00  bman  23.03.22 First release
*
******************************************************************************/

/***************************** Include ***************************************/
#include "framework.h"
#include "time_manager.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <time.h>

/***************************** Definition ************************************/
#define TIME_MGR_CHECK_NTP          "rdate -p time.bora.net"
#define TIME_MGR_SYNC_NTP           "sudo rdate -s time.bora.net"

//#define CONFIG_TIME_MANAGER_DEBUG   (1)
#define TIME_MANAGER_TIME_MAX_SIZE  (19)

/***************************** Enum and Structure ****************************/

/***************************** Static Variable *******************************/
FILE* sh_pTimeMgrMsg;
static int s_nTimeTaskMsgId;
static key_t s_timeTaskMsgKey = FRAMEWORK_TIME_TASK_MSG_KEY;

static pthread_t sh_TimeMgrTask;
static struct timespec stCurTime, stCheckBeginTime, stCheckEndTime;

/***************************** Function  *************************************/

static int32_t P_TIME_MANAGER_SyncNtpServer(TIME_MANAGER_T *pstTimeMgr)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstTimeMgr == NULL)
    {
        PrintError("pstTimeMgr == NULL!!");
        return nRet;
    }

    nRet = system(TIME_MGR_CHECK_NTP);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("system() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    nRet = system(TIME_MGR_SYNC_NTP);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("system() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    return nRet;
}

static void *P_TIME_MANAGER_Task(void *arg)
{
    TIME_MANAGER_EVENT_MSG_T stEventMsg;
    int32_t nRet = FRAMEWORK_ERROR;
    memset(&stEventMsg, 0, sizeof(TIME_MANAGER_EVENT_MSG_T));

    UNUSED(arg);
    UNUSED(nRet);

    while (1)
    {
        if(msgrcv(s_nTimeTaskMsgId, &stEventMsg, sizeof(TIME_MANAGER_EVENT_MSG_T), 0, MSG_NOERROR) == FRAMEWORK_MSG_ERR)
        {
            PrintError("msgrcv() is failed!");
        }
        else
        {
            PrintError("TODO");
            nRet = FRAMEWORK_OK;
        }

        usleep(1000);
    }

    return NULL;
}

static void P_TIME_MANAGER_PrintMsgInfo(int msqid)
{

    struct msqid_ds m_stat;

    PrintDebug("========== Messege Queue Infomation =============");

    if(msgctl(msqid, IPC_STAT, &m_stat) == FRAMEWORK_MSG_ERR)
    {
        PrintError("msgctl() is failed!!");
    }

    PrintDebug("msg_lspid : %d", m_stat.msg_lspid);
    PrintDebug("msg_qnum : %ld", m_stat.msg_qnum);
    PrintDebug("msg_stime : %ld", m_stat.msg_stime);

    PrintDebug("=================================================");
}

int32_t P_TIME_MANAGER_CreateTask(void)
{
	int32_t nRet = FRAMEWORK_ERROR;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    nRet = pthread_create(&sh_TimeMgrTask, &attr, P_TIME_MANAGER_Task, NULL);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("pthread_join() is failed!! (P_TIME_MANAGER_Task) [nRet:%d]", nRet);
    }
    else
    {
        PrintTrace("P_TIME_MANAGER_Task() is successfully created.");
        nRet = FRAMEWORK_OK;
    }

#if defined(CONFIG_PTHREAD_JOINABLE)
    nRet = pthread_join(sh_TimeMgrTask, NULL);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("pthread_join() is failed!! (P_TIME_MANAGER_Task) [nRet:%d]", nRet);
    }
    else
    {
        PrintDebug("P_TIME_MANAGER_Task() is successfully joined.");
        nRet = FRAMEWORK_OK;
    }
#endif
	return nRet;
}

static int32_t P_TIME_MANAGER_Init(TIME_MANAGER_T *pstTimeMgr)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstTimeMgr == NULL)
    {
        PrintError("pstTimeMgr == NULL!!");
        return nRet;
    }

    if((s_nTimeTaskMsgId = msgget(s_timeTaskMsgKey, IPC_CREAT|0666)) == FRAMEWORK_MSG_ERR)
    {
        PrintError("msgget() is failed!");
        return nRet;
    }
    else
    {
        P_TIME_MANAGER_PrintMsgInfo(s_nTimeTaskMsgId);
    }

    nRet = P_TIME_MANAGER_CreateTask();
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("P_TIME_MANAGER_CreateTask() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}
static int32_t P_TIME_MANAGER_DeInit(TIME_MANAGER_T *pstTimeMgr)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstTimeMgr == NULL)
    {
        PrintError("pstTimeMgr == NULL!!");
        return nRet;
    }

    return nRet;
}


int32_t TIME_MANAGER_Get(TIME_MANAGER_T *pstTimeMgr)
{
    int32_t nRet = FRAMEWORK_ERROR;
    struct tm stLocalTime;
    struct tm *stRetPtr = NULL;
    const char *chTimestampFmtStr = "%Y%m%d%H%M%S";
    char chTimeTempStr[100], chTimeTempNsStr[10];
    char chTimestampStr[50];
    size_t unBytesWritten = 0;
    long lValue;

    if(pstTimeMgr == NULL)
    {
        PrintError("pstTimeMgr == NULL!!");
        return nRet;
    }

    nRet = clock_gettime(CLOCK_REALTIME, &stCurTime);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("clock_gettime() is failed! [nRet:%d]", nRet);
    }

#if defined(CONFIG_TIME_MANAGER_DEBUG)
    PrintDebug("timestamp = %li.%09li sec", stCurTime.tv_sec, stCurTime.tv_nsec);
#endif
    stRetPtr = localtime_r(&stCurTime.tv_sec, &stLocalTime);
    if (stRetPtr == NULL)
    {
        PrintError("localtime_r() is failed to convert to localtime!!");
    }

#if defined(CONFIG_TIME_MANAGER_DEBUG)
    PrintDebug("Human-readable System Time (NTP Epoch)\n"
           "  ns    = %li\n" // ns (0-999999999) from clock_gettime's tv_nsec
           "  sec   = %i\n"  // sec (0-60)
           "  min   = %i\n"  // min (0-59)
           "  hour  = %i\n"  // hour (0-23)
           "  mday  = %i\n"  // day (1-31)
           "  mon   = %i\n"  // month (0-11)
           "  year  = %i\n"  // year - 1900
           "  wday  = %i\n"  // day of the week (0-6, Sunday = 0)
           "  yday  = %i\n"  // day in the year (0-365, 1 Jan = 0)
           "  isdst = %i\n"  // daylight saving time
           "\n",
           stCurTime.tv_nsec,
           stLocalTime.tm_sec,
           stLocalTime.tm_min,
           stLocalTime.tm_hour,
           stLocalTime.tm_mday,
           stLocalTime.tm_mon + 1,
           stLocalTime.tm_year + 1900,
           stLocalTime.tm_wday,
           stLocalTime.tm_yday,
           stLocalTime.tm_isdst);
#endif

    unBytesWritten = strftime(chTimeTempStr, sizeof(chTimeTempStr), chTimestampFmtStr, &stLocalTime);
    if (unBytesWritten == 0)
    {
        PrintError("strftime() is failed to convert `struct tm` to a human-readable time string.");
    }

    sprintf(chTimeTempNsStr, "%09li", stCurTime.tv_nsec);
    strcat(chTimeTempStr, chTimeTempNsStr);
    strncpy(chTimestampStr, chTimeTempStr, TIME_MANAGER_TIME_MAX_SIZE);
    lValue = atol(chTimestampStr);
    pstTimeMgr->ulTimeStamp = lValue;

#if defined(CONFIG_TIME_MANAGER_DEBUG)
    PrintTrace("tiemstamp system time is [%lu], [%ld]", lValue, pstTimeMgr->ulTimeStamp);
#endif

    return nRet;
}


int32_t TIME_MANAGER_Open(TIME_MANAGER_T *pstTimeMgr)
{
    int32_t nRet = FRAMEWORK_ERROR;

    PrintWarn("TODO");

    if(pstTimeMgr == NULL)
    {
        PrintError("pstTimeMgr == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t TIME_MANAGER_Close(TIME_MANAGER_T *pstTimeMgr)
{
    int32_t nRet = FRAMEWORK_ERROR;

    PrintWarn("TODO");

    if(pstTimeMgr == NULL)
    {
        PrintError("pstTimeMgr == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t TIME_MANAGER_Start(TIME_MANAGER_T *pstTimeMgr)
{
    int32_t nRet = FRAMEWORK_ERROR;

    PrintWarn("TODO");

    if(pstTimeMgr == NULL)
    {
        PrintError("pstTimeMgr == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t TIME_MANAGER_Stop(TIME_MANAGER_T *pstTimeMgr)
{
    int32_t nRet = FRAMEWORK_ERROR;

    PrintWarn("TODO");

    if(pstTimeMgr == NULL)
    {
        PrintError("pstTimeMgr == NULL!!");
        return nRet;
    }

    return nRet;
}

void TIME_MANAGER_Status(TIME_MANAGER_T *pstTimeMgr)
{
    int32_t nRet = FRAMEWORK_ERROR;

    PrintWarn("TODO");

    UNUSED(nRet);

    if(pstTimeMgr == NULL)
    {
        PrintError("pstTimeMgr == NULL!!");
    }
}

void TIME_MANAGER_CheckLatencyTime(char *pStr, TIME_MANAGER_T *pstTimeMgr)
{
    int32_t nRet = FRAMEWORK_ERROR;

    UNUSED(nRet);

    if(pstTimeMgr == NULL)
    {
        PrintError("pstTimeMgr == NULL!!");
    }

#if defined(CONFIG_TIME_MANAGER_DEBUG)
    PrintDebug("[%s] Time (Nano): %ld", pStr, pstTimeMgr->ulLatency);
    PrintDebug("[%s] Time (Micro): %lf", pStr, (double)pstTimeMgr->ulLatency/TIME_MILLI_SECOND);
    PrintDebug("[%s] Time (Milli): %lf", pStr, (double)pstTimeMgr->ulLatency/TIME_MICRO_SECOND);
#endif
    PrintDebug("[%s] Time (Second): %lf", pStr, (double)pstTimeMgr->ulLatency/TIME_NANO_SECOND);

}

void TIME_MANAGER_CheckLatencyBegin(TIME_MANAGER_T *pstTimeMgr)
{
    int32_t nRet = FRAMEWORK_ERROR;

    UNUSED(nRet);

    if(pstTimeMgr == NULL)
    {
        PrintError("pstTimeMgr == NULL!!");
    }

    nRet = clock_gettime(CLOCK_REALTIME, &stCheckBeginTime);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("clock_gettime() is failed! [nRet:%d]", nRet);
    }
}

void TIME_MANAGER_CheckLatencyEnd(TIME_MANAGER_T *pstTimeMgr)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstTimeMgr == NULL)
    {
        PrintError("pstTimeMgr == NULL!!");
    }

    nRet = clock_gettime(CLOCK_REALTIME, &stCheckEndTime);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("clock_gettime() is failed! [nRet:%d]", nRet);
    }

    pstTimeMgr->ulLatency = (stCheckEndTime.tv_sec - stCheckBeginTime.tv_sec) + (stCheckEndTime.tv_nsec - stCheckBeginTime.tv_nsec);
}

int32_t TIME_MANAGER_Init(TIME_MANAGER_T *pstTimeMgr)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstTimeMgr == NULL)
    {
        PrintError("pstTimeMgr == NULL!!");
        return nRet;
    }

    nRet = P_TIME_MANAGER_Init(pstTimeMgr);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_TIME_MANAGER_Init() is failed! [unRet:%d]", nRet);
        return nRet;
    }
    else
    {
        PrintWarn("is successfully initialized.");
    }

    nRet= P_TIME_MANAGER_SyncNtpServer(pstTimeMgr);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_TIME_MANAGER_Init() is failed! [unRet:%d]", nRet);
    }
    else
    {
        PrintWarn("P_TIME_MANAGER_SyncNtpServer() is successfully updated.");
    }


    return nRet;
}

int32_t TIME_MANAGER_DeInit(TIME_MANAGER_T *pstTimeMgr)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstTimeMgr == NULL)
    {
        PrintError("pstTimeMgr == NULL!!");
        return nRet;
    }

    nRet = P_TIME_MANAGER_DeInit(pstTimeMgr);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_TIME_MANAGER_DeInit() is failed! [unRet:%d]", nRet);
        return nRet;
    }
    else
    {
        PrintWarn("is successfully initialized.");
    }

    return nRet;
}
