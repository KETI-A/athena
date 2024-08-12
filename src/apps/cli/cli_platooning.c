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
* @file cli_platooning.c
*
* @note
*
* CLI Platooning Source
*
******************************************************************************/


/***************************** Include ***************************************/
#include "cli.h"
#include "app.h"
#include "db_v2x.h"
#include "db_v2x_status.h"
#include "db_manager.h"
#include "framework.h"

/***************************** Definition ************************************/

/***************************** Static Variable *******************************/
static char s_chSetBufDevId[CLI_DB_V2X_DEFAULT_BUF_LEN];

/***************************** Function Protype ******************************/

static int P_CLI_PLATOONING_CheckPtSvc(void)
{
    int32_t nRet = APP_OK;
    SVC_PLATOONING_T *pstSvcPlatooning;
    pstSvcPlatooning = APP_GetSvcPlatooningInstance();

    nRet = SVC_PLATOONING_GetSettings(pstSvcPlatooning);
    if(nRet != APP_OK)
    {
        PrintError("SVC_PLATOONING_SetSettings() is failed! [nRet:%d]", nRet);
    }

    (void)SVC_PLATOONING_ShowSettings(pstSvcPlatooning);

    return nRet;
}

static int P_CLI_PLATOONING_SetPtSvc(CLI_CMDLINE_T *pstCmd)
{
    int32_t nRet = APP_OK;
    SVC_PLATOONING_T *pstSvcPlatooning;
    char *pcCmd;
    pstSvcPlatooning = APP_GetSvcPlatooningInstance();

    if(pstCmd == NULL)
    {
        PrintError("pstCmd == NULL!!");
        return CLI_CMD_Showusage(pstCmd);
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

        nRet = SVC_PLATOONING_GetSettings(pstSvcPlatooning);
        if(nRet != APP_OK)
        {
            PrintError("SVC_PLATOONING_SetSettings() is failed! [nRet:%d]", nRet);
        }

        pstSvcPlatooning->stDbV2x.unDeviceId = (uint32_t)atoi(pcCmd);
        sprintf(s_chSetBufDevId, "%s", pcCmd);
        pstSvcPlatooning->pchDeviceName = s_chSetBufDevId;

        if(strcmp(pstSvcPlatooning->pchDeviceName, DB_MGR_DEFAULT_COMM_DEV_ID) == 0)
        {
            PrintWarn("INSERT DEVICE ID is failed!");
            nRet = APP_ERROR;
        }
        else
        {
            PrintDebug("SET:unDeviceId[%d]", pstSvcPlatooning->stDbV2x.unDeviceId);
            PrintDebug("SET:pchDeviceName[%s]", pstSvcPlatooning->pchDeviceName);
        }

        nRet = SVC_PLATOONING_SetSettings(pstSvcPlatooning);
        if(nRet != APP_OK)
        {
            PrintError("SVC_PLATOONING_SetSettings() is failed! [nRet:%d]", nRet);
        }
    }
    else if (strcmp(pcCmd, "type") == 0)
    {
        pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
        if (strcmp(pcCmd, "lv") == 0)
        {
            DB_MANAGER_T *pstDbManager;

            pstDbManager = FRAMEWORK_GetDbManagerInstance();
            PrintDebug("pstDbManager[0x%p]", pstDbManager);

            nRet = SVC_PLATOONING_GetSettings(pstSvcPlatooning);
            if(nRet != APP_OK)
            {
                PrintError("SVC_PLATOONING_GetSettings() is failed! [nRet:%d]", nRet);
            }

            pstSvcPlatooning->stDbV2xPt.eDbV2XPtType = eDB_V2X_PT_TYPE_LV;
            pstDbManager->stDbV2xPt.eDbV2XPtType = eDB_V2X_PT_TYPE_LV;

            PrintDebug("SET:eDbV2XPtType[%d]", pstSvcPlatooning->stDbV2xPt.eDbV2XPtType);

            nRet = SVC_PLATOONING_SetSettings(pstSvcPlatooning);
            if(nRet != APP_OK)
            {
                PrintError("SVC_PLATOONING_SetSettings() is failed! [nRet:%d]", nRet);
            }
        }
        else if (strcmp(pcCmd, "fv") == 0)
        {
            DB_MANAGER_T *pstDbManager;

            pstDbManager = FRAMEWORK_GetDbManagerInstance();
            PrintDebug("pstDbManager[0x%p]", pstDbManager);

            nRet = SVC_PLATOONING_GetSettings(pstSvcPlatooning);
            if(nRet != APP_OK)
            {
                PrintError("SVC_PLATOONING_GetSettings() is failed! [nRet:%d]", nRet);
            }

            pstSvcPlatooning->stDbV2xPt.eDbV2XPtType = eDB_V2X_PT_TYPE_FV;
            pstDbManager->stDbV2xPt.eDbV2XPtType = eDB_V2X_PT_TYPE_FV;

            PrintDebug("SET:eDbV2XPtType[%d]", pstSvcPlatooning->stDbV2xPt.eDbV2XPtType);

            nRet = SVC_PLATOONING_SetSettings(pstSvcPlatooning);
            if(nRet != APP_OK)
            {
                PrintError("SVC_PLATOONING_SetSettings() is failed! [nRet:%d]", nRet);
            }
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

        nRet = SVC_PLATOONING_GetSettings(pstSvcPlatooning);
        if(nRet != APP_OK)
        {
            PrintError("SVC_PLATOONING_SetSettings() is failed! [nRet:%d]", nRet);
        }

        pstSvcPlatooning->stDbV2xStatusTx.unTxVehicleSpeed = (uint32_t)atoi(pcCmd);

        PrintDebug("SET:unTxVehicleSpeed[%d]", pstSvcPlatooning->stDbV2xStatusTx.unTxVehicleSpeed);

        nRet = SVC_PLATOONING_SetSettings(pstSvcPlatooning);
        if(nRet != APP_OK)
        {
            PrintError("SVC_PLATOONING_SetSettings() is failed! [nRet:%d]", nRet);
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

        nRet = SVC_PLATOONING_GetSettings(pstSvcPlatooning);
        if(nRet != APP_OK)
        {
            PrintError("SVC_PLATOONING_SetSettings() is failed! [nRet:%d]", nRet);
        }

        pstSvcPlatooning->stDbV2x.eRegionId = (uint32_t)atoi(pcCmd);

        PrintDebug("SET:eRegionId[0x%x]", pstSvcPlatooning->stDbV2x.eRegionId);

        nRet = SVC_PLATOONING_SetSettings(pstSvcPlatooning);
        if(nRet != APP_OK)
        {
            PrintError("SVC_PLATOONING_SetSettings() is failed! [nRet:%d]", nRet);
        }
    }
    else
    {
        PrintWarn("unknown set type");
        nRet = APP_ERROR;
    }
    return nRet;
}

static int P_CLI_PLATOONING_StartPtSvc(void)
{
    int32_t nRet = APP_OK;
    SVC_PLATOONING_T *pstSvcPlatooning;
    pstSvcPlatooning = APP_GetSvcPlatooningInstance();

    nRet = SVC_PLATOONING_Start(pstSvcPlatooning);
    if(nRet != APP_OK)
    {
        PrintError("SVC_PLATOONING_Start() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

static int P_CLI_PLATOONING_StopPtSvc(void)
{
    int32_t nRet = APP_OK;
    SVC_PLATOONING_T *pstSvcPlatooning;
    pstSvcPlatooning = APP_GetSvcPlatooningInstance();

    nRet = SVC_PLATOONING_Stop(pstSvcPlatooning);
    if(nRet != APP_OK)
    {
        PrintError("SVC_PLATOONING_Stop() is failed! [nRet:%d]", nRet);
    }

    nRet = SVC_PLATOONING_Close(pstSvcPlatooning);
    if(nRet != APP_OK)
    {
        PrintError("SVC_PLATOONING_Close() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

static int P_CLI_PLATOONING_ReadyPtSvc(void)
{
    int32_t nRet = APP_OK;
    SVC_PLATOONING_T *pstSvcPlatooning;
    pstSvcPlatooning = APP_GetSvcPlatooningInstance();

    (void)SVC_PLATOONING_ShowSettings(pstSvcPlatooning);

    nRet = SVC_PLATOONING_Open(pstSvcPlatooning);
    if (nRet != APP_OK)
    {
        PrintError("SVC_PLATOONING_Open() is failed! [nRet:%d]", nRet);
    }

    nRet = SVC_PLATOONING_GetSettings(pstSvcPlatooning);
    if (nRet != APP_OK)
    {
        PrintError("SVC_PLATOONING_GetSettings() is failed! [nRet:%d]", nRet);
    }

    nRet = SVC_PLATOONING_SetSettings(pstSvcPlatooning);
    if (nRet != APP_OK)
    {
        PrintError("SVC_PLATOONING_SetSettings() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}


static int P_CLI_PLATOONING(CLI_CMDLINE_T *pstCmd, int argc, char *argv[])
{
    int32_t nRet = APP_OK;
    char *pcCmd;

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
        else if(IS_CMD(pcCmd, "sce"))
        {
            pcCmd = CLI_CMD_GetArg(pstCmd, CMD_1);
            if(pcCmd != NULL)
            {
                if(IS_CMD(pcCmd, "base"))
                {
                    PrintTrace("Open DB");

                    PrintTrace("Connect V2X Device (OBU/RSU)");

                    PrintTrace("Setting Parameters");

                    PrintTrace("Start V2X Tx/Rx Communication of Platooning");

                    PrintTrace("Save DB");
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
        else if(IS_CMD(pcCmd, "set"))
        {
            pcCmd = CLI_CMD_GetArg(pstCmd, CMD_1);
            if (pcCmd != NULL)
            {
                if (IS_CMD(pcCmd, "dev"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if (pcCmd != NULL)
                    {
                        nRet = P_CLI_PLATOONING_SetPtSvc(pstCmd);
                        if(nRet != APP_OK)
                        {
                            PrintError("P_CLI_PLATOONING_SetPtSvc() is failed![nRet:%d]", nRet);
                        }
                    }
                }
                else if(IS_CMD(pcCmd, "type"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if (pcCmd != NULL)
                    {
                        if (IS_CMD(pcCmd, "lv"))
                        {
                            nRet = P_CLI_PLATOONING_SetPtSvc(pstCmd);
                            if(nRet != APP_OK)
                            {
                                PrintError("P_CLI_PLATOONING_SetPtSvc() is failed![nRet:%d]", nRet);
                            }
                        }
                        else if (IS_CMD(pcCmd, "fv"))
                        {
                            nRet = P_CLI_PLATOONING_SetPtSvc(pstCmd);
                            if(nRet != APP_OK)
                            {
                                PrintError("P_CLI_PLATOONING_SetPtSvc() is failed![nRet:%d]", nRet);
                            }
                        }
                    }
                }
                else if(IS_CMD(pcCmd, "spd"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        nRet = P_CLI_PLATOONING_SetPtSvc(pstCmd);
                        if(nRet != APP_OK)
                        {
                            PrintError("P_CLI_PLATOONING_SetPtSvc() is failed![nRet:%d]", nRet);
                        }
                    }
                }
                else if(IS_CMD(pcCmd, "rg"))
                {
                    pcCmd = CLI_CMD_GetArg(pstCmd, CMD_2);
                    if(pcCmd != NULL)
                    {
                        nRet = P_CLI_PLATOONING_SetPtSvc(pstCmd);
                        if(nRet != APP_OK)
                        {
                            PrintError("P_CLI_PLATOONING_SetPtSvc() is failed![nRet:%d]", nRet);
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
        else if(IS_CMD(pcCmd, "start"))
        {
            nRet = P_CLI_PLATOONING_StartPtSvc();
            if (nRet != APP_OK)
            {
                PrintError("P_CLI_PLATOONING_StartPtSvc() is failed! [nRet:%d]", nRet);
            }
        }
        else if(IS_CMD(pcCmd, "stop"))
        {
            nRet = P_CLI_PLATOONING_StopPtSvc();
            if (nRet != APP_OK)
            {
                PrintError("P_CLI_PLATOONING_StopPtSvc() is failed! [nRet:%d]", nRet);
            }
        }
        else if(IS_CMD(pcCmd, "ready"))
        {
            nRet = P_CLI_PLATOONING_ReadyPtSvc();
            if (nRet != APP_OK)
            {
                PrintError("P_CLI_PLATOONING_ReadyPtSvc() is failed![nRet:%d]", nRet);
            }
        }
        else if(IS_CMD(pcCmd, "check"))
        {
            nRet = P_CLI_PLATOONING_CheckPtSvc();
            if (nRet != APP_OK)
            {
                PrintError("P_CLI_PLATOONING_CheckPtSvc() is failed![nRet:%d]", nRet);
            }
        }
        else
        {
            return CLI_CMD_Showusage(pstCmd);
        }
    }

	return nRet;
}

int32_t CLI_PLATOONING_InitCmds(void)
{
    int32_t nRet = APP_ERROR;

    nRet = CLI_CMD_AddCmd("pt",
               P_CLI_PLATOONING,
               NULL,
               "help for Platooning commands",
               "pt [enter command]\n\n"
               "Without any parameters, the 'pt' show a description\n"
               "of available commands. For more details on a command, type and enter 'pt'\n"
               "and the command name.\n\n"
               "pt [OPTIONS]\n"
               "   test                         test pt command\n"
               "   ready                        ready platooning scenario\n"
               "   start                        start platooning scenario (should be set pt ready first)\n"
               "   stop                         stop  platooning scenario\n"
               "   check                        check platooning scenario\n"
               "pt set [OPT] [PARAM]\n"
               "       type  [lv/fv]            set vehicle type\n"
               "       dev   [id]               set device id\n"
               "       spd   [speed km/hrs]     set tx / rx vehicle speed\n"
               "       rg    [region id]        set region ID (1:SEOUL, 2:SEJONG, 3:BUSAN, 4:DAEGEON, 5:INCHEON\n"
               "                                               6:DAEGU, 7:DAEGU PG, 8:CHEONGJU, 9:SEONGNAM)\n"
               "pt sce [OPTIONS]\n"
               "  base                          start a base platooing scenario\n"
               "pt info                         get a status platooning\n",
               "");
    if(nRet != APP_OK)
    {
        PrintError("CLI_CMD_AddCmd() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

