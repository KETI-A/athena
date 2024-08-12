#ifndef	_SVC_PLATOONING_H_
#define	_SVC_PLATOONING_H_

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
* @file db_manager.h
*
* @note
*
* DB Manager Header
*
******************************************************************************/


/***************************** Include ***************************************/
#include "type.h"
#include "db_v2x.h"
#include "db_v2x_status.h"
#include "db_v2x_platooning.h"

/***************************** Definition ************************************/
#define SVC_PLATOONING_TASK_MSG_KEY               (0x230531)
#define SVC_PLATOONING_DEFAULT_ETH_DEV            "eth1"
#define SVC_PLATOONING_V2V_PSID                   (58200)

#define SVC_PLATOONING_GPS_OPEN_RETRY_CNT         (10)
#define SVC_PLATOONING_GPS_OPEN_RETRY_DELAY       (1000)

#define SVC_PLATOONING_GPS_VALUE_CONVERT          (1000000)
#define SVC_PLATOONING_GPS_VALUE_CONVERT_DOUBLE   (1000000.0f)

#define SVC_PLATOONING_STR_BUF_LEN                (20)
#define SVC_PLATOONING_DATE_LEN                   (8)
#define SVC_PLATOONING_HOUR_LEN                   (2)
#define SVC_PLATOONING_MIN_LEN                    (2)
#define SVC_PLATOONING_SEC_LEN                    (2)
#define SVC_PLATOONING_DB_TX                      "Tx"
#define SVC_PLATOONING_DB_RX                      "Rx"
#define SVC_PLATOONING_DEV_OBU                    "OBU"
#define SVC_PLATOONING_DEV_RSU                    "RSU"
#define SVC_PLATOONING_DEV_UNKNOWN                "UNKNOWN"

#if defined(CONFIG_EXT_DATA_FORMAT)
#define SVC_PLATOONING_DEFAULT_IP                 "192.168.1.11"
#define SVC_PLATOONING_DEFAULT_PORT               (47347)
#endif

/***************************** Enum and Structure ****************************/

/**
* @details SVC_PLATOONING_EVENT_E
* @param SVC_PLATOONING_EVENT_START
* @param SVC_PLATOONING_EVENT_STOP

*/
typedef enum {
    SVC_PLATOONING_EVENT_UNKNOWN          = 0x0000,
    SVC_PLATOONING_EVENT_START            = 0x0001,
    SVC_PLATOONING_EVENT_STOP             = 0x0002,
    SVC_PLATOONING_EVENT_UNDEFINED_1,
    SVC_PLATOONING_EVENT_UNDEFINED_2,
    SVC_PLATOONING_EVENT_MAX              = 0xFFFF
} SVC_PLATOONING_EVENT_E;

/**
* @details SVC_PLATOONING_EVENT_MSG_T
* @param pstTimeMgrSetting
*/
typedef struct SVC_PLATOONING_EVENT_MSG_t {
    SVC_PLATOONING_EVENT_E        eEventType;
} SVC_PLATOONING_EVENT_MSG_T;

/**
* @details SVC_PLATOONING_STATUS_E
* @param SVC_PLATOONING_STATUS_START
* @param SVC_PLATOONING_STATUS_STOP
*/
typedef enum {
    SVC_PLATOONING_STATUS_IDLE            = 0x0000,
    SVC_PLATOONING_STATUS_START           = 0x0001,
    SVC_PLATOONING_STATUS_STOP            = 0x0002,
    SVC_PLATOONING_STATUS_MAX             = 0xFFFF
} SVC_PLATOONING_STATUS_E;


/**
* @details SVC_PLATOONING_T
* @param bLogLevel
* @param unReserved
*/
typedef struct SVC_PLATOONING_t {
    bool                                  bLogLevel;
    DB_V2X_PLATOONING_T                   stDbV2xPt;
    DB_V2X_PLATOONING_LV_T                stDbV2XPtLv;
    DB_V2X_PLATOONING_FV_T                stDbV2XPtFv;
    SVC_PLATOONING_STATUS_E               eSvcPlatooningStatus;
    DB_MANAGER_WRITE_T                    stDbManagerWrite;
    MSG_MANAGER_TX_T                      stMsgManagerTx;
    MSG_MANAGER_RX_T                      stMsgManagerRx;
    DB_V2X_T                              stDbV2x;
    DB_V2X_STATUS_TX_T                    stDbV2xStatusTx;
    DB_V2X_STATUS_RX_T                    stDbV2xStatusRx;
    char                                  *pchDeviceName;
    char                                  *pchIfaceName;
    uint32_t                              unPsid;
    uint64_t                              ulDbStartTime;
    uint64_t                              ulDbEndTime;
    uint32_t                              unDbTotalWrittenTime;
    uint32_t                              unReserved;
    char                                  *pchIpAddr;
    uint32_t                              unPort;
} SVC_PLATOONING_T;

/***************************** Function Protype ******************************/

int32_t SVC_PLATOONING_SetLog(SVC_PLATOONING_T *pstSvcPlatooning);

int32_t SVC_PLATOONING_Open(SVC_PLATOONING_T *pstSvcPlatooning);
int32_t SVC_PLATOONING_Close(SVC_PLATOONING_T *pstSvcPlatooning);
int32_t SVC_PLATOONING_Start(SVC_PLATOONING_T *pstSvcPlatooning);
int32_t SVC_PLATOONING_Stop(SVC_PLATOONING_T *pstSvcPlatooning);
int32_t SVC_PLATOONING_Status(SVC_PLATOONING_T *pstSvcPlatooning);

void SVC_PLATOONING_ShowSettings(SVC_PLATOONING_T *pstSvcPlatooning);

int32_t SVC_PLATOONING_SetSettings(SVC_PLATOONING_T *pstSvcPlatooning);
int32_t SVC_PLATOONING_GetSettings(SVC_PLATOONING_T *pstSvcPlatooning);

int32_t SVC_PLATOONING_Init(SVC_PLATOONING_T *pstSvcPlatooning);
int32_t SVC_PLATOONING_DeInit(SVC_PLATOONING_T *pstSvcPlatooning);

#endif	/* _SVC_PLATOONING_H_ */

