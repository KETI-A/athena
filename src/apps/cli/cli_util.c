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
* @file cli_util.c
*
* This file contains a CLI design
*
* @note
*
* CLI Source File
*
*
******************************************************************************/

/***************************** Include ***************************************/
#include "cli.h"

/***************************** Definition ************************************/


/***************************** Static Variable *******************************/


/***************************** Function  *************************************/

/* Lookup table for reversing 4 bits. */
static uint8_t s_unReverse4bitTable[] = {
    0x0, 0x8, 0x4, 0xC,
    0x2, 0xA, 0x6, 0xE,
    0x1, 0x9, 0x5, 0xD,
    0x3, 0xB, 0x7, 0xF
};

unsigned short g_usaCRC16Table[256] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

static uint8_t P_CLI_UTIL_ReverseByte(uint8_t unVal)
{
    return (s_unReverse4bitTable[unVal & 0xF] << 4) | s_unReverse4bitTable[unVal >> 4];
}

static uint32_t P_CLI_UTIL_ReverseUint32(uint32_t unVal)
{
    return (P_CLI_UTIL_ReverseByte(unVal) << 24) | (P_CLI_UTIL_ReverseByte(unVal >> 8) << 16) | (P_CLI_UTIL_ReverseByte(unVal >> 16) << 8) | P_CLI_UTIL_ReverseByte(unVal >> 24);
}

uint16_t CLI_UTIL_GetCrc16 (const uint8_t* pucaData, uint16_t usLenData)
{
	register uint8_t ucCrcTableIdx;
	register uint16_t usCRC = 0x0000;

	while (usLenData--)
	{
		ucCrcTableIdx = (usCRC >> 8) ^ *pucaData++;
		usCRC = (usCRC << 8) ^ g_usaCRC16Table[ucCrcTableIdx];
	}

	return usCRC;
}

uint32_t CLI_UTIL_GetCrc32(const uint8_t* pBuf, size_t unSize)
{
    uint32_t unCrc = 0xFFFFFFFF;

    for (size_t i = 0; i < unSize; ++i)
    {
        unCrc = unCrc ^ ((uint32_t)P_CLI_UTIL_ReverseByte(pBuf[i]) << 24);

        for (int j = 0; j < 8; ++j)
        {
            if (unCrc & 0x80000000)
            {
                unCrc = (unCrc << 1) ^ 0x04C11DB7;
            }
            else
            {
                unCrc <<= 1;
            }
        }
    }

    return P_CLI_UTIL_ReverseUint32(~unCrc);
}

void CLI_UTIL_Enqueue(CLI_UTIL_QUEUE_T *qb, CLI_UTIL_QUEUE_T *item)
{
    qb->q_prev->q_next = item;
    item->q_next = qb;
    item->q_prev = qb->q_prev;
    qb->q_prev = item;
}

void CLI_UTIL_Dequeue(CLI_UTIL_QUEUE_T *item)
{
    item->q_prev->q_next = item->q_next;
    item->q_next->q_prev = item->q_prev;
}

CLI_UTIL_QUEUE_T *CLI_UTIL_DequeueNext(CLI_UTIL_QUEUE_T *qb)
{
    if (qb->q_next == qb)
    {
        return NULL;
    }

    qb = qb->q_next;

    qb->q_prev->q_next = qb->q_next;
    qb->q_next->q_prev = qb->q_prev;

    return qb;
}

int CLI_UTIL_QueueMap(CLI_UTIL_QUEUE_T *qb, int (*func)(CLI_UTIL_QUEUE_T *, unsigned int, unsigned int), unsigned int a, unsigned int b)
{
    CLI_UTIL_QUEUE_T *qe;
    CLI_UTIL_QUEUE_T *nextq;
    int res;

    qe = qb;

    qe = qb->q_next;

    while (qe != qb)
    {
        nextq = qe->q_next;
        if ((res = (*func)(qe, a, b)))
        {
            return res;
        }
        qe = nextq;
    }

    return 0;
}

int CLI_UTIL_CountQueue(CLI_UTIL_QUEUE_T *qb)
{
    CLI_UTIL_QUEUE_T *qe;
    int res = 0;

    qe = qb;

    while (qe->q_next != qb)
    {
        qe = qe->q_next;
        res++;
    }

    return res;
}

int CLI_UTIL_FindQueue(CLI_UTIL_QUEUE_T *qb, CLI_UTIL_QUEUE_T *item)
{
    CLI_UTIL_QUEUE_T *q;
    int res = 1;

    q = qb->q_next;

    while (q != item)
    {
        if (q == qb)
        {
            return 0;
        }

        q = q->q_next;
        res++;
    }

    return res;
}

