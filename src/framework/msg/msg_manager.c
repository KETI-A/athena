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
* @file msg_manager.c
*
* This file contains a data format design
*
* @note
*
* V2X Data Format Message Source File
*
* MODIFICATION HISTORY:
* Ver   Who  Date     Changes
* ----- ---- -------- ----------------------------------------------------
* 1.00  bman  23.04.07 First release
*
******************************************************************************/

/***************************** Include ***************************************/
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "framework.h"
#include "msg_manager.h"
#include "db_manager.h"
#include "time_manager.h"
#include <sys/time.h>
#include "svc_cp.h"

#include "v2x_defs.h"
#include "v2x_ext_type.h"
#include "v2x_app_ext.h"
#include "nr_v2x_interface.h"

#include "cli_util.h"

/***************************** Definition ************************************/
#define SAMPLE_V2X_API_VER                  (0x0001)
#define SAMPLE_V2X_IP_ADDR                  ("192.168.1.11")
#define SAMPLE_V2X_MSG_LEN                  (100)
#define SAMPLE_V2X_PORT_ADDR                (47347)

#define MSG_MANAGER_EXT_MSG_HEADER_SIZE     (sizeof(MSG_MANAGER_EXT_MSG))
#define MSG_MANAGER_EXT_MSG_TX_SIZE         (sizeof(MSG_MANAGER_EXT_MSG_TX))
#define MSG_MANAGER_EXT_MSG_RX_SIZE         (sizeof(MSG_MANAGER_EXT_MSG_RX))
#define MSG_MANAGER_EXT_TLVC_SIZE           (sizeof(MSG_MANAGER_EXT_MSG_TLVC))
#define MSG_MANAGER_EXT_WSC_SIZE            (sizeof(MSG_MANAGER_EXT_MSG_WSC) + MSG_MANAGER_EXT_MSG_HEADER_SIZE)
#define MSG_MANAGER_EXT_WSR_SIZE            (sizeof(MSG_MANAGER_EXT_MSG_WSR) + MSG_MANAGER_EXT_MSG_HEADER_SIZE)

#define MSG_MANAGER_MAX_DATA_SIZE			(8999) /* RAW Message Size */
#define MSG_MANAGER_MAX_TX_PAYLOAD			(MSG_MANAGER_MAX_DATA_SIZE)
#define MSG_MANAGER_MAX_RX_PAYLOAD			(MSG_MANAGER_MAX_RX_PAYLOAD)

#define MSG_MANAGER_MAX_TX_PKG_SIZE			(MSG_MANAGER_MAX_DATA_SIZE + MSG_MANAGER_EXT_MSG_HEADER_SIZE + MSG_MANAGER_EXT_MSG_TX_SIZE + MSG_MANAGER_CRC16_LEN)
#define MSG_MANAGER_MAX_RX_PKG_SIZE			(MSG_MANAGER_MAX_DATA_SIZE + MSG_MANAGER_EXT_MSG_HEADER_SIZE + MSG_MANAGER_EXT_MSG_RX_SIZE + MSG_MANAGER_CRC16_LEN)

#define MSG_MGR_RSU_LISTENQ                 (1024)

//#define CONFIG_TEMP_OBU_TEST (1)
//#define CONFIG_TEST_EXT_MSG_STATUS_PKG (1)

#ifdef WORDS_BIGENDIAN
#define htonll(x)   (x)
#define ntohll(x)   (x)
#else
#define htonll(x)   ((((uint64_t)htonl(x)) << 32) + htonl(x >> 32))
#define ntohll(x)   ((((uint64_t)ntohl(x)) << 32) + ntohl(x >> 32))
#endif

/***************************** Static Variable *******************************/
static int32_t s_nSocketHandle = -1;
static int s_nDbTaskMsgId, s_nMsgTxTaskMsgId, s_nMsgRxTaskMsgId;
static key_t s_dbTaskMsgKey = FRAMEWORK_DB_TASK_MSG_KEY;
static key_t s_MsgTxTaskMsgKey = FRAMEWORK_MSG_TX_TASK_MSG_KEY;
static key_t s_MsgRxTaskMsgKey = FRAMEWORK_MSG_RX_TASK_MSG_KEY;

static pthread_t sh_msgMgrTxTask;
static pthread_t sh_msgMgrRxTask;

static bool s_bMsgMgrLog = OFF;
static bool s_bFirstPacket = TRUE;

static uint32_t s_unV2xMsgTxLen = 0, s_unV2xMsgRxLen = 0;

/***************************** Function  *************************************/

/////////////////////////////////////////////////////////////////////////////////////////
/* Global Variable Value */
V2xAction_t e_action_g = eV2xAction_ADD;
V2xPayloadType_t e_payload_type_g = eRaw;
V2xPsid_t psid_g = 5271;
V2XCommType_t e_comm_type_g = eV2XCommType_5GNRV2X;
V2xPowerDbm_t tx_power_g = 20;
V2xSignerId_t e_signer_id_g = eV2xSignerId_UNSECURED;
V2xMsgPriority_t e_priority_g = eV2xPriority_CV2X_PPPP_0;
uint32_t tx_cnt_g = 100;
uint32_t tx_delay_g = 100;
V2xFrequency_t freq_g = 5900;
V2xDataRate_t e_data_rate_g = eV2xDataRate_6MBPS;
V2xTimeSlot_t e_time_slot_g = eV2xTimeSlot_Continuous;
uint8_t peer_mac_addr_g[MAC_EUI48_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint32_t transmitter_profile_id_g = 100;
uint32_t peer_l2id_g = 0;

#if defined(CONFIG_TEMP_OBU_TEST)
uint32_t delay_time_sec_g = 10;
#endif

static void P_MSG_NABAGER_PrintMsgInfo(int msqid)
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

static int32_t P_MSG_MANAGER_ConnectObu(MSG_MANAGER_T *pstMsgManager)
{
    int32_t nRet = FRAMEWORK_ERROR;
    int32_t nSocketHandle = -1;
    int32_t nFlags = 0;

    if (pstMsgManager == NULL)
    {
        PrintError("pstMsgManager is NULL!");
        return nRet;
    }

    nSocketHandle = socket(AF_INET, SOCK_STREAM, 0);
    if (nSocketHandle < 0)
    {
        PrintError("socket() is failed!!");
        nRet = FRAMEWORK_ERROR;
        return nRet;
    }

    if (pstMsgManager->pchIfaceName == NULL)
    {
        PrintError("pstMsgManager->pchIfaceName is NULL!");
        return nRet;
    }

    PrintDebug("pstMsgManager->pchIfaceName[%s]", pstMsgManager->pchIfaceName);

    if (pstMsgManager->pchIpAddr == NULL)
    {
        PrintError("pstMsgManager->pchIpAddr is NULL!");
        return nRet;
    }

    PrintDebug("pchIpAddr[%s]", pstMsgManager->pchIpAddr);
    PrintDebug("unPort[%d]", pstMsgManager->unPort);

    nRet = setsockopt(nSocketHandle, SOL_SOCKET, SO_BINDTODEVICE, pstMsgManager->pchIfaceName, strlen(pstMsgManager->pchIfaceName));
    if (nRet < 0)
    {
        PrintError("setsockopt() is failed");
        return nRet;
    }

    struct sockaddr_in server_addr =
    {
        .sin_family = AF_INET,
        .sin_addr.s_addr = inet_addr(pstMsgManager->pchIpAddr),
        .sin_port = htons(pstMsgManager->unPort)
    };

    nRet = connect(nSocketHandle, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (nRet < 0)
    {
        PrintError("connect() is failed");
        return nRet;
    }

    /* Change to NON-BLOCK socket */
    nFlags = fcntl(nSocketHandle, F_GETFL, 0);
    if (nFlags == -1)
    {
        PrintError("fcntl() is F_GETFL failed");
        return nRet;
    }

    nFlags |= O_NONBLOCK;
    nRet = fcntl(nSocketHandle, F_SETFL, nFlags);
    if (nRet < 0)
    {
        PrintError("fcntl() is F_SETFL failed");
        return nRet;
    }

    s_nSocketHandle = nSocketHandle;

    PrintTrace("Connection of V2X Device is successed! [s_nSocketHandle:0x%x]", s_nSocketHandle);

    nRet = FRAMEWORK_OK;

    return nRet;
}

static int32_t P_MSG_MANAGER_ConnectRsu(MSG_MANAGER_T *pstMsgManager)
{
    int32_t nRet = FRAMEWORK_ERROR;
    int32_t nSocketHandle = -1;
    int32_t nClientSocket = -1;
    int nVal = 1;

    struct sockaddr_in stServerAddr, stClientAddr;
    socklen_t stClientLen = sizeof(stClientAddr);

    if (pstMsgManager == NULL)
    {
        PrintError("pstMsgManager is NULL!");
        return nRet;
    }

    if((nSocketHandle = socket(AF_INET , SOCK_STREAM , 0)) == -1)
    {
        PrintError("socket error.\n");
        return nRet;
    }

    bzero(&stServerAddr, sizeof(stServerAddr));

    stServerAddr.sin_family = AF_INET;
    stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    stServerAddr.sin_port = htons(pstMsgManager->unPort);

    if (setsockopt(nSocketHandle, SOL_SOCKET, SO_REUSEADDR, (char *) &nVal, sizeof nVal) < 0) {
        PrintError("setsockopt() is failed!");
        close(nSocketHandle);
        return nRet;
    }

    if(bind(nSocketHandle, (struct sockaddr *)&stServerAddr, sizeof(stServerAddr)) < 0)
    {
        PrintError("bind() is failed!");
        return nRet;
    }

    if(listen(nSocketHandle, MSG_MGR_RSU_LISTENQ) < 0)
    {
        PrintError("listen() is failed!");
        return nRet;
    }

    PrintWarn("Listening to the client of RSU controller.");

    if((nClientSocket = accept(nSocketHandle, (struct sockaddr *)&stClientAddr, &stClientLen)) < 0)
    {
        PrintError("accept() is failed!");
        return nRet;
    }

    PrintTrace("server: got connection from [IP: %s, Port: %d]", inet_ntoa(stClientAddr.sin_addr), ntohs(stClientAddr.sin_port));


    s_nSocketHandle = nClientSocket;

    PrintTrace("[s_nSocketHandle: 0x%x]", s_nSocketHandle);

    nRet = FRAMEWORK_OK;

    return nRet;
}

static int32_t P_MSG_MANAGER_ConnectV2XDevice(MSG_MANAGER_T *pstMsgManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if (pstMsgManager == NULL)
    {
        PrintError("pstMsgManager is NULL!");
        return nRet;
    }

    switch(pstMsgManager->eDeviceType)
    {
        case DB_V2X_DEVICE_TYPE_OBU:
        {
            PrintTrace("DB_V2X_DEVICE_TYPE_OBU");
            nRet = P_MSG_MANAGER_ConnectObu(pstMsgManager);
            if (nRet != FRAMEWORK_OK)
            {
                PrintError("P_MSG_MANAGER_ConnectObu() is failed![%d],", nRet);
                return nRet;
            }
            break;
        }

        case DB_V2X_DEVICE_TYPE_RSU:
        {
            PrintTrace("DB_V2X_DEVICE_TYPE_RSU");
            nRet = P_MSG_MANAGER_ConnectRsu(pstMsgManager);
            if (nRet != FRAMEWORK_OK)
            {
                PrintError("P_MSG_MANAGER_ConnectRsu() is failed![%d],", nRet);
                return nRet;
            }
            break;
        }

        default:
            PrintError("Error! unknown device type[%d]", pstMsgManager->eDeviceType);
            break;

    }

    return nRet;
}

static int32_t P_MSG_MANAGER_DisconnectV2XDevice(void)
{
    int32_t nRet = FRAMEWORK_ERROR;

	if (s_nSocketHandle >= 0)
	{
		close(s_nSocketHandle);
        s_nSocketHandle = -1;
        PrintTrace("s_nSocketHandle is closed, s_nSocketHandle[%d]", s_nSocketHandle);
        nRet = FRAMEWORK_OK;
	}
    else
    {
        PrintError("s_nSocketHandle is not available!!");
    }

    return nRet;
}

#if defined(CONFIG_EXT_DATA_FORMAT)
void P_MSG_MANAGER_PrintMsgData(unsigned char* ucMsgData, int nLength)
{
    int i;
    char cMsgBuf[MSG_MANAGER_MSG_BUF_MAX_LEN], cHexStr[MSG_MANAGER_MSG_HEX_STR_LEN];

    if (s_bMsgMgrLog == TRUE)
    {
        PrintTrace("===============================================================");
        PrintDebug("length [0x%x, %d] bytes", nLength, nLength);
        PrintDebug("---------------------------------------------------------------");
        PrintDebug("Hex.   00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F");
        PrintDebug("---------------------------------------------------------------");

        for(i = 0; i < nLength; i++)
        {
            if((i % MSG_MANAGER_MSG_HEX_SIZE) == 0)
            {
                if (i == 0)
                {
                    sprintf(cMsgBuf, "%03X- : ", (i/MSG_MANAGER_MSG_HEX_SIZE));
                }
                else
                {
                    printf("%s\n", cMsgBuf);
                    sprintf(cMsgBuf, "%03X- : ", (i/MSG_MANAGER_MSG_HEX_SIZE));
                }
            }

            sprintf(cHexStr, "%02X ", ucMsgData[i]);
            strcat(cMsgBuf, cHexStr);
        }

        PrintDebug("%s", cMsgBuf);
        PrintTrace("===============================================================\n");
    }
}

int32_t P_MSG_MANAGER_SetV2xWsrSetting(MSG_MANAGER_T *pstMsgManager)
{
	int32_t nRet = FRAMEWORK_ERROR;
	int nRxLen = 0, nTxLen = 0;
	uint8_t ucTxMsgBuf[MSG_MANAGER_MAX_TX_PKG_SIZE];
	uint8_t ucRxMsgBuf[MSG_MANAGER_MAX_RX_PKG_SIZE];
    uint16_t *pusCrc16 = NULL;
	MSG_MANAGER_EXT_MSG *pstTxMsgHdr = (MSG_MANAGER_EXT_MSG *)ucTxMsgBuf;
	MSG_MANAGER_EXT_MSG *pstRxMsgHdr = (MSG_MANAGER_EXT_MSG *)ucRxMsgBuf;

    MSG_MANAGER_EXT_MSG_WSR* pstWsr = (MSG_MANAGER_EXT_MSG_WSR*)pstTxMsgHdr->ucPayload;
    MSG_MANAGER_EXT_MSG_WSC* pstWsc = (MSG_MANAGER_EXT_MSG_WSC*)pstRxMsgHdr->ucPayload;

	memset(ucTxMsgBuf, 0, sizeof(ucTxMsgBuf));
	memset(ucRxMsgBuf, 0, sizeof(ucRxMsgBuf));

    PrintWarn("Magic Number Name [%s], WSR length[%d]", MSG_MANAGER_EXT_MSG_MAGIC_NUM_NAME, (int)sizeof(MSG_MANAGER_EXT_WSR_SIZE));

    memcpy(pstTxMsgHdr->cMagicNumber, MSG_MANAGER_EXT_MSG_MAGIC_NUM_NAME, sizeof(pstTxMsgHdr->cMagicNumber));

    pstTxMsgHdr->usLength = htons(MSG_MANAGER_EXT_WSR_SIZE - 6); // 6??
    pstTxMsgHdr->usSeqNum = 0;
    pstTxMsgHdr->usPayloadId = htons(eMSG_MANAGER_EXT_MSG_PAYLOAD_ID_WSM_SVC_REQ);

    pstWsr->ucAction = pstMsgManager->stExtMsgWsr.ucAction;

    pstWsr->unPsid = htonl(pstMsgManager->stExtMsgWsr.unPsid);
    nTxLen = SIZE_WSR_DATA;

    pusCrc16 = (uint16_t*)&ucTxMsgBuf[nTxLen - sizeof(pstWsr->usCrc16)];
    pstWsr->usCrc16 = CLI_UTIL_GetCrc16(ucTxMsgBuf + SIZE_MAGIC_NUMBER_OF_HEADER, nTxLen - sizeof(pstTxMsgHdr->cMagicNumber) - sizeof(pstWsr->usCrc16));
    *pusCrc16 = htons(pstWsr->usCrc16);

    PrintDebug("Action ID[%s], PSID[%u]", (pstMsgManager->stExtMsgWsr.ucAction == eMSG_MANAGER_EXT_MSG_ACTION_ADD) ? "ADD":"DEL", pstMsgManager->stExtMsgWsr.unPsid);

    PrintEnter("\nWSM Service REQ>\n"
           "  cMagicNumber   : %s\n"
           "  usLength       : %d\n"
           "  usSeqNum       : %d\n"
           "  usPayloadId    : 0x%x\n"
           "  ucAction       : %d\n"
           "  psid           : %d\n"
           "  crc16          : %d\n",
           pstTxMsgHdr->cMagicNumber,
           ntohs(pstTxMsgHdr->usLength),
           ntohs(pstTxMsgHdr->usSeqNum),
           ntohs(pstTxMsgHdr->usPayloadId),
           pstWsr->ucAction,
           ntohl(pstWsr->unPsid),
           ntohs(pstWsr->usCrc16));

    P_MSG_MANAGER_PrintMsgData(ucTxMsgBuf, nTxLen);
    nRxLen = send(s_nSocketHandle, ucTxMsgBuf, nTxLen, 0);
	if ((nRxLen < 0) || (nRxLen == 0))
	{
		PrintError("send() is failed!!");
		return nRet;
	}
    else if (nRxLen != nTxLen)
    {
        PrintError("send() sent a different number of bytes than expected\n");
    }
    else
    {
        PrintDebug("successfully request WSR to OBU");
    }

    nRxLen = -1;

	while (nRxLen <= 0)
	{
		nRxLen = recv(s_nSocketHandle, &ucRxMsgBuf, sizeof(ucRxMsgBuf), 0);
		if (nRxLen < 0)
		{
			if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
			{
				PrintError("recv() is failed!!");
				break;
			}
		}
		else if (nRxLen == 0)
		{
			PrintError("recv()'connection is closed by peer!!");
		}
        else
        {
            P_MSG_MANAGER_PrintMsgData(ucRxMsgBuf, nRxLen);

            PrintExit("\nWSM Service RESP>\n"
                   "  cMagicNumber   : %s\n"
                   "  usLength       : %d\n"
                   "  usSeqNum       : %d\n"
                   "  usPayloadId    : 0x%x\n"
                   "  ucActionRst    : %d\n"
                   "  psid           : %d\n",
                   pstRxMsgHdr->cMagicNumber,
                   ntohs(pstRxMsgHdr->usLength),
                   ntohs(pstRxMsgHdr->usSeqNum),
                   ntohs(pstRxMsgHdr->usPayloadId),
                   pstWsc->ucActionRst,
                   ntohl(pstWsc->unPsid));

            if(ntohs(pstRxMsgHdr->usPayloadId) != eMSG_MANAGER_EXT_MSG_PAYLOAD_ID_WSM_SVC_CONFIRM)
            {
                PrintError("Error! payload ID is not matched[0x%x]", ntohs(pstRxMsgHdr->usPayloadId));

                if(ntohs(pstWsc->ucActionRst) == eMSG_MANAGER_EXT_MSG_WSC_ACTION_FAIL)
                {
                    PrintError("WSR failure [%d]", ntohs(pstWsc->ucActionRst));
                }

                nRet = FRAMEWORK_ERROR;
            }
            else
            {
                PrintTrace("PSID[%d] is successfully registered [WSR Action:0x%x]", pstMsgManager->stExtMsgWsr.unPsid, pstWsc->ucActionRst);
                nRet = FRAMEWORK_OK;
            }
        }
	}

    /* Temp : Will be Fixed by OBU */
    nRet = FRAMEWORK_OK;

	return nRet;
}
#else
int32_t P_MSG_MANAGER_SetV2xWsrSetting(void)
{
	int32_t nRet = FRAMEWORK_ERROR;

	// Prepare the Ext_WSReq_t structure
	Ext_WSReq_t ws_req;
	memset(&ws_req, 0, sizeof(ws_req));
	ws_req.magic_num = htons(MAGIC_WSREQ);
	ws_req.ver = htons(SAMPLE_V2X_API_VER);
	ws_req.e_action = eV2xAction_ADD;
	ws_req.e_payload_type = e_payload_type_g;
	ws_req.psid = htonl(psid_g);

	printf("\nWSM Service REQ>>\n"
		   "  magic_num        : 0x%04X\n"
		   "  ver              : 0x%04X\n"
		   "  e_action         : %d\n"
		   "  e_payload_type   : %d\n"
		   "  psid             : %u\n",
		   ntohs(ws_req.magic_num),
		   ntohs(ws_req.ver),
		   ws_req.e_action,
		   ws_req.e_payload_type,
		   ntohl(ws_req.psid));

	// Send the request
	ssize_t n = send(s_nSocketHandle, &ws_req, sizeof(ws_req), 0);
	if (n < 0)
	{
		PrintError("send() is failed!!");
		return nRet;
	}
	else if (n != sizeof(ws_req))
	{
		PrintError("send() sent a different number of bytes than expected!");
		return nRet;
	}

	// Wait for the response
	Ext_WSResp_t ws_resp;
	memset(&ws_resp, 0, sizeof(ws_resp));
	n = -1;

	while (n <= 0)
	{
		n = recv(s_nSocketHandle, &ws_resp, sizeof(ws_resp), 0);
		if (n < 0)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				PrintError("recv() is failed!!");
				break;
			}
		}
		else if (n == 0)
		{
			PrintError("recv()'connection is closed by peer!!");
		}
		else if (n != sizeof(ws_resp))
		{
			PrintError("recv() is received a different number of bytes than expected!!");
		}
        else
        {
            nRet = FRAMEWORK_OK;
            PrintTrace("recv() is success to get ws_resp");
        }

		usleep(1000);
	}

	PrintDebug("\nWSM Service RESP>>\n"
		   "  magic_num      : 0x%04X\n"
		   "  ver            : 0x%04X\n"
		   "  e_action       : %d\n"
		   "  is_confirmed   : %d\n"
		   "  psid           : %u\n",
		   ntohs(ws_resp.magic_num),
		   ntohs(ws_resp.ver),
		   ws_resp.e_action,
		   ws_resp.is_confirmed,
		   ntohl(ws_resp.psid));

	return nRet;
}
#endif

static int32_t P_MSG_MANAGER_SendTxMsgToDbMgr(MSG_MANAGER_TX_EVENT_MSG_T *pstEventMsg, uint32_t unCrc32)
{
    int32_t nRet = FRAMEWORK_ERROR;
    DB_MANAGER_WRITE_T stDbManagerWrite;
    DB_MANAGER_EVENT_MSG_T stEventMsg;
    DB_MANAGER_T *pstDbManager;

    (void*)memset(&stDbManagerWrite, 0x00, sizeof(DB_MANAGER_WRITE_T));

    pstDbManager = FRAMEWORK_GetDbManagerInstance();
    if(pstDbManager == NULL)
    {
        PrintError("FRAMEWORK_GetDbManagerInstance() is failed!! pstDbManager is NULL");
        return nRet;
    }

    stDbManagerWrite.eFileType = pstDbManager->eFileType;
    stDbManagerWrite.eCommMsgType = DB_MANAGER_COMM_MSG_TYPE_TX;
    stDbManagerWrite.eProc = DB_MANAGER_PROC_WRITE;
    stDbManagerWrite.unCrc32 = unCrc32;

    stEventMsg.pstDbManagerWrite = &stDbManagerWrite;
    stEventMsg.pstDbV2x = pstEventMsg->pstDbV2x;

    /* free at P_DB_MANAGER_WriteXXX() */
    stEventMsg.pPayload = malloc(pstEventMsg->pstDbV2x->ulPayloadLength);
    if(stEventMsg.pPayload == NULL)
    {
        PrintError("malloc() is failed! [NULL]");
        return nRet;
    }

    memcpy(stEventMsg.pPayload, pstEventMsg->pPayload, pstEventMsg->pstDbV2x->ulPayloadLength);

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

#if defined(CONFIG_EXT_DATA_FORMAT)
#if defined(CONFIG_TEST_EXT_MSG_STATUS_PKG)
static int32_t P_MSG_MANAGER_CreateStatusPkg(MSG_MANAGER_EXT_MSG_TLVC_OVERALL *pstExtMsgOverall, MSG_MANAGER_EXT_MSG_STATUS_E eStatus)
{
    int32_t nRet = FRAMEWORK_ERROR;

	MSG_MANAGER_EXT_MSG_TLVC_COMM_UNIT *pstCommUnit;
	struct timeval stTvUsec;
	struct tm *pstTm;
	uint64_t ulSysTime;
	uint16_t usPkgLen = ntohs(pstExtMsgOverall->usLenOfPkg);

	pstCommUnit  = (MSG_MANAGER_EXT_MSG_TLVC_COMM_UNIT *)((uint8_t*)pstExtMsgOverall + sizeof(MSG_MANAGER_EXT_MSG_TLVC_OVERALL) + usPkgLen);

	pstExtMsgOverall->ucNumOfPkg++;
	usPkgLen += sizeof(MSG_MANAGER_EXT_MSG_TLVC_COMM_UNIT);
	pstExtMsgOverall->usLenOfPkg = htons(usPkgLen);
	pstExtMsgOverall->usCrc16 = htons(CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgOverall, sizeof(MSG_MANAGER_EXT_MSG_TLVC_OVERALL) - MSG_MANAGER_CRC16_LEN));

	pstCommUnit->unType = htonl(MSG_MANAGER_EXT_MSG_STATUS_PKG);
	pstCommUnit->usLenth = htons(sizeof(MSG_MANAGER_EXT_MSG_TLVC_COMM_UNIT) - 6);
	pstCommUnit->ucDevType = eMSG_MANAGER_EXT_MSG_DEV_TYPE_OBU;
	pstCommUnit->ucStatus = eStatus;
	pstCommUnit->unDevId = htonl(1);
	pstCommUnit->usHwVer = htons(2);
	pstCommUnit->usSwVer = htons(3);

	gettimeofday(&stTvUsec, NULL);
	stTvUsec.tv_sec = stTvUsec.tv_sec + (3600 * 9); /* UTC -> KST */

	pstTm = localtime(&stTvUsec.tv_sec);
	ulSysTime = (uint64_t)(pstTm->tm_year + 1900) * 1000000000000000 +
				(uint64_t)(pstTm->tm_mon + 1)     * 10000000000000 +
				(uint64_t)pstTm->tm_mday        * 100000000000 +
				(uint64_t)pstTm->tm_hour        * 1000000000 +
				(uint64_t)pstTm->tm_min         * 10000000 +
				(uint64_t)pstTm->tm_sec         * 100000 +
				(uint64_t)stTvUsec.tv_usec        / 10;

	pstCommUnit->ulTimeStamp = htobe64(ulSysTime);
	pstCommUnit->usCrc16 = htons(CLI_UTIL_GetCrc16((uint8_t*)pstCommUnit, sizeof(MSG_MANAGER_EXT_MSG_TLVC_COMM_UNIT) - MSG_MANAGER_CRC16_LEN));

    nRet = FRAMEWORK_OK;

	return nRet;
}
#endif

static int32_t P_MSG_MANAGER_SendTxMsg(MSG_MANAGER_TX_EVENT_MSG_T *pstEventMsg)
{
    int32_t nRet = FRAMEWORK_ERROR;
    uint32_t unDbV2xPacketLength = sizeof(DB_V2X_T) + pstEventMsg->pstDbV2x->ulPayloadLength + sizeof(pstEventMsg->pstDbV2x->ulReserved); /* ulReserved = CRC32*/
    uint32_t unDbV2xCrcCalcuatedLength = sizeof(DB_V2X_T) + pstEventMsg->pstDbV2x->ulPayloadLength;
    ssize_t nRetSendSize = 0;
    uint32_t ulTempDbV2xTotalPacketCrc32 = 0, ulDbV2xTotalPacketCrc32 = 0;
    TIME_MANAGER_T *pstTimeManager;

    DB_V2X_T *pstDbV2x = NULL;
    uint8_t *pchDbV2xCrc = NULL;
    uint32_t unTxMsgLen;
    uint32_t unPsid = 0;
    uint8_t ucMsgBuf[MSG_MANAGER_MAX_TX_PKG_SIZE];

    memset(ucMsgBuf, 0, sizeof(ucMsgBuf));
    MSG_MANAGER_EXT_MSG* pstExtMsg = (MSG_MANAGER_EXT_MSG*)ucMsgBuf;

#if defined(CONFIG_TEST_EXT_MSG_PKG)
    int32_t nRawPkgSize = 10;
    ssize_t i = 0;
#endif
    uint16_t *pusCrc16;
    uint16_t usCalcCrc16;
    MSG_MANAGER_EXT_MSG_TX* pstExtMsgTx = (MSG_MANAGER_EXT_MSG_TX*)pstExtMsg->ucPayload;
    MSG_MANAGER_EXT_MSG_TLVC_OVERALL *pstExtMsgOverall;
#if defined(CONFIG_TEST_EXT_MSG_PKG)
    MSG_MANAGER_EXT_MSG_TLVC *pstTxPkg;
#endif
    MSG_MANAGER_EXT_MSG_SSOV *pstTxSsovPkg;
    uint16_t unExtMsgPkgLen;

    s_unV2xMsgTxLen = unDbV2xPacketLength;

    if(s_bMsgMgrLog == ON)
    {
        PrintWarn("s_unV2xMsgTxLen[%d]", s_unV2xMsgTxLen);
        PrintDebug("unDbV2xPacketLength[%d] = sizeof(DB_V2X_T)[%ld]+ulPayloadLength[%d]+sizeof(ulReserved)[%ld]", unDbV2xPacketLength, sizeof(DB_V2X_T), pstEventMsg->pstDbV2x->ulPayloadLength, sizeof(pstEventMsg->pstDbV2x->ulReserved));
    }

    pstDbV2x = malloc(unDbV2xPacketLength);
    if(pstDbV2x == NULL)
    {
        PrintError("malloc() is failed! [NULL]");
        return nRet;
    }

    memset(pstDbV2x, 0, unDbV2xPacketLength);

    pstDbV2x->eDeviceType = htons(pstEventMsg->pstDbV2x->eDeviceType);
    pstDbV2x->eTeleCommType = htons(pstEventMsg->pstDbV2x->eTeleCommType);
    pstDbV2x->unDeviceId = htonl(pstEventMsg->pstDbV2x->unDeviceId);
    pstDbV2x->ulTimeStamp = htonll(pstEventMsg->pstDbV2x->ulTimeStamp);
    pstDbV2x->eServiceId = htons(pstEventMsg->pstDbV2x->eServiceId);
    pstDbV2x->eActionType = htons(pstEventMsg->pstDbV2x->eActionType);
    pstDbV2x->eRegionId = htons(pstEventMsg->pstDbV2x->eRegionId);
    pstDbV2x->ePayloadType = htons(pstEventMsg->pstDbV2x->ePayloadType);
    pstDbV2x->eCommId = htons(pstEventMsg->pstDbV2x->eCommId);
    pstDbV2x->usDbVer = htons(pstEventMsg->pstDbV2x->usDbVer);
    pstDbV2x->usHwVer = htons(pstEventMsg->pstDbV2x->usHwVer);
    pstDbV2x->usSwVer = htons(pstEventMsg->pstDbV2x->usSwVer);
    pstDbV2x->ulPayloadLength = htonl(pstEventMsg->pstDbV2x->ulPayloadLength);
    pstDbV2x->ulReserved = htonl(pstEventMsg->pstDbV2x->ulReserved);

    if(s_bMsgMgrLog == ON)
    {
        PrintDebug("pstDbV2x->eDeviceType[%d]", ntohs(pstDbV2x->eDeviceType));
        PrintDebug("pstDbV2x->eTeleCommType[%d]", ntohs(pstDbV2x->eTeleCommType));
        PrintDebug("pstDbV2x->unDeviceId[%d]", ntohl(pstDbV2x->unDeviceId));
        PrintDebug("pstDbV2x->ulTimeStamp[%ld]", ntohll(pstDbV2x->ulTimeStamp));
        PrintDebug("pstDbV2x->eServiceId[%d]", ntohs(pstDbV2x->eServiceId));
        PrintDebug("pstDbV2x->eActionType[%d]", ntohs(pstDbV2x->eActionType));
        PrintDebug("pstDbV2x->eRegionId[%d]", ntohs(pstDbV2x->eRegionId));
        PrintDebug("pstDbV2x->ePayloadType[%d]", ntohs(pstDbV2x->ePayloadType));
        PrintDebug("pstDbV2x->eCommId[%d]", ntohs(pstDbV2x->eCommId));
        PrintDebug("pstDbV2x->usDbVer[%d.%d]", ntohs(pstDbV2x->usDbVer) >> CLI_DB_V2X_MAJOR_SHIFT, ntohs(pstDbV2x->usDbVer) & CLI_DB_V2X_MINOR_MASK);
        PrintDebug("pstDbV2x->usHwVer[%d]", ntohs(pstDbV2x->usHwVer));
        PrintDebug("pstDbV2x->usSwVer[%d]", ntohs(pstDbV2x->usSwVer));
        PrintDebug("pstDbV2x->ulPayloadLength[%d]", ntohl(pstDbV2x->ulPayloadLength));
        PrintDebug("pstDbV2xs->ulReserved[0x%x]", ntohl(pstDbV2x->ulReserved));
    }

    pchDbV2xCrc = malloc(unDbV2xPacketLength);
    if(pchDbV2xCrc == NULL)
    {
        PrintError("malloc() is failed! [NULL]");
        return nRet;
    }
    memset(pchDbV2xCrc, 0, unDbV2xCrcCalcuatedLength);
    memcpy(pchDbV2xCrc, pstDbV2x, unDbV2xPacketLength);
    memcpy(pchDbV2xCrc + sizeof(DB_V2X_T), pstEventMsg->pPayload, pstEventMsg->pstDbV2x->ulPayloadLength);

    P_MSG_MANAGER_PrintMsgData(pchDbV2xCrc, sizeof(DB_V2X_T) + pstEventMsg->pstDbV2x->ulPayloadLength);

    ulTempDbV2xTotalPacketCrc32 = CLI_UTIL_GetCrc32((uint8_t*)pchDbV2xCrc, sizeof(DB_V2X_T) + pstEventMsg->pstDbV2x->ulPayloadLength);
    ulDbV2xTotalPacketCrc32 = htonl(ulTempDbV2xTotalPacketCrc32);

    if(s_bMsgMgrLog == ON)
    {
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
            /* The average delay between svc and send() is about 10 us, so use the timestamp of svc */
            PrintDebug("[%ld]-[%ld]=[%ld]", pstTimeManager->ulTimeStamp, pstEventMsg->pstDbV2x->ulTimeStamp, pstTimeManager->ulTimeStamp-pstEventMsg->pstDbV2x->ulTimeStamp);
        }
    }

    if (pstEventMsg->pstDbV2x->eCommId == DB_V2X_COMM_ID_V2V)
    {
        unPsid = MSG_MANAGER_EXT_MSG_V2V_PSID;
    }
    else if (pstEventMsg->pstDbV2x->eCommId ==  DB_V2X_COMM_ID_V2I)
    {
        unPsid = MSG_MANAGER_EXT_MSG_V2I_PSID;
    }
    else if (pstEventMsg->pstDbV2x->eCommId == DB_V2X_COMM_ID_I2V)
    {
        unPsid = MSG_MANAGER_EXT_MSG_I2V_PSID;
    }
    else
    {
        unPsid = MSG_MANAGER_EXT_MSG_V2V_PSID;
        PrintError("unknown pstEventMsg->pstDbV2x->eCommId[%d] set PSID of V2V", pstEventMsg->pstDbV2x->eCommId);
    }

    if(s_bMsgMgrLog == ON)
    {
        PrintDebug("eCommId[%d]. unPsid[%d]", pstEventMsg->pstDbV2x->eCommId, unPsid);
    }

    pstExtMsgOverall = (MSG_MANAGER_EXT_MSG_TLVC_OVERALL*)pstExtMsgTx->ucPayload;
    pstExtMsgOverall->unType = htonl(MSG_MANAGER_EXT_MSG_OVERALL_PKG);
    pstExtMsgOverall->usLength = htons(sizeof(MSG_MANAGER_EXT_MSG_TLVC_OVERALL) - (MSG_MANAGER_EXT_MSG_MAGIC_NUM_LEN + MSG_MANAGER_CRC16_LEN));
    pstExtMsgOverall->chMagicNum[0] = MSG_MANAGER_EXT_MSG_OVERALL_PKG_EXTENSIBLE;
    pstExtMsgOverall->chMagicNum[1] = MSG_MANAGER_EXT_MSG_OVERALL_PKG_MESSAGE;
    pstExtMsgOverall->chMagicNum[2] = MSG_MANAGER_EXT_MSG_OVERALL_PKG_OVERALL;
    pstExtMsgOverall->chMagicNum[3] = MSG_MANAGER_EXT_MSG_OVERALL_PKG_PACKAGE;
    pstExtMsgOverall->ucVersion = MSG_MANAGER_EXT_MSG_OVERALL_PKG_VER;
    pstExtMsgOverall->ucNumOfPkg = 0;
    pstExtMsgOverall->usLenOfPkg = 0;
    pstExtMsgOverall->ucBitwise = 0x77;

    unExtMsgPkgLen = ntohs(pstExtMsgOverall->usLenOfPkg);

    /* SSOV Pkg */
    if (unDbV2xPacketLength > 0)
    {
        pstTxSsovPkg = (MSG_MANAGER_EXT_MSG_SSOV*)((uint8_t*)pstExtMsgOverall + sizeof(MSG_MANAGER_EXT_MSG_TLVC_OVERALL) + unExtMsgPkgLen);

        pstExtMsgOverall->ucNumOfPkg++;
        unExtMsgPkgLen = unExtMsgPkgLen + 8 + unDbV2xPacketLength;
        pstExtMsgOverall->usLenOfPkg = htons(unExtMsgPkgLen);
        pstExtMsgOverall->usCrc16 = htons(CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgOverall, sizeof(MSG_MANAGER_EXT_MSG_TLVC_OVERALL) - MSG_MANAGER_CRC16_LEN));

        pstTxSsovPkg->unType = htonl(MSG_MANAGER_EXT_MSG_SSOV_PKG);
        pstTxSsovPkg->usLength = htons(unDbV2xPacketLength + MSG_MANAGER_CRC16_LEN);

        memcpy(pstTxSsovPkg->ucPayload, pstDbV2x, unDbV2xPacketLength);
        memcpy(pstTxSsovPkg->ucPayload + sizeof(DB_V2X_T), pstEventMsg->pPayload, pstEventMsg->pstDbV2x->ulPayloadLength);
        memcpy(pstTxSsovPkg->ucPayload + sizeof(DB_V2X_T) + pstEventMsg->pstDbV2x->ulPayloadLength, &ulDbV2xTotalPacketCrc32, sizeof(uint32_t));

        pusCrc16 = (uint16_t*)((uint8_t*)pstTxSsovPkg + (sizeof(pstTxSsovPkg->unType) + sizeof(pstTxSsovPkg->usLength)) + unDbV2xPacketLength);
        *pusCrc16 = htons(CLI_UTIL_GetCrc16((uint8_t*)pstTxSsovPkg, sizeof(pstTxSsovPkg->unType) + sizeof(pstTxSsovPkg->usLength) + unDbV2xPacketLength));

        if(s_bMsgMgrLog == ON)
        {
            P_MSG_MANAGER_PrintMsgData((uint8_t*)pstTxSsovPkg, ntohs(pstTxSsovPkg->usLength) + sizeof(pstTxSsovPkg->unType) + sizeof(pstTxSsovPkg->usLength));
            PrintDebug("unType[%d], usLength[%d], usCrc16[0x%x]", ntohl(pstTxSsovPkg->unType), ntohs(pstTxSsovPkg->usLength), *pusCrc16);
        }
    }

#if defined(CONFIG_TEST_EXT_MSG_PKG)
    if (nRawPkgSize > 0)
    {
        pstTxPkg = (MSG_MANAGER_EXT_MSG_TLVC*)((uint8_t*)pstExtMsgOverall + sizeof(MSG_MANAGER_EXT_MSG_TLVC_OVERALL) + unExtMsgPkgLen);

        pstExtMsgOverall->ucNumOfPkg++;
        unExtMsgPkgLen = unExtMsgPkgLen + 8 + nRawPkgSize;
        pstExtMsgOverall->usLenOfPkg = htons(unExtMsgPkgLen);
        pstExtMsgOverall->usCrc16 = htons(CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgOverall, sizeof(MSG_MANAGER_EXT_MSG_TLVC_OVERALL) - MSG_MANAGER_CRC16_LEN));

        pstTxPkg->unType = htonl(MSG_MANAGER_EXT_MSG_RAW_DATA_PKG);
        pstTxPkg->usLength = htons(nRawPkgSize + MSG_MANAGER_CRC16_LEN);
        for(i = 0; i < nRawPkgSize; i++)
        {
            pstTxPkg->ucPayload[i] = i % 255;
        }

        pusCrc16 = (uint16_t*)((uint8_t*)pstTxPkg + (MSG_MANAGER_EXT_MSG_MAGIC_NUM_LEN + MSG_MANAGER_CRC16_LEN) + nRawPkgSize);
        *pusCrc16 = htons(CLI_UTIL_GetCrc16((uint8_t*)pstTxPkg, nRawPkgSize + (MSG_MANAGER_EXT_MSG_MAGIC_NUM_LEN + MSG_MANAGER_CRC16_LEN)));   // TLVC 중 CRC만 제외
    }

    nRet = P_MSG_MANAGER_CreateStatusPkg(pstExtMsgOverall, eMSG_MANAGER_EXT_MSG_STATUS_TX);
    if(nRet != FRAMEWORK_OK)
    {
        PrintError("P_MSG_MANAGER_CreateStatusPkg() is failed! [nRet:%d]", nRet);
    }
#endif

    memcpy(pstExtMsg->cMagicNumber, MSG_MANAGER_EXT_MSG_MAGIC_NUM_NAME, sizeof(pstExtMsg->cMagicNumber));
    unTxMsgLen = 16 + sizeof(MSG_MANAGER_EXT_MSG_TLVC_OVERALL) + ntohs(pstExtMsgOverall->usLenOfPkg);   // 16 : header(10) + psid(4) + crc(2)
    pstExtMsg->usLength = htons(SIZE_HDR_LEN_EXCEPT_DATA + sizeof(MSG_MANAGER_EXT_MSG_TLVC_OVERALL) + ntohs(pstExtMsgOverall->usLenOfPkg));        // seq(2) + payload id(2) + crc(2) + psid(4)
    pstExtMsg->usSeqNum = 0;
    pstExtMsg->usPayloadId = htons(eMSG_MANAGER_EXT_MSG_PAYLOAD_ID_TX);
    pstExtMsgTx->unPsid = htonl(unPsid);

    unTxMsgLen = ntohs(pstExtMsg->usLength) + (MSG_MANAGER_EXT_MSG_MAGIC_NUM_LEN + MSG_MANAGER_CRC16_LEN);

    pusCrc16 = (uint16_t*)&ucMsgBuf[unTxMsgLen - MSG_MANAGER_CRC16_LEN];
    usCalcCrc16 = CLI_UTIL_GetCrc16(ucMsgBuf + MSG_MANAGER_EXT_MSG_MAGIC_NUM_LEN, unTxMsgLen - (MSG_MANAGER_EXT_MSG_MAGIC_NUM_LEN + MSG_MANAGER_CRC16_LEN));
    *pusCrc16 = htons(usCalcCrc16);

    /* free the allocated payload */
    if(pstEventMsg->pPayload != NULL)
    {
        if (pstEventMsg->pstDbV2x->ePayloadType != DB_V2X_PAYLOAD_TYPE_PLATOONING_THROUGHPUT)
        {
            free(pstEventMsg->pPayload);
        }
    }

    nRetSendSize = send(s_nSocketHandle, ucMsgBuf, unTxMsgLen, 0);
    if (nRetSendSize < 0)
    {
        PrintError("send() is failed! [nRetSendSize:%ld]", nRetSendSize);
    }
    else if (nRetSendSize != (int32_t)unTxMsgLen)
    {
        PrintError("send() is failed! sent a different number of bytes than expected [nRetSendSize:%ld], unTxMsgLen[%d]", nRetSendSize, unTxMsgLen);
    }
    else if (nRetSendSize == 0)
    {
        PrintError("send() is failed! [nRetSendSize:%ld]", nRetSendSize);
    }
    else
    {
        nRet = FRAMEWORK_OK;
    }

    nRet = P_MSG_MANAGER_SendTxMsgToDbMgr(pstEventMsg, ntohl(ulDbV2xTotalPacketCrc32));
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("P_MSG_MANAGER_SendTxMsgToDbMgr() is faild! [nRet:%d]", nRet);
        return nRet;
    }

    if(pstDbV2x != NULL)
    {
        free(pstDbV2x);
    }

    if(pchDbV2xCrc != NULL)
    {
        free(pchDbV2xCrc);
    }

    return nRet;
}
#else
static int32_t P_MSG_MANAGER_SendTxMsg(MSG_MANAGER_TX_EVENT_MSG_T *pstEventMsg)
{
    int32_t nRet = FRAMEWORK_ERROR;
    uint32_t unDbV2xPacketLength = sizeof(DB_V2X_T) + pstEventMsg->pstDbV2x->ulPayloadLength + sizeof(pstEventMsg->pstDbV2x->ulReserved);
    uint32_t unDbV2xCrcCalcuatedLength = sizeof(DB_V2X_T) + pstEventMsg->pstDbV2x->ulPayloadLength;
    uint32_t unV2xTxPduLength = sizeof(Ext_V2X_TxPDU_t) + unDbV2xPacketLength;
    ssize_t nRetSendSize = 0;
    uint32_t ulTempDbV2xTotalPacketCrc32 = 0, ulDbV2xTotalPacketCrc32 = 0;
    TIME_MANAGER_T *pstTimeManager;

    Ext_V2X_TxPDU_t *pstV2xTxPdu = NULL;
    DB_V2X_T *pstDbV2x = NULL;
    uint8_t *pchDbV2xCrc = NULL;

    pstV2xTxPdu = malloc(unV2xTxPduLength);
    if(pstV2xTxPdu == NULL)
    {
        PrintError("malloc() is failed! [NULL]");
        return nRet;
    }

    memset(pstV2xTxPdu, 0, sizeof(Ext_V2X_TxPDU_t));

    pstV2xTxPdu->ver = htons(SAMPLE_V2X_API_VER);
    pstV2xTxPdu->e_payload_type = e_payload_type_g;
    pstV2xTxPdu->psid = htonl(psid_g);
    pstV2xTxPdu->tx_power = tx_power_g;
    pstV2xTxPdu->e_signer_id = e_signer_id_g;
    pstV2xTxPdu->e_priority = e_priority_g;

    if ((e_comm_type_g == eV2XCommType_LTEV2X) || (e_comm_type_g == eV2XCommType_5GNRV2X))
    {
        pstV2xTxPdu->magic_num = htons(MAGIC_CV2X_TX_PDU);
        pstV2xTxPdu->u.config_cv2x.transmitter_profile_id = htonl(transmitter_profile_id_g);
        pstV2xTxPdu->u.config_cv2x.peer_l2id = htonl(peer_l2id_g);
    }
    else if (e_comm_type_g == eV2XCommType_DSRC)
    {
        pstV2xTxPdu->magic_num = htons(MAGIC_DSRC_TX_PDU);
        pstV2xTxPdu->u.config_wave.freq = htons(freq_g);
        pstV2xTxPdu->u.config_wave.e_data_rate = htons(e_data_rate_g);
        pstV2xTxPdu->u.config_wave.e_time_slot = e_time_slot_g;
        memcpy(pstV2xTxPdu->u.config_wave.peer_mac_addr, peer_mac_addr_g, MAC_EUI48_LEN);
    }

    pstV2xTxPdu->v2x_msg.length = htons(unDbV2xPacketLength);
    s_unV2xMsgTxLen = unDbV2xPacketLength;

    pstDbV2x = malloc(unDbV2xPacketLength);
    if(pstDbV2x == NULL)
    {
        PrintError("malloc() is failed! [NULL]");
        return nRet;
    }

    memset(pstDbV2x, 0, unDbV2xPacketLength);

    pstDbV2x->eDeviceType = htons(pstEventMsg->pstDbV2x->eDeviceType);
    pstDbV2x->eTeleCommType = htons(pstEventMsg->pstDbV2x->eTeleCommType);
    pstDbV2x->unDeviceId = htonl(pstEventMsg->pstDbV2x->unDeviceId);
    pstDbV2x->ulTimeStamp = htonll(pstEventMsg->pstDbV2x->ulTimeStamp);
    pstDbV2x->eServiceId = htons(pstEventMsg->pstDbV2x->eServiceId);
    pstDbV2x->eActionType = htons(pstEventMsg->pstDbV2x->eActionType);
    pstDbV2x->eRegionId = htons(pstEventMsg->pstDbV2x->eRegionId);
    pstDbV2x->ePayloadType = htons(pstEventMsg->pstDbV2x->ePayloadType);
    pstDbV2x->eCommId = htons(pstEventMsg->pstDbV2x->eCommId);
    pstDbV2x->usDbVer = htons(pstEventMsg->pstDbV2x->usDbVer);
    pstDbV2x->usHwVer = htons(pstEventMsg->pstDbV2x->usHwVer);
    pstDbV2x->usSwVer = htons(pstEventMsg->pstDbV2x->usSwVer);
    pstDbV2x->ulPayloadLength = htonl(pstEventMsg->pstDbV2x->ulPayloadLength);
    pstDbV2x->ulReserved = htonl(pstEventMsg->pstDbV2x->ulReserved);

    pchDbV2xCrc = malloc(unDbV2xPacketLength);
    if(pchDbV2xCrc == NULL)
    {
        PrintError("malloc() is failed! [NULL]");
        return nRet;
    }
    memset(pchDbV2xCrc, 0, unDbV2xCrcCalcuatedLength);
    memcpy(pchDbV2xCrc, pstDbV2x, unDbV2xPacketLength);
    memcpy(pchDbV2xCrc + sizeof(DB_V2X_T), pstEventMsg->pPayload, pstEventMsg->pstDbV2x->ulPayloadLength);

    ulTempDbV2xTotalPacketCrc32 = CLI_UTIL_GetCrc32((uint8_t*)pchDbV2xCrc, sizeof(DB_V2X_T) + pstEventMsg->pstDbV2x->ulPayloadLength);
    ulDbV2xTotalPacketCrc32 = htonl(ulTempDbV2xTotalPacketCrc32);

    memcpy(pstV2xTxPdu->v2x_msg.data, pstDbV2x, unDbV2xPacketLength);
    memcpy(pstV2xTxPdu->v2x_msg.data + sizeof(DB_V2X_T), pstEventMsg->pPayload, pstEventMsg->pstDbV2x->ulPayloadLength);
    memcpy(pstV2xTxPdu->v2x_msg.data + sizeof(DB_V2X_T) + pstEventMsg->pstDbV2x->ulPayloadLength, &ulDbV2xTotalPacketCrc32, sizeof(uint32_t));

    /* free the allocated payload */
    if(pstEventMsg->pPayload != NULL)
    {
        if (pstEventMsg->pstDbV2x->ePayloadType != DB_V2X_PAYLOAD_TYPE_PLATOONING_THROUGHPUT)
        {
            free(pstEventMsg->pPayload);
        }
    }

    if(s_bMsgMgrLog == ON)
    {
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
            /* The average delay between svc and send() is about 10 us, so use the timestamp of svc */
            PrintDebug("[%ld]-[%ld]=[%ld]", pstTimeManager->ulTimeStamp, pstEventMsg->pstDbV2x->ulTimeStamp, pstTimeManager->ulTimeStamp-pstEventMsg->pstDbV2x->ulTimeStamp);
        }

        printf("\nV2X TX PDU>>\n"
        "  magic_num        : 0x%04X\n"
        "  ver              : 0x%04X\n"
        "  e_payload_type   : %d\n"
        "  psid             : %u\n"
        "  tx_power         : %d\n"
        "  e_signer_id      : %d\n"
        "  e_priority       : %d\n",
        ntohs(pstV2xTxPdu->magic_num),
        ntohs(pstV2xTxPdu->ver),
        pstV2xTxPdu->e_payload_type,
        ntohl(pstV2xTxPdu->psid),
        pstV2xTxPdu->tx_power,
        pstV2xTxPdu->e_signer_id,
        pstV2xTxPdu->e_priority);

        if (e_comm_type_g == eV2XCommType_LTEV2X || e_comm_type_g == eV2XCommType_5GNRV2X)
        {
            printf("  u.config_cv2x.transmitter_profile_id : %u\n"
            "  u.config_cv2x.peer_l2id              : %u\n",
            ntohl(pstV2xTxPdu->u.config_cv2x.transmitter_profile_id),
            ntohl(pstV2xTxPdu->u.config_cv2x.peer_l2id));
        }
        else if (e_comm_type_g == eV2XCommType_DSRC)
        {
            printf("  u.config_wave.freq                  : %d\n"
            "  u.config_wave.e_data_rate           : %d\n"
            "  u.config_wave.e_time_slot           : %d\n"
            "  u.config_wave.peer_mac_addr         : %s\n",
            ntohs(pstV2xTxPdu->u.config_wave.freq),
            ntohs(pstV2xTxPdu->u.config_wave.e_data_rate),
            pstV2xTxPdu->u.config_wave.e_time_slot,
            pstV2xTxPdu->u.config_wave.peer_mac_addr);
        }
    }

    nRetSendSize = send(s_nSocketHandle, pstV2xTxPdu, unV2xTxPduLength, 0);
    if (nRetSendSize < 0)
    {
        PrintError("send() is failed!!");
        nRet = FRAMEWORK_ERROR;
        return nRet;
    }
    else if (nRetSendSize != unV2xTxPduLength)
    {
        PrintError("send() sent a different number of bytes than expected!!");
        nRet = FRAMEWORK_ERROR;
        return nRet;
    }
    else
    {
        if(s_bMsgMgrLog == ON)
        {
            PrintDebug("tx send success (%ld bytes)", nRetSendSize);
        }
    }

    nRet = P_MSG_MANAGER_SendTxMsgToDbMgr(pstEventMsg, ntohl(ulDbV2xTotalPacketCrc32));
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("P_MSG_MANAGER_SendTxMsgToDbMgr() is faild! [nRet:%d]", nRet);
        return nRet;
    }

    if(pstV2xTxPdu != NULL)
    {
        free(pstV2xTxPdu);
    }

    if(pstDbV2x != NULL)
    {
        free(pstDbV2x);
    }

    if(pchDbV2xCrc != NULL)
    {
        free(pchDbV2xCrc);
    }

    return nRet;
}
#endif

static int32_t P_MSG_MANAGER_SendRxMsgToDbMgr(MSG_MANAGER_RX_EVENT_MSG_T *pstEventMsg, uint32_t unCrc32)
{
    int32_t nRet = FRAMEWORK_ERROR;
    DB_MANAGER_WRITE_T stDbManagerWrite;
    DB_MANAGER_EVENT_MSG_T stEventMsg;
    DB_MANAGER_T *pstDbManager;

    (void*)memset(&stDbManagerWrite, 0x00, sizeof(DB_MANAGER_WRITE_T));

    pstDbManager = FRAMEWORK_GetDbManagerInstance();
    if(pstDbManager == NULL)
    {
        PrintError("FRAMEWORK_GetDbManagerInstance() is failed!! pstDbManager is NULL");
        return nRet;
    }

    stDbManagerWrite.eFileType = pstDbManager->eFileType;
    stDbManagerWrite.eCommMsgType = DB_MANAGER_COMM_MSG_TYPE_RX;
    stDbManagerWrite.eProc = DB_MANAGER_PROC_WRITE;
    stDbManagerWrite.unCrc32 = unCrc32;

    stEventMsg.pstDbManagerWrite = &stDbManagerWrite;
    stEventMsg.pstDbV2x = pstEventMsg->pstDbV2x;

    /* free at P_DB_MANAGER_WriteXXX() */
    stEventMsg.pPayload = malloc(pstEventMsg->pstDbV2x->ulPayloadLength);
    if(stEventMsg.pPayload == NULL)
    {
        PrintError("malloc() is failed! [NULL]");
        return nRet;
    }

    memcpy(stEventMsg.pPayload, pstEventMsg->pPayload, pstEventMsg->pstDbV2x->ulPayloadLength);

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

#if defined(CONFIG_EXT_DATA_FORMAT)
static int32_t P_MSG_MANAGER_ProcessExtMsgPkg(MSG_MANAGER_RX_EVENT_MSG_T *pstEventMsg, void *pvExtMsgPkg)
{
    int32_t nRet = FRAMEWORK_ERROR;

    MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_TX *pstExtMsgModemTx;
    MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_RX *pstExtMsgModemRx;
    MSG_MANAGER_EXT_MSG_TLVC_COMM_UNIT *pstExtMsgComm;
    MSG_MANAGER_EXT_MSG_TLVC_CONTROL_UNIT *pstExtMsgCtrl;
    uint8_t *pucDeviceType = (uint8_t*)pvExtMsgPkg + 6;	// T, L 뒤에 dev_type 존재
    uint16_t usCalcCrc16;
    uint8_t ucStatus;
    DB_MANAGER_V2X_STATUS_T stDbV2xStatus;

    if(pstEventMsg == NULL)
    {
        PrintError("pstEventMsg is NULL");
        return nRet;
    }

    if(pvExtMsgPkg == NULL)
    {
        PrintError("pvExtMsgPkg is NULL");
        return nRet;
    }

    if(pucDeviceType == NULL)
    {
        PrintError("pucDeviceType is NULL");
        return nRet;
    }

    switch(*pucDeviceType)
    {
        case eMSG_MANAGER_EXT_MSG_DEV_TYPE_OBU_MODEM:
        {
            pstExtMsgModemTx = (MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_TX*)pvExtMsgPkg;
            pstExtMsgModemRx = (MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_RX*)pvExtMsgPkg;
            ucStatus = pstExtMsgModemTx->ucStatus;

            if (ucStatus == eMSG_MANAGER_EXT_MSG_STATUS_TX)
            {
                usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgModemTx, htons(pstExtMsgModemTx->usLenth) + 4); // T, L, V 길이
                if(usCalcCrc16 != ntohs(pstExtMsgModemTx->usCrc16))
                {
                    PrintError("[Error! crc16 error[0x%04x] != [0x%04x]", ntohs(pstExtMsgModemTx->usCrc16), usCalcCrc16);
                }

                nRet = DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
                if(nRet != FRAMEWORK_OK)
                {
                    PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                }

                stDbV2xStatus.stV2xStatusTx.stDbV2xDevL1.ulTimeStamp = ntohll(pstExtMsgModemTx->ulTimeStamp);
                stDbV2xStatus.stV2xStatusTx.stDbV2xDevL1.unDevId = htonl(pstExtMsgModemTx->unDevId);
                stDbV2xStatus.stV2xStatusTx.stDbV2xDevL1.usHwVer = htons(pstExtMsgModemTx->usHwVer);
                stDbV2xStatus.stV2xStatusTx.stDbV2xDevL1.usSwVer = htons(pstExtMsgModemTx->usSwVer);
                stDbV2xStatus.stV2xStatusTx.ucTxPwr = pstExtMsgModemTx->ucTxPwr;
                stDbV2xStatus.stV2xStatusTx.usTxFreq = htons(pstExtMsgModemTx->usTxFreq);
                stDbV2xStatus.stV2xStatusTx.ucTxBw = pstExtMsgModemTx->ucTxBw;

                stDbV2xStatus.stV2xStatusTx.ucScs = pstExtMsgModemTx->ucScs;
                stDbV2xStatus.stV2xStatusTx.ucMcs = pstExtMsgModemTx->ucMcs;
                stDbV2xStatus.stV2xGpsInfoTx.nLatitudeNow = htonl(pstExtMsgModemTx->nLatitude);
                stDbV2xStatus.stV2xGpsInfoTx.nLongitudeNow = htonl(pstExtMsgModemTx->nLongitude);

                nRet = DB_MANAGER_SetV2xStatus(&stDbV2xStatus);
                if(nRet != FRAMEWORK_OK)
                {
                    PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                }
            }
            else if (ucStatus == eMSG_MANAGER_EXT_MSG_STATUS_RX)
            {
                usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgModemRx, htons(pstExtMsgModemRx->usLenth) + 4);	// T, L, V 길이
                if(usCalcCrc16 != ntohs(pstExtMsgModemRx->usCrc16))
                {
                    PrintError("[Error! crc16 error[0x%04x] != [0x%04x]", ntohs(pstExtMsgModemRx->usCrc16), usCalcCrc16);
                }

                nRet = DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
                if(nRet != FRAMEWORK_OK)
                {
                    PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                }

                stDbV2xStatus.stV2xStatusRx.stDbV2xDevL1.ulTimeStamp = ntohll(pstExtMsgModemRx->ulTimeStamp);
                stDbV2xStatus.stV2xStatusRx.stDbV2xDevL1.unDevId = htonl(pstExtMsgModemRx->unDevId);
                stDbV2xStatus.stV2xStatusRx.stDbV2xDevL1.usHwVer = htons(pstExtMsgModemRx->usHwVer);
                stDbV2xStatus.stV2xStatusRx.stDbV2xDevL1.usSwVer = htons(pstExtMsgModemRx->usSwVer);
                stDbV2xStatus.stV2xStatusRx.nRssi = pstExtMsgModemRx->nRssi;
                stDbV2xStatus.stV2xStatusRx.ucRcpi = pstExtMsgModemRx->ucRcpi;

                stDbV2xStatus.stV2xGpsInfoRx.nLatitudeNow = htonl(pstExtMsgModemRx->nLatitude);
                stDbV2xStatus.stV2xGpsInfoRx.nLongitudeNow = htonl(pstExtMsgModemRx->nLongitude);

                nRet = DB_MANAGER_SetV2xStatus(&stDbV2xStatus);
                if(nRet != FRAMEWORK_OK)
                {
                    PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                }
            }
            else
            {
                PrintError("Error type [ucStatus:%d]\n", ucStatus);
                return nRet;
            }

            break;
        }

        case eMSG_MANAGER_EXT_MSG_DEV_TYPE_OBU:
        {
            pstExtMsgComm = (MSG_MANAGER_EXT_MSG_TLVC_COMM_UNIT*)pvExtMsgPkg;

            nRet = DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
            if(nRet != FRAMEWORK_OK)
            {
                PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
            }

            if(pstExtMsgComm->ucStatus == eMSG_MANAGER_EXT_MSG_STATUS_TX)
            {
                stDbV2xStatus.stV2xStatusTx.stDbV2xDevL2.ulTimeStamp = ntohll(pstExtMsgComm->ulTimeStamp);
                stDbV2xStatus.stV2xStatusTx.stDbV2xDevL2.unDevId = htonl(pstExtMsgComm->unDevId);
                stDbV2xStatus.stV2xStatusTx.stDbV2xDevL2.usHwVer = htons(pstExtMsgComm->usHwVer);
                stDbV2xStatus.stV2xStatusTx.stDbV2xDevL2.usSwVer = htons(pstExtMsgComm->usSwVer);
            }
            else
            {
                stDbV2xStatus.stV2xStatusRx.stDbV2xDevL2.ulTimeStamp = ntohll(pstExtMsgComm->ulTimeStamp);
                stDbV2xStatus.stV2xStatusRx.stDbV2xDevL2.unDevId = htonl(pstExtMsgComm->unDevId);
                stDbV2xStatus.stV2xStatusRx.stDbV2xDevL2.usHwVer = htons(pstExtMsgComm->usHwVer);
                stDbV2xStatus.stV2xStatusRx.stDbV2xDevL2.usSwVer = htons(pstExtMsgComm->usSwVer);
            }

            nRet = DB_MANAGER_SetV2xStatus(&stDbV2xStatus);
            if(nRet != FRAMEWORK_OK)
            {
                PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
            }

            usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgComm, htons(pstExtMsgComm->usLenth) + 4);	// T, L, V 길이
            if(usCalcCrc16 != ntohs(pstExtMsgComm->usCrc16))
            {
                PrintError("[Error! crc16 error[0x%04x] != [0x%04x]", ntohs(pstExtMsgComm->usCrc16), usCalcCrc16);
            }

            break;
        }

        case eMSG_MANAGER_EXT_MSG_DEV_TYPE_RSU:
        {
            pstExtMsgComm = (MSG_MANAGER_EXT_MSG_TLVC_COMM_UNIT*)pvExtMsgPkg;
            usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgComm, htons(pstExtMsgComm->usLenth) + 4);	// T, L, V 길이
            if(usCalcCrc16 != ntohs(pstExtMsgComm->usCrc16))
            {
                PrintError("[Error! crc16 error[0x%04x] != [0x%04x]", ntohs(pstExtMsgComm->usCrc16), usCalcCrc16);
            }

            break;
        }

        case eMSG_MANAGER_EXT_MSG_DEV_TYPE_RSU_MODEM:
        {
            pstExtMsgModemTx = (MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_TX*)pvExtMsgPkg;
            pstExtMsgModemRx = (MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_RX*)pvExtMsgPkg;
            ucStatus = pstExtMsgModemTx->ucStatus;

            if (ucStatus == eMSG_MANAGER_EXT_MSG_STATUS_TX)
            {
                usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgModemTx, htons(pstExtMsgModemTx->usLenth) + 4);	// T, L, V 길이
                if(usCalcCrc16 != ntohs(pstExtMsgModemTx->usCrc16))
                {
                    PrintError("[Error! crc16 error[0x%04x] != [0x%04x]", ntohs(pstExtMsgModemTx->usCrc16), usCalcCrc16);
                }
            }
            else if (ucStatus == eMSG_MANAGER_EXT_MSG_STATUS_RX)
            {
                usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgModemRx, htons(pstExtMsgModemRx->usLenth) + 4);	// T, L, V 길이
                if(usCalcCrc16 != ntohs(pstExtMsgModemRx->usCrc16))
                {
                    PrintError("[Error! crc16 error[0x%04x] != [0x%04x]", ntohs(pstExtMsgModemRx->usCrc16), usCalcCrc16);
                }
            }
            else
            {
                PrintError("Error type [ucStatus:%d]\n", ucStatus);
                return nRet;
            }

            break;
        }

        case eMSG_MANAGER_EXT_MSG_DEV_TYPE_RSU_CTL:
        {
            pstExtMsgCtrl = (MSG_MANAGER_EXT_MSG_TLVC_CONTROL_UNIT*)pvExtMsgPkg;
            usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgCtrl, htons(pstExtMsgCtrl->usLenth) + 4);	// T, L, V 길이
            if(usCalcCrc16 != ntohs(pstExtMsgCtrl->usCrc16))
            {
                PrintError("Error! crc16 error[0x%04x] != [0x%04x]", ntohs(pstExtMsgCtrl->usCrc16), usCalcCrc16);
            }

            break;
        }

        default:
        {
            PrintError("Error! unknown device type[%d]", *pucDeviceType);
            break;
        }
    }

    nRet = FRAMEWORK_OK;

	return nRet;
}

static void P_MSG_MANAGER_PrintExtMsgPkg(void *pvExtMsgPkg)
{
    MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_TX *pstExtMsgModemTx;
    MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_RX *pstExtMsgModemRx;
    MSG_MANAGER_EXT_MSG_TLVC_COMM_UNIT *pstExtMsgComm;
    MSG_MANAGER_EXT_MSG_TLVC_CONTROL_UNIT *pstExtMsgCtrl;
    uint8_t *pucDeviceType = (uint8_t*)pvExtMsgPkg + 6;	// T, L 뒤에 dev_type 존재
    uint16_t usCalcCrc16;
    uint8_t ucStatus;

    if(pvExtMsgPkg == NULL)
    {
        PrintError("pstEventMsg is NULL");
        return;
    }

    if(pucDeviceType == NULL)
    {
        PrintError("pucDeviceType is NULL");
        return;
    }

    switch(*pucDeviceType)
    {
        case eMSG_MANAGER_EXT_MSG_DEV_TYPE_OBU_MODEM:
        {
            pstExtMsgModemTx = (MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_TX*)pvExtMsgPkg;
            pstExtMsgModemRx = (MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_RX*)pvExtMsgPkg;
            ucStatus = pstExtMsgModemTx->ucStatus;

            if (ucStatus == eMSG_MANAGER_EXT_MSG_STATUS_TX)
            {
                PrintDebug("OBU Modem : Tx");
                PrintDebug("unDevId[%u]", htonl(pstExtMsgModemTx->unDevId));
                PrintDebug("usHwVer[%d], usSwVer[%d]", htons(pstExtMsgModemTx->usHwVer), htons(pstExtMsgModemTx->usSwVer));
                PrintDebug("ucTxPwr[%d], usTxFreq[%d], ucTxBw[%d], ucMcs[%d], ucScs[%d]", pstExtMsgModemTx->ucTxPwr, htons(pstExtMsgModemTx->usTxFreq), pstExtMsgModemTx->ucTxBw, pstExtMsgModemTx->ucMcs, pstExtMsgModemTx->ucScs);
                PrintDebug("nLatitude[%d], nLongitude[%d]", htonl(pstExtMsgModemTx->nLatitude), htonl(pstExtMsgModemTx->nLongitude));
                PrintDebug("ulTimeStamp[%ld]", ntohll(pstExtMsgModemTx->ulTimeStamp));
                PrintDebug("ulTimeStamp[%ld]", ntohll(pstExtMsgModemTx->ulTimeStamp));
                PrintDebug("chCpuTemp[%d], chPeriTemp[%d]", pstExtMsgModemTx->chCpuTemp, pstExtMsgModemTx->chPeriTemp);

                usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgModemTx, htons(pstExtMsgModemTx->usLenth) + 4); // T, L, V 길이
                if(usCalcCrc16 != ntohs(pstExtMsgModemTx->usCrc16))
                {
                    PrintError("[Error! crc16 error[0x%04x] != [0x%04x]", ntohs(pstExtMsgModemTx->usCrc16), usCalcCrc16);
                }
            }
            else if (ucStatus == eMSG_MANAGER_EXT_MSG_STATUS_RX)
            {
                PrintDebug("OBU Modem : Rx");
                PrintDebug("unDevId[%u]", htonl(pstExtMsgModemRx->unDevId));
                PrintDebug("usHwVer[%d], usSwVer[%d]", htons(pstExtMsgModemRx->usHwVer), htons(pstExtMsgModemRx->usSwVer));
                PrintDebug("nRssi[%d], ucRcpi[%d]", pstExtMsgModemRx->nRssi, pstExtMsgModemRx->ucRcpi);
                PrintDebug("nLatitude[%d], nLongitude[%d]", htonl(pstExtMsgModemRx->nLatitude), htonl(pstExtMsgModemRx->nLongitude));
                PrintDebug("ulTimeStamp[%ld]", ntohll(pstExtMsgModemRx->ulTimeStamp));
                PrintDebug("chCpuTemp[%d], chPeriTemp[%d]", pstExtMsgModemRx->chCpuTemp, pstExtMsgModemRx->chPeriTemp);

                usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgModemRx, htons(pstExtMsgModemRx->usLenth) + 4);	// T, L, V 길이
                if(usCalcCrc16 != ntohs(pstExtMsgModemRx->usCrc16))
                {
                    PrintError("[Error! crc16 error[0x%04x] != [0x%04x]", ntohs(pstExtMsgModemRx->usCrc16), usCalcCrc16);
                }
            }
            else
            {
                PrintError("Error type [ucStatus:%d]\n", ucStatus);
                return;
            }

            break;
        }

        case eMSG_MANAGER_EXT_MSG_DEV_TYPE_OBU:
        {
            pstExtMsgComm = (MSG_MANAGER_EXT_MSG_TLVC_COMM_UNIT*)pvExtMsgPkg;

            PrintDebug("OBU AP : %s", (pstExtMsgComm->ucStatus == eMSG_MANAGER_EXT_MSG_STATUS_TX) ? "Tx":"Rx");
            PrintDebug("unDevId[%d]", htonl(pstExtMsgComm->unDevId));
            PrintDebug("usHwVer[%d], usSwVer[%d]", htons(pstExtMsgComm->usHwVer), htons(pstExtMsgComm->usSwVer));
            PrintDebug("ulTimeStamp[%ld]", ntohll(pstExtMsgComm->ulTimeStamp));
            PrintDebug("chCpuTemp[%d], chPeriTemp[%d]", pstExtMsgComm->chCpuTemp, pstExtMsgComm->chPeriTemp);

            usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgComm, htons(pstExtMsgComm->usLenth) + 4);	// T, L, V 길이
            if(usCalcCrc16 != ntohs(pstExtMsgComm->usCrc16))
            {
                PrintError("[Error! crc16 error[0x%04x] != [0x%04x]", ntohs(pstExtMsgComm->usCrc16), usCalcCrc16);
            }

            break;
        }

        case eMSG_MANAGER_EXT_MSG_DEV_TYPE_RSU:
        {
            pstExtMsgComm = (MSG_MANAGER_EXT_MSG_TLVC_COMM_UNIT*)pvExtMsgPkg;

            PrintDebug("RSU AP : %s", (pstExtMsgComm->ucStatus == eMSG_MANAGER_EXT_MSG_STATUS_TX) ? "Tx":"Rx");
            PrintDebug("unDevId[%d]", htonl(pstExtMsgComm->unDevId));
            PrintDebug("usHwVer[%d], usSwVer[%d]", htons(pstExtMsgComm->usHwVer), htons(pstExtMsgComm->usSwVer));
            PrintDebug("ulTimeStamp[%ld]", ntohll(pstExtMsgComm->ulTimeStamp));
            PrintDebug("chCpuTemp[%d], chPeriTemp[%d]", pstExtMsgComm->chCpuTemp, pstExtMsgComm->chPeriTemp);

            usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgComm, htons(pstExtMsgComm->usLenth) + 4);	// T, L, V 길이
            if(usCalcCrc16 != ntohs(pstExtMsgComm->usCrc16))
            {
                PrintError("[Error! crc16 error[0x%04x] != [0x%04x]", ntohs(pstExtMsgComm->usCrc16), usCalcCrc16);
            }

            break;
        }

        case eMSG_MANAGER_EXT_MSG_DEV_TYPE_RSU_MODEM:
        {
            pstExtMsgModemTx = (MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_TX*)pvExtMsgPkg;
            pstExtMsgModemRx = (MSG_MANAGER_EXT_MSG_TLVC_MODEM_UNIT_RX*)pvExtMsgPkg;
            ucStatus = pstExtMsgModemTx->ucStatus;

            if (ucStatus == eMSG_MANAGER_EXT_MSG_STATUS_TX)
            {
                PrintDebug("RSU Modem : Tx");
                PrintDebug("unDevId[%u]", htonl(pstExtMsgModemTx->unDevId));
                PrintDebug("usHwVer[%d], usSwVer[%d]", htons(pstExtMsgModemTx->usHwVer), htons(pstExtMsgModemTx->usSwVer));
                PrintDebug("ucTxPwr[%d], usTxFreq[%d], ucTxBw[%d], ucMcs[%d], ucScs[%d]", pstExtMsgModemTx->ucTxPwr, htons(pstExtMsgModemTx->usTxFreq), pstExtMsgModemTx->ucTxBw, pstExtMsgModemTx->ucMcs, pstExtMsgModemTx->ucScs);
                PrintDebug("nLatitude[%d], nLongitude[%d]", htonl(pstExtMsgModemTx->nLatitude), htonl(pstExtMsgModemTx->nLongitude));
                PrintDebug("ulTimeStamp[%ld]", ntohll(pstExtMsgModemTx->ulTimeStamp));
                PrintDebug("chCpuTemp[%d], chPeriTemp[%d]", pstExtMsgModemTx->chCpuTemp, pstExtMsgModemTx->chPeriTemp);

                usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgModemTx, htons(pstExtMsgModemTx->usLenth) + 4);	// T, L, V 길이
                if(usCalcCrc16 != ntohs(pstExtMsgModemTx->usCrc16))
                {
                    PrintError("[Error! crc16 error[0x%04x] != [0x%04x]", ntohs(pstExtMsgModemTx->usCrc16), usCalcCrc16);
                }
            }
            else if (ucStatus == eMSG_MANAGER_EXT_MSG_STATUS_RX)
            {
                PrintDebug("RSU Modem : Rx");
                PrintDebug("unDevId[%u]", htonl(pstExtMsgModemRx->unDevId));
                PrintDebug("usHwVer[%d], usSwVer[%d]", htons(pstExtMsgModemRx->usHwVer), htons(pstExtMsgModemRx->usSwVer));
                PrintDebug("nRssi[%d], ucRcpi[%d]", pstExtMsgModemRx->nRssi, pstExtMsgModemRx->ucRcpi);
                PrintDebug("nLatitude[%d], nLongitude[%d]", htonl(pstExtMsgModemRx->nLatitude), htonl(pstExtMsgModemRx->nLongitude));
                PrintDebug("ulTimeStamp[%ld]", ntohll(pstExtMsgModemRx->ulTimeStamp));
                PrintDebug("chCpuTemp[%d], chPeriTemp[%d]", pstExtMsgModemRx->chCpuTemp, pstExtMsgModemRx->chPeriTemp);

                usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgModemRx, htons(pstExtMsgModemRx->usLenth) + 4);	// T, L, V 길이
                if(usCalcCrc16 != ntohs(pstExtMsgModemRx->usCrc16))
                {
                    PrintError("[Error! crc16 error[0x%04x] != [0x%04x]", ntohs(pstExtMsgModemRx->usCrc16), usCalcCrc16);
                }
            }
            else
            {
                PrintError("Error type [ucStatus:%d]\n", ucStatus);
                return;
            }

            break;
        }

        case eMSG_MANAGER_EXT_MSG_DEV_TYPE_RSU_CTL:
        {
            pstExtMsgCtrl = (MSG_MANAGER_EXT_MSG_TLVC_CONTROL_UNIT*)pvExtMsgPkg;

            PrintDebug("RSU Controller : %s", (pstExtMsgCtrl->ucStatus == eMSG_MANAGER_EXT_MSG_STATUS_TX) ? "Tx":"Rx");
            PrintDebug("unDevId[%d]", htonl(pstExtMsgCtrl->unDevId));
            PrintDebug("usHwVer[%d], usSwVer[%d]", htons(pstExtMsgCtrl->usHwVer), htons(pstExtMsgCtrl->usSwVer));
            PrintDebug("ulTimeStamp[%ld]", ntohll(pstExtMsgCtrl->ulTimeStamp));
            PrintDebug("chCpuTemp[%d], chPeriTemp[%d]", pstExtMsgCtrl->chCpuTemp, pstExtMsgCtrl->chPeriTemp);

            usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgCtrl, htons(pstExtMsgCtrl->usLenth) + 4);	// T, L, V 길이
            if(usCalcCrc16 != ntohs(pstExtMsgCtrl->usCrc16))
            {
                PrintError("Error! crc16 error[0x%04x] != [0x%04x]", ntohs(pstExtMsgCtrl->usCrc16), usCalcCrc16);
            }

            break;
        }

        default:
        {
            PrintError("Error! unknown device type[%d]", *pucDeviceType);
            break;
        }
    }

    return;
}

static int32_t P_MSG_MANAGER_AnalyzeRxMsg(MSG_MANAGER_RX_EVENT_MSG_T *pstEventMsg, uint8_t *pucMsg, int32_t nRxLen)
{
    int32_t nRet = FRAMEWORK_ERROR;
    uint8_t ucNumPkgCnt;
    uint32_t unOverallPkgLen, unTotalPkgLen, unRemainedPkgLen, unTlvcPkgLen;
    uint16_t usCalcCrc16;
    void *pvNextRxPkg;
    MSG_MANAGER_EXT_MSG_TLVC_OVERALL *pstExtMsgOverall = NULL;
    MSG_MANAGER_EXT_MSG *pstExtMsg = (MSG_MANAGER_EXT_MSG *)pucMsg;
    MSG_MANAGER_EXT_MSG_RX *pstExtMsgRx = (MSG_MANAGER_EXT_MSG_RX *)pstExtMsg->ucPayload;
    uint32_t unPsid = ntohl(pstExtMsgRx->unPsid);
    bool bExtMsgFlag = FALSE;
    uint32_t unType;
    MSG_MANAGER_EXT_MSG_TLVC *pstRxPkg;

    if(pstEventMsg == NULL)
    {
        PrintError("pstEventMsg is NULL");
        return nRet;
    }

    if(pucMsg == NULL)
    {
        PrintError("pucMsg is NULL");
        return nRet;
    }

    if (nRxLen > 0)
    {
        pstExtMsgOverall = (MSG_MANAGER_EXT_MSG_TLVC_OVERALL*)pstExtMsgRx->ucPayload;
    }
    else
    {
        PrintError("Error! extensible message length[%d]", nRxLen);
        return nRet;
    }

    if(unPsid == MSG_MANAGER_EXT_MSG_V2V_PSID)
    {
        PrintTrace("Get Extensible Message - V2V");
        bExtMsgFlag = TRUE;
    }
    else if (unPsid == MSG_MANAGER_EXT_MSG_V2I_PSID)
    {
        PrintTrace("Get Extensible Message - V2I");
        bExtMsgFlag = TRUE;
    }
    else if (unPsid == MSG_MANAGER_EXT_MSG_I2V_PSID)
    {
        PrintTrace("Get Extensible Message - I2V");
        bExtMsgFlag = TRUE;
    }
    else
    {
        PrintTrace("Get Normal Message - PSID(%d)", unPsid);
    }

    if (bExtMsgFlag == TRUE)
    {
        if (ntohl(pstExtMsgOverall->unType) != MSG_MANAGER_EXT_MSG_OVERALL_PKG)
        {
            PrintError("Error! overall type[%d] != [%d]", ntohl(pstExtMsgOverall->unType), MSG_MANAGER_EXT_MSG_OVERALL_PKG);
            return nRet;
        }

        unOverallPkgLen = ntohs(pstExtMsgOverall->usLength);
        unRemainedPkgLen = unTotalPkgLen = pstExtMsgOverall->usLenOfPkg;

        PrintWarn("[Overall Package] ucVersion[%d], unOverallPkgLen[%d]", pstExtMsgOverall->ucVersion, unOverallPkgLen);
        PrintDebug("Number of Packages[%d], Total Length of Package[%d]", pstExtMsgOverall->ucNumOfPkg, ntohs(unTotalPkgLen));
        PrintDebug("bitsize[%d]", pstExtMsgOverall->ucBitwise);

        usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgOverall, unOverallPkgLen + 4);	// T, L, V 길이
        if(usCalcCrc16 != ntohs(pstExtMsgOverall->usCrc16))
        {
            PrintError("Error! crc16 error[0x%04x] != [0x%04x]", ntohs(pstExtMsgOverall->usCrc16), usCalcCrc16);
        }

        pvNextRxPkg = (uint8_t*)pstExtMsgOverall + sizeof(MSG_MANAGER_EXT_MSG_TLVC_OVERALL); // next TLVC

        for (ucNumPkgCnt = 1; ucNumPkgCnt <= pstExtMsgOverall->ucNumOfPkg; ucNumPkgCnt++)
        {
            pstRxPkg = (MSG_MANAGER_EXT_MSG_TLVC *)pvNextRxPkg;
            unTlvcPkgLen = ntohs(pstRxPkg->usLength);
            unType = ntohl(pstRxPkg->unType);

            if (unRemainedPkgLen < unTlvcPkgLen)
            {
                PrintError("Error! remain length [unTlvcPkgLen:%d]", unTlvcPkgLen);
                break;
            }

            if (unType == MSG_MANAGER_EXT_MSG_STATUS_PKG)
            {
                PrintWarn("Package : %d (Status Package)", ucNumPkgCnt);
                P_MSG_MANAGER_PrintExtMsgPkg(pvNextRxPkg);
            }
            else
            {
                PrintDebug("Package : %d\n\tPSID : %d, TLV lenth : %d", ucNumPkgCnt, unType, unTlvcPkgLen + 6);
                P_MSG_MANAGER_PrintMsgData((uint8_t*)pvNextRxPkg, unTlvcPkgLen + 6);   // 6: T, L 크기 추가
            }

            pvNextRxPkg = pvNextRxPkg + unTlvcPkgLen + 6; // 6: T, L 크기
            unRemainedPkgLen = unRemainedPkgLen - unTlvcPkgLen - 6; // 6: T, L 크기
        }
    }
    else
    {
        (void)P_MSG_MANAGER_PrintMsgData(pucMsg, nRxLen);
    }

    nRet = FRAMEWORK_OK;

    return nRet;
}

static int32_t P_MSG_MANAGER_ProcessSsovPkg(MSG_MANAGER_RX_EVENT_MSG_T *pstEventMsg, void *pvExtMsgPkg)
{
    int32_t nRet = FRAMEWORK_ERROR;

    MSG_MANAGER_EXT_MSG_SSOV *pstExtMsgSsov;
    uint16_t usExtMsgSsovLength;

    DB_V2X_T *pstDbV2x = NULL;
    uint32_t ulDbV2xTotalPacketCrc32 = 0, ulCompDbV2xTotalPacketCrc32 = 0, ulTempDbV2xTotalPacketCrc32 = 0;
    DB_MANAGER_V2X_STATUS_T stDbV2xStatus;
    uint32_t ulRxPayloadLength = 0;
    uint32_t ulExtMsgSsovCrcBuf = 0;
    uint16_t usExtMsgSsovCrc16 = 0, usCalcCrc16 = 0, usTempExtMsgSsovCrc16 = 0;
    uint32_t ulExtMsgSsovTotalPkgSize = 0;

    if(pstEventMsg == NULL)
    {
        PrintError("pstEventMsg is NULL");
        return nRet;
    }

    if(pvExtMsgPkg == NULL)
    {
        PrintError("pvExtMsgPkg is NULL");
        return nRet;
    }

    pstExtMsgSsov = (MSG_MANAGER_EXT_MSG_SSOV*)pvExtMsgPkg;
    usExtMsgSsovLength = ntohs(pstExtMsgSsov->usLength);
    ulExtMsgSsovTotalPkgSize = sizeof(pstExtMsgSsov->unType) + sizeof(pstExtMsgSsov->usLength) + usExtMsgSsovLength;

    memcpy(&usTempExtMsgSsovCrc16, pstExtMsgSsov->ucPayload + usExtMsgSsovLength - sizeof(uint16_t), sizeof(uint16_t));
    usExtMsgSsovCrc16 = ntohs(usTempExtMsgSsovCrc16);

    if(s_bMsgMgrLog == TRUE)
    {
        PrintDebug("unType[%d], usLength[%d]", ntohl(pstExtMsgSsov->unType), ntohs(pstExtMsgSsov->usLength));
        PrintDebug("ulExtMsgSsovTotalPkgSize(%d), ulExtMsgSsovCrcBuf[0x%x], usExtMsgSsovCrc16[0x%x]", ulExtMsgSsovTotalPkgSize, ulExtMsgSsovCrcBuf, usExtMsgSsovCrc16);
        P_MSG_MANAGER_PrintMsgData((uint8_t*)pstExtMsgSsov, ulExtMsgSsovTotalPkgSize);
    }

    usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgSsov, ulExtMsgSsovTotalPkgSize - sizeof(uint16_t));
    if(usCalcCrc16 != usExtMsgSsovCrc16)
    {
        PrintError("Error! crc16 usCalcCrc16[0x%04x] != usExtMsgSsovCrc16[0x%04x]", usCalcCrc16, usExtMsgSsovCrc16);
    }

    if(s_unV2xMsgTxLen != 0)
    {
        if(s_bFirstPacket == TRUE)
        {
            s_unV2xMsgRxLen = s_unV2xMsgTxLen;
            PrintTrace("Update s_unV2xMsgTxLen[%d] => s_unV2xMsgRxLen[%d]", s_unV2xMsgTxLen, s_unV2xMsgRxLen);
        }

        if(s_unV2xMsgRxLen != (uint32_t)(usExtMsgSsovLength - MSG_MANAGER_CRC16_LEN))
        {
            PrintError("Tx and Rx size does not matched!! check s_unV2xMsgRxLen[%d] != ntohs(pstExtMsgSsov->usLength)[%d]", s_unV2xMsgRxLen, usExtMsgSsovLength);
            nRet = DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
            if(nRet != FRAMEWORK_OK)
            {
                PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
            }

            stDbV2xStatus.stV2xStatusRx.ucErrIndicator = TRUE;
            stDbV2xStatus.stV2xStatusRx.ulTotalErrCnt++;
            PrintWarn("increase ulTotalErrCnt [from %ld to %ld]", (stDbV2xStatus.stV2xStatusRx.ulTotalErrCnt-1), stDbV2xStatus.stV2xStatusRx.ulTotalErrCnt);

            nRet = DB_MANAGER_SetV2xStatus(&stDbV2xStatus);
            if(nRet != FRAMEWORK_OK)
            {
                PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
            }
        }
        else
        {
            pstDbV2x = malloc(usExtMsgSsovLength);
            if(pstDbV2x == NULL)
            {
                PrintError("malloc() is failed! [NULL]");
            }
            else
            {
                memset(pstDbV2x, 0, usExtMsgSsovLength);
                memcpy(pstDbV2x, pstExtMsgSsov->ucPayload, sizeof(DB_V2X_T));
                ulRxPayloadLength = ntohl(pstDbV2x->ulPayloadLength);

                P_MSG_MANAGER_PrintMsgData(pstExtMsgSsov->ucPayload, sizeof(DB_V2X_T) + ulRxPayloadLength);
                memcpy(&ulTempDbV2xTotalPacketCrc32, pstExtMsgSsov->ucPayload + sizeof(DB_V2X_T) + ulRxPayloadLength, sizeof(uint32_t));
                ulDbV2xTotalPacketCrc32 = ntohl(ulTempDbV2xTotalPacketCrc32);

                ulCompDbV2xTotalPacketCrc32 = CLI_UTIL_GetCrc32((uint8_t*)&pstExtMsgSsov->ucPayload[0], sizeof(DB_V2X_T) + ulRxPayloadLength);
                if(ulDbV2xTotalPacketCrc32 != ulCompDbV2xTotalPacketCrc32)
                {
                    PrintError("CRC32 does not matched!! check Get:ulDbV2xTotalPacketCrc32[0x%x] != Calculate:ulCompDbV2xTotalPacketCrc32[0x%x]", ulDbV2xTotalPacketCrc32, ulCompDbV2xTotalPacketCrc32);
                    nRet = DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
                    if(nRet != FRAMEWORK_OK)
                    {
                        PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                    }

                    stDbV2xStatus.stV2xStatusRx.ucErrIndicator = TRUE;
                    stDbV2xStatus.stV2xStatusRx.ulTotalErrCnt++;
                    PrintWarn("increase ulTotalErrCnt [from %ld to %ld]", (stDbV2xStatus.stV2xStatusRx.ulTotalErrCnt-1), stDbV2xStatus.stV2xStatusRx.ulTotalErrCnt);

                    nRet = DB_MANAGER_SetV2xStatus(&stDbV2xStatus);
                    if(nRet != FRAMEWORK_OK)
                    {
                        PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                    }
                }
                else
                {
                    nRet = DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
                    if(nRet != FRAMEWORK_OK)
                    {
                        PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                    }

                    stDbV2xStatus.stV2xStatusRx.ulTotalPacketCnt++;

                    if(s_bFirstPacket == TRUE)
                    {
                        s_bFirstPacket = FALSE;
                        stDbV2xStatus.bFirstPacket = TRUE;
                        PrintTrace("Received the first packets, stDbV2xStatus.bFirstPacket [%d]", stDbV2xStatus.bFirstPacket);
                        /* The first packet number is updated at db manager, P_DB_MANAGER_UpdateStatus() */
                    }

                    nRet = DB_MANAGER_SetV2xStatus(&stDbV2xStatus);
                    if(nRet != FRAMEWORK_OK)
                    {
                        PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                    }

                    pstEventMsg->pPayload = malloc(ulRxPayloadLength);
                    if(pstEventMsg->pPayload == NULL)
                    {
                        PrintError("malloc() is failed! [NULL]");
                    }
                    else
                    {
                        if(s_bMsgMgrLog == ON)
                        {
                            PrintDebug("db_v2x_tmp_p->eDeviceType[%d]", ntohs(pstDbV2x->eDeviceType));
                            PrintDebug("db_v2x_tmp_p->eTeleCommType[%d]", ntohs(pstDbV2x->eTeleCommType));
                            PrintDebug("db_v2x_tmp_p->unDeviceId[%d]", ntohl(pstDbV2x->unDeviceId));
                            PrintDebug("db_v2x_tmp_p->ulTimeStamp[%ld]", ntohll(pstDbV2x->ulTimeStamp));
                            PrintDebug("db_v2x_tmp_p->eServiceId[%d]", ntohs(pstDbV2x->eServiceId));
                            PrintDebug("db_v2x_tmp_p->eActionType[%d]", ntohs(pstDbV2x->eActionType));
                            PrintDebug("db_v2x_tmp_p->eRegionId[%d]", ntohs(pstDbV2x->eRegionId));
                            PrintDebug("db_v2x_tmp_p->ePayloadType[%d]", ntohs(pstDbV2x->ePayloadType));
                            PrintDebug("db_v2x_tmp_p->eCommId[%d]", ntohs(pstDbV2x->eCommId));
                            PrintDebug("db_v2x_tmp_p->usDbVer[%d.%d]", ntohs(pstDbV2x->usDbVer) >> CLI_DB_V2X_MAJOR_SHIFT, ntohs(pstDbV2x->usDbVer) & CLI_DB_V2X_MINOR_MASK);
                            PrintDebug("db_v2x_tmp_p->usHwVer[%d]", ntohs(pstDbV2x->usHwVer));
                            PrintDebug("db_v2x_tmp_p->usSwVer[%d]", ntohs(pstDbV2x->usSwVer));
                            PrintDebug("db_v2x_tmp_p->ulPayloadLength[%d]", ulRxPayloadLength);
                            PrintDebug("db_v2x_tmp_p->ulReserved[0x%x]", ntohl(pstDbV2x->ulReserved));

                            PrintDebug("received CRC:ulDbV2xTotalPacketCrc32[0x%x]", ulDbV2xTotalPacketCrc32);
                            PrintDebug("calcuated CRC:ulCompDbV2xTotalPacketCrc32[0x%x]", ulCompDbV2xTotalPacketCrc32);

                            if(ulDbV2xTotalPacketCrc32 == ulCompDbV2xTotalPacketCrc32)
                            {
                                PrintTrace("CRC32 is matched!");
                            }
                        }

                        memcpy(pstEventMsg->pPayload, pstExtMsgSsov->ucPayload + sizeof(DB_V2X_T), ulRxPayloadLength);

                        pstEventMsg->pstDbV2x->eDeviceType = ntohs(pstDbV2x->eDeviceType);
                        pstEventMsg->pstDbV2x->eTeleCommType = ntohs(pstDbV2x->eTeleCommType);
                        pstEventMsg->pstDbV2x->unDeviceId = ntohl(pstDbV2x->unDeviceId);
                        pstEventMsg->pstDbV2x->ulTimeStamp = ntohll(pstDbV2x->ulTimeStamp);
                        pstEventMsg->pstDbV2x->eServiceId = ntohs(pstDbV2x->eServiceId);
                        pstEventMsg->pstDbV2x->eActionType = ntohs(pstDbV2x->eActionType);
                        pstEventMsg->pstDbV2x->eRegionId = ntohs(pstDbV2x->eRegionId);
                        pstEventMsg->pstDbV2x->ePayloadType = ntohs(pstDbV2x->ePayloadType);
                        pstEventMsg->pstDbV2x->eCommId = ntohs(pstDbV2x->eCommId);
                        pstEventMsg->pstDbV2x->usDbVer = ntohs(pstDbV2x->usDbVer);
                        pstEventMsg->pstDbV2x->usHwVer = ntohs(pstDbV2x->usHwVer);
                        pstEventMsg->pstDbV2x->usSwVer = ntohs(pstDbV2x->usSwVer);
                        pstEventMsg->pstDbV2x->ulPayloadLength = ulRxPayloadLength;
                        pstEventMsg->pstDbV2x->ulReserved = ntohl(pstDbV2x->ulReserved);

                        nRet = DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
                        if(nRet != FRAMEWORK_OK)
                        {
                            PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                        }

                        stDbV2xStatus.ulTxTimeStamp = pstEventMsg->pstDbV2x->ulTimeStamp;

                        nRet = DB_MANAGER_SetV2xStatus(&stDbV2xStatus);
                        if(nRet != FRAMEWORK_OK)
                        {
                            PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                        }

                        nRet = P_MSG_MANAGER_SendRxMsgToDbMgr(pstEventMsg, ulDbV2xTotalPacketCrc32);
                        if (nRet != FRAMEWORK_OK)
                        {
                            PrintError("P_MSG_MANAGER_SendTxMsgToDbMgr() is faild! [nRet:%d]", nRet);
                        }

                        if(pstEventMsg->pPayload != NULL)
                        {
                            free(pstEventMsg->pPayload);
                        }
                    }
                }

                if(pstDbV2x != NULL)
                {
                    free(pstDbV2x);
                }
            }
        }
    }
    else
    {
        if(s_bMsgMgrLog == ON)
        {
            PrintWarn("The Message Manager is not started yet.");
        }
    }

    nRet = FRAMEWORK_OK;

    return nRet;
}

static int32_t P_MSG_MANAGER_ProcessRxMsg(MSG_MANAGER_RX_EVENT_MSG_T *pstEventMsg, uint8_t *pucMsg, int32_t nRxLen)
{
    int32_t nRet = FRAMEWORK_ERROR;
    uint8_t ucNumPkgCnt;
    uint32_t unOverallPkgLen, unTotalPkgLen, unRemainedPkgLen, unTlvcPkgLen;
    uint16_t usCalcCrc16;
    void *pvNextRxPkg;
    MSG_MANAGER_EXT_MSG_TLVC_OVERALL *pstExtMsgOverall = NULL;
    MSG_MANAGER_EXT_MSG *pstExtMsg = (MSG_MANAGER_EXT_MSG *)pucMsg;
    MSG_MANAGER_EXT_MSG_RX *pstExtMsgRx = (MSG_MANAGER_EXT_MSG_RX *)pstExtMsg->ucPayload;
    uint32_t unPsid = ntohl(pstExtMsgRx->unPsid);
    bool bExtMsgFlag = FALSE;
    uint32_t unType;
    MSG_MANAGER_EXT_MSG_TLVC *pstRxPkg;

    if (nRxLen > 0)
    {
        pstExtMsgOverall = (MSG_MANAGER_EXT_MSG_TLVC_OVERALL*)pstExtMsgRx->ucPayload;
    }
    else
    {
        PrintError("Error! extensible message length[%d]", nRxLen);
        return nRet;
    }

    if(unPsid == MSG_MANAGER_EXT_MSG_V2V_PSID)
    {
        if (s_bMsgMgrLog == ON)
        {
            PrintTrace("Get Extensible Message - V2V");
        }
        bExtMsgFlag = TRUE;
    }
    else if (unPsid == MSG_MANAGER_EXT_MSG_V2I_PSID)
    {
        if (s_bMsgMgrLog == ON)
        {
            PrintTrace("Get Extensible Message - V2I");
        }
        bExtMsgFlag = TRUE;
    }
    else if (unPsid == MSG_MANAGER_EXT_MSG_I2V_PSID)
    {
        if (s_bMsgMgrLog == ON)
        {
            PrintTrace("Get Extensible Message - I2V");
        }
        bExtMsgFlag = TRUE;
    }
    else
    {
        PrintTrace("Get Normal Message - PSID(%d)", unPsid);
    }

    if (bExtMsgFlag == TRUE)
    {
        if (ntohl(pstExtMsgOverall->unType) != MSG_MANAGER_EXT_MSG_OVERALL_PKG)
        {
            PrintError("Error! overall type[%d] != [%d]", ntohl(pstExtMsgOverall->unType), MSG_MANAGER_EXT_MSG_OVERALL_PKG);
            return nRet;
        }

        unOverallPkgLen = ntohs(pstExtMsgOverall->usLength);
        unRemainedPkgLen = unTotalPkgLen = pstExtMsgOverall->usLenOfPkg;

        if(s_bMsgMgrLog == ON)
        {
            PrintWarn("[Overall Package] ucVersion[%d], unOverallPkgLen[%d]", pstExtMsgOverall->ucVersion, unOverallPkgLen);
            PrintDebug("Number of Packages[%d], Total Length of Package[%d]", pstExtMsgOverall->ucNumOfPkg, ntohs(unTotalPkgLen));
        }

        usCalcCrc16 = CLI_UTIL_GetCrc16((uint8_t*)pstExtMsgOverall, unOverallPkgLen + 4);	// T, L, V 길이
        if(usCalcCrc16 != ntohs(pstExtMsgOverall->usCrc16))
        {
            PrintError("Error! crc16 error[0x%04x] != [0x%04x]", ntohs(pstExtMsgOverall->usCrc16), usCalcCrc16);
        }

        pvNextRxPkg = (uint8_t*)pstExtMsgOverall + sizeof(MSG_MANAGER_EXT_MSG_TLVC_OVERALL); // next TLVC

        for (ucNumPkgCnt = 1; ucNumPkgCnt <= pstExtMsgOverall->ucNumOfPkg; ucNumPkgCnt++)
        {
            pstRxPkg = (MSG_MANAGER_EXT_MSG_TLVC *)pvNextRxPkg;
            unTlvcPkgLen = ntohs(pstRxPkg->usLength);
            unType = ntohl(pstRxPkg->unType);

            if (unRemainedPkgLen < unTlvcPkgLen)
            {
                PrintError("Error! remain length [unTlvcPkgLen:%d]", unTlvcPkgLen);
                break;
            }

            if (unType == MSG_MANAGER_EXT_MSG_STATUS_PKG)
            {
                if(s_bMsgMgrLog == ON)
                {
                    PrintWarn("Package : %d (Status Package)", ucNumPkgCnt);
                }

                nRet = P_MSG_MANAGER_ProcessExtMsgPkg(pstEventMsg, pvNextRxPkg);
                if(nRet != FRAMEWORK_OK)
                {
                    PrintError("P_MSG_MANAGER_ProcessExtMsgPkg() is failed! [nRet:%d]", nRet);
                }
            }
            else if (unType == MSG_MANAGER_EXT_MSG_SSOV_PKG)
            {
                if (s_bMsgMgrLog == ON)
                {
                    PrintWarn("SSOV Package : %d\n\tPSID : %d, TLV lenth : %d", ucNumPkgCnt, unType, unTlvcPkgLen + 6);
                }
                nRet = P_MSG_MANAGER_ProcessSsovPkg(pstEventMsg, pvNextRxPkg);
                if(nRet != FRAMEWORK_OK)
                {
                    PrintError("P_MSG_MANAGER_ProcessSsovPkg() is failed! [nRet:%d]", nRet);
                }
            }
            else
            {
                PrintDebug("Package : %d\n\tPSID : %d, TLV lenth : %d", ucNumPkgCnt, unType, unTlvcPkgLen + 6);
                P_MSG_MANAGER_PrintMsgData((uint8_t*)pvNextRxPkg, unTlvcPkgLen + 6);   // 6: T, L 크기 추가
            }

            pvNextRxPkg = pvNextRxPkg + unTlvcPkgLen + 6; // 6: T, L 크기
            unRemainedPkgLen = unRemainedPkgLen - unTlvcPkgLen - 6; // 6: T, L 크기
        }
    }
    else
    {
        (void)P_MSG_MANAGER_PrintMsgData(pucMsg, nRxLen);
    }

    nRet = FRAMEWORK_OK;

    return nRet;
}

static int32_t P_MSG_MANAGER_ReceiveRxMsg(MSG_MANAGER_RX_EVENT_MSG_T *pstEventMsg)
{
    int32_t nRet = FRAMEWORK_ERROR;
    uint8_t ucMsgBuf[MSG_MANAGER_MAX_RX_PKG_SIZE] = {0};
    int nRecvLen = -1;

    if(pstEventMsg == NULL)
    {
        PrintError("pstEventMsg is NULL");
        return nRet;
    }

    while (1)
    {
        nRecvLen = recv(s_nSocketHandle, ucMsgBuf, sizeof(ucMsgBuf), 0);
        if (nRecvLen < 0)
        {
            if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
            {
                if(s_bMsgMgrLog == ON)
                {
                    PrintError("recv() is failed!!");
                }

                usleep(1000*1000);
            }
            else
            {
                usleep(10*1000);
                continue;
            }
        }
        else if (nRecvLen == 0)
        {
            PrintError("recv()'s connection is closed by peer!!");
            break;
        }
        else
        {
            if(s_bMsgMgrLog == ON)
            {
                PrintDebug("recv() is success, nRecvLen[%u]", nRecvLen);
                nRet = P_MSG_MANAGER_AnalyzeRxMsg(pstEventMsg, ucMsgBuf, nRecvLen);
                if(nRet != FRAMEWORK_OK)
                {
                    PrintError("P_MSG_MANAGER_AnalyzeRxMsg() is failed! [nRet:%d]", nRet);
                }
            }

            nRet =  P_MSG_MANAGER_ProcessRxMsg(pstEventMsg, ucMsgBuf, nRecvLen);
            if(nRet != FRAMEWORK_OK)
            {
                PrintError("P_MSG_MANAGER_ProcessRxMsg() is failed! [nRet:%d]", nRet);
            }
        }
    }

    return nRet;
}
#else
static int32_t P_MSG_MANAGER_ReceiveRxMsg(MSG_MANAGER_RX_EVENT_MSG_T *pstEventMsg)
{
    int32_t nRet = FRAMEWORK_ERROR;
    uint8_t buf[4096] = {0};
    int nRecvLen = -1;
    Ext_V2X_RxPDU_t *pstV2xRxPdu = NULL;
    DB_V2X_T *pstDbV2x = NULL;
    uint32_t ulDbV2xTotalPacketCrc32 = 0, ulCompDbV2xTotalPacketCrc32 = 0, ulTempDbV2xTotalPacketCrc32 = 0;
    DB_MANAGER_V2X_STATUS_T stDbV2xStatus;
    uint32_t ulRxPayloadLength = 0;

    if(pstEventMsg == NULL)
    {
        PrintError("pstEventMsg is NULL");
        return nRet;
    }

    while (1)
    {
        nRecvLen = recv(s_nSocketHandle, buf, sizeof(buf), 0);
        if (nRecvLen < 0)
        {
            if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
            {
                if(s_bMsgMgrLog == ON)
                {
                    PrintError("recv() is failed!!");
                }

                usleep(1000*1000);
            }
            else
            {
                usleep(10*1000);
                continue;
            }
        }
        else if (nRecvLen == 0)
        {
            PrintError("recv()'s connection is closed by peer!!");
            break;
        }
        else
        {
            if(s_bMsgMgrLog == ON)
            {
                PrintDebug("recv() is success, nRecvLen[%u]", nRecvLen);
            }

            pstV2xRxPdu = malloc(nRecvLen);
            if(pstV2xRxPdu == NULL)
            {
                PrintError("malloc() is failed! [NULL]");
            }
            else
            {
                memset(pstV2xRxPdu, 0, nRecvLen);
                memcpy(pstV2xRxPdu, buf, nRecvLen);

                if(s_unV2xMsgTxLen != 0)
                {
                    if(s_bFirstPacket == TRUE)
                    {
                        s_unV2xMsgRxLen = s_unV2xMsgTxLen;
                        PrintTrace("Update s_unV2xMsgTxLen[%d] => s_unV2xMsgRxLen[%d]", s_unV2xMsgTxLen, s_unV2xMsgRxLen);
                    }

                    if(s_unV2xMsgRxLen != ntohs(pstV2xRxPdu->v2x_msg.length))
                    {
                        PrintError("Tx and Rx size does not matched!! check s_unV2xMsgRxLen[%d] != pstV2xRxPdu->v2x_msg.length[%d]", s_unV2xMsgRxLen, ntohs(pstV2xRxPdu->v2x_msg.length));
                        nRet = DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
                        if(nRet != FRAMEWORK_OK)
                        {
                            PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                        }

                        stDbV2xStatus.stV2xStatusRx.ucErrIndicator = TRUE;
                        stDbV2xStatus.stV2xStatusRx.ulTotalErrCnt++;
                        PrintWarn("increase ulTotalErrCnt [from %ld to %ld]", (stDbV2xStatus.stV2xStatusRx.ulTotalErrCnt-1), stDbV2xStatus.stV2xStatusRx.ulTotalErrCnt);

                        nRet = DB_MANAGER_SetV2xStatus(&stDbV2xStatus);
                        if(nRet != FRAMEWORK_OK)
                        {
                            PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                        }
                    }
                    else
                    {
                        pstDbV2x = malloc(ntohs(pstV2xRxPdu->v2x_msg.length));
                        if(pstDbV2x == NULL)
                        {
                            PrintError("malloc() is failed! [NULL]");
                        }
                        else
                        {
                            memset(pstDbV2x, 0, ntohs(pstV2xRxPdu->v2x_msg.length));
                            memcpy(pstDbV2x, pstV2xRxPdu->v2x_msg.data, sizeof(DB_V2X_T));

                            ulRxPayloadLength = ntohl(pstDbV2x->ulPayloadLength);

                            memcpy(&ulTempDbV2xTotalPacketCrc32, pstV2xRxPdu->v2x_msg.data + sizeof(DB_V2X_T) + ulRxPayloadLength, sizeof(uint32_t));
                            ulDbV2xTotalPacketCrc32 = ntohl(ulTempDbV2xTotalPacketCrc32);

                            ulCompDbV2xTotalPacketCrc32 = CLI_UTIL_GetCrc32((uint8_t*)&pstV2xRxPdu->v2x_msg.data[0], sizeof(DB_V2X_T) + ulRxPayloadLength);
                            if(ulDbV2xTotalPacketCrc32 != ulCompDbV2xTotalPacketCrc32)
                            {
                                PrintError("CRC32 does not matched!! check Get:ulDbV2xTotalPacketCrc32[0x%x] != Calculate:ulCompDbV2xTotalPacketCrc32[0x%x]", ulDbV2xTotalPacketCrc32, ulCompDbV2xTotalPacketCrc32);
                                PrintError("Check nRecvLen[%d], sizeof(Ext_V2X_RxPDU_t)[%ld]+pstV2xRxPdu->v2x_msg.length[%d]", nRecvLen, sizeof(Ext_V2X_RxPDU_t), ntohs(pstV2xRxPdu->v2x_msg.length));
                                nRet = DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
                                if(nRet != FRAMEWORK_OK)
                                {
                                    PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                                }

                                stDbV2xStatus.stV2xStatusRx.ucErrIndicator = TRUE;
                                stDbV2xStatus.stV2xStatusRx.ulTotalErrCnt++;
                                PrintWarn("increase ulTotalErrCnt [from %ld to %ld]", (stDbV2xStatus.stV2xStatusRx.ulTotalErrCnt-1), stDbV2xStatus.stV2xStatusRx.ulTotalErrCnt);

                                nRet = DB_MANAGER_SetV2xStatus(&stDbV2xStatus);
                                if(nRet != FRAMEWORK_OK)
                                {
                                    PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                                }
                            }
                            else
                            {
                                nRet = DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
                                if(nRet != FRAMEWORK_OK)
                                {
                                    PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                                }

                                stDbV2xStatus.stV2xStatusRx.ulTotalPacketCnt++;

                                if(s_bFirstPacket == TRUE)
                                {
                                    s_bFirstPacket = FALSE;
                                    stDbV2xStatus.bFirstPacket = TRUE;
                                    PrintTrace("Received the first packets, stDbV2xStatus.bFirstPacket [%d]", stDbV2xStatus.bFirstPacket);
                                    /* The first packet number is updated at db manager, P_DB_MANAGER_UpdateStatus() */
                                }

                                nRet = DB_MANAGER_SetV2xStatus(&stDbV2xStatus);
                                if(nRet != FRAMEWORK_OK)
                                {
                                    PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                                }

                                pstEventMsg->pPayload = malloc(ulRxPayloadLength);
                                if(pstEventMsg->pPayload == NULL)
                                {
                                    PrintError("malloc() is failed! [NULL]");
                                }
                                else
                                {
                                    if(s_bMsgMgrLog == ON)
                                    {
                                        printf("\nV2X RX PDU>>\n"
                                        "  magic_num        : 0x%04X\n"
                                        "  ver              : 0x%04X\n"
                                        "  e_v2x_comm_type   : %d\n"
                                        "  e_payload_type   : %d\n"
                                        "  psid             : %u\n"
                                        "  freq             : %d\n"
                                        "  rssi             : %d\n"
                                        "  reserved1        : %d\n"
                                        "  peer_l2id        : %d\n"
                                        "  crc              : %d\n"
                                        "  v2x_msg.length   : %d\n",
                                        ntohs(pstV2xRxPdu->magic_num),
                                        ntohs(pstV2xRxPdu->ver),
                                        pstV2xRxPdu->e_v2x_comm_type,
                                        pstV2xRxPdu->e_payload_type,
                                        ntohl(pstV2xRxPdu->psid),
                                        pstV2xRxPdu->freq,
                                        pstV2xRxPdu->rssi,
                                        pstV2xRxPdu->reserved1,
                                        pstV2xRxPdu->u.peer_l2id,
                                        pstV2xRxPdu->crc,
                                        ntohs(pstV2xRxPdu->v2x_msg.length));

                                        PrintDebug("db_v2x_tmp_p->eDeviceType[%d]", ntohs(pstDbV2x->eDeviceType));
                                        PrintDebug("db_v2x_tmp_p->eTeleCommType[%d]", ntohs(pstDbV2x->eTeleCommType));
                                        PrintDebug("db_v2x_tmp_p->unDeviceId[%d]", ntohl(pstDbV2x->unDeviceId));
                                        PrintDebug("db_v2x_tmp_p->ulTimeStamp[%ld]", ntohll(pstDbV2x->ulTimeStamp));
                                        PrintDebug("db_v2x_tmp_p->eServiceId[%d]", ntohs(pstDbV2x->eServiceId));
                                        PrintDebug("db_v2x_tmp_p->eActionType[%d]", ntohs(pstDbV2x->eActionType));
                                        PrintDebug("db_v2x_tmp_p->eRegionId[%d]", ntohs(pstDbV2x->eRegionId));
                                        PrintDebug("db_v2x_tmp_p->ePayloadType[%d]", ntohs(pstDbV2x->ePayloadType));
                                        PrintDebug("db_v2x_tmp_p->eCommId[%d]", ntohs(pstDbV2x->eCommId));
                                        PrintDebug("db_v2x_tmp_p->usDbVer[%d.%d]", ntohs(pstDbV2x->usDbVer) >> CLI_DB_V2X_MAJOR_SHIFT, ntohs(pstDbV2x->usDbVer) & CLI_DB_V2X_MINOR_MASK);
                                        PrintDebug("db_v2x_tmp_p->usHwVer[%d]", ntohs(pstDbV2x->usHwVer));
                                        PrintDebug("db_v2x_tmp_p->usSwVer[%d]", ntohs(pstDbV2x->usSwVer));
                                        PrintDebug("db_v2x_tmp_p->ulPayloadLength[%d]", ulRxPayloadLength);
                                        PrintDebug("db_v2x_tmp_p->ulReserved[0x%x]", ntohl(pstDbV2x->ulReserved));

                                        PrintDebug("received CRC:ulDbV2xTotalPacketCrc32[0x%x]", ulDbV2xTotalPacketCrc32);
                                        PrintDebug("calcuated CRC:ulCompDbV2xTotalPacketCrc32[0x%x]", ulCompDbV2xTotalPacketCrc32);

                                        if(ulDbV2xTotalPacketCrc32 == ulCompDbV2xTotalPacketCrc32)
                                        {
                                            PrintTrace("CRC32 is matched!");
                                        }
                                    }

                                    memcpy(pstEventMsg->pPayload, pstV2xRxPdu->v2x_msg.data + sizeof(DB_V2X_T), ulRxPayloadLength);

                                    pstEventMsg->pstDbV2x->eDeviceType = ntohs(pstDbV2x->eDeviceType);
                                    pstEventMsg->pstDbV2x->eTeleCommType = ntohs(pstDbV2x->eTeleCommType);
                                    pstEventMsg->pstDbV2x->unDeviceId = ntohl(pstDbV2x->unDeviceId);
                                    pstEventMsg->pstDbV2x->ulTimeStamp = ntohll(pstDbV2x->ulTimeStamp);
                                    pstEventMsg->pstDbV2x->eServiceId = ntohs(pstDbV2x->eServiceId);
                                    pstEventMsg->pstDbV2x->eActionType = ntohs(pstDbV2x->eActionType);
                                    pstEventMsg->pstDbV2x->eRegionId = ntohs(pstDbV2x->eRegionId);
                                    pstEventMsg->pstDbV2x->ePayloadType = ntohs(pstDbV2x->ePayloadType);
                                    pstEventMsg->pstDbV2x->eCommId = ntohs(pstDbV2x->eCommId);
                                    pstEventMsg->pstDbV2x->usDbVer = ntohs(pstDbV2x->usDbVer);
                                    pstEventMsg->pstDbV2x->usHwVer = ntohs(pstDbV2x->usHwVer);
                                    pstEventMsg->pstDbV2x->usSwVer = ntohs(pstDbV2x->usSwVer);
                                    pstEventMsg->pstDbV2x->ulPayloadLength = ulRxPayloadLength;
                                    pstEventMsg->pstDbV2x->ulReserved = ntohl(pstDbV2x->ulReserved);

                                    nRet = DB_MANAGER_GetV2xStatus(&stDbV2xStatus);
                                    if(nRet != FRAMEWORK_OK)
                                    {
                                        PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                                    }

                                    stDbV2xStatus.ulTxTimeStamp = pstEventMsg->pstDbV2x->ulTimeStamp;

                                    nRet = DB_MANAGER_SetV2xStatus(&stDbV2xStatus);
                                    if(nRet != FRAMEWORK_OK)
                                    {
                                        PrintError("DB_MANAGER_GetV2xStatus() is failed! [nRet:%d]", nRet);
                                    }

                                    nRet = P_MSG_MANAGER_SendRxMsgToDbMgr(pstEventMsg, ulDbV2xTotalPacketCrc32);
                                    if (nRet != FRAMEWORK_OK)
                                    {
                                        PrintError("P_MSG_MANAGER_SendTxMsgToDbMgr() is faild! [nRet:%d]", nRet);
                                    }

                                    if(pstEventMsg->pPayload != NULL)
                                    {
                                        free(pstEventMsg->pPayload);
                                    }
                                }
                            }

                            if(pstDbV2x != NULL)
                            {
                                free(pstDbV2x);
                            }
                        }
                    }
                }
                else
                {
                    if(s_bMsgMgrLog == ON)
                    {
                        PrintWarn("The Message Manager is not started yet.");
                    }
                }

                if(pstV2xRxPdu != NULL)
                {
                    free(pstV2xRxPdu);
                }
            }
        }
    }

    return nRet;
}
#endif

static void *P_MSG_MANAGER_TxTask(void *arg)
{
    MSG_MANAGER_TX_EVENT_MSG_T stEventMsg;
    int32_t nRet = FRAMEWORK_ERROR;

    UNUSED(arg);

    memset(&stEventMsg, 0, sizeof(MSG_MANAGER_TX_EVENT_MSG_T));

    while (1)
    {
        if(msgrcv(s_nMsgTxTaskMsgId, &stEventMsg, sizeof(MSG_MANAGER_TX_EVENT_MSG_T), 0, MSG_NOERROR) == FRAMEWORK_MSG_ERR)
        {
            PrintError("msgrcv() is failed!");
        }
        else
        {
            nRet = P_MSG_MANAGER_SendTxMsg(&stEventMsg);
            if (nRet != FRAMEWORK_OK)
            {
                PrintError("P_MSG_MANAGER_SendTxMsg() is faild! [nRet:%d]", nRet);
            }
        }
    }

    return NULL;
}

static void *P_MSG_MANAGER_RxTask(void *arg)
{
    int32_t nRet = FRAMEWORK_ERROR;

    MSG_MANAGER_RX_EVENT_MSG_T stEventMsg;
    MSG_MANAGER_RX_T           stMsgManagerRx;
    DB_V2X_T                   stDbV2x;

    UNUSED(arg);

    (void*)memset(&stEventMsg, 0x00, sizeof(MSG_MANAGER_RX_EVENT_MSG_T));
    (void*)memset(&stMsgManagerRx, 0x00, sizeof(MSG_MANAGER_RX_T));
    (void*)memset(&stDbV2x, 0x00, sizeof(DB_V2X_T));

    stEventMsg.pstMsgManagerRx = &stMsgManagerRx;
    stEventMsg.pstDbV2x = &stDbV2x;

    nRet = P_MSG_MANAGER_ReceiveRxMsg(&stEventMsg);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("P_MSG_MANAGER_ReceiveRxMsg() is faild! [nRet:%d]", nRet);
    }

    return NULL;
}

#if defined(CONFIG_TEMP_OBU_TEST)
void *MSG_MANAGER_TxTask(void *arg)
{
    (void)arg;

    // Prepare the Ext_WSReq_t structure
    int db_v2x_tmp_size = sizeof(DB_V2X_T) + SAMPLE_V2X_MSG_LEN;
    int v2x_tx_pdu_size = sizeof(Ext_V2X_TxPDU_t) + db_v2x_tmp_size;

    Ext_V2X_TxPDU_t *v2x_tx_pdu_p = NULL;
    DB_V2X_T *db_v2x_tmp_p = NULL;
    MSG_MANAGER_TX_EVENT_MSG_T stEventMsg;

    v2x_tx_pdu_p = malloc(v2x_tx_pdu_size);
    if(v2x_tx_pdu_p == NULL)
    {
        PrintError("malloc() is failed! [NULL]");
        return NULL;
    }

    memset(&stEventMsg, 0, sizeof(MSG_MANAGER_TX_T));
    memset(v2x_tx_pdu_p, 0, sizeof(Ext_V2X_TxPDU_t));

    v2x_tx_pdu_p->ver = htons(SAMPLE_V2X_API_VER);
    v2x_tx_pdu_p->e_payload_type = e_payload_type_g;
    v2x_tx_pdu_p->psid = htonl(psid_g);
    v2x_tx_pdu_p->tx_power = tx_power_g;
    v2x_tx_pdu_p->e_signer_id = e_signer_id_g;
    v2x_tx_pdu_p->e_priority = e_priority_g;

    if (e_comm_type_g == eV2XCommType_LTEV2X || e_comm_type_g == eV2XCommType_5GNRV2X)
    {
        v2x_tx_pdu_p->magic_num = htons(MAGIC_CV2X_TX_PDU);
        v2x_tx_pdu_p->u.config_cv2x.transmitter_profile_id = htonl(transmitter_profile_id_g);
        v2x_tx_pdu_p->u.config_cv2x.peer_l2id = htonl(peer_l2id_g);
    }
    else if (e_comm_type_g == eV2XCommType_DSRC)
    {
        v2x_tx_pdu_p->magic_num = htons(MAGIC_DSRC_TX_PDU);
        v2x_tx_pdu_p->u.config_wave.freq = htons(freq_g);
        v2x_tx_pdu_p->u.config_wave.e_data_rate = htons(e_data_rate_g);
        v2x_tx_pdu_p->u.config_wave.e_time_slot = e_time_slot_g;
        memcpy(v2x_tx_pdu_p->u.config_wave.peer_mac_addr, peer_mac_addr_g, MAC_EUI48_LEN);
    }

    // Payload = KETI Format
    v2x_tx_pdu_p->v2x_msg.length = htons(db_v2x_tmp_size);

    db_v2x_tmp_p = malloc(db_v2x_tmp_size);
    if(db_v2x_tmp_p == NULL)
    {
        PrintError("malloc() is failed! [NULL]");
        return NULL;
    }

    memset(db_v2x_tmp_p, 0, db_v2x_tmp_size);

    db_v2x_tmp_p->eDeviceType = DB_V2X_DEVICE_TYPE_OBU;
    db_v2x_tmp_p->eTeleCommType = DB_V2X_TELECOMM_TYPE_5G_PC5;
    db_v2x_tmp_p->unDeviceId = 0;
    db_v2x_tmp_p->ulTimeStamp = 0ULL;
    db_v2x_tmp_p->eServiceId = DB_V2X_SERVICE_ID_PLATOONING;
    db_v2x_tmp_p->eActionType = DB_V2X_ACTION_TYPE_REQUEST;
    db_v2x_tmp_p->eRegionId = DB_V2X_REGION_ID_SEOUL;
    db_v2x_tmp_p->ePayloadType = DB_V2X_PAYLOAD_TYPE_SAE_J2735_BSM;
    db_v2x_tmp_p->eCommId = DB_V2X_COMM_ID_V2V;
    db_v2x_tmp_p->usDbVer = 0;
    db_v2x_tmp_p->usHwVer = 0;
    db_v2x_tmp_p->usSwVer = 0;
    db_v2x_tmp_p->ulPayloadLength = SAMPLE_V2X_MSG_LEN;
    db_v2x_tmp_p->ulReserved = 0;

    memcpy(v2x_tx_pdu_p->v2x_msg.data, db_v2x_tmp_p, db_v2x_tmp_size);

    printf("\nV2X TX PDU>>\n"
    "  magic_num        : 0x%04X\n"
    "  ver              : 0x%04X\n"
    "  e_payload_type   : %d\n"
    "  psid             : %u\n"
    "  tx_power         : %d\n"
    "  e_signer_id      : %d\n"
    "  e_priority       : %d\n",
    ntohs(v2x_tx_pdu_p->magic_num),
    ntohs(v2x_tx_pdu_p->ver),
    v2x_tx_pdu_p->e_payload_type,
    ntohl(v2x_tx_pdu_p->psid),
    v2x_tx_pdu_p->tx_power,
    v2x_tx_pdu_p->e_signer_id,
    v2x_tx_pdu_p->e_priority);

    if (e_comm_type_g == eV2XCommType_LTEV2X || e_comm_type_g == eV2XCommType_5GNRV2X)
    {
        printf("  u.config_cv2x.transmitter_profile_id : %u\n"
        "  u.config_cv2x.peer_l2id              : %u\n",
        ntohl(v2x_tx_pdu_p->u.config_cv2x.transmitter_profile_id),
        ntohl(v2x_tx_pdu_p->u.config_cv2x.peer_l2id));
    }
    else if (e_comm_type_g == eV2XCommType_DSRC)
    {
        printf("  u.config_wave.freq                  : %d\n"
        "  u.config_wave.e_data_rate           : %d\n"
        "  u.config_wave.e_time_slot           : %d\n"
        "  u.config_wave.peer_mac_addr         : %s\n",
        ntohs(v2x_tx_pdu_p->u.config_wave.freq),
        ntohs(v2x_tx_pdu_p->u.config_wave.e_data_rate),
        v2x_tx_pdu_p->u.config_wave.e_time_slot,
        v2x_tx_pdu_p->u.config_wave.peer_mac_addr);
    }

    uint32_t i;
    ssize_t n;

    for (i = 0; i < tx_cnt_g; i++)
    {
        n = send(s_nSocketHandle, v2x_tx_pdu_p, v2x_tx_pdu_size, 0);
        if (n < 0)
        {
            PrintError("send() is failed!!");
            break;
        }
        else if (n != v2x_tx_pdu_size)
        {
            PrintError("send() sent a different number of bytes than expected!!");
            break;
        }
        else
        {
            PrintDebug("tx send success (%ld bytes) : [%u/%u]", n, i + 1, tx_cnt_g);
        }

        P_MSG_MANAGER_SendTxMsgToDbMgr(&stEventMsg);

        usleep((1000 * tx_delay_g));
    }

    free(v2x_tx_pdu_p);
    free(db_v2x_tmp_p);

    return NULL;
}

void *MSG_MANAGER_RxTask(void *arg)
{
	(void)arg;

	uint8_t buf[4096] = {0};
	int nRecvLen = -1;
	time_t start_time = time(NULL);

	while (1)
	{
		time_t current_time = time(NULL);
		if (current_time - start_time >= delay_time_sec_g)
		{
			break;
		}

		nRecvLen = recv(s_nSocketHandle, buf, sizeof(buf), 0);
		if (nRecvLen < 0)
		{
			if ((errno != EAGAIN) && (errno != EWOULDBLOCK)
			{
				PrintError("recv() is failed!!");
				break;
			}
			else
			{
				usleep(10000);
				continue;
			}
		}
		else if (nRecvLen == 0)
		{
			PrintError("recv()'s connection is closed by peer!!");
			break;
		}
		else
		{
			PrintDebug("recv() is success : nRecvLen[%u]", nRecvLen);
		}
	}

	return NULL;
}
#endif

int32_t P_MSG_MANAGER_CreateTask(void)
{
	int32_t nRet = FRAMEWORK_ERROR;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    nRet = pthread_create(&sh_msgMgrTxTask, &attr, P_MSG_MANAGER_TxTask, NULL);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("pthread_create() is failed!! (P_MSG_MANAGER_TxTask) [nRet:%d]", nRet);
    }
    else
    {
        PrintTrace("P_MSG_MANAGER_TxTask() is successfully created.");
        nRet = FRAMEWORK_OK;
    }

    nRet = pthread_create(&sh_msgMgrRxTask, &attr, P_MSG_MANAGER_RxTask, NULL);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("pthread_create() is failed!! (P_MSG_MANAGER_RxTask) [nRet:%d]", nRet);
    }
    else
    {
        PrintTrace("P_MSG_MANAGER_RxTask() is successfully created.");
        nRet = FRAMEWORK_OK;
    }

#if defined(CONFIG_PTHREAD_JOINABLE)
    nRet = pthread_join(sh_msgMgrTxTask, NULL);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("pthread_join() is failed!! (P_MSG_MANAGER_TxTask) [nRet:%d]", nRet);
    }
    else
    {
        PrintDebug("P_MSG_MANAGER_TxTask() is successfully joined.");
        nRet = FRAMEWORK_OK;
    }

    nRet = pthread_join(sh_msgMgrRxTask, NULL);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("pthread_join() is failed!! (P_MSG_MANAGER_RxTask) [nRet:%d]", nRet);
    }
    else
    {
        PrintDebug("P_MSG_MANAGER_RxTask() is successfully joined.");
        nRet = FRAMEWORK_OK;
    }
#endif
	return nRet;
}

#if defined(CONFIG_TEMP_OBU_TEST)
int32_t P_MSG_MANAGER_CreateObuTask(void)
{
	int32_t nRet = FRAMEWORK_ERROR;
	pthread_t h_TxTask;
	pthread_t h_RxTask;

	void *pTxTaskRet;
	void *pRxTaskRet;

	pthread_create(&h_TxTask, NULL, MSG_MANAGER_TxTask, NULL);
	pthread_create(&h_RxTask, NULL, MSG_MANAGER_RxTask, NULL);

	pthread_join(h_TxTask, &pTxTaskRet);
	pthread_join(h_RxTask, &pRxTaskRet);

    nRet = FRAMEWORK_OK;

	return nRet;
}
#endif

int32_t MSG_MANAGER_Transmit(MSG_MANAGER_TX_T *pstMsgMgrTx, DB_V2X_T *pstDbV2x, void *pPayload)
{
    int32_t nRet = FRAMEWORK_ERROR;
    MSG_MANAGER_TX_EVENT_MSG_T stEventMsg;

    if(pstMsgMgrTx == NULL)
    {
        PrintError("pstMsgMgrTx == NULL!!");
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

    stEventMsg.pstMsgManagerTx = pstMsgMgrTx;
    stEventMsg.pstDbV2x = pstDbV2x;
    stEventMsg.pPayload = pPayload;

    if(msgsnd(s_nMsgTxTaskMsgId, &stEventMsg, sizeof(DB_MANAGER_EVENT_MSG_T), IPC_NOWAIT) == FRAMEWORK_MSG_ERR)
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

int32_t MSG_MANAGER_Receive(MSG_MANAGER_RX_T *pstMsgMgrRx, DB_V2X_T *pstDbV2x, void *pPayload)
{
    int32_t nRet = FRAMEWORK_ERROR;

    PrintWarn("TODO");

    if(pstMsgMgrRx == NULL)
    {
        PrintError("pstMsgMgrRx == NULL!!");
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

int32_t MSG_MANAGER_SetLog(MSG_MANAGER_T *pstMsgManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstMsgManager == NULL)
    {
        PrintError("pstMsgManager == NULL!!");
        return nRet;
    }

    s_bMsgMgrLog = pstMsgManager->bLogLevel;
    PrintTrace("SET:s_bMsgMgrLog [%s]", s_bMsgMgrLog == ON ? "ON" : "OFF");

    if (s_bMsgMgrLog == ON)
    {
        switch (pstMsgManager->eLogLevel)
        {
            case LOG_LEVEL_ALL:
                PrintWarn("TODO");
                break;

            case LOG_LEVEL_DEBUG:
                PrintWarn("TODO");
                break;

            case LOG_LEVEL_WARN:
                PrintWarn("TODO");
                break;

            case LOG_LEVEL_ERROR:
                PrintWarn("TODO");
                break;

            default:
                PrintError("Unknown Log Type [%d]", s_bMsgMgrLog);
                break;
        }
    }

    nRet = FRAMEWORK_OK;

    return nRet;
}

int32_t MSG_MANAGER_Open(MSG_MANAGER_T *pstMsgManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstMsgManager == NULL)
    {
        PrintError("pstMsgManager == NULL!!");
        return nRet;
    }

    nRet = P_MSG_MANAGER_ConnectV2XDevice(pstMsgManager);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("P_MSG_MANAGER_ConnectV2XDevice() is failed!!, nRet[%d]", nRet);
        return nRet;
    }

#if defined(CONFIG_EXT_DATA_FORMAT)
    pstMsgManager->stExtMsgWsr.ucAction = eMSG_MANAGER_EXT_MSG_ACTION_ADD;

    switch(pstMsgManager->eDeviceType)
    {
        case DB_V2X_DEVICE_TYPE_OBU:
        {
            PrintTrace("DB_V2X_DEVICE_TYPE_OBU");
            nRet = P_MSG_MANAGER_SetV2xWsrSetting(pstMsgManager);
            if (nRet != FRAMEWORK_OK)
            {
                PrintError("P_MSG_MANAGER_SetV2xWsrSetting() is failed!!, nRet[%d]", nRet);
                return nRet;
            }

            /* Set I2V */
            pstMsgManager->stExtMsgWsr.unPsid = SVC_CP_I2V_PSID;
            nRet = P_MSG_MANAGER_SetV2xWsrSetting(pstMsgManager);
            if (nRet != FRAMEWORK_OK)
            {
                PrintError("P_MSG_MANAGER_SetV2xWsrSetting() is failed!!, nRet[%d]", nRet);
                return nRet;
            }

            /* Reset Value as V2V */
            pstMsgManager->stExtMsgWsr.unPsid = SVC_CP_V2V_PSID;
            break;
        }

        case DB_V2X_DEVICE_TYPE_RSU:
        {
            PrintTrace("DB_V2X_DEVICE_TYPE_RSU");
            nRet = P_MSG_MANAGER_SetV2xWsrSetting(pstMsgManager);
            if (nRet != FRAMEWORK_OK)
            {
                PrintError("P_MSG_MANAGER_SetV2xWsrSetting() is failed!!, nRet[%d]", nRet);
                return nRet;
            }
            break;
        }

        default:
            PrintError("Error! unknown device type[%d]", pstMsgManager->eDeviceType);
            break;

    }

#else
	nRet = P_MSG_MANAGER_SetV2xWsrSetting();
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("P_MSG_MANAGER_SetV2xWsrSetting() is failed!!, nRet[%d]", nRet);
        return nRet;
    }
#endif

#if defined(CONFIG_TEMP_OBU_TEST)
    nRet = P_MSG_MANAGER_CreateObuTask();
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("P_MSG_MANAGER_CreateObuTask() is failed!!, nRet[%d]", nRet);
        return nRet;
    }
#endif

    s_bFirstPacket = TRUE;

    return nRet;
}

int32_t MSG_MANAGER_Close(MSG_MANAGER_T *pstMsgManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstMsgManager == NULL)
    {
        PrintError("pstMsgManager == NULL!!");
        return nRet;
    }

    if(s_nSocketHandle != 0)
    {
        close(s_nSocketHandle);
        s_nSocketHandle = 0;
        PrintTrace("Close Connection of V2X Device is successed! [s_nSocketHandle:0x%x]", s_nSocketHandle);
        nRet = FRAMEWORK_OK;
    }
    else
    {
        PrintError("Disconnected [s_nSocketHandle:0x%x]", s_nSocketHandle);
    }

    s_bFirstPacket = TRUE;

    return nRet;
}

int32_t MSG_MANAGER_Start(MSG_MANAGER_T *pstMsgManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    PrintWarn("TODO");

    if(pstMsgManager == NULL)
    {
        PrintError("pstMsgManager == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t MSG_MANAGER_Stop(MSG_MANAGER_T *pstMsgManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    PrintWarn("TODO");

    if(pstMsgManager == NULL)
    {
        PrintError("pstMsgManager == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t MSG_MANAGER_Status(MSG_MANAGER_T *pstMsgManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    PrintWarn("TODO");

    if(pstMsgManager == NULL)
    {
        PrintError("pstMsgManager == NULL!!");
        return nRet;
    }

    return nRet;
}

int32_t MSG_MANAGER_Init(MSG_MANAGER_T *pstMsgManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstMsgManager == NULL)
    {
        PrintError("pstMsgManager == NULL!!");
        return nRet;
    }

    if((s_nDbTaskMsgId = msgget(s_dbTaskMsgKey, IPC_CREAT|0666)) == FRAMEWORK_MSG_ERR)
    {
        PrintError("msgget() is failed!");
        return nRet;
    }
    else
    {
        P_MSG_NABAGER_PrintMsgInfo(s_nDbTaskMsgId);
        nRet = FRAMEWORK_OK;
    }

    if((s_nMsgTxTaskMsgId = msgget(s_MsgTxTaskMsgKey, IPC_CREAT|0666)) == FRAMEWORK_MSG_ERR)
    {
        PrintError("msgget() is failed!");
        return nRet;
    }
    else
    {
        P_MSG_NABAGER_PrintMsgInfo(s_nMsgTxTaskMsgId);
        nRet = FRAMEWORK_OK;
    }

    if((s_nMsgRxTaskMsgId = msgget(s_MsgRxTaskMsgKey, IPC_CREAT|0666)) == FRAMEWORK_MSG_ERR)
    {
        PrintError("msgget() is failed!");
        return nRet;
    }
    else
    {
        P_MSG_NABAGER_PrintMsgInfo(s_nMsgRxTaskMsgId);
        nRet = FRAMEWORK_OK;
    }

    nRet = P_MSG_MANAGER_CreateTask();
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("P_MSG_MANAGER_CreateTask() is failed!!, nRet[%d]", nRet);
        return nRet;
    }

    s_bMsgMgrLog = pstMsgManager->bLogLevel;
    PrintDebug("s_bMsgMgrLog [%s]", s_bMsgMgrLog == ON ? "ON" : "OFF");

    return nRet;
}

int32_t MSG_MANAGER_DeInit(MSG_MANAGER_T *pstMsgManager)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(pstMsgManager == NULL)
    {
        PrintError("pstMsgManager == NULL!!");
        return nRet;
    }

    nRet = P_MSG_MANAGER_DisconnectV2XDevice();
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("P_MSG_MANAGER_DisconnectV2XDevice() is failed!!, nRet[%d]", nRet);
        return nRet;
    }

    return nRet;
}

