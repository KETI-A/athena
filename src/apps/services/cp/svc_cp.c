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
* @file svc_cp.c
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
#include "svc_cp.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

/***************************** Definition ************************************/

/***************************** Enum and Structure ****************************/

/***************************** Static Variable *******************************/
FILE* sh_pSvcCpTxMsg;
FILE* sh_pSvcCpRxMsg;

static int s_nSvcCpTaskMsgId;

static key_t s_SvcCpTaskMsgKey = SVC_CP_TASK_MSG_KEY;

static pthread_t sh_SvcCpTask;
static pthread_t sh_SvcCpTaskTx;

static SVC_CP_T s_stSvcCp;

/***************************** Function  *************************************/

int32_t P_SVC_CP_SetSettings(SVC_CP_T *pstSvcCp)
{
    int32_t nRet = APP_ERROR;

    if(pstSvcCp == NULL)
    {
        PrintError("pstSvcCp is NULL!!");
    }

    memcpy(&s_stSvcCp, pstSvcCp, sizeof(SVC_CP_T));
    nRet = APP_OK;

    return nRet;
}

int32_t P_SVC_CP_GetSettings(SVC_CP_T *pstSvcCp)
{
    int32_t nRet = APP_ERROR;

    if(pstSvcCp == NULL)
    {
        PrintError("pstSvcCp is NULL!!");
    }

    memcpy(pstSvcCp, &s_stSvcCp, sizeof(SVC_CP_T));
    nRet = APP_OK;

    return nRet;
}

int32_t P_SVC_CP_SetDefaultSettings(SVC_CP_T *pstSvcCp)
{
    int32_t nRet = APP_ERROR;

    if(pstSvcCp == NULL)
    {
        PrintError("pstSvcCp == NULL!!");
        return nRet;
    }

    pstSvcCp->stDbManagerWrite.eFileType = DB_MANAGER_FILE_TYPE_CSV;
    pstSvcCp->stDbManagerWrite.eCommMsgType = DB_MANAGER_COMM_MSG_TYPE_TX;
    pstSvcCp->stDbManagerWrite.eProc = DB_MANAGER_PROC_WRITE;

    pstSvcCp->stMsgManagerTx.ePayloadType = MSG_MANAGER_PAYLOAD_TYPE_RAW;
    pstSvcCp->stMsgManagerTx.eCommType = MSG_MANAGER_COMM_TYPE_5GNRV2X;
    pstSvcCp->stMsgManagerTx.eSignId = MSG_MANAGER_SIGN_ID_UNSECURED;
    pstSvcCp->stMsgManagerTx.eV2xFreq = MSG_MANAGER_V2X_FREQ_5900;
    pstSvcCp->stMsgManagerTx.ePriority = MSG_MANAGER_PRIORITY_CV2X_PPPP_0;
    pstSvcCp->stMsgManagerTx.eV2xDataRate = MSG_MANAGER_V2X_DATA_RATE_6MBPS;
    pstSvcCp->stMsgManagerTx.eV2xTimeSlot = MSG_MANAGER_V2X_TIME_SLOT_CONTINUOUS;
    pstSvcCp->stMsgManagerTx.unPsid = DB_V2X_PSID;
    pstSvcCp->stMsgManagerTx.nTxPower = MSG_MANAGER_V2X_TX_POWER;
    pstSvcCp->stMsgManagerTx.unTxCount = 10;
    pstSvcCp->stMsgManagerTx.unTxDelay = 100;

    for(int i = 0; i < MSG_MANAGER_MAC_LENGTH; i++)
    {
        pstSvcCp->stMsgManagerTx.uchPeerMacAddr[i] = 0xFF;
    }

    pstSvcCp->stMsgManagerTx.unTransmitterProfileId = MSG_MANAGER_V2X_TX_PROFILE_ID;
    pstSvcCp->stMsgManagerTx.unPeerL2Id = MSG_MANAGER_V2X_TX_PEER_L2_ID;

    pstSvcCp->stDbV2x.eDeviceType = DB_V2X_DEVICE_TYPE_OBU;
    pstSvcCp->stDbV2x.eTeleCommType = DB_V2X_TELECOMM_TYPE_5G_PC5_BROADCAST;
    pstSvcCp->stDbV2x.unDeviceId = CLI_DB_V2X_DEFAULT_DEVICE_ID;
    pstSvcCp->stDbV2x.eServiceId = DB_V2X_SERVICE_ID_PLATOONING;
    pstSvcCp->stDbV2x.eActionType = DB_V2X_ACTION_TYPE_REQUEST;
    pstSvcCp->stDbV2x.eRegionId = DB_V2X_REGION_ID_SEONGNAM;
    pstSvcCp->stDbV2x.ePayloadType = DB_V2X_PAYLOAD_TYPE_V2X_STATUS;
    pstSvcCp->stDbV2x.eCommId = DB_V2X_COMM_ID_V2V;
    pstSvcCp->stDbV2x.usDbVer = (DB_V2X_VERSION_MAJOR << CLI_DB_V2X_MAJOR_SHIFT) | DB_V2X_VERSION_MINOR;
    pstSvcCp->stDbV2x.usHwVer = CLI_DB_V2X_DEFAULT_HW_VER;
    pstSvcCp->stDbV2x.usSwVer = CLI_DB_V2X_DEFAULT_SW_VER;

    pstSvcCp->pchIfaceName = SVC_CP_DEFAULT_ETH_DEV;
    pstSvcCp->stDbV2xStatusTx.ulTxTimeStampL1 = 0;
    pstSvcCp->stDbV2xStatusTx.ulTxTimeStampL2 = 0;
    pstSvcCp->stDbV2xStatusTx.ulTxTimeStampL3 = 0;
    pstSvcCp->stDbV2xStatusTx.unRxDeviceId = 0;
    pstSvcCp->stDbV2xStatusTx.eChannel = DB_V2X_STATUS_CHANNEL_5_895_5_905;
    pstSvcCp->stDbV2xStatusTx.sPower = MSG_MANAGER_V2X_TX_POWER;
    pstSvcCp->stDbV2xStatusTx.eBandwidth = DB_V2X_STATUS_BANDWIDTH_20MHZ;
    pstSvcCp->stDbV2xStatusTx.usTxRatio = pstSvcCp->stMsgManagerTx.unTxDelay;
    pstSvcCp->stDbV2xStatusTx.stTxPosition.nTxLatitude = 0;
    pstSvcCp->stDbV2xStatusTx.stTxPosition.nTxLongitude = 0;
    pstSvcCp->stDbV2xStatusTx.stTxPosition.nTxAttitude = 0;

    pstSvcCp->stDbV2xStatusTx.unSeqNum = 0;
    pstSvcCp->stDbV2xStatusTx.unContCnt = 0;
    pstSvcCp->stDbV2xStatusTx.unTxVehicleSpeed = 60;

    nRet = APP_OK;

    PrintDebug("P_SVC_CP_SetDefaultSettings() set is finished.[eth:%s]", pstSvcCp->pchIfaceName);

    return nRet;
}
static int32_t P_SVC_CP_Start(SVC_CP_EVENT_MSG_T *stEventMsg)
{
    int32_t nRet = APP_ERROR;
    UNUSED(stEventMsg);

    if ((s_stSvcCp.eSvcCpStatus == SVC_CP_STATUS_STOP) || (s_stSvcCp.eSvcCpStatus == SVC_CP_STATUS_IDLE))
    {
        s_stSvcCp.eSvcCpStatus = SVC_CP_STATUS_START;
        PrintTrace("eSvcCpStatus starts now [%d]", s_stSvcCp.eSvcCpStatus);

        if(s_stSvcCp.eSvcCpStatus == SVC_CP_STATUS_START)
        {
            nRet = APP_OK;
        }
    }
    else
    {
        PrintWarn("unknown status type");
    }

    return nRet;
}

static int32_t P_SVC_CP_Stop(SVC_CP_EVENT_MSG_T *stEventMsg)
{
    int32_t nRet = APP_ERROR;
    UNUSED(stEventMsg);

    if(s_stSvcCp.eSvcCpStatus == SVC_CP_STATUS_START)
    {
        s_stSvcCp.eSvcCpStatus = SVC_CP_STATUS_STOP;
        PrintTrace("eSvcCpStatus stops now [%d]", s_stSvcCp.eSvcCpStatus);

        if(s_stSvcCp.eSvcCpStatus == SVC_CP_STATUS_STOP)
        {
            nRet = APP_OK;
        }
    }
    else
    {
        PrintWarn("unknown status type");
    }

    return nRet;

}

static void *P_SVC_CP_TaskTx(void *arg)
{
    UNUSED(arg);
    TIME_MANAGER_T *pstTimeManager;
    char *pchPayload = NULL;
    int32_t nRet = APP_ERROR;
    int32_t nFrameWorkRet = FRAMEWORK_ERROR;
    bool bMsgTx = FALSE;

    while (1)
    {
        if(s_stSvcCp.eSvcCpStatus == SVC_CP_STATUS_START)
        {
            s_stSvcCp.stDbV2x.ulPayloadLength = sizeof(s_stSvcCp.stDbV2xStatusTx);

            pchPayload = (char*)malloc(sizeof(char)*s_stSvcCp.stDbV2x.ulPayloadLength);
            if(pchPayload == NULL)
            {
                PrintError("malloc() is failed! [NULL]");
                return nRet;
            }

            (void*)memset(pchPayload, 0x00, sizeof(sizeof(char)*s_stSvcCp.stDbV2x.ulPayloadLength));

            pstTimeManager = FRAMEWORK_GetTimeManagerInstance();
            nFrameWorkRet = TIME_MANAGER_Get(pstTimeManager);
            if(nFrameWorkRet != FRAMEWORK_OK)
            {
                PrintError("TIME_MANAGER_Get() is failed! [nRet:%d]", nFrameWorkRet);
            }
            else
            {
                s_stSvcCp.stDbV2x.ulTimeStamp = pstTimeManager->ulTimeStamp;

                s_stSvcCp.stDbV2xStatusTx.ulTxTimeStampL1 = 19840919;
                s_stSvcCp.stDbV2xStatusTx.ulTxTimeStampL2 = 19850501;
                s_stSvcCp.stDbV2xStatusTx.ulTxTimeStampL3 = pstTimeManager->ulTimeStamp;
            }

            memcpy(pchPayload, (char*)&s_stSvcCp.stDbV2xStatusTx, sizeof(char)*s_stSvcCp.stDbV2x.ulPayloadLength);

            s_stSvcCp.stDbV2x.ulReserved = 0;

            if(bMsgTx == TRUE)
            {
                nFrameWorkRet = MSG_MANAGER_Transmit(&s_stSvcCp.stMsgManagerTx, &s_stSvcCp.stDbV2x, (char*)pchPayload);
                if(nFrameWorkRet != FRAMEWORK_OK)
                {
                    PrintError("MSG_MANAGER_Transmit() is failed! [nRet:%d]", nFrameWorkRet);
                }
                else
                {
                    PrintDebug("Tx Success, Counts[%u], Delay[%d ms]", s_stSvcCp.stMsgManagerTx.unTxCount, s_stSvcCp.stMsgManagerTx.unTxDelay);
                }
            }
            else
            {
                nFrameWorkRet = DB_MANAGER_Write(&s_stSvcCp.stDbManagerWrite, &s_stSvcCp.stDbV2x, (char*)pchPayload);
                if(nFrameWorkRet != FRAMEWORK_OK)
                {
                    PrintError("DB_MANAGER_Write() is failed! [nRet:%d]", nFrameWorkRet);
                }
            }

            usleep((s_stSvcCp.stMsgManagerTx.unTxDelay * USLEEP_MS));
        }
        else
        {
            if(s_stSvcCp.bLogLevel == TRUE)
            {
                PrintError("s_stSvcCp.eSvcCpStatus [%d]", s_stSvcCp.eSvcCpStatus);
            }
            usleep(1000);
        }
    }

    return NULL;
}

static void *P_SVC_CP_Task(void *arg)
{
    SVC_CP_EVENT_MSG_T stEventMsg;
    int32_t nRet = APP_ERROR;

    UNUSED(arg);

    memset(&stEventMsg, 0, sizeof(SVC_CP_EVENT_MSG_T));

    while (1)
    {
        if(msgrcv(s_nSvcCpTaskMsgId, &stEventMsg, sizeof(SVC_CP_EVENT_MSG_T), 0, MSG_NOERROR) == APP_MSG_ERR)
        {
            PrintError("msgrcv() is failed!");
        }
        else
        {
            switch(stEventMsg.eEventType)
            {
                case SVC_CP_EVENT_START:
                {
                    nRet = P_SVC_CP_Start(&stEventMsg);
                    if (nRet != APP_OK)
                    {
                        PrintError("SVC_CP_Start() is failed! [unRet:%d]", nRet);
                    }
                    break;
                }

                case SVC_CP_EVENT_STOP:
                {
                    nRet = P_SVC_CP_Stop(&stEventMsg);
                    if (nRet != APP_OK)
                    {
                        PrintError("SVC_CP_Stop() is failed! [unRet:%d]", nRet);
                    }
                    break;
                }

                default:
                    PrintWarn("unknown event type [%d]", stEventMsg.eEventType);
                    break;
            }
        }

        usleep(1000);
    }

    return NULL;
}

static void P_SVC_CP_PrintMsgInfo(int msqid)
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

int32_t P_SVC_CP_CreateTask(void)
{
	int32_t nRet = APP_ERROR;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    nRet = pthread_create(&sh_SvcCpTask, &attr, P_SVC_CP_Task, NULL);
    if (nRet != APP_OK)
    {
        PrintError("pthread_join() is failed!! (P_SVC_CP_Task) [nRet:%d]", nRet);
    }
    else
    {
        PrintTrace("P_SVC_CP_Task() is successfully created.");
        nRet = APP_OK;
    }

    nRet = pthread_create(&sh_SvcCpTaskTx, &attr, P_SVC_CP_TaskTx, NULL);
    if (nRet != APP_OK)
    {
        PrintError("pthread_join() is failed!! (P_SVC_CP_TaskTx) [nRet:%d]", nRet);
    }
    else
    {
        PrintTrace("P_SVC_CP_TaskTx() is successfully created.");
        nRet = APP_OK;
    }

#if defined(CONFIG_PTHREAD_JOINABLE)
    nRet = pthread_join(sh_SvcCpTask, NULL);
    if (nRet != APP_OK)
    {
        PrintError("pthread_join() is failed!! (P_SVC_CP_Task) [nRet:%d]", nRet);
    }
    else
    {
        PrintDebug("P_SVC_CP_Task() is successfully joined.");
        nRet = APP_OK;
    }
#endif
	return nRet;
}

static int32_t P_SVC_CP_Init(SVC_CP_T *pstSvcCp)
{
    int32_t nRet = APP_ERROR;

    if(pstSvcCp == NULL)
    {
        PrintError("pstSvcCp == NULL!!");
        return nRet;
    }

    if((s_nSvcCpTaskMsgId = msgget(s_SvcCpTaskMsgKey, IPC_CREAT|0666)) == APP_MSG_ERR)
    {
        PrintError("msgget() is failed!");
        return nRet;
    }
    else
    {
        P_SVC_CP_PrintMsgInfo(s_nSvcCpTaskMsgId);
    }

    nRet = P_SVC_CP_CreateTask();
    if (nRet != APP_OK)
    {
        PrintError("P_SVC_CP_CreateTask() is failed! [nRet:%d]", nRet);
    }

    (void*)memset(&pstSvcCp->stDbManagerWrite, 0x00, sizeof(DB_MANAGER_WRITE_T));
    (void*)memset(&pstSvcCp->stMsgManagerTx, 0x00, sizeof(MSG_MANAGER_TX_T));
    (void*)memset(&pstSvcCp->stMsgManagerRx, 0x00, sizeof(MSG_MANAGER_RX_T));
    (void*)memset(&pstSvcCp->stDbV2x, 0x00, sizeof(DB_V2X_T));
    (void*)memset(&pstSvcCp->stDbV2xStatusTx, 0x00, sizeof(DB_V2X_STATUS_TX_T));
    (void*)memset(&s_stSvcCp, 0x00, sizeof(SVC_CP_T));

    nRet = P_SVC_CP_SetDefaultSettings(pstSvcCp);
    if(nRet != APP_OK)
    {
        PrintError("P_SVC_CP_SetDefaultSettings() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

static int32_t P_SVC_CP_DeInit(SVC_CP_T *pstSvcCp)
{
    int32_t nRet = APP_ERROR;

    if(pstSvcCp == NULL)
    {
        PrintError("pstSvcCp == NULL!!");
        return nRet;
    }

    return nRet;
}

void SVC_CP_ShowSettings(SVC_CP_T *pstSvcCp)
{
    PrintTrace("========================================================");
    PrintWarn("MSG V2X Tx Info>");
    PrintDebug(" ePayloadType[%d]", pstSvcCp->stMsgManagerTx.ePayloadType);
    PrintDebug(" eCommType[%d]", pstSvcCp->stMsgManagerTx.eCommType);
    PrintDebug(" eSignId[%d]", pstSvcCp->stMsgManagerTx.eSignId);
    PrintDebug(" eV2xFreq[%d]", pstSvcCp->stMsgManagerTx.eV2xFreq);
    PrintDebug(" ePriority[%d]", pstSvcCp->stMsgManagerTx.ePriority);
    PrintDebug(" eV2xDataRate[%d]", pstSvcCp->stMsgManagerTx.eV2xDataRate);
    PrintDebug(" eV2xTimeSlot[%d]", pstSvcCp->stMsgManagerTx.eV2xTimeSlot);
    PrintDebug(" unPsid[%d]", pstSvcCp->stMsgManagerTx.unPsid);
    PrintDebug(" nTxPower[%d]", pstSvcCp->stMsgManagerTx.nTxPower);
    PrintDebug(" unTxCount[%d]", pstSvcCp->stMsgManagerTx.unTxCount);
    PrintDebug(" unTxDelay[%d ms]", pstSvcCp->stMsgManagerTx.unTxDelay);
    for(int i = 0; i < MSG_MANAGER_MAC_LENGTH; i++)
    {
        PrintDebug(" unTxCount[i:%d][0x%x]", i, pstSvcCp->stMsgManagerTx.uchPeerMacAddr[i]);
    }

    PrintDebug(" unTransmitterProfileId[%d]", pstSvcCp->stMsgManagerTx.unTransmitterProfileId);
    PrintDebug(" unPeerL2Id[%d]", pstSvcCp->stMsgManagerTx.unPeerL2Id);

    PrintWarn("DB V2X Info>");
    PrintDebug(" eDeviceType[%d]", pstSvcCp->stDbV2x.eDeviceType);
    PrintDebug(" eTeleCommType[%d]", pstSvcCp->stDbV2x.eTeleCommType);
    PrintDebug(" unDeviceId[0x%x]", pstSvcCp->stDbV2x.unDeviceId);
    PrintDebug(" eServiceId[%d]", pstSvcCp->stDbV2x.eServiceId);
    PrintDebug(" eActionType[%d]", pstSvcCp->stDbV2x.eActionType);
    PrintDebug(" eRegionId[%d]", pstSvcCp->stDbV2x.eRegionId);
    PrintDebug(" ePayloadType[%d]", pstSvcCp->stDbV2x.ePayloadType);
    PrintDebug(" eCommId[%d]", pstSvcCp->stDbV2x.eCommId);
    PrintDebug(" usDbVer[%d.%d]", pstSvcCp->stDbV2x.usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, pstSvcCp->stDbV2x.usDbVer & CLI_DB_V2X_MINOR_MASK);
    PrintDebug(" usHwVer[0x%x]", pstSvcCp->stDbV2x.usHwVer);
    PrintDebug(" usSwVer[0x%x]", pstSvcCp->stDbV2x.usSwVer);

    PrintWarn("Device Info>");
    PrintDebug("Ethernet Interface [%s]", pstSvcCp->pchIfaceName);

    PrintWarn("V2X Status Tx Info>");
    PrintDebug(" ulTxTimeStampL1 [%ld]", pstSvcCp->stDbV2xStatusTx.ulTxTimeStampL1);
    PrintDebug(" ulTxTimeStampL2 [%ld]", pstSvcCp->stDbV2xStatusTx.ulTxTimeStampL2);
    PrintDebug(" ulTxTimeStampL3 [%ld]", pstSvcCp->stDbV2xStatusTx.ulTxTimeStampL3);
    PrintDebug(" unRxDeviceId [%d]", pstSvcCp->stDbV2xStatusTx.unRxDeviceId);
    PrintDebug(" eChannel [%d]", pstSvcCp->stDbV2xStatusTx.eChannel);
    PrintDebug(" sPower [%d]", pstSvcCp->stDbV2xStatusTx.sPower);
    PrintDebug(" eBandwidth [%d]", pstSvcCp->stDbV2xStatusTx.eBandwidth);
    PrintDebug(" eChannel [%d]", pstSvcCp->stDbV2xStatusTx.eChannel);
    PrintDebug(" usTxRatio [%d]", pstSvcCp->stDbV2xStatusTx.usTxRatio);
    PrintDebug(" nTxLatitude [%d]", pstSvcCp->stDbV2xStatusTx.stTxPosition.nTxLatitude);
    PrintDebug(" nTxLongitude [%d]", pstSvcCp->stDbV2xStatusTx.stTxPosition.nTxLongitude);
    PrintDebug(" nTxAttitude [%d]", pstSvcCp->stDbV2xStatusTx.stTxPosition.nTxAttitude);
    PrintDebug(" unSeqNum [%d]", pstSvcCp->stDbV2xStatusTx.unSeqNum);
    PrintDebug(" unContCnt [%d]", pstSvcCp->stDbV2xStatusTx.unContCnt);
    PrintDebug(" unTxVehicleSpeed [%d]", pstSvcCp->stDbV2xStatusTx.unTxVehicleSpeed);

    PrintTrace("========================================================");
}

int32_t SVC_CP_GetSettings(SVC_CP_T *pstSvcCp)
{
    int32_t nRet = APP_ERROR;

    if(pstSvcCp == NULL)
    {
        PrintError("pstSvcCp == NULL!!");
        return nRet;
    }

    nRet = P_SVC_CP_GetSettings(pstSvcCp);
    if (nRet != APP_OK)
    {
        PrintError("P_SVC_CP_GetSettings() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

int32_t SVC_CP_SetSettings(SVC_CP_T *pstSvcCp)
{
    int32_t nRet = APP_ERROR;

    if(pstSvcCp == NULL)
    {
        PrintError("pstSvcCp == NULL!!");
        return nRet;
    }

    nRet = P_SVC_CP_SetSettings(pstSvcCp);
    if(nRet != APP_OK)
    {
        PrintError("P_SVC_CP_SetSettings() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

int32_t SVC_CP_SetLog(SVC_CP_T *pstSvcCp)
{
    int32_t nRet = APP_ERROR;

    if(pstSvcCp == NULL)
    {
        PrintError("pstSvcCp == NULL!!");
        return nRet;
    }

    s_stSvcCp.bLogLevel = pstSvcCp->bLogLevel;
    PrintTrace("SET:s_stSvcCp.bLogLevel [%s]", s_stSvcCp.bLogLevel == ON ? "ON" : "OFF");

    nRet = APP_OK;

    return nRet;
}

int32_t SVC_CP_Open(SVC_CP_T *pstSvcCp)
{
    int32_t nRet = APP_ERROR;
    int32_t nFrameWorkRet = FRAMEWORK_ERROR;
    DB_MANAGER_T *pstDbManager;
    MSG_MANAGER_T *pstMsgManager;

    if(pstSvcCp == NULL)
    {
        PrintError("pstSvcCp == NULL!!");
        return nRet;
    }

    pstDbManager = FRAMEWORK_GetDbManagerInstance();
    pstDbManager->eFileType = pstSvcCp->stDbManagerWrite.eFileType;

    nFrameWorkRet = DB_MANAGER_Open(pstDbManager);
    if(nFrameWorkRet != FRAMEWORK_OK)
    {
        PrintError("DB_MANAGER_Open() is failed! [nRet:%d]", nFrameWorkRet);
        return nRet;
    }

    pstMsgManager = FRAMEWORK_GetMsgManagerInstance();
    pstMsgManager->pchIfaceName = pstSvcCp->pchIfaceName;

    nFrameWorkRet = MSG_MANAGER_Open(pstMsgManager);
    if(nFrameWorkRet != FRAMEWORK_OK)
    {
        PrintError("MSG_MANAGER_Open() is failed! [nRet:%d]", nFrameWorkRet);
        return nRet;
    }

    nRet = nFrameWorkRet;

    return nRet;
}

int32_t SVC_CP_Close(SVC_CP_T *pstSvcCp)
{
    int32_t nRet = APP_ERROR;

    PrintWarn("TODO");

    if(pstSvcCp == NULL)
    {
        PrintError("pstSvcCp == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t SVC_CP_Start(SVC_CP_T *pstSvcCp)
{
    int32_t nRet = APP_ERROR;
    SVC_CP_EVENT_MSG_T stEventMsg;

    if(pstSvcCp == NULL)
    {
        PrintError("pstSvcCp == NULL!!");
        return nRet;
    }

    stEventMsg.eEventType = SVC_CP_EVENT_START;

    if(msgsnd(s_nSvcCpTaskMsgId, &stEventMsg, sizeof(SVC_CP_EVENT_MSG_T), IPC_NOWAIT) == APP_MSG_ERR)
    {
        PrintError("msgsnd() is failed!!");
        return nRet;
    }
    else
    {
        nRet = APP_OK;
    }

    return nRet;
}

int32_t SVC_CP_Stop(SVC_CP_T *pstSvcCp)
{
    int32_t nRet = APP_ERROR;
    SVC_CP_EVENT_MSG_T stEventMsg;

    if(pstSvcCp == NULL)
    {
        PrintError("pstSvcCp == NULL!!");
        return nRet;
    }

    stEventMsg.eEventType = SVC_CP_EVENT_STOP;

    if(msgsnd(s_nSvcCpTaskMsgId, &stEventMsg, sizeof(SVC_CP_EVENT_MSG_T), IPC_NOWAIT) == APP_MSG_ERR)
    {
        PrintError("msgsnd() is failed!!");
        return nRet;
    }
    else
    {
        nRet = APP_OK;
    }

    return nRet;
}

int32_t SVC_CP_Status(SVC_CP_T *pstSvcCp)
{
    int32_t nRet = APP_ERROR;

    PrintWarn("TODO");

    if(pstSvcCp == NULL)
    {
        PrintError("pstSvcCp == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t SVC_CP_Init(SVC_CP_T *pstSvcCp)
{
    int32_t nRet = APP_ERROR;

    if(pstSvcCp == NULL)
    {
        PrintError("pstSvcCp == NULL!!");
        return nRet;
    }

    nRet = P_SVC_CP_Init(pstSvcCp);
    if(nRet != APP_OK)
    {
        PrintError("P_SVC_CP_Init() is failed! [unRet:%d]", nRet);
        return nRet;
    }
    else
    {
        PrintWarn("is successfully initialized.");
    }

    s_stSvcCp.bLogLevel = pstSvcCp->bLogLevel;
    PrintDebug("s_stSvcCp.bLogLevel [%s]", s_stSvcCp.bLogLevel == ON ? "ON" : "OFF");

    return nRet;
}

int32_t SVC_CP_DeInit(SVC_CP_T *pstSvcCp)
{
    int32_t nRet = APP_ERROR;

    if(pstSvcCp == NULL)
    {
        PrintError("pstSvcCp == NULL!!");
        return nRet;
    }

    nRet = P_SVC_CP_DeInit(pstSvcCp);
    if(nRet != APP_OK)
    {
        PrintError("P_SVC_CP_DeInit() is failed! [unRet:%d]", nRet);
        return nRet;
    }
    else
    {
        PrintWarn("is successfully initialized.");
    }

    return nRet;
}
