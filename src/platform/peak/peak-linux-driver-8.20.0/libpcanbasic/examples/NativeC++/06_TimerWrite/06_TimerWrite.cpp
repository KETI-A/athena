/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * 06_TimerWrite.cpp - PCANBasic Example: TimerWrite
 *
 * Copyright (C) 2001-2020  PEAK System-Technik GmbH <www.peak-system.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Contact:    <linux@peak-system.com>
 * Maintainer:  Fabrice Vergnaud <f.vergnaud@peak-system.com>
 *                 Romain Tissier <r.tissier@peak-system.com>
 */
#include "06_TimerWrite.h"

TimerWrite::TimerWrite()
{
    ShowConfigurationHelp(); // Shows information about this sample
    ShowCurrentConfiguration(); // Shows the current parameters configuration

    // Checks if PCANBasic.dll is available, if not, the program terminates
    m_DLLFound = CheckForLibrary();
    if (!m_DLLFound)
        return;

    TPCANStatus stsResult;
    // Initialization of the selected channel
    if (IsFD)
        stsResult = CAN_InitializeFD(PcanHandle, BitrateFD);
    else
        stsResult = CAN_Initialize(PcanHandle, Bitrate);

    if (stsResult != PCAN_ERROR_OK)
    {
        std::cout << "Can not initialize. Please check the defines in the code.\n";
        ShowStatus(stsResult);
        std::cout << "\n";
        std::cout << "Closing...\n";
        std::cout << "Press any key to continue...\n";
        _getch();
        return;
    }

    // Writing messages...
    std::cout << "Successfully initialized.\n";
    m_TimerOn = true;
    m_hTimer = new std::thread(&TimerWrite::TimerThread, this);
    std::cout << "Started writing messages...\n";
    std::cout << "\n";
    std::cout << "For closing: ";
    std::cout << "Press any key to continue...\n";
    _getch();
}

TimerWrite::~TimerWrite()
{
    m_TimerOn = false;
    m_hTimer->join();
    delete m_hTimer;
    if (m_DLLFound)
        CAN_Uninitialize(PCAN_NONEBUS);
}

void TimerWrite::TimerThread()
{
    while (m_TimerOn)
    {
        usleep(TimerInterval*1000);
        WriteMessages();
    }
}

void TimerWrite::WriteMessages()
{
    TPCANStatus stsResult;

    if (IsFD)
        stsResult = WriteMessageFD();
    else
        stsResult = WriteMessage();

    // Checks if the message was sent
    if (stsResult != PCAN_ERROR_OK)
        ShowStatus(stsResult);
    else
        std::cout << "\nMessage was successfully SENT";
}

TPCANStatus TimerWrite::WriteMessage()
{
    // Sends a CAN message with extended ID, and 8 data bytes
    TPCANMsg msgCanMessage;
    msgCanMessage.ID = 0x100;
    msgCanMessage.LEN = (BYTE)8;
    msgCanMessage.MSGTYPE = PCAN_MESSAGE_EXTENDED;
    for (BYTE i = 0; i < 8; i++)
    {
        msgCanMessage.DATA[i] = i;
    }
    return CAN_Write(PcanHandle, &msgCanMessage);
}

TPCANStatus TimerWrite::WriteMessageFD()
{
    // Sends a CAN-FD message with standard ID, 64 data bytes, and bitrate switch
    TPCANMsgFD msgCanMessageFD;
    msgCanMessageFD.ID = 0x100;
    msgCanMessageFD.DLC = 15;
    msgCanMessageFD.MSGTYPE = PCAN_MESSAGE_FD | PCAN_MESSAGE_BRS;
    for (BYTE i = 0; i < 64; i++)
    {
        msgCanMessageFD.DATA[i] = i;
    }
    return CAN_WriteFD(PcanHandle, &msgCanMessageFD);
}

bool TimerWrite::CheckForLibrary()
{
    // Check for dll file
    try
    {
        CAN_Uninitialize(PCAN_NONEBUS);
        return true;
    }
    catch (const std::exception&)
    {
        std::cout << ("Unable to find the library: PCANBasic::dll !\n");
        std::cout << ("Closing...\n");
        std::cout << "Press any key to continue...\n";
        _getch();
    }

    return false;
}

void TimerWrite::ShowConfigurationHelp()
{
    std::cout << "=========================================================================================\n";
    std::cout << "|                         PCAN-Basic TimerWrite Example                               |\n";
    std::cout << "=========================================================================================\n";
    std::cout << "Following parameters are to be adjusted before launching, according to the hardware used |\n";
    std::cout << "                                                                                         |\n";
    std::cout << "* PcanHandle: Numeric value that represents the handle of the PCAN-Basic channel to use. |\n";
    std::cout << "              See 'PCAN-Handle Definitions' within the documentation                     |\n";
    std::cout << "* IsFD: Boolean value that indicates the communication mode, CAN (false) or CAN-FD (true)|\n";
    std::cout << "* Bitrate: Numeric value that represents the BTR0/BR1 bitrate value to be used for CAN   |\n";
    std::cout << "           communication                                                                 |\n";
    std::cout << "* BitrateFD: String value that represents the nominal/data bitrate value to be used for  |\n";
    std::cout << "             CAN-FD communication                                                        |\n";
    std::cout << "* TimerInterval: The time, in milliseconds, to wait before trying to read the messages   |\n";
    std::cout << "=========================================================================================\n";
    std::cout << "\n";
}

void TimerWrite::ShowCurrentConfiguration()
{
    std::cout << "Parameter values used\n";
    std::cout << "----------------------\n";
    char buffer[MAX_PATH];
    FormatChannelName(PcanHandle, buffer, IsFD);
    std::cout << "* PCANHandle: " << buffer << "\n";
    if (IsFD)
        std::cout << "* IsFD: True\n";
    else
        std::cout << "* IsFD: False\n";
    ConvertBitrateToString(Bitrate, buffer);
    std::cout << "* Bitrate: " << buffer << "\n";
    std::cout << "* BitrateFD: " << BitrateFD << "\n";
    std::cout << "* TimerInterval: " << TimerInterval << "\n";
    std::cout << "\n";
}

void TimerWrite::ShowStatus(TPCANStatus status)
{
    std::cout << "=========================================================================================\n";
    char buffer[MAX_PATH];
    GetFormattedError(status, buffer);
    std::cout << buffer << "\n";
    std::cout << "=========================================================================================\n";
}

void TimerWrite::FormatChannelName(TPCANHandle handle, LPSTR buffer, bool isFD)
{
    TPCANDevice devDevice;
    BYTE byChannel;

    // Gets the owner device and channel for a PCAN-Basic handle
    if (handle < 0x100)
    {
        devDevice = (TPCANDevice)(handle >> 4);
        byChannel = (BYTE)(handle & 0xF);
    }
    else
    {
        devDevice = (TPCANDevice)(handle >> 8);
        byChannel = (BYTE)(handle & 0xFF);
    }

    // Constructs the PCAN-Basic Channel name and return it
    char handleBuffer[MAX_PATH];
    GetTPCANHandleName(handle, handleBuffer);
    if (isFD)
        sprintf_s(buffer, MAX_PATH, "%s:FD %d (%Xh)", handleBuffer, byChannel, handle);
    else
        sprintf_s(buffer, MAX_PATH, "%s %d (%Xh)", handleBuffer, byChannel, handle);
}

void TimerWrite::GetTPCANHandleName(TPCANHandle handle, LPSTR buffer)
{
    strcpy_s(buffer, MAX_PATH, "PCAN_NONE");
    switch (handle)
    {
    case PCAN_PCIBUS1:
    case PCAN_PCIBUS2:
    case PCAN_PCIBUS3:
    case PCAN_PCIBUS4:
    case PCAN_PCIBUS5:
    case PCAN_PCIBUS6:
    case PCAN_PCIBUS7:
    case PCAN_PCIBUS8:
    case PCAN_PCIBUS9:
    case PCAN_PCIBUS10:
    case PCAN_PCIBUS11:
    case PCAN_PCIBUS12:
    case PCAN_PCIBUS13:
    case PCAN_PCIBUS14:
    case PCAN_PCIBUS15:
    case PCAN_PCIBUS16:
        strcpy_s(buffer, MAX_PATH, "PCAN_PCI");
        break;

    case PCAN_USBBUS1:
    case PCAN_USBBUS2:
    case PCAN_USBBUS3:
    case PCAN_USBBUS4:
    case PCAN_USBBUS5:
    case PCAN_USBBUS6:
    case PCAN_USBBUS7:
    case PCAN_USBBUS8:
    case PCAN_USBBUS9:
    case PCAN_USBBUS10:
    case PCAN_USBBUS11:
    case PCAN_USBBUS12:
    case PCAN_USBBUS13:
    case PCAN_USBBUS14:
    case PCAN_USBBUS15:
    case PCAN_USBBUS16:
        strcpy_s(buffer, MAX_PATH, "PCAN_USB");
        break;

    case PCAN_LANBUS1:
    case PCAN_LANBUS2:
    case PCAN_LANBUS3:
    case PCAN_LANBUS4:
    case PCAN_LANBUS5:
    case PCAN_LANBUS6:
    case PCAN_LANBUS7:
    case PCAN_LANBUS8:
    case PCAN_LANBUS9:
    case PCAN_LANBUS10:
    case PCAN_LANBUS11:
    case PCAN_LANBUS12:
    case PCAN_LANBUS13:
    case PCAN_LANBUS14:
    case PCAN_LANBUS15:
    case PCAN_LANBUS16:
        strcpy_s(buffer, MAX_PATH, "PCAN_LAN");
        break;

    default:
        strcpy_s(buffer, MAX_PATH, "UNKNOWN");
        break;
    }
}

void TimerWrite::GetFormattedError(TPCANStatus error, LPSTR buffer)
{
    // Gets the text using the GetErrorText API function. If the function success, the translated error is returned.
    // If it fails, a text describing the current error is returned.
    if (CAN_GetErrorText(error, 0x09, buffer) != PCAN_ERROR_OK)
        sprintf_s(buffer, MAX_PATH, "An error occurred. Error-code's text (%Xh) couldn't be retrieved", error);
}

void TimerWrite::ConvertBitrateToString(TPCANBaudrate bitrate, LPSTR buffer)
{
    switch (bitrate)
    {
    case PCAN_BAUD_1M:
        strcpy_s(buffer, MAX_PATH, "1 MBit/sec");
        break;
    case PCAN_BAUD_800K:
        strcpy_s(buffer, MAX_PATH, "800 kBit/sec");
        break;
    case PCAN_BAUD_500K:
        strcpy_s(buffer, MAX_PATH, "500 kBit/sec");
        break;
    case PCAN_BAUD_250K:
        strcpy_s(buffer, MAX_PATH, "250 kBit/sec");
        break;
    case PCAN_BAUD_125K:
        strcpy_s(buffer, MAX_PATH, "125 kBit/sec");
        break;
    case PCAN_BAUD_100K:
        strcpy_s(buffer, MAX_PATH, "100 kBit/sec");
        break;
    case PCAN_BAUD_95K:
        strcpy_s(buffer, MAX_PATH, "95,238 kBit/sec");
        break;
    case PCAN_BAUD_83K:
        strcpy_s(buffer, MAX_PATH, "83,333 kBit/sec");
        break;
    case PCAN_BAUD_50K:
        strcpy_s(buffer, MAX_PATH, "50 kBit/sec");
        break;
    case PCAN_BAUD_47K:
        strcpy_s(buffer, MAX_PATH, "47,619 kBit/sec");
        break;
    case PCAN_BAUD_33K:
        strcpy_s(buffer, MAX_PATH, "33,333 kBit/sec");
        break;
    case PCAN_BAUD_20K:
        strcpy_s(buffer, MAX_PATH, "20 kBit/sec");
        break;
    case PCAN_BAUD_10K:
        strcpy_s(buffer, MAX_PATH, "10 kBit/sec");
        break;
    case PCAN_BAUD_5K:
        strcpy_s(buffer, MAX_PATH, "5 kBit/sec");
        break;
    default:
        strcpy_s(buffer, MAX_PATH, "Unknown Bitrate");
        break;
    }
}