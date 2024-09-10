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
* @file cli_msg.c
*
* @note
*
* CLI MSG Source
*
******************************************************************************/


/***************************** Include ***************************************/
#include "cli.h"
#include "app.h"
#include "db_v2x.h"
#include "db_manager.h"
#include "msg_manager.h"
#include "framework.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#if defined(CONFIG_WEBSOCKET)
#include <libwebsockets.h>
#endif

/***************************** Definition ************************************/
//#define CONFIG_CLI_MSG_DEBUG        (1)
const int MAX_LINE = 2048;
const int PORT = 50531;
const int BACKLOG = 10;
const int LISTENQ = 6666;
const int MAX_CONNECT = 20;

/***************************** Static Variable *******************************/
static MSG_MANAGER_TX_T s_stMsgManagerTx;
static MSG_MANAGER_RX_T s_stMsgManagerRx;
static DB_V2X_T s_stDbV2x;
static bool s_bCliMsgLog = FALSE;

#if defined(CONFIG_WEBSOCKET)
struct per_session_data {
};

#define CLI_MSG_DEBUG                       (1)

static FILE *s_hWebSocket = NULL;
static struct lws_context *s_pLwsContext = NULL;
static long s_lLastPos = 0;
static char s_chLastLine[MSG_MANAGER_WEBSOCKET_BUF_MAX_LEN] = "";
static MSG_MANAGER_FILE_TYPE_E s_eFileType = eMSG_MANAGER_FILE_TYPE_RX;
#endif
/***************************** Function Protype ******************************/
#if defined(CONFIG_WEBSOCKET)
static int32_t P_MSG_MANAGER_WebSocketCallback(struct lws *pstWsi, enum lws_callback_reasons eCbReason, void *pvUser, void *pvIn, size_t szLen)
{
    int32_t nRet = FRAMEWORK_ERROR;
    char chLine[MSG_MANAGER_WEBSOCKET_BUF_MAX_LEN];
    long lFileSize = 0;

    UNUSED(pvUser);
    UNUSED(pvIn);
    UNUSED(szLen);

    if (pstWsi == NULL)
    {
        PrintError("pstWsi is NULL!");
        return nRet;
    }

    switch (eCbReason)
    {
        case LWS_CALLBACK_ESTABLISHED:
            PrintWarn("LWS_CALLBACK_ESTABLISHED");

            if(s_eFileType == eMSG_MANAGER_FILE_TYPE_TX)
            {
                s_hWebSocket = fopen(MSG_MANAGER_WEBSERVER_FILE_TX, "r");
                PrintTrace("MSG_MANAGER_WEBSERVER_FILE_TX:%s", MSG_MANAGER_WEBSERVER_FILE_TX);
            }
            else if(s_eFileType == eMSG_MANAGER_FILE_TYPE_RX)
            {
                s_hWebSocket = fopen(MSG_MANAGER_WEBSERVER_FILE_RX, "r");
                PrintTrace("MSG_MANAGER_WEBSERVER_FILE_RX:%s", MSG_MANAGER_WEBSERVER_FILE_RX);
            }
            else if(s_eFileType == eMSG_MANAGER_FILE_TYPE_SAMPLE_TX)
            {
                nRet = system("cp -f ~/work/athena/src/apps/html/db/"MSG_MANAGER_WEBSERVER_FILE_SAMPLE_TX " "MSG_MANAGER_DB_FILE_PATH);
                if(nRet < FRAMEWORK_OK)
                {
                    PrintError("system() is failed! [nRet:%d], locate your source code at ~/work/athena", nRet);
                    return nRet;
                }

                s_hWebSocket = fopen("/tmp/"MSG_MANAGER_WEBSERVER_FILE_SAMPLE_TX, "r");
                PrintTrace("MSG_MANAGER_WEBSERVER_FILE_SAMPLE_TX:/tmp/%s", MSG_MANAGER_WEBSERVER_FILE_SAMPLE_TX);
            }
            else if(s_eFileType == eMSG_MANAGER_FILE_TYPE_SAMPLE_RX)
            {
                nRet = system("cp -f ~/work/athena/src/apps/html/db/"MSG_MANAGER_WEBSERVER_FILE_SAMPLE_RX " "MSG_MANAGER_DB_FILE_PATH);
                if(nRet < FRAMEWORK_OK)
                {
                    PrintError("system() is failed! [nRet:%d], locate your source code at ~/work/athena", nRet);
                    return nRet;
                }

                s_hWebSocket = fopen("/tmp/"MSG_MANAGER_WEBSERVER_FILE_SAMPLE_RX, "r");
                PrintTrace("eMSG_MANAGER_FILE_TYPE_SAMPLE_RX:/tmp/%s", MSG_MANAGER_WEBSERVER_FILE_SAMPLE_RX);
            }
            else
            {
                PrintError("unknown file format[%d]", s_eFileType);
            }

            if(s_hWebSocket == NULL)
            {
                PrintError("s_hWebSocket is NULL!!");
                return nRet;
            }
            else
            {
                PrintTrace("file type[%d] is opened", s_eFileType);
            }

            if((s_eFileType == eMSG_MANAGER_FILE_TYPE_SAMPLE_TX) || (s_eFileType == eMSG_MANAGER_FILE_TYPE_SAMPLE_RX))
            {
                /* read from the first */
                fseek(s_hWebSocket, 0, SEEK_SET);
            }
            else
            {
                fseek(s_hWebSocket, 0, SEEK_END);
                s_lLastPos = ftell(s_hWebSocket);
                PrintDebug("Initial file position: %ld\n", s_lLastPos);
            }

            lws_callback_on_writable(pstWsi);
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            if((s_eFileType == eMSG_MANAGER_FILE_TYPE_SAMPLE_TX) || (s_eFileType == eMSG_MANAGER_FILE_TYPE_SAMPLE_RX))
            {
                if (fgets(chLine, sizeof(chLine), s_hWebSocket))
                {
                    if(s_bCliMsgLog == TRUE)
                    {
                        PrintDebug("Read line: %s", chLine);
                    }

                    lws_write(pstWsi, (unsigned char *)chLine, strlen(chLine), LWS_WRITE_TEXT);
                }
                else
                {
                    fseek(s_hWebSocket, 0, SEEK_SET);
                }
            }
            else
            {
                fseek(s_hWebSocket, 0, SEEK_END);
                lFileSize = ftell(s_hWebSocket);

                for (long i = lFileSize - 2; i >= 0; i--)
                {
                    fseek(s_hWebSocket, i, SEEK_SET);
                    if (fgetc(s_hWebSocket) == '\n')
                    {
                        s_lLastPos = ftell(s_hWebSocket);
                        break;
                    }
                }

                fseek(s_hWebSocket, s_lLastPos, SEEK_SET);
                if (fgets(chLine, sizeof(chLine), s_hWebSocket))
                {
                    if(s_bCliMsgLog == TRUE)
                    {
                        PrintDebug("Read last line: %s", chLine);
                    }

                    strcpy(s_chLastLine, chLine);
                    lws_write(pstWsi, (unsigned char *)chLine, strlen(chLine), LWS_WRITE_TEXT);
                }
                else if (strlen(s_chLastLine) > 0)
                {
                    PrintWarn("Sending last line again: %s", s_chLastLine);
                    lws_write(pstWsi, (unsigned char *)s_chLastLine, strlen(s_chLastLine), LWS_WRITE_TEXT);
                }
            }

            usleep(SVC_PLATOONING_TX_DELAY*1000);
            lws_callback_on_writable(pstWsi);
            break;

        case LWS_CALLBACK_CLOSED:
            PrintWarn("LWS_CALLBACK_CLOSED");
            if (s_hWebSocket != NULL)
            {
                fclose(s_hWebSocket);
                s_hWebSocket = NULL;
            }
            break;

        default:
            PrintError("unknown reason[%d]", eCbReason);
            break;
    }

    nRet = FRAMEWORK_OK;

    return nRet;
}

void P_MSG_MANAGER_WebSocketProcess(void)
{
    lws_service(s_pLwsContext, 50);
}

static int32_t P_MSG_MANAGER_WebSocketInit(void)
{
    int32_t nRet = FRAMEWORK_ERROR;

    static struct lws_protocols protocols[] =
    {
        {
            "v2x-web-protocol",
            P_MSG_MANAGER_WebSocketCallback,
            sizeof(struct per_session_data),
            0,
            0,
            NULL,
            0
        },
        { NULL, NULL, 0, 0, 0, NULL, 0 }
    };

    struct lws_context_creation_info info = {
        .port = MSG_MANAGER_WEBSOCKET_PORT,
        .protocols = protocols
    };

    s_pLwsContext = lws_create_context(&info);
    if (!s_pLwsContext)
    {
        PrintError("Failed to create WebSocket context.\n");
        return nRet;
    }

    nRet = FRAMEWORK_OK;

    return nRet;
}
static int32_t P_MSG_MANAGER_WebSocketDeInit(void)
{
    int32_t nRet = FRAMEWORK_ERROR;

    if(s_pLwsContext != NULL)
    {
        lws_context_destroy(s_pLwsContext);
        PrintDebug("P_MSG_MANAGER_WebSocketDeInit() is deinit.");
        nRet = FRAMEWORK_OK;
    }
    else
    {
        PrintError("s_pLwsContext is NULL!");
    }

    return nRet;
}

void *P_CLI_MSG_SebSocketTask(void *fd)
{
    UNUSED(fd);

    while (1)
    {
        (void)P_MSG_MANAGER_WebSocketProcess();
    }

    return NULL;

}

int32_t P_CLI_MSG_SebSocketStart(void)
{
    int32_t nRet = APP_ERROR;
    pthread_t h_stRecvTid;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    nRet = pthread_create(&h_stRecvTid, &attr, P_CLI_MSG_SebSocketTask, NULL);
    if (nRet != FRAMEWORK_OK)
    {
        PrintError("pthread_create() is failed!! (P_CLI_MSG_SebSocketTask) [nRet:%d]", nRet);
    }
    else
    {
        PrintTrace("P_CLI_MSG_SebSocketTask() is successfully created.");
        nRet = FRAMEWORK_OK;
    }

    return nRet;
}
#endif

void *P_CLI_MSG_TcpServerTask(void *fd)
{
    int h_nSocket = *(int *)fd;
    char cMsgBuf[MAX_LINE];
    int nRevLen;

    while(1)
    {
        memset(cMsgBuf , 0 , MAX_LINE);
        if((nRevLen = recv(h_nSocket , cMsgBuf , MAX_LINE , 0)) == -1)
        {
            PrintError("recv error.\n");
            break;
        }

        cMsgBuf[nRevLen] = '\0';

        if (nRevLen == 0)
        {
            PrintDebug("Client closed.\n");
            close(h_nSocket);
            break;
        }

        if(strcmp(cMsgBuf , "exit") == 0)
        {
            PrintDebug("Client closed.\n");
            close(h_nSocket);
            break;
        }

        PrintTrace("Received message from client: %s", cMsgBuf);
    }

	return NULL;
}

int32_t P_CLI_MSG_TcpServerStart(void)
{
    int32_t nRet = APP_ERROR;
    int h_nListen , h_nConn;
    pthread_t h_stRecvTid;
    struct sockaddr_in stServerAddr, stClientAddr;
    socklen_t stClientLen = sizeof(stClientAddr);
    char cMsgBuf[MAX_LINE];
    memset(cMsgBuf, 0, MAX_LINE);
    int nVal = 1;

    if((h_nListen = socket(AF_INET , SOCK_STREAM , 0)) == -1)
    {
        PrintError("socket error.\n");
        return nRet;
    }

    bzero(&stServerAddr, sizeof(stServerAddr));

    stServerAddr.sin_family = AF_INET;
    stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    stServerAddr.sin_port = htons(PORT);

    if (setsockopt(h_nListen, SOL_SOCKET, SO_REUSEADDR, (char *) &nVal, sizeof nVal) < 0) {
        PrintError("setsockopt");
        close(h_nListen);
        return -1;
    }

    if(bind(h_nListen, (struct sockaddr *)&stServerAddr, sizeof(stServerAddr)) < 0)
    {
        PrintError("bind error.");
        return nRet;
    }

    if(listen(h_nListen, LISTENQ) < 0)
    {
        PrintError("listen error.");
        return nRet;
    }

    if((h_nConn = accept(h_nListen, (struct sockaddr *)&stClientAddr, &stClientLen)) < 0)
    {
        PrintError("accept error.");
        return nRet;
    }

    PrintDebug("server: got connection from %s", inet_ntoa(stClientAddr.sin_addr));

    if(pthread_create(&h_stRecvTid, NULL, P_CLI_MSG_TcpServerTask, &h_nConn) == -1)
    {
        PrintError("pthread create error.");
        return nRet;
    }


    while(fgets(cMsgBuf, MAX_LINE, stdin) != NULL)
    {
        if(strcmp(cMsgBuf, "exit\n") == 0)
        {
            close(h_nConn);
            exit(0);
        }

        if(send(h_nConn, cMsgBuf, strlen(cMsgBuf), 0) == -1)
        {
            PrintError("send error.");
            return nRet;
        }
    }

    return nRet;
}

void *P_CLI_MSG_TcpClientTask(void *fd)
{
    int h_nSocket = *(int *)fd;
    char cMsgBuf[MAX_LINE];
    int nRevLen;

    while(1)
    {
        memset(cMsgBuf, 0, MAX_LINE);
        if((nRevLen = recv(h_nSocket, cMsgBuf, MAX_LINE , 0)) == -1)
        {
            PrintError("recv error.");
            break;
        }
        cMsgBuf[nRevLen] = '\0';

        if (nRevLen == 0)
        {
            PrintDebug("Server is closed.");
            close(h_nSocket);
            break;
        }

        if(strcmp(cMsgBuf , "exit") == 0)
        {
            PrintDebug("Server is closed.");
            close(h_nSocket);
            break;
        }

        PrintTrace("Received message from server: %s", cMsgBuf);
    }

	return NULL;
}

int32_t P_CLI_MSG_TcpClientStart(char *pcIpAddr)
{
    int32_t nRet = APP_ERROR;
    int h_nSocket;
    pthread_t h_stRecvTid;
    struct sockaddr_in stServerAddr;
    char cMsgBuf[MAX_LINE];
    memset(cMsgBuf , 0 , MAX_LINE);

    if(pcIpAddr == NULL)
    {
        PrintError("pucIpAddr is NULL!");
        return nRet;
    }

    PrintWarn("Server IP [%s]", pcIpAddr);

    if((h_nSocket = socket(AF_INET , SOCK_STREAM , 0)) == -1)
    {
        PrintError("socket error");
        return nRet;
    }

    bzero(&stServerAddr , sizeof(stServerAddr));

    stServerAddr.sin_family = AF_INET;
    stServerAddr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, pcIpAddr, &stServerAddr.sin_addr) < 0)
    {
        PrintError("inet_pton error for %s", pcIpAddr);
        return nRet;
    }

    if( connect(h_nSocket, (struct sockaddr *)&stServerAddr, sizeof(stServerAddr)) < 0)
    {
        PrintError("connect error");
        return nRet;
    }

    if(pthread_create(&h_stRecvTid, NULL, P_CLI_MSG_TcpClientTask, &h_nSocket) == -1)
    {
        PrintError("pthread create error");
        return nRet;
    }

    while(fgets(cMsgBuf, MAX_LINE, stdin) != NULL)
    {
        if(strcmp(cMsgBuf, "exit\n") == 0)
        {
            PrintWarn("Exit");
            close(h_nSocket);
            break;
        }

        if(send(h_nSocket, cMsgBuf, strlen(cMsgBuf) , 0) == -1)
        {
            PrintError("send error");
            return nRet;
        }
    }

    return nRet;
}

int32_t P_CLI_MSG_TcpMultiServerStart(void)
{
    int32_t nRet = APP_ERROR;
    int h_nListen, h_nConn;
    pthread_t h_stRecvTid;
    struct sockaddr_in stServerAddr, stClientAddr;
    socklen_t stClientLen = sizeof(stClientAddr);
    int nVal = 1;

    if((h_nListen = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        PrintError("socket error.\n");
        return nRet;
    }

    stServerAddr.sin_family = AF_INET;
    stServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    stServerAddr.sin_port = htons(PORT);

    if (setsockopt(h_nListen, SOL_SOCKET, SO_REUSEADDR, (char *) &nVal, sizeof nVal) < 0) {
        PrintError("setsockopt");
        close(h_nListen);
        return -1;
    }

    if(bind(h_nListen, (struct sockaddr *)&stServerAddr, sizeof(stServerAddr)) < 0)
    {
        PrintError("bind error.");
        return nRet;
    }

    if(listen(h_nListen, LISTENQ) < 0)
    {
        PrintError("listen error.");
        return nRet;
    }

    while(1)
    {
        if((h_nConn = accept(h_nListen, (struct sockaddr *)&stClientAddr, &stClientLen)) < 0)
        {
            PrintError("accept error.");
            return nRet;
        }

        PrintDebug("server: got connection from %s", inet_ntoa(stClientAddr.sin_addr));

        if(pthread_create(&h_stRecvTid, NULL, P_CLI_MSG_TcpServerTask, &h_nConn) == -1)
        {
            PrintError("pthread create error.");
            return nRet;
        }
    }

    return nRet;
}

int32_t P_CLI_MSG_TcpMultiClientStart(char *pcIpAddr, char *pcId)
{
    int32_t nRet = APP_ERROR;
    int h_nSocket;
    pthread_t h_stRecvTid;
    struct sockaddr_in stServerAddr;
    char cMsgBuf[MAX_LINE];
    memset(cMsgBuf, 0, MAX_LINE);

    if(pcIpAddr == NULL || pcId == NULL)
    {
        PrintError("IP address or ID is NULL!");
        return nRet;
    }

    PrintWarn("Connecting to server IP [%s] with ID [%s]", pcIpAddr, pcId);

    if((h_nSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        PrintError("socket error");
        return nRet;
    }

    stServerAddr.sin_family = AF_INET;
    stServerAddr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, pcIpAddr, &stServerAddr.sin_addr) < 0)
    {
        PrintError("inet_pton error for %s", pcIpAddr);
        return nRet;
    }

    if(connect(h_nSocket, (struct sockaddr *)&stServerAddr, sizeof(stServerAddr)) < 0)
    {
        PrintError("connect error");
        return nRet;
    }

    // Send the client ID to the server upon connection
    if(send(h_nSocket, pcId, strlen(pcId), 0) == -1)
    {
        PrintError("send ID error");
        return nRet;
    }

    if(pthread_create(&h_stRecvTid, NULL, P_CLI_MSG_TcpClientTask, &h_nSocket) == -1)
    {
        PrintError("pthread create error");
        return nRet;
    }

    while(fgets(cMsgBuf, MAX_LINE, stdin) != NULL)
    {
        if(strcmp(cMsgBuf, "exit\n") == 0)
        {
            PrintWarn("Exit");
            close(h_nSocket);
            break;
        }

        if(send(h_nSocket, cMsgBuf, strlen(cMsgBuf), 0) == -1)
        {
            PrintError("send error");
            return nRet;
        }
    }

    return nRet;
}

void P_CLI_MSG_ShowTxSettings(void)
{
    PrintTrace("========================================================");
    PrintWarn("MSG V2X Tx Info>");
    PrintDebug(" ePayloadType[%d]", s_stMsgManagerTx.ePayloadType);
    PrintDebug(" eCommType[%d]", s_stMsgManagerTx.eCommType);
    PrintDebug(" eSignId[%d]", s_stMsgManagerTx.eSignId);
    PrintDebug(" eV2xFreq[%d]", s_stMsgManagerTx.eV2xFreq);
    PrintDebug(" ePriority[%d]", s_stMsgManagerTx.ePriority);
    PrintDebug(" eV2xDataRate[%d]", s_stMsgManagerTx.eV2xDataRate);
    PrintDebug(" eV2xTimeSlot[%d]", s_stMsgManagerTx.eV2xTimeSlot);
    PrintDebug(" unPsid[%d]", s_stMsgManagerTx.unPsid);
    PrintDebug(" nTxPower[%d]", s_stMsgManagerTx.nTxPower);
    PrintDebug(" unTxCount[%d]", s_stMsgManagerTx.unTxCount);
    PrintDebug(" unTxDelay[%d ms]", s_stMsgManagerTx.unTxDelay);
    for(int i = 0; i < MSG_MANAGER_MAC_LENGTH; i++)
    {
        PrintDebug(" unTxCount[i:%d][0x%x]", i, s_stMsgManagerTx.uchPeerMacAddr[i]);
    }

    PrintDebug(" unTransmitterProfileId[%d]", s_stMsgManagerTx.unTransmitterProfileId);
    PrintDebug(" unPeerL2Id[%d]", s_stMsgManagerTx.unPeerL2Id);

    PrintWarn("DB V2X Info>");
    PrintDebug(" eDeviceType[%d]", s_stDbV2x.eDeviceType);
    PrintDebug(" eTeleCommType[%d]", s_stDbV2x.eTeleCommType);
    PrintDebug(" unDeviceId[%d]", s_stDbV2x.unDeviceId);
    PrintDebug(" eServiceId[%d]", s_stDbV2x.eServiceId);
    PrintDebug(" eActionType[%d]", s_stDbV2x.eActionType);
    PrintDebug(" eRegionId[%d]", s_stDbV2x.eRegionId);
    PrintDebug(" ePayloadType[%d]", s_stDbV2x.ePayloadType);
    PrintDebug(" eCommId[%d]", s_stDbV2x.eCommId);
    PrintDebug(" usDbVer[%d.%d]", s_stDbV2x.usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, s_stDbV2x.usDbVer & CLI_DB_V2X_MINOR_MASK);
    PrintDebug(" usHwVer[0x%x]", s_stDbV2x.usHwVer);
    PrintDebug(" usSwVer[0x%x]", s_stDbV2x.usSwVer);
    PrintTrace("========================================================");
}

int32_t P_CLI_MSG_SetSettings(MSG_MANAGER_TX_T *pstMsgManagerTx, DB_V2X_T *pstDbV2x)
{
    int32_t nRet = APP_ERROR;

    if((pstMsgManagerTx == NULL) || (pstDbV2x == NULL))
    {
        PrintError("pstMsgManagerTx, or pstDbV2x is NULL!!");
    }

    memcpy(&s_stMsgManagerTx, pstMsgManagerTx, sizeof(MSG_MANAGER_TX_T));
    memcpy(&s_stDbV2x, pstDbV2x, sizeof(DB_V2X_T));

    nRet = APP_OK;

    return nRet;
}

int32_t P_CLI_MSG_GetSettings(MSG_MANAGER_TX_T *pstMsgManagerTx, DB_V2X_T *pstDbV2x)
{
    int32_t nRet = APP_ERROR;

    if((pstMsgManagerTx == NULL) || (pstDbV2x == NULL))
    {
        PrintError("pstMsgManagerTx, or pstDbV2x is NULL!!");
    }

    memcpy(pstMsgManagerTx, &s_stMsgManagerTx, sizeof(MSG_MANAGER_TX_T));
    memcpy(pstDbV2x, &s_stDbV2x, sizeof(DB_V2X_T));

    nRet = APP_OK;

    return nRet;
}

int32_t P_CLI_MSG_SetDefaultSettings(void)
{
    int32_t nRet = APP_ERROR;

    MSG_MANAGER_TX_T stMsgManagerTx;
    DB_V2X_T stDbV2x;

    stMsgManagerTx.ePayloadType = eMSG_MANAGER_PAYLOAD_TYPE_RAW;
    stMsgManagerTx.eCommType = eMSG_MANAGER_COMM_TYPE_5GNRV2X;
    stMsgManagerTx.eSignId = eMSG_MANAGER_SIGN_ID_UNSECURED;
    stMsgManagerTx.eV2xFreq = eMSG_MANAGER_V2X_FREQ_5900;
    stMsgManagerTx.ePriority = eMSG_MANAGER_PRIORITY_CV2X_PPPP_0;
    stMsgManagerTx.eV2xDataRate = eMSG_MANAGER_V2X_DATA_RATE_6MBPS;
    stMsgManagerTx.eV2xTimeSlot = eMSG_MANAGER_V2X_TIME_SLOT_CONTINUOUS;
    stMsgManagerTx.unPsid = DB_V2X_PSID;
    stMsgManagerTx.nTxPower = MSG_MANAGER_V2X_TX_POWER;
    stMsgManagerTx.unTxCount = 10;
    stMsgManagerTx.unTxDelay = 100;

    for(int i = 0; i < MSG_MANAGER_MAC_LENGTH; i++)
    {
        stMsgManagerTx.uchPeerMacAddr[i] = 0xFF;
    }

    stMsgManagerTx.unTransmitterProfileId = MSG_MANAGER_V2X_TX_PROFILE_ID;
    stMsgManagerTx.unPeerL2Id = MSG_MANAGER_V2X_TX_PEER_L2_ID;

    stDbV2x.eDeviceType = DB_V2X_DEVICE_TYPE_OBU;
    stDbV2x.eTeleCommType = DB_V2X_TELECOMM_TYPE_5G_PC5_BROADCAST;
    stDbV2x.unDeviceId = CLI_DB_V2X_DEFAULT_DEVICE_ID;
    stDbV2x.eServiceId = DB_V2X_SERVICE_ID_PLATOONING;
    stDbV2x.eActionType = DB_V2X_ACTION_TYPE_REQUEST;
    stDbV2x.eRegionId = DB_V2X_REGION_ID_SEONGNAM;
    stDbV2x.ePayloadType = DB_V2X_PAYLOAD_TYPE_PLATOONING_THROUGHPUT;
    stDbV2x.eCommId = DB_V2X_COMM_ID_V2V;
    stDbV2x.usDbVer = (DB_V2X_VERSION_MAJOR << CLI_DB_V2X_MAJOR_SHIFT) | DB_V2X_VERSION_MINOR;
    stDbV2x.usHwVer = CLI_DB_V2X_DEFAULT_HW_VER;
    stDbV2x.usSwVer = CLI_DB_V2X_DEFAULT_SW_VER;

    nRet = P_CLI_MSG_SetSettings(&stMsgManagerTx, &stDbV2x);
    if(nRet != APP_OK)
    {
        PrintError("P_CLI_MSG_SetSettings() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

static int P_CLI_MSG(CLI_CMDLINE_T *pstCmd, int argc, char *argv[])
{
    int32_t nRet = APP_OK;
    int nFrameWorkRet = FRAMEWORK_ERROR;
    char *pcCmd;
    char cPayload[CLI_DB_V2X_DEFAULT_PAYLOAD_LEN];
    TIME_MANAGER_T *pstTimeManager;
    uint32_t unTxCount = 0;
    int i = 0;

    (void*)memset(&cPayload, 0x00, sizeof(cPayload));

    UNUSED(argc);

    if(argv == NULL)
    {
        PrintError("argv == NULL!!");
        return nRet;
    }

    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_0);
    if (pcCmd == NULL)
    {
        return CLI_CMD_Showusage(pstCmd);
    }
    else
    {
        pcCmd = CLI_CMD_GetArg(pstCmd, CMD_0);
        if(IS_CMD(pcCmd, "test"))
        {
            for(int i = 0; i < CMD_MAX; i++)
            {
                pcCmd = CLI_CMD_GetArg(pstCmd, i);
                PrintDebug("pcCmd[idx:%d][value:%s]", i, pcCmd);
            }
        }
        if(IS_CMD(pcCmd, "log"))
        {
            if(s_bCliMsgLog == TRUE)
            {
                s_bCliMsgLog = FALSE;
            }
            else
            {
                s_bCliMsgLog = TRUE;
            }

            PrintWarn("set s_bCliMsgLog[%d]", s_bCliMsgLog);
        }
#if defined(CONFIG_WEBSOCKET)
        else if(IS_CMD(pcCmd, "web"))
        {
            pcCmd = CLI_CMD_GetArg(pstCmd, CMD_1);
            if(pcCmd != NULL)
            {
                if(IS_CMD(pcCmd, "start"))
                {
                    nRet = P_MSG_MANAGER_WebSocketInit();
                    if (nRet != FRAMEWORK_OK)
                    {
                        PrintError("P_MSG_MANAGER_WebSocketInit() is failed!!, nRet[%d]", nRet);
                        return nRet;
                    }

                    nRet = P_CLI_MSG_SebSocketStart();
                    if (nRet != FRAMEWORK_OK)
                    {
                        PrintError("P_CLI_MSG_SebSocketStart() is failed!!, nRet[%d]", nRet);
                        return nRet;
                    }
                }
                else if(IS_CMD(pcCmd, "stop"))
                {
                    nRet = P_MSG_MANAGER_WebSocketDeInit();
                    if (nRet != FRAMEWORK_OK)
                    {
                        PrintError("P_MSG_MANAGER_WebSocketDeInit() is failed!!, nRet[%d]", nRet);
                        return nRet;
                    }
                }
                else if(IS_CMD(pcCmd, "file"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        s_eFileType = (MSG_MANAGER_FILE_TYPE_E)atoi(pcCmd);

                        PrintTrace("s_eFileTypet[%d]", s_eFileType);
                    }
                    else
                    {
                        PrintError("set file [id], 0:unknown, 1:tx, 2:rx, 3:sample");
                    }

                }
                else
                {
                    PrintError("set file type and web start/stop");
                }
            }
        }
#endif
        else if(IS_CMD(pcCmd, "tx"))
        {
            pstTimeManager = FRAMEWORK_GetTimeManagerInstance();

            (void)TIME_MANAGER_CheckLatencyBegin(pstTimeManager);

            for (unTxCount = 0; unTxCount < s_stMsgManagerTx.unTxCount; unTxCount++)
            {
                nFrameWorkRet = TIME_MANAGER_Get(pstTimeManager);
                if(nFrameWorkRet != FRAMEWORK_OK)
                {
                    PrintError("TIME_MANAGER_Get() is failed! [nRet:%d]", nFrameWorkRet);
                }
                else
                {
                    s_stDbV2x.ulTimeStamp = pstTimeManager->ulTimeStamp;
                }

                s_stDbV2x.ulPayloadLength = CLI_DB_V2X_DEFAULT_PAYLOAD_LEN;
                for(i = 0; i < (int)s_stDbV2x.ulPayloadLength; i++)
                {
                    cPayload[i] = rand();
                }
                s_stDbV2x.ulReserved = CLI_UTIL_GetCrc32((uint8_t*)&cPayload, s_stDbV2x.ulPayloadLength);

#if defined(CONFIG_CLI_MSG_DEBUG)
                (void)P_CLI_MSG_ShowTxSettings();

                PrintTrace("========================================================");
                PrintDebug("ulTimeStamp[%ld]", s_stDbV2x.ulTimeStamp);
                PrintDebug("ulPayloadLength[%d]", s_stDbV2x.ulPayloadLength);
                PrintDebug("cPayload");
                for(i = 0; i < CLI_DB_V2X_DEFAULT_PAYLOAD_LEN; i++)
                {
                    cPayload[i] = rand();
                    PrintDebug("[%d:%d] ", i, cPayload[i]);
                }
                PrintDebug("\r\n");

                PrintDebug("ulReserved[0x%x]", s_stDbV2x.ulReserved);
                PrintTrace("========================================================");
#endif

                nFrameWorkRet = MSG_MANAGER_Transmit(&s_stMsgManagerTx, &s_stDbV2x, (char*)&cPayload);
                if(nFrameWorkRet != FRAMEWORK_OK)
                {
                    PrintError("MSG_MANAGER_Transmit() is failed! [nRet:%d]", nFrameWorkRet);
                }
                else
                {
                    PrintDebug("Tx Success, Counts[%u/%u], Delay[%d ms]", unTxCount + 1, s_stMsgManagerTx.unTxCount, s_stMsgManagerTx.unTxDelay);
                }

                usleep((s_stMsgManagerTx.unTxDelay*USLEEP_MS));
            }

            (void)TIME_MANAGER_CheckLatencyEnd(pstTimeManager);
            (void)TIME_MANAGER_CheckLatencyTime("Tx Total Time", pstTimeManager);
        }
        else if(IS_CMD(pcCmd, "txC"))
        {
            pstTimeManager = FRAMEWORK_GetTimeManagerInstance();

            (void)TIME_MANAGER_CheckLatencyBegin(pstTimeManager);

            for (unTxCount = 0; unTxCount < s_stMsgManagerTx.unTxCount; unTxCount++)
            {
                nFrameWorkRet = TIME_MANAGER_Get(pstTimeManager);
                if(nFrameWorkRet != FRAMEWORK_OK)
                {
                    PrintError("TIME_MANAGER_Get() is failed! [nRet:%d]", nFrameWorkRet);
                }
                else
                {
                    s_stDbV2x.ulTimeStamp = pstTimeManager->ulTimeStamp;
                }

                s_stDbV2x.ulPayloadLength = CLI_DB_V2X_DEFAULT_PAYLOAD_LEN;
                for(i = 0; i < (int)s_stDbV2x.ulPayloadLength; i++)
                {
                    cPayload[i] = unTxCount;
                }
                s_stDbV2x.ulReserved = CLI_UTIL_GetCrc32((uint8_t*)&cPayload, s_stDbV2x.ulPayloadLength);

#if defined(CONFIG_CLI_MSG_DEBUG)
                (void)P_CLI_MSG_ShowTxSettings();

                PrintTrace("========================================================");
                PrintDebug("ulTimeStamp[%ld]", s_stDbV2x.ulTimeStamp);
                PrintDebug("ulPayloadLength[%d]", s_stDbV2x.ulPayloadLength);
                PrintDebug("cPayload");
                for(i = 0; i < CLI_DB_V2X_DEFAULT_PAYLOAD_LEN; i++)
                {
                    cPayload[i] = unTxCount;
                    PrintDebug("[%d:%d] ", i, cPayload[i]);
                }
                PrintDebug("\r\n");

                PrintDebug("ulReserved[0x%x]", s_stDbV2x.ulReserved);
                PrintTrace("========================================================");
#endif

                nFrameWorkRet = MSG_MANAGER_Transmit(&s_stMsgManagerTx, &s_stDbV2x, (char*)&cPayload);
                if(nFrameWorkRet != FRAMEWORK_OK)
                {
                    PrintError("MSG_MANAGER_Transmit() is failed! [nRet:%d]", nFrameWorkRet);
                }
                else
                {
                    PrintDebug("Tx Success, Counts[%u/%u], Delay[%d ms]", unTxCount + 1, s_stMsgManagerTx.unTxCount, s_stMsgManagerTx.unTxDelay);
                }

                usleep((s_stMsgManagerTx.unTxDelay*USLEEP_MS));
            }

            (void)TIME_MANAGER_CheckLatencyEnd(pstTimeManager);
            (void)TIME_MANAGER_CheckLatencyTime("Tx Total Time", pstTimeManager);
        }
        else if(IS_CMD(pcCmd, "open"))
        {
            pcCmd = CLI_CMD_GetArg(pstCmd, CMD_1);
            if(pcCmd != NULL)
            {
                MSG_MANAGER_T *pstMsgManager;

                pstMsgManager = FRAMEWORK_GetMsgManagerInstance();
                PrintDebug("pstMsgManager[0x%p]", pstMsgManager);


#if defined(CONFIG_OBU)
                pstMsgManager->eDeviceType = DB_V2X_DEVICE_TYPE_OBU;
                PrintTrace("CONFIG_OBU is enabled, eDeviceType [%d]", pstMsgManager->eDeviceType);
#elif defined(CONFIG_RSU)
                pstMsgManager->eDeviceType = DB_V2X_DEVICE_TYPE_RSU;
                PrintTrace("CONFIG_RSU is enabled, eDeviceType [%d]", pstMsgManager->eDeviceType);
#else
                PrintError("check device type!!");
                return APP_ERROR;
#endif

                pstMsgManager->pchIfaceName = pcCmd;
                pstMsgManager->stExtMsgWsr.unPsid = MSG_MANAGER_EXT_MSG_V2V_PSID;

                PrintTrace("pstMsgManager->pchIfaceName[%s]", pstMsgManager->pchIfaceName);
                PrintTrace("pstMsgManager->stExtMsgWsr.unPsid[%d]", pstMsgManager->stExtMsgWsr.unPsid);
                pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                if(pcCmd != NULL)
                {
                    pstMsgManager->pchIpAddr = pcCmd;

                    PrintTrace("pstMsgManager->pchIpAddr[%s]", pstMsgManager->pchIpAddr);
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_3);
                    if(pcCmd != NULL)
                    {
                        pstMsgManager->unPort = (uint32_t)atoi(pcCmd);

                        PrintTrace("pstMsgManager->unPort[%d]", pstMsgManager->unPort);
                    }
                    else
                    {
                        PrintError("set port number, e.g. CLI> msg open eth1 ip port");
                    }
                }
                else
                {
                    PrintError("set ip address, e.g. CLI> msg open eth1 ip port");
                }

                nFrameWorkRet = MSG_MANAGER_Open(pstMsgManager);
                if(nFrameWorkRet != FRAMEWORK_OK)
                {
                   PrintError("MSG_MANAGER_Open() is failed! [nRet:%d]", nFrameWorkRet);
                }
            }
            else
            {
                PrintError("set an ethernet interface name, e.g. CLI> msg open eth1 ip port");
            }
        }
        else if(IS_CMD(pcCmd, "close"))
        {
            MSG_MANAGER_T *pstMsgManager;

            pstMsgManager = FRAMEWORK_GetMsgManagerInstance();
            PrintDebug("pstDbManager[0x%p]", pstMsgManager);

            nFrameWorkRet = MSG_MANAGER_Close(pstMsgManager);
            if(nFrameWorkRet != FRAMEWORK_OK)
            {
                PrintError("MSG_MANAGER_Close() is failed! [nRet:%d]", nFrameWorkRet);
            }
        }
        else if(IS_CMD(pcCmd, "info"))
        {
            (void)P_CLI_MSG_ShowTxSettings();
        }
        else if(IS_CMD(pcCmd, "log"))
        {
            pcCmd = CLI_CMD_GetArg(pstCmd, CMD_1);
            if(pcCmd != NULL)
            {
                FRAMEWORK_T *pstFramework;
                pstFramework = APP_GetFrameworkInstance();
                PrintDebug("pstFramework [0x%p]", pstFramework);

                if(IS_CMD(pcCmd, "on"))
                {
                    (void)FRAMEWORK_SetLog(pstFramework, ON);
                }
                else if(IS_CMD(pcCmd, "off"))
                {
                    (void)FRAMEWORK_SetLog(pstFramework, OFF);
                }
                else
                {
                    PrintError("msg log on/off, e.g. msg log on, or msg log off");
                }
            }
            else
            {
                PrintError("msg log on/off, e.g. msg log on, or msg log off");
            }
        }
        else if(IS_CMD(pcCmd, "set"))
        {
            pcCmd = CLI_CMD_GetArg(pstCmd, CMD_1);
            if(pcCmd != NULL)
            {
                MSG_MANAGER_TX_T stMsgManagerTx;
                DB_V2X_T stDbV2x;

                nRet = P_CLI_MSG_GetSettings(&stMsgManagerTx, &stDbV2x);
                if (nRet != APP_OK)
                {
                    PrintError("P_CLI_MSG_GetSettings() is failed! [nRet:%d]", nRet);
                }

                if(IS_CMD(pcCmd, "payload"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        uint32_t unTmp = 0;
                        unTmp = (uint32_t)atoi(pcCmd);

                        PrintDebug("SET:payload type[%d]", unTmp);
                        stMsgManagerTx.ePayloadType = unTmp;
                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else if(IS_CMD(pcCmd, "psid"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        uint32_t unTmp = 0;
                        unTmp = (uint32_t)atoi(pcCmd);

                        PrintDebug("SET:psid[%d]", unTmp);
                        stMsgManagerTx.unPsid = unTmp;
                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else if(IS_CMD(pcCmd, "comm"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        uint32_t unTmp = 0;
                        unTmp = (uint32_t)atoi(pcCmd);

                        PrintDebug("SET:communication type[%d]", unTmp);
                        stMsgManagerTx.eCommType = unTmp;
                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else if(IS_CMD(pcCmd, "tx_power"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        int8_t unTmp = 0;
                        unTmp = (int8_t)atoi(pcCmd);

                        PrintDebug("SET:Tx Power dBm[%d]", unTmp);
                        stMsgManagerTx.nTxPower = unTmp;
                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else if(IS_CMD(pcCmd, "sign"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        uint32_t unTmp = 0;
                        unTmp = (uint32_t)atoi(pcCmd);

                        PrintDebug("SET:Signer ID[%d]", unTmp);
                        stMsgManagerTx.eSignId = unTmp;
                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else if(IS_CMD(pcCmd, "priority"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        uint32_t unTmp = 0;
                        unTmp = (uint32_t)atoi(pcCmd);

                        PrintDebug("SET:Priority[%d]", unTmp);
                        stMsgManagerTx.ePriority = unTmp;
                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else if(IS_CMD(pcCmd, "tx_count"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        uint32_t unTmp = 0;
                        unTmp = (uint32_t)atoi(pcCmd);

                        PrintDebug("SET:Tx Count[%d]", unTmp);
                        stMsgManagerTx.unTxCount = unTmp;
                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else if(IS_CMD(pcCmd, "tx_delay"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        uint32_t unTmp = 0;
                        unTmp = (uint32_t)atoi(pcCmd);

                        PrintDebug("SET:Tx delay[%d]", unTmp);
                        stMsgManagerTx.unTxDelay = unTmp;
                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else if(IS_CMD(pcCmd, "device"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        uint32_t unTmp = 0;
                        unTmp = (uint32_t)atoi(pcCmd);

                        PrintDebug("SET:Device Type[%d]", unTmp);
                        stDbV2x.eDeviceType = unTmp;
                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else if(IS_CMD(pcCmd, "tele_comm"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        uint32_t unTmp = 0;
                        unTmp = (uint32_t)atoi(pcCmd);

                        PrintDebug("SET:Telecommunication Type[%d]", unTmp);
                        stDbV2x.eTeleCommType = unTmp;
                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else if(IS_CMD(pcCmd, "device_id"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        uint32_t unTmp = 0;
                        unTmp = (uint32_t)atoi(pcCmd);

                        PrintDebug("SET:Device ID[%d]", unTmp);
                        stDbV2x.unDeviceId = unTmp;
                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else if(IS_CMD(pcCmd, "service_id"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        uint32_t unTmp = 0;
                        unTmp = (uint32_t)atoi(pcCmd);

                        PrintDebug("SET:Service ID[%d]", unTmp);
                        stDbV2x.eServiceId = unTmp;
                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else if(IS_CMD(pcCmd, "action"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        uint32_t unTmp = 0;
                        unTmp = (uint32_t)atoi(pcCmd);

                        PrintDebug("SET:Action Type[%d]", unTmp);
                        stDbV2x.eActionType = unTmp;
                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else if(IS_CMD(pcCmd, "region"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        uint32_t unTmp = 0;
                        unTmp = (uint32_t)atoi(pcCmd);

                        PrintDebug("SET:Region ID[%d]", unTmp);
                        stDbV2x.eRegionId = unTmp;
                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else if(IS_CMD(pcCmd, "pl_type"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        uint32_t unTmp = 0;
                        unTmp = (uint32_t)atoi(pcCmd);

                        PrintDebug("SET:Payload Type[%d]", unTmp);
                        stDbV2x.ePayloadType = unTmp;
                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else if(IS_CMD(pcCmd, "comm_id"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        uint32_t unTmp = 0;
                        unTmp = (uint32_t)atoi(pcCmd);

                        PrintDebug("SET:Communication ID[%d]", unTmp);
                        stDbV2x.eCommId = unTmp;
                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else
                {
                    PrintError("CMD_2 is NULL, see. msg help");
                }

                nRet = P_CLI_MSG_SetSettings(&stMsgManagerTx, &stDbV2x);
                if (nRet != APP_OK)
                {
                    PrintError("P_CLI_MSG_GetSettings() is failed! [nRet:%d]", nRet);
                }
            }
            else
            {
                PrintError("msg set [OPTIONS]\n"
                           "  payload         [0]Raw, [1]EncodedbyJ2735, [2]eITSK00130, [3]eKETI, [4]eETRI, (default : Raw)\n"
                           "  psid            <decimal number> (default : 32)\n"
                           "  comm            [0] UNKNOWN, [1]DSRC, [2]LTEV2X, [3]5GNRV2X, (default : 5GNRV2X)\n"
                           "  tx_power        tx power dbm (default : 20)\n"
                           "  sign            [0]UNSECURED, [1]CERTIFICATE, [2]DIGEST, [3]ALTERNATE (default : UNSECURED)\n"
                           "  priority        0~7 (default : 0)\n"
                           "  tx_count        total tx count (default : 10)\n"
                           "  tx_delay        msec delay (default : 100ms)\n"
                           "  device          [0]UNKNOWN, [1]OBU, [2]RSU, [3]Contrl Center\n"
                           "  tele_comm       [0]UNKNOWN, [1]4G,  [20]5G Uu, [30]5G PC5, [31]5G PC5 Broadcast, [32]5G PC5 Unicast, [33]5G PC5 Multicast, [34]5G PC5 Groupcast\n"
                           "  device_id       device id (default : 23040015)\n"
                           "  service_id      [0]UNKONWN, [1]Platooning(default), [2]Sensor sharing, [3]Remote driving, [4]Advanced driving\n"
                           "  action          [0]UNKONWN, [1]Request(default), [2]Response\n"
                           "  region          [0]UNKONWN, [1]Seoul, [2]Sejong, [3]Busan, [4]Daegeon, [5]Incheon, [6]Daegu, [7]Daegu KIAPI PG, [8]Cheongju, [9]Seongnam(default)\n"
                           "  pl_type         [0]UNKONWN, [1]SAE J2735 BSM, [2]SAE J2736 PVD, [201]Platooning(default), [301]Sensor sharing, [401]Remote driving, [501] Advanced driving\n"
                           "  comm_id         [0]UNKONWN, [1]V2V(default), [2]V2I,...\n"
                           "");
            }
        }
        else if(IS_CMD(pcCmd, "get"))
        {
            MSG_MANAGER_TX_T stMsgManagerTx;
            DB_V2X_T stDbV2x;

            nRet = P_CLI_MSG_GetSettings(&stMsgManagerTx, &stDbV2x);
            if (nRet != APP_OK)
            {
                PrintError("P_CLI_MSG_GetSettings() is failed! [nRet:%d]", nRet);
            }

            PrintTrace("========================================================");
            PrintWarn("MSG V2X Tx Info>");
            PrintDebug(" ePayloadType[%d]", stMsgManagerTx.ePayloadType);
            PrintDebug(" eCommType[%d]", stMsgManagerTx.eCommType);
            PrintDebug(" eSignId[%d]", stMsgManagerTx.eSignId);
            PrintDebug(" eV2xFreq[%d]", stMsgManagerTx.eV2xFreq);
            PrintDebug(" ePriority[%d]", stMsgManagerTx.ePriority);
            PrintDebug(" eV2xDataRate[%d]", stMsgManagerTx.eV2xDataRate);
            PrintDebug(" eV2xTimeSlot[%d]", stMsgManagerTx.eV2xTimeSlot);
            PrintDebug(" unPsid[%d]", stMsgManagerTx.unPsid);
            PrintDebug(" nTxPower[%d]", stMsgManagerTx.nTxPower);
            PrintDebug(" unTxCount[%d]", stMsgManagerTx.unTxCount);
            PrintDebug(" unTxDelay[%d ms]", stMsgManagerTx.unTxDelay);
            for(int i = 0; i < MSG_MANAGER_MAC_LENGTH; i++)
            {
                PrintDebug(" unTxCount[i:%d][0x%x]", i, stMsgManagerTx.uchPeerMacAddr[i]);
            }

            PrintDebug(" unTransmitterProfileId[%d]", stMsgManagerTx.unTransmitterProfileId);
            PrintDebug(" unPeerL2Id[%d]", stMsgManagerTx.unPeerL2Id);

            PrintWarn("DB V2X Info>");
            PrintDebug(" eDeviceType[%d]", stDbV2x.eDeviceType);
            PrintDebug(" eTeleCommType[%d]", stDbV2x.eTeleCommType);
            PrintDebug(" unDeviceId[%d]", stDbV2x.unDeviceId);
            PrintDebug(" eServiceId[%d]", stDbV2x.eServiceId);
            PrintDebug(" eActionType[%d]", stDbV2x.eActionType);
            PrintDebug(" eRegionId[%d]", stDbV2x.eRegionId);
            PrintDebug(" ePayloadType[%d]", stDbV2x.ePayloadType);
            PrintDebug(" eCommId[%d]", stDbV2x.eCommId);
            PrintDebug(" usDbVer[%d.%d]", stDbV2x.usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, s_stDbV2x.usDbVer & CLI_DB_V2X_MINOR_MASK);
            PrintDebug(" usHwVer[0x%x]", stDbV2x.usHwVer);
            PrintDebug(" usSwVer[0x%x]", stDbV2x.usSwVer);
            PrintTrace("========================================================");
        }
        else if(IS_CMD(pcCmd, "tcp"))
        {
            pcCmd = CLI_CMD_GetArg(pstCmd, CMD_1);
            if(pcCmd != NULL)
            {
                if(IS_CMD(pcCmd, "server"))
                {
                    nRet = P_CLI_MSG_TcpServerStart();
                    if (nRet != APP_OK)
                    {
                        PrintError("P_CLI_MSG_TcpServerStart() is failed! [nRet:%d]", nRet);
                    }
                }
                else if(IS_CMD(pcCmd, "client"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        nRet = P_CLI_MSG_TcpClientStart(pcCmd);
                    if (nRet != APP_OK)
                    {
                        PrintError("P_CLI_MSG_TcpClientStart() is failed! [nRet:%d]", nRet);
                    }

                    }
                    else
                    {
                        PrintError("CMD_2 is NULL, see. msg help");
                    }
                }
                else if(IS_CMD(pcCmd, "multiserver"))
                {
                    nRet = P_CLI_MSG_TcpMultiServerStart();
                    if (nRet != APP_OK)
                    {
                        PrintError("P_CLI_MSG_TcpMultiServerStart() is failed! [nRet:%d]", nRet);
                    }
                }
                else if(IS_CMD(pcCmd, "multiclient"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    char *pcId = CLI_CMD_GetArg(pstCmd, CMD_3);
                    if (pcCmd != NULL && pcId != NULL)
                    {
                        nRet = P_CLI_MSG_TcpMultiClientStart(pcCmd, pcId);
                        if (nRet != APP_OK)
                        {
                            PrintError("P_CLI_MSG_TcpMultiClientStart() is failed! [nRet:%d]", nRet);
                        }
                    }
                }
                else
                {
                    PrintError("IP address or ID is missing, see msg help.");
                }
            }
            else
            {
                PrintError("msg tcp [OPTIONS]\n"
                           "  server           start TCP server (127.0.0.1)\n"
                           "  client [IP Addr] start TCP client with IP server IP address\n"
                           "");
            }
        }
        else
        {
            return CLI_CMD_Showusage(pstCmd);
        }
    }

	return nRet;
}

int32_t CLI_MSG_InitCmds(void)
{
    int32_t nRet = APP_ERROR;

    nRet = CLI_CMD_AddCmd("msg",
               P_CLI_MSG,
               NULL,
               "help for MSG commands",
               "msg [enter command]\n\n"
               "Without any parameters, the 'msg' show a description\n"
               "of available commands. For more details on a command, type and enter 'msg'\n"
               "and the command name."
               "and the command name.\n\n"
               "msg info          show msg settings\n"
               "msg log           on/off\n"
               "msg log [opt]     show msg debug logs (on/off)\n"
               "msg open [eth#]   open message protocol, connect TCP server, e.g. msg open eth1\n"
               "msg close         close message protocol\n"
               "msg tx            send v2x messages to v2x devices (set msg open before)\n"
               "msg txC           Sequentially send v2x messages to v2x devices (set msg open before)\n"
               "msg set [OPTIONS]\n"
               "  payload         [0]Raw, [1]EncodedbyJ2735, [2]eITSK00130, [3]eKETI, [4]eETRI, (default : Raw)\n"
               "  psid            <decimal number> (default : 32)\n"
               "  comm            [0] UNKNOWN, [1]DSRC, [2]LTEV2X, [3]5GNRV2X, (default : 5GNRV2X)\n"
               "  tx_power        tx port dbm (default : 20)\n"
               "  sign            [0]UNSECURED, [1]CERTIFICATE, [2]DIGEST, [3]ALTERNATE (default : UNSECURED)\n"
               "  priority        0~7 (default : 0)\n"
               "  tx_count        total tx count (default : 10)\n"
               "  tx_delay        msec delay (default : 100ms)\n"
               "  device          [0]UNKNOWN, [1]OBU, [2]RSU, [3]Contrl Center\n"
               "  tele_comm       [0]UNKNOWN, [1]4G,  [20]5G Uu, [30]5G PC5, [31]5G PC5 Broadcast, [32]5G PC5 Unicast, [33]5G PC5 Multicast, [34]5G PC5 Groupcast\n"
               "  device_id       device id (default : 23040015)\n"
               "  service_id      [0]UNKONWN, [1]Platooning(default), [2]Sensor sharing, [3]Remote driving, [4]Advanced driving\n"
               "  action          [0]UNKONWN, [1]Request(default), [2]Response\n"
               "  region          [0]UNKONWN, [1]Seoul, [2]Sejong, [3]Busan, [4]Daegeon, [5]Incheon, [6]Daegu, [7]Daegu KIAPI PG, [8]Cheongju, [9]Seongnam(default)\n"
               "  pl_type         [0]UNKONWN, [1]SAE J2735 BSM, [2]SAE J2736 PVD, [201]Platooning(default), [301]Sensor sharing, [401]Remote driving, [501] Advanced driving\n"
               "  comm_id         [0]UNKONWN, [1]V2V(default), [2]V2I,...\n"
               "msg get           get setting values of v2x structures\n"
               "msg web file [id] set file type, 0:unknown, 1:tx, 2:rx(default), 3:sample Tx, 4:sample Rx\n"
               "msg web start     start web server, set file type first\n"
               "msg web stop      stop web server\n"
               "msg tcp [OPTIONS]\n"
               "  server           start TCP server (127.0.0.1)\n"
               "  client [IP Addr] start TCP client with IP server IP address\n",
               "");
    if(nRet != APP_OK)
    {
        PrintError("CLI_CMD_AddCmd() is failed! [nRet:%d]", nRet);
    }

    (void*)memset(&s_stMsgManagerTx, 0x00, sizeof(MSG_MANAGER_TX_T));
    (void*)memset(&s_stMsgManagerRx, 0x00, sizeof(MSG_MANAGER_RX_T));
    (void*)memset(&s_stDbV2x, 0x00, sizeof(DB_V2X_T));

    nRet = P_CLI_MSG_SetDefaultSettings();
    if(nRet != APP_OK)
    {
        PrintError("P_CLI_MSG_SetDefaultSettings() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

