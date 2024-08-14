#ifndef	_DB_V2X_PLATOONING_H_
#define	_DB_V2X_PLATOONING_H_

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
* @file db_v2x_platooning.h
*
* This file contains a data format design of platooning service
*
* @note
*
* V2X Data Format Header of Platooning Service
*
* MODIFICATION HISTORY:
* Ver   Who  Date     Changes
* ----- ---- -------- ----------------------------------------------------
* 1  bman  23.06.19 First draft
*
******************************************************************************/

/***************************** Include ***************************************/
#include <stdint.h>

/***************************** Definition ************************************/
#define DB_V2X_PT_LV_VEHICLE_ID_LEN             10
#define DB_V2X_PT_LV_VEHICLE_ID_C_VEH           "C-VEH"
#define DB_V2X_PT_LV_VEHICLE_ID_A_VEH           "A-VEH"
#define DB_V2X_PT_LV_VEHICLE_ID_E_VEH           "E-VEH"
#define DB_V2X_PT_LV_VEHICLE_NUM_LEN            20
#define DB_V2X_PT_LV_VEHICLE_NUM_IONIQ5         "21HO0139"
#define DB_V2X_PT_LV_LANE_LEN                   20
#define DB_V2X_PT_LV_LANE_DEFAULT               "A2207G001101"
#define DB_V2X_PT_LV_PATH_PLAN_MAX_LEN          10
#define DB_V2X_PT_LV_DEFAULT_LATITUDE           37.406348
#define DB_V2X_PT_LV_DEFAULT_LONGITUDE          127.102576
#define DB_V2X_PT_FV_VEHICLE_ID_LEN             10
#define DB_V2X_PT_FV_VEHICLE_NUM_LEN            20
#define DB_V2X_PT_FV_LANE_LEN                   20
#define DB_V2X_PT_FV_PATH_PLAN_MAX_LEN          10

/***************************** Enum and Structure ****************************/

/* LV (Lead Vehicle) */
/**
* @details Service ID of the lead vehicle
* @param DB_V2X_PT_LV_SVC_ID_E
*/
typedef enum {
    eDB_V2X_PT_LV_SVC_ID_UNKNOWN               = 0,
    eDB_V2X_PT_LV_SVC_ID_PLATOONING            = 1,
    eDB_V2X_PT_LV_SVC_ID_SENSOR_SHARING        = 2,
    eDB_V2X_PT_LV_SVC_ID_REMOTE_DRIVING        = 3,
    eDB_V2X_PT_LV_SVC_ID_ADVANCED_DRIVING      = 4,
    eDB_V2X_PT_LV_SVC_ID_MAX
} DB_V2X_PT_LV_SVC_ID_E;

/**
* @details Method ID of the lead vehicle
* @param DB_V2X_PT_LV_METHOD_ID_E
*/
typedef enum {
    eDB_V2X_PT_LV_METHOD_ID_UNKNOWN            = 0,
    eDB_V2X_PT_LV_METHOD_ID_V2V                = 1,
    eDB_V2X_PT_LV_METHOD_ID_V2I                = 2,
    eDB_V2X_PT_LV_METHOD_ID_V2P                = 3,
    eDB_V2X_PT_LV_METHOD_ID_V2N                = 4,
    eDB_V2X_PT_LV_METHOD_ID_I2V                = 5,
    eDB_V2X_PT_LV_METHOD_ID_I2P                = 6,
    eDB_V2X_PT_LV_METHOD_ID_I2N                = 7,
    eDB_V2X_PT_LV_METHOD_ID_P2V                = 8,
    eDB_V2X_PT_LV_METHOD_ID_P2I                = 9,
    eDB_V2X_PT_LV_METHOD_ID_P2N                = 10,
    eDB_V2X_PT_LV_METHOD_ID_N2V                = 11,
    eDB_V2X_PT_LV_METHOD_ID_N2I                = 12,
    eDB_V2X_PT_LV_METHOD_ID_N2P                = 13,
    eDB_V2X_PT_LV_METHOD_ID_MAX
} DB_V2X_PT_LV_METHOD_ID_E;

/**
* @details Communication service method
* @param DB_V2X_PT_LV_MSG_TYPE_E
*/
typedef enum {
    eDB_V2X_PT_LV_MSG_TYPE_UNKNOWN             = 0,
    eDB_V2X_PT_LV_MSG_TYPE_BROADCAST           = 1,
    eDB_V2X_PT_LV_MSG_TYPE_UNICAST             = 2,
    eDB_V2X_PT_LV_MSG_TYPE_MULTICASST          = 3,
    eDB_V2X_PT_LV_MSG_TYPE_GRAOUPCAST          = 4,
    eDB_V2X_PT_LV_MSG_TYPE_MAX
} DB_V2X_PT_LV_MSG_TYPE_E;

/**
* @details Platooning Vehicle Type
* @param DB_V2X_PT_LV_VEHICLE_TYPE_E
*/
typedef enum {
    eDB_V2X_PT_LV_VEHICLE_TYPE_UNKNOWN         = 0,
    eDB_V2X_PT_LV_VEHICLE_TYPE_LEAD            = 1,
    eDB_V2X_PT_LV_VEHICLE_TYPE_FOLLOW          = 2,
    eDB_V2X_PT_LV_VEHICLE_TYPE_MAX
} DB_V2X_PT_LV_VEHICLE_TYPE_E;

/**
* @details Message Identifier
* @param DB_V2X_PT_LV_MSG_ID_E
*/
typedef enum {
    eDB_V2X_PT_LV_MSG_ID_INVALID               = 0,
    eDB_V2X_PT_LV_MSG_ID_REQ_L2F               = 1,
    eDB_V2X_PT_LV_MSG_ID_REQ_F2F               = 2,
    eDB_V2X_PT_LV_MSG_ID_RES_F2L               = 3,
    eDB_V2X_PT_LV_MSG_ID_RES_F2F               = 4,
    eDB_V2X_PT_LV_MSG_ID_MAX
} DB_V2X_PT_LV_MSG_ID_E;

/**
* @details a driving lane
* @param DB_V2X_PT_LV_DRIVE_STATUS_E
*/
typedef enum {
    eDB_V2X_PT_LV_DRIVE_STATUS_STAY_LANE       = 0,
    eDB_V2X_PT_LV_DRIVE_STATUS_CHANGE_LANE     = 1,
    eDB_V2X_PT_LV_DRIVE_STATUS_STOP            = 2,
    eDB_V2X_PT_LV_DRIVE_STATUS_SLOW            = 3,
    eDB_V2X_PT_LV_DRIVE_STATUS_MAX
} DB_V2X_PT_LV_DRIVE_STATUS_E;

/**
* @details Change the path
* @param DB_V2X_PT_LV_CHANGE_CODE_E
*/
typedef enum {
    eDB_V2X_PT_LV_CHANGE_NO                    = 0,
    eDB_V2X_PT_LV_CHANGE                       = 1,
    eDB_V2X_PT_LV_CHANGE_MAX
} DB_V2X_PT_LV_CHANGE_CODE_E;

/**
* @details Plan lane change in the
* @param DB_V2X_PT_LV_LANE_PLAN_E
*/
typedef enum {
    eDB_V2X_PT_LV_LANE_PLAN_STAY               = 0,
    eDB_V2X_PT_LV_LANE_PLAN_CHANGE             = 1,
    eDB_V2X_PT_LV_LANE_PLAN_MAX
} DB_V2X_PT_LV_LANE_PLAN_E;

/**
* @details Crossway
* @param DB_V2X_PT_LV_CROSSWAY_E
*/
typedef enum {
    eDB_V2X_PT_LV_GEN_LANE                     = 0,
    eDB_V2X_PT_LV_CROSSWAY_ENTER               = 1,
    eDB_V2X_PT_LV_CROSSWAY_INSIDE              = 2,
    eDB_V2X_PT_LV_CROSSWAY_OUT                 = 3,
    eDB_V2X_PT_LV_CROSSWAY_MAX
} DB_V2X_PT_LV_CROSSWAY_E;

/**
* @details Vehicle operation information on the link
* @param DB_V2X_PT_LV_LANE_MANEUVER_E
*/
typedef enum {
    eDB_V2X_PT_LV_LANE_STRAIGHT                = 0,
    eDB_V2X_PT_LV_LANE_LEFT                    = 1,
    eDB_V2X_PT_LV_LANE_RIGHT                   = 2,
    eDB_V2X_PT_LV_LANE_UTURN                   = 3,
    eDB_V2X_PT_LV_LANE_STRAIGHT_LEFT           = 4,
    eDB_V2X_PT_LV_LANE_STRAIGHT_RIGHT          = 5,
    eDB_V2X_PT_LV_LANE_MANEUVER_MAX
} DB_V2X_PT_LV_LANE_MANEUVER_E;

/**
* @details Provides path of GPS at intervals of 30 centimeters
* @param DB_V2X_PT_LV_PATH_PLAN_T
*/
typedef struct DB_V2X_PT_LV_PATH_PLAN_t {
    int32_t                          anLvLatitude[DB_V2X_PT_LV_PATH_PLAN_MAX_LEN];
    int32_t                          anLvLongitude[DB_V2X_PT_LV_PATH_PLAN_MAX_LEN];
} __attribute__((__packed__)) DB_V2X_PT_LV_PATH_PLAN_T;

/* FV (Follow Vehicle) */
/**
* @details Service ID of the follow vehicle
* @param DB_V2X_PT_FV_SVC_ID_E
*/
typedef enum {
    eDB_V2X_PT_FV_SVC_ID_UNKNOWN               = 0,
    eDB_V2X_PT_FV_SVC_ID_PLATOONING            = 1,
    eDB_V2X_PT_FV_SVC_ID_SENSOR_SHARING        = 2,
    eDB_V2X_PT_FV_SVC_ID_REMOTE_DRIVING        = 3,
    eDB_V2X_PT_FV_SVC_ID_ADVANCED_DRIVING      = 4,
    eDB_V2X_PT_FV_SVC_ID_MAX
} DB_V2X_PT_FV_SVC_ID_E;

/**
* @details Method ID of the follow vehicle
* @param DB_V2X_PT_FV_METHOD_ID_E
*/
typedef enum {
    eDB_V2X_PT_FV_METHOD_ID_UNKNOWN            = 0,
    eDB_V2X_PT_FV_METHOD_ID_V2V                = 1,
    eDB_V2X_PT_FV_METHOD_ID_V2I                = 2,
    eDB_V2X_PT_FV_METHOD_ID_V2P                = 3,
    eDB_V2X_PT_FV_METHOD_ID_V2N                = 4,
    eDB_V2X_PT_FV_METHOD_ID_I2V                = 5,
    eDB_V2X_PT_FV_METHOD_ID_I2P                = 6,
    eDB_V2X_PT_FV_METHOD_ID_I2N                = 7,
    eDB_V2X_PT_FV_METHOD_ID_P2V                = 8,
    eDB_V2X_PT_FV_METHOD_ID_P2I                = 9,
    eDB_V2X_PT_FV_METHOD_ID_P2N                = 10,
    eDB_V2X_PT_FV_METHOD_ID_N2V                = 11,
    eDB_V2X_PT_FV_METHOD_ID_N2I                = 12,
    eDB_V2X_PT_FV_METHOD_ID_N2P                = 13,
    eDB_V2X_PT_FV_METHOD_ID_MAX
} DB_V2X_PT_FV_METHOD_ID_E;

/**
* @details Communication service method
* @param DB_V2X_PT_FV_MSG_TYPE_E
*/
typedef enum {
    eDB_V2X_PT_FV_MSG_TYPE_UNKNOWN             = 0,
    eDB_V2X_PT_FV_MSG_TYPE_BROADCAST           = 1,
    eDB_V2X_PT_FV_MSG_TYPE_UNICAST             = 2,
    eDB_V2X_PT_FV_MSG_TYPE_MULTICAST           = 3,
    eDB_V2X_PT_FV_MSG_TYPE_GRAOUPCAST          = 4,
    eDB_V2X_PT_FV_MSG_TYPE_MAX
} DB_V2X_PT_FV_MSG_TYPE_E;

/**
* @details Platooning Vehicle Type
* @param DB_V2X_PT_FV_VEHICLE_TYPE_E
*/
typedef enum {
    eDB_V2X_PT_FV_VEHICLE_TYPE_UNKNOWN         = 0,
    eDB_V2X_PT_FV_VEHICLE_TYPE_LEAD            = 1,
    eDB_V2X_PT_FV_VEHICLE_TYPE_FOLLOW          = 2,
    eDB_V2X_PT_FV_VEHICLE_TYPE_MAX
} DB_V2X_PT_FV_VEHICLE_TYPE_E;

/**
* @details Message Identifier
* @param DB_V2X_PT_FV_MSG_ID_E
*/
typedef enum {
    eDB_V2X_PT_FV_MSG_ID_INVALID               = 0,
    eDB_V2X_PT_FV_MSG_ID_REQ_L2F               = 1,
    eDB_V2X_PT_FV_MSG_ID_REQ_F2F               = 2,
    eDB_V2X_PT_FV_MSG_ID_RES_F2L               = 3,
    eDB_V2X_PT_FV_MSG_ID_RES_F2F               = 4,
    eDB_V2X_PT_FV_MSG_ID_MAX
} DB_V2X_PT_FV_MSG_ID_E;

/**
* @details a driving lane
* @param DB_V2X_PT_FV_DRIVE_STATUS_E
*/
typedef enum {
    eDB_V2X_PT_FV_DRIVE_STATUS_STAY_LANE       = 0,
    eDB_V2X_PT_FV_DRIVE_STATUS_CHANGE_LANE     = 1,
    eDB_V2X_PT_FV_DRIVE_STATUS_STOP            = 2,
    eDB_V2X_PT_FV_DRIVE_STATUS_SLOW            = 3,
    eDB_V2X_PT_FV_DRIVE_STATUS_MAX
} DB_V2X_PT_FV_DRIVE_STATUS_E;

/**
* @details Change the path
* @param DB_V2X_PT_FV_CHANGE_CODE_E
*/
typedef enum {
    eDB_V2X_PT_FV_CHANGE_NO                    = 0,
    eDB_V2X_PT_FV_CHANGE                       = 1,
    eDB_V2X_PT_FV_CHANGE_MAX
} DB_V2X_PT_FV_CHANGE_CODE_E;

/**
* @details Change the path
* @param DB_V2X_PLATOONING_TYPE_E
*/
typedef enum {
    eDB_V2X_PT_TYPE_NONE                       = 0,
    eDB_V2X_PT_TYPE_LV                         = 1,
    eDB_V2X_PT_TYPE_FV                         = 2,
    eDB_V2X_PT_TYPE_MAX
} DB_V2X_PLATOONING_TYPE_E;

/**
* @details Provides path of GPS at intervals of 30 centimeters
* @param DB_V2X_PT_FV_PATH_PLAN_T
*/
typedef struct DB_V2X_PT_FV_PATH_PLAN_t {
    int32_t                          anFvLatitude[DB_V2X_PT_FV_PATH_PLAN_MAX_LEN];
    int32_t                          anFvLongitude[DB_V2X_PT_FV_PATH_PLAN_MAX_LEN];
} __attribute__((__packed__)) DB_V2X_PT_FV_PATH_PLAN_T;


/**
* @details Service ID of the Lead vehicle (PTLV)
* @param DB_V2X_PLATOONING_LV_T
*/
typedef struct DB_V2X_PLATOONING_LV_t {
    DB_V2X_PT_LV_SVC_ID_E            eLvServiceId;
    DB_V2X_PT_LV_METHOD_ID_E         eLvMethodId;
    uint32_t                         unLvLength;
    uint16_t                         usLvClientId;
    uint16_t                         usLvSessionId;
    uint8_t                          ucLvProtocolVer;
    uint8_t                          ucLvInterfaceVer;
    DB_V2X_PT_LV_MSG_TYPE_E          eLvMsgType;
    uint8_t                          ucLvReturnCode;
    DB_V2X_PT_LV_VEHICLE_TYPE_E      eLvVehicleType;
    uint8_t                          szLvVehicleId[DB_V2X_PT_LV_VEHICLE_ID_LEN];
    uint8_t                          szLvVehicleNum[DB_V2X_PT_LV_VEHICLE_NUM_LEN];
    uint16_t                         usLvMsgCount;
    DB_V2X_PT_LV_MSG_ID_E            eLvMsgId;
    int32_t                          nLvLatitude;
    int32_t                          nLvLongitude;
    uint16_t                         usLvHeading;
    uint16_t                         usLvSpeed;
    uint8_t                          szLvDriveLaneId[DB_V2X_PT_LV_LANE_LEN];
    DB_V2X_PT_LV_DRIVE_STATUS_E      eLvDriveStatus;
    DB_V2X_PT_LV_CHANGE_CODE_E       eLvChangeCode;
    uint16_t                         usLvPathId;
    uint8_t                          szLvLaneId[DB_V2X_PT_LV_LANE_LEN];
    DB_V2X_PT_LV_LANE_PLAN_E         eLvLanePlan;
    DB_V2X_PT_LV_CROSSWAY_E          eLvCrossway;
    DB_V2X_PT_LV_LANE_MANEUVER_E     eLvLaneManeuver;
    DB_V2X_PT_LV_PATH_PLAN_T         stLvPathPlan;
    uint64_t                         unReserved1;
} __attribute__((__packed__)) DB_V2X_PLATOONING_LV_T;

/**
* @details Service ID of the follow vehicle (PTFV)
* @param DB_V2X_PLATOONING_FV_T
*/
typedef struct DB_V2X_PLATOONING_FV_t {
    DB_V2X_PT_FV_SVC_ID_E            eFvServiceId;
    DB_V2X_PT_FV_METHOD_ID_E         eFvMethodId;
    uint32_t                         unFvLength;
    uint16_t                         usFvClientId;
    uint16_t                         usFvSessionId;
    uint8_t                          ucFvProtocolVer;
    uint8_t                          ucFvInterfaceVer;
    DB_V2X_PT_FV_MSG_TYPE_E          eFvMsgType;
    uint8_t                          ucFvReturnCode;
    DB_V2X_PT_FV_VEHICLE_TYPE_E      eFvVehicleType;
    uint8_t                          szFvVehicleId[DB_V2X_PT_FV_VEHICLE_ID_LEN];
    uint8_t                          szFvVehicleNum[DB_V2X_PT_FV_VEHICLE_NUM_LEN];
    uint16_t                         usFvMsgCount;
    DB_V2X_PT_FV_MSG_ID_E            eFvMsgId;
    int32_t                          nFvLatitude;
    int32_t                          nFvLongitude;
    uint16_t                         usFvHeading;
    uint16_t                         usFvSpeed;
    uint8_t                          szFvDriveLaneId[DB_V2X_PT_FV_LANE_LEN];
    DB_V2X_PT_FV_DRIVE_STATUS_E      eFvDriveStatus;
    DB_V2X_PT_FV_CHANGE_CODE_E       eFvChangeCode;
    DB_V2X_PT_FV_PATH_PLAN_T         stFvPathPlan;
    uint16_t                         usFvRecommDistance;
    uint16_t                         usFvRecommSpeed;
    uint64_t                         unReserved1;
    uint64_t                         unReserved2;
    uint64_t                         unReserved3;
    uint64_t                         unReserved4;
    uint32_t                         unReserved5;
    uint16_t                         unReserved6;
} __attribute__((__packed__)) DB_V2X_PLATOONING_FV_T;

/**
* @details DB V2X Platooning Struct
* @param ulReserved        reserved
*/
typedef struct DB_V2X_PLATOONING_t {
    DB_V2X_PLATOONING_TYPE_E         eDbV2XPtType;
    uint16_t                         usV2xGroupId;
} __attribute__((__packed__)) DB_V2X_PLATOONING_T;

/***************************** Function Protype ******************************/

#endif	/* _DB_V2X_PLATOONING_H_ */

