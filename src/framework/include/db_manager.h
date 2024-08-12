#ifndef	_DB_MANAGER_H_
#define	_DB_MANAGER_H_

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
#define CLI_DB_V2X_DEFAULT_DEVICE_ID              23040015
#define CLI_DB_V2X_DEFAULT_TIMESTAMP              2023032314344766828
#define CLI_DB_V2X_DEFAULT_HW_VER                 0x0001 // OBU rel. 230518
#define CLI_DB_V2X_DEFAULT_SW_VER                 0x0001 // OBU rel. 230523
#define CLI_DB_V2X_DEFAULT_PAYLOAD_LEN            1024
#define CLI_DB_V2X_DEFAULT_BYTE_LEN               254
#define CLI_DB_V2X_DEFAULT_BUF_LEN                20

#define DB_MGR_FILE_MAX_LENGTH                    100
#define DB_MGR_SYSTEM_CALL_MAX_LENGTH             300

#define DB_MGR_DEFAULT_VEHICLE_SPEED              60
#define DB_MGR_DEFAULT_COMM_DEV_CNT               1

#define DB_MGR_DEFAULT_COMM_TYPE                  "Tx"
#define DB_MGR_DEFAULT_DEV_TYPE                   "OBU"
#define DB_MGR_DEFAULT_COMM_DEV_ID                "KETIV2X01"
#define DB_MGR_DEFAULT_START_TIME                 "2309010911"
#define DB_MGR_DEFAULT_END_TIME                   "2309011011"
#define DB_MGR_DEFAULT_TOTAL_TIME                 "1h"

#define DB_V2X_FOLDER_DIR                         "v2x-db"

#define DB_V2X_PORT                               50008
#define DB_V2X_IP                                 "221.140.137.186"
#define DB_V2X_YEAR                               "2024"
#define DB_V2X_MONTH                              "M_08"
#define DB_V2X_STORAGE                            "/exStorage/hdd_v2x_db_1/5g-nr-v2x/5g-nr-v2x-db/keti/results"

#if defined(CONFIG_PLATFORM_OBU_CHEMTRONICS)
#define DB_V2X_DEVICE_ID_PREFIX                   "CKCOX"
#else
#define DB_V2X_DEVICE_ID_PREFIX                   "UNKNOWN"
#endif

/***************************** Enum and Structure ****************************/

/**
* @details DB_MANAGER_FILE_TYPE_E
* @param DB_MANAGER_FILE_TYPE_TXT
* @param DB_MANAGER_FILE_TYPE_CSV
* @param DB_MANAGER_FILE_TYPE_SQLITE
*/
typedef enum {
    DB_MANAGER_FILE_TYPE_UNKNOWN                  = 0x0000,
    DB_MANAGER_FILE_TYPE_TXT                      = 0x0001,
    DB_MANAGER_FILE_TYPE_CSV                      = 0x0002,
    DB_MANAGER_FILE_TYPE_SQLITE                   = 0x0003,
    DB_MANAGER_FILE_TYPE_UNDEFINED_1,
    DB_MANAGER_FILE_TYPE_UNDEFINED_2,
    DB_MANAGER_FILE_TYPE_UNDEFINED_3,
    DB_MANAGER_FILE_TYPE_MAX                      = 0xFFFF
} DB_MANAGER_FILE_TYPE_E;

/**
* @details DB_MANAGER_SVC_TYPE_E
*/
typedef enum {
    DB_MANAGER_SVC_TYPE_BASE                      = 0,
    DB_MANAGER_SVC_TYPE_V2X_STATUS                = 1,
    DB_MANAGER_SVC_TYPE_PLATOONING                = 2,
    DB_MANAGER_SVC_TYPE_MAX                       = 255
} DB_MANAGER_SVC_TYPE_E;

/**
* @details DB_MANAGER_COMM_MSG_TYPE_E
* @param DB_MANAGER_COMM_MSG_TYPE_TX
* @param DB_MANAGER_COMM_MSG_TYPE_RX
*/
typedef enum {
    DB_MANAGER_COMM_MSG_TYPE_UNKNOW               = 0x0000,
    DB_MANAGER_COMM_MSG_TYPE_TX                   = 0x0001,
    DB_MANAGER_COMM_MSG_TYPE_RX                   = 0x0002,
    DB_MANAGER_COMM_MSG_TYPE_MAX                  = 0xFFFF
} DB_MANAGER_COMM_MSG_TYPE_E;

/**
* @details DB_MANAGER_PROC_E
* @param DB_MANAGER_PROC_WRITE
* @param DB_MANAGER_PROC_CONVERT
*/
typedef enum {
    DB_MANAGER_PROC_UNKNOWN                        = 0x0000,
    DB_MANAGER_PROC_WRITE                          = 0x0001,
    DB_MANAGER_PROC_READ                           = 0x0002,
    DB_MANAGER_PROC_CONVERT                        = 0x0003,
    DB_MANAGER_PROC_MAX                            = 0xFFFF
} DB_MANAGER_PROC_E;

/**
* @details DB_MANAGER_WRITE_T
* @param eFileType
* @param eCommType
* @param eProc
* @param unReserved
*/
typedef struct DB_MANAGER_WRITE_t {
    DB_MANAGER_FILE_TYPE_E      eFileType;
    DB_MANAGER_COMM_MSG_TYPE_E  eCommMsgType;
    DB_MANAGER_PROC_E           eProc;
    uint32_t                    unCrc32;
    uint32_t                    unReserved;
} DB_MANAGER_WRITE_T;

/**
* @details DB_MANAGER_READ_T
* @param eFileType
* @param eProc
*/
typedef struct DB_MANAGER_READ_t {
    DB_MANAGER_FILE_TYPE_E      eFileType;
    DB_MANAGER_PROC_E           eProc;
    uint32_t                    unReserved;
} DB_MANAGER_READ_T;

/**
* @details DB_MANAGER_EVENT_MSG_T
* @param pstDbManagerWrite
* @param pstDbV2x
* @param pPayload
*/
typedef struct DB_MANAGER_EVENT_MSG_t {
    DB_MANAGER_WRITE_T      *pstDbManagerWrite;
    DB_V2X_T                *pstDbV2x;
    void                    *pPayload;
} DB_MANAGER_EVENT_MSG_T;

typedef struct DB_MANAGER_FILE_t {
    char                    *pchTxRxType;
    char                    *pchDeviceType;
    char                    *pchDeviceId;
    char                    *pchStartTime;
    char                    *pchEndTime;
    char                    *pchTotalTime;
} DB_MANAGER_FILE_T;


/**
* @details DB_MANAGER_V2X_STATUS_t
* @param eFileType
* @param unReserved
*/
typedef struct DB_MANAGER_V2X_STATUS_t {
    DB_V2X_STATUS_TX_T      stV2xStatusTx;
    DB_V2X_STATUS_RX_T      stV2xStatusRx;
    DB_V2X_GPS_INFO_T       stV2xGpsInfoTx;
    DB_V2X_GPS_INFO_T       stV2xGpsInfoRx;
	DB_V2X_GPS_INFO_T		stV2xGpsInfoHeadingTx;
	DB_V2X_GPS_INFO_T		stV2xGpsInfoHeadingRx;
    uint64_t                ulTxTimeStamp;
    uint32_t                unLastContCnt;
    uint32_t                unCurrentContCnt;
    uint32_t                unContCntLoss;
    bool                    bFirstPacket;
} DB_MANAGER_V2X_STATUS_T;


/**
* @details DB_MANAGER_T
* @param eFileType
* @param unReserved
*/
typedef struct DB_MANAGER_t {
    DB_MANAGER_FILE_T       stDbFile;
    DB_MANAGER_FILE_TYPE_E  eFileType;
    DB_MANAGER_SVC_TYPE_E   eSvcType;
    DB_V2X_PLATOONING_T     stDbV2xPt;
    bool                    bLogLevel;
    uint32_t                unReserved;
} DB_MANAGER_T;

/***************************** Function Protype ******************************/

int32_t DB_MANAGER_Write(DB_MANAGER_WRITE_T *pstDbManagerWrite, DB_V2X_T *pstDbV2x, void *pPayload);
int32_t DB_MANAGER_Read(DB_MANAGER_READ_T *pstDbManagerRead, DB_V2X_T *pstDbV2x, void *pPayload);
int32_t DB_MANAGER_Converter(DB_MANAGER_READ_T *pstDbManagerRead, DB_MANAGER_WRITE_T *pstDbManagerWrite, DB_V2X_T *pstDbV2x, void *pPayload);

int32_t DB_MANAGER_SetLog(DB_MANAGER_T *pstDbManager);

int32_t DB_MANAGER_RemoveTempFile(DB_MANAGER_T *pstDbManager);
int32_t DB_MANAGER_UploadFile(DB_MANAGER_T *pstDbManager);
int32_t DB_MANAGER_MakeDbFile(DB_MANAGER_T *pstDbManager);
int32_t DB_MANAGER_Open(DB_MANAGER_T *pstDbManager);
int32_t DB_MANAGER_Close(DB_MANAGER_T *pstDbManager);
int32_t DB_MANAGER_Start(DB_MANAGER_T *pstDbManager);
int32_t DB_MANAGER_Stop(DB_MANAGER_T *pstDbManager);
int32_t DB_MANAGER_Status(DB_MANAGER_T *pstDbManager);

int32_t DB_MANAGER_Init(DB_MANAGER_T *pstDbManager);
int32_t DB_MANAGER_DeInit(DB_MANAGER_T *pstDbManager);
int32_t DB_MANAGER_SetV2xStatus(DB_MANAGER_V2X_STATUS_T *pstDbV2xStatus);
int32_t DB_MANAGER_GetV2xStatus(DB_MANAGER_V2X_STATUS_T *pstDbV2xStatus);

#endif	/* _DB_MANAGER_H_ */

