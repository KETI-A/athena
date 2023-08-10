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
* @file cli_db.c
*
* @note
*
* CLI DB Source
*
******************************************************************************/


/***************************** Include ***************************************/
#include "cli.h"
#include "app.h"
#include "db_v2x.h"
#include "db_manager.h"
#include "framework.h"

/***************************** Definition ************************************/

/***************************** Static Variable *******************************/


/***************************** Function Protype ******************************/

static int P_CLI_DB(CLI_CMDLINE_T *pstCmd, int argc, char *argv[])
{
    int32_t nRet = APP_OK;
    int nFrameWorkRet = FRAMEWORK_ERROR;
    char *pcCmd;
    DB_MANAGER_WRITE_T stDbManagerWrite;
    DB_V2X_T stDbV2x;
    char cPayload[CLI_DB_V2X_DEFAULT_PAYLOAD_LEN];

    (void*)memset(&stDbManagerWrite, 0x00, sizeof(DB_MANAGER_WRITE_T));
    (void*)memset(&stDbV2x, 0x00, sizeof(DB_V2X_T));
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
        else if(IS_CMD(pcCmd, "v2x"))
        {
            pcCmd = CLI_CMD_GetArg(pstCmd, CMD_1);
            if (pcCmd != NULL)
            {
                if(IS_CMD(pcCmd, "txt"))
                {
                    TIME_MANAGER_T *pstTimeManager;

                    stDbManagerWrite.eFileType = DB_MANAGER_FILE_TYPE_TXT;
                    stDbManagerWrite.eCommMsgType = DB_MANAGER_COMM_MSG_TYPE_TX;
                    stDbManagerWrite.eProc = DB_MANAGER_PROC_WRITE;

                    stDbV2x.eDeviceType = DB_V2X_DEVICE_TYPE_OBU;
                    stDbV2x.eTeleCommType = DB_V2X_TELECOMM_TYPE_5G_PC5_BROADCAST;
                    stDbV2x.unDeviceId = CLI_DB_V2X_DEFAULT_DEVICE_ID;

                    pstTimeManager = FRAMEWORK_GetTimeManagerInstance();
                    nFrameWorkRet = TIME_MANAGER_Get(pstTimeManager);
                    if(nFrameWorkRet != FRAMEWORK_OK)
                    {
                        PrintError("TIME_MANAGER_Get() is failed! [nRet:%d]", nFrameWorkRet);
                    }
                    else
                    {
                        PrintTrace("Get:Current a timestamp is [%ld]", pstTimeManager->ulTimeStamp);
                    }
                    stDbV2x.ulTimeStamp = pstTimeManager->ulTimeStamp;
                    stDbV2x.eServiceId = DB_V2X_SERVICE_ID_PLATOONING;
                    stDbV2x.eActionType = DB_V2X_ACTION_TYPE_REQUEST;
                    stDbV2x.eRegionId = DB_V2X_REGION_ID_SEONGNAM;
                    stDbV2x.ePayloadType = DB_V2X_PAYLOAD_TYPE_PLATOONING;
                    stDbV2x.eCommId = DB_V2X_COMM_ID_V2V;
                    stDbV2x.usDbVer = (DB_V2X_VERSION_MAJOR << CLI_DB_V2X_MAJOR_SHIFT) | DB_V2X_VERSION_MINOR;
                    stDbV2x.usHwVer = CLI_DB_V2X_DEFAULT_HW_VER;
                    stDbV2x.usSwVer = CLI_DB_V2X_DEFAULT_SW_VER;
                    stDbV2x.ulPayloadLength = sizeof(cPayload);
                    stDbV2x.ulPacketCrc32 = 0;

                    PrintTrace("========================================================");
                    PrintDebug("eFileType[%d]", stDbManagerWrite.eFileType);
                    PrintDebug("eCommMsgType[%d]", stDbManagerWrite.eCommMsgType);
                    PrintDebug("eProc[%d]", stDbManagerWrite.eProc);

                    PrintDebug("eDeviceType[%d]", stDbV2x.eDeviceType);
                    PrintDebug("eTeleCommType[%d]", stDbV2x.eTeleCommType);
                    PrintDebug("unDeviceId[0x%x]", stDbV2x.unDeviceId);
                    PrintDebug("ulTimeStamp[%ld]", stDbV2x.ulTimeStamp);
                    PrintDebug("eServiceId[%d]", stDbV2x.eServiceId);
                    PrintDebug("eActionType[%d]", stDbV2x.eActionType);
                    PrintDebug("eRegionId[%d]", stDbV2x.eRegionId);
                    PrintDebug("ePayloadType[%d]", stDbV2x.ePayloadType);
                    PrintDebug("eCommId[%d]", stDbV2x.eCommId);
                    PrintDebug("usDbVer[%d.%d]", stDbV2x.usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, stDbV2x.usDbVer & CLI_DB_V2X_MINOR_MASK);
                    PrintDebug("usHwVer[%d]", stDbV2x.usHwVer);
                    PrintDebug("usSwVer[%d]", stDbV2x.usSwVer);
                    PrintDebug("ulPayloadLength[%d]", stDbV2x.ulPayloadLength);
                    PrintDebug("cPayload");
                    for(int i=0; i < CLI_DB_V2X_DEFAULT_PAYLOAD_LEN; i++)
                    {
                        cPayload[i] = rand();
                        printf("[%d:%d] ", i, cPayload[i]);
                    }
                    printf("\r\n");

                    stDbV2x.ulPacketCrc32 = CLI_UTIL_GetCrc32((uint8_t*)&cPayload, stDbV2x.ulPayloadLength);

                    PrintDebug("ulPayloadCrc32[0x%x]", stDbV2x.ulPacketCrc32);
                    PrintTrace("========================================================");

                    nFrameWorkRet = DB_MANAGER_Write(&stDbManagerWrite, &stDbV2x, (char*)&cPayload);
                    if(nFrameWorkRet != FRAMEWORK_OK)
                    {
                        PrintError("DB_MANAGER_Write() is failed! [nRet:%d]", nFrameWorkRet);
                    }
                }
                else if(IS_CMD(pcCmd,"csv"))
                {
                    TIME_MANAGER_T *pstTimeManager;

                    stDbManagerWrite.eFileType = DB_MANAGER_FILE_TYPE_CSV;
                    stDbManagerWrite.eCommMsgType = DB_MANAGER_COMM_MSG_TYPE_TX;
                    stDbManagerWrite.eProc = DB_MANAGER_PROC_WRITE;

                    stDbV2x.eDeviceType = DB_V2X_DEVICE_TYPE_OBU;
                    stDbV2x.eTeleCommType = DB_V2X_TELECOMM_TYPE_5G_PC5_BROADCAST;
                    stDbV2x.unDeviceId = CLI_DB_V2X_DEFAULT_DEVICE_ID;

                    pstTimeManager = FRAMEWORK_GetTimeManagerInstance();
                    nFrameWorkRet = TIME_MANAGER_Get(pstTimeManager);
                    if(nFrameWorkRet != FRAMEWORK_OK)
                    {
                        PrintError("TIME_MANAGER_Get() is failed! [nRet:%d]", nFrameWorkRet);
                    }
                    else
                    {
                        PrintTrace("Get:Current a timestamp is [%ld]", pstTimeManager->ulTimeStamp);
                    }
                    stDbV2x.ulTimeStamp = pstTimeManager->ulTimeStamp;
                    stDbV2x.eServiceId = DB_V2X_SERVICE_ID_PLATOONING;
                    stDbV2x.eActionType = DB_V2X_ACTION_TYPE_REQUEST;
                    stDbV2x.eRegionId = DB_V2X_REGION_ID_SEONGNAM;
                    stDbV2x.ePayloadType = DB_V2X_PAYLOAD_TYPE_PLATOONING;
                    stDbV2x.eCommId = DB_V2X_COMM_ID_V2V;
                    stDbV2x.usDbVer = (DB_V2X_VERSION_MAJOR << CLI_DB_V2X_MAJOR_SHIFT) | DB_V2X_VERSION_MINOR;
                    stDbV2x.usHwVer = CLI_DB_V2X_DEFAULT_HW_VER;
                    stDbV2x.usSwVer = CLI_DB_V2X_DEFAULT_SW_VER;
                    stDbV2x.ulPayloadLength = sizeof(cPayload);
                    stDbV2x.ulPacketCrc32 = 0;

                    PrintTrace("========================================================");
                    PrintDebug("eFileType[%d]", stDbManagerWrite.eFileType);
                    PrintDebug("eCommMsgType[%d]", stDbManagerWrite.eCommMsgType);
                    PrintDebug("eProc[%d]", stDbManagerWrite.eProc);

                    PrintDebug("eDeviceType[%d]", stDbV2x.eDeviceType);
                    PrintDebug("eTeleCommType[%d]", stDbV2x.eTeleCommType);
                    PrintDebug("unDeviceId[0x%x]", stDbV2x.unDeviceId);
                    PrintDebug("ulTimeStamp[%ld]", stDbV2x.ulTimeStamp);
                    PrintDebug("eServiceId[%d]", stDbV2x.eServiceId);
                    PrintDebug("eActionType[%d]", stDbV2x.eActionType);
                    PrintDebug("eRegionId[%d]", stDbV2x.eRegionId);
                    PrintDebug("ePayloadType[%d]", stDbV2x.ePayloadType);
                    PrintDebug("eCommId[%d]", stDbV2x.eCommId);
                    PrintDebug("usDbVer[%d.%d]", stDbV2x.usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, stDbV2x.usDbVer & CLI_DB_V2X_MINOR_MASK);
                    PrintDebug("usHwVer[%d]", stDbV2x.usHwVer);
                    PrintDebug("usSwVer[%d]", stDbV2x.usSwVer);
                    PrintDebug("ulPayloadLength[%d]", stDbV2x.ulPayloadLength);
                    PrintDebug("cPayload");
                    for(int i=0; i < CLI_DB_V2X_DEFAULT_PAYLOAD_LEN; i++)
                    {
                        cPayload[i] = rand();
                        printf("[%d:%d] ", i, cPayload[i]);
                    }
                    printf("\r\n");

                    stDbV2x.ulPacketCrc32 = CLI_UTIL_GetCrc32((uint8_t*)&cPayload, stDbV2x.ulPayloadLength);

                    PrintDebug("ulPayloadCrc32[0x%x]", stDbV2x.ulPacketCrc32);
                    PrintTrace("========================================================");

                    nFrameWorkRet = DB_MANAGER_Write(&stDbManagerWrite, &stDbV2x, (char*)&cPayload);
                    if(nFrameWorkRet != FRAMEWORK_OK)
                    {
                        PrintError("DB_MANAGER_Write() is failed! [nRet:%d]", nFrameWorkRet);
                    }
                }
                if(IS_CMD(pcCmd, "sqlite"))
                {
                    TIME_MANAGER_T *pstTimeManager;

                    stDbManagerWrite.eFileType = DB_MANAGER_FILE_TYPE_SQLITE;
                    stDbManagerWrite.eCommMsgType = DB_MANAGER_COMM_MSG_TYPE_TX;
                    stDbManagerWrite.eProc = DB_MANAGER_PROC_WRITE;

                    stDbV2x.eDeviceType = DB_V2X_DEVICE_TYPE_OBU;
                    stDbV2x.eTeleCommType = DB_V2X_TELECOMM_TYPE_5G_PC5_BROADCAST;
                    stDbV2x.unDeviceId = CLI_DB_V2X_DEFAULT_DEVICE_ID;

                    pstTimeManager = FRAMEWORK_GetTimeManagerInstance();
                    nFrameWorkRet = TIME_MANAGER_Get(pstTimeManager);
                    if(nFrameWorkRet != FRAMEWORK_OK)
                    {
                        PrintError("TIME_MANAGER_Get() is failed! [nRet:%d]", nFrameWorkRet);
                    }
                    else
                    {
                        PrintTrace("Get:Current a timestamp is [%ld]", pstTimeManager->ulTimeStamp);
                    }
                    stDbV2x.ulTimeStamp = pstTimeManager->ulTimeStamp;
                    stDbV2x.eServiceId = DB_V2X_SERVICE_ID_PLATOONING;
                    stDbV2x.eActionType = DB_V2X_ACTION_TYPE_REQUEST;
                    stDbV2x.eRegionId = DB_V2X_REGION_ID_SEONGNAM;
                    stDbV2x.ePayloadType = DB_V2X_PAYLOAD_TYPE_PLATOONING;
                    stDbV2x.eCommId = DB_V2X_COMM_ID_V2V;
                    stDbV2x.usDbVer = (DB_V2X_VERSION_MAJOR << CLI_DB_V2X_MAJOR_SHIFT) | DB_V2X_VERSION_MINOR;
                    stDbV2x.usHwVer = CLI_DB_V2X_DEFAULT_HW_VER;
                    stDbV2x.usSwVer = CLI_DB_V2X_DEFAULT_SW_VER;
                    stDbV2x.ulPayloadLength = sizeof(cPayload);
                    stDbV2x.ulPacketCrc32 = 0;

                    PrintTrace("========================================================");
                    PrintDebug("eFileType[%d]", stDbManagerWrite.eFileType);
                    PrintDebug("eCommMsgType[%d]", stDbManagerWrite.eCommMsgType);
                    PrintDebug("eProc[%d]", stDbManagerWrite.eProc);

                    PrintDebug("eDeviceType[%d]", stDbV2x.eDeviceType);
                    PrintDebug("eTeleCommType[%d]", stDbV2x.eTeleCommType);
                    PrintDebug("unDeviceId[0x%x]", stDbV2x.unDeviceId);
                    PrintDebug("ulTimeStamp[%ld]", stDbV2x.ulTimeStamp);
                    PrintDebug("eServiceId[%d]", stDbV2x.eServiceId);
                    PrintDebug("eActionType[%d]", stDbV2x.eActionType);
                    PrintDebug("eRegionId[%d]", stDbV2x.eRegionId);
                    PrintDebug("ePayloadType[%d]", stDbV2x.ePayloadType);
                    PrintDebug("eCommId[%d]", stDbV2x.eCommId);
                    PrintDebug("usDbVer[%d.%d]", stDbV2x.usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, stDbV2x.usDbVer & CLI_DB_V2X_MINOR_MASK);
                    PrintDebug("usHwVer[%d]", stDbV2x.usHwVer);
                    PrintDebug("usSwVer[%d]", stDbV2x.usSwVer);
                    PrintDebug("ulPayloadLength[%d]", stDbV2x.ulPayloadLength);
                    PrintDebug("cPayload");
                    for(int i=0; i < CLI_DB_V2X_DEFAULT_PAYLOAD_LEN; i++)
                    {
                        cPayload[i] = rand();
                        printf("[%d:%d] ", i, cPayload[i]);
                    }
                    printf("\r\n");

                    stDbV2x.ulPacketCrc32 = CLI_UTIL_GetCrc32((uint8_t*)&cPayload, stDbV2x.ulPayloadLength);

                    PrintDebug("ulPayloadCrc32[0x%x]", stDbV2x.ulPacketCrc32);
                    PrintTrace("========================================================");

                    nFrameWorkRet = DB_MANAGER_Write(&stDbManagerWrite, &stDbV2x, (char*)&cPayload);
                    if(nFrameWorkRet != FRAMEWORK_OK)
                    {
                        PrintError("DB_MANAGER_Write() is failed! [nRet:%d]", nFrameWorkRet);
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
        else if(IS_CMD(pcCmd, "v2xC"))
        {
            pcCmd = CLI_CMD_GetArg(pstCmd, CMD_1);
            if (pcCmd != NULL)
            {
                if(IS_CMD(pcCmd,"csv"))
                {
                    for(int a=0; a < 244; a++)
                    {
                        TIME_MANAGER_T *pstTimeManager;

                        stDbManagerWrite.eFileType = DB_MANAGER_FILE_TYPE_CSV;
                        stDbManagerWrite.eCommMsgType = DB_MANAGER_COMM_MSG_TYPE_TX;
                        stDbManagerWrite.eProc = DB_MANAGER_PROC_WRITE;

                        stDbV2x.eDeviceType = DB_V2X_DEVICE_TYPE_OBU;
                        stDbV2x.eTeleCommType = DB_V2X_TELECOMM_TYPE_5G_PC5_BROADCAST;
                        stDbV2x.unDeviceId = CLI_DB_V2X_DEFAULT_DEVICE_ID;

                        pstTimeManager = FRAMEWORK_GetTimeManagerInstance();
                        nFrameWorkRet = TIME_MANAGER_Get(pstTimeManager);
                        if(nFrameWorkRet != FRAMEWORK_OK)
                        {
                            PrintError("TIME_MANAGER_Get() is failed! [nRet:%d]", nFrameWorkRet);
                        }
                        else
                        {
                            PrintTrace("Get:Current a timestamp is [%ld]", pstTimeManager->ulTimeStamp);
                        }
                        stDbV2x.ulTimeStamp = pstTimeManager->ulTimeStamp;
                        stDbV2x.eServiceId = DB_V2X_SERVICE_ID_PLATOONING;
                        stDbV2x.eActionType = DB_V2X_ACTION_TYPE_REQUEST;
                        stDbV2x.eRegionId = DB_V2X_REGION_ID_SEONGNAM;
                        stDbV2x.ePayloadType = DB_V2X_PAYLOAD_TYPE_PLATOONING;
                        stDbV2x.eCommId = DB_V2X_COMM_ID_V2V;
                        stDbV2x.usDbVer = (DB_V2X_VERSION_MAJOR << CLI_DB_V2X_MAJOR_SHIFT) | DB_V2X_VERSION_MINOR;
                        stDbV2x.usHwVer = CLI_DB_V2X_DEFAULT_HW_VER;
                        stDbV2x.usSwVer = CLI_DB_V2X_DEFAULT_SW_VER;
                        stDbV2x.ulPayloadLength = sizeof(cPayload);
                        stDbV2x.ulPacketCrc32 = 0;

                        PrintTrace("========================================================");
                        PrintDebug("eFileType[%d]", stDbManagerWrite.eFileType);
                        PrintDebug("eCommMsgType[%d]", stDbManagerWrite.eCommMsgType);
                        PrintDebug("eProc[%d]", stDbManagerWrite.eProc);

                        PrintDebug("eDeviceType[%d]", stDbV2x.eDeviceType);
                        PrintDebug("eTeleCommType[%d]", stDbV2x.eTeleCommType);
                        PrintDebug("unDeviceId[0x%x]", stDbV2x.unDeviceId);
                        PrintDebug("ulTimeStamp[%ld]", stDbV2x.ulTimeStamp);
                        PrintDebug("eServiceId[%d]", stDbV2x.eServiceId);
                        PrintDebug("eActionType[%d]", stDbV2x.eActionType);
                        PrintDebug("eRegionId[%d]", stDbV2x.eRegionId);
                        PrintDebug("ePayloadType[%d]", stDbV2x.ePayloadType);
                        PrintDebug("eCommId[%d]", stDbV2x.eCommId);
                        PrintDebug("usDbVer[%d.%d]", stDbV2x.usDbVer >> CLI_DB_V2X_MAJOR_SHIFT, stDbV2x.usDbVer & CLI_DB_V2X_MINOR_MASK);
                        PrintDebug("usHwVer[%d]", stDbV2x.usHwVer);
                        PrintDebug("usSwVer[%d]", stDbV2x.usSwVer);
                        PrintDebug("ulPayloadLength[%d]", stDbV2x.ulPayloadLength);
                        PrintDebug("cPayload");
   
                        for(int i=0; i < CLI_DB_V2X_DEFAULT_PAYLOAD_LEN; i++)
                        {
                            cPayload[i] = a;
                            printf("[%d:%d] ", a, cPayload[i]);
                        }
                        printf("\r\n");

                        stDbV2x.ulPacketCrc32 = CLI_UTIL_GetCrc32((uint8_t*)&cPayload, stDbV2x.ulPayloadLength);

                        PrintDebug("ulPayloadCrc32[0x%x]", stDbV2x.ulPacketCrc32);
                        PrintTrace("========================================================");

                        nFrameWorkRet = DB_MANAGER_Write(&stDbManagerWrite, &stDbV2x, (char*)&cPayload);
                        if(nFrameWorkRet != FRAMEWORK_OK)
                        {
                            PrintError("DB_MANAGER_Write() is failed! [nRet:%d]", nFrameWorkRet);
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
        else if(IS_CMD(pcCmd, "open"))
        {
            pcCmd = CLI_CMD_GetArg(pstCmd, CMD_1);
            if (pcCmd != NULL)
            {
                if(IS_CMD(pcCmd, "txt"))
                {
                    DB_MANAGER_T *pstDbManager;

                    pstDbManager = FRAMEWORK_GetDbManagerInstance();
                    PrintDebug("pstDbManager[0x%p]", pstDbManager);

                    pstDbManager->eFileType = DB_MANAGER_FILE_TYPE_TXT;

                    nFrameWorkRet = DB_MANAGER_Open(pstDbManager);
                    if(nFrameWorkRet != FRAMEWORK_OK)
                    {
                        PrintError("DB_MANAGER_Open() is failed! [nRet:%d]", nFrameWorkRet);
                    }
                }
                else if(IS_CMD(pcCmd, "csv"))
                {
                    DB_MANAGER_T *pstDbManager;

                    pstDbManager = FRAMEWORK_GetDbManagerInstance();
                    PrintDebug("pstDbManager[0x%p]", pstDbManager);

                    pstDbManager->eFileType = DB_MANAGER_FILE_TYPE_CSV;

                    nFrameWorkRet = DB_MANAGER_Open(pstDbManager);
                    if(nFrameWorkRet != FRAMEWORK_OK)
                    {
                        PrintError("DB_MANAGER_Open() is failed! [nRet:%d]", nFrameWorkRet);
                    }
                }
                else if(IS_CMD(pcCmd, "sqlite"))
                {
                    DB_MANAGER_T *pstDbManager;

                    pstDbManager = FRAMEWORK_GetDbManagerInstance();
                    PrintDebug("pstDbManager[0x%p]", pstDbManager);

                    pstDbManager->eFileType = DB_MANAGER_FILE_TYPE_SQLITE;

                    nFrameWorkRet = DB_MANAGER_Open(pstDbManager);
                    if(nFrameWorkRet != FRAMEWORK_OK)
                    {
                        PrintError("DB_MANAGER_Open() is failed! [nRet:%d]", nFrameWorkRet);
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
        else if(IS_CMD(pcCmd, "close"))
        {
            DB_MANAGER_T *pstDbManager;

            pstDbManager = FRAMEWORK_GetDbManagerInstance();
            PrintDebug("pstDbManager[0x%p]", pstDbManager);

            nFrameWorkRet = DB_MANAGER_Close(pstDbManager);
            if(nFrameWorkRet != FRAMEWORK_OK)
            {
                PrintError("DB_MANAGER_Close() is failed! [nRet:%d]", nFrameWorkRet);
            }
        }
        else if(IS_CMD(pcCmd, "time"))
        {
            TIME_MANAGER_T *pstTimeManager;

            pstTimeManager = FRAMEWORK_GetTimeManagerInstance();
            PrintDebug("pstTimeManager[0x%p]", pstTimeManager);

            nFrameWorkRet = TIME_MANAGER_Get(pstTimeManager);
            if(nFrameWorkRet != FRAMEWORK_OK)
            {
                PrintError("TIME_MANAGER_Get() is failed! [nRet:%d]", nFrameWorkRet);
            }
            else
            {
                PrintTrace("Get:Current a timestamp is [%ld]", pstTimeManager->ulTimeStamp);
            }
        }
        else
        {
            return CLI_CMD_Showusage(pstCmd);
        }
    }

	return nRet;
}

int32_t CLI_DB_InitCmds(void)
{
    int32_t nRet = APP_ERROR;

    nRet = CLI_CMD_AddCmd("db",
               P_CLI_DB,
               NULL,
               "help for DB commands",
               "db [enter command]\n\n"
               "Without any parameters, the 'db' show a description\n"
               "of available commands. For more details on a command, type and enter 'db'\n"
               "and the command name.\n\n"
               "db test           test db command\n"
               "db v2x txt        save TXT file, test db v2x sample command (first, set CLI> db open)\n"
               "db v2x csv        save CSV file, test db v2x sample command (first, set CLI> db open)\n"
               "db v2x sqlite     save sqlte file, test db v2x sample command (first, set CLI> db open)\n"
               "db v2xC csv       save CSV file, Sequentially test db v2x sample command  (first, set CLI> db open)\n"
               "db open txt       open a db TXT file\n"
               "db open csv       open a db CSV file\n"
               "db open sqlite    open a db sqlite file\n"               
               "db close          close a db file\n"
               "db time           get a current timestamp\n",
               "");
    if(nRet != APP_OK)
    {
        PrintError("CLI_CMD_AddCmd() is failed! [nRet:%d]", nRet);
    }

    return nRet;
}

