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
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#if defined(CONFIG_SQLITE)
#include <sqlite3.h>
#endif
#include "db_v2x_status.h"
#include "framework.h"
#include "db_manager.h"
#include "di.h"
#include "app.h"
#include "svc_cp.h"

/***************************** Definition ************************************/

#define DB_MANAGER_DB_TEMP_PATH         "/tmp/"

#define DB_MANAGER_TXT_TX_FILE          DB_MANAGER_DB_TEMP_PATH"db_v2x_tx_temp_writing.txt"
#define DB_MANAGER_TXT_RX_FILE          DB_MANAGER_DB_TEMP_PATH"db_v2x_rx_temp_writing.txt"

#define DB_MANAGER_CSV_TX_FILE          DB_MANAGER_DB_TEMP_PATH"db_v2x_tx_temp_writing.csv"
#define DB_MANAGER_CSV_RX_FILE          DB_MANAGER_DB_TEMP_PATH"db_v2x_rx_temp_writing.csv"

#if defined(CONFIG_SQLITE)
#define DB_MANAGER_SQL_TX_FILE          DB_MANAGER_DB_TEMP_PATH"db_v2x_tx_temp_writing.db"
#define DB_MANAGER_SQL_RX_FILE          DB_MANAGER_DB_TEMP_PATH"db_v2x_rx_temp_writing.db"
#endif

#define MSG_MGR_PDR_PER_CONVERT_RATE    (100000)
#define MSG_MGR_MAX_CONT_CNT            (100)
//#define DB_MGR_TEST                   (1)

#define SVC_CP_GPS_SPEED_CAL_CNT_MAX    (10)
#define DB_MGR_TIME_US_TO_MS            (1000)

/***************************** Enum and Structure ****************************/

/***************************** Static Variable *******************************/
FILE* sh_pDbMgrTxMsg;
FILE* sh_pDbMgrRxMsg;
#if defined(CONFIG_SQLITE)
sqlite3* sh_pDbMgrTxSqlMsg;
sqlite3* sh_pDbMgrRxSqlMsg;
#endif

static int s_nDbTaskMsgId, s_nMsgTxTaskMsgId, s_nMsgRxTaskMsgId;
static key_t s_dbTaskMsgKey = FRAMEWORK_DB_TASK_MSG_KEY;
static key_t s_MsgTxTaskMsgKey = FRAMEWORK_MSG_TX_TASK_MSG_KEY;
static key_t s_MsgRxTaskMsgKey = FRAMEWORK_MSG_RX_TASK_MSG_KEY;

static pthread_t sh_DbMgrTask;

static bool s_bDbMgrLog = OFF;

static DB_MANAGER_V2X_STATUS_T s_stDbV2xStatusRx;

static int32_t P_DB_MANAGER_SetV2xStatus(DB_MANAGER_V2X_STATUS_T *pstDbV2xStatus);
static int32_t P_DB_MANAGER_GetV2xStatus(DB_MANAGER_V2X_STATUS_T *pstDbV2xStatus);

static uint16_t s_usGpsSpeedCalCnt = 0;
static uint32_t s_usLastSpeedTx;
static uint32_t s_usLastSpeedRx;

/***************************** Function  *************************************/

static int32_t P_DB_MANAGER_PrintStatus(DB_V2X_STATUS_TX_T *pstDbV2xStatusTx, DB_V2X_STATUS_RX_T *pstDbV2xStatusRx)
{
    int32_t nRet = FRAMEWORK_ERROR;
    DB_MANAGER_V2X_STATUS_T stDbV2xStatus;

    if(pstDbV2xStatusTx == NULL)
    {
        PrintError("pstDbV2xStatusTx is NULL!");
        return nRet;
    }

    if(pstDbV2xStatusRx == NULL)
    {
        PrintError("pstDbV2xStatusRx is NULL!");
        return nRet;
    }

    nRet = P_DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
    }
    else
    {
        nRet = FRAMEWORK_OK;
        if(stDbV2xStatus.unCurrentContCnt == MSG_MGR_MAX_CONT_CNT)
        {
            PrintDebug("(Tx)unSeqNum[%d], (Rx)ulTotalPacketCnt[%ld], unPdr[%.3f], unPer[%.3f], ulTotalErrCnt[%ld]", pstDbV2xStatusTx->unSeqNum, pstDbV2xStatusRx->ulTotalPacketCnt, (float)(pstDbV2xStatusRx->unPdr/1000.0f), (float)(pstDbV2xStatusRx->unPer/1000.0f), pstDbV2xStatusRx->ulTotalErrCnt);
            PrintDebug("(Tx)unTxVehicleSpeed[%d], unTxVehicleHeading[%d], (Rx)unRxVehicleSpeed[%d], unRxVehicleHeading[%d]", pstDbV2xStatusTx->unTxVehicleSpeed, pstDbV2xStatusTx->unTxVehicleHeading, pstDbV2xStatusRx->unRxVehicleSpeed, pstDbV2xStatusRx->unRxVehicleHeading);
        }
    }
    return nRet;
}

static int32_t P_DB_MANAGER_PrintStatusPt(DB_V2X_PLATOONING_T *pstDbV2xPt)
{
    int32_t nRet = FRAMEWORK_ERROR;
    DB_MANAGER_V2X_STATUS_T stDbV2xStatus;

    if(pstDbV2xPt == NULL)
    {
        PrintError("pstDbV2xPt is NULL!");
        return nRet;
    }

    nRet = P_DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
    }
    else
    {
        nRet = FRAMEWORK_OK;
        if(stDbV2xStatus.unCurrentContCnt == MSG_MGR_MAX_CONT_CNT)
        {
            PrintDebug("eDbV2XPtType[%d], usV2xGroupId[%d]", pstDbV2xPt->eDbV2XPtType, pstDbV2xPt->usV2xGroupId);
        }
    }
    return nRet;
}

static int32_t P_DB_MANAGER_PrintStatusPtLv(DB_V2X_PLATOONING_LV_T *pstDbV2XPtLv)
{
    int32_t nRet = FRAMEWORK_ERROR;
    DB_MANAGER_V2X_STATUS_T stDbV2xStatus;

    if(pstDbV2XPtLv == NULL)
    {
        PrintError("pstDbV2XPtLv is NULL!");
        return nRet;
    }

    nRet = P_DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
    }
    else
    {
        nRet = FRAMEWORK_OK;
        if(stDbV2xStatus.unCurrentContCnt == MSG_MGR_MAX_CONT_CNT)
        {
            PrintDebug("eLvServiceId[%d], eLvMethodId[%d], unLvLength[%d], usLvClientId[%d], usLvSessionId[%d], ucLvProtocolVer[%d]", pstDbV2XPtLv->eLvServiceId, pstDbV2XPtLv->eLvMethodId, pstDbV2XPtLv->unLvLength, pstDbV2XPtLv->usLvClientId, pstDbV2XPtLv->usLvSessionId, pstDbV2XPtLv->ucLvProtocolVer);
            PrintDebug("ucLvInterfaceVer[%d], eLvMsgType[%d], ucLvReturnCode[%d], eLvVehicleType[%d]", pstDbV2XPtLv->ucLvInterfaceVer, pstDbV2XPtLv->eLvMsgType, pstDbV2XPtLv->ucLvReturnCode, pstDbV2XPtLv->eLvVehicleType);
            PrintDebug("szLvVehicleId[%s], szLvVehicleNum[%s], usLvMsgCount[%d], eLvMsgId[%d]", pstDbV2XPtLv->szLvVehicleId, pstDbV2XPtLv->szLvVehicleNum, pstDbV2XPtLv->usLvMsgCount, pstDbV2XPtLv->eLvMsgId);
            PrintDebug("nLvLatitude[%d], nLvLongitude[%d], usLvHeading[%d], usLvSpeed[%d]", pstDbV2XPtLv->nLvLatitude, pstDbV2XPtLv->nLvLongitude, pstDbV2XPtLv->usLvHeading, pstDbV2XPtLv->usLvSpeed);
            PrintDebug("szLvDriveLaneId[%s], eLvDriveStatus[%d], eLvChangeCode[%d]", pstDbV2XPtLv->szLvDriveLaneId, pstDbV2XPtLv->eLvDriveStatus, pstDbV2XPtLv->eLvChangeCode);
            PrintDebug("usLvPathId[%d], szLvLaneId[%s], eLvLanePlan[%d]", pstDbV2XPtLv->usLvPathId, pstDbV2XPtLv->szLvLaneId, pstDbV2XPtLv->eLvLanePlan);
            PrintDebug("eLvCrossway[%d], eLvLaneManeuver[%d]", pstDbV2XPtLv->eLvCrossway, pstDbV2XPtLv->eLvLaneManeuver);
            for (int i=0; i < DB_V2X_PT_LV_PATH_PLAN_MAX_LEN; i++)
            {
                PrintDebug("anLvLatitude[%d] = %d", i, pstDbV2XPtLv->stLvPathPlan.anLvLatitude[i]);
            }
            for (int i=0; i < DB_V2X_PT_LV_PATH_PLAN_MAX_LEN; i++)
            {
                PrintDebug("anLvLongitude[%d] = %d", i, pstDbV2XPtLv->stLvPathPlan.anLvLongitude[i]);
            }
            PrintDebug("unReserved1[%ld]", pstDbV2XPtLv->unReserved1);
        }
    }
    return nRet;
}

static int32_t P_DB_MANAGER_PrintStatusPtFv(DB_V2X_PLATOONING_FV_T *pstDbV2XPtFv)
{
    int32_t nRet = FRAMEWORK_ERROR;
    DB_MANAGER_V2X_STATUS_T stDbV2xStatus;

    if(pstDbV2XPtFv == NULL)
    {
        PrintError("pstDbV2XPtFv is NULL!");
        return nRet;
    }

    nRet = P_DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
    }
    else
    {
        nRet = FRAMEWORK_OK;
        if(stDbV2xStatus.unCurrentContCnt == MSG_MGR_MAX_CONT_CNT)
        {
            PrintDebug("eFvServiceId[%d], eFvMethodId[%d], unFvLength[%d], usFvClientId[%d], usFvSessionId[%d], ucFvProtocolVer[%d]", pstDbV2XPtFv->eFvServiceId, pstDbV2XPtFv->eFvMethodId, pstDbV2XPtFv->unFvLength, pstDbV2XPtFv->usFvClientId, pstDbV2XPtFv->usFvSessionId, pstDbV2XPtFv->ucFvProtocolVer);
            PrintDebug("ucFvInterfaceVer[%d], eFvMsgType[%d], ucFvReturnCode[%d], eFvVehicleType[%d]", pstDbV2XPtFv->ucFvInterfaceVer, pstDbV2XPtFv->eFvMsgType, pstDbV2XPtFv->ucFvReturnCode, pstDbV2XPtFv->eFvVehicleType);
            PrintDebug("szFvVehicleId[%s], szFvVehicleNum[%s], usFvMsgCount[%d], eFvMsgId[%d]", pstDbV2XPtFv->szFvVehicleId, pstDbV2XPtFv->szFvVehicleNum, pstDbV2XPtFv->usFvMsgCount, pstDbV2XPtFv->eFvMsgId);
            PrintDebug("nFvLatitude[%d], nFvLongitude[%d], usFvHeading[%d], usFvSpeed[%d]", pstDbV2XPtFv->nFvLatitude, pstDbV2XPtFv->nFvLongitude, pstDbV2XPtFv->usFvHeading, pstDbV2XPtFv->usFvSpeed);
            PrintDebug("szFvDriveLaneId[%s], eFvDriveStatus[%d], eFvChangeCode[%d]", pstDbV2XPtFv->szFvDriveLaneId, pstDbV2XPtFv->eFvDriveStatus, pstDbV2XPtFv->eFvChangeCode);
            for (int i=0; i < DB_V2X_PT_FV_PATH_PLAN_MAX_LEN; i++)
            {
                PrintDebug("anFvLatitude[%d] = %d", i, pstDbV2XPtFv->stFvPathPlan.anFvLatitude[i]);
            }
            for (int i=0; i < DB_V2X_PT_FV_PATH_PLAN_MAX_LEN; i++)
            {
                PrintDebug("anFvLongitude[%d] = %d", i, pstDbV2XPtFv->stFvPathPlan.anFvLongitude[i]);
            }
            PrintDebug("usFvRecommDistance[%d], usFvRecommSpeed[%d]", pstDbV2XPtFv->usFvRecommDistance, pstDbV2XPtFv->usFvRecommSpeed);
            PrintDebug("unReserved1[%ld], unReserved2[%ld], unReserved3[%ld], unReserved4[%ld], unReserved5[%d], unReserved6[%d]", pstDbV2XPtFv->unReserved1, pstDbV2XPtFv->unReserved2, pstDbV2XPtFv->unReserved3, pstDbV2XPtFv->unReserved4, pstDbV2XPtFv->unReserved5, pstDbV2XPtFv->unReserved6);
        }
    }
    return nRet;
}

static int32_t P_DB_MANAGER_UpdateStatus(DB_MANAGER_EVENT_MSG_T *pstEventMsg, DB_V2X_STATUS_TX_T *pstDbV2xStatusTx, DB_V2X_STATUS_RX_T *pstDbV2xStatusRx)
{
    int32_t nRet = FRAMEWORK_ERROR;
    uint32_t nCurrSpeedTx;
    uint32_t nCurrSpeedRx;
    TIME_MANAGER_T *pstTimeManager;
    DI_T *pstDi;
    DB_MANAGER_V2X_STATUS_T stDbV2xStatus;
    float fTemp = 0.0f;
    double dRxlat, dRxLon, dTxLat, dTxLon, dDistMeter;
    double dHeading;

    if(pstEventMsg == NULL)
    {
        PrintError("pstEventMsg is NULL!");
        return nRet;
    }

    if(pstDbV2xStatusTx == NULL)
    {
        PrintError("pstDbV2xStatusTx is NULL!");
        return nRet;
    }

    if(pstDbV2xStatusRx == NULL)
    {
        PrintError("pstDbV2xStatusRx is NULL!");
        return nRet;
    }

    nRet = P_DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
    }

    if(pstEventMsg->pstDbV2x->ulTimeStamp != stDbV2xStatus.ulTxTimeStamp)
    {
        PrintError("MSG Rx Task Timestamp[%ld], DB Checked Tx Timestamp[%ld]", pstEventMsg->pstDbV2x->ulTimeStamp, stDbV2xStatus.ulTxTimeStamp);
    }

    pstTimeManager = FRAMEWORK_GetTimeManagerInstance();
    if(pstTimeManager == NULL)
    {
        PrintError("pstTimeManager is NULL!");
    }

    pstDbV2xStatusTx->stDbV2xDevL1.ulTimeStamp = stDbV2xStatus.stV2xStatusTx.stDbV2xDevL1.ulTimeStamp;
    pstDbV2xStatusTx->stDbV2xDevL2.ulTimeStamp = stDbV2xStatus.stV2xStatusTx.stDbV2xDevL2.ulTimeStamp;

    pstDbV2xStatusRx->stDbV2xDevL1.ulTimeStamp = stDbV2xStatus.stV2xStatusRx.stDbV2xDevL1.ulTimeStamp;
    pstDbV2xStatusRx->stDbV2xDevL2.ulTimeStamp  = stDbV2xStatus.stV2xStatusRx.stDbV2xDevL2.ulTimeStamp;

    nRet = TIME_MANAGER_Get(pstTimeManager);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("TIME_MANAGER_Get() is failed! [nRet:%d]", nRet);
        pstDbV2xStatusRx->stDbV2xDevL3.ulTimeStamp = 0;
    }
    else
    {
        pstDbV2xStatusRx->stDbV2xDevL3.ulTimeStamp = pstTimeManager->ulTimeStamp;
    }

    pstDbV2xStatusRx->stDbV2xDevL1.ulLatency = pstDbV2xStatusRx->stDbV2xDevL1.ulTimeStamp - pstDbV2xStatusTx->stDbV2xDevL1.ulTimeStamp;
    pstDbV2xStatusRx->stDbV2xDevL2.ulLatency = pstDbV2xStatusRx->stDbV2xDevL2.ulTimeStamp - pstDbV2xStatusTx->stDbV2xDevL2.ulTimeStamp;
    pstDbV2xStatusRx->stDbV2xDevL3.ulLatency = pstDbV2xStatusRx->stDbV2xDevL3.ulTimeStamp - pstDbV2xStatusTx->stDbV2xDevL3.ulTimeStamp;

    if(s_bDbMgrLog == TRUE)
    {
        /* L1 e.g. 2023-09-14:13:40:10.8606 - 2023-09-14-13:00:10.7814 = 792us */
        PrintTrace("Latency of Layer #1: Rx[%ld]-Tx[%ld]=[%ld]us", pstDbV2xStatusRx->stDbV2xDevL1.ulTimeStamp, pstDbV2xStatusTx->stDbV2xDevL1.ulTimeStamp, pstDbV2xStatusRx->stDbV2xDevL1.ulLatency);
        PrintTrace("Latency of Layer #2: Rx[%ld]-Tx[%ld]=[%ld]us", pstDbV2xStatusRx->stDbV2xDevL2.ulTimeStamp, pstDbV2xStatusTx->stDbV2xDevL2.ulTimeStamp, pstDbV2xStatusRx->stDbV2xDevL2.ulLatency);
        PrintTrace("Latency of Layer #3: Rx[%ld]-Tx[%ld]=[%ld]us", pstDbV2xStatusRx->stDbV2xDevL3.ulTimeStamp, pstDbV2xStatusTx->stDbV2xDevL3.ulTimeStamp, pstDbV2xStatusRx->stDbV2xDevL3.ulLatency);
    }

    pstDbV2xStatusRx->stDbV2xDevL1.unDevId = stDbV2xStatus.stV2xStatusRx.stDbV2xDevL1.unDevId;
    pstDbV2xStatusRx->stDbV2xDevL2.unDevId = stDbV2xStatus.stV2xStatusRx.stDbV2xDevL2.unDevId;
    pstDbV2xStatusRx->stDbV2xDevL3.unDevId = stDbV2xStatus.stV2xStatusRx.stDbV2xDevL2.unDevId; /* TODO-Update App Dev ID */

    pstDbV2xStatusRx->stDbV2xDevL1.usSwVer = stDbV2xStatus.stV2xStatusRx.stDbV2xDevL1.usSwVer;
    pstDbV2xStatusRx->stDbV2xDevL2.usSwVer = stDbV2xStatus.stV2xStatusRx.stDbV2xDevL2.usSwVer;
    pstDbV2xStatusRx->stDbV2xDevL3.usSwVer = pstEventMsg->pstDbV2x->usSwVer;

    pstDbV2xStatusRx->stDbV2xDevL1.usHwVer = stDbV2xStatus.stV2xStatusRx.stDbV2xDevL1.unDevId;
    pstDbV2xStatusRx->stDbV2xDevL2.usHwVer = stDbV2xStatus.stV2xStatusRx.stDbV2xDevL2.unDevId;
    pstDbV2xStatusRx->stDbV2xDevL3.usHwVer = pstEventMsg->pstDbV2x->usHwVer;

    pstDbV2xStatusTx->stDbV2xDevL1.unDevId = stDbV2xStatus.stV2xStatusTx.stDbV2xDevL1.unDevId;
    pstDbV2xStatusTx->stDbV2xDevL2.unDevId = stDbV2xStatus.stV2xStatusTx.stDbV2xDevL2.unDevId;
    pstDbV2xStatusTx->stDbV2xDevL3.unDevId = stDbV2xStatus.stV2xStatusTx.stDbV2xDevL2.unDevId; /* TODO-Update App Dev ID */

    pstDbV2xStatusTx->stDbV2xDevL1.usSwVer = stDbV2xStatus.stV2xStatusTx.stDbV2xDevL1.usSwVer;
    pstDbV2xStatusTx->stDbV2xDevL2.usSwVer = stDbV2xStatus.stV2xStatusTx.stDbV2xDevL2.usSwVer;

    pstDbV2xStatusTx->stDbV2xDevL1.usHwVer = stDbV2xStatus.stV2xStatusTx.stDbV2xDevL1.usHwVer;
    pstDbV2xStatusTx->stDbV2xDevL2.usHwVer = stDbV2xStatus.stV2xStatusTx.stDbV2xDevL2.usHwVer;

    pstDbV2xStatusTx->ucTxPwr = stDbV2xStatus.stV2xStatusTx.ucTxPwr;
    pstDbV2xStatusTx->usTxFreq = stDbV2xStatus.stV2xStatusTx.usTxFreq;
    pstDbV2xStatusTx->ucTxBw = stDbV2xStatus.stV2xStatusTx.ucTxBw;
    pstDbV2xStatusTx->ucScs = stDbV2xStatus.stV2xStatusTx.ucScs;
    pstDbV2xStatusTx->ucMcs = stDbV2xStatus.stV2xStatusTx.ucMcs;

    pstDbV2xStatusRx->unTotalCommDevCnt = DB_MGR_DEFAULT_COMM_DEV_CNT;
    pstDbV2xStatusRx->nRssi = stDbV2xStatus.stV2xStatusRx.nRssi;
    pstDbV2xStatusRx->ucRcpi = stDbV2xStatus.stV2xStatusRx.ucRcpi;
    pstDbV2xStatusRx->eRsvLevel = stDbV2xStatus.stV2xStatusRx.eRsvLevel;

#if defined(CONFIG_GPS_OBU)
    UNUSED(pstDi);

    dRxlat = (double)stDbV2xStatus.stV2xGpsInfoRx.nLatitudeNow / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    dRxLon = (double)stDbV2xStatus.stV2xGpsInfoRx.nLongitudeNow / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;

    dTxLat = (double)stDbV2xStatus.stV2xGpsInfoTx.nLatitudeNow / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    dTxLon = (double)stDbV2xStatus.stV2xGpsInfoTx.nLatitudeNow / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;

    dDistMeter = DI_GPS_CalculateDistance(dRxlat, dRxLon, dTxLat, dTxLon);

    pstDbV2xStatusTx->stTxPosition.nTxLatitude = stDbV2xStatus.stV2xGpsInfoTx.nLatitudeNow;
    pstDbV2xStatusTx->stTxPosition.nTxLongitude = stDbV2xStatus.stV2xGpsInfoTx.nLongitudeNow;
    pstDbV2xStatusTx->stTxPosition.nTxAttitude = 0;

    pstDbV2xStatusRx->stRxPosition.unCommDistance = (uint32_t)(dDistMeter * SVC_CP_GPS_VALUE_CONVERT);

    pstDbV2xStatusRx->stRxPosition.nRxLatitude = stDbV2xStatus.stV2xGpsInfoRx.nLatitudeNow;
    pstDbV2xStatusRx->stRxPosition.nRxLongitude = stDbV2xStatus.stV2xGpsInfoRx.nLongitudeNow;
    pstDbV2xStatusRx->stRxPosition.nRxAttitude = 0;

    stDbV2xStatus.stV2xGpsInfoHeadingRx.nLatitudeNow = pstDbV2xStatusRx->stRxPosition.nRxLatitude;
    stDbV2xStatus.stV2xGpsInfoHeadingRx.nLongitudeNow = pstDbV2xStatusRx->stRxPosition.nRxLongitude;
    stDbV2xStatus.stV2xGpsInfoHeadingRx.ulTimeStampNow = pstTimeManager->ulTimeStamp;

    dHeading = DI_GPS_CalculateHeading(&stDbV2xStatus.stV2xGpsInfoHeadingTx);
    if (dHeading < 0)
    {
        PrintError("DI_GPS_CalculateHeading() is failed! [dHeading:%lf]", dHeading);
    }

    pstDbV2xStatusRx->unRxVehicleHeading = (uint32_t)dHeading;

	stDbV2xStatus.stV2xGpsInfoHeadingRx.nLatitudeLast = pstDbV2xStatusRx->stRxPosition.nRxLatitude;
    stDbV2xStatus.stV2xGpsInfoHeadingRx.nLongitudeLast = pstDbV2xStatusRx->stRxPosition.nRxLongitude;
    stDbV2xStatus.stV2xGpsInfoHeadingRx.ulTimeStampLast = pstTimeManager->ulTimeStamp;
#else
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

    dRxlat = (double)pstDi->stDiGps.stDiGpsData.fLatitude;
    dRxLon = (double)pstDi->stDiGps.stDiGpsData.fLongitude;

    dTxLat = (double)pstDbV2xStatusTx->stTxPosition.nTxLatitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    dTxLon = (double)pstDbV2xStatusTx->stTxPosition.nTxLongitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;

    dDistMeter = DI_GPS_CalculateDistance(dRxlat, dRxLon, dTxLat, dTxLon);
    pstDbV2xStatusRx->stRxPosition.unCommDistance = (uint32_t)(dDistMeter * SVC_CP_GPS_VALUE_CONVERT);

    pstDbV2xStatusRx->stRxPosition.nRxLatitude = (int32_t)(pstDi->stDiGps.stDiGpsData.fLatitude * SVC_CP_GPS_VALUE_CONVERT);
    pstDbV2xStatusRx->stRxPosition.nRxLongitude = (int32_t)(pstDi->stDiGps.stDiGpsData.fLongitude * SVC_CP_GPS_VALUE_CONVERT);
    pstDbV2xStatusRx->stRxPosition.nRxAttitude = (int32_t)(pstDi->stDiGps.stDiGpsData.fAltitude * SVC_CP_GPS_VALUE_CONVERT);

    dHeading = DI_GPS_GetHeading(&pstDi->stDiGps);
    if (nRet != DI_OK)
    {
        PrintError("DI_GPS_GetHeading() is failed! [nRet:%d]", nRet);
    }

    pstDbV2xStatusRx->unRxVehicleHeading = (uint32_t)dHeading;
#endif

    if(stDbV2xStatus.bFirstPacket == TRUE)
    {
        pstDbV2xStatusTx->unTxVehicleSpeed = DB_MGR_DEFAULT_VEHICLE_SPEED;
        PrintWarn("stDbV2xStatus.bFirstPacket's speed is the default value [%d]", pstDbV2xStatusTx->unTxVehicleSpeed);
        pstDbV2xStatusRx->unRxVehicleSpeed = DB_MGR_DEFAULT_VEHICLE_SPEED;
        PrintWarn("stDbV2xStatus.bFirstPacket's speed is the default value [%d]", pstDbV2xStatusRx->unRxVehicleSpeed);
    }
    else
    {
        if(s_usGpsSpeedCalCnt == SVC_CP_GPS_SPEED_CAL_CNT_MAX)
        {
            stDbV2xStatus.stV2xGpsInfoTx.nLatitudeNow = pstDbV2xStatusTx->stTxPosition.nTxLatitude;
            stDbV2xStatus.stV2xGpsInfoTx.nLongitudeNow = pstDbV2xStatusTx->stTxPosition.nTxLongitude;
            stDbV2xStatus.stV2xGpsInfoTx.ulTimeStampNow = pstTimeManager->ulTimeStamp;
            stDbV2xStatus.stV2xGpsInfoRx.nLatitudeNow = pstDbV2xStatusRx->stRxPosition.nRxLatitude;
            stDbV2xStatus.stV2xGpsInfoRx.nLongitudeNow = pstDbV2xStatusRx->stRxPosition.nRxLongitude;
            stDbV2xStatus.stV2xGpsInfoRx.ulTimeStampNow = pstTimeManager->ulTimeStamp;

            nCurrSpeedTx = DI_GPS_CalculateSpeed(&stDbV2xStatus.stV2xGpsInfoTx);
            nCurrSpeedRx = DI_GPS_CalculateSpeed(&stDbV2xStatus.stV2xGpsInfoRx);
            if(nCurrSpeedTx == 0 && nCurrSpeedRx == 0)
            {
                stDbV2xStatus.stV2xStatusTx.unTxVehicleSpeed = s_usLastSpeedTx;
                stDbV2xStatus.stV2xStatusRx.unRxVehicleSpeed = s_usLastSpeedRx;
            }
            else if(nCurrSpeedTx == 0)
            {
                stDbV2xStatus.stV2xStatusTx.unTxVehicleSpeed = s_usLastSpeedTx;
                stDbV2xStatus.stV2xStatusRx.unRxVehicleSpeed = nCurrSpeedRx;
            }
            else if(nCurrSpeedRx == 0)
            {
                stDbV2xStatus.stV2xStatusTx.unTxVehicleSpeed = nCurrSpeedTx;
                stDbV2xStatus.stV2xStatusRx.unRxVehicleSpeed = s_usLastSpeedRx;
            }
            else
            {
                stDbV2xStatus.stV2xStatusTx.unTxVehicleSpeed = nCurrSpeedTx;
                stDbV2xStatus.stV2xStatusRx.unRxVehicleSpeed = nCurrSpeedRx;
            }

            s_usLastSpeedTx = nCurrSpeedTx;
            s_usLastSpeedRx = nCurrSpeedRx;

            s_usGpsSpeedCalCnt = 0;
        }
    }

    stDbV2xStatus.unCurrentContCnt = pstDbV2xStatusTx->unContCnt;

    if(stDbV2xStatus.bFirstPacket == TRUE)
    {
        stDbV2xStatus.unLastContCnt = pstDbV2xStatusTx->unContCnt;
        stDbV2xStatus.bFirstPacket = FALSE;
        stDbV2xStatus.stV2xStatusRx.ulTotalPacketCnt = pstDbV2xStatusTx->unSeqNum;
        PrintDebug("update unLastContCnt as the first packet's unCountCnt [%d], stDbV2xStatus.bFirstPacket[%d]", stDbV2xStatus.unLastContCnt, stDbV2xStatus.bFirstPacket);
        PrintDebug("update ulTotalPacketCnt[%ld] as the unSeqNum[%d]", pstDbV2xStatusRx->ulTotalPacketCnt, pstDbV2xStatusTx->unSeqNum);
    }

    if(stDbV2xStatus.unLastContCnt != stDbV2xStatus.unCurrentContCnt)
    {
        PrintError("ContCnt does not be matched! [+1 increased unLastContCnt:%d], [unCurrentContCnt:%d]", stDbV2xStatus.unLastContCnt, stDbV2xStatus.unCurrentContCnt);
        stDbV2xStatus.unContCntLoss++;
        stDbV2xStatus.stV2xStatusRx.ulTotalErrCnt++;
        stDbV2xStatus.stV2xStatusRx.ucErrIndicator = TRUE;
        PrintWarn("Increased unContCntLoss[%d], ulTotalErrCnt[%ld], set ucErrIndicator[%d]", stDbV2xStatus.unContCntLoss, stDbV2xStatus.stV2xStatusRx.ulTotalErrCnt, stDbV2xStatus.stV2xStatusRx.ucErrIndicator);
    }

    if(s_bDbMgrLog  == TRUE)
    {
        PrintDebug("[+1 increased unLastContCnt:%d] == [unCurrentContCnt:%d]", stDbV2xStatus.unLastContCnt, stDbV2xStatus.unCurrentContCnt);
    }

    pstDbV2xStatusRx->ucErrIndicator = stDbV2xStatus.stV2xStatusRx.ucErrIndicator;
    pstDbV2xStatusRx->ulTotalPacketCnt = stDbV2xStatus.stV2xStatusRx.ulTotalPacketCnt;

    stDbV2xStatus.unLastContCnt = stDbV2xStatus.unCurrentContCnt;
    stDbV2xStatus.unLastContCnt++;
    if(stDbV2xStatus.unLastContCnt > DB_V2X_STATUS_CONT_CNT_MAX)
    {
        stDbV2xStatus.unLastContCnt = 1;
        if(s_bDbMgrLog  == TRUE)
        {
            PrintWarn("Reset unLastContCnt as [%d]", stDbV2xStatus.unLastContCnt);
        }
    }

    pstDbV2xStatusRx->ulTotalErrCnt = stDbV2xStatus.stV2xStatusRx.ulTotalErrCnt;
    fTemp = (float)pstDbV2xStatusRx->ulTotalPacketCnt/(float)pstDbV2xStatusTx->unSeqNum;
    pstDbV2xStatusRx->unPdr = (uint32_t)(fTemp*MSG_MGR_PDR_PER_CONVERT_RATE);
    pstDbV2xStatusRx->unPer = MSG_MGR_PDR_PER_CONVERT_RATE - pstDbV2xStatusRx->unPdr;

    if(s_usGpsSpeedCalCnt == 0)
    {
        stDbV2xStatus.stV2xGpsInfoTx.nLatitudeLast = pstDbV2xStatusTx->stTxPosition.nTxLatitude;
        stDbV2xStatus.stV2xGpsInfoTx.nLongitudeLast = pstDbV2xStatusTx->stTxPosition.nTxLongitude;
        stDbV2xStatus.stV2xGpsInfoTx.ulTimeStampLast = pstTimeManager->ulTimeStamp;
        stDbV2xStatus.stV2xGpsInfoRx.nLatitudeLast = pstDbV2xStatusRx->stRxPosition.nRxLatitude;
        stDbV2xStatus.stV2xGpsInfoRx.nLongitudeLast = pstDbV2xStatusRx->stRxPosition.nRxLongitude;
        stDbV2xStatus.stV2xGpsInfoRx.ulTimeStampLast = pstTimeManager->ulTimeStamp;
    }

    s_usGpsSpeedCalCnt++;

    nRet = P_DB_MANAGER_SetV2xStatus(&stDbV2xStatus);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

#if defined(CONFIG_SQLITE)
static int32_t P_DB_MANAGER_WriteSqlite(DB_MANAGER_EVENT_MSG_T *pstEventMsg)
{
    int32_t nRet = FRAMEWORK_ERROR;
    char *pchPayload = NULL;
    char *ErrorMsg = NULL;
    int sql_TxStatus;
    int sql_RxStatus;
    const char* TxCreate = NULL;
    const char* RxCreate = NULL;

    if(pstEventMsg == NULL)
    {
        PrintError("pstEventMsg is NULL!");
        return nRet;
    }

    char *InsertTxData = (char *)malloc(sizeof(char)*pstEventMsg->pstDbV2x->ulPayloadLength);
    if(InsertTxData == NULL)
        {
            PrintError("InsertTxData_malloc() is failed! [NULL]");
            return nRet;
        }
    char *InsertRxData = (char *)malloc(sizeof(char)*pstEventMsg->pstDbV2x->ulPayloadLength);
    if(InsertRxData == NULL)
        {
            PrintError("InsertRxData_malloc() is failed! [NULL]");
            return nRet;
        }

    switch (pstEventMsg->pstDbManagerWrite->eCommMsgType)
    {
        case DB_MANAGER_COMM_MSG_TYPE_TX:
        {
            if (sh_pDbMgrTxSqlMsg != NULL)
            {
                pchPayload = (char*)malloc(sizeof(char)*pstEventMsg->pstDbV2x->ulPayloadLength);
                if(pchPayload == NULL)
                {
                    PrintError("malloc() is failed! [NULL]");
                    return nRet;
                }

                memcpy(pchPayload, (char *)pstEventMsg->pPayload, pstEventMsg->pstDbV2x->ulPayloadLength);

                // Connect Database
                sql_TxStatus = sqlite3_open(DB_MANAGER_SQL_TX_FILE, &sh_pDbMgrTxSqlMsg);
                if (sql_TxStatus != SQLITE_OK)
                {
                    PrintError("Can't open Tx database : %s\n", sqlite3_errmsg(sh_pDbMgrTxSqlMsg));
                    sql_TxStatus = sqlite3_close(sh_pDbMgrTxSqlMsg);
                    if(sql_TxStatus != SQLITE_OK)
                    {
                        PrintError("Can't close Tx database : %s\n", sqlite3_errmsg(sh_pDbMgrTxSqlMsg));
                        nRet = FRAMEWORK_ERROR;
                        return nRet;
                    }
                }
                // Create Table
                if(TxCreate == NULL)
                {
                    TxCreate = "CREATE TABLE IF NOT EXISTS Txtable (eDeviceType INTEGER, eTeleCommType INTEGER, unDeviceId INTEGER, ulTimeStamp INTEGER,\
                    eServiceId INTEGER, eActionType INTEGER, eRegionId INTEGER, ePayloadType INTEGER, eCommId INTEGER, usDbVer INTEGER, usHwVer INTEGER, usSwVer INTEGER,\
                    ulPayloadLength INTEGER, unTotalpacketCrc32 INTEGER)";
                    sql_TxStatus = sqlite3_exec(sh_pDbMgrTxSqlMsg, TxCreate, 0, 0, &ErrorMsg);
                    if (sql_TxStatus != SQLITE_OK)
                    {
                        PrintError("Can't create Txtable : %s\n", ErrorMsg);
                        (void)sqlite3_free(ErrorMsg);
                        sql_TxStatus = sqlite3_close(sh_pDbMgrTxSqlMsg);
                        if(sql_TxStatus != SQLITE_OK)
                        {
                            PrintError("Can't close Tx database : %s\n", sqlite3_errmsg(sh_pDbMgrTxSqlMsg));
                            nRet = FRAMEWORK_ERROR;
                            return nRet;
                        }
                    }
                }
                // Insert data (execpt cPayload)
                if (InsertTxData != NULL)
                {
                    sprintf(InsertTxData,\
                    "INSERT INTO Txtable(eDeviceType, eTeleCommType, unDeviceId, ulTimeStamp, eServiceId, eActionType, eRegionId, ePayloadType, eCommId, usDbVer, usHwVer,\
                    usSwVer, ulPayloadLength, unTotalpacketCrc32)\
                    VALUES (%d, %d, %d, %ld, %d, %d, %d, %d, %d, %d.%d, 0x%x, 0x%x, %d, 0x%x)",\
                    pstEventMsg->pstDbV2x->eDeviceType,\
                    pstEventMsg->pstDbV2x->eTeleCommType,\
                    pstEventMsg->pstDbV2x->unDeviceId,\
                    pstEventMsg->pstDbV2x->ulTimeStamp,\
                    pstEventMsg->pstDbV2x->eServiceId,\
                    pstEventMsg->pstDbV2x->eActionType,\
                    pstEventMsg->pstDbV2x->eRegionId,\
                    pstEventMsg->pstDbV2x->ePayloadType,\
                    pstEventMsg->pstDbV2x->eCommId,\
                    pstEventMsg->pstDbV2x->usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, pstEventMsg->pstDbV2x->usDbVer & CLI_DB_V2X_MINOR_MASK,\
                    pstEventMsg->pstDbV2x->usHwVer,\
                    pstEventMsg->pstDbV2x->usSwVer,\
                    pstEventMsg->pstDbV2x->ulPayloadLength,\
                    pstEventMsg->pstDbManagerWrite->unCrc32);
                }
                sql_TxStatus = sqlite3_exec(sh_pDbMgrTxSqlMsg, InsertTxData, 0, 0, &ErrorMsg);
                if (sql_TxStatus != SQLITE_OK)
                {
                    PrintError("Can't insert Txtable : %s\n", ErrorMsg);
                    (void)sqlite3_free(ErrorMsg);
                    sql_TxStatus = sqlite3_close(sh_pDbMgrTxSqlMsg);
                    if(sql_TxStatus != SQLITE_OK)
                    {
                        PrintError("Can't close Tx database : %s\n", sqlite3_errmsg(sh_pDbMgrTxSqlMsg));
                        nRet = FRAMEWORK_ERROR;
                        return nRet;
                    }
                }
                // Close Database Connection
                sql_TxStatus = sqlite3_close(sh_pDbMgrTxSqlMsg);
                if(sql_TxStatus != SQLITE_OK)
                    {
                        PrintError("Can't close Tx database : %s\n", sqlite3_errmsg(sh_pDbMgrTxSqlMsg));
                        nRet = FRAMEWORK_ERROR;
                        return nRet;
                    }
                nRet = FRAMEWORK_OK;
                return nRet;
            }
            else
            {
                PrintError("sh_pDbMgrTxSqlMsg is NULL!!, check whethter sh_pDbMgrTxSqlMsg is opened before.");
            }

            break;
        }
        case DB_MANAGER_COMM_MSG_TYPE_RX:
        {
            if (sh_pDbMgrRxSqlMsg != NULL)
            {
                pchPayload = (char*)malloc(sizeof(char)*pstEventMsg->pstDbV2x->ulPayloadLength);
                if(pchPayload == NULL)
                {
                    PrintError("malloc() is failed! [NULL]");
                    return nRet;
                }

                memcpy(pchPayload, (char *)pstEventMsg->pPayload, pstEventMsg->pstDbV2x->ulPayloadLength);

                // Connect Database
                sql_RxStatus = sqlite3_open(DB_MANAGER_SQL_RX_FILE, &sh_pDbMgrRxSqlMsg);
                if (sql_RxStatus != SQLITE_OK)
                {
                    PrintError("Can't open Rx database : %s\n", sqlite3_errmsg(sh_pDbMgrRxSqlMsg));
                    sql_RxStatus = sqlite3_close(sh_pDbMgrRxSqlMsg);
                    if(sql_RxStatus != SQLITE_OK)
                    {
                        PrintError("Can't close Rx database : %s\n", sqlite3_errmsg(sh_pDbMgrRxSqlMsg));
                        nRet = FRAMEWORK_ERROR;
                        return nRet;
                    }
                }
                // Create Table
                if(RxCreate == NULL)
                {
                    RxCreate = "CREATE TABLE IF NOT EXISTS Rxtable (eDeviceType INTEGER, eTeleCommType INTEGER, unDeviceId INTEGER, ulTimeStamp INTEGER,\
                    eServiceId INTEGER, eActionType INTEGER, eRegionId INTEGER, ePayloadType INTEGER, eCommId INTEGER, usDbVer INTEGER, usHwVer INTEGER, usSwVer INTEGER,\
                    ulPayloadLength INTEGER, unTotalpacketCrc32 INTEGER)";
                    sql_RxStatus = sqlite3_exec(sh_pDbMgrRxSqlMsg, RxCreate, 0, 0, &ErrorMsg);
                    if (sql_RxStatus != SQLITE_OK)
                    {
                        PrintError("Can't create Rxtable : %s\n", ErrorMsg);
                        (void)sqlite3_free(ErrorMsg);
                        sql_RxStatus = sqlite3_close(sh_pDbMgrRxSqlMsg);
                        if(sql_RxStatus != SQLITE_OK)
                        {
                            PrintError("Can't close Rx database : %s\n", sqlite3_errmsg(sh_pDbMgrRxSqlMsg));
                            nRet = FRAMEWORK_ERROR;
                            return nRet;
                        }
                    }
                }
                // Insert data (execpt cPayload)
                if (InsertRxData != NULL)
                {
                    sprintf(InsertRxData,\
                    "INSERT INTO Rxtable(eDeviceType, eTeleCommType, unDeviceId, ulTimeStamp, eServiceId, eActionType, eRegionId, ePayloadType, eCommId, usDbVer, usHwVer,\
                    usSwVer, ulPayloadLength, unTotalpacketCrc32)\
                    VALUES (%d, %d, %d, %ld, %d, %d, %d, %d, %d, %d.%d, 0x%x, 0x%x, %d, 0x%x)",\
                    pstEventMsg->pstDbV2x->eDeviceType,\
                    pstEventMsg->pstDbV2x->eTeleCommType,\
                    pstEventMsg->pstDbV2x->unDeviceId,\
                    pstEventMsg->pstDbV2x->ulTimeStamp,\
                    pstEventMsg->pstDbV2x->eServiceId,\
                    pstEventMsg->pstDbV2x->eActionType,\
                    pstEventMsg->pstDbV2x->eRegionId,\
                    pstEventMsg->pstDbV2x->ePayloadType,\
                    pstEventMsg->pstDbV2x->eCommId,\
                    pstEventMsg->pstDbV2x->usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, pstEventMsg->pstDbV2x->usDbVer & CLI_DB_V2X_MINOR_MASK,\
                    pstEventMsg->pstDbV2x->usHwVer,\
                    pstEventMsg->pstDbV2x->usSwVer,\
                    pstEventMsg->pstDbV2x->ulPayloadLength,\
                    pstEventMsg->pstDbManagerWrite->unCrc32);
                }
                sql_RxStatus = sqlite3_exec(sh_pDbMgrRxSqlMsg, InsertRxData, 0, 0, &ErrorMsg);
                if (sql_RxStatus != SQLITE_OK)
                {
                    PrintError("Can't insert Rxtable : %s\n", ErrorMsg);
                    (void)sqlite3_free(ErrorMsg);
                    sql_RxStatus = sqlite3_close(sh_pDbMgrRxSqlMsg);
                    if(sql_RxStatus != SQLITE_OK)
                    {
                        PrintError("Can't close Rx database : %s\n", sqlite3_errmsg(sh_pDbMgrRxSqlMsg));
                        nRet = FRAMEWORK_ERROR;
                        return nRet;
                    }
                }
                // Close Database Connection
                sql_RxStatus = sqlite3_close(sh_pDbMgrRxSqlMsg);
                if(sql_RxStatus != SQLITE_OK)
                    {
                        PrintError("Can't close Rx database : %s\n", sqlite3_errmsg(sh_pDbMgrRxSqlMsg));
                        nRet = FRAMEWORK_ERROR;
                        return nRet;
                    }
                nRet = FRAMEWORK_OK;
                return nRet;
            }
            else
            {
                PrintError("sh_pDbMgrRxSqlMsg is NULL!!, check whethter sh_pDbMgrRxSqlMsg is opened before.");
            }

            break;
        }
        default:
            PrintError("unknown eCommMsgType [%d]", pstEventMsg->pstDbManagerWrite->eCommMsgType);
            break;
    }

    return nRet;
}

static int32_t P_DB_MANAGER_OpenSqlite(DB_MANAGER_T *pstDbManager)
{
    int32_t nRet = FRAMEWORK_ERROR;
    int sql_TxStatus;
    int sql_RxStatus;

    UNUSED(pstDbManager);

    if(pstDbManager == NULL)
    {
        PrintError("pstDbManager is NULL!");
        return nRet;
    }

    if(sh_pDbMgrTxSqlMsg == NULL)
    {
        sql_TxStatus = sqlite3_open(DB_MANAGER_SQL_TX_FILE, &sh_pDbMgrTxSqlMsg);
        if(sql_TxStatus != SQLITE_OK)
        {
            PrintError("Can't open Tx database!!");
        }
        else
        {
            PrintTrace("DB_MANAGER_SQL_TX_FILE[%s] is opened.", DB_MANAGER_SQL_TX_FILE);
            nRet = FRAMEWORK_OK;
        }
        sql_TxStatus = sqlite3_close(sh_pDbMgrTxSqlMsg);
        if(sql_TxStatus != SQLITE_OK)
        {
            PrintError("Can't close Tx database : %s\n", sqlite3_errmsg(sh_pDbMgrTxSqlMsg));
        }
    }

    if(sh_pDbMgrRxSqlMsg == NULL)
    {
        sql_RxStatus = sqlite3_open(DB_MANAGER_SQL_RX_FILE, &sh_pDbMgrRxSqlMsg);
        if(sql_RxStatus != SQLITE_OK)
        {
            PrintError("Can't open Rx database!!");
        }
        else
        {
            PrintTrace("DB_MANAGER_SQL_RX_FILE[%s] is opened.", DB_MANAGER_SQL_RX_FILE);
            nRet = FRAMEWORK_OK;
        }
        sql_RxStatus = sqlite3_close(sh_pDbMgrRxSqlMsg);
        if(sql_RxStatus != SQLITE_OK)
        {
            PrintError("Can't close Rx database : %s\n", sqlite3_errmsg(sh_pDbMgrRxSqlMsg));
        }
    }

    if(sh_pDbMgrTxSqlMsg != NULL)
    {
        sql_TxStatus = sqlite3_open(DB_MANAGER_SQL_TX_FILE, &sh_pDbMgrTxSqlMsg);
        if(sql_TxStatus != SQLITE_OK)
        {
            PrintError("Can't open Tx database!!");
        }
        else
        {
            nRet = FRAMEWORK_OK;
        }
        sql_TxStatus = sqlite3_close(sh_pDbMgrTxSqlMsg);
        if(sql_TxStatus != SQLITE_OK)
        {
            PrintError("Can't close Tx database : %s\n", sqlite3_errmsg(sh_pDbMgrTxSqlMsg));
        }
    }

    if(sh_pDbMgrRxSqlMsg != NULL)
    {
        sql_RxStatus = sqlite3_open(DB_MANAGER_SQL_RX_FILE, &sh_pDbMgrRxSqlMsg);
        if(sql_RxStatus != SQLITE_OK)
        {
            PrintError("Can't open Rx database!!");
        }
        else
        {
            nRet = FRAMEWORK_OK;
        }
        sql_RxStatus = sqlite3_close(sh_pDbMgrRxSqlMsg);
        if(sql_RxStatus != SQLITE_OK)
        {
            PrintError("Can't close Rx database : %s\n", sqlite3_errmsg(sh_pDbMgrRxSqlMsg));
        }
    }

    return nRet;
}
#endif

static int32_t P_DB_MANAGER_WriteTxt(DB_MANAGER_EVENT_MSG_T *pstEventMsg)
{
    int32_t nRet = FRAMEWORK_ERROR;
    char *pchPayload = NULL;

    if(pstEventMsg == NULL)
    {
        PrintError("pstEventMsg is NULL!");
        return nRet;
    }

    switch(pstEventMsg->pstDbManagerWrite->eCommMsgType)
    {
        case DB_MANAGER_COMM_MSG_TYPE_TX:
        {
            if (sh_pDbMgrTxMsg != NULL)
            {
                pchPayload = (char*)malloc(sizeof(char)*pstEventMsg->pstDbV2x->ulPayloadLength);
                if(pchPayload == NULL)
                {
                    PrintError("malloc() is failed! [NULL]");
                    return nRet;
                }

                memcpy(pchPayload, (char *)pstEventMsg->pPayload, pstEventMsg->pstDbV2x->ulPayloadLength);

                fprintf(sh_pDbMgrTxMsg, "eDeviceType[%d], ", pstEventMsg->pstDbV2x->eDeviceType);
                fprintf(sh_pDbMgrTxMsg, "eTeleCommType[%d], ", pstEventMsg->pstDbV2x->eTeleCommType);
                fprintf(sh_pDbMgrTxMsg, "unDeviceId[%d], ", pstEventMsg->pstDbV2x->unDeviceId);
                fprintf(sh_pDbMgrTxMsg, "ulTimeStamp[%ld], ", pstEventMsg->pstDbV2x->ulTimeStamp);
                fprintf(sh_pDbMgrTxMsg, "eServiceId[%d], ", pstEventMsg->pstDbV2x->eServiceId);
                fprintf(sh_pDbMgrTxMsg, "eActionType[%d], ", pstEventMsg->pstDbV2x->eActionType);
                fprintf(sh_pDbMgrTxMsg, "eRegionId[%d], ", pstEventMsg->pstDbV2x->eRegionId);
                fprintf(sh_pDbMgrTxMsg, "ePayloadType[%d], ", pstEventMsg->pstDbV2x->ePayloadType);
                fprintf(sh_pDbMgrTxMsg, "eCommId[%d], ", pstEventMsg->pstDbV2x->eCommId);
                fprintf(sh_pDbMgrTxMsg, "usDbVer[%d.%d], ", pstEventMsg->pstDbV2x->usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, pstEventMsg->pstDbV2x->usDbVer & CLI_DB_V2X_MINOR_MASK);
                fprintf(sh_pDbMgrTxMsg, "usHwVer[0x%x], ", pstEventMsg->pstDbV2x->usHwVer);
                fprintf(sh_pDbMgrTxMsg, "usSwVer[0x%x], ", pstEventMsg->pstDbV2x->usSwVer);
                fprintf(sh_pDbMgrTxMsg, "ulPayloadLength[%d], ", pstEventMsg->pstDbV2x->ulPayloadLength);

                fprintf(sh_pDbMgrTxMsg, "cPayload[");
                for(int i = 0; i < (int)pstEventMsg->pstDbV2x->ulPayloadLength; i++)
                {
                      fprintf(sh_pDbMgrTxMsg, "%d ", pchPayload[i]);
                }
                fprintf(sh_pDbMgrTxMsg, "], ");

                fprintf(sh_pDbMgrTxMsg, "unTotalPacketCrc32[0x%x]", pstEventMsg->pstDbManagerWrite->unCrc32);
                fprintf(sh_pDbMgrTxMsg, "\r\n");

                nRet = fflush(sh_pDbMgrTxMsg);
                if (nRet < 0)
                {
                    PrintError("fflush() is failed! [unRet:%d]", nRet);
                }

                if(pchPayload != NULL)
                {
                    free(pchPayload);
                }

                if(pstEventMsg->pPayload != NULL)
                {
                    if(s_bDbMgrLog == ON)
                    {
                        PrintDebug("free [%p] allocated at P_MSG_MANAGER_SendRxMsgToDbMgr()", pstEventMsg->pPayload);
                    }
                    free(pstEventMsg->pPayload);
                }
            }
            else
            {
                PrintError("sh_pDbMgrTxMsg is NULL!!, check whethter sh_pDbMgrTxMsg is opened before.");
            }

            break;
        }
        case DB_MANAGER_COMM_MSG_TYPE_RX:
        {
            if (sh_pDbMgrRxMsg != NULL)
            {
                pchPayload = (char*)malloc(sizeof(char)*pstEventMsg->pstDbV2x->ulPayloadLength);
                if(pchPayload == NULL)
                {
                    PrintError("malloc() is failed! [NULL]");
                    return nRet;
                }

                memcpy(pchPayload, (char *)pstEventMsg->pPayload, pstEventMsg->pstDbV2x->ulPayloadLength);

                fprintf(sh_pDbMgrRxMsg, "eDeviceType[%d], ", pstEventMsg->pstDbV2x->eDeviceType);
                fprintf(sh_pDbMgrRxMsg, "eTeleCommType[%d], ", pstEventMsg->pstDbV2x->eTeleCommType);
                fprintf(sh_pDbMgrRxMsg, "unDeviceId[%d], ", pstEventMsg->pstDbV2x->unDeviceId);
                fprintf(sh_pDbMgrRxMsg, "ulTimeStamp[%ld], ", pstEventMsg->pstDbV2x->ulTimeStamp);
                fprintf(sh_pDbMgrRxMsg, "eServiceId[%d], ", pstEventMsg->pstDbV2x->eServiceId);
                fprintf(sh_pDbMgrRxMsg, "eActionType[%d], ", pstEventMsg->pstDbV2x->eActionType);
                fprintf(sh_pDbMgrRxMsg, "eRegionId[%d], ", pstEventMsg->pstDbV2x->eRegionId);
                fprintf(sh_pDbMgrRxMsg, "ePayloadType[%d], ", pstEventMsg->pstDbV2x->ePayloadType);
                fprintf(sh_pDbMgrRxMsg, "eCommId[%d], ", pstEventMsg->pstDbV2x->eCommId);
                fprintf(sh_pDbMgrRxMsg, "usDbVer[%d.%d], ", pstEventMsg->pstDbV2x->usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, pstEventMsg->pstDbV2x->usDbVer & CLI_DB_V2X_MINOR_MASK);
                fprintf(sh_pDbMgrRxMsg, "usHwVer[0x%x], ", pstEventMsg->pstDbV2x->usHwVer);
                fprintf(sh_pDbMgrRxMsg, "usSwVer[0x%x], ", pstEventMsg->pstDbV2x->usSwVer);
                fprintf(sh_pDbMgrRxMsg, "ulPayloadLength[%d], ", pstEventMsg->pstDbV2x->ulPayloadLength);

                fprintf(sh_pDbMgrRxMsg, "cPayload[");
                for(int i = 0; i < (int)pstEventMsg->pstDbV2x->ulPayloadLength; i++)
                {
                      fprintf(sh_pDbMgrRxMsg, "%d ", pchPayload[i]);
                }
                fprintf(sh_pDbMgrRxMsg, "], ");

                fprintf(sh_pDbMgrRxMsg, "unTotalPacketCrc32[0x%x]", pstEventMsg->pstDbManagerWrite->unCrc32);
                fprintf(sh_pDbMgrRxMsg, "\r\n");

                nRet = fflush(sh_pDbMgrRxMsg);
                if (nRet < 0)
                {
                    PrintError("fflush() is failed! [unRet:%d]", nRet);
                }

                if(pchPayload != NULL)
                {
                    free(pchPayload);
                }

                if(pstEventMsg->pPayload != NULL)
                {
                    if(s_bDbMgrLog == ON)
                    {
                        PrintDebug("free [%p] allocated at P_MSG_MANAGER_SendRxMsgToDbMgr()", pstEventMsg->pPayload);
                    }
                    free(pstEventMsg->pPayload);
                }
            }
            else
            {
                PrintError("sh_pDbMgrRxMsg is NULL!!, check whethter sh_pDbMgrRxMsg is opened before.");
            }

            break;
        }
        default:
            PrintError("unknown eCommMsgType [%d]", pstEventMsg->pstDbManagerWrite->eCommMsgType);
            break;
    }

    return nRet;
}

static int32_t P_DB_MANAGER_OpenCsv(DB_MANAGER_T *pstDbManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstDbManager == NULL)
    {
        PrintError("pstDbManager is NULL!");
        return nRet;
    }

    UNUSED(pstDbManager);

    if(sh_pDbMgrTxMsg == NULL)
    {
        sh_pDbMgrTxMsg = fopen(DB_MANAGER_CSV_TX_FILE, "a+");
        if(sh_pDbMgrTxMsg == NULL)
        {
            PrintError("fopen() is failed!!");
        }
        else
        {
            PrintTrace("DB_MANAGER_TX_FILE[%s] is opened.", DB_MANAGER_CSV_TX_FILE);
            nRet = FRAMEWORK_OK;
        }
    }

    if(sh_pDbMgrRxMsg == NULL)
    {
        sh_pDbMgrRxMsg = fopen(DB_MANAGER_CSV_RX_FILE, "a+");
        if(sh_pDbMgrRxMsg == NULL)
        {
            PrintError("fopen() is failed!!");
        }
        else
        {
            PrintTrace("DB_MANAGER_RX_FILE[%s] is opened.", DB_MANAGER_CSV_RX_FILE);
            nRet = FRAMEWORK_OK;
        }
    }

    if (sh_pDbMgrTxMsg != NULL)
    {
        fprintf(sh_pDbMgrTxMsg, "eDeviceType,");
        fprintf(sh_pDbMgrTxMsg, "eTeleCommType,");
        fprintf(sh_pDbMgrTxMsg, "unDeviceId,");
        fprintf(sh_pDbMgrTxMsg, "ulTimeStamp,");
        fprintf(sh_pDbMgrTxMsg, "eServiceId,");
        fprintf(sh_pDbMgrTxMsg, "eActionType,");
        fprintf(sh_pDbMgrTxMsg, "eRegionId,");
        fprintf(sh_pDbMgrTxMsg, "ePayloadType,");
        fprintf(sh_pDbMgrTxMsg, "eCommId,");
        fprintf(sh_pDbMgrTxMsg, "usDbVer,");
        fprintf(sh_pDbMgrTxMsg, "usHwVer,");
        fprintf(sh_pDbMgrTxMsg, "usSwVer,");
        fprintf(sh_pDbMgrTxMsg, "ulPayloadLength,");

        if(pstDbManager->eSvcType == DB_MANAGER_SVC_TYPE_V2X_STATUS)
        {
            fprintf(sh_pDbMgrTxMsg, "ulTxTimeStampL1,");
            fprintf(sh_pDbMgrTxMsg, "ulTxTimeStampL2,");
            fprintf(sh_pDbMgrTxMsg, "ulTxTimeStampL3,");
            fprintf(sh_pDbMgrTxMsg, "unRxTargetDeviceId,");
            fprintf(sh_pDbMgrTxMsg, "usTxFreq,");
            fprintf(sh_pDbMgrTxMsg, "ucTxPwr,");
            fprintf(sh_pDbMgrTxMsg, "ucTxBw,");
            fprintf(sh_pDbMgrTxMsg, "ucScs,");
            fprintf(sh_pDbMgrTxMsg, "ucMcs,");
            fprintf(sh_pDbMgrTxMsg, "usTxRatio,");
            fprintf(sh_pDbMgrTxMsg, "nTxLatitude,");
            fprintf(sh_pDbMgrTxMsg, "nTxLongitude,");
            fprintf(sh_pDbMgrTxMsg, "nTxAttitude,");
            fprintf(sh_pDbMgrTxMsg, "unSeqNum,");
            fprintf(sh_pDbMgrTxMsg, "unContCnt,");
            fprintf(sh_pDbMgrTxMsg, "unTxVehicleSpeed,");
            fprintf(sh_pDbMgrTxMsg, "unTxVehicleHeading,");
            fprintf(sh_pDbMgrTxMsg, "unTotalPacketCrc32");
        }
        else if(pstDbManager->eSvcType == DB_MANAGER_SVC_TYPE_PLATOONING)
        {
            /* V2X Status */
            fprintf(sh_pDbMgrTxMsg, "ulTxTimeStampL1,");
            fprintf(sh_pDbMgrTxMsg, "ulTxTimeStampL2,");
            fprintf(sh_pDbMgrTxMsg, "ulTxTimeStampL3,");
            fprintf(sh_pDbMgrTxMsg, "unRxTargetDeviceId,");
            fprintf(sh_pDbMgrTxMsg, "usTxFreq,");
            fprintf(sh_pDbMgrTxMsg, "ucTxPwr,");
            fprintf(sh_pDbMgrTxMsg, "ucTxBw,");
            fprintf(sh_pDbMgrTxMsg, "ucScs,");
            fprintf(sh_pDbMgrTxMsg, "ucMcs,");
            fprintf(sh_pDbMgrTxMsg, "usTxRatio,");
            fprintf(sh_pDbMgrTxMsg, "nTxLatitude,");
            fprintf(sh_pDbMgrTxMsg, "nTxLongitude,");
            fprintf(sh_pDbMgrTxMsg, "nTxAttitude,");
            fprintf(sh_pDbMgrTxMsg, "unSeqNum,");
            fprintf(sh_pDbMgrTxMsg, "unContCnt,");
            fprintf(sh_pDbMgrTxMsg, "unTxVehicleSpeed,");
            fprintf(sh_pDbMgrTxMsg, "unTxVehicleHeading,");
            /* PLATOONING */
            fprintf(sh_pDbMgrTxMsg, "eDbV2XPtType,");
            fprintf(sh_pDbMgrTxMsg, "usV2xGroupId,");
            if (pstDbManager->stDbV2xPt.eDbV2XPtType == eDB_V2X_PT_TYPE_LV)
            {
                fprintf(sh_pDbMgrTxMsg, "eLvServiceId,");
                fprintf(sh_pDbMgrTxMsg, "eLvMethodId,");
                fprintf(sh_pDbMgrTxMsg, "unLvLength,");
                fprintf(sh_pDbMgrTxMsg, "usLvClientId,");
                fprintf(sh_pDbMgrTxMsg, "usLvSessionId,");
                fprintf(sh_pDbMgrTxMsg, "ucLvProtocolVer,");
                fprintf(sh_pDbMgrTxMsg, "ucLvInterfaceVer,");
                fprintf(sh_pDbMgrTxMsg, "eLvMsgType,");
                fprintf(sh_pDbMgrTxMsg, "ucLvReturnCode,");
                fprintf(sh_pDbMgrTxMsg, "eLvVehicleType,");
                fprintf(sh_pDbMgrTxMsg, "szLvVehicleId,");
                fprintf(sh_pDbMgrTxMsg, "szLvVehicleNum,");
                fprintf(sh_pDbMgrTxMsg, "usLvMsgCount,");
                fprintf(sh_pDbMgrTxMsg, "eLvMsgId,");
                fprintf(sh_pDbMgrTxMsg, "nLvLatitude,");
                fprintf(sh_pDbMgrTxMsg, "nLvLongitude,");
                fprintf(sh_pDbMgrTxMsg, "usLvHeading,");
                fprintf(sh_pDbMgrTxMsg, "usLvSpeed,");
                fprintf(sh_pDbMgrTxMsg, "szLvDriveLaneId,");
                fprintf(sh_pDbMgrTxMsg, "eLvDriveStatus,");
                fprintf(sh_pDbMgrTxMsg, "eLvChangeCode,");
                fprintf(sh_pDbMgrTxMsg, "usLvPathId,");
                fprintf(sh_pDbMgrTxMsg, "szLvLaneId,");
                fprintf(sh_pDbMgrTxMsg, "eLvLanePlan,");
                fprintf(sh_pDbMgrTxMsg, "eLvCrossway,");
                fprintf(sh_pDbMgrTxMsg, "eLvLaneManeuver,");
                fprintf(sh_pDbMgrTxMsg, "anLvLatitude,");
                fprintf(sh_pDbMgrTxMsg, "anLvLongitude,");
                fprintf(sh_pDbMgrTxMsg, "unReserved1,");
            }
            else if (pstDbManager->stDbV2xPt.eDbV2XPtType == eDB_V2X_PT_TYPE_FV)
            {
                fprintf(sh_pDbMgrTxMsg, "eFvServiceId,");
                fprintf(sh_pDbMgrTxMsg, "eFvMethodId,");
                fprintf(sh_pDbMgrTxMsg, "unFvLength,");
                fprintf(sh_pDbMgrTxMsg, "usFvClientId,");
                fprintf(sh_pDbMgrTxMsg, "usFvSessionId,");
                fprintf(sh_pDbMgrTxMsg, "ucFvProtocolVer,");
                fprintf(sh_pDbMgrTxMsg, "ucFvInterfaceVer,");
                fprintf(sh_pDbMgrTxMsg, "eFvMsgType,");
                fprintf(sh_pDbMgrTxMsg, "ucFvReturnCode,");
                fprintf(sh_pDbMgrTxMsg, "eFvVehicleType,");
                fprintf(sh_pDbMgrTxMsg, "szFvVehicleId,");
                fprintf(sh_pDbMgrTxMsg, "szFvVehicleNum,");
                fprintf(sh_pDbMgrTxMsg, "usFvMsgCount,");
                fprintf(sh_pDbMgrTxMsg, "eFvMsgId,");
                fprintf(sh_pDbMgrTxMsg, "nFvLatitude,");
                fprintf(sh_pDbMgrTxMsg, "nFvLongitude,");
                fprintf(sh_pDbMgrTxMsg, "usFvHeading,");
                fprintf(sh_pDbMgrTxMsg, "usFvSpeed,");
                fprintf(sh_pDbMgrTxMsg, "szFvDriveLaneId,");
                fprintf(sh_pDbMgrTxMsg, "eFvDriveStatus,");
                fprintf(sh_pDbMgrTxMsg, "eFvChangeCode,");
                fprintf(sh_pDbMgrTxMsg, "anFvLatitude,");
                fprintf(sh_pDbMgrTxMsg, "anFvLongitude,");
                fprintf(sh_pDbMgrTxMsg, "usFvRecommDistance,");
                fprintf(sh_pDbMgrTxMsg, "usFvRecommSpeed,");
                fprintf(sh_pDbMgrTxMsg, "unReserved1,");
                fprintf(sh_pDbMgrTxMsg, "unReserved2,");
                fprintf(sh_pDbMgrTxMsg, "unReserved3,");
                fprintf(sh_pDbMgrTxMsg, "unReserved4,");
                fprintf(sh_pDbMgrTxMsg, "unReserved5,");
                fprintf(sh_pDbMgrTxMsg, "unReserved6,");
            }
            else
            {
                PrintError("unknown eDbV2XPtType[%d]", pstDbManager->stDbV2xPt.eDbV2XPtType);
            }
            /* CRC */
            fprintf(sh_pDbMgrTxMsg, "unTotalPacketCrc32");
        }
        else if(pstDbManager->eSvcType == DB_MANAGER_SVC_TYPE_BASE)
        {
            fprintf(sh_pDbMgrTxMsg, "cPayload,");
            fprintf(sh_pDbMgrTxMsg, "unTotalPacketCrc32");
        }
        else
        {
            PrintError("unknown eSvcType[nRet:%d]", pstDbManager->eSvcType);
            return nRet;
        }

        fprintf(sh_pDbMgrTxMsg, "\r\n");
    }

    nRet = fflush(sh_pDbMgrTxMsg);
    if (nRet < 0)
    {
        PrintError("fflush() is failed! [unRet:%d]", nRet);
    }

    if (sh_pDbMgrRxMsg != NULL)
    {
        fprintf(sh_pDbMgrRxMsg, "eDeviceType,");
        fprintf(sh_pDbMgrRxMsg, "eTeleCommType,");
        fprintf(sh_pDbMgrRxMsg, "unDeviceId,");
        fprintf(sh_pDbMgrRxMsg, "ulTimeStamp,");
        fprintf(sh_pDbMgrRxMsg, "eServiceId,");
        fprintf(sh_pDbMgrRxMsg, "eActionType,");
        fprintf(sh_pDbMgrRxMsg, "eRegionId,");
        fprintf(sh_pDbMgrRxMsg, "ePayloadType,");
        fprintf(sh_pDbMgrRxMsg, "eCommId,");
        fprintf(sh_pDbMgrRxMsg, "usDbVer,");
        fprintf(sh_pDbMgrRxMsg, "usHwVer,");
        fprintf(sh_pDbMgrRxMsg, "usSwVer,");
        fprintf(sh_pDbMgrRxMsg, "ulPayloadLength,");

        if(pstDbManager->eSvcType == DB_MANAGER_SVC_TYPE_V2X_STATUS)
        {
            /* Tx */
            fprintf(sh_pDbMgrRxMsg, "ulTxTimeStampL1,");
            fprintf(sh_pDbMgrRxMsg, "ulTxTimeStampL2,");
            fprintf(sh_pDbMgrRxMsg, "ulTxTimeStampL3,");
            fprintf(sh_pDbMgrRxMsg, "unTxDeviceIdL1,");
            fprintf(sh_pDbMgrRxMsg, "unTxDeviceIdL2,");
            fprintf(sh_pDbMgrRxMsg, "unTxDeviceIdL3,");
            fprintf(sh_pDbMgrRxMsg, "usTxSwVerL1,");
            fprintf(sh_pDbMgrRxMsg, "usTxSwVerL2,");
            fprintf(sh_pDbMgrRxMsg, "usTxSwVerL3,");
            fprintf(sh_pDbMgrRxMsg, "usTxHwVerL1,");
            fprintf(sh_pDbMgrRxMsg, "usTxHwVerL2,");
            fprintf(sh_pDbMgrRxMsg, "usTxHwVerL3,");
            fprintf(sh_pDbMgrRxMsg, "unRxTargetDeviceId,");
            fprintf(sh_pDbMgrRxMsg, "usTxFreq,");
            fprintf(sh_pDbMgrRxMsg, "ucTxPwr,");
            fprintf(sh_pDbMgrRxMsg, "ucTxBw,");
            fprintf(sh_pDbMgrRxMsg, "ucScs,");
            fprintf(sh_pDbMgrRxMsg, "ucMcs,");
            fprintf(sh_pDbMgrRxMsg, "usTxRatio,");
            fprintf(sh_pDbMgrRxMsg, "nTxLatitude,");
            fprintf(sh_pDbMgrRxMsg, "nTxLongitude,");
            fprintf(sh_pDbMgrRxMsg, "nTxAttitude,");
            fprintf(sh_pDbMgrRxMsg, "unSeqNum,");
            fprintf(sh_pDbMgrRxMsg, "unContCnt,");
            fprintf(sh_pDbMgrRxMsg, "unTxVehicleSpeed,");
            fprintf(sh_pDbMgrRxMsg, "unTxVehicleHeading,");
            fprintf(sh_pDbMgrRxMsg, "unTotalPacketCrc32,");

            /* Rx */
            fprintf(sh_pDbMgrRxMsg, "ulRxTimeStampL1,");
            fprintf(sh_pDbMgrRxMsg, "ulRxTimeStampL2,");
            fprintf(sh_pDbMgrRxMsg, "ulRxTimeStampL3,");
            fprintf(sh_pDbMgrRxMsg, "ulLatencyL1(us),");
            fprintf(sh_pDbMgrRxMsg, "ulLatencyL2(us),");
            fprintf(sh_pDbMgrRxMsg, "ulLatencyL3(us),");
            fprintf(sh_pDbMgrRxMsg, "unRxDeviceIdL1,");
            fprintf(sh_pDbMgrRxMsg, "unRxDeviceIdL2,");
            fprintf(sh_pDbMgrRxMsg, "unRxDeviceIdL3,");
            fprintf(sh_pDbMgrRxMsg, "usRxSwVerL1,");
            fprintf(sh_pDbMgrRxMsg, "usRxSwVerL2,");
            fprintf(sh_pDbMgrRxMsg, "usRxSwVerL3,");
            fprintf(sh_pDbMgrRxMsg, "usRxHwVerL1,");
            fprintf(sh_pDbMgrRxMsg, "usRxHwVerL2,");
            fprintf(sh_pDbMgrRxMsg, "usRxHwVerL3,");
            fprintf(sh_pDbMgrRxMsg, "unRxVehicleSpeed,");
            fprintf(sh_pDbMgrRxMsg, "unRxVehicleHeading,");
            fprintf(sh_pDbMgrRxMsg, "unTotalCommDevCnt,");
            fprintf(sh_pDbMgrRxMsg, "nRssi,");
            fprintf(sh_pDbMgrRxMsg, "ucRcpi,");
            fprintf(sh_pDbMgrRxMsg, "eRsvLevel,");
            fprintf(sh_pDbMgrRxMsg, "usCommDistance,");
            fprintf(sh_pDbMgrRxMsg, "nRxLatitude,");
            fprintf(sh_pDbMgrRxMsg, "nRxLongitude,");
            fprintf(sh_pDbMgrRxMsg, "nRxAttitude,");
            fprintf(sh_pDbMgrRxMsg, "ucErrIndicator,");
            fprintf(sh_pDbMgrRxMsg, "ulTotalPacketCnt,");
            fprintf(sh_pDbMgrRxMsg, "ulTotalErrCnt,");
            fprintf(sh_pDbMgrRxMsg, "unPdr(percent),");
            fprintf(sh_pDbMgrRxMsg, "unPer(percent)");
        }
        else if(pstDbManager->eSvcType == DB_MANAGER_SVC_TYPE_PLATOONING)
        {
            /* PT Tx */
            fprintf(sh_pDbMgrRxMsg, "ulTxTimeStampL1,");
            fprintf(sh_pDbMgrRxMsg, "ulTxTimeStampL2,");
            fprintf(sh_pDbMgrRxMsg, "ulTxTimeStampL3,");
            fprintf(sh_pDbMgrRxMsg, "unTxDeviceIdL1,");
            fprintf(sh_pDbMgrRxMsg, "unTxDeviceIdL2,");
            fprintf(sh_pDbMgrRxMsg, "unTxDeviceIdL3,");
            fprintf(sh_pDbMgrRxMsg, "usTxSwVerL1,");
            fprintf(sh_pDbMgrRxMsg, "usTxSwVerL2,");
            fprintf(sh_pDbMgrRxMsg, "usTxSwVerL3,");
            fprintf(sh_pDbMgrRxMsg, "usTxHwVerL1,");
            fprintf(sh_pDbMgrRxMsg, "usTxHwVerL2,");
            fprintf(sh_pDbMgrRxMsg, "usTxHwVerL3,");
            fprintf(sh_pDbMgrRxMsg, "unRxTargetDeviceId,");
            fprintf(sh_pDbMgrRxMsg, "usTxFreq,");
            fprintf(sh_pDbMgrRxMsg, "ucTxPwr,");
            fprintf(sh_pDbMgrRxMsg, "ucTxBw,");
            fprintf(sh_pDbMgrRxMsg, "ucScs,");
            fprintf(sh_pDbMgrRxMsg, "ucMcs,");
            fprintf(sh_pDbMgrRxMsg, "usTxRatio,");
            fprintf(sh_pDbMgrRxMsg, "nTxLatitude,");
            fprintf(sh_pDbMgrRxMsg, "nTxLongitude,");
            fprintf(sh_pDbMgrRxMsg, "nTxAttitude,");
            fprintf(sh_pDbMgrRxMsg, "unSeqNum,");
            fprintf(sh_pDbMgrRxMsg, "unContCnt,");
            fprintf(sh_pDbMgrRxMsg, "unTxVehicleSpeed,");
            fprintf(sh_pDbMgrRxMsg, "unTxVehicleHeading,");
            fprintf(sh_pDbMgrRxMsg, "unTotalPacketCrc32,");

            /* PT Rx */
            fprintf(sh_pDbMgrRxMsg, "ulRxTimeStampL1,");
            fprintf(sh_pDbMgrRxMsg, "ulRxTimeStampL2,");
            fprintf(sh_pDbMgrRxMsg, "ulRxTimeStampL3,");
            fprintf(sh_pDbMgrRxMsg, "ulLatencyL1(us),");
            fprintf(sh_pDbMgrRxMsg, "ulLatencyL2(us),");
            fprintf(sh_pDbMgrRxMsg, "ulLatencyL3(us),");
            fprintf(sh_pDbMgrRxMsg, "unRxDeviceIdL1,");
            fprintf(sh_pDbMgrRxMsg, "unRxDeviceIdL2,");
            fprintf(sh_pDbMgrRxMsg, "unRxDeviceIdL3,");
            fprintf(sh_pDbMgrRxMsg, "usRxSwVerL1,");
            fprintf(sh_pDbMgrRxMsg, "usRxSwVerL2,");
            fprintf(sh_pDbMgrRxMsg, "usRxSwVerL3,");
            fprintf(sh_pDbMgrRxMsg, "usRxHwVerL1,");
            fprintf(sh_pDbMgrRxMsg, "usRxHwVerL2,");
            fprintf(sh_pDbMgrRxMsg, "usRxHwVerL3,");
            fprintf(sh_pDbMgrRxMsg, "unRxVehicleSpeed,");
            fprintf(sh_pDbMgrRxMsg, "unRxVehicleHeading,");
            fprintf(sh_pDbMgrRxMsg, "unTotalCommDevCnt,");
            fprintf(sh_pDbMgrRxMsg, "nRssi,");
            fprintf(sh_pDbMgrRxMsg, "ucRcpi,");
            fprintf(sh_pDbMgrRxMsg, "eRsvLevel,");
            fprintf(sh_pDbMgrRxMsg, "usCommDistance,");
            fprintf(sh_pDbMgrRxMsg, "nRxLatitude,");
            fprintf(sh_pDbMgrRxMsg, "nRxLongitude,");
            fprintf(sh_pDbMgrRxMsg, "nRxAttitude,");
            fprintf(sh_pDbMgrRxMsg, "ucErrIndicator,");
            fprintf(sh_pDbMgrRxMsg, "ulTotalPacketCnt,");
            fprintf(sh_pDbMgrRxMsg, "ulTotalErrCnt,");
            fprintf(sh_pDbMgrRxMsg, "unPdr(percent),");
            fprintf(sh_pDbMgrRxMsg, "unPer(percent),");
            fprintf(sh_pDbMgrRxMsg, "eDbV2XPtType,");
            fprintf(sh_pDbMgrRxMsg, "usV2xGroupId,");

            if (pstDbManager->stDbV2xPt.eDbV2XPtType == eDB_V2X_PT_TYPE_LV)
            {
                fprintf(sh_pDbMgrRxMsg, "eFvServiceId,");
                fprintf(sh_pDbMgrRxMsg, "eFvMethodId,");
                fprintf(sh_pDbMgrRxMsg, "unFvLength,");
                fprintf(sh_pDbMgrRxMsg, "usFvClientId,");
                fprintf(sh_pDbMgrRxMsg, "usFvSessionId,");
                fprintf(sh_pDbMgrRxMsg, "ucFvProtocolVer,");
                fprintf(sh_pDbMgrRxMsg, "ucFvInterfaceVer,");
                fprintf(sh_pDbMgrRxMsg, "eFvMsgType,");
                fprintf(sh_pDbMgrRxMsg, "ucFvReturnCode,");
                fprintf(sh_pDbMgrRxMsg, "eFvVehicleType,");
                fprintf(sh_pDbMgrRxMsg, "szFvVehicleId,");
                fprintf(sh_pDbMgrRxMsg, "szFvVehicleNum,");
                fprintf(sh_pDbMgrRxMsg, "usFvMsgCount,");
                fprintf(sh_pDbMgrRxMsg, "eFvMsgId,");
                fprintf(sh_pDbMgrRxMsg, "nFvLatitude,");
                fprintf(sh_pDbMgrRxMsg, "nFvLongitude,");
                fprintf(sh_pDbMgrRxMsg, "usFvHeading,");
                fprintf(sh_pDbMgrRxMsg, "usFvSpeed,");
                fprintf(sh_pDbMgrRxMsg, "szFvDriveLaneId,");
                fprintf(sh_pDbMgrRxMsg, "eFvDriveStatus,");
                fprintf(sh_pDbMgrRxMsg, "eFvChangeCode,");
                fprintf(sh_pDbMgrRxMsg, "anFvLatitude,");
                fprintf(sh_pDbMgrRxMsg, "anFvLongitude,");
                fprintf(sh_pDbMgrRxMsg, "usFvRecommDistance,");
                fprintf(sh_pDbMgrRxMsg, "usFvRecommSpeed,");
                fprintf(sh_pDbMgrRxMsg, "unReserved1,");
                fprintf(sh_pDbMgrRxMsg, "unReserved2,");
                fprintf(sh_pDbMgrRxMsg, "unReserved3,");
                fprintf(sh_pDbMgrRxMsg, "unReserved4,");
                fprintf(sh_pDbMgrRxMsg, "unReserved5,");
                fprintf(sh_pDbMgrRxMsg, "unReserved6");
            }
            else if (pstDbManager->stDbV2xPt.eDbV2XPtType == eDB_V2X_PT_TYPE_FV)
            {
                fprintf(sh_pDbMgrRxMsg, "eLvServiceId,");
                fprintf(sh_pDbMgrRxMsg, "eLvMethodId,");
                fprintf(sh_pDbMgrRxMsg, "unLvLength,");
                fprintf(sh_pDbMgrRxMsg, "usLvClientId,");
                fprintf(sh_pDbMgrRxMsg, "usLvSessionId,");
                fprintf(sh_pDbMgrRxMsg, "ucLvProtocolVer,");
                fprintf(sh_pDbMgrRxMsg, "ucLvInterfaceVer,");
                fprintf(sh_pDbMgrRxMsg, "eLvMsgType,");
                fprintf(sh_pDbMgrRxMsg, "ucLvReturnCode,");
                fprintf(sh_pDbMgrRxMsg, "eLvVehicleType,");
                fprintf(sh_pDbMgrRxMsg, "szLvVehicleId,");
                fprintf(sh_pDbMgrRxMsg, "szLvVehicleNum,");
                fprintf(sh_pDbMgrRxMsg, "usLvMsgCount,");
                fprintf(sh_pDbMgrRxMsg, "eLvMsgId,");
                fprintf(sh_pDbMgrRxMsg, "nLvLatitude,");
                fprintf(sh_pDbMgrRxMsg, "nLvLongitude,");
                fprintf(sh_pDbMgrRxMsg, "usLvHeading,");
                fprintf(sh_pDbMgrRxMsg, "usLvSpeed,");
                fprintf(sh_pDbMgrRxMsg, "szLvDriveLaneId,");
                fprintf(sh_pDbMgrRxMsg, "eLvDriveStatus,");
                fprintf(sh_pDbMgrRxMsg, "eLvChangeCode,");
                fprintf(sh_pDbMgrRxMsg, "usLvPathId,");
                fprintf(sh_pDbMgrRxMsg, "szLvLaneId,");
                fprintf(sh_pDbMgrRxMsg, "eLvLanePlan,");
                fprintf(sh_pDbMgrRxMsg, "eLvCrossway,");
                fprintf(sh_pDbMgrRxMsg, "eLvLaneManeuver,");
                fprintf(sh_pDbMgrRxMsg, "anLvLatitude,");
                fprintf(sh_pDbMgrRxMsg, "anLvLongitude,");
                fprintf(sh_pDbMgrRxMsg, "unReserved1");
            }
            else
            {
                PrintError("unknown eDbV2XPtType[%d]", pstDbManager->stDbV2xPt.eDbV2XPtType);
            }
        }
        else if(pstDbManager->eSvcType == DB_MANAGER_SVC_TYPE_BASE)
        {
            fprintf(sh_pDbMgrRxMsg, "cPayload,");
            fprintf(sh_pDbMgrRxMsg, "unTotalPacketCrc32");
        }
        else
        {
            PrintError("unknown eSvcType[nRet:%d]", pstDbManager->eSvcType);
            return nRet;
        }

        fprintf(sh_pDbMgrRxMsg, "\r\n");
    }

    nRet = fflush(sh_pDbMgrRxMsg);
    if (nRet < 0)
    {
        PrintError("fflush() is failed! [unRet:%d]", nRet);
    }

    return nRet;
}

static int32_t P_DB_MANAGER_WriteCsvPlatooningThroughput(DB_MANAGER_EVENT_MSG_T *pstEventMsg)
{
    int32_t nRet = FRAMEWORK_ERROR;
    char *pchPayload = NULL;

    if(pstEventMsg == NULL)
    {
        PrintError("pstEventMsg is NULL!");
        return nRet;
    }

    switch(pstEventMsg->pstDbManagerWrite->eCommMsgType)
    {
        case DB_MANAGER_COMM_MSG_TYPE_TX:
        {
            if (sh_pDbMgrTxMsg != NULL)
            {
                pchPayload = (char*)malloc(sizeof(char)*pstEventMsg->pstDbV2x->ulPayloadLength);
                if(pchPayload == NULL)
                {
                    PrintError("malloc() is failed! [NULL]");
                    return nRet;
                }

                memcpy(pchPayload, (char *)pstEventMsg->pPayload, pstEventMsg->pstDbV2x->ulPayloadLength);

                fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eDeviceType);
                fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eTeleCommType);
                fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->unDeviceId);
                fprintf(sh_pDbMgrTxMsg, "%ld,", pstEventMsg->pstDbV2x->ulTimeStamp);
                fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eServiceId);
                fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eActionType);
                fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eRegionId);
                fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->ePayloadType);
                fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eCommId);
                fprintf(sh_pDbMgrTxMsg, "%d.%d,", pstEventMsg->pstDbV2x->usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, pstEventMsg->pstDbV2x->usDbVer & CLI_DB_V2X_MINOR_MASK);
                fprintf(sh_pDbMgrTxMsg, "0x%x,", pstEventMsg->pstDbV2x->usHwVer);
                fprintf(sh_pDbMgrTxMsg, "0x%x,", pstEventMsg->pstDbV2x->usSwVer);
                fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->ulPayloadLength);

                for(int i = 0; i < (int)pstEventMsg->pstDbV2x->ulPayloadLength; i++)
                {
                      fprintf(sh_pDbMgrTxMsg, "%d ", pchPayload[i]);
                }
                fprintf(sh_pDbMgrTxMsg, ",");

                fprintf(sh_pDbMgrTxMsg, "0x%x", pstEventMsg->pstDbManagerWrite->unCrc32);
                fprintf(sh_pDbMgrTxMsg, "\r\n");

                nRet = fflush(sh_pDbMgrTxMsg);
                if (nRet < 0)
                {
                    PrintError("fflush() is failed! [unRet:%d]", nRet);
                }

                if(pchPayload != NULL)
                {
                    free(pchPayload);
                }

                if(pstEventMsg->pPayload != NULL)
                {
                    if(s_bDbMgrLog == ON)
                    {
                        PrintDebug("free [%p] allocated at P_MSG_MANAGER_SendRxMsgToDbMgr()", pstEventMsg->pPayload);
                    }
                    free(pstEventMsg->pPayload);
                }
            }
            else
            {
                PrintError("sh_pDbMgrTxMsg is NULL!!, check whethter sh_pDbMgrTxMsg is opened before.");
            }

            break;
        }
        case DB_MANAGER_COMM_MSG_TYPE_RX:
        {
            if (sh_pDbMgrRxMsg != NULL)
            {
                pchPayload = (char*)malloc(sizeof(char)*pstEventMsg->pstDbV2x->ulPayloadLength);
                if(pchPayload == NULL)
                {
                    PrintError("malloc() is failed! [NULL]");
                    return nRet;
                }

                memcpy(pchPayload, (char *)pstEventMsg->pPayload, pstEventMsg->pstDbV2x->ulPayloadLength);

                fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eDeviceType);
                fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eTeleCommType);
                fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->unDeviceId);
                fprintf(sh_pDbMgrRxMsg, "%ld,", pstEventMsg->pstDbV2x->ulTimeStamp);
                fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eServiceId);
                fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eActionType);
                fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eRegionId);
                fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->ePayloadType);
                fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eCommId);
                fprintf(sh_pDbMgrRxMsg, "%d.%d,", pstEventMsg->pstDbV2x->usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, pstEventMsg->pstDbV2x->usDbVer & CLI_DB_V2X_MINOR_MASK);
                fprintf(sh_pDbMgrRxMsg, "0x%x,", pstEventMsg->pstDbV2x->usHwVer);
                fprintf(sh_pDbMgrRxMsg, "0x%x,", pstEventMsg->pstDbV2x->usSwVer);
                fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->ulPayloadLength);

                for(int i = 0; i < (int)pstEventMsg->pstDbV2x->ulPayloadLength; i++)
                {
                      fprintf(sh_pDbMgrRxMsg, "%d ", pchPayload[i]);
                }
                fprintf(sh_pDbMgrRxMsg, ",");

                fprintf(sh_pDbMgrRxMsg, "0x%x", pstEventMsg->pstDbManagerWrite->unCrc32);
                fprintf(sh_pDbMgrRxMsg, "\r\n");

                nRet = fflush(sh_pDbMgrRxMsg);
                if (nRet < 0)
                {
                    PrintError("fflush() is failed! [unRet:%d]", nRet);
                }

                if(pchPayload != NULL)
                {
                    free(pchPayload);
                }

                if(pstEventMsg->pPayload != NULL)
                {
                    if(s_bDbMgrLog == ON)
                    {
                        PrintDebug("free [%p] allocated at P_MSG_MANAGER_SendRxMsgToDbMgr()", pstEventMsg->pPayload);
                    }
                    free(pstEventMsg->pPayload);
                }
            }
            else
            {
                PrintError("sh_pDbMgrRxMsg is NULL!!, check whethter sh_pDbMgrRxMsg is opened before.");
            }

            break;
        }
        default:
            PrintError("unknown eCommMsgType [%d]", pstEventMsg->pstDbManagerWrite->eCommMsgType);
            break;
    }

    return nRet;
}

static int32_t P_DB_MANAGER_WriteCsvPlatooningTx(DB_MANAGER_EVENT_MSG_T *pstEventMsg)
{
    int32_t nRet = FRAMEWORK_ERROR;
    char *pchPayload = NULL;
    DB_V2X_STATUS_TX_T stDbV2xStatusTx;
    DB_V2X_PLATOONING_T stDbV2XPt;
    DB_V2X_PLATOONING_LV_T stDbV2XPtLv;
    DB_V2X_PLATOONING_FV_T stDbV2XPtFv;
    double dTemp;

    if(pstEventMsg == NULL)
    {
        PrintError("pstEventMsg is NULL!");
        return nRet;
    }

    memset(&stDbV2xStatusTx, 0, sizeof(DB_V2X_STATUS_TX_T));
    memset(&stDbV2XPt, 0, sizeof(DB_V2X_PLATOONING_T));
    memset(&stDbV2XPtLv, 0, sizeof(DB_V2X_PLATOONING_LV_T));
    memset(&stDbV2XPtFv, 0, sizeof(DB_V2X_PLATOONING_FV_T));

    pchPayload = (char*)malloc(sizeof(char)*pstEventMsg->pstDbV2x->ulPayloadLength);
    if(pchPayload == NULL)
    {
        PrintError("malloc() is failed! [NULL]");
        return nRet;
    }

    memcpy(pchPayload, (char *)pstEventMsg->pPayload, pstEventMsg->pstDbV2x->ulPayloadLength);

    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eDeviceType);
    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eTeleCommType);
    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->unDeviceId);
    fprintf(sh_pDbMgrTxMsg, "%ld,", pstEventMsg->pstDbV2x->ulTimeStamp);
    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eServiceId);
    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eActionType);
    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eRegionId);
    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->ePayloadType);
    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eCommId);
    fprintf(sh_pDbMgrTxMsg, "%d.%d,", pstEventMsg->pstDbV2x->usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, pstEventMsg->pstDbV2x->usDbVer & CLI_DB_V2X_MINOR_MASK);
    fprintf(sh_pDbMgrTxMsg, "0x%x,", pstEventMsg->pstDbV2x->usHwVer);
    fprintf(sh_pDbMgrTxMsg, "0x%x,", pstEventMsg->pstDbV2x->usSwVer);
    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->ulPayloadLength);

    memcpy(&stDbV2xStatusTx, pchPayload, sizeof(DB_V2X_STATUS_TX_T));
    memcpy(&stDbV2XPt, pchPayload + sizeof(DB_V2X_STATUS_TX_T), sizeof(DB_V2X_PLATOONING_T));

    fprintf(sh_pDbMgrTxMsg, "%ld,", stDbV2xStatusTx.stDbV2xDevL1.ulTimeStamp);
    fprintf(sh_pDbMgrTxMsg, "%ld,", stDbV2xStatusTx.stDbV2xDevL2.ulTimeStamp);
    fprintf(sh_pDbMgrTxMsg, "%ld,", stDbV2xStatusTx.stDbV2xDevL3.ulTimeStamp);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.unRxTargetDeviceId);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.usTxFreq);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.ucTxPwr);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.ucTxBw);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.ucScs);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.ucMcs);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.usTxRatio);
    dTemp = (double)stDbV2xStatusTx.stTxPosition.nTxLatitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrTxMsg, "%lf,", dTemp);
    dTemp = (double)stDbV2xStatusTx.stTxPosition.nTxLongitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrTxMsg, "%lf,", dTemp);
    dTemp = (double)stDbV2xStatusTx.stTxPosition.nTxAttitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrTxMsg, "%lf,", dTemp);

    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.unSeqNum);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.unContCnt);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.unTxVehicleSpeed);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.unTxVehicleHeading);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPt.eDbV2XPtType);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPt.usV2xGroupId);
    if (stDbV2XPt.eDbV2XPtType == eDB_V2X_PT_TYPE_LV)
    {
        memcpy(&stDbV2XPtLv, pchPayload + sizeof(DB_V2X_STATUS_TX_T) + sizeof(DB_V2X_PLATOONING_T), sizeof(DB_V2X_PLATOONING_LV_T));

        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.eLvServiceId);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.eLvMethodId);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.unLvLength);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.usLvClientId);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.usLvSessionId);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.ucLvProtocolVer);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.ucLvInterfaceVer);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.eLvMsgType);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.ucLvReturnCode);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.eLvVehicleType);
        fprintf(sh_pDbMgrTxMsg, "%s,", stDbV2XPtLv.szLvVehicleId);
        fprintf(sh_pDbMgrTxMsg, "%s,", stDbV2XPtLv.szLvVehicleNum);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.usLvMsgCount);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.eLvMsgId);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.nLvLatitude);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.nLvLongitude);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.usLvHeading);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.usLvSpeed);
        fprintf(sh_pDbMgrTxMsg, "%s,", stDbV2XPtLv.szLvDriveLaneId);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.eLvDriveStatus);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.eLvChangeCode);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.usLvPathId);
        fprintf(sh_pDbMgrTxMsg, "%s,", stDbV2XPtLv.szLvLaneId);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.eLvLanePlan);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.eLvCrossway);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtLv.eLvLaneManeuver);
        fprintf(sh_pDbMgrTxMsg, "\"");
        for (int i = 0; i < DB_V2X_PT_LV_PATH_PLAN_MAX_LEN; i++)
        {
            dTemp = (double)stDbV2XPtLv.stLvPathPlan.anLvLatitude[i] / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
            fprintf(sh_pDbMgrTxMsg, "%lf", dTemp);
            if (i < DB_V2X_PT_LV_PATH_PLAN_MAX_LEN - 1)
            {
                fprintf(sh_pDbMgrTxMsg, ",");
            }
        }
        fprintf(sh_pDbMgrTxMsg, "\",");
        fprintf(sh_pDbMgrTxMsg, "\"");
        for (int i = 0; i < DB_V2X_PT_LV_PATH_PLAN_MAX_LEN; i++)
        {
            dTemp = (double)stDbV2XPtLv.stLvPathPlan.anLvLongitude[i] / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
            fprintf(sh_pDbMgrTxMsg, "%lf", dTemp);
            if (i < DB_V2X_PT_LV_PATH_PLAN_MAX_LEN - 1)
            {
                fprintf(sh_pDbMgrTxMsg, ",");
            }
        }
        fprintf(sh_pDbMgrTxMsg, "\",");
        fprintf(sh_pDbMgrTxMsg, "%ld,", stDbV2XPtLv.unReserved1);
    }
    else if (stDbV2XPt.eDbV2XPtType == eDB_V2X_PT_TYPE_FV)
    {
        memcpy(&stDbV2XPtFv, pchPayload + sizeof(DB_V2X_STATUS_TX_T) + sizeof(DB_V2X_PLATOONING_T), sizeof(DB_V2X_PLATOONING_FV_T));

        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.eFvServiceId);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.eFvMethodId);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.unFvLength);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.usFvClientId);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.usFvSessionId);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.ucFvProtocolVer);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.ucFvInterfaceVer);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.eFvMsgType);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.ucFvReturnCode);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.eFvVehicleType);
        fprintf(sh_pDbMgrTxMsg, "%s,", stDbV2XPtFv.szFvVehicleId);
        fprintf(sh_pDbMgrTxMsg, "%s,", stDbV2XPtFv.szFvVehicleNum);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.usFvMsgCount);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.eFvMsgId);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.nFvLatitude);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.nFvLongitude);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.usFvHeading);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.usFvSpeed);
        fprintf(sh_pDbMgrTxMsg, "%s,", stDbV2XPtFv.szFvDriveLaneId);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.eFvDriveStatus);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.eFvChangeCode);
        fprintf(sh_pDbMgrTxMsg, "\"");
        for (int i = 0; i < DB_V2X_PT_FV_PATH_PLAN_MAX_LEN; i++)
        {
            dTemp = (double)stDbV2XPtFv.stFvPathPlan.anFvLatitude[i] / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
            fprintf(sh_pDbMgrTxMsg, "%lf", dTemp);
            if (i < DB_V2X_PT_FV_PATH_PLAN_MAX_LEN - 1)
            {
                fprintf(sh_pDbMgrTxMsg, ",");
            }
        }
        fprintf(sh_pDbMgrTxMsg, "\",");
        fprintf(sh_pDbMgrTxMsg, "\"");
        for (int i = 0; i < DB_V2X_PT_FV_PATH_PLAN_MAX_LEN; i++)
        {
            dTemp = (double)stDbV2XPtFv.stFvPathPlan.anFvLongitude[i] / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
            fprintf(sh_pDbMgrTxMsg, "%lf", dTemp);
            if (i < DB_V2X_PT_FV_PATH_PLAN_MAX_LEN - 1)
            {
                fprintf(sh_pDbMgrTxMsg, ",");
            }
        }
        fprintf(sh_pDbMgrTxMsg, "\",");
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.usFvRecommDistance);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.usFvRecommSpeed);
        fprintf(sh_pDbMgrTxMsg, "%ld,", stDbV2XPtFv.unReserved1);
        fprintf(sh_pDbMgrTxMsg, "%ld,", stDbV2XPtFv.unReserved2);
        fprintf(sh_pDbMgrTxMsg, "%ld,", stDbV2XPtFv.unReserved3);
        fprintf(sh_pDbMgrTxMsg, "%ld,", stDbV2XPtFv.unReserved4);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.unReserved5);
        fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2XPtFv.unReserved6);
    }
    else
    {
        PrintError("unknown eDbV2XPtType[%d]", stDbV2XPt.eDbV2XPtType);
    }
    fprintf(sh_pDbMgrTxMsg, "0x%x", pstEventMsg->pstDbManagerWrite->unCrc32);
    fprintf(sh_pDbMgrTxMsg, "\r\n");

    nRet = fflush(sh_pDbMgrTxMsg);
    if (nRet < 0)
    {
        PrintError("fflush() is failed! [unRet:%d]", nRet);
    }

    if(pchPayload != NULL)
    {
        free(pchPayload);
    }

    if(pstEventMsg->pPayload != NULL)
    {
        if(s_bDbMgrLog == ON)
        {
            PrintDebug("free [%p] allocated at P_MSG_MANAGER_SendRxMsgToDbMgr()", pstEventMsg->pPayload);
        }
        free(pstEventMsg->pPayload);
    }

    return nRet;

}

static int32_t P_DB_MANAGER_WriteCsvPlatooningRx(DB_MANAGER_EVENT_MSG_T *pstEventMsg)
{
    int32_t nRet = FRAMEWORK_ERROR;
    char *pchPayload = NULL;
    DB_V2X_STATUS_TX_T stDbV2xStatusTx;
    DB_V2X_STATUS_RX_T stDbV2xStatusRx;
    DB_MANAGER_V2X_STATUS_T stDbV2xStatus;
    DB_V2X_PLATOONING_T stDbV2XPt;
    DB_V2X_PLATOONING_LV_T stDbV2XPtLv;
    DB_V2X_PLATOONING_FV_T stDbV2XPtFv;
    DI_T *pstDi;
    float fTemp;
    double dTemp;

    if(pstEventMsg == NULL)
    {
        PrintError("pstEventMsg is NULL!");
        return nRet;
    }

    nRet = P_DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
    }

    memset(&stDbV2xStatusTx, 0, sizeof(DB_V2X_STATUS_TX_T));
    memset(&stDbV2xStatusRx, 0, sizeof(DB_V2X_STATUS_RX_T));
    memset(&stDbV2XPt, 0, sizeof(DB_V2X_PLATOONING_T));
    memset(&stDbV2XPtLv, 0, sizeof(DB_V2X_PLATOONING_LV_T));
    memset(&stDbV2XPtFv, 0, sizeof(DB_V2X_PLATOONING_FV_T));

    pchPayload = (char*)malloc(sizeof(char)*pstEventMsg->pstDbV2x->ulPayloadLength);
    if(pchPayload == NULL)
    {
        PrintError("malloc() is failed! [NULL]");
        return nRet;
    }

    memcpy(pchPayload, (char *)pstEventMsg->pPayload, pstEventMsg->pstDbV2x->ulPayloadLength);

    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eDeviceType);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eTeleCommType);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->unDeviceId);
    fprintf(sh_pDbMgrRxMsg, "%ld,", pstEventMsg->pstDbV2x->ulTimeStamp);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eServiceId);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eActionType);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eRegionId);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->ePayloadType);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eCommId);
    fprintf(sh_pDbMgrRxMsg, "%d.%d,", pstEventMsg->pstDbV2x->usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, pstEventMsg->pstDbV2x->usDbVer & CLI_DB_V2X_MINOR_MASK);
    fprintf(sh_pDbMgrRxMsg, "0x%x,", pstEventMsg->pstDbV2x->usHwVer);
    fprintf(sh_pDbMgrRxMsg, "0x%x,", pstEventMsg->pstDbV2x->usSwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->ulPayloadLength);

    memcpy(&stDbV2xStatusTx, pchPayload, sizeof(DB_V2X_STATUS_TX_T));
    memcpy(&stDbV2XPt, pchPayload + sizeof(DB_V2X_STATUS_TX_T), sizeof(DB_V2X_PLATOONING_T));

    nRet = P_DB_MANAGER_UpdateStatus(pstEventMsg, &stDbV2xStatusTx, &stDbV2xStatusRx);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_UpdateStatus() is failed! [unRet:%d]", nRet);
    }

    nRet = P_DB_MANAGER_PrintStatus(&stDbV2xStatusTx, &stDbV2xStatusRx);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_PrintStatus() is failed! [unRet:%d]", nRet);
    }

    nRet = P_DB_MANAGER_PrintStatusPt(&stDbV2XPt);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_PrintStatusPt() is failed! [unRet:%d]", nRet);
    }

    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusTx.stDbV2xDevL1.ulTimeStamp);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusTx.stDbV2xDevL2.ulTimeStamp);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusTx.stDbV2xDevL3.ulTimeStamp);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL1.unDevId);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL2.unDevId);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL3.unDevId);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL1.usSwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL2.usSwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL3.usSwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL1.usHwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL2.usHwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL3.usHwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.unRxTargetDeviceId);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.usTxFreq);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.ucTxPwr);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.ucTxBw);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.ucScs);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.ucMcs);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.usTxRatio);

    dTemp = (double)stDbV2xStatusTx.stTxPosition.nTxLatitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrRxMsg, "%lf,", dTemp);
    dTemp = (double)stDbV2xStatusTx.stTxPosition.nTxLongitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrRxMsg, "%lf,", dTemp);
    dTemp = (double)stDbV2xStatusTx.stTxPosition.nTxAttitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrRxMsg, "%lf,", dTemp);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.unSeqNum);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.unContCnt);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.unTxVehicleSpeed);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.unTxVehicleHeading);

    fprintf(sh_pDbMgrRxMsg, "0x%x,", pstEventMsg->pstDbManagerWrite->unCrc32);

    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusRx.stDbV2xDevL1.ulTimeStamp);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusRx.stDbV2xDevL2.ulTimeStamp);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusRx.stDbV2xDevL3.ulTimeStamp);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusRx.stDbV2xDevL1.ulLatency);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusRx.stDbV2xDevL2.ulLatency);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusRx.stDbV2xDevL3.ulLatency);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL1.unDevId);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL2.unDevId);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL3.unDevId);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL1.usSwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL2.usSwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL3.usSwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL1.usHwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL2.usHwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL3.usHwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.unRxVehicleSpeed);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.unRxVehicleHeading);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.unTotalCommDevCnt);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.nRssi);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.ucRcpi);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.eRsvLevel);

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
    dTemp = (double)stDbV2xStatusRx.stRxPosition.unCommDistance / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrRxMsg, "%lf,", dTemp);
    dTemp = (double)stDbV2xStatusRx.stRxPosition.nRxLatitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrRxMsg, "%lf,", dTemp);
    dTemp = (double)stDbV2xStatusRx.stRxPosition.nRxLongitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrRxMsg, "%lf,", dTemp);
    dTemp = (double)stDbV2xStatusRx.stRxPosition.nRxAttitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrRxMsg, "%lf,", dTemp);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.ucErrIndicator);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusRx.ulTotalPacketCnt);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusRx.ulTotalErrCnt);
    fTemp = (float)stDbV2xStatusRx.unPdr / 1000.0f;
    fprintf(sh_pDbMgrRxMsg, "%.3f,", fTemp);
    fTemp = (float)stDbV2xStatusRx.unPer / 1000.0f;
    fprintf(sh_pDbMgrRxMsg, "%.3f,", fTemp);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPt.eDbV2XPtType);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPt.usV2xGroupId);

    if (stDbV2XPt.eDbV2XPtType == eDB_V2X_PT_TYPE_LV)
    {
        memcpy(&stDbV2XPtLv, pchPayload + sizeof(DB_V2X_STATUS_TX_T) + sizeof(DB_V2X_PLATOONING_T), sizeof(DB_V2X_PLATOONING_LV_T));

        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.eLvServiceId);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.eLvMethodId);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.unLvLength);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.usLvClientId);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.usLvSessionId);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.ucLvProtocolVer);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.ucLvInterfaceVer);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.eLvMsgType);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.ucLvReturnCode);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.eLvVehicleType);
        fprintf(sh_pDbMgrRxMsg, "%s,", stDbV2XPtLv.szLvVehicleId);
        fprintf(sh_pDbMgrRxMsg, "%s,", stDbV2XPtLv.szLvVehicleNum);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.usLvMsgCount);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.eLvMsgId);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.nLvLatitude);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.nLvLongitude);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.usLvHeading);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.usLvSpeed);
        fprintf(sh_pDbMgrRxMsg, "%s,", stDbV2XPtLv.szLvDriveLaneId);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.eLvDriveStatus);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.eLvChangeCode);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.usLvPathId);
        fprintf(sh_pDbMgrRxMsg, "%s,", stDbV2XPtLv.szLvLaneId);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.eLvLanePlan);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.eLvCrossway);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtLv.eLvLaneManeuver);
        fprintf(sh_pDbMgrRxMsg, "\"");
        for (int i = 0; i < DB_V2X_PT_LV_PATH_PLAN_MAX_LEN; i++)
        {
            dTemp = (double)stDbV2XPtLv.stLvPathPlan.anLvLatitude[i] / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
            fprintf(sh_pDbMgrRxMsg, "%lf", dTemp);
            if (i < DB_V2X_PT_LV_PATH_PLAN_MAX_LEN - 1)
            {
                fprintf(sh_pDbMgrRxMsg, ",");
            }
        }
        fprintf(sh_pDbMgrRxMsg, "\",");
        fprintf(sh_pDbMgrRxMsg, "\"");
        for (int i = 0; i < DB_V2X_PT_LV_PATH_PLAN_MAX_LEN; i++)
        {
            dTemp = (double)stDbV2XPtLv.stLvPathPlan.anLvLongitude[i] / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
            fprintf(sh_pDbMgrRxMsg, "%lf", dTemp);
            if (i < DB_V2X_PT_LV_PATH_PLAN_MAX_LEN - 1)
            {
                fprintf(sh_pDbMgrRxMsg, ",");
            }
        }
        fprintf(sh_pDbMgrRxMsg, "\",");
        fprintf(sh_pDbMgrRxMsg, "%ld", stDbV2XPtLv.unReserved1);

        nRet = P_DB_MANAGER_PrintStatusPtLv(&stDbV2XPtLv);
        if (nRet != FRAMEWORK_OK)
        {
            PrintError("P_DB_MANAGER_PrintStatusPtLv() is failed! [unRet:%d]", nRet);
        }

    }
    else if (stDbV2XPt.eDbV2XPtType == eDB_V2X_PT_TYPE_FV)
    {
        memcpy(&stDbV2XPtFv, pchPayload + sizeof(DB_V2X_STATUS_TX_T) + sizeof(DB_V2X_PLATOONING_T), sizeof(DB_V2X_PLATOONING_FV_T));

        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.eFvServiceId);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.eFvMethodId);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.unFvLength);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.usFvClientId);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.usFvSessionId);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.ucFvProtocolVer);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.ucFvInterfaceVer);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.eFvMsgType);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.ucFvReturnCode);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.eFvVehicleType);
        fprintf(sh_pDbMgrRxMsg, "%s,", stDbV2XPtFv.szFvVehicleId);
        fprintf(sh_pDbMgrRxMsg, "%s,", stDbV2XPtFv.szFvVehicleNum);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.usFvMsgCount);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.eFvMsgId);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.nFvLatitude);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.nFvLongitude);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.usFvHeading);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.usFvSpeed);
        fprintf(sh_pDbMgrRxMsg, "%s,", stDbV2XPtFv.szFvDriveLaneId);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.eFvDriveStatus);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.eFvChangeCode);
        fprintf(sh_pDbMgrRxMsg, "\"");
        for (int i = 0; i < DB_V2X_PT_FV_PATH_PLAN_MAX_LEN; i++)
        {
            dTemp = (double)stDbV2XPtFv.stFvPathPlan.anFvLatitude[i] / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
            fprintf(sh_pDbMgrRxMsg, "%lf", dTemp);
            if (i < DB_V2X_PT_FV_PATH_PLAN_MAX_LEN - 1)
            {
                fprintf(sh_pDbMgrRxMsg, ",");
            }
        }
        fprintf(sh_pDbMgrRxMsg, "\",");
        fprintf(sh_pDbMgrRxMsg, "\"");
        for (int i = 0; i < DB_V2X_PT_FV_PATH_PLAN_MAX_LEN; i++)
        {
            dTemp = (double)stDbV2XPtFv.stFvPathPlan.anFvLongitude[i] / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
            fprintf(sh_pDbMgrRxMsg, "%lf", dTemp);
            if (i < DB_V2X_PT_FV_PATH_PLAN_MAX_LEN - 1)
            {
                fprintf(sh_pDbMgrRxMsg, ",");
            }
        }
        fprintf(sh_pDbMgrRxMsg, "\",");
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.usFvRecommDistance);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.usFvRecommSpeed);
        fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2XPtFv.unReserved1);
        fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2XPtFv.unReserved2);
        fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2XPtFv.unReserved3);
        fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2XPtFv.unReserved4);
        fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2XPtFv.unReserved5);
        fprintf(sh_pDbMgrRxMsg, "%d", stDbV2XPtFv.unReserved6);

        nRet = P_DB_MANAGER_PrintStatusPtFv(&stDbV2XPtFv);
        if (nRet != FRAMEWORK_OK)
        {
            PrintError("P_DB_MANAGER_PrintStatusPtFv() is failed! [unRet:%d]", nRet);
        }
    }
    else
    {
        PrintError("unknown eDbV2XPtType[%d]", stDbV2XPt.eDbV2XPtType);
    }

    fprintf(sh_pDbMgrRxMsg, "\r\n");

    nRet = P_DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
    }

    /* Reset the err indicator */
    stDbV2xStatus.stV2xStatusRx.ucErrIndicator = FALSE;
    nRet = P_DB_MANAGER_SetV2xStatus(&stDbV2xStatus);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
    }

    nRet = fflush(sh_pDbMgrRxMsg);
    if (nRet < 0)
    {
        PrintError("fflush() is failed! [unRet:%d]", nRet);
    }

    if(pchPayload != NULL)
    {
        free(pchPayload);
    }

    if(pstEventMsg->pPayload != NULL)
    {
        if(s_bDbMgrLog == ON)
        {
            PrintDebug("free [%p] allocated at P_MSG_MANAGER_SendRxMsgToDbMgr()", pstEventMsg->pPayload);
        }
        free(pstEventMsg->pPayload);
    }

    return nRet;
}

static int32_t P_DB_MANAGER_WriteCsvPlatooning(DB_MANAGER_EVENT_MSG_T *pstEventMsg)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstEventMsg == NULL)
    {
        PrintError("pstEventMsg is NULL!");
        return nRet;
    }

    switch(pstEventMsg->pstDbManagerWrite->eCommMsgType)
    {
        case DB_MANAGER_COMM_MSG_TYPE_TX:
        {
            if (sh_pDbMgrTxMsg != NULL)
            {
                nRet = P_DB_MANAGER_WriteCsvPlatooningTx(pstEventMsg);
                if(nRet != FRAMEWORK_OK)
                {
                    PrintError("P_DB_MANAGER_WriteCsvPlatooningTx() is failed! [unRet:%d]", nRet);
                }
            }
            else
            {
                PrintError("sh_pDbMgrTxMsg is NULL!!, check whethter sh_pDbMgrTxMsg is opened before.");
            }
            break;
        }
        case DB_MANAGER_COMM_MSG_TYPE_RX:
        {
            if (sh_pDbMgrRxMsg != NULL)
            {
                nRet = P_DB_MANAGER_WriteCsvPlatooningRx(pstEventMsg);
                if(nRet != FRAMEWORK_OK)
                {
                    PrintError("P_DB_MANAGER_WriteCsvPlatooningRx() is failed! [unRet:%d]", nRet);
                }
            }
            else
            {
                PrintError("sh_pDbMgrRxMsg is NULL!!, check whethter sh_pDbMgrRxMsg is opened before.");
            }
            break;
        }
        default:
            PrintError("unknown eCommMsgType [%d]", pstEventMsg->pstDbManagerWrite->eCommMsgType);
            break;
    }
    nRet = FRAMEWORK_OK;

    return nRet;
}

static int32_t P_DB_MANAGER_WriteCsvV2xStatusTx(DB_MANAGER_EVENT_MSG_T *pstEventMsg)
{
    int32_t nRet = FRAMEWORK_ERROR;
    char *pchPayload = NULL;
    DB_V2X_STATUS_TX_T stDbV2xStatusTx;
    double dTemp;

    if(pstEventMsg == NULL)
    {
        PrintError("pstEventMsg is NULL!");
        return nRet;
    }

    pchPayload = (char*)malloc(sizeof(char)*pstEventMsg->pstDbV2x->ulPayloadLength);
    if(pchPayload == NULL)
    {
        PrintError("malloc() is failed! [NULL]");
        return nRet;
    }

    memcpy(pchPayload, (char *)pstEventMsg->pPayload, pstEventMsg->pstDbV2x->ulPayloadLength);

    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eDeviceType);
    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eTeleCommType);
    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->unDeviceId);
    fprintf(sh_pDbMgrTxMsg, "%ld,", pstEventMsg->pstDbV2x->ulTimeStamp);
    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eServiceId);
    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eActionType);
    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eRegionId);
    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->ePayloadType);
    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->eCommId);
    fprintf(sh_pDbMgrTxMsg, "%d.%d,", pstEventMsg->pstDbV2x->usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, pstEventMsg->pstDbV2x->usDbVer & CLI_DB_V2X_MINOR_MASK);
    fprintf(sh_pDbMgrTxMsg, "0x%x,", pstEventMsg->pstDbV2x->usHwVer);
    fprintf(sh_pDbMgrTxMsg, "0x%x,", pstEventMsg->pstDbV2x->usSwVer);
    fprintf(sh_pDbMgrTxMsg, "%d,", pstEventMsg->pstDbV2x->ulPayloadLength);

    memcpy(&stDbV2xStatusTx, pchPayload, sizeof(char)*pstEventMsg->pstDbV2x->ulPayloadLength);

    fprintf(sh_pDbMgrTxMsg, "%ld,", stDbV2xStatusTx.stDbV2xDevL1.ulTimeStamp);
    fprintf(sh_pDbMgrTxMsg, "%ld,", stDbV2xStatusTx.stDbV2xDevL2.ulTimeStamp);
    fprintf(sh_pDbMgrTxMsg, "%ld,", stDbV2xStatusTx.stDbV2xDevL3.ulTimeStamp);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.unRxTargetDeviceId);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.usTxFreq);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.ucTxPwr);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.ucTxBw);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.ucScs);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.ucMcs);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.usTxRatio);
    dTemp = (double)stDbV2xStatusTx.stTxPosition.nTxLatitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrTxMsg, "%lf,", dTemp);
    dTemp = (double)stDbV2xStatusTx.stTxPosition.nTxLongitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrTxMsg, "%lf,", dTemp);
    dTemp = (double)stDbV2xStatusTx.stTxPosition.nTxAttitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrTxMsg, "%lf,", dTemp);

    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.unSeqNum);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.unContCnt);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.unTxVehicleSpeed);
    fprintf(sh_pDbMgrTxMsg, "%d,", stDbV2xStatusTx.unTxVehicleHeading);
    fprintf(sh_pDbMgrTxMsg, "0x%x", pstEventMsg->pstDbManagerWrite->unCrc32);
    fprintf(sh_pDbMgrTxMsg, "\r\n");

    nRet = fflush(sh_pDbMgrTxMsg);
    if (nRet < 0)
    {
        PrintError("fflush() is failed! [unRet:%d]", nRet);
    }

    if(pchPayload != NULL)
    {
        free(pchPayload);
    }

    if(pstEventMsg->pPayload != NULL)
    {
        if(s_bDbMgrLog == ON)
        {
            PrintDebug("free [%p] allocated at P_MSG_MANAGER_SendRxMsgToDbMgr()", pstEventMsg->pPayload);
        }
        free(pstEventMsg->pPayload);
    }

    return nRet;
}

#if defined(DB_MGR_TEST)
static int32_t P_DB_MANAGER_TestPacket(DB_MANAGER_V2X_STATUS_T *pstDbV2xStatus)
{
    int32_t nRet = FRAMEWORK_ERROR;

    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "0x%x,", 0);
    fprintf(sh_pDbMgrRxMsg, "%ld,", 0UL);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d.%d,", 0, 0);
    fprintf(sh_pDbMgrRxMsg, "0x%x,", 0);
    fprintf(sh_pDbMgrRxMsg, "0x%x,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%ld,", 0UL);
    fprintf(sh_pDbMgrRxMsg, "%ld,", 0UL);
    fprintf(sh_pDbMgrRxMsg, "%ld,", 0UL);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%lf,", 0.0f);
    fprintf(sh_pDbMgrRxMsg, "%lf,", 0.0f);
    fprintf(sh_pDbMgrRxMsg, "%lf,", 0.0f);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "0x%x,", 0);
    fprintf(sh_pDbMgrRxMsg, "%ld,", 0UL);
    fprintf(sh_pDbMgrRxMsg, "%ld,", 0UL);
    fprintf(sh_pDbMgrRxMsg, "%ld,", 0UL);
    fprintf(sh_pDbMgrRxMsg, "%ld,", 0UL);
    fprintf(sh_pDbMgrRxMsg, "%ld,", 0UL);
    fprintf(sh_pDbMgrRxMsg, "%ld,", 0UL);
    fprintf(sh_pDbMgrRxMsg, "0x%x,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%d,", 0);
    fprintf(sh_pDbMgrRxMsg, "%lf,", 0.0f);
    fprintf(sh_pDbMgrRxMsg, "%lf,", 0.0f);
    fprintf(sh_pDbMgrRxMsg, "%lf,", 0.0f);
    fprintf(sh_pDbMgrRxMsg, "%lf,", 0.0f);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstDbV2xStatus->stV2xStatusRx.ucErrIndicator);
    fprintf(sh_pDbMgrRxMsg, "%ld,", 0UL);
    fprintf(sh_pDbMgrRxMsg, "%ld,", pstDbV2xStatus->stV2xStatusRx.ulTotalErrCnt);
    fprintf(sh_pDbMgrRxMsg, "%.2f,", 0.0f);
    fprintf(sh_pDbMgrRxMsg, "%.2f", 0.0f);
    fprintf(sh_pDbMgrRxMsg, "\r\n");

    nRet = fflush(sh_pDbMgrRxMsg);
    if (nRet < 0)
    {
        PrintError("fflush() is failed! [unRet:%d]", nRet);
    }

    return nRet;
}
#endif

static int32_t P_DB_MANAGER_WriteCsvV2xStatusRx(DB_MANAGER_EVENT_MSG_T *pstEventMsg)
{
    int32_t nRet = FRAMEWORK_ERROR;
    char *pchPayload = NULL;
    DB_V2X_STATUS_TX_T stDbV2xStatusTx;
    DB_V2X_STATUS_RX_T stDbV2xStatusRx;
    DB_MANAGER_V2X_STATUS_T stDbV2xStatus;
    DI_T *pstDi;
    float fTemp;
    double dTemp;

    if(pstEventMsg == NULL)
    {
        PrintError("pstEventMsg is NULL!");
        return nRet;
    }

    nRet = P_DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
    }

    memset(&stDbV2xStatusTx, 0, sizeof(DB_V2X_STATUS_TX_T));
    memset(&stDbV2xStatusRx, 0, sizeof(DB_V2X_STATUS_RX_T));

    pchPayload = (char*)malloc(sizeof(char)*pstEventMsg->pstDbV2x->ulPayloadLength);
    if(pchPayload == NULL)
    {
        PrintError("malloc() is failed! [NULL]");
        return nRet;
    }

    memcpy(pchPayload, (char *)pstEventMsg->pPayload, pstEventMsg->pstDbV2x->ulPayloadLength);

    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eDeviceType);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eTeleCommType);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->unDeviceId);
    fprintf(sh_pDbMgrRxMsg, "%ld,", pstEventMsg->pstDbV2x->ulTimeStamp);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eServiceId);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eActionType);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eRegionId);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->ePayloadType);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->eCommId);
    fprintf(sh_pDbMgrRxMsg, "%d.%d,", pstEventMsg->pstDbV2x->usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, pstEventMsg->pstDbV2x->usDbVer & CLI_DB_V2X_MINOR_MASK);
    fprintf(sh_pDbMgrRxMsg, "0x%x,", pstEventMsg->pstDbV2x->usHwVer);
    fprintf(sh_pDbMgrRxMsg, "0x%x,", pstEventMsg->pstDbV2x->usSwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", pstEventMsg->pstDbV2x->ulPayloadLength);

    memcpy(&stDbV2xStatusTx, pchPayload, sizeof(char)*pstEventMsg->pstDbV2x->ulPayloadLength);

    nRet = P_DB_MANAGER_UpdateStatus(pstEventMsg, &stDbV2xStatusTx, &stDbV2xStatusRx);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_UpdateStatus() is failed! [unRet:%d]", nRet);
    }

    nRet = P_DB_MANAGER_PrintStatus(&stDbV2xStatusTx, &stDbV2xStatusRx);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_PrintStatus() is failed! [unRet:%d]", nRet);
    }

    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusTx.stDbV2xDevL1.ulTimeStamp);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusTx.stDbV2xDevL2.ulTimeStamp);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusTx.stDbV2xDevL3.ulTimeStamp);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL1.unDevId);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL2.unDevId);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL3.unDevId);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL1.usSwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL2.usSwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL3.usSwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL1.usHwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL2.usHwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.stDbV2xDevL3.usHwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.unRxTargetDeviceId);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.usTxFreq);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.ucTxPwr);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.ucTxBw);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.ucScs);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.ucMcs);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.usTxRatio);

    dTemp = (double)stDbV2xStatusTx.stTxPosition.nTxLatitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrRxMsg, "%lf,", dTemp);
    dTemp = (double)stDbV2xStatusTx.stTxPosition.nTxLongitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrRxMsg, "%lf,", dTemp);
    dTemp = (double)stDbV2xStatusTx.stTxPosition.nTxAttitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrRxMsg, "%lf,", dTemp);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.unSeqNum);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.unContCnt);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.unTxVehicleSpeed);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusTx.unTxVehicleHeading);

    fprintf(sh_pDbMgrRxMsg, "0x%x,", pstEventMsg->pstDbManagerWrite->unCrc32);

    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusRx.stDbV2xDevL1.ulTimeStamp);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusRx.stDbV2xDevL2.ulTimeStamp);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusRx.stDbV2xDevL3.ulTimeStamp);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusRx.stDbV2xDevL1.ulLatency);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusRx.stDbV2xDevL2.ulLatency);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusRx.stDbV2xDevL3.ulLatency);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL1.unDevId);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL2.unDevId);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL3.unDevId);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL1.usSwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL2.usSwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL3.usSwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL1.usHwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL2.usHwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.stDbV2xDevL3.usHwVer);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.unRxVehicleSpeed);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.unRxVehicleHeading);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.unTotalCommDevCnt);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.nRssi);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.ucRcpi);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.eRsvLevel);

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
    dTemp = (double)stDbV2xStatusRx.stRxPosition.unCommDistance / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrRxMsg, "%lf,", dTemp);
    dTemp = (double)stDbV2xStatusRx.stRxPosition.nRxLatitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrRxMsg, "%lf,", dTemp);
    dTemp = (double)stDbV2xStatusRx.stRxPosition.nRxLongitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrRxMsg, "%lf,", dTemp);
    dTemp = (double)stDbV2xStatusRx.stRxPosition.nRxAttitude / SVC_CP_GPS_VALUE_CONVERT_DOUBLE;
    fprintf(sh_pDbMgrRxMsg, "%lf,", dTemp);
    fprintf(sh_pDbMgrRxMsg, "%d,", stDbV2xStatusRx.ucErrIndicator);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusRx.ulTotalPacketCnt);
    fprintf(sh_pDbMgrRxMsg, "%ld,", stDbV2xStatusRx.ulTotalErrCnt);
    fTemp = (float)stDbV2xStatusRx.unPdr / 1000.0f;
    fprintf(sh_pDbMgrRxMsg, "%.3f,", fTemp);
    fTemp = (float)stDbV2xStatusRx.unPer / 1000.0f;
    fprintf(sh_pDbMgrRxMsg, "%.3f", fTemp);

    fprintf(sh_pDbMgrRxMsg, "\r\n");

    nRet = P_DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
    }

    /* Reset the err indicator */
    stDbV2xStatus.stV2xStatusRx.ucErrIndicator = FALSE;
    nRet = P_DB_MANAGER_SetV2xStatus(&stDbV2xStatus);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
    }

    nRet = fflush(sh_pDbMgrRxMsg);
    if (nRet < 0)
    {
        PrintError("fflush() is failed! [unRet:%d]", nRet);
    }

    if(pchPayload != NULL)
    {
        free(pchPayload);
    }

    if(pstEventMsg->pPayload != NULL)
    {
        if(s_bDbMgrLog == ON)
        {
            PrintDebug("free [%p] allocated at P_MSG_MANAGER_SendRxMsgToDbMgr()", pstEventMsg->pPayload);
        }
        free(pstEventMsg->pPayload);
    }

    return nRet;
}

static int32_t P_DB_MANAGER_WriteCsvV2xStatus(DB_MANAGER_EVENT_MSG_T *pstEventMsg)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstEventMsg == NULL)
    {
        PrintError("pstEventMsg is NULL!");
        return nRet;
    }

    switch(pstEventMsg->pstDbManagerWrite->eCommMsgType)
    {
        case DB_MANAGER_COMM_MSG_TYPE_TX:
        {
            if (sh_pDbMgrTxMsg != NULL)
            {
                nRet = P_DB_MANAGER_WriteCsvV2xStatusTx(pstEventMsg);
                if(nRet != FRAMEWORK_OK)
                {
                    PrintError("P_DB_MANAGER_WriteCsvV2xStatusTx() is failed! [unRet:%d]", nRet);
                }
            }
            else
            {
                PrintError("sh_pDbMgrTxMsg is NULL!!, check whethter sh_pDbMgrTxMsg is opened before.");
            }
            break;
        }
        case DB_MANAGER_COMM_MSG_TYPE_RX:
        {
            if (sh_pDbMgrRxMsg != NULL)
            {
                nRet = P_DB_MANAGER_WriteCsvV2xStatusRx(pstEventMsg);
                if(nRet != FRAMEWORK_OK)
                {
                    PrintError("P_DB_MANAGER_WriteCsvV2xStatusRx() is failed! [unRet:%d]", nRet);
                }
            }
            else
            {
                PrintError("sh_pDbMgrRxMsg is NULL!!, check whethter sh_pDbMgrRxMsg is opened before.");
            }
            break;
        }
        default:
            PrintError("unknown eCommMsgType [%d]", pstEventMsg->pstDbManagerWrite->eCommMsgType);
            break;
    }
    nRet = FRAMEWORK_OK;

    return nRet;
}

static int32_t P_DB_MANAGER_WriteCsv(DB_MANAGER_EVENT_MSG_T *pstEventMsg)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstEventMsg == NULL)
    {
        PrintError("pstEventMsg is NULL!");
        return nRet;
    }

    switch(pstEventMsg->pstDbV2x->ePayloadType)
    {
        case DB_V2X_PAYLOAD_TYPE_V2X_STATUS:
        {
            nRet = P_DB_MANAGER_WriteCsvV2xStatus(pstEventMsg);
            if(nRet != FRAMEWORK_OK)
            {
                PrintError("P_DB_MANAGER_WriteCsvV2xStatus() is failed! [unRet:%d]", nRet);
            }
            break;
        }
        case DB_V2X_PAYLOAD_TYPE_PLATOONING:
        {
            nRet = P_DB_MANAGER_WriteCsvPlatooning(pstEventMsg);
            if(nRet != FRAMEWORK_OK)
            {
                PrintError("P_DB_MANAGER_WriteCsvPlatooning() is failed! [unRet:%d]", nRet);
            }
            break;
        }
        case DB_V2X_PAYLOAD_TYPE_PLATOONING_THROUGHPUT:
        {
            nRet = P_DB_MANAGER_WriteCsvPlatooningThroughput(pstEventMsg);
            if(nRet != FRAMEWORK_OK)
            {
                PrintError("P_DB_MANAGER_WriteCsvPlatooningThroughput() is failed! [unRet:%d]", nRet);
            }
            break;
        }
        default:
            PrintError("unknown ePayloadType [%d]", pstEventMsg->pstDbV2x->ePayloadType);
            break;
    }

    return nRet;
}

static void *P_DB_MANAGER_Task(void *arg)
{
    DB_MANAGER_EVENT_MSG_T stEventMsg;
    int32_t nRet = FRAMEWORK_ERROR;
    memset(&stEventMsg, 0, sizeof(DB_MANAGER_EVENT_MSG_T));

    (void)arg;

    while (1)
    {
        if(msgrcv(s_nDbTaskMsgId, &stEventMsg, sizeof(DB_MANAGER_EVENT_MSG_T), 0, MSG_NOERROR) == FRAMEWORK_MSG_ERR)
        {
            PrintError("msgrcv() is failed!");
        }
        else
        {
            if(stEventMsg.pstDbManagerWrite->eProc == DB_MANAGER_PROC_WRITE)
            {
                if (s_bDbMgrLog == ON)
                {
                    PrintDebug("DB_MANAGER_PROC_WRITE [%d]", stEventMsg.pstDbManagerWrite->eProc);
                }

                switch(stEventMsg.pstDbManagerWrite->eFileType)
                {
                    case DB_MANAGER_FILE_TYPE_TXT:
                    {
                        if (s_bDbMgrLog == ON)
                        {
                            PrintDebug("DB_MANAGER_FILE_TYPE_TXT [%d]", stEventMsg.pstDbManagerWrite->eFileType);
                        }

                        nRet = P_DB_MANAGER_WriteTxt(&stEventMsg);
                        if(nRet != FRAMEWORK_OK)
                        {
                            PrintError("P_DB_MANAGER_WriteTxt() is failed! [unRet:%d]", nRet);
                        }
                        break;
                    }
                    case DB_MANAGER_FILE_TYPE_CSV:
                    {
                        if (s_bDbMgrLog == ON)
                        {
                            PrintDebug("DB_MANAGER_FILE_TYPE_CSV [%d]", stEventMsg.pstDbManagerWrite->eFileType);
                        }

                        nRet = P_DB_MANAGER_WriteCsv(&stEventMsg);
                        if(nRet != FRAMEWORK_OK)
                        {
                            PrintError("P_DB_MANAGER_WriteCsv() is failed! [unRet:%d]", nRet);
                        }
                        break;
                    }
#if defined(CONFIG_SQLITE)
                    case DB_MANAGER_FILE_TYPE_SQLITE:
                    {
                        if (s_bDbMgrLog == ON)
                        {
                            PrintDebug("DB_MANAGER_FILE_TYPE_SQLITE [%d]", stEventMsg.pstDbManagerWrite->eFileType);
                        }

                        nRet = P_DB_MANAGER_WriteSqlite(&stEventMsg);
                        if(nRet != FRAMEWORK_OK)
                        {
                            PrintError("P_DB_MANAGER_WriteSqlite() is failed! [unRet:%d]", nRet);
                        }
                        break;
                    }
#endif
                    default:
                        PrintWarn("unknown file type [%d]", stEventMsg.pstDbManagerWrite->eFileType);
                        break;
                }
            }
            else if(stEventMsg.pstDbManagerWrite->eProc == DB_MANAGER_PROC_READ)
            {
                PrintDebug("DB_MANAGER_PROC_READ [%d]", stEventMsg.pstDbManagerWrite->eProc);
                PrintWarn("TODO");

            }
            else if(stEventMsg.pstDbManagerWrite->eProc == DB_MANAGER_PROC_CONVERT)
            {
                PrintDebug("DB_MANAGER_PROC_CONVERT [%d]", stEventMsg.pstDbManagerWrite->eProc);
                PrintWarn("TODO");
            }
            else
            {
                PrintWarn("unknown processing type [%d]", stEventMsg.pstDbManagerWrite->eProc);
            }
        }
    }

    return NULL;
}

static void P_DB_MANAGER_PrintMsgInfo(int msqid)
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

int32_t P_DB_MANAGER_CreateTask(void)
{
	int32_t nRet = FRAMEWORK_ERROR;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    nRet = pthread_create(&sh_DbMgrTask, &attr, P_DB_MANAGER_Task, NULL);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("pthread_join() is failed!! (P_DB_MANAGER_Task) [nRet:%d]", nRet);
    }
    else
    {
        PrintTrace("P_DB_MANAGER_Task() is successfully created.");
        nRet = FRAMEWORK_OK;
    }

#if defined(CONFIG_PTHREAD_JOINABLE)
    nRet = pthread_join(sh_DbMgrTask, NULL);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("pthread_join() is failed!! (P_DB_MANAGER_Task) [nRet:%d]", nRet);
    }
    else
    {
        PrintDebug("P_DB_MANAGER_Task() is successfully joined.");
        nRet = FRAMEWORK_OK;
    }
#endif
	return nRet;
}

static int32_t P_DB_MANAGER_Init(DB_MANAGER_T *pstDbManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstDbManager == NULL)
    {
        PrintError("pstDbManager == NULL!!");
        return nRet;
    }

    if((s_nDbTaskMsgId = msgget(s_dbTaskMsgKey, IPC_CREAT|0666)) == FRAMEWORK_MSG_ERR)
    {
        PrintError("msgget() is failed!");
        return nRet;
    }
    else
    {
        P_DB_MANAGER_PrintMsgInfo(s_nDbTaskMsgId);
    }

    if((s_nMsgTxTaskMsgId = msgget(s_MsgTxTaskMsgKey, IPC_CREAT|0666)) == FRAMEWORK_MSG_ERR)
    {
        PrintError("msgget() is failed!");
        return nRet;
    }
    else
    {
        P_DB_MANAGER_PrintMsgInfo(s_nMsgTxTaskMsgId);
    }

    if((s_nMsgRxTaskMsgId = msgget(s_MsgRxTaskMsgKey, IPC_CREAT|0666)) == FRAMEWORK_MSG_ERR)
    {
        PrintError("msgget() is failed!");
        return nRet;
    }
    else
    {
        P_DB_MANAGER_PrintMsgInfo(s_nMsgRxTaskMsgId);
    }

    nRet = P_DB_MANAGER_CreateTask();
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_CreateTask() is failed! [nRet:%d]", nRet);
    }

    memset(&s_stDbV2xStatusRx, 0, sizeof(DB_MANAGER_V2X_STATUS_T));
    memset(&s_stDbV2xStatusRx.stV2xStatusRx, 0, sizeof(DB_V2X_STATUS_RX_T));
    memset(&s_stDbV2xStatusRx.stV2xStatusRx.stRxPosition, 0, sizeof(DB_V2X_POSITION_RX_T));

    return nRet;
}

static int32_t P_DB_MANAGER_DeInit(DB_MANAGER_T *pstDbManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstDbManager == NULL)
    {
        PrintError("pstDbManager == NULL!!");
        return nRet;
    }

    memset(&s_stDbV2xStatusRx, 0, sizeof(DB_MANAGER_V2X_STATUS_T));
    memset(&s_stDbV2xStatusRx.stV2xStatusRx, 0, sizeof(DB_V2X_STATUS_RX_T));
    memset(&s_stDbV2xStatusRx.stV2xStatusRx.stRxPosition, 0, sizeof(DB_V2X_POSITION_RX_T));

    return nRet;
}

static int32_t P_DB_MANAGER_ResetV2xStatus(DB_MANAGER_V2X_STATUS_T *pstDbV2xStatus)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstDbV2xStatus == NULL)
    {
        PrintError("pstDbV2xStatus is NULL!!");
    }

    memset(&s_stDbV2xStatusRx, 0, sizeof(DB_MANAGER_V2X_STATUS_T));
    memset(&s_stDbV2xStatusRx.stV2xStatusRx, 0, sizeof(DB_V2X_STATUS_RX_T));
    memset(&s_stDbV2xStatusRx.stV2xStatusRx.stRxPosition, 0, sizeof(DB_V2X_POSITION_RX_T));

    memcpy(pstDbV2xStatus, &s_stDbV2xStatusRx, sizeof(DB_MANAGER_V2X_STATUS_T));

    nRet = FRAMEWORK_OK;

    return nRet;
}

static int32_t P_DB_MANAGER_SetV2xStatus(DB_MANAGER_V2X_STATUS_T *pstDbV2xStatus)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstDbV2xStatus == NULL)
    {
        PrintError("pstDbV2xStatus is NULL!!");
    }

    memcpy(&s_stDbV2xStatusRx, pstDbV2xStatus, sizeof(DB_MANAGER_V2X_STATUS_T));
    nRet = FRAMEWORK_OK;

    return nRet;
}

static int32_t P_DB_MANAGER_GetV2xStatus(DB_MANAGER_V2X_STATUS_T *pstDbV2xStatus)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstDbV2xStatus == NULL)
    {
        PrintError("pstDbV2xStatus is NULL!!");
    }

    memcpy(pstDbV2xStatus, &s_stDbV2xStatusRx, sizeof(DB_MANAGER_V2X_STATUS_T));
    nRet = FRAMEWORK_OK;

    return nRet;
}

int32_t DB_MANAGER_SetV2xStatus(DB_MANAGER_V2X_STATUS_T *pstDbV2xStatus)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstDbV2xStatus == NULL)
    {
        PrintError("pstDbV2xStatus == NULL!!");
        return nRet;
    }

    nRet = P_DB_MANAGER_SetV2xStatus(pstDbV2xStatus);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_SetV2xStatus() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

int32_t DB_MANAGER_GetV2xStatus(DB_MANAGER_V2X_STATUS_T *pstDbV2xStatus)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstDbV2xStatus == NULL)
    {
        PrintError("pstDbV2xStatus == NULL!!");
        return nRet;
    }

    nRet = P_DB_MANAGER_GetV2xStatus(pstDbV2xStatus);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

int32_t DB_MANAGER_ResetV2xStatus(DB_MANAGER_V2X_STATUS_T *pstDbV2xStatus)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstDbV2xStatus == NULL)
    {
        PrintError("pstDbV2xStatus == NULL!!");
        return nRet;
    }

    nRet = P_DB_MANAGER_ResetV2xStatus(pstDbV2xStatus);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_ResetV2xStatus() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}


int32_t DB_MANAGER_Write(DB_MANAGER_WRITE_T *pstDbManagerWrite, DB_V2X_T *pstDbV2x, void *pPayload)
{
    int32_t nRet = FRAMEWORK_ERROR;
    DB_MANAGER_EVENT_MSG_T stEventMsg;

    if(pstDbManagerWrite == NULL)
    {
        PrintError("pstDbManagerWrite == NULL!!");
        return nRet;
    }

    if(pstDbV2x == NULL)
    {
        PrintError("pstDbV2x == NULL!!");
        return nRet;
    }

    if(pPayload == NULL)
    {
        PrintError("pPayload == NULL!!");
        return nRet;
    }

#if defined(CONFIG_SQLITE)
    if((pstDbManagerWrite->eFileType == DB_MANAGER_FILE_TYPE_SQLITE) && (sh_pDbMgrTxSqlMsg == NULL))
    {
        PrintError("sh_pDbMgrTxSqlMsg == NULL!!, check DB_MANAGER_Open() is called.");
        return nRet;
    }
#endif

    if((pstDbManagerWrite->eFileType == DB_MANAGER_FILE_TYPE_CSV) && (sh_pDbMgrTxMsg == NULL))
    {
        PrintError("sh_pDbMgrTxMsg == NULL!!, check DB_MANAGER_Open() is called.");
        return nRet;
    }

    stEventMsg.pstDbManagerWrite = pstDbManagerWrite;
    stEventMsg.pstDbV2x = pstDbV2x;
    stEventMsg.pPayload = pPayload;

    if(msgsnd(s_nDbTaskMsgId, &stEventMsg, sizeof(DB_MANAGER_EVENT_MSG_T), IPC_NOWAIT) == FRAMEWORK_MSG_ERR)
    {
        PrintError("msgsnd() is failed!!");
        return nRet;
    }
    else
    {
        nRet = FRAMEWORK_OK;
    }

    return nRet;
}

int32_t DB_MANAGER_Read(DB_MANAGER_READ_T *pstDbManagerRead, DB_V2X_T *pstDbV2x, void* pPayload)
{
    int32_t nRet = FRAMEWORK_ERROR;

    PrintWarn("TODO");

    if(pstDbManagerRead == NULL)
    {
        PrintError("pstDbManagerRead == NULL!!");
        return nRet;
    }

    if(pstDbV2x == NULL)
    {
        PrintError("pstDbV2x == NULL!!");
        return nRet;
    }

    if(pPayload == NULL)
    {
        PrintError("pPayload == NULL!!");
        return nRet;
    }

#if defined(CONFIG_SQLITE)
    if(sh_pDbMgrTxSqlMsg == NULL)
    {
        PrintError("sh_pDbMgrTxSqlMsg == NULL!!, check DB_MANAGER_Open() is called.");
        return nRet;
    }
#endif

    if(sh_pDbMgrTxMsg == NULL)
    {
        PrintError("sh_pDbMgrTxMsg == NULL!!, check DB_MANAGER_Open() is called.");
        return nRet;
    }

    return nRet;
}

int32_t DB_MANAGER_Converter(DB_MANAGER_READ_T *pstDbManagerRead, DB_MANAGER_WRITE_T *pstDbManagerWrite, DB_V2X_T *pstDbV2x, void* pPayload)
{
    int32_t nRet = FRAMEWORK_ERROR;

    PrintWarn("TODO");

    if(pstDbManagerRead == NULL)
    {
        PrintError("pstDbManagerRead == NULL!!");
        return nRet;
    }

    if(pstDbManagerWrite == NULL)
    {
        PrintError("pstDbManagerWrite == NULL!!");
        return nRet;
    }

    if(pstDbV2x == NULL)
    {
        PrintError("pstDbV2x == NULL!!");
        return nRet;
    }

    if(pPayload == NULL)
    {
        PrintError("pPayload == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t DB_MANAGER_SetLog(DB_MANAGER_T *pstDbManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstDbManager == NULL)
    {
        PrintError("pstDbManager == NULL!!");
        return nRet;
    }

    s_bDbMgrLog = pstDbManager->bLogLevel;
    PrintTrace("SET:s_bDbMgrLog [%s]", s_bDbMgrLog == ON ? "ON" : "OFF");

    nRet = FRAMEWORK_OK;

    return nRet;
}

int32_t DB_MANAGER_Open(DB_MANAGER_T *pstDbManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstDbManager == NULL)
    {
        PrintError("pstDbManager == NULL!!");
        return nRet;
    }

    switch(pstDbManager->eFileType)
    {
        case DB_MANAGER_FILE_TYPE_TXT:
            PrintDebug("DB_MANAGER_FILE_TYPE_TXT [%d]", pstDbManager->eFileType);

            if(sh_pDbMgrTxMsg == NULL)
            {
                sh_pDbMgrTxMsg = fopen(DB_MANAGER_TXT_TX_FILE, "a+");
                if(sh_pDbMgrTxMsg == NULL)
                {
                    PrintError("fopen() is failed!!");
                }
                else
                {
                    PrintTrace("DB_MANAGER_TX_FILE[%s] is opened.", DB_MANAGER_TXT_TX_FILE);
                    nRet = FRAMEWORK_OK;
                }
            }

            if(sh_pDbMgrRxMsg == NULL)
            {
                sh_pDbMgrRxMsg = fopen(DB_MANAGER_TXT_RX_FILE, "a+");
                if(sh_pDbMgrRxMsg == NULL)
                {
                    PrintError("fopen() is failed!!");
                }
                else
                {
                    PrintTrace("DB_MANAGER_RX_FILE[%s] is opened.", DB_MANAGER_TXT_RX_FILE);
                    nRet = FRAMEWORK_OK;
                }
            }

            break;

        case DB_MANAGER_FILE_TYPE_CSV:
            PrintDebug("DB_MANAGER_FILE_TYPE_CSV [%d]", pstDbManager->eFileType);

            nRet = P_DB_MANAGER_OpenCsv(pstDbManager);
            if (nRet != FRAMEWORK_OK)
            {
                PrintError("P_DB_MANAGER_OpenCsv() is failed! [nRet:%d]", nRet);
                return nRet;
            }
            break;

#if defined(CONFIG_SQLITE)
        case DB_MANAGER_FILE_TYPE_SQLITE:
            PrintDebug("DB_MANAGER_FILE_TYPE_SQLITE [%d]", pstDbManager->eFileType);

            nRet = P_DB_MANAGER_OpenSqlite(pstDbManager);
            if (nRet != FRAMEWORK_OK)
            {
                PrintError("P_DB_MANAGER_OpenSqlite() is failed! [nRet:%d]", nRet);
                return nRet;
            }
            break;
#endif

        default:
            PrintWarn("unknown file type [%d]", pstDbManager->eFileType);
            break;

    }

    return nRet;
}

int32_t DB_MANAGER_MakeDbFile(DB_MANAGER_T *pstDbManager)
{
    int32_t nRet = FRAMEWORK_ERROR;
    char chFileName[DB_MGR_FILE_MAX_LENGTH];
    char chSysCallStr[DB_MGR_SYSTEM_CALL_MAX_LENGTH];
    int nCharCnt = 0;

    if(pstDbManager == NULL)
    {
        PrintError("pstDbManager == NULL!!");
        return nRet;
    }

    sprintf(chSysCallStr, "mkdir -p %s", DB_V2X_FOLDER_DIR);
    nRet = system(chSysCallStr);
    if(nRet < FRAMEWORK_OK)
    {
        PrintError("system() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    nCharCnt = sprintf(chFileName, "%s_", pstDbManager->stDbFile.pchTxRxType);
    nCharCnt += sprintf(chFileName+nCharCnt, "%s_", pstDbManager->stDbFile.pchDeviceType);
    nCharCnt += sprintf(chFileName+nCharCnt, "%s_", pstDbManager->stDbFile.pchDeviceId);
    nCharCnt += sprintf(chFileName+nCharCnt, "%s_", pstDbManager->stDbFile.pchStartTime);
    nCharCnt += sprintf(chFileName+nCharCnt, "%s_", pstDbManager->stDbFile.pchEndTime);
    nCharCnt += sprintf(chFileName+nCharCnt, "%s.", pstDbManager->stDbFile.pchTotalTime);

    switch(pstDbManager->eFileType)
    {
        case DB_MANAGER_FILE_TYPE_TXT:
            nCharCnt += sprintf(chFileName+nCharCnt, "%s", "txt");
            if(strcmp("Tx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                sprintf(chSysCallStr, "cp %s %s/%s", DB_MANAGER_TXT_TX_FILE, DB_V2X_FOLDER_DIR, chFileName);
            }
            else if(strcmp("Rx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                sprintf(chSysCallStr, "cp %s %s/%s", DB_MANAGER_TXT_RX_FILE, DB_V2X_FOLDER_DIR, chFileName);
            }
            else
            {
                PrintError("unknown type [%s]", pstDbManager->stDbFile.pchTxRxType);
                return nRet;
            }
            break;

        case DB_MANAGER_FILE_TYPE_CSV:
            nCharCnt += sprintf(chFileName+nCharCnt, "%s", "csv");
            if(strcmp("Tx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                sprintf(chSysCallStr, "cp %s %s/%s", DB_MANAGER_CSV_TX_FILE, DB_V2X_FOLDER_DIR, chFileName);
            }
            else if(strcmp("Rx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                sprintf(chSysCallStr, "cp %s %s/%s", DB_MANAGER_CSV_RX_FILE, DB_V2X_FOLDER_DIR, chFileName);
            }
            else
            {
                PrintError("unknown type [%s]", pstDbManager->stDbFile.pchTxRxType);
                return nRet;
            }
            break;

        case DB_MANAGER_FILE_TYPE_SQLITE:
            nCharCnt += sprintf(chFileName+nCharCnt, "%s", "db");
            if(strcmp("Tx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                sprintf(chSysCallStr, "cp %s %s/%s", DB_MANAGER_SQL_TX_FILE, DB_V2X_FOLDER_DIR, chFileName);
            }
            else if(strcmp("Rx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                sprintf(chSysCallStr, "cp %s %s/%s", DB_MANAGER_SQL_RX_FILE, DB_V2X_FOLDER_DIR, chFileName);
            }
            else
            {
                PrintError("unknown type [%s]", pstDbManager->stDbFile.pchTxRxType);
                return nRet;
            }
            break;

        default:
            PrintError("unknown file type [%d]", pstDbManager->eFileType);
            break;
    }

    PrintTrace("[nCharCnt:%d][%s/%s] is successfully made!", nCharCnt, DB_V2X_FOLDER_DIR, chFileName);
    PrintDebug("%s", chSysCallStr);

    nRet = system(chSysCallStr);
    if(nRet < FRAMEWORK_OK)
    {
        PrintError("system() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    return nRet;
}

int32_t DB_MANAGER_UploadFile(DB_MANAGER_T *pstDbManager)
{
    int32_t nRet = FRAMEWORK_ERROR;
    char chFileName[DB_MGR_FILE_MAX_LENGTH];
    char chSysCallStr[DB_MGR_SYSTEM_CALL_MAX_LENGTH];
    int nCharCnt = 0, nCharCmdCnt;

    if(pstDbManager == NULL)
    {
        PrintError("pstDbManager == NULL!!");
        return nRet;
    }

    nCharCnt = sprintf(chFileName, "%s_", pstDbManager->stDbFile.pchTxRxType);
    nCharCnt += sprintf(chFileName+nCharCnt, "%s_", pstDbManager->stDbFile.pchDeviceType);
    nCharCnt += sprintf(chFileName+nCharCnt, "%s_", pstDbManager->stDbFile.pchDeviceId);
    nCharCnt += sprintf(chFileName+nCharCnt, "%s_", pstDbManager->stDbFile.pchStartTime);
    nCharCnt += sprintf(chFileName+nCharCnt, "%s_", pstDbManager->stDbFile.pchEndTime);
    nCharCnt += sprintf(chFileName+nCharCnt, "%s.", pstDbManager->stDbFile.pchTotalTime);

    switch(pstDbManager->eFileType)
    {
        case DB_MANAGER_FILE_TYPE_TXT:
            nCharCnt += sprintf(chFileName+nCharCnt, "%s", "txt");
            if(strcmp("Tx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                nCharCmdCnt = sprintf(chSysCallStr, "scp -P %d %s/%s keti@%s:%s", DB_V2X_PORT, DB_V2X_FOLDER_DIR, chFileName, DB_V2X_IP, DB_V2X_STORAGE);
                sprintf(chSysCallStr+nCharCmdCnt, "/%s/%s", DB_V2X_YEAR, DB_V2X_MONTH);
            }
            else if(strcmp("Rx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                nCharCmdCnt = sprintf(chSysCallStr, "scp -P %d %s/%s keti@%s:%s", DB_V2X_PORT, DB_V2X_FOLDER_DIR, chFileName, DB_V2X_IP, DB_V2X_STORAGE);
                sprintf(chSysCallStr+nCharCmdCnt, "/%s/%s", DB_V2X_YEAR, DB_V2X_MONTH);
            }
            else
            {
                PrintError("unknown type [%s]", pstDbManager->stDbFile.pchTxRxType);
                return nRet;
            }
            break;

        case DB_MANAGER_FILE_TYPE_CSV:
            nCharCnt += sprintf(chFileName+nCharCnt, "%s", "csv");
            if(strcmp("Tx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                nCharCmdCnt = sprintf(chSysCallStr, "scp -P %d %s/%s keti@%s:%s", DB_V2X_PORT, DB_V2X_FOLDER_DIR, chFileName, DB_V2X_IP, DB_V2X_STORAGE);
                sprintf(chSysCallStr+nCharCmdCnt, "/%s/%s", DB_V2X_YEAR, DB_V2X_MONTH);
            }
            else if(strcmp("Rx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                nCharCmdCnt = sprintf(chSysCallStr, "scp -P %d %s/%s keti@%s:%s", DB_V2X_PORT, DB_V2X_FOLDER_DIR, chFileName, DB_V2X_IP, DB_V2X_STORAGE);
                sprintf(chSysCallStr+nCharCmdCnt, "/%s/%s", DB_V2X_YEAR, DB_V2X_MONTH);
            }
            else
            {
                PrintError("unknown type [%s]", pstDbManager->stDbFile.pchTxRxType);
                return nRet;
            }
            break;

        case DB_MANAGER_FILE_TYPE_SQLITE:
            nCharCnt += sprintf(chFileName+nCharCnt, "%s", "db");
            if(strcmp("Tx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                nCharCmdCnt = sprintf(chSysCallStr, "scp -P %d %s/%s keti@%s:%s", DB_V2X_PORT, DB_V2X_FOLDER_DIR, chFileName, DB_V2X_IP, DB_V2X_STORAGE);
                sprintf(chSysCallStr+nCharCmdCnt, "/%s/%s", DB_V2X_YEAR, DB_V2X_MONTH);
            }
            else if(strcmp("Rx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                nCharCmdCnt = sprintf(chSysCallStr, "scp -P %d %s/%s keti@%s:%s", DB_V2X_PORT, DB_V2X_FOLDER_DIR, chFileName, DB_V2X_IP, DB_V2X_STORAGE);
                sprintf(chSysCallStr+nCharCmdCnt, "/%s/%s", DB_V2X_YEAR, DB_V2X_MONTH);
            }
            else
            {
                PrintError("unknown type [%s]", pstDbManager->stDbFile.pchTxRxType);
                return nRet;
            }
            break;

        default:
            PrintError("unknown file type [%d]", pstDbManager->eFileType);
            break;
    }

    PrintTrace("[scp:%s][%s/%s] is successfully send!", DB_V2X_IP, DB_V2X_FOLDER_DIR, chFileName);
    PrintDebug("%s", chSysCallStr);

    nRet = system(chSysCallStr);
    if(nRet < FRAMEWORK_OK)
    {
        PrintError("system() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    return nRet;
}

int32_t DB_MANAGER_Close(DB_MANAGER_T *pstDbManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstDbManager == NULL)
    {
        PrintError("pstDbManager == NULL!!");
        return nRet;
    }

    switch(pstDbManager->eFileType)
    {
        case DB_MANAGER_FILE_TYPE_TXT:
            PrintDebug("DB_MANAGER_FILE_TYPE_TXT [%d]", pstDbManager->eFileType);

            if(sh_pDbMgrTxMsg != NULL)
            {
                nRet = fflush(sh_pDbMgrTxMsg);
                if (nRet < 0)
                {
                    PrintError("fflush() is failed! [unRet:%d]", nRet);
                }

                nRet = fclose(sh_pDbMgrTxMsg);
                if (nRet < 0)
                {
                    PrintError("fclose() is failed! [unRet:%d]", nRet);
                }
                else
                {
                    PrintTrace("DB_MANAGER_TX_FILE[%s] is closed.", DB_MANAGER_TXT_TX_FILE);
                    sh_pDbMgrTxMsg = NULL;
                    nRet = FRAMEWORK_OK;
                }
            }

            if(sh_pDbMgrRxMsg != NULL)
            {
                nRet = fflush(sh_pDbMgrRxMsg);
                if (nRet < 0)
                {
                    PrintError("fflush() is failed! [unRet:%d]", nRet);
                }

                nRet = fclose(sh_pDbMgrRxMsg);
                if (nRet < 0)
                {
                    PrintError("fclose() is failed! [unRet:%d]", nRet);
                }
                else
                {
                    PrintTrace("DB_MANAGER_RX_FILE[%s] is closed.", DB_MANAGER_TXT_RX_FILE);
                    sh_pDbMgrRxMsg = NULL;
                    nRet = FRAMEWORK_OK;
                }
            }
            break;

        case DB_MANAGER_FILE_TYPE_CSV:
            PrintDebug("DB_MANAGER_FILE_TYPE_CSV [%d]", pstDbManager->eFileType);
            if(sh_pDbMgrTxMsg != NULL)
            {
                nRet = fflush(sh_pDbMgrTxMsg);
                if (nRet < 0)
                {
                    PrintError("fflush() is failed! [unRet:%d]", nRet);
                }

                nRet = fclose(sh_pDbMgrTxMsg);
                if (nRet < 0)
                {
                    PrintError("fclose() is failed! [unRet:%d]", nRet);
                }
                else
                {
                    PrintTrace("DB_MANAGER_TX_FILE[%s] is closed.", DB_MANAGER_CSV_TX_FILE);
                    sh_pDbMgrTxMsg = NULL;
                    nRet = FRAMEWORK_OK;
                }
            }

            if(sh_pDbMgrRxMsg != NULL)
            {
                nRet = fflush(sh_pDbMgrRxMsg);
                if (nRet < 0)
                {
                    PrintError("fflush() is failed! [unRet:%d]", nRet);
                }

                nRet = fclose(sh_pDbMgrRxMsg);
                if (nRet < 0)
                {
                    PrintError("fclose() is failed! [unRet:%d]", nRet);
                }
                else
                {
                    PrintTrace("DB_MANAGER_RX_FILE[%s] is closed.", DB_MANAGER_CSV_RX_FILE);
                    sh_pDbMgrRxMsg = NULL;
                    nRet = FRAMEWORK_OK;
                }
            }
            break;

#if defined(CONFIG_SQLITE)
        case DB_MANAGER_FILE_TYPE_SQLITE:
            PrintDebug("DB_MANAGER_FILE_TYPE_SQLITE [%d]", pstDbManager->eFileType);
            if(sh_pDbMgrTxMsg != NULL)
            {
                nRet = fflush(sh_pDbMgrTxMsg);
                if (nRet < 0)
                {
                    PrintError("fflush() is failed! [unRet:%d]", nRet);
                }

                nRet = fclose(sh_pDbMgrTxMsg);
                if (nRet < 0)
                {
                    PrintError("fclose() is failed! [unRet:%d]", nRet);
                }
                else
                {
                    PrintTrace("DB_MANAGER_TX_FILE[%s] is closed.", DB_MANAGER_SQL_TX_FILE);
                    sh_pDbMgrTxMsg = NULL;
                    nRet = FRAMEWORK_OK;
                }
            }

            if(sh_pDbMgrRxMsg != NULL)
            {
                nRet = fflush(sh_pDbMgrRxMsg);
                if (nRet < 0)
                {
                    PrintError("fflush() is failed! [unRet:%d]", nRet);
                }

                nRet = fclose(sh_pDbMgrRxMsg);
                if (nRet < 0)
                {
                    PrintError("fclose() is failed! [unRet:%d]", nRet);
                }
                else
                {
                    PrintTrace("DB_MANAGER_RX_FILE[%s] is closed.", DB_MANAGER_SQL_RX_FILE);
                    sh_pDbMgrRxMsg = NULL;
                    nRet = FRAMEWORK_OK;
                }
            }
            break;
#endif

        default:
            PrintWarn("unknown file type [%d]", pstDbManager->eFileType);
            break;

    }

    return nRet;
}

int32_t DB_MANAGER_RemoveTempFile(DB_MANAGER_T *pstDbManager)
{
    int32_t nRet = FRAMEWORK_ERROR;
    char chSysCallStr[DB_MGR_SYSTEM_CALL_MAX_LENGTH];

    if(pstDbManager == NULL)
    {
        PrintError("pstDbManager == NULL!!");
        return nRet;
    }

    switch(pstDbManager->eFileType)
    {
        case DB_MANAGER_FILE_TYPE_TXT:
            if(strcmp("Tx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                sprintf(chSysCallStr, "rm -rf %s", DB_MANAGER_TXT_TX_FILE);
            }
            else if(strcmp("Rx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                sprintf(chSysCallStr, "rm -rf %s", DB_MANAGER_TXT_RX_FILE);
            }
            else
            {
                PrintError("unknown type [%s]", pstDbManager->stDbFile.pchTxRxType);
                return nRet;
            }
            break;

        case DB_MANAGER_FILE_TYPE_CSV:
            if(strcmp("Tx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                sprintf(chSysCallStr, "rm -rf %s", DB_MANAGER_CSV_TX_FILE);
            }
            else if(strcmp("Rx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                sprintf(chSysCallStr, "rm -rf %s", DB_MANAGER_CSV_RX_FILE);
            }
            else
            {
                PrintError("unknown type [%s]", pstDbManager->stDbFile.pchTxRxType);
                return nRet;
            }
            break;

        case DB_MANAGER_FILE_TYPE_SQLITE:
            if(strcmp("Tx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                sprintf(chSysCallStr, "rm -rf %s", DB_MANAGER_SQL_TX_FILE);
            }
            else if(strcmp("Rx", pstDbManager->stDbFile.pchTxRxType) == 0)
            {
                sprintf(chSysCallStr, "rm -rf %s", DB_MANAGER_SQL_RX_FILE);
            }
            else
            {
                PrintError("unknown type [%s]", pstDbManager->stDbFile.pchTxRxType);
                return nRet;
            }
            break;

        default:
            PrintError("unknown file type [%d]", pstDbManager->eFileType);
            break;
    }

    PrintDebug("%s", chSysCallStr);

    nRet = system(chSysCallStr);
    if(nRet < FRAMEWORK_OK)
    {
        PrintError("system() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    return nRet;
}

int32_t DB_MANAGER_Start(DB_MANAGER_T *pstDbManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    PrintWarn("TODO");

    if(pstDbManager == NULL)
    {
        PrintError("pstDbManager == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t DB_MANAGER_Stop(DB_MANAGER_T *pstDbManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    PrintWarn("TODO");

    if(pstDbManager == NULL)
    {
        PrintError("pstDbManager == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t DB_MANAGER_Status(DB_MANAGER_T *pstDbManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    PrintWarn("TODO");

    if(pstDbManager == NULL)
    {
        PrintError("pstDbManager == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t DB_MANAGER_Init(DB_MANAGER_T *pstDbManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstDbManager == NULL)
    {
        PrintError("pstDbManager == NULL!!");
        return nRet;
    }

    nRet = P_DB_MANAGER_Init(pstDbManager);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_Init() is failed! [unRet:%d]", nRet);
        return nRet;
    }
    else
    {
        PrintWarn("is successfully initialized.");
    }

    s_bDbMgrLog = pstDbManager->bLogLevel;
    PrintDebug("s_bDbMgrLog [%s]", s_bDbMgrLog == ON ? "ON" : "OFF");

    return nRet;
}

int32_t DB_MANAGER_DeInit(DB_MANAGER_T *pstDbManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstDbManager == NULL)
    {
        PrintError("pstDbManager == NULL!!");
        return nRet;
    }

    nRet = P_DB_MANAGER_DeInit(pstDbManager);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_DB_MANAGER_DeInit() is failed! [unRet:%d]", nRet);
        return nRet;
    }
    else
    {
        PrintWarn("is successfully initialized.");
    }

    return nRet;
}

