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
#include "app.h"
#include "di.h"
#include "db_v2x_platooning.h"

/***************************** Definition ************************************/
#define SVC_PLATOONING_GPS_SPEED_CAL_CNT_MAX    (10)

/***************************** Enum and Structure ****************************/

/***************************** Static Variable *******************************/
FILE* sh_pSvcPlatooningTxMsg;
FILE* sh_pSvcPlatooningRxMsg;

static int s_nSvcPlatooningTaskMsgId;

static key_t s_SvcPlatooningTaskMsgKey = SVC_PLATOONING_TASK_MSG_KEY;

static pthread_t sh_SvcPlatooningTask;
static pthread_t sh_SvcPlatooningTaskTx;

static SVC_PLATOONING_T s_stSvcPlatooning;
static DB_MANAGER_V2X_STATUS_T s_stDbV2xStatus;
static bool s_bFirstMsg = TRUE;

static uint16_t s_usGpsSpeedCalCnt = 0;
static uint32_t s_usLastSpeedTx;

static bool s_bSvcPlatooningLog = OFF;

static char s_chStrBufTxRxType[SVC_PLATOONING_STR_BUF_LEN];
static char s_chStrBufDevType[SVC_PLATOONING_STR_BUF_LEN];
static char s_chStrBufDevId[SVC_PLATOONING_STR_BUF_LEN];
static char s_chStrBufStartTime[SVC_PLATOONING_STR_BUF_LEN];
static char s_chStrBufEndTime[SVC_PLATOONING_STR_BUF_LEN];
static char s_chStrBufTotalTime[SVC_PLATOONING_STR_BUF_LEN];

/***************************** Function  *************************************/

int32_t P_SVC_PLATOONING_SetSettings(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;

    if (pstSvcPlatooning == NULL)
        {
        PrintError("pstSvcPlatooning is NULL!!");
        }

    memcpy(&s_stSvcPlatooning, pstSvcPlatooning, sizeof(SVC_PLATOONING_T));
    nRet = APP_OK;

    return nRet;
}

int32_t P_SVC_PLATOONING_GetSettings(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;

    if (pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning is NULL!!");
    }

    memcpy(pstSvcPlatooning, &s_stSvcPlatooning, sizeof(SVC_PLATOONING_T));
    nRet = APP_OK;

    return nRet;
}

int32_t P_SVC_PLATOONING_SetDefaultSettings(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;

    if(pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    pstSvcPlatooning->stDbManagerWrite.eFileType = DB_MANAGER_FILE_TYPE_CSV;
    pstSvcPlatooning->stDbManagerWrite.eCommMsgType = DB_MANAGER_COMM_MSG_TYPE_TX;
    pstSvcPlatooning->stDbManagerWrite.eProc = DB_MANAGER_PROC_WRITE;

    pstSvcPlatooning->stMsgManagerTx.ePayloadType = eMSG_MANAGER_PAYLOAD_TYPE_RAW;
    pstSvcPlatooning->stMsgManagerTx.eCommType = eMSG_MANAGER_COMM_TYPE_5GNRV2X;
    pstSvcPlatooning->stMsgManagerTx.eSignId = eMSG_MANAGER_SIGN_ID_UNSECURED;
    pstSvcPlatooning->stMsgManagerTx.eV2xFreq = eMSG_MANAGER_V2X_FREQ_5900;
    pstSvcPlatooning->stMsgManagerTx.ePriority = eMSG_MANAGER_PRIORITY_CV2X_PPPP_0;
    pstSvcPlatooning->stMsgManagerTx.eV2xDataRate = eMSG_MANAGER_V2X_DATA_RATE_6MBPS;
    pstSvcPlatooning->stMsgManagerTx.eV2xTimeSlot = eMSG_MANAGER_V2X_TIME_SLOT_CONTINUOUS;
    pstSvcPlatooning->stMsgManagerTx.unPsid = DB_V2X_PSID;
    pstSvcPlatooning->stMsgManagerTx.nTxPower = MSG_MANAGER_V2X_TX_POWER;
    pstSvcPlatooning->stMsgManagerTx.unTxCount = 10;
    pstSvcPlatooning->stMsgManagerTx.unTxDelay = 100;

    for(int i = 0; i < MSG_MANAGER_MAC_LENGTH; i++)
    {
        pstSvcPlatooning->stMsgManagerTx.uchPeerMacAddr[i] = 0xFF;
    }

    pstSvcPlatooning->stMsgManagerTx.unTransmitterProfileId = MSG_MANAGER_V2X_TX_PROFILE_ID;
    pstSvcPlatooning->stMsgManagerTx.unPeerL2Id = MSG_MANAGER_V2X_TX_PEER_L2_ID;

    pstSvcPlatooning->stDbV2xPt.eDbV2XPtType = eDB_V2X_PT_TYPE_NONE;
    pstSvcPlatooning->stDbV2x.eDeviceType = DB_V2X_DEVICE_TYPE_OBU;
    pstSvcPlatooning->stDbV2x.eTeleCommType = DB_V2X_TELECOMM_TYPE_5G_PC5_BROADCAST;
    pstSvcPlatooning->stDbV2x.unDeviceId = CLI_DB_V2X_DEFAULT_DEVICE_ID;
    pstSvcPlatooning->stDbV2x.eServiceId = DB_V2X_SERVICE_ID_PLATOONING;
    pstSvcPlatooning->stDbV2x.eActionType = DB_V2X_ACTION_TYPE_REQUEST;
    pstSvcPlatooning->stDbV2x.eRegionId = DB_V2X_REGION_ID_SEONGNAM;
    pstSvcPlatooning->stDbV2x.ePayloadType = DB_V2X_PAYLOAD_TYPE_PLATOONING;
    pstSvcPlatooning->stDbV2x.eCommId = DB_V2X_COMM_ID_V2V;
    pstSvcPlatooning->stDbV2x.usDbVer = (DB_V2X_VERSION_MAJOR << CLI_DB_V2X_MAJOR_SHIFT) | DB_V2X_VERSION_MINOR;
    pstSvcPlatooning->stDbV2x.usHwVer = CLI_DB_V2X_DEFAULT_HW_VER;
    pstSvcPlatooning->stDbV2x.usSwVer = CLI_DB_V2X_DEFAULT_SW_VER;

#if defined(CONFIG_EXT_DATA_FORMAT)
    pstSvcPlatooning->pchIpAddr = SVC_PLATOONING_DEFAULT_IP;
    pstSvcPlatooning->unPort = SVC_PLATOONING_DEFAULT_PORT;
#endif

    pstSvcPlatooning->pchIfaceName = SVC_PLATOONING_DEFAULT_ETH_DEV;
    pstSvcPlatooning->unPsid = SVC_PLATOONING_V2V_PSID;
    pstSvcPlatooning->stDbV2xStatusTx.stDbV2xDevL1.ulTimeStamp = 0;
    pstSvcPlatooning->stDbV2xStatusTx.stDbV2xDevL2.ulTimeStamp = 0;
    pstSvcPlatooning->stDbV2xStatusTx.stDbV2xDevL3.ulTimeStamp = 0;
    pstSvcPlatooning->stDbV2xStatusTx.unRxTargetDeviceId = 0;
    pstSvcPlatooning->stDbV2xStatusTx.usTxFreq = MSG_MANAGER_V2X_TX_FREQ;
    pstSvcPlatooning->stDbV2xStatusTx.ucTxPwr = MSG_MANAGER_V2X_TX_POWER;
    pstSvcPlatooning->stDbV2xStatusTx.ucTxBw = MSG_MANAGER_V2X_TX_BW;
    pstSvcPlatooning->stDbV2xStatusTx.ucScs = 0;
    pstSvcPlatooning->stDbV2xStatusTx.ucMcs = 0;

    pstSvcPlatooning->stDbV2xStatusTx.usTxRatio = pstSvcPlatooning->stMsgManagerTx.unTxDelay;
    pstSvcPlatooning->stDbV2xStatusTx.stTxPosition.nTxLatitude = 0;
    pstSvcPlatooning->stDbV2xStatusTx.stTxPosition.nTxLongitude = 0;
    pstSvcPlatooning->stDbV2xStatusTx.stTxPosition.nTxAttitude = 0;

    pstSvcPlatooning->stDbV2xStatusTx.unSeqNum = 1;
    pstSvcPlatooning->stDbV2xStatusTx.unContCnt = 1;
    pstSvcPlatooning->stDbV2xStatusTx.unTxVehicleSpeed = DB_MGR_DEFAULT_VEHICLE_SPEED;
    pstSvcPlatooning->stDbV2xStatusTx.unTxVehicleHeading = 0;

    pstSvcPlatooning->pchDeviceName = DB_MGR_DEFAULT_COMM_DEV_ID;
    pstSvcPlatooning->ulDbStartTime = 0;
    pstSvcPlatooning->ulDbEndTime = 0;
    pstSvcPlatooning->unDbTotalWrittenTime = 0;

    pstSvcPlatooning->stDbV2XPtLv.eLvServiceId = eDB_V2X_PT_LV_SVC_ID_PLATOONING;
    pstSvcPlatooning->stDbV2XPtLv.eLvMethodId = eDB_V2X_PT_LV_METHOD_ID_V2V;
    pstSvcPlatooning->stDbV2XPtLv.unLvLength = 0;
    pstSvcPlatooning->stDbV2XPtLv.usLvClientId = 0;
    pstSvcPlatooning->stDbV2XPtLv.usLvSessionId = 0;
    pstSvcPlatooning->stDbV2XPtLv.ucLvProtocolVer = 0;
    pstSvcPlatooning->stDbV2XPtLv.ucLvInterfaceVer = 1;
    pstSvcPlatooning->stDbV2XPtLv.eLvMsgType = eDB_V2X_PT_LV_MSG_TYPE_BROADCAST;
    pstSvcPlatooning->stDbV2XPtLv.ucLvReturnCode = 0;
    pstSvcPlatooning->stDbV2XPtLv.eLvVehicleType = eDB_V2X_PT_LV_VEHICLE_TYPE_UNKNOWN;
    memset(pstSvcPlatooning->stDbV2XPtLv.szLvVehicleId, 0, DB_V2X_PT_LV_VEHICLE_ID_LEN);
    strcpy((char*)pstSvcPlatooning->stDbV2XPtLv.szLvVehicleId, DB_V2X_PT_LV_VEHICLE_ID_C_VEH);
    PrintDebug("Vehicle ID: %s\n", pstSvcPlatooning->stDbV2XPtLv.szLvVehicleId);
    strcpy((char*)pstSvcPlatooning->stDbV2XPtLv.szLvVehicleNum, DB_V2X_PT_LV_VEHICLE_NUM_IONIQ5);
    pstSvcPlatooning->stDbV2XPtLv.usLvMsgCount = 0;
    pstSvcPlatooning->stDbV2XPtLv.eLvMsgId = eDB_V2X_PT_LV_MSG_ID_INVALID;
    pstSvcPlatooning->stDbV2XPtLv.nLvLatitude = 0;
    pstSvcPlatooning->stDbV2XPtLv.nLvLongitude = 0;
    pstSvcPlatooning->stDbV2XPtLv.usLvHeading = 0;
    pstSvcPlatooning->stDbV2XPtLv.usLvSpeed = DB_MGR_DEFAULT_VEHICLE_SPEED;
    strcpy((char*)pstSvcPlatooning->stDbV2XPtLv.szLvDriveLaneId, DB_V2X_PT_LV_LANE_DEFAULT);
    pstSvcPlatooning->stDbV2XPtLv.eLvDriveStatus = eDB_V2X_PT_LV_DRIVE_STATUS_STAY_LANE;
    pstSvcPlatooning->stDbV2XPtLv.eLvChangeCode = eDB_V2X_PT_LV_CHANGE_NO;
    pstSvcPlatooning->stDbV2XPtLv.usLvPathId = 0;
    strcpy((char*)pstSvcPlatooning->stDbV2XPtLv.szLvLaneId, DB_V2X_PT_LV_LANE_DEFAULT);
    pstSvcPlatooning->stDbV2XPtLv.eLvLanePlan = eDB_V2X_PT_LV_LANE_PLAN_STAY;
    pstSvcPlatooning->stDbV2XPtLv.eLvCrossway = eDB_V2X_PT_LV_GEN_LANE;
    pstSvcPlatooning->stDbV2XPtLv.eLvLaneManeuver = eDB_V2X_PT_LV_LANE_STRAIGHT;
    memset(pstSvcPlatooning->stDbV2XPtLv.stLvPathPlan.anLvLatitude, 0, sizeof(pstSvcPlatooning->stDbV2XPtLv.stLvPathPlan.anLvLatitude));
    memset(pstSvcPlatooning->stDbV2XPtLv.stLvPathPlan.anLvLongitude, 0, sizeof(pstSvcPlatooning->stDbV2XPtLv.stLvPathPlan.anLvLongitude));

    nRet = APP_OK;

    PrintDebug("P_SVC_PLATOONING_SetDefaultSettings() set is finished.[eth:%s,ip:%s,port:%d]", pstSvcPlatooning->pchIfaceName, pstSvcPlatooning->pchIpAddr, pstSvcPlatooning->unPort);

    return nRet;
}

static int32_t P_SVC_PLATOONING_Start(SVC_PLATOONING_EVENT_MSG_T *stEventMsg)
{
    int32_t nRet = APP_ERROR;
    TIME_MANAGER_T *pstTimeManager;
    UNUSED(stEventMsg);

    if ((s_stSvcPlatooning.eSvcPlatooningStatus == SVC_PLATOONING_STATUS_STOP) || (s_stSvcPlatooning.eSvcPlatooningStatus == SVC_PLATOONING_STATUS_IDLE))
    {
        s_stSvcPlatooning.eSvcPlatooningStatus = SVC_PLATOONING_STATUS_START;

        pstTimeManager = FRAMEWORK_GetTimeManagerInstance();
        if(pstTimeManager == NULL)
        {
            PrintError("pstTimeManager is NULL!");
        }

        nRet = TIME_MANAGER_Get(pstTimeManager);
        if(nRet != FRAMEWORK_OK)
        {
            PrintError("TIME_MANAGER_Get() is failed! [nRet:%d]", nRet);
        }
        else
        {
            PrintDebug("pstTimeManager->ulTimeStamp[%ld]", pstTimeManager->ulTimeStamp);
            s_stSvcPlatooning.ulDbStartTime = pstTimeManager->ulTimeStamp;
        }

        nRet = TIME_MANAGER_SetDbTxBegin(pstTimeManager);
        if(nRet != FRAMEWORK_OK)
        {
            PrintError("TIME_MANAGER_SetDbTxBegin() is failed! [nRet:%d]", nRet);
        }

        PrintTrace("eSvcPlatooningStatus[%d] STARTs NOW [Time:%ld]", s_stSvcPlatooning.eSvcPlatooningStatus, s_stSvcPlatooning.ulDbStartTime);

        if(s_stSvcPlatooning.eSvcPlatooningStatus == SVC_PLATOONING_STATUS_START)
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

static int32_t P_SVC_PLATOONING_Stop(SVC_PLATOONING_EVENT_MSG_T *stEventMsg)
{
    int32_t nRet = APP_ERROR;
    TIME_MANAGER_T *pstTimeManager;
    UNUSED(stEventMsg);

    if(s_stSvcPlatooning.eSvcPlatooningStatus == SVC_PLATOONING_STATUS_START)
    {
        s_stSvcPlatooning.eSvcPlatooningStatus = SVC_PLATOONING_STATUS_STOP;

        pstTimeManager = FRAMEWORK_GetTimeManagerInstance();
        if(pstTimeManager == NULL)
        {
            PrintError("pstTimeManager is NULL!");
        }

        nRet = TIME_MANAGER_Get(pstTimeManager);
        if(nRet != FRAMEWORK_OK)
        {
            PrintError("TIME_MANAGER_Get() is failed! [nRet:%d]", nRet);
        }
        else
        {
            PrintDebug("pstTimeManager->ulTimeStamp[%ld]", pstTimeManager->ulTimeStamp);
            s_stSvcPlatooning.ulDbEndTime = pstTimeManager->ulTimeStamp;
        }

        nRet = TIME_MANAGER_SetDbTxEnd(pstTimeManager);
        if(nRet != FRAMEWORK_OK)
        {
            PrintError("TIME_MANAGER_SetDbTxBegin() is failed! [nRet:%d]", nRet);
        }

        s_stSvcPlatooning.unDbTotalWrittenTime = pstTimeManager->unDbTxTotalTime;

        PrintTrace("eSvcPlatooningStatus[%d] STOPs NOW [Time:%ld], unDbTotalWrittenTime[%d seconds]", s_stSvcPlatooning.eSvcPlatooningStatus, s_stSvcPlatooning.ulDbEndTime, s_stSvcPlatooning.unDbTotalWrittenTime);

        if(s_stSvcPlatooning.eSvcPlatooningStatus == SVC_PLATOONING_STATUS_STOP)
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

static void *P_SVC_PLATOONING_TaskTx(void *arg)
{
    UNUSED(arg);
    TIME_MANAGER_T *pstTimeManager;
    char *pchPayload = NULL;
    int32_t nFrameWorkRet = FRAMEWORK_ERROR;
    bool bMsgTx = TRUE;
    DI_T *pstDi;
    int32_t nRet = APP_ERROR;
    uint32_t nCurrSpeed = 0;
    double dHeading;

    while (1)
    {
        if(s_stSvcPlatooning.eSvcPlatooningStatus == SVC_PLATOONING_STATUS_START)
        {
            s_stSvcPlatooning.stDbV2x.ulPayloadLength = sizeof(s_stSvcPlatooning.stDbV2xStatusTx) + sizeof(s_stSvcPlatooning.stDbV2xPt);

            pchPayload = (char*)malloc(sizeof(char)*s_stSvcPlatooning.stDbV2x.ulPayloadLength);
            if(pchPayload == NULL)
            {
                PrintError("malloc() is failed! [NULL]");
                break;
            }

            (void*)memset(pchPayload, 0x00, sizeof(sizeof(char)*s_stSvcPlatooning.stDbV2x.ulPayloadLength));

            pstTimeManager = FRAMEWORK_GetTimeManagerInstance();
            if(pstTimeManager == NULL)
            {
                PrintError("pstTimeManager is NULL!");
            }

            /* Set at the Rx Device by Using Ext Msg */
            s_stSvcPlatooning.stDbV2xStatusTx.stDbV2xDevL1.ulTimeStamp = 0;
            s_stSvcPlatooning.stDbV2xStatusTx.stDbV2xDevL2.ulTimeStamp = 0;

            nFrameWorkRet = TIME_MANAGER_Get(pstTimeManager);
            if(nFrameWorkRet != FRAMEWORK_OK)
            {
                PrintError("TIME_MANAGER_Get() is failed! [nRet:%d]", nFrameWorkRet);
            }
            else
            {
                s_stSvcPlatooning.stDbV2x.ulTimeStamp = pstTimeManager->ulTimeStamp;

                /* Set the application timestamp before sending */
                s_stSvcPlatooning.stDbV2xStatusTx.stDbV2xDevL3.ulTimeStamp = pstTimeManager->ulTimeStamp;
            }

            pstDi = APP_GetDiInstance();
            if(pstDi == NULL)
            {
                PrintError("pstDi is NULL!");
            }

            /* Set the GPS values */
            nRet = DI_GPS_Get(&pstDi->stDiGps);
            if (nRet != DI_OK)
            {
                PrintError("DI_GPS_Get() is failed! [nRet:%d]", nRet);
            }

            s_stSvcPlatooning.stDbV2xStatusTx.stTxPosition.nTxLatitude = (int32_t)(pstDi->stDiGps.stDiGpsData.fLatitude * SVC_PLATOONING_GPS_VALUE_CONVERT);
            s_stSvcPlatooning.stDbV2xStatusTx.stTxPosition.nTxLongitude = (int32_t)(pstDi->stDiGps.stDiGpsData.fLongitude * SVC_PLATOONING_GPS_VALUE_CONVERT);
            s_stSvcPlatooning.stDbV2xStatusTx.stTxPosition.nTxAttitude = (int32_t)(pstDi->stDiGps.stDiGpsData.fAltitude * SVC_PLATOONING_GPS_VALUE_CONVERT);

            memcpy(pchPayload, (char*)&s_stSvcPlatooning.stDbV2xStatusTx, sizeof(s_stSvcPlatooning.stDbV2xStatusTx));
            memcpy(pchPayload + sizeof(s_stSvcPlatooning.stDbV2xStatusTx), (char*)&s_stSvcPlatooning.stDbV2xPt, sizeof(s_stSvcPlatooning.stDbV2xPt));

            if(s_bFirstMsg == TRUE)
            {
                s_stSvcPlatooning.stDbV2xStatusTx.unTxVehicleSpeed = DB_MGR_DEFAULT_VEHICLE_SPEED;
                s_bFirstMsg = FALSE;
                PrintWarn("stDbV2xStatus.bFirstPacket's speed is the default value [%d]", s_stSvcPlatooning.stDbV2xStatusTx.unTxVehicleSpeed);
            }
            else
            {
                if(s_usGpsSpeedCalCnt == SVC_PLATOONING_GPS_SPEED_CAL_CNT_MAX)
                {
                    s_stDbV2xStatus.stV2xGpsInfoTx.nLatitudeNow = s_stSvcPlatooning.stDbV2xStatusTx.stTxPosition.nTxLatitude;
                    s_stDbV2xStatus.stV2xGpsInfoTx.nLongitudeNow = s_stSvcPlatooning.stDbV2xStatusTx.stTxPosition.nTxLongitude;
                    s_stDbV2xStatus.stV2xGpsInfoTx.ulTimeStampNow = pstTimeManager->ulTimeStamp;

                    nCurrSpeed = DI_GPS_CalculateSpeed(&s_stDbV2xStatus.stV2xGpsInfoTx);
                    if(nCurrSpeed == 0)
                    {
                        s_stSvcPlatooning.stDbV2xStatusTx.unTxVehicleSpeed = s_usLastSpeedTx;
                    }
                    else
                    {
                        s_stSvcPlatooning.stDbV2xStatusTx.unTxVehicleSpeed = nCurrSpeed;
                    }

                    s_usLastSpeedTx = nCurrSpeed;

                    s_usGpsSpeedCalCnt = 0;
                }
            }

            if(s_usGpsSpeedCalCnt == 0)
            {
                s_stDbV2xStatus.stV2xGpsInfoTx.nLatitudeLast = s_stSvcPlatooning.stDbV2xStatusTx.stTxPosition.nTxLatitude;
                s_stDbV2xStatus.stV2xGpsInfoTx.nLongitudeLast = s_stSvcPlatooning.stDbV2xStatusTx.stTxPosition.nTxLongitude;
                s_stDbV2xStatus.stV2xGpsInfoTx.ulTimeStampLast = pstTimeManager->ulTimeStamp;
            }

            s_usGpsSpeedCalCnt++;

#if defined(CONFIG_GPS_OBU)
			s_stDbV2xStatus.stV2xGpsInfoHeadingTx.nLatitudeNow = s_stSvcPlatooning.stDbV2xStatusTx.stTxPosition.nTxLatitude;
            s_stDbV2xStatus.stV2xGpsInfoHeadingTx.nLongitudeNow = s_stSvcPlatooning.stDbV2xStatusTx.stTxPosition.nTxLongitude;
            s_stDbV2xStatus.stV2xGpsInfoHeadingTx.ulTimeStampNow = pstTimeManager->ulTimeStamp;

			dHeading = DI_GPS_CalculateHeading(&s_stDbV2xStatus.stV2xGpsInfoHeadingTx);
            if (dHeading < 0)
            {
                PrintError("DI_GPS_CalculateHeading() is failed! [dHeading:%lf]", dHeading);
            }
#else
            dHeading = DI_GPS_GetHeading(&pstDi->stDiGps);
            if (dHeading < 0)
            {
                PrintError("DI_GPS_GetHeading() is failed! [dHeading:%lf]", dHeading);
            }
#endif
            s_stSvcPlatooning.stDbV2xStatusTx.unTxVehicleHeading = (uint32_t)dHeading;

			s_stDbV2xStatus.stV2xGpsInfoHeadingTx.nLatitudeLast = s_stSvcPlatooning.stDbV2xStatusTx.stTxPosition.nTxLatitude;
            s_stDbV2xStatus.stV2xGpsInfoHeadingTx.nLongitudeLast = s_stSvcPlatooning.stDbV2xStatusTx.stTxPosition.nTxLongitude;
            s_stDbV2xStatus.stV2xGpsInfoHeadingTx.ulTimeStampLast = pstTimeManager->ulTimeStamp;

            s_stSvcPlatooning.stDbV2x.ulReserved = 0;

            if(bMsgTx == TRUE)
            {
                nFrameWorkRet = MSG_MANAGER_Transmit(&s_stSvcPlatooning.stMsgManagerTx, &s_stSvcPlatooning.stDbV2x, (char*)pchPayload);
                if(nFrameWorkRet != FRAMEWORK_OK)
                {
                    PrintError("MSG_MANAGER_Transmit() is failed! [nRet:%d]", nFrameWorkRet);
                }
            }
            else
            {
                nFrameWorkRet = DB_MANAGER_Write(&s_stSvcPlatooning.stDbManagerWrite, &s_stSvcPlatooning.stDbV2x, (char*)pchPayload);
                if(nFrameWorkRet != FRAMEWORK_OK)
                {
                    PrintError("DB_MANAGER_Write() is failed! [nRet:%d]", nFrameWorkRet);
                }
            }

            /* free(pchPayload) is free at the P_MSG_MANAGER_SendTxMsg() */

            if(s_stSvcPlatooning.stDbV2xStatusTx.unSeqNum == DB_V2X_STATUS_SEQ_NUM_MAX)
            {
                /* Reset the sequence number */
                s_stSvcPlatooning.stDbV2xStatusTx.unSeqNum = 0;
            }

            if(s_stSvcPlatooning.stDbV2xStatusTx.unContCnt == DB_V2X_STATUS_CONT_CNT_MAX)
            {
                /* Reset the continuity counter */
                s_stSvcPlatooning.stDbV2xStatusTx.unContCnt = 0;
            }

            /* Increase the sequence number */
            s_stSvcPlatooning.stDbV2xStatusTx.unSeqNum++;

            /* Increase the continuity counter */
            s_stSvcPlatooning.stDbV2xStatusTx.unContCnt++;

            usleep((s_stSvcPlatooning.stMsgManagerTx.unTxDelay * USLEEP_MS));
        }
        else
        {
            if(s_stSvcPlatooning.bLogLevel == TRUE)
            {
                PrintError("s_stSvcPlatooning.eSvcPlatooningStatus [%d]", s_stSvcPlatooning.eSvcPlatooningStatus);
            }
            usleep(1000);
        }
    }

    return NULL;
}

static void *P_SVC_PLATOONING_Task(void *arg)
{
    SVC_PLATOONING_EVENT_MSG_T stEventMsg;
    int32_t nRet = APP_ERROR;

    UNUSED(arg);

    memset(&stEventMsg, 0, sizeof(SVC_PLATOONING_EVENT_MSG_T));

    while (1)
    {
        if(msgrcv(s_nSvcPlatooningTaskMsgId, &stEventMsg, sizeof(SVC_PLATOONING_EVENT_MSG_T), 0, MSG_NOERROR) == APP_MSG_ERR)
        {
            PrintError("msgrcv() is failed!");
        }
        else
        {
            switch(stEventMsg.eEventType)
            {
                case SVC_PLATOONING_EVENT_START:
                {
                    nRet = P_SVC_PLATOONING_Start(&stEventMsg);
                    if (nRet != APP_OK)
                    {
                        PrintError("SVC_PLATOONING_Start() is failed! [unRet:%d]", nRet);
                    }
                    break;
                }

                case SVC_PLATOONING_EVENT_STOP:
                {
                    nRet = P_SVC_PLATOONING_Stop(&stEventMsg);
                    if (nRet != APP_OK)
                    {
                        PrintError("SVC_PLATOONING_Stop() is failed! [unRet:%d]", nRet);
                    }
                    break;
                }

                default:
                    PrintWarn("unKnown event type [%d]", stEventMsg.eEventType);
                    break;
            }
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

    nRet = pthread_create(&sh_SvcPlatooningTaskTx, &attr, P_SVC_PLATOONING_TaskTx, NULL);
    if (nRet != APP_OK)
    {
        PrintError("pthread_join() is failed!! (P_SVC_PLATOONING_TaskTx) [nRet:%d]", nRet);
    }
    else
    {
        PrintTrace("P_SVC_PLATOONING_TaskTx() is successfully created.");
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

    if (pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    if ((s_nSvcPlatooningTaskMsgId = msgget(s_SvcPlatooningTaskMsgKey, IPC_CREAT|0666)) == APP_MSG_ERR)
    {
        PrintError("msgget() is failed!");
        return nRet;
    }
    else
    {
        P_SVC_PLATOONING_PrintMsgInfo(s_nSvcPlatooningTaskMsgId);
    }

    nRet = P_SVC_PLATOONING_CreateTask();
    if (nRet != APP_OK)
    {
        PrintError("P_SVC_PLATOONING_CreateTask() is failed! [nRet:%d]", nRet);
    }

    (void*)memset(&pstSvcPlatooning->stDbManagerWrite, 0x00, sizeof(DB_MANAGER_WRITE_T));
    (void*)memset(&pstSvcPlatooning->stMsgManagerTx, 0x00, sizeof(MSG_MANAGER_TX_T));
    (void*)memset(&pstSvcPlatooning->stMsgManagerRx, 0x00, sizeof(MSG_MANAGER_RX_T));
    (void*)memset(&pstSvcPlatooning->stDbV2x, 0x00, sizeof(DB_V2X_T));
    (void*)memset(&pstSvcPlatooning->stDbV2xStatusTx, 0x00, sizeof(DB_V2X_STATUS_TX_T));
    (void*)memset(&s_stSvcPlatooning, 0x00, sizeof(SVC_PLATOONING_T));

    nRet = P_SVC_PLATOONING_SetDefaultSettings(pstSvcPlatooning);
    if (nRet != APP_OK)
    {
        PrintError("P_SVC_PLATOONING_SetDefaultSettings() is failed! [nRet:%d]", nRet);
    }

    nRet = P_SVC_PLATOONING_SetSettings(pstSvcPlatooning);
    if (nRet != APP_OK)
    {
        PrintError("P_SVC_PLATOONING_SetSettings() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

void SVC_PLATOONING_ShowSettings(SVC_PLATOONING_T *pstSvcPlatooning)
{
    PrintTrace("========================================================");
    PrintWarn("MSG V2X Tx Info>");
    PrintDebug(" ePayloadType[%d]", pstSvcPlatooning->stMsgManagerTx.ePayloadType);
    PrintDebug(" eCommType[%d]", pstSvcPlatooning->stMsgManagerTx.eCommType);
    PrintDebug(" eSignId[%d]", pstSvcPlatooning->stMsgManagerTx.eSignId);
    PrintDebug(" eV2xFreq[%d]", pstSvcPlatooning->stMsgManagerTx.eV2xFreq);
    PrintDebug(" ePriority[%d]", pstSvcPlatooning->stMsgManagerTx.ePriority);
    PrintDebug(" eV2xDataRate[%d]", pstSvcPlatooning->stMsgManagerTx.eV2xDataRate);
    PrintDebug(" eV2xTimeSlot[%d]", pstSvcPlatooning->stMsgManagerTx.eV2xTimeSlot);
    PrintDebug(" unPsid[%d]", pstSvcPlatooning->stMsgManagerTx.unPsid);
    PrintDebug(" nTxPower[%d]", pstSvcPlatooning->stMsgManagerTx.nTxPower);
    PrintDebug(" unTxCount[%d]", pstSvcPlatooning->stMsgManagerTx.unTxCount);
    PrintDebug(" unTxDelay[%d ms]", pstSvcPlatooning->stMsgManagerTx.unTxDelay);
    for(int i = 0; i < MSG_MANAGER_MAC_LENGTH; i++)
    {
        PrintDebug(" unTxCount[i:%d][0x%x]", i, pstSvcPlatooning->stMsgManagerTx.uchPeerMacAddr[i]);
    }

    PrintDebug(" unTransmitterProfileId[%d]", pstSvcPlatooning->stMsgManagerTx.unTransmitterProfileId);
    PrintDebug(" unPeerL2Id[%d]", pstSvcPlatooning->stMsgManagerTx.unPeerL2Id);

    PrintWarn("DB V2X Info>");
    if (pstSvcPlatooning->stDbV2xPt.eDbV2XPtType == eDB_V2X_PT_TYPE_LV)
    {
        PrintDebug(" eDbV2XPtType [lv]");
    }
    else if (pstSvcPlatooning->stDbV2xPt.eDbV2XPtType == eDB_V2X_PT_TYPE_FV)
    {
        PrintDebug(" eDbV2XPtType [fv]");
    }
    else
    {
        PrintDebug(" eDbV2XPtType [none]");
    }
    PrintDebug(" eDeviceType[%d]", pstSvcPlatooning->stDbV2x.eDeviceType);
    PrintDebug(" eTeleCommType[%d]", pstSvcPlatooning->stDbV2x.eTeleCommType);
    PrintDebug(" unDeviceId[%d]", pstSvcPlatooning->stDbV2x.unDeviceId);
    PrintDebug(" eServiceId[%d]", pstSvcPlatooning->stDbV2x.eServiceId);
    PrintDebug(" eActionType[%d]", pstSvcPlatooning->stDbV2x.eActionType);
    PrintDebug(" eRegionId[%d]", pstSvcPlatooning->stDbV2x.eRegionId);
    PrintDebug(" ePayloadType[%d]", pstSvcPlatooning->stDbV2x.ePayloadType);
    PrintDebug(" eCommId[%d]", pstSvcPlatooning->stDbV2x.eCommId);
    PrintDebug(" usDbVer[%d.%d]", pstSvcPlatooning->stDbV2x.usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, pstSvcPlatooning->stDbV2x.usDbVer & CLI_DB_V2X_MINOR_MASK);
    PrintDebug(" usHwVer[0x%x]", pstSvcPlatooning->stDbV2x.usHwVer);
    PrintDebug(" usSwVer[0x%x]", pstSvcPlatooning->stDbV2x.usSwVer);

    PrintWarn("Device Info>");
    PrintDebug("Ethernet Interface [%s]", pstSvcPlatooning->pchIfaceName);
    PrintDebug("PSID [%d]", pstSvcPlatooning->unPsid);
#if defined(CONFIG_EXT_DATA_FORMAT)
    PrintDebug("pchIpAddr [%s]", pstSvcPlatooning->pchIpAddr);
    PrintDebug("unPort [%d]", pstSvcPlatooning->unPort);
#endif

    PrintDebug("pchDeviceName [%s]", pstSvcPlatooning->pchDeviceName);
    PrintDebug("ulDbStartTime [%ld]", pstSvcPlatooning->ulDbStartTime);
    PrintDebug("ulDbEndTime [%ld]", pstSvcPlatooning->ulDbEndTime);
    PrintDebug("unDbTotalWrittenTime [%d]", pstSvcPlatooning->unDbTotalWrittenTime);

    PrintWarn("V2X Status Tx Info>");
    PrintDebug(" stDbV2xDevL1.ulTimeStamp [%ld]", pstSvcPlatooning->stDbV2xStatusTx.stDbV2xDevL1.ulTimeStamp);
    PrintDebug(" stDbV2xDevL2.ulTimeStamp [%ld]", pstSvcPlatooning->stDbV2xStatusTx.stDbV2xDevL2.ulTimeStamp);
    PrintDebug(" stDbV2xDevL3.ulTimeStamp [%ld]", pstSvcPlatooning->stDbV2xStatusTx.stDbV2xDevL3.ulTimeStamp);
    PrintDebug(" unRxTargetDeviceId [%d]", pstSvcPlatooning->stDbV2xStatusTx.unRxTargetDeviceId);
    PrintDebug(" usTxFreq [%d]", pstSvcPlatooning->stDbV2xStatusTx.usTxFreq);
    PrintDebug(" ucTxPwr [%d]", pstSvcPlatooning->stDbV2xStatusTx.ucTxPwr);
    PrintDebug(" ucTxBw [%d]", pstSvcPlatooning->stDbV2xStatusTx.ucTxBw);
    PrintDebug(" ucScs [%d]", pstSvcPlatooning->stDbV2xStatusTx.ucScs);
    PrintDebug(" ucMcs [%d]", pstSvcPlatooning->stDbV2xStatusTx.ucMcs);
    PrintDebug(" usTxRatio [%d]", pstSvcPlatooning->stDbV2xStatusTx.usTxRatio);
    PrintDebug(" nTxLatitude [%d]", pstSvcPlatooning->stDbV2xStatusTx.stTxPosition.nTxLatitude);
    PrintDebug(" nTxLongitude [%d]", pstSvcPlatooning->stDbV2xStatusTx.stTxPosition.nTxLongitude);
    PrintDebug(" nTxAttitude [%d]", pstSvcPlatooning->stDbV2xStatusTx.stTxPosition.nTxAttitude);
    PrintDebug(" unSeqNum [%d]", pstSvcPlatooning->stDbV2xStatusTx.unSeqNum);
    PrintDebug(" unContCnt [%d]", pstSvcPlatooning->stDbV2xStatusTx.unContCnt);
    PrintDebug(" unTxVehicleSpeed [%d]", pstSvcPlatooning->stDbV2xStatusTx.unTxVehicleSpeed);
    PrintDebug(" unTxVehicleHeading [%d]", pstSvcPlatooning->stDbV2xStatusTx.unTxVehicleHeading);

    PrintTrace("========================================================");
}

int32_t SVC_PLATOONING_GetSettings(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;

    if (pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    nRet = P_SVC_PLATOONING_GetSettings(pstSvcPlatooning);
    if (nRet != APP_OK)
    {
        PrintError("P_SVC_PLATOONING_GetSettings() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

int32_t SVC_PLATOONING_SetSettings(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;

    if (pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    nRet = P_SVC_PLATOONING_SetSettings(pstSvcPlatooning);
    if (nRet != APP_OK)
    {
        PrintError("P_SVC_PLATOONING_SetSettings() is failed! [nRet:%d]", nRet);
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
    int32_t nFrameWorkRet = FRAMEWORK_ERROR;
    DB_MANAGER_T *pstDbManager;
    MSG_MANAGER_T *pstMsgManager;
    DI_T *pstDi;
    uint32_t nRetryCnt = 0;

    if (pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    pstDbManager = FRAMEWORK_GetDbManagerInstance();
    if (pstDbManager == NULL)
    {
        PrintError("pstDbManager == NULL!!");
        nRet = APP_ERROR;
        return nRet;
    }

    pstDbManager->eFileType = pstSvcPlatooning->stDbManagerWrite.eFileType;
    pstDbManager->eSvcType = DB_MANAGER_SVC_TYPE_PLATOONING;

    nFrameWorkRet = DB_MANAGER_Open(pstDbManager);
    if (nFrameWorkRet != FRAMEWORK_OK)
    {
        PrintError("DB_MANAGER_Open() is failed! [nRet:%d]", nFrameWorkRet);
        return nRet;
    }

    pstMsgManager = FRAMEWORK_GetMsgManagerInstance();
    if (pstMsgManager == NULL)
    {
        PrintError("pstMsgManager == NULL!!");
        nRet = APP_ERROR;
        return nRet;
    }

    pstMsgManager->pchIfaceName = pstSvcPlatooning->pchIfaceName;
    pstMsgManager->stExtMsgWsr.unPsid = pstSvcPlatooning->unPsid;
#if defined(CONFIG_EXT_DATA_FORMAT)
    pstMsgManager->pchIpAddr = pstSvcPlatooning->pchIpAddr;
    pstMsgManager->unPort = pstSvcPlatooning->unPort;
#endif

    nFrameWorkRet = MSG_MANAGER_Open(pstMsgManager);
    if (nFrameWorkRet != FRAMEWORK_OK)
    {
        PrintError("MSG_MANAGER_Open() is failed! [nRet:%d]", nFrameWorkRet);
        return nRet;
    }

    pstDi = APP_GetDiInstance();
    if (pstDi == NULL)
    {
        nRet = APP_ERROR;
        PrintError("APP_GetDiInstance() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    for(nRetryCnt = 0; nRetryCnt < SVC_PLATOONING_GPS_OPEN_RETRY_CNT; nRetryCnt++)
    {
        nRet = DI_GPS_Open(&pstDi->stDiGps);
        if (nRet != DI_OK)
        {
            PrintError("DI_GPS_Open() is failed! [nRet:%d], nRetryCne[%d/%d], retry after [%d us]", nRet, nRetryCnt, SVC_PLATOONING_GPS_OPEN_RETRY_CNT, SVC_PLATOONING_GPS_OPEN_RETRY_DELAY * USLEEP_MS);
            usleep(SVC_PLATOONING_GPS_OPEN_RETRY_DELAY * USLEEP_MS);
        }
        else
        {
            PrintTrace("DI_GPS_Open() is successfully opened");
            break;
        }
    }

    nRet = nFrameWorkRet;

    return nRet;
}

int32_t SVC_PLATOONING_Close(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;
    DI_T *pstDi;
    DB_MANAGER_T *pstDbManager;
    MSG_MANAGER_T *pstMsgManager;
    char chTempDate[SVC_PLATOONING_DATE_LEN+1];
    char chTempHour[SVC_PLATOONING_HOUR_LEN+1];
    char chTempMin[SVC_PLATOONING_MIN_LEN+1];
    char chTempSec[SVC_PLATOONING_SEC_LEN+1];

    if (pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    pstDi = APP_GetDiInstance();
    if (pstDi == NULL)
    {
        PrintError("APP_GetDiInstance() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    pstMsgManager = FRAMEWORK_GetMsgManagerInstance();
    if (pstMsgManager == NULL)
    {
        PrintError("pstMsgManager == NULL!!");
        nRet = APP_ERROR;
        return nRet;
    }

    pstMsgManager->pchIfaceName = pstSvcPlatooning->pchIfaceName;
    pstMsgManager->stExtMsgWsr.unPsid = pstSvcPlatooning->unPsid;

    nRet = MSG_MANAGER_Close(pstMsgManager);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("MSG_MANAGER_Close() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    nRet = DI_GPS_Close(&pstDi->stDiGps);
    if (nRet != DI_OK)
    {
        PrintError("DI_GPS_Close() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    pstDbManager = FRAMEWORK_GetDbManagerInstance();
    if (pstDbManager == NULL)
    {
        PrintError("FRMEWORK_GetDbManagerInstance() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    nRet = DB_MANAGER_Close(pstDbManager);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("DB_MANAGER_Close() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    /* Tx */
    sprintf(s_chStrBufTxRxType, "%s", SVC_PLATOONING_DB_TX);
    pstDbManager->stDbFile.pchTxRxType = s_chStrBufTxRxType;
    if (s_stSvcPlatooning.stDbV2x.eDeviceType == DB_V2X_DEVICE_TYPE_OBU)
    {
        sprintf(s_chStrBufDevType, "%s", SVC_PLATOONING_DEV_OBU);
    }
    else if (s_stSvcPlatooning.stDbV2x.eDeviceType == DB_V2X_DEVICE_TYPE_RSU)
    {
        sprintf(s_chStrBufDevType, "%s", SVC_PLATOONING_DEV_RSU);
    }
    else
    {
        sprintf(s_chStrBufDevType, "%s", SVC_PLATOONING_DEV_UNKNOWN);
        PrintError("unknown device type[%d]", s_stSvcPlatooning.stDbV2x.eDeviceType);
    }
    pstDbManager->stDbFile.pchDeviceType = s_chStrBufDevType;

    sprintf(s_chStrBufDevId, "%s%s", DB_V2X_DEVICE_ID_PREFIX, s_stSvcPlatooning.pchDeviceName);
    pstDbManager->stDbFile.pchDeviceId = s_chStrBufDevId;

    sprintf(s_chStrBufStartTime, "%ld", s_stSvcPlatooning.ulDbStartTime);
    strncpy(chTempDate, s_chStrBufStartTime, SVC_PLATOONING_DATE_LEN);
    chTempDate[SVC_PLATOONING_DATE_LEN] = '\0';
    strncpy(chTempHour, s_chStrBufStartTime + SVC_PLATOONING_DATE_LEN, SVC_PLATOONING_HOUR_LEN);
    chTempHour[SVC_PLATOONING_HOUR_LEN] = '\0';
    strncpy(chTempMin, s_chStrBufStartTime + SVC_PLATOONING_DATE_LEN + SVC_PLATOONING_HOUR_LEN, SVC_PLATOONING_MIN_LEN);
    chTempMin[SVC_PLATOONING_MIN_LEN] = '\0';
    strncpy(chTempSec, s_chStrBufStartTime + SVC_PLATOONING_DATE_LEN + SVC_PLATOONING_HOUR_LEN + SVC_PLATOONING_MIN_LEN, SVC_PLATOONING_SEC_LEN);
    chTempSec[SVC_PLATOONING_SEC_LEN] = '\0';
    sprintf(s_chStrBufStartTime, "%s-%s-%s-%s", chTempDate, chTempHour, chTempMin, chTempSec);
    pstDbManager->stDbFile.pchStartTime = s_chStrBufStartTime;

    sprintf(s_chStrBufEndTime, "%ld", s_stSvcPlatooning.ulDbEndTime);
    strncpy(chTempDate, s_chStrBufEndTime, SVC_PLATOONING_DATE_LEN);
    chTempDate[SVC_PLATOONING_DATE_LEN] = '\0';
    strncpy(chTempHour, s_chStrBufEndTime + SVC_PLATOONING_DATE_LEN, SVC_PLATOONING_HOUR_LEN);
    chTempHour[SVC_PLATOONING_HOUR_LEN] = '\0';
    strncpy(chTempMin, s_chStrBufEndTime + SVC_PLATOONING_DATE_LEN + SVC_PLATOONING_HOUR_LEN, SVC_PLATOONING_MIN_LEN);
    chTempMin[SVC_PLATOONING_MIN_LEN] = '\0';
    strncpy(chTempSec, s_chStrBufEndTime + SVC_PLATOONING_DATE_LEN + SVC_PLATOONING_HOUR_LEN + SVC_PLATOONING_MIN_LEN, SVC_PLATOONING_SEC_LEN);
    chTempSec[SVC_PLATOONING_SEC_LEN] = '\0';

    sprintf(s_chStrBufEndTime, "%s-%s-%s-%s", chTempDate, chTempHour, chTempMin, chTempSec);
    pstDbManager->stDbFile.pchEndTime = s_chStrBufEndTime;

    sprintf(s_chStrBufTotalTime, "%d%s", s_stSvcPlatooning.unDbTotalWrittenTime, "secs");
    pstDbManager->stDbFile.pchTotalTime = s_chStrBufTotalTime;

    nRet = DB_MANAGER_MakeDbFile(pstDbManager);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("DB_MANAGER_MakeDbFile() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    nRet = DB_MANAGER_RemoveTempFile(pstDbManager);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("DB_MANAGER_RemoveTempFile() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    nRet = DB_MANAGER_UploadFile(pstDbManager);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("DB_MANAGER_UploadFile() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    /* Rx */
    sprintf(s_chStrBufTxRxType, "%s", SVC_PLATOONING_DB_RX);
    pstDbManager->stDbFile.pchTxRxType = s_chStrBufTxRxType;

    nRet = DB_MANAGER_MakeDbFile(pstDbManager);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("DB_MANAGER_MakeDbFile() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    nRet = DB_MANAGER_RemoveTempFile(pstDbManager);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("DB_MANAGER_RemoveTempFile() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    nRet = DB_MANAGER_UploadFile(pstDbManager);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("DB_MANAGER_UploadFile() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    return nRet;
}

int32_t SVC_PLATOONING_Start(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;
    SVC_PLATOONING_EVENT_MSG_T stEventMsg;

    if (pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    (void*)memset(&stEventMsg, 0x00, sizeof(SVC_PLATOONING_EVENT_MSG_T));

    stEventMsg.eEventType = SVC_PLATOONING_EVENT_START;

    nRet = msgsnd(s_nSvcPlatooningTaskMsgId, &stEventMsg, sizeof(SVC_PLATOONING_EVENT_MSG_T), IPC_NOWAIT);
    if (nRet < 0)
    {
        PrintError("msgsnd() is failed!!, [nRet:%d]", nRet);
        return nRet;
    }
    else
    {
        nRet = APP_OK;
    }

    return nRet;
}

int32_t SVC_PLATOONING_Stop(SVC_PLATOONING_T *pstSvcPlatooning)
{
    int32_t nRet = APP_ERROR;
    SVC_PLATOONING_EVENT_MSG_T stEventMsg;

    if (pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    (void*)memset(&stEventMsg, 0x00, sizeof(SVC_PLATOONING_EVENT_MSG_T));

    stEventMsg.eEventType = SVC_PLATOONING_EVENT_STOP;

    nRet = msgsnd(s_nSvcPlatooningTaskMsgId, &stEventMsg, sizeof(SVC_PLATOONING_EVENT_MSG_T), IPC_NOWAIT);
    if (nRet < 0)
    {
        PrintError("msgsnd() is failed!!, [nRet:%d]", nRet);
        return nRet;
    }
    else
    {
        nRet = APP_OK;
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

    if (pstSvcPlatooning == NULL)
    {
        PrintError("pstSvcPlatooning == NULL!!");
        return nRet;
    }

    nRet = P_SVC_PLATOONING_Init(pstSvcPlatooning);
    if (nRet != APP_OK)
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

