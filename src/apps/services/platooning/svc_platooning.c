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
* @file db_manager.c
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
#include "db_manager.h"
#include "svc_platooning.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

/***************************** Definition ************************************/

/***************************** Enum and Structure ****************************/

/***************************** Static Variable *******************************/
FILE* sh_pSvcPlatooningTxMsg;
FILE* sh_pSvcPlatooningRxMsg;

static int s_nSvcPlatooningTaskMsgId;

static key_t s_SvcPlatooningTaskMsgKey = SVC_PLATOONING_TASK_MSG_KEY;

static pthread_t sh_SvcPlatooningTask;

static bool s_bSvcPlatooningLog = OFF;

/***************************** Function  *************************************/

static void *P_SVC_PLATOONING_Task(void *arg)
{
    SVC_PLATOONING_EVENT_MSG_T stEventMsg;
    int32_t nRet = APP_ERROR;
    memset(&stEventMsg, 0, sizeof(SVC_PLATOONING_EVENT_MSG_T));

    UNUSED(arg);
    UNUSED(nRet);

    while (1)
    {
        if(msgrcv(s_nSvcPlatooningTaskMsgId, &stEventMsg, sizeof(SVC_PLATOONING_EVENT_MSG_T), 0, MSG_NOERROR) == APP_MSG_ERR)
        {
            PrintError("msgrcv() is failed!");
        }
        else
        {
            PrintError("TODO");
            nRet = APP_OK;
        }

        usleep(1000);
    }

    return NULL;
}

static void P_SVC_PLATOONING_PrintMsgInfo(int msqid)
{

    struct msqid_ds m_stat;

    PrintDebug("========== Messege Queue Infomation =============");

    if(msgctl(msqid, IPC_STAT, &m_stat) == APP_MSG_ERR)
    {
        PrintError("msgctl() is failed!!");
    }

    PrintDebug("msg_lspid : %d", m_stat.msg_lspid);
    PrintDebug("msg_qnum : %ld", m_stat.msg_qnum);
    PrintDebug("msg_stime : %ld", m_stat.msg_stime);

    PrintDebug("=================================================");
}

int32_t P_SVC_PLATOONING_CreateTask(void)
{
	int32_t nRet = APP_ERROR;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    nRet = pthread_create(&sh_SvcPlatooningTask, &attr, P_SVC_PLATOONING_Task, NULL);
    if (nRet != APP_OK)
    {
        PrintError("pthread_join() is failed!! (P_SVC_PLATOONING_Task) [nRet:%d]", nRet);
    }
    else
    {
        PrintTrace("P_SVC_PLATOONING_Task() is successfully created.");
        nRet = APP_OK;
    }

#if defined(CONFIG_PTHREAD_JOINABLE)
    nRet = pthread_join(sh_SvcPlatooningTask, NULL);
    if (nRet != APP_OK)
    {
        PrintError("pthread_join() is failed!! (P_SVC_PLATOONING_Task) [nRet:%d]", nRet);
    }
    else
    {
        PrintDebug("P_SVC_PLATOONING_Task() is successfully joined.");
        nRet = APP_OK;
    }
#endif
	return nRet;
}

static int32_t P_SVC_PLATOONING_Init(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;

    if(pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    if((s_SvcPlatooningTaskMsgKey = msgget(s_SvcPlatooningTaskMsgKey, IPC_CREAT|0666)) == APP_MSG_ERR)
    {
        PrintError("msgget() is failed!");
        return nRet;
    }
    else
    {
        P_SVC_PLATOONING_PrintMsgInfo(s_SvcPlatooningTaskMsgKey);
    }

    nRet = P_SVC_PLATOONING_CreateTask();
    if (nRet != APP_OK)
    {
        PrintError("P_SVC_PLATOONING_CreateTask() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}
static int32_t P_SVC_PLATOONING_DeInit(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;

    if(pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t SVC_PLATOONING_SetLog(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;

    if(pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    s_bSvcPlatooningLog = pstSvcPlatooning->bLogLevel;
    PrintTrace("SET:s_bSvcPlatooningLog [%s]", s_bSvcPlatooningLog == ON ? "ON" : "OFF");

    nRet = APP_OK;

    return nRet;
}

int32_t SVC_PLATOONING_Open(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;

    PrintWarn("TODO");

    if(pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t SVC_PLATOONING_Close(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;

    PrintWarn("TODO");

    if(pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t SVC_PLATOONING_Start(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;

    PrintWarn("TODO");

    if(pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t SVC_PLATOONING_Stop(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;

    PrintWarn("TODO");

    if(pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t SVC_PLATOONING_Status(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;

    PrintWarn("TODO");

    if(pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t SVC_PLATOONING_Init(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;

    if(pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    nRet = P_SVC_PLATOONING_Init(pstSvcPlatooning);
    if(nRet != APP_OK)
    {
        PrintError("P_SVC_PLATOONING_Init() is failed! [unRet:%d]", nRet);
        return nRet;
    }
    else
    {
        PrintWarn("is successfully initialized.");
    }

    s_bSvcPlatooningLog = pstSvcPlatooning->bLogLevel;
    PrintDebug("s_bSvcPlatooningLog [%s]", s_bSvcPlatooningLog == ON ? "ON" : "OFF");

    return nRet;
}

int32_t SVC_PLATOONING_DeInit(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;

    if(pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    nRet = P_SVC_PLATOONING_DeInit(pstSvcPlatooning);
    if(nRet != APP_OK)
    {
        PrintError("P_SVC_PLATOONING_DeInit() is failed! [unRet:%d]", nRet);
        return nRet;
    }
    else
    {
        PrintWarn("is successfully initialized.");
    }

    return nRet;
}
