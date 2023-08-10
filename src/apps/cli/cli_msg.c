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

/***************************** Definition ************************************/
//#define CONFIG_CLI_MSG_DEBUG        (1)

/***************************** Static Variable *******************************/
static MSG_MANAGER_TX_T s_stMsgManagerTx;
static MSG_MANAGER_RX_T s_stMsgManagerRx;
static DB_V2X_T s_stDbV2x;

/***************************** Function Protype ******************************/
static int P_CLI_MSG_TcpRlogin(CLI_CMDLINE_T *pstCmd, int argc, char *argv[])
{
    int32_t nRet = APP_OK;
    char *pcCmd;

    UNUSED(argc);

    if(argv == NULL)
    {
        PrintError("argv == NULL!!");
        return nRet;
    }

    pcCmd = CLI_CMD_GetArg(pstCmd, 0);
    if (pcCmd == NULL)
    {
        return CLI_CMD_Showusage(pstCmd);
    }

    return nRet;
}

static int P_CLI_MSG_TcpConnect(CLI_CMDLINE_T *pstCmd, int argc, char *argv[])
{
    int32_t nRet = APP_OK;
    char *pcCmd;

    UNUSED(argc);

    if(argv == NULL)
    {
        PrintError("argv == NULL!!");
        return nRet;
    }

    pcCmd = CLI_CMD_GetArg(pstCmd, 0);
    if (pcCmd == NULL)
    {
        return CLI_CMD_Showusage(pstCmd);
    }


    return nRet;
}

static int P_CLI_MSG_TcpListen(CLI_CMDLINE_T *pstCmd, int argc, char *argv[])
{
    int32_t nRet = APP_OK;
    char *pcCmd;

    UNUSED(argc);

    if(argv == NULL)
    {
        PrintError("argv == NULL!!");
        return nRet;
    }

    pcCmd = CLI_CMD_GetArg(pstCmd, 0);
    if (pcCmd == NULL)
    {
        return CLI_CMD_Showusage(pstCmd);
    }

    return nRet;
}

static int P_CLI_MSG_TcpConsTest(CLI_CMDLINE_T *pstCmd, int argc, char *argv[])
{
    int32_t nRet = APP_OK;
    char *pcCmd;

    UNUSED(argc);

    if(argv == NULL)
    {
        PrintError("argv == NULL!!");
        return nRet;
    }

    pcCmd = CLI_CMD_GetArg(pstCmd, 0);
    if (pcCmd == NULL)
    {
        return CLI_CMD_Showusage(pstCmd);
    }

    return nRet;
}

static int P_CLI_MSG_TcpTest(CLI_CMDLINE_T *pstCmd, int argc, char *argv[])
{
    int32_t nRet = APP_OK;
    char *pcCmd;

    UNUSED(argc);

    if(argv == NULL)
    {
        PrintError("argv == NULL!!");
        return nRet;
    }

    pcCmd = CLI_CMD_GetArg(pstCmd, 0);
    if (pcCmd == NULL)
    {
        return CLI_CMD_Showusage(pstCmd);
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
    PrintDebug(" unDeviceId[0x%x]", s_stDbV2x.unDeviceId);
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

    stMsgManagerTx.ePayloadType = MSG_MANAGER_PAYLOAD_TYPE_RAW;
    stMsgManagerTx.eCommType = MSG_MANAGER_COMM_TYPE_5GNRV2X;
    stMsgManagerTx.eSignId = MSG_MANAGER_SIGN_ID_UNSECURED;
    stMsgManagerTx.eV2xFreq = MSG_MANAGER_V2X_FREQ_5900;
    stMsgManagerTx.ePriority = MSG_MANAGER_PRIORITY_CV2X_PPPP_0;
    stMsgManagerTx.eV2xDataRate = MSG_MANAGER_V2X_DATA_RATE_6MBPS;
    stMsgManagerTx.eV2xTimeSlot = MSG_MANAGER_V2X_TIME_SLOT_CONTINUOUS;
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
    stDbV2x.ePayloadType = DB_V2X_PAYLOAD_TYPE_PLATOONING;
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
                s_stDbV2x.ulPacketCrc32 = CLI_UTIL_GetCrc32((uint8_t*)&cPayload, s_stDbV2x.ulPayloadLength);

#if defined(CONFIG_CLI_MSG_DEBUG)
                (void)P_CLI_MSG_ShowTxSettings();

                PrintTrace("========================================================");
                PrintDebug("ulTimeStamp[%ld]", s_stDbV2x.ulTimeStamp);
                PrintDebug("ulPayloadLength[%d]", s_stDbV2x.ulPayloadLength);
                PrintDebug("cPayload");
                for(i = 0; i < CLI_DB_V2X_DEFAULT_PAYLOAD_LEN; i++)
                {
                    cPayload[i] = rand();
                    printf("[%d:%d] ", i, cPayload[i]);
                }
                printf("\r\n");

                PrintDebug("ulPayloadCrc32[0x%x]", s_stDbV2x.ulPacketCrc32);
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
                s_stDbV2x.ulPacketCrc32 = CLI_UTIL_GetCrc32((uint8_t*)&cPayload, s_stDbV2x.ulPayloadLength);

#if defined(CONFIG_CLI_MSG_DEBUG)
                (void)P_CLI_MSG_ShowTxSettings();

                PrintTrace("========================================================");
                PrintDebug("ulTimeStamp[%ld]", s_stDbV2x.ulTimeStamp);
                PrintDebug("ulPayloadLength[%d]", s_stDbV2x.ulPayloadLength);
                PrintDebug("cPayload");
                for(i = 0; i < CLI_DB_V2X_DEFAULT_PAYLOAD_LEN; i++)
                {
                    cPayload[i] = unTxCount;
                    printf("[%d:%d] ", i, cPayload[i]);
                }
                printf("\r\n");

                PrintDebug("ulPayloadCrc32[0x%x]", s_stDbV2x.ulPacketCrc32);
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

                pstMsgManager->pchIfaceName = pcCmd;

                PrintTrace("pstMsgManager->pchIfaceName[%s]", pstMsgManager->pchIfaceName);

                nFrameWorkRet = MSG_MANAGER_Open(pstMsgManager);
                if(nFrameWorkRet != FRAMEWORK_OK)
                {
                    PrintError("MSG_MANAGER_Open() is failed! [nRet:%d]", nFrameWorkRet);
                }
            }
            else
            {
                PrintError("set an ethernet interface name, e.g. CLI> msg open eth1");
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
            PrintDebug(" unDeviceId[0x%x]", stDbV2x.unDeviceId);
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
               "msg get           get setting values of v2x structures\n",
               "");
    if(nRet != APP_OK)
    {
        PrintError("CLI_CMD_AddCmd() is failed! [nRet:%d]", nRet);
    }

    nRet = CLI_CMD_AddCmd("tcp-rlogin",
	       P_CLI_MSG_TcpRlogin,
	       NULL,
	       "mini rlogin client.",
	       "rlogin hostname [username]\n\n"
	       "Connects to a remote system using the RLOGIN protocol.\n"
	       "The remote system must have appropriate permissions in place\n"
	       "(usually via the file '.rhosts') for CFE to connect.\n"
	       "To terminate the session, type\n"
	       "a tilde (~) character followed by a period (.)",
	       "");
    if(nRet != APP_OK)
    {
        PrintError("CLI_CMD_AddCmd() is failed! [nRet:%d]", nRet);
    }

    nRet = CLI_CMD_AddCmd("tcp-connect",
	       P_CLI_MSG_TcpConnect,
	       NULL,
	       "TCP connection test.",
	       "tcp connect hostname [portnum]",
	       "-q;sink output, don't display on terminal|"
	       "-d;Send junk data to discard|"
	       "-nodelay;set nodelay option on socket|"
	       "-srcport=*;Specify the source port");
    if(nRet != APP_OK)
    {
        PrintError("CLI_CMD_AddCmd() is failed! [nRet:%d]", nRet);
    }

    nRet = CLI_CMD_AddCmd("tcp-listen",
	       P_CLI_MSG_TcpListen,
	       NULL,
	       "port listener.",
	       "tcp listen portnum",
	       "-q;sink output, don't display on terminal|"
	       "-d;Send junk data to discard|"
	       "-nodelay;set nodelay option on socket");
    if(nRet != APP_OK)
    {
        PrintError("CLI_CMD_AddCmd() is failed! [nRet:%d]", nRet);
    }

    nRet = CLI_CMD_AddCmd("tcp-constest",
	       P_CLI_MSG_TcpConsTest,
	       NULL,
	       "tcp console test.",
	       "tcp constest device",
	       "");
    if(nRet != APP_OK)
    {
        PrintError("CLI_CMD_AddCmd() is failed! [nRet:%d]", nRet);
    }

    nRet = CLI_CMD_AddCmd("tcp-test",
	       P_CLI_MSG_TcpTest,
	       NULL,
	       "TCP test command.",
	       "ttcp -t [-options] host\n"
	       "ttcp -r [-options]\n\n",
	       "-t;Source a pattern to the network|"
	       "-r;Sink (discard) data from the network|"
	       "-D;Don't buffer TCP writes (TCP_NODELAY)|"
	       "-n=*;Number of buffers to send (-t only) (default 2048)|"
	       "-l=*;Size of buffer to send/receive (default 2048)|"
	       "-p=*;Port number to use (default 5001)");
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

