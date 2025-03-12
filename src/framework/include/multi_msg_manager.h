#ifndef	_MULTI_MSG_MANAGER_H_
#define	_MULTI_MSG_MANAGER_H_

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
* @file multi_msg_manager.h
*
* @note
*
* MULTI MSG Manager Header
*
******************************************************************************/


/***************************** Include ***************************************/
#include "type.h"
#include "db_v2x.h"

/***************************** Definition ************************************/
#define MULTI_MSG_MANAGER_MAC_LENGTH                      (6)
#if defined(CONFIG_OBU_MAX_DEV)
#define MULTI_MSG_MANAGER_MAX_IP_COUNT                    CONFIG_OBU_MAX_DEV
#endif

 /**
* @details V2X TX POWER
* @param MULTI_MSG_MANAGER_V2X_TX_POWER       DSRC (0 ~ 23, Korea 20), C-V2X (-30~23)
*/
#define MULTI_MSG_MANAGER_V2X_TX_POWER                    (20)
#define MULTI_MSG_MANAGER_V2X_TX_BW                       (20)
#define MULTI_MSG_MANAGER_V2X_TX_FREQ                     (5915)

#define MULTI_MSG_MANAGER_V2X_TX_PROFILE_ID               (100)
#define MULTI_MSG_MANAGER_V2X_TX_PEER_L2_ID               (0)

#define MULTI_MSG_MANAGER_MSG_BUF_MAX_LEN                 (1024)
#define MULTI_MSG_MANAGER_MSG_HEX_STR_LEN                 (5)
#define MULTI_MSG_MANAGER_MSG_HEX_SIZE                    (16)

#define MULTI_MSG_MANAGER_CRC16_LEN                       (2)
#define MULTI_MSG_MANAGER_CRC32_LEN                       (4)

#define MULTI_MSG_MANAGER_V2X_TX_COUNT                    (10)
#define MULTI_MSG_MANAGER_V2X_TX_DELAY                    (1000)

#define MULTI_MSG_MANAGER_SEND_DELAY                      (100 * 1000)

 /**
* @details Multi V2X Extensible Message Format
*/
#define MULTI_MSG_MANAGER_EXT_MSG_MAGIC_NUM_LEN           (4)
#define MULTI_MSG_MANAGER_EXT_MSG_MAGIC_NUM_NAME          "5GVX"
#define MULTI_MSG_MANAGER_EXT_MSG_OVERALL_PKG_EXTENSIBLE    'E'
#define MULTI_MSG_MANAGER_EXT_MSG_OVERALL_PKG_MESSAGE       'M'
#define MULTI_MSG_MANAGER_EXT_MSG_OVERALL_PKG_OVERALL       'O'
#define MULTI_MSG_MANAGER_EXT_MSG_OVERALL_PKG_PACKAGE       'P'

#define MULTI_MSG_MANAGER_EXT_MSG_OVERALL_PKG_VER         (2)
#define MULTI_MSG_MANAGER_EXT_MSG_V2V_PSID                (58200)
#define MULTI_MSG_MANAGER_EXT_MSG_V2I_PSID                (58201)
#define MULTI_MSG_MANAGER_EXT_MSG_I2V_PSID                (58202)

#define MULTI_MSG_MANAGER_EXT_MSG_OVERALL_PKG             (58220)
#define MULTI_MSG_MANAGER_EXT_MSG_RAW_DATA_PKG            (58221)
#define MULTI_MSG_MANAGER_EXT_MSG_SSOV_PKG                (58222)
#define MULTI_MSG_MANAGER_EXT_MSG_STATUS_PKG              (58223)

#define MULTI_MSG_MANAGER_EXT_MSG_FTP_REQ                 (58240)
#define MULTI_MSG_MANAGER_EXT_MSG_FTP_RESP                (58241)

#define MULTI_MSG_MANAGER_EXT_MSG_TEMP_NO_MEASURE         (-128)
#define MULTI_MSG_MANAGER_EXT_MSG_TEMP_LIMIT_OVER         (127)
#define MULTI_MSG_MANAGER_EXT_MSG_TEMP_LIMIT_BELOW        (-127)

#define MULTI_MSG_MANAGER_EXT_MSG_UNIT_ID_OBU_MODEM       (0x1)
#define MULTI_MSG_MANAGER_EXT_MSG_UNIT_ID_OBU_COMM        (0x2)
#define MULTI_MSG_MANAGER_EXT_MSG_UNIT_ID_OBU_CONTROL     (0x4)
#define MULTI_MSG_MANAGER_EXT_MSG_UNIT_ID_RSU_MODEM       (0x10)
#define MULTI_MSG_MANAGER_EXT_MSG_UNIT_ID_RSU_COMM        (0x20)
#define MULTI_MSG_MANAGER_EXT_MSG_UNIT_ID_RSU_CONTROL     (0x40)

 /**
* @details Multi V2X Web Socket
*/
#define MULTI_MSG_MANAGER_READ_TAIL                       (1)
#define MULTI_MSG_MANAGER_WEBSOCKET_PORT                  (3001)
#define MULTI_MSG_MANAGER_WEBSOCKET_BUF_MAX_LEN           (1024)

#define MULTI_MSG_MANAGER_DB_FILE_PATH                    "/tmp"

#define MULTI_MSG_MANAGER_WEBSERVER_FILE_SAMPLE_RX        "rx_ctack_240905.csv"
#define MULTI_MSG_MANAGER_WEBSERVER_FILE_SAMPLE_TX        "tx_ctrack_240820.csv"

#define MULTI_MSG_MANAGER_WEBSERVER_FILE_TX               "/tmp/db_v2x_tx_temp_writing.csv"
#define MULTI_MSG_MANAGER_WEBSERVER_FILE_RX               "/tmp/db_v2x_rx_temp_writing.csv"

/***************************** Enum and Structure ****************************/

/**
* @details MULTI_MSG_MANAGER_TX_PAYLOAD_TYPE_E
* @param MULTI_MSG_MANAGER_TX_PAYLOAD_TYPE_RAW
* @param MULTI_MSG_MANAGER_TX_PAYLOAD_TYPE_ENCODED_J2735   ASN.1 encoded J2735 Msg
* @param MULTI_MSG_MANAGER_TX_PAYLOAD_TYPE_ITSK00130       ITSK-00130
* @param MULTI_MSG_MANAGER_TX_PAYLOAD_TYPE_5GNRV2X         5G-NR-V2X data frame
* @param MULTI_MSG_MANAGER_TX_PAYLOAD_TYPE_5GNRV2X_SS      5G-NR-V2X sensor sharing data frame
*/
typedef enum
{
    eMULTI_MSG_MANAGER_PAYLOAD_TYPE_RAW = 0,
    eMULTI_MSG_MANAGER_PAYLOAD_TYPE_ENCODED_J2735 = 1,
    eMULTI_MSG_MANAGER_PAYLOAD_TYPE_ITSK00130 = 2,
    eMULTI_MSG_MANAGER_PAYLOAD_TYPE_5GNRV2X = 3,
    eMULTI_MSG_MANAGER_PAYLOAD_TYPE_5GNRV2X_SS = 4,
    eMULTI_MSG_MANAGER_PAYLOAD_TYPE_MAX = 0xFF
} MULTI_MSG_MANAGER_PAYLOAD_TYPE_E;

typedef enum
{
    eMULTI_MSG_MANAGER_COMM_TYPE_UNKNOWN = 0,
    eMULTI_MSG_MANAGER_COMM_TYPE_DSRC,
    eMULTI_MSG_MANAGER_COMM_TYPE_LTEV2X,
    eMULTI_MSG_MANAGER_COMM_TYPE_5GNRV2X,
    eMULTI_MSG_MANAGER_COMM_TYPE_MAX = 0xFF
} MULTI_MSG_MANAGER_COMM_TYPE_E;

typedef enum
{
    eMULTI_MSG_MANAGER_SIGN_ID_UNSECURED = 0,
    eMULTI_MSG_MANAGER_SIGN_ID_CERTIFICATE,
    eMULTI_MSG_MANAGER_SIGN_ID_DIGEST,
    eMULTI_MSG_MANAGER_SIGN_ID_ALTERNATE,
    eMULTI_MSG_MANAGER_SIGN_ID_MAX = 0xFF
} MULTI_MSG_MANAGER_SIGN_ID_E;

typedef enum
{
    eMULTI_MSG_MANAGER_FILE_TYPE_UNKNOWN = 0,
    eMULTI_MSG_MANAGER_FILE_TYPE_TX,
    eMULTI_MSG_MANAGER_FILE_TYPE_RX,
    eMULTI_MSG_MANAGER_FILE_TYPE_SAMPLE_TX,
    eMULTI_MSG_MANAGER_FILE_TYPE_SAMPLE_RX,
    eMULTI_MSG_MANAGER_FILE_TYPE_MAX = 0xFF
} MULTI_MSG_MANAGER_FILE_TYPE_E;

typedef enum
{
    eMULTI_MSG_MANAGER_PRIORITY_CV2X_PPPP_MIN = 0,      /* CV2X */
    eMULTI_MSG_MANAGER_PRIORITY_CV2X_PPPP_0 = 0,
    eMULTI_MSG_MANAGER_PRIORITY_CV2X_PPPP_1 = 1,
    eMULTI_MSG_MANAGER_PRIORITY_CV2X_PPPP_2 = 2,
    eMULTI_MSG_MANAGER_PRIORITY_CV2X_PPPP_3 = 3,
    eMULTI_MSG_MANAGER_PRIORITY_CV2X_PPPP_4 = 4,
    eMULTI_MSG_MANAGER_PRIORITY_CV2X_PPPP_5 = 5,
    eMULTI_MSG_MANAGER_PRIORITY_CV2X_PPPP_6 = 6,
    eMULTI_MSG_MANAGER_PRIORITY_CV2X_PPPP_7 = 7,
    eMULTI_MSG_MANAGER_PRIORITY_CV2X_PPPP_MAX = 7,
    eMULTI_MSG_MANAGER_PRIORITY_CV2X_PPPP_INVALID = 8,
    eMULTI_MSG_MANAGER_PRIORITY_DSRC_USER_PRIO_MIN = 0, /* DSRC */
    eMULTI_MSG_MANAGER_PRIORITY_DSRC_USER_PRIO_00 = 0,
    eMULTI_MSG_MANAGER_PRIORITY_DSRC_USER_PRIO_01 = 1,
    eMULTI_MSG_MANAGER_PRIORITY_DSRC_USER_PRIO_02 = 2,
    eMULTI_MSG_MANAGER_PRIORITY_DSRC_USER_PRIO_03 = 3,
    eMULTI_MSG_MANAGER_PRIORITY_DSRC_USER_PRIO_04 = 4,
    eMULTI_MSG_MANAGER_PRIORITY_DSRC_USER_PRIO_05 = 5,
    eMULTI_MSG_MANAGER_PRIORITY_DSRC_USER_PRIO_06 = 6,
    eMULTI_MSG_MANAGER_PRIORITY_DSRC_USER_PRIO_07 = 7,
    eMULTI_MSG_MANAGER_PRIORITY_DSRC_USER_PRIO_MAX = 7,
    eMULTI_MSG_MANAGER_PRIORITY_DSRC_USER_PRIO_INVALID = 8,
    eMULTI_MSG_MANAGER_PRIORITY_MAX
} MULTI_MSG_MANAGER_PRIORITY_E;

typedef enum
{
    eMULTI_MSG_MANAGER_V2X_FREQ_5860 = 5860, /* 172 ch */
    eMULTI_MSG_MANAGER_V2X_FREQ_5870 = 5870, /* 174 ch */
    eMULTI_MSG_MANAGER_V2X_FREQ_5880 = 5880, /* 176 ch */
    eMULTI_MSG_MANAGER_V2X_FREQ_5890 = 5890, /* 178 ch */
    eMULTI_MSG_MANAGER_V2X_FREQ_5900 = 5900, /* 180 ch */
    eMULTI_MSG_MANAGER_V2X_FREQ_5910 = 5910, /* 182 ch */
    eMULTI_MSG_MANAGER_V2X_FREQ_5920 = 5920, /* 184 ch */
    eMULTI_MSG_MANAGER_V2X_FREQ_Min = eMULTI_MSG_MANAGER_V2X_FREQ_5860,
    eMULTI_MSG_MANAGER_V2X_FREQ_Max = eMULTI_MSG_MANAGER_V2X_FREQ_5920
} MULTI_MSG_MANAGER_V2X_FREQ_E;

typedef enum
{
    eMULTI_MSG_MANAGER_V2X_DATA_RATE_NA = 0,
    eMULTI_MSG_MANAGER_V2X_DATA_RATE_3MBPS = 0x06,
    eMULTI_MSG_MANAGER_V2X_DATA_RATE_4_5MBPS = 0x09,
    eMULTI_MSG_MANAGER_V2X_DATA_RATE_6MBPS = 0x0C,
    eMULTI_MSG_MANAGER_V2X_DATA_RATE_9MBPS = 0x12,
    eMULTI_MSG_MANAGER_V2X_DATA_RATE_12MBPS = 0x18,
    eMULTI_MSG_MANAGER_V2X_DATA_RATE_18MBPS = 0x24,
    eMULTI_MSG_MANAGER_V2X_DATA_RATE_24MBPS = 0x30,
    eMULTI_MSG_MANAGER_V2X_DATA_RATE_27MBPS = 0x36,
    eMULTI_MSG_MANAGER_V2X_DATA_RATE_MAX
} MULTI_MSG_MANAGER_V2X_DATA_RATE_E;

/**
* @details MULTI_MSG_MANAGER_V2X_TIME_SLOT_E (1609.3 - 7.3.2.2)
*/
typedef enum
{
    eMULTI_MSG_MANAGER_V2X_TIME_SLOT_0 = 0,
    eMULTI_MSG_MANAGER_V2X_TIME_SLOT_1,
    eMULTI_MSG_MANAGER_V2X_TIME_SLOT_CONTINUOUS,
    eMULTI_MSG_MANAGER_V2X_TIME_SLOT_MAX
} MULTI_MSG_MANAGER_V2X_TIME_SLOT_E;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_PAYLOAD_ID_E
*/
typedef enum
{
    eMULTI_MSG_MANAGER_EXT_MSG_PAYLOAD_ID_RESERVED = 0x00,
    eMULTI_MSG_MANAGER_EXT_MSG_PAYLOAD_ID_ACK,
    eMULTI_MSG_MANAGER_EXT_MSG_PAYLOAD_ID_NAK,
    eMULTI_MSG_MANAGER_EXT_MSG_PAYLOAD_ID_HEARTBEAT,
    eMULTI_MSG_MANAGER_EXT_MSG_PAYLOAD_ID_TIME,
    eMULTI_MSG_MANAGER_EXT_MSG_PAYLOAD_ID_STATUS,
    eMULTI_MSG_MANAGER_EXT_MSG_PAYLOAD_ID_REPAIR,
    eMULTI_MSG_MANAGER_EXT_MSG_PAYLOAD_ID_CMD,
    eMULTI_MSG_MANAGER_EXT_MSG_PAYLOAD_ID_TX = 0x10,
    eMULTI_MSG_MANAGER_EXT_MSG_PAYLOAD_ID_RX,
    eMULTI_MSG_MANAGER_EXT_MSG_PAYLOAD_ID_WSM_SVC_REQ,
    eMULTI_MSG_MANAGER_EXT_MSG_PAYLOAD_ID_WSM_SVC_CONFIRM,
    eMULTI_MSG_MANAGER_EXT_MSG_PAYLOAD_ID_MAX = 0xFF
} MULTI_MSG_MANAGER_EXT_MSG_PAYLOAD_ID_E;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_ACTION_E
*/
typedef enum
{
    eMULTI_MSG_MANAGER_EXT_MSG_ACTION_ADD = 0,
    eMULTI_MSG_MANAGER_EXT_MSG_ACTION_DEL = 1,
    eMULTI_MSG_MANAGER_EXT_MSG_ACTION_MAX
} MULTI_MSG_MANAGER_EXT_MSG_ACTION_E;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_WSC_ACTION_E
*/
typedef enum
{
    eMULTI_MSG_MANAGER_EXT_MSG_WSC_ACTION_FAIL = 0x00,
    eMULTI_MSG_MANAGER_EXT_MSG_WSC_ACTION_ADDED,
    eMULTI_MSG_MANAGER_EXT_MSG_WSC_ACTION_ALREADY_ADD,
    eMULTI_MSG_MANAGER_EXT_MSG_WSC_ACTION_DELETED,
    eMULTI_MSG_MANAGER_EXT_MSG_WSC_ACTION_NOT_EXIST,
    eMULTI_MSG_MANAGER_EXT_MSG_WSC_ACTION_ALL_DELETED,
    eMULTI_MSG_MANAGER_EXT_MSG_WSC_ACTION_MAX
} MULTI_MSG_MANAGER_EXT_MSG_WSC_ACTION_E;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_DEV_TYPE_E
*/
typedef enum
{
    eMULTI_MSG_MANAGER_EXT_MSG_DEV_TYPE_OBU_MODEM = 10,
    eMULTI_MSG_MANAGER_EXT_MSG_DEV_TYPE_OBU = 11,
    eMULTI_MSG_MANAGER_EXT_MSG_DEV_TYPE_RSU_MODEM = 20,
    eMULTI_MSG_MANAGER_EXT_MSG_DEV_TYPE_RSU = 21,
    eMULTI_MSG_MANAGER_EXT_MSG_DEV_TYPE_RSU_CTL = 22,
    eMULTI_MSG_MANAGER_EXT_MSG_DEV_TYPE_MAX
} MULTI_MSG_MANAGER_EXT_MSG_DEV_TYPE_E;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_STATUS_E
*/
typedef enum
{
    eMULTI_MSG_MANAGER_EXT_MSG_STATUS_TX = 0,
    eMULTI_MSG_MANAGER_EXT_MSG_STATUS_RX = 1,
    eMULTI_MSG_MANAGER_EXT_MSG_STATUS_MAX
} MULTI_MSG_MANAGER_EXT_MSG_STATUS_E;

/**
* @details MULTI_MSG_MANAGER_TASK
* @param unDevIdx  device ID
*/
typedef struct MULTI_MSG_MANAGER_TASK_t {
    uint32_t    unDevIdx;
} MULTI_MSG_MANAGER_TASK_T;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG
* @param cMagicNumber (4bytes) 5GVX, frame identifier
* @param usLength (2bytes) length between usSeqNum and CRC16
* @param usSeqNum (2bytes) 0000h ~ FFFFh, sequence number, increases for each frame, frame duplication check, ACK target used for designation
* @param usPayloadId (2bytes) specify payload type and format of payload area
* @param ucPayload (n) Transfer data defined according to payload ID
*/
typedef struct MULTI_MSG_MANAGER_EXT_MSG_t
{
    char        cMagicNumber[MULTI_MSG_MANAGER_EXT_MSG_MAGIC_NUM_LEN];
    uint16_t    usLength;
    uint16_t    usSeqNum;
    uint16_t    usPayloadId;
    uint8_t     ucPayload[0];
    /* CRC16 (2bytes) is added at the end of packet from usLength to ucPayload */
}__attribute__((__packed__)) MULTI_MSG_MANAGER_EXT_MSG;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_WSR
* @param ucAction (1bytes) 0:add PSID, 1:delte PSID, 2:delte all (receive all PSID)
* @param unPsid (4bytes) 0 ~ 270549119, not used if action is delete all
*/
typedef struct  MULTI_MSG_MANAGER_EXT_MSG_WSR_t
{
    uint8_t     ucAction;
    uint32_t    unPsid;
    uint16_t    usCrc16;
}__attribute__((__packed__)) MULTI_MSG_MANAGER_EXT_MSG_WSR;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_WSC
* @param ucActionRst (1bytes) see (MULTI_MSG_MANAGER_EXT_MSG_WSC_ACTION_E)
* @param unPsid (4bytes) 0 ~ 270549119, target PSID of action result, not used if action is delete all
*/
typedef struct MULTI_MSG_MANAGER_EXT_MSG_WSC_t
{
    uint8_t     ucActionRst;
    uint32_t    unPsid;
    uint16_t    usCrc16;
}__attribute__((__packed__)) MULTI_MSG_MANAGER_EXT_MSG_WSC;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_TX
* @param unPsid (4bytes) 0 ~ 270549119
* @param ucPayload (n) byte sequence (fixed message, extensible message, etc.)
*/
typedef struct MULTI_MSG_MANAGER_EXT_MSG_TX_t
{
    uint32_t    unPsid;
    uint8_t     ucPayload[0];
}__attribute__((__packed__)) MULTI_MSG_MANAGER_EXT_MSG_TX;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_RX
* @param unPsid (4bytes) 0 ~ 270549119
* @param ucRcpi (1bytes) 0 ~ 255
* @param ucPayload byte sequence (fixed message, extensible message, etc.)
*/
typedef struct MULTI_MSG_MANAGER_EXT_MSG_RX_t
{
    uint32_t    unPsid;
    uint8_t     ucRcpi;
    uint8_t     ucPayload[0];
}__attribute__((__packed__)) MULTI_MSG_MANAGER_EXT_MSG_RX;

/**
* Extensible Message (Payload)
* - | PSID | T L V C |
*   |------|---------|
*   |  4   | 4 2 N 2 |
*
*	PSID : 58200(V2V), 58201(V2I), 58202(I2V)
*	T : 58220(Overall), 588221(Raw Data), 58222(SSOV), 58223(Status)
*	L :
*	V :
*	C : CRC16
*/

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_SSOV (define TLVC of Extensible Message)
* @param unType (T) package types (overall, raw data, J2735, SSOV, etc)
* @param usLength (L) length between value and crc
* @param ucPayload (V) defined by unType
* @param CRC16 (C) crc of TLV (polynomial 1021h)
*/
typedef struct MULTI_MSG_MANAGER_EXT_MSG_SSOV_t
{
    uint32_t    unType;
    uint16_t    usLength;
    uint8_t     ucPayload[0];
    /* CRC16 (2bytes) is added at the end of packet from usLength to ucPayload */
}__attribute__((__packed__)) MULTI_MSG_MANAGER_EXT_MSG_SSOV;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_TLVC (define TLVC of Extensible Message)
* @param unType (T) package types (overall, raw data, J2735, SSOV, etc)
* @param usLength (L) length between value and crc
* @param ucPayload (V) defined by unType
* @param CRC16 (C) crc of TLV (polynomial 1021h)
*/
typedef struct MULTI_MSG_MANAGER_EXT_MSG_TLVC_t
{
    uint32_t    unType;
    uint16_t    usLength;
    uint8_t     ucPayload[0];
    /* CRC16 (2bytes) is added at the end of packet from usLength to ucPayload */
}__attribute__((__packed__)) MULTI_MSG_MANAGER_EXT_MSG_TLVC;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_TLVC_OVERALL
* @param unType (T) the type of overall package (58220)
* @param usLength (L) the length of overall package
* @param cMagicNum (V) the type of magic number (EMOP:Extensible Message Overall Package)
* @param ucVersion (V) the overall package version (0~255)
* @param ucNumOfPkg (V) the number of following packages (n) (excluding overall package) number of packages)
* @param usLenOfPkg (V) the sum of the following package lengths (= T+L+V+C) (length sum) (Total length of packages excluding overall package)
* @param ucBitsize (V) optional status package additional field (unit bit 1 means adding status package), unit bit (0:OBU Modem unit)
                        . bit 0 : OBU modem unit
                        . bit 1 : OBU communication unit
                        . bit 2 : OBU control unit
                        . bit 3 : unused
                        . bit 4 : RSU modem unit
                        . bit 5 : RSU communication unit
                        . bit 6 : RSU control unit
                        . bit 7 : unused
* @param usCrcv (C) CRC16
*/
typedef struct MULTI_MSG_MANAGER_EXT_MSG_TLVC_OVERALL_t
{
    uint32_t    unType;
    uint16_t    usLength;
    char        chMagicNum[MSG_MANAGER_EXT_MSG_MAGIC_NUM_LEN];
    uint8_t     ucVersion;
    uint8_t     ucNumOfPkg;
    uint16_t    usLenOfPkg;
    uint8_t     ucBitwise;
    uint16_t    usCrc16;
}__attribute__((__packed__)) MULTI_MSG_MANAGER_EXT_MSG_TLVC_OVERALL;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_TX
* @param unType         the type of modem unit package (58223)
* @param usLenth        the length
* @param ucDevType      the device type obu_modem:10, obu:11, rsu_modem:20, rsu:21, rsu_control:22
* @param ucStatus       the ID Tx (0) / Rx (1)
* @param unDevId        the device id of obu modem
* @param usHwVer        the hardware version of obu modem
* @param usSwVer        the software version of obu modem
* @param ulTimeStamp    the Tx timestamp of obu modem
* @param ucTxPwr        the Tx power of obu modem (dBm)
* @param usTxFreq       the Tx frequence of obu modem (MHz)
* @param ucTxBw         the Tx bandwidth of obu modem (MHz)
* @param ucScs          the subcarrier spacing (numerology), [15, 30, 60] (kHz)
* @param ucMcs          the MCS Index (x, [7..6] mcs table [5…0] mcs index)
* @param nLatitude      the latitide of obu modem
* @param nLongitude     the longtitude of obu modem
* @param chCpuTemp      the CPU temperature of inside
* @param chPeriTemp     the CPU temeprature of outside by using temperature sensors if it is available
* @param usCrc          the CRC16
*/
typedef struct MULTI_MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_TX_t
{
    uint32_t    unType;
    uint16_t    usLenth;
    uint8_t     ucDevType;
    uint8_t     ucStatus;
    uint32_t    unDevId;
    uint16_t    usHwVer;
    uint16_t    usSwVer;
    uint64_t    ulTimeStamp;
    uint8_t     ucTxPwr;
    uint16_t    usTxFreq;
    uint8_t     ucTxBw;
    uint8_t     ucScs;
    uint8_t     ucMcs;
    int32_t     nLatitude;
    int32_t     nLongitude;
    char        chCpuTemp;
    char        chPeriTemp;
    uint16_t    usCrc16;
}__attribute__((__packed__)) MULTI_MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_TX;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_RX
* @param unType         the type of modem unit package (58223)
* @param usLenth        the length
* @param ucDevType      the device type obu_modem:10, obu:11, rsu_modem:20, rsu:21, rsu_control:22
* @param ucStatus       the ID Tx (0) / Rx (1)
* @param unDevId        the device id of obu modem
* @param usHwVer        the hardware version of obu modem
* @param usSwVer        the software version of obu modem
* @param ulTimeStamp    the Rx timestamp of obu modem
* @param nRssi          the receive signal strength indication (dBm)
* @param ucRcpi         the receive channel power indicator [0~100] 0 : cRssi < -100 dBm, 1~99 : (cRssi + 101), 100 : cRssi >= -25 dBm
* @param nLatitude      the latitide of obu modem
* @param nLongitude     the longtitude of obu modem
* @param chCpuTemp      the CPU temperature of inside
* @param chPeriTemp     the CPU temeprature of outside by using temperature sensors if it is available
* @param usCrc          the CRC16
*/
typedef struct MULTI_MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_RX_t
{
    uint32_t    unType;
    uint16_t    usLenth;
    uint8_t     ucDevType;
    uint8_t     ucStatus;
    uint32_t    unDevId;
    uint16_t    usHwVer;
    uint16_t    usSwVer;
    uint64_t    ulTimeStamp;
    int8_t      nRssi;
    uint8_t     ucRcpi;
    int32_t     nLatitude;
    int32_t     nLongitude;
    char        chCpuTemp;
    char        chPeriTemp;
    uint16_t    usCrc16;
}__attribute__((__packed__)) MULTI_MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_RX;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_TLVC_COMM_UNIT
* @param unType         the type of modem unit package (58223)
* @param usLenth        the length
* @param ucDevType      the device type obu_modem:10, obu:11, rsu_modem:20, rsu:21, rsu_control:22
* @param ucStatus       the status of Tx (0) / Rx (1)
* @param unDevId        the device id of obu modem
* @param usHwVer        the hardware version of obu modem
* @param usSwVer        the software version of obu modem
* @param ulTimeStamp    the Rx timestamp of obu modem
* @param chCpuTemp      the CPU temperature of inside
* @param chPeriTemp     the CPU temeprature of outside by using temperature sensors if it is available
* @param usCrc          the CRC16
*/
typedef struct MULTI_MSG_MANAGER_EXT_MSG_TLVC_COMM_UNIT_t
{
    uint32_t    unType;
    uint16_t    usLenth;
    uint8_t     ucDevType;
    uint8_t     ucStatus;
    uint32_t    unDevId;
    uint16_t    usHwVer;
    uint16_t    usSwVer;
    uint64_t    ulTimeStamp;
    char        chCpuTemp;
    char        chPeriTemp;
    uint16_t    usCrc16;
}__attribute__((__packed__)) MULTI_MSG_MANAGER_EXT_MSG_TLVC_COMM_UNIT;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_TLVC_CONTROL_UNIT
* @param unType         the type of modem unit package (58223)
* @param usLenth        the length
* @param ucDevType      the device type obu_modem:10, obu:11, rsu_modem:20, rsu:21, rsu_control:22
* @param ucStatus       the ID Tx (0) / Rx (1)
* @param unDevId        the device id of obu modem
* @param usHwVer        the hardware version of obu modem
* @param usSwVer        the software version of obu modem
* @param ulTimeStamp    the Rx timestamp of obu modem
* @param chCpuTemp      the CPU temperature of inside
* @param chPeriTemp     the CPU temeprature of outside by using temperature sensors if it is available
* @param usCrc          the CRC16
*/
typedef struct MULTI_MSG_MANAGER_EXT_MSG_TLVC_CONTROL_UNIT_t
{
    uint32_t    unType;
    uint16_t    usLenth;
    uint8_t     ucDevType;
    uint8_t     ucStatus;
    uint32_t    unDevId;
    uint16_t    usHwVer;
    uint16_t    usSwVer;
    uint64_t    ulTimeStamp;
    char        chCpuTemp;
    char        chPeriTemp;
    uint16_t    usCrc16;
}__attribute__((__packed__)) MULTI_MSG_MANAGER_EXT_MSG_TLVC_CONTROL_UNIT;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_FTP_CONN_INFO_REQ
* @param unPsid         the PSID : 58240
* @param ucUnitId       the unit ID
                        . 01h : OBU modem unit – 1 (ettifos)
                        . 02h : OBU communication unit
                        . 04h : OBU control unit
                        . 08h : OBU modem unit – 2 (autotalks)
                        . 10h : RSU modem unit – 1 (ettifos)
                        . 20h : RSU communication unit
                        . 40h : RSU control unit
                        . 80h : RSU modem unit – 2 (autotalks)
* @param unLinkId       the identification ID of the unit that sent the FTP connection information request
                        4 byte random number generated by unit
* @param usCrc          the CRC16 (TODO)
*/
typedef struct MULTI_MSG_MANAGER_EXT_MSG_FTP_CONN_INFO_REQ_t
{
	uint32_t	unPsid;
	uint8_t		ucUnitId;
	uint32_t	unLinkId;
//	uint16_t	usCrc;
}__attribute__((__packed__)) MULTI_MSG_MANAGER_EXT_MSG_FTP_CONN_INFO_REQ;

/**
* @details MULTI_MSG_MANAGER_EXT_MSG_FTP_CONN_INFO_RESP
* @param unPsid         the
* @param ucUnitId       the same as MSG_MANAGER_EXT_MSG_FTP_CONN_INFO_REQ
* @param unLinkId       the same as MSG_MANAGER_EXT_MSG_FTP_CONN_INFO_REQ
* @param unIpAddr       the FTP server address
* @param usPort         the FTP server port
* @param chData         the NULL data of between ID and Password string
* @param usCrc          the CRC16 (TODO)
*/
typedef struct MULTI_MSG_MANAGER_EXT_MSG_FTP_CONN_INFO_RESP_t
{
	uint32_t	unPsid;
	uint8_t		ucUnitId;
	uint32_t	unLinkId;
	uint32_t	unIpAddr;
	uint16_t	usPort;
	char		chData[];
	//uint16_t	usCrc;
}__attribute__((__packed__)) MULTI_MSG_MANAGER_EXT_MSG_FTP_CONN_INFO_RESP;


/**
* @details MULTI_MSG_MANAGER_TX_T
* @param ePayloadType
* @param eCommType
* @param eSignId
* @param eV2xFreq
* @param eV2xDataRate
* @param eV2xTimeSlot
* @param unPsid
* @param nTxPower
* @param unTxCount
* @param uchPeerMacAddr
* @param unTransmitterProfileId
* @param unPeerL2Id
* @param unReserved
*/
typedef struct MULTI_MSG_MANAGER_TX_t {
    MULTI_MSG_MANAGER_PAYLOAD_TYPE_E      ePayloadType;
    MULTI_MSG_MANAGER_COMM_TYPE_E         eCommType;
    MULTI_MSG_MANAGER_SIGN_ID_E           eSignId;
    MULTI_MSG_MANAGER_V2X_FREQ_E          eV2xFreq;
    MULTI_MSG_MANAGER_PRIORITY_E          ePriority;
    MULTI_MSG_MANAGER_V2X_DATA_RATE_E     eV2xDataRate;
    MULTI_MSG_MANAGER_V2X_TIME_SLOT_E     eV2xTimeSlot;
    uint32_t                              unPsid;
    int8_t                                nTxPower;
    uint32_t                              unTxCount;
    uint32_t                              unTxDelay;
    uint8_t                               uchPeerMacAddr[MULTI_MSG_MANAGER_MAC_LENGTH];
    uint32_t                              unTransmitterProfileId;
    uint32_t                              unPeerL2Id;
    uint32_t                              unReserved;
} MULTI_MSG_MANAGER_TX_T;

typedef struct MULTI_MSG_MANAGER_RX_t {
    uint32_t                        unDelayTime;
    uint32_t                        unVerifiedTxRxCrc32;
    uint32_t                        unReserved;
} MULTI_MSG_MANAGER_RX_T;

/**
* @details MULTI_MSG_MANAGER_TX_EVENT_MSG_T
* @param pstMultiMsgManagerTx
* @param pstDbV2x
* @param pPayload
*/
typedef struct MULTI_MSG_MANAGER_TX_EVENT_MSG_t {
    MULTI_MSG_MANAGER_TX_T          *pstMultiMsgManagerTx;
    DB_V2X_T                        *pstDbV2x;
    void                            *pPayload;
} MULTI_MSG_MANAGER_TX_EVENT_MSG_T;

/**
* @details MULTI_MSG_MANAGER_RX_EVENT_MSG_T
* @param pstMultiMsgManagerRx
* @param pstDbV2x
* @param pPayload
*/
typedef struct MULTI_MSG_MANAGER_RX_EVENT_MSG_t {
    MULTI_MSG_MANAGER_RX_T          *pstMultiMsgManagerRx;
    DB_V2X_T                        *pstDbV2x;
    void                            *pPayload;
} MULTI_MSG_MANAGER_RX_EVENT_MSG_T;

typedef struct MULTI_MSG_MANAGER_t {
    DB_V2X_DEVICE_TYPE_E                   eDeviceType;
    char                                  *pchIfaceName;
#if defined(CONFIG_OBU_MAX_DEV)
    char                                  *pchIpAddr[MULTI_MSG_MANAGER_MAX_IP_COUNT];
    uint32_t                              unIpCount;
#else
    char                                  *pchIpAddr;
#endif
    uint32_t                              unPort;
    LOG_TYPE_E                            eLogType;
    LOG_LEVEL_E                           eLogLevel;
    bool                                  bLogLevel;
    MULTI_MSG_MANAGER_EXT_MSG_WSR         stExtMultiMsgWsr;
    uint32_t                              unMaxDevCnt;
    uint32_t                              unReserved;
} MULTI_MSG_MANAGER_T;

/***************************** Function Protype ******************************/

int32_t MULTI_MSG_MANAGER_Transmit(MULTI_MSG_MANAGER_TX_T *pstMultiMsgMgrTx, DB_V2X_T *pstDbV2x, void *pPayload);
int32_t MULTI_MSG_MANAGER_Receive(MULTI_MSG_MANAGER_RX_T *pstMultiMsgMgrRx, DB_V2X_T *pstDbV2x, void *pPayload);

int32_t MULTI_MSG_MANAGER_SetLog(MULTI_MSG_MANAGER_T *pstMultiMsgManager);

int32_t MULTI_MSG_MANAGER_Open(MULTI_MSG_MANAGER_T *pstMultiMsgManager);
int32_t MULTI_MSG_MANAGER_Close(MULTI_MSG_MANAGER_T *pstMultiMsgManager);
int32_t MULTI_MSG_MANAGER_Start(MULTI_MSG_MANAGER_T *pstMultiMsgManager);
int32_t MULTI_MSG_MANAGER_Stop(MULTI_MSG_MANAGER_T *pstMultiMsgManager);
int32_t MULTI_MSG_MANAGER_Status(MULTI_MSG_MANAGER_T *pstMultiMsgManager);

int32_t MULTI_MSG_MANAGER_Init(MULTI_MSG_MANAGER_T *pstMultiMsgManager);
int32_t MULTI_MSG_MANAGER_DeInit(MULTI_MSG_MANAGER_T *pstMultiMsgManager);

#endif	/* _MULTI_MSG_MANAGER_H_ */

