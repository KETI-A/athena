#ifndef _SVC_MCP_H_
#define _SVC_MCP_H_

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
* @file svc_mcp.h
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
#include "multi_db_manager.h"
#include "multi_msg_manager.h"

/***************************** Definition ************************************/
#define SVC_MCP_TASK_MSG_KEY                 (0x2531)
#define SVC_MCP_DEFAULT_ETH_DEV              "eth1"
#define SVC_MCP_DEFAULT_RSU_ETH_DEV          "eno8303"
#define SVC_MCP_V2V_PSID                     (58200)
#define SVC_MCP_V2I_PSID                     (58201)
#define SVC_MCP_I2V_PSID                     (58202)

#define SVC_MCP_GPS_OPEN_RETRY_CNT           (10)
#define SVC_MCP_GPS_OPEN_RETRY_DELAY         (1000)

#define SVC_MCP_GPS_VALUE_CONVERT            (1000000)
#define SVC_MCP_GPS_VALUE_CONVERT_DOUBLE     (1000000.0f)

#define SVC_MCP_DEFAULT_TOTAL_DB_WRITE_TIME  (1000*1000*30) /* 1 hours */
#define SVC_MCP_STR_BUF_LEN                  (20)
#define SVC_MCP_DATE_LEN                     (8)
#define SVC_MCP_HOUR_LEN                     (2)
#define SVC_MCP_MIN_LEN                      (2)
#define SVC_MCP_SEC_LEN                      (2)
#define SVC_MCP_DB_TX                        "Tx"
#define SVC_MCP_DB_RX                        "Rx"
#define SVC_MCP_DEV_OBU                      "OBU"
#define SVC_MCP_DEV_RSU                      "RSU"
#define SVC_MCP_DEV_UNKNOWN                  "UNKNOWN"
#define SVC_MCP_STOP_DELAY                   (1000*1000)

#define SVC_MCP_DEFAULT_IP                   "192.168.1.11"
#define SVC_MCP_DEFAULT_PORT                 (47347)
#if defined(CONFIG_OBU_MAX_DEV)
#define SVC_MCP_MAX_IP_COUNT                 CONFIG_OBU_MAX_DEV
#endif

#define SVC_MCP_DEFAULT_RSU_IP               "127.0.0.1"
#define SVC_MCP_DEFAULT_RSU_PORT             (30531)
/***************************** Enum and Structure ****************************/

/**
* @details SVC_MCP_EVENT_E
* @param SVC_MCP_EVENT_START
* @param SVC_MCP_EVENT_STOP
*/
typedef enum {
    SVC_MCP_EVENT_UNKNOWN                    = 0x0000,
    SVC_MCP_EVENT_START                      = 0x0001,
    SVC_MCP_EVENT_STOP                       = 0x0002,
    SVC_MCP_EVENT_UNDEFINED_1,
    SVC_MCP_EVENT_UNDEFINED_2,
    SVC_MCP_EVENT_MAX                        = 0xFFFF
} SVC_MCP_EVENT_E;

/**
* @details SVC_MCP_STATUS_E
* @param SVC_MCP_STATUS_START
* @param SVC_MCP_STATUS_STOP
*/
typedef enum {
    SVC_MCP_STATUS_IDLE                       = 0x0000,
    SVC_MCP_STATUS_START                      = 0x0001,
    SVC_MCP_STATUS_STOP                       = 0x0002,
    SVC_MCP_STATUS_MAX                        = 0xFFFF
} SVC_MCP_STATUS_E;

/**
* @details SVC_MCP_EVENT_MSG_T
* @param eEventType
*/
typedef struct SVC_MCP_EVENT_MSG_t {
    SVC_MCP_EVENT_E          eEventType;
} SVC_MCP_EVENT_MSG_T;

/**
* @details SVC_MCP_T
* @param bLogLevel
* @param unReserved
*/
typedef struct SVC_MCP_t {
    bool                          bLogLevel;
    SVC_MCP_STATUS_E              eSvcMCpStatus;
    MULTI_DB_MANAGER_WRITE_T      stMultiDbManagerWrite;
    MULTI_MSG_MANAGER_TX_T        stMultiMsgManagerTx;
    MULTI_MSG_MANAGER_RX_T        stMultiMsgManagerRx;
    DB_V2X_T                      stDbV2x;
    DB_V2X_STATUS_TX_T            stDbV2xStatusTx;
    DB_V2X_STATUS_RX_T            stDbV2xStatusRx;
    char                          *pchIfaceName;
    uint32_t                      unPsid;
    char                          *pchDeviceName;
    uint64_t                      ulDbStartTime;
    uint64_t                      ulDbEndTime;
    uint32_t                      unDbTotalWrittenTime;
    uint32_t                      unReserved;
#if defined(CONFIG_OBU_MAX_DEV)
    char                          *pchIpAddr[SVC_MCP_MAX_IP_COUNT];
    uint32_t                      unIpCount;
#else
    char                          *pchIpAddr;
#endif
    uint32_t                      unPort;
} SVC_MCP_T;

/***************************** Function Protype ******************************/

int32_t SVC_MCP_SetLog(SVC_MCP_T *pstSvcMCp);

int32_t SVC_MCP_Open(SVC_MCP_T *pstSvcMCp);
int32_t SVC_MCP_Close(SVC_MCP_T *pstSvcMCp);
int32_t SVC_MCP_Start(SVC_MCP_T *pstSvcMCp);
int32_t SVC_MCP_Stop(SVC_MCP_T *pstSvcMCp);
int32_t SVC_MCP_Status(SVC_MCP_T *pstSvcMCp);

void SVC_MCP_ShowSettings(SVC_MCP_T *pstSvcMCp);

int32_t SVC_MCP_SetSettings(SVC_MCP_T *pstSvcMCp);
int32_t SVC_MCP_GetSettings(SVC_MCP_T *pstSvcMCp);
int32_t SVC_MCP_UpdateSettings(SVC_MCP_T *pstSvcMCp);

int32_t SVC_MCP_Init(SVC_MCP_T *pstSvcMCp);
int32_t SVC_MCP_DeInit(SVC_MCP_T *pstSvcMCp);

#endif	/* _SVC_MCP_H_ */

