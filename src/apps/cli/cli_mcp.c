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
* @file cli_mcp.c
*
* @note
*
* CLI Multi Communication Performance Source
*
******************************************************************************/


/***************************** Include ***************************************/
#include "cli.h"
#include "app.h"
#include "db_v2x.h"
#include "db_v2x_status.h"
#include "multi_db_manager.h"
#include "framework.h"

/***************************** Definition ************************************/

/***************************** Static Variable *******************************/
static char s_chMultiSetBufDevId[CLI_MULTI_DB_V2X_DEFAULT_BUF_LEN];
static char s_chMultiSetEth[CLI_MULTI_DB_V2X_DEFAULT_BUF_LEN];
static char s_chMultiSetIp[CLI_MULTI_DB_V2X_DEFAULT_BUF_LEN];

/***************************** Function Protype ******************************/
void P_CLI_MCP_WriteConfigToFile(FILE *h_fdModelConf, SVC_MCP_T *pstSvcMCp)
{
    fprintf(h_fdModelConf, "model=%s\n", CONFIG_MODEL_NAME);
    fprintf(h_fdModelConf, "pchDeviceName=%s\n", pstSvcMCp->pchDeviceName);
    fprintf(h_fdModelConf, "unDeviceId=%u\n", pstSvcMCp->stDbV2x.unDeviceId);
    fprintf(h_fdModelConf, "pchIfaceName=%s\n", pstSvcMCp->pchIfaceName);
    fprintf(h_fdModelConf, "pchIpAddr=%s\n", pstSvcMCp->pchIpAddr);
    fprintf(h_fdModelConf, "unPort=%d\n", pstSvcMCp->unPort);
}

static int P_CLI_MCP_SetV2xStatusScenario(CLI_CMDLINE_T *pstCmd)
{
    int32_t nRet = APP_OK;
    SVC_MCP_T *pstSvcMCp;
    char *pcCmd;
    char chModelNameFile[MAX_MODEL_NAME_LEN] = {0};
    FILE *h_fdModelConf;

    pstSvcMCp = APP_GetSvcMCpInstance();

    if(pstCmd == NULL)
    {
        PrintError("pstCmd == NULL!!");
        return CLI_CMD_Showusage(pstCmd);
    }

    nRet = SVC_MCP_GetSettings(pstSvcMCp);
    if(nRet != APP_OK)
    {
        PrintError("SVC_MCP_SetSettings() is failed! [nRet:%d]", nRet);
    }

    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_1);
    if(pcCmd == NULL)
    {
        PrintError("pcCmd == NULL!!");
        return CLI_CMD_Showusage(pstCmd);
    }
    else if(strcmp(pcCmd, "dev") == 0)
    {
        pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
        if(pcCmd == NULL)
        {
            PrintError("pcCmd == NULL!!");
            return CLI_CMD_Showusage(pstCmd);
        }

        pstSvcMCp->stDbV2x.unDeviceId = (uint32_t)atoi(pcCmd);
        sprintf(s_chMultiSetBufDevId, "%s", pcCmd);
        pstSvcMCp->pchDeviceName = s_chMultiSetBufDevId;

        if(strcmp(pstSvcMCp->pchDeviceName, MULTI_DB_MGR_DEFAULT_COMM_DEV_ID) == 0)
        {
            PrintWarn("INSERT DEVICE ID is failed!");
            nRet = APP_ERROR;
        }
        else
        {
            PrintDebug("SET:unDeviceId[%d]", pstSvcMCp->stDbV2x.unDeviceId);
            PrintDebug("SET:pchDeviceName[%s]", pstSvcMCp->pchDeviceName);
        }

        nRet = SVC_MCP_SetSettings(pstSvcMCp);
        if(nRet != APP_OK)
        {
            PrintError("SVC_MCP_SetSettings() is failed! [nRet:%d]", nRet);
        }
    }
    else if(strcmp(pcCmd, "eth") == 0)
    {
        pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
        if(pcCmd == NULL)
        {
            PrintError("pcCmd == NULL!!");
            return CLI_CMD_Showusage(pstCmd);
        }

        sprintf(s_chMultiSetEth, "%s", pcCmd);
        pstSvcMCp->pchIfaceName = s_chMultiSetEth;

        PrintDebug("SET:pchIfaceName[%s]", pstSvcMCp->pchIfaceName);

        nRet = SVC_MCP_SetSettings(pstSvcMCp);
        if(nRet != APP_OK)
        {
            PrintError("SVC_MCP_SetSettings() is failed! [nRet:%d]", nRet);
        }

    }
    else if(strcmp(pcCmd, "txrat") == 0)
    {
        pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
        if(pcCmd == NULL)
        {
            PrintError("pcCmd == NULL!!");
            return CLI_CMD_Showusage(pstCmd);
        }

        pstSvcMCp->stMultiMsgManagerTx.unTxDelay = (uint16_t)atoi(pcCmd);
        pstSvcMCp->stDbV2xStatusTx.usTxRatio = (uint16_t)atoi(pcCmd);

        PrintDebug("SET:unTxDelay[%d]", pstSvcMCp->stMultiMsgManagerTx.unTxDelay);
        PrintDebug("SET:usTxRatio[%d]", pstSvcMCp->stDbV2xStatusTx.usTxRatio);

        nRet = SVC_MCP_SetSettings(pstSvcMCp);
        if(nRet != APP_OK)
        {
            PrintError("SVC_MCP_SetSettings() is failed! [nRet:%d]", nRet);
        }
    }
    else if(strcmp(pcCmd, "spd") == 0)
    {
        pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
        if(pcCmd == NULL)
        {
            PrintError("pcCmd == NULL!!");
            return CLI_CMD_Showusage(pstCmd);
        }

        pstSvcMCp->stDbV2xStatusTx.unTxVehicleSpeed = (uint32_t)atoi(pcCmd);

        PrintDebug("SET:unTxVehicleSpeed[%d]", pstSvcMCp->stDbV2xStatusTx.unTxVehicleSpeed);

        nRet = SVC_MCP_SetSettings(pstSvcMCp);
        if(nRet != APP_OK)
        {
            PrintError("SVC_MCP_SetSettings() is failed! [nRet:%d]", nRet);
        }
    }
    else if(strcmp(pcCmd, "rg") == 0)
    {
        pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
        if(pcCmd == NULL)
        {
            PrintError("pcCmd == NULL!!");
            return CLI_CMD_Showusage(pstCmd);
        }

        pstSvcMCp->stDbV2x.eRegionId = (uint32_t)atoi(pcCmd);

        PrintDebug("SET:eRegionId[0x%x]", pstSvcMCp->stDbV2x.eRegionId);

        nRet = SVC_MCP_SetSettings(pstSvcMCp);
        if(nRet != APP_OK)
        {
            PrintError("SVC_MCP_SetSettings() is failed! [nRet:%d]", nRet);
        }
    }
    else if(strcmp(pcCmd, "ip") == 0)
    {
        pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
        if(pcCmd == NULL)
        {
            PrintError("pcCmd == NULL!!");
            return CLI_CMD_Showusage(pstCmd);
        }

        sprintf(s_chMultiSetIp, "%s", pcCmd);
        pstSvcMCp->pchIpAddr = s_chMultiSetIp;

        PrintDebug("Set:IP[%s]", pstSvcMCp->pchIpAddr);

        nRet = SVC_MCP_SetSettings(pstSvcMCp);
        if(nRet != APP_OK)
        {
            PrintError("SVC_MCP_SetSettings() is failed! [nRet:%d]", nRet);
        }
    }
    else if(strcmp(pcCmd, "port") == 0)
    {
        pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
        if(pcCmd == NULL)
        {
            PrintError("pcCmd == NULL!!");
            return CLI_CMD_Showusage(pstCmd);
        }

        pstSvcMCp->unPort = (uint32_t)atoi(pcCmd);

        PrintDebug("SET:port[%d]", pstSvcMCp->unPort);

        nRet = SVC_MCP_SetSettings(pstSvcMCp);
        if(nRet != APP_OK)
        {
            PrintError("SVC_MCP_SetSettings() is failed! [nRet:%d]", nRet);
        }
    }
    else
    {
        PrintWarn("unknown set type");
        nRet = APP_ERROR;
    }

    snprintf(chModelNameFile, sizeof(chModelNameFile), "%s%s", CONFIG_MODEL_NAME, MODEL_NAME_FILE_SUFFIX);

    h_fdModelConf = fopen(chModelNameFile, "r+");
    if (h_fdModelConf == NULL)
    {
        h_fdModelConf = fopen(chModelNameFile, "w");
        if (h_fdModelConf == NULL)
        {
            PrintError("Failed to open or create file: %s", chModelNameFile);
            return APP_ERROR;
        }
        P_CLI_MCP_WriteConfigToFile(h_fdModelConf, pstSvcMCp);
    }
    else
    {
        char chExistingModelName[MAX_MODEL_NAME_LEN] = {0};
        if (fgets(chExistingModelName, sizeof(chExistingModelName), h_fdModelConf) != NULL)
        {
            if ((strncmp(chExistingModelName, MODEL_PREFIX, MODEL_PREFIX_LEN) != 0) || (strcmp(chExistingModelName + MODEL_PREFIX_LEN, CONFIG_MODEL_NAME) != 0))
            {
                h_fdModelConf = freopen(chModelNameFile, "w", h_fdModelConf);
                if (h_fdModelConf == NULL)
                {
                    PrintError("Failed to reopen file: %s", chModelNameFile);
                    return APP_ERROR;
                }
                P_CLI_MCP_WriteConfigToFile(h_fdModelConf, pstSvcMCp);
            }
        }
        else
        {
            // If we couldn't read the existing content, rewrite the file
            h_fdModelConf = freopen(chModelNameFile, "w", h_fdModelConf);
            if (h_fdModelConf == NULL)
            {
                PrintError("Failed to reopen file: %s", chModelNameFile);
                return APP_ERROR;
            }
            P_CLI_MCP_WriteConfigToFile(h_fdModelConf, pstSvcMCp);
        }
    }

    if (h_fdModelConf != NULL)
    {
        fclose(h_fdModelConf);
    }
    else
    {
        PrintError("h_fdModelConf is NULL!!");
    }

    return nRet;
}

static int P_CLI_MCP_CheckV2xStatusScenario(void)
{
    int32_t nRet = APP_OK;
    SVC_MCP_T *pstSvcMCp;
    pstSvcMCp = APP_GetSvcMCpInstance();

    nRet = SVC_MCP_GetSettings(pstSvcMCp);
    if(nRet != APP_OK)
    {
        PrintError("SVC_MCP_SetSettings() is failed! [nRet:%d]", nRet);
    }

    (void)SVC_MCP_ShowSettings(pstSvcMCp);

    return nRet;
}

static int P_CLI_MCP_ReadyV2xStatusScenario(void)
{
    int32_t nRet = APP_OK;
    SVC_MCP_T *pstSvcMCp;

    pstSvcMCp = APP_GetSvcMCpInstance();

    if (pstSvcMCp == NULL)
    {
        PrintError("Failed to get service control point instance.");
        return APP_ERROR;
    }

    (void)SVC_MCP_ShowSettings(pstSvcMCp);

    nRet = SVC_MCP_GetSettings(pstSvcMCp);
    if (nRet != APP_OK)
    {
        PrintError("SVC_MCP_GetSettings() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    nRet = SVC_MCP_UpdateSettings(pstSvcMCp);
    if (nRet != APP_OK)
    {
        PrintError("SVC_MCP_UpdateSettings() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    nRet = SVC_MCP_SetSettings(pstSvcMCp);
    if (nRet != APP_OK)
    {
        PrintError("SVC_MCP_SetSettings() is failed! [nRet:%d]", nRet);
    }

    nRet = SVC_MCP_Open(pstSvcMCp);
    if (nRet != APP_OK)
    {
        PrintError("SVC_MCP_Open() is failed! [nRet:%d]", nRet);
        return nRet;
    }

    return nRet;
}


static int P_CLI_MCP_StartV2xStatusScenario(void)
{
    int32_t nRet = APP_OK;
    SVC_MCP_T *pstSvcMCp;
    pstSvcMCp = APP_GetSvcMCpInstance();

    nRet = SVC_MCP_Start(pstSvcMCp);
    if(nRet != APP_OK)
    {
        PrintError("SVC_MCP_Start() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

static int P_CLI_MCP_StopV2xStatusScenario(void)
{
    int32_t nRet = APP_OK;
    SVC_MCP_T *pstSvcMCp;
    pstSvcMCp = APP_GetSvcMCpInstance();

    nRet = SVC_MCP_Stop(pstSvcMCp);
    if(nRet != APP_OK)
    {
        PrintError("SVC_MCP_Start() is failed! [nRet:%d]", nRet);
    }

    usleep(SVC_MCP_STOP_DELAY);

    nRet = SVC_MCP_Close(pstSvcMCp);
    if(nRet != APP_OK)
    {
        PrintError("SVC_MCP_Close() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

static int P_CLI_MCP_StartV2xStatus(bool bMsgTx, bool bLogOnOff)
{
    int32_t nRet = APP_OK;
    int nFrameWorkRet = FRAMEWORK_ERROR;
    TIME_MANAGER_T *pstTimeManager;
    MULTI_DB_MANAGER_T *pstMultiDbManager;
    MULTI_MSG_MANAGER_T *pstMultiMsgManager;
    SVC_MCP_T *pstSvcMCp;
    char *pchPayload = NULL;

    pstSvcMCp = APP_GetSvcMCpInstance();

    pstTimeManager = FRAMEWORK_GetTimeManagerInstance();

    (void)TIME_MANAGER_CheckLatencyBegin(pstTimeManager);

    pstMultiDbManager = FRAMEWORK_GetMultiDbManagerInstance();
    PrintDebug("pstMultiDbManager[0x%p]", pstMultiDbManager);

    pstMultiDbManager->eMultiFileType = MULTI_DB_MANAGER_FILE_TYPE_CSV;
    pstMultiDbManager->eMultiSvcType = MULTI_DB_MANAGER_SVC_TYPE_V2X_STATUS;

    nFrameWorkRet = MULTI_DB_MANAGER_Open(pstMultiDbManager);
    if(nFrameWorkRet != FRAMEWORK_OK)
    {
        PrintError("MULTI_DB_MANAGER_Open() is failed! [nRet:%d]", nFrameWorkRet);
    }

    if(bMsgTx == TRUE)
    {
        pstMultiMsgManager = FRAMEWORK_GetMultiMsgManagerInstance();
        PrintDebug("pstMultiMsgManager[0x%p]", pstMultiMsgManager);

        pstMultiMsgManager->eDeviceType = pstSvcMCp->stDbV2x.eDeviceType;

        pstMultiMsgManager->pchIfaceName = pstSvcMCp->pchIfaceName;
        pstMultiMsgManager->stExtMultiMsgWsr.unPsid = pstSvcMCp->unPsid;
        pstMultiMsgManager->pchIpAddr = pstSvcMCp->pchIpAddr;
        pstMultiMsgManager->unPort = pstSvcMCp->unPort;

        PrintTrace("pchIfaceName[%s], pchIpAddr[%s], unPort[%d]", pstMultiMsgManager->pchIfaceName, pstMultiMsgManager->pchIpAddr, pstMultiMsgManager->unPort);
        PrintTrace("pchIfaceName[%s], unPsid[%d]", pstMultiMsgManager->pchIfaceName, pstMultiMsgManager->stExtMultiMsgWsr.unPsid);

        nFrameWorkRet = MULTI_MSG_MANAGER_Open(pstMultiMsgManager);
        if(nFrameWorkRet != FRAMEWORK_OK)
        {
            PrintError("MULTI_MSG_MANAGER_Open() is failed! [nRet:%d]", nFrameWorkRet);
        }
    }


    pstSvcMCp->stDbV2x.ulPayloadLength = sizeof(pstSvcMCp->stDbV2xStatusTx);

    pchPayload = (char*)malloc(sizeof(char)*pstSvcMCp->stDbV2x.ulPayloadLength);
    if(pchPayload == NULL)
    {
        PrintError("malloc() is failed! [NULL]");
        return nRet;
    }

    (void*)memset(pchPayload, 0x00, sizeof(sizeof(char)*pstSvcMCp->stDbV2x.ulPayloadLength));

    nFrameWorkRet = TIME_MANAGER_Get(pstTimeManager);
    if(nFrameWorkRet != FRAMEWORK_OK)
    {
        PrintError("TIME_MANAGER_Get() is failed! [nRet:%d]", nFrameWorkRet);
    }
    else
    {
        pstSvcMCp->stDbV2x.ulTimeStamp = pstTimeManager->ulTimeStamp;

        pstSvcMCp->stDbV2xStatusTx.stDbV2xDevL1.ulTimeStamp = 19840919;
        pstSvcMCp->stDbV2xStatusTx.stDbV2xDevL2.ulTimeStamp = 19850501;
        pstSvcMCp->stDbV2xStatusTx.stDbV2xDevL3.ulTimeStamp = pstTimeManager->ulTimeStamp;
    }

    memcpy(pchPayload, (char*)&pstSvcMCp->stDbV2xStatusTx, sizeof(char)*pstSvcMCp->stDbV2x.ulPayloadLength);

    pstSvcMCp->stDbV2x.ulReserved = 0;

    if (bLogOnOff == TRUE)
    {
        (void)SVC_MCP_ShowSettings(pstSvcMCp);

        PrintTrace("========================================================");
        PrintDebug("ulTimeStamp[%ld]", pstSvcMCp->stDbV2x.ulTimeStamp);
        PrintDebug("ulPayloadLength[%d]", pstSvcMCp->stDbV2x.ulPayloadLength);
        PrintDebug("ulReserved[0x%x]", pstSvcMCp->stDbV2x.ulReserved);
        PrintTrace("========================================================");
    }

    if(bMsgTx == TRUE)
    {
        nFrameWorkRet = MULTI_MSG_MANAGER_Transmit(&pstSvcMCp->stMultiMsgManagerTx, &pstSvcMCp->stDbV2x, (char*)pchPayload);
        if(nFrameWorkRet != FRAMEWORK_OK)
        {
            PrintError("MULTI_MSG_MANAGER_Transmit() is failed! [nRet:%d]", nFrameWorkRet);
        }
        else
        {
            PrintDebug("Tx Success, Counts[%u], Delay[%d ms]", pstSvcMCp->stMultiMsgManagerTx.unTxCount, pstSvcMCp->stMultiMsgManagerTx.unTxDelay);
        }
    }
    else
    {
        nFrameWorkRet = MULTI_DB_MANAGER_Write(&pstSvcMCp->stMultiDbManagerWrite, &pstSvcMCp->stDbV2x, (char*)pchPayload);
        if(nFrameWorkRet != FRAMEWORK_OK)
        {
            PrintError("MULTI_DB_MANAGER_Write() is failed! [nRet:%d]", nFrameWorkRet);
        }
    }

    (void)TIME_MANAGER_CheckLatencyEnd(pstTimeManager);
    (void)TIME_MANAGER_CheckLatencyTime("Tx Total Time", pstTimeManager);

    /* free(pchPayload) is free at the P_MSG_MANAGER_SendTxMsg() */

    return nRet;
}

static int P_CLI_MCP(CLI_CMDLINE_T *pstCmd, int argc, char *argv[])
{
    int32_t nRet = APP_OK;
    char *pcCmd;
    bool bMsgTx = FALSE, bLogOnOff = FALSE;

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
        if(IS_CMD(pcCmd, "test"))
        {
            for(int i = 0; i < CMD_MAX; i++)
            {
                pcCmd = CLI_CMD_GetArg(pstCmd, i);
                PrintDebug("pcCmd[idx:%d][value:%s]", i, pcCmd);
            }
        }
        else if(IS_CMD(pcCmd, "set"))
        {
            pcCmd = CLI_CMD_GetArg(pstCmd, CMD_1);
            if(pcCmd != NULL)
            {
                if(IS_CMD(pcCmd, "dev"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        nRet = P_CLI_MCP_SetV2xStatusScenario(pstCmd);
                        if(nRet != APP_OK)
                        {
                            PrintError("P_CLI_MCP_SetOptV2xStatusScenario() is failed![nRet:%d]", nRet);
                        }
                    }
                }
                else if(IS_CMD(pcCmd, "eth"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        nRet = P_CLI_MCP_SetV2xStatusScenario(pstCmd);
                        if(nRet != APP_OK)
                        {
                            PrintError("P_CLI_MCP_SetOptV2xStatusScenario() is failed![nRet:%d]", nRet);
                        }
                    }
                }
                else if(IS_CMD(pcCmd, "txrat"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        nRet = P_CLI_MCP_SetV2xStatusScenario(pstCmd);
                        if(nRet != APP_OK)
                        {
                            PrintError("P_CLI_MCP_SetOptV2xStatusScenario() is failed![nRet:%d]", nRet);
                        }
                    }
                }
                else if(IS_CMD(pcCmd, "spd"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        nRet = P_CLI_MCP_SetV2xStatusScenario(pstCmd);
                        if(nRet != APP_OK)
                        {
                            PrintError("P_CLI_MCP_SetOptV2xStatusScenario() is failed![nRet:%d]", nRet);
                        }
                    }
                }
                else if(IS_CMD(pcCmd, "rg"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        nRet = P_CLI_MCP_SetV2xStatusScenario(pstCmd);
                        if(nRet != APP_OK)
                        {
                            PrintError("P_CLI_MCP_SetOptV2xStatusScenario() is failed![nRet:%d]", nRet);
                        }
                    }
                }
                else if(IS_CMD(pcCmd, "ip"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        nRet = P_CLI_MCP_SetV2xStatusScenario(pstCmd);
                        if(nRet != APP_OK)
                        {
                            PrintError("P_CLI_MCP_SetOptV2xStatusScenario() is failed![nRet:%d]", nRet);
                        }
                    }
                }
                else if(IS_CMD(pcCmd, "port"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        nRet = P_CLI_MCP_SetV2xStatusScenario(pstCmd);
                        if(nRet != APP_OK)
                        {
                            PrintError("P_CLI_MCP_SetOptV2xStatusScenario() is failed![nRet:%d]", nRet);
                        }
                    }
                }
                else
                {
                    return CLI_CMD_Showusage(pstCmd);
                }
            }
            else
            {
                return CLI_CMD_Showusage(pstCmd);
            }
        }
        else if(IS_CMD(pcCmd, "check") || IS_CMD(pcCmd, "get"))
        {
            nRet = P_CLI_MCP_CheckV2xStatusScenario();
            if(nRet != APP_OK)
            {
                PrintError("P_CLI_MCP_CheckV2xStatusScenario() is failed![nRet:%d]", nRet);
            }
        }
        else if(IS_CMD(pcCmd, "base"))
        {
            PrintTrace("Open DB");

            PrintTrace("Connect V2X Device (OBU/RSU)");

            PrintTrace("Setting Parameters");

            PrintTrace("Start V2X Tx/Rx Communication of Platooning");

            PrintTrace("Save DB");
        }
        else if(IS_CMD(pcCmd, "ready"))
        {
            nRet = P_CLI_MCP_ReadyV2xStatusScenario();
            if(nRet != APP_OK)
            {
                PrintError("P_CLI_MCP_ReadyV2xStatusScenario() is failed![nRet:%d]", nRet);
            }
        }
        else if(IS_CMD(pcCmd, "start"))
        {
            nRet = P_CLI_MCP_StartV2xStatusScenario();
            if(nRet != APP_OK)
            {
                PrintError("P_CLI_MCP_StartV2xStatusScenario() is failed![nRet:%d]", nRet);
            }
        }
        else if(IS_CMD(pcCmd, "stop"))
        {
            nRet = P_CLI_MCP_StopV2xStatusScenario();
            if(nRet != APP_OK)
            {
                PrintError("P_CLI_MCP_StopV2xStatusScenario() is failed![nRet:%d]", nRet);
            }
        }
        else if(IS_CMD(pcCmd, "status"))
        {
            pcCmd = CLI_CMD_GetArg(pstCmd, CMD_1);
            if (pcCmd != NULL)
            {
                if(IS_CMD(pcCmd, "start"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if (pcCmd != NULL)
                    {
                        bLogOnOff = TRUE;

                        if(IS_CMD(pcCmd, "msg"))
                        {
                            bMsgTx = TRUE;
                        }
                        else if(IS_CMD(pcCmd, "db"))
                        {
                            bMsgTx = FALSE;
                        }

                        PrintTrace("bMsgTx[%d], bLogOnOff[%d]", bMsgTx, bLogOnOff);

                        nRet = P_CLI_MCP_StartV2xStatus(bMsgTx, bLogOnOff);
                        if(nRet != APP_OK)
                        {
                            PrintError("P_CLI_MCP_StartV2xStatus() is failed![nRet:%d]", nRet);
                        }
                    }
                    else
                    {
                        return CLI_CMD_Showusage(pstCmd);
                    }
                }
                else
                {
                    return CLI_CMD_Showusage(pstCmd);
                }
            }
            else
            {
                return CLI_CMD_Showusage(pstCmd);
            }
        }
        else
        {
            return CLI_CMD_Showusage(pstCmd);
        }
    }

    return nRet;
}

int32_t CLI_MCP_InitCmds(void)
{
    int32_t nRet = APP_ERROR;

    nRet = CLI_CMD_AddCmd("mcp",
               P_CLI_MCP,
               NULL,
               "help for Multi Communication Performance commands",
               "mcp [enter command]\n\n"
               "Without any parameters, the 'mcp' show a description\n"
               "of available commands. For more details on a command, type and enter 'mcp'\n"
               "and the command name.\n\n"
               "mcp [OPTIONS]\n"
               "    test                         test mcp command\n"
               "    set                          set Device ID (should be set mcp ready first)\n"
               "    check                        check V2X scenario\n"
               "    base                         start a base Multi Communication Performance scenario\n"
               "    ready                        ready V2X scenario\n"
               "    start                        start V2X scenario (should be set mcp ready first)\n"
               "    stop                         stop V2X scenario\n"
               "    status start msg             start a test sample of V2X status data of msg tx\n"
               "    status start db              start a test sample of V2X status data of db\n"
               "    info                         get a status Multi Communication Performance\n"
               "mcp set [OPT] [PARAM]\n"
               "        dev   [id]               set device id Device ID\n"
               "        eth   [dev]              set ethernet i/f name\n"
               "        txrat [param ms]         set tx ratio\n"
               "        spd   [speed km/hrs]     set tx / rx vehicle speed\n"
               "        rg    [region id]        set region ID (1:SEOUL, 2:SEJONG, 3:BUSAN, 4:DAEGEON, 5:INCHEON\n"
               "                                                6:DAEGU, 7:DAEGU PG, 8:CHEONGJU, 9:SEONGNAM\n"
               "        ip    [ip]               set Ip (default ip : 192.168.1.11)\n"
               "        port  [port]             set port (default port : 47347)\n",
               "");
    if(nRet != APP_OK)
    {
        PrintError("CLI_CMD_AddCmd() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

