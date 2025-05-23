/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * @file pcbtrace.c
 * @brief Tools to trace CAN message within PCANBasic API. *
 * $Id: pcbtrace.c 20948 2025-01-10 12:50:38Z Fabrice $
 *
 * Copyright (C) 2001-2025  PEAK System-Technik GmbH <www.peak-system.com>
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
 * PCAN is a registered Trademark of PEAK-System Germany GmbH
 *
 * Contact:      <linux@peak-system.com>
 * Maintainer:   Fabrice Vergnaud <f.vergnaud@peak-system.com>
 */

#include "pcbtrace.h"
#include "pcaninfo.h"
#include "pcanlog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <time.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include "version.h"

#define PCBTRACE_DEFAULT_PATH	"."
#define PCBTRACE_MAX_MSG	600
#define EPOCH				(time_t)(-2209161600LL)
#define DAY_IN_MILLISEC		86400000.0L
#define FRAC_PRECISION_DAY	10000000000

#define GET_DAY_IN_MILLISEC(s,ns)	((((s - EPOCH) % 86400) * 1000 + (ns / 1000000)))
#define FRACTION_OF_DAY(s,ns)	(long)( (((long double)GET_DAY_IN_MILLISEC(s, ns)) / DAY_IN_MILLISEC) * FRAC_PRECISION_DAY)

extern int pcanbasic_get_fd_len(__u8 dlc);

static int pcbtrace_write_header(struct pcbtrace_ctx *ctx, enum pcbtrace_version version);
static void pcbtrace_size_check(struct pcbtrace_ctx *ctx);
static void pcbtrace_open_init(struct pcbtrace_ctx *ctx, enum pcaninfo_hw hw, unsigned int ch_idx);
static int pcbtrace_open_next(struct pcbtrace_ctx *ctx);
static const char* pcbtrace_get_type(TPCANMsgFD *msg);
static const char* pcbtrace_get_canid(TPCANMsgFD *msg, char* buf, size_t size, int *noid);

/* PRIVATE FUNCTIONS */

int pcbtrace_write_header(struct pcbtrace_ctx *ctx, enum pcbtrace_version version) {
	char buf[PCBTRACE_MAX_MSG];
	char tmp[PCANINFO_MAX_CHAR_SIZE];
	size_t n, ntmp;
	struct tm *t;

	if (ctx == NULL || !ctx->status || !ctx->pfile)
		return -EINVAL;

	/* file version */
	switch (version) {
	case V1_1:
		snprintf(buf, PCBTRACE_MAX_MSG, ";$FILEVERSION=1.1\n");
		break;
	case V2_0:
	default:
		snprintf(buf, PCBTRACE_MAX_MSG, ";$FILEVERSION=2.0\n");
		break;
	}
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	/* starttime */
	//	Integral part = Number of days that have passed since 30. December 1899.
	//	Fractional Part =  Fraction of a 24-hour day that has elapsed, resolution is 1 millisecond
	t = localtime(&ctx->time_start.tv_sec);
	snprintf(buf, PCBTRACE_MAX_MSG, ";$STARTTIME=%" PRId64 ".%ld\n", 
		(int64_t)((ctx->time_start.tv_sec + t->tm_gmtoff - EPOCH) / 86400), 
		FRACTION_OF_DAY(ctx->time_start.tv_sec + t->tm_gmtoff, ctx->time_start.tv_nsec));
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	/* columns */
	switch (version) {
	case V1_1:
		/* not supported */
		break;
	case V2_0:
	default:
		// see flags info in https://www.peak-system.com/produktcd/Pdf/English/PEAK_CAN_TRC_File_Format.pdf
		if (ctx->flags & TRACE_FILE_DATA_LENGTH)
			snprintf(buf, PCBTRACE_MAX_MSG, ";$COLUMNS=N,O,T,I,d,l,D\n");
		else
			snprintf(buf, PCBTRACE_MAX_MSG, ";$COLUMNS=N,O,T,I,d,L,D\n");
		n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
		if (n <= 0)
			return -errno;
		break;
	}
	/* separator */
	snprintf(buf, PCBTRACE_MAX_MSG, ";\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;

	/* file path */
	snprintf(buf, PCBTRACE_MAX_MSG, ";   %s\n", ctx->filename);
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	/* start time */
	snprintf(buf, PCBTRACE_MAX_MSG, ";   Start time: %.2d/%.2d/%.4d %.2d:%.2d:%.2d.000.0\n",
		t->tm_mday, t->tm_mon + 1, (1900 + t->tm_year),t->tm_hour,t->tm_min,t->tm_sec);
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	/* generated by */
	snprintf(buf, PCBTRACE_MAX_MSG, ";   Generated by PCAN-Basic API (Linux), version %d.%d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_BUILD);
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";-------------------------------------------------------------------------------\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;

	/* connection information */
	if (ctx->pinfo != NULL) {
		snprintf(buf, PCBTRACE_MAX_MSG, ";   Connection                                Bit rate\n");
		n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
		if (n <= 0)
			return -errno;
		/* channel info */
		snprintf(buf, PCBTRACE_MAX_MSG, ";   %s (%s)           ", ctx->chname, ctx->pinfo->path);
		ntmp = strlen(buf);
		n = fwrite(buf, ntmp, sizeof(char), ctx->pfile);
		if (n <= 0)
			return -errno;
		/* init string */
		snprintf(buf, PCBTRACE_MAX_MSG, "%s\n", 
			pcaninfo_bitrate_to_init_string(ctx->pinfo, tmp, sizeof(tmp)));
		n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
		if (n <= 0)
			return -errno;
		/* human-readable bitrate */
		memset(buf, ' ', ntmp);
		buf[0] = ';';
		buf[ntmp] = 0;
		n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
		if (n <= 0)
			return -errno;
		snprintf(buf, PCBTRACE_MAX_MSG, "%s\n", 
			pcaninfo_bitrate_to_string(ctx->pinfo, tmp, sizeof(tmp)));
		n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
		if (n <= 0)
			return -errno;
		snprintf(buf, PCBTRACE_MAX_MSG, ";-------------------------------------------------------------------------------\n");
		n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
		if (n <= 0)
			return -errno;
	}

	/* connection information */
	snprintf(buf, PCBTRACE_MAX_MSG, "; Glossary:\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";   Direction of Message:\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";     Rx: The frame was received\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";     Tx: The frame was transmitted\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";   Type of message:\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";     DT: CAN or J1939 data frame\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";     FD: CAN FD data frame\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";     FB: CAN FD data frame with BRS bit set (Bit Rate Switch)\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";     FE: CAN FD data frame with ESI bit set (Error State Indicator)\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";     BI: CAN FD data frame with both BRS and ESI bits set\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";     RR: Remote Request Frame\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";     ST: Hardware Status change\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";     ER: Error Frame\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";     EV: Event. User-defined text, begins directly after 2-digit type indicator\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";-------------------------------------------------------------------------------\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";   Message   Time    Type ID     Rx/Tx\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";   Number    Offset  |    [hex]  |  Data Length");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	if (ctx->flags & TRACE_FILE_DATA_LENGTH)
		snprintf(buf, PCBTRACE_MAX_MSG, "\n");
	else
		snprintf(buf, PCBTRACE_MAX_MSG, " Code\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";   |         [ms]    |    |      |  |  Data [hex]\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";   |         |       |    |      |  |  | \n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	snprintf(buf, PCBTRACE_MAX_MSG, ";---+-- ------+------ +- --+----- +- +- +- -- -- -- -- -- -- --\n");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;

	return 0;
}

void pcbtrace_size_check(struct pcbtrace_ctx *ctx) {
	struct stat st;

	stat(ctx->filename, &st);
	if (st.st_size > ctx->maxsize * 1000000) {
		pcbtrace_close(ctx);
		if (ctx->flags & TRACE_FILE_SEGMENTED)
			pcbtrace_open_next(ctx);
		else
			ctx->status = PCAN_PARAMETER_OFF;
	}
}

void pcbtrace_open_init(struct pcbtrace_ctx *ctx, enum pcaninfo_hw hw, unsigned int ch_idx) {
	char filename[100];
	char strtmp[9];
	char *str;
	size_t len;
	time_t traw;
	struct tm *t;

	/* check trace is already closed */
	pcbtrace_close(ctx);
	/* build filename based on time and channel */
	time(&traw);
	t = localtime(&traw);
	filename[0] = '\0';
	str = "";
	if (ctx->flags & TRACE_FILE_DATE) {
		len = snprintf(strtmp, sizeof(strtmp), "%04u%02u%02u", 1900 + t->tm_year, t->tm_mon + 1, t->tm_mday);
		strncat(filename, strtmp, len);
		str = "_";
	}
	if (ctx->flags & TRACE_FILE_TIME) {
		len = snprintf(strtmp, sizeof(strtmp), "%02u%02u%02u", t->tm_hour, t->tm_min, t->tm_sec);
		strncat(filename, strtmp, len);
		str = "_";
	}
	snprintf(ctx->chname, sizeof(ctx->chname), "%s%d", pcaninfo_hw_to_string(hw, 0), ch_idx);
	snprintf(ctx->filename_chunk, sizeof(ctx->filename_chunk), "%s%s%s", filename, str, ctx->chname);
	/* remove trailing '/' in location */
	if (ctx->directory != NULL) {
		len = strlen(ctx->directory);
		if (len > 2 && ctx->directory[len - 1] == 0 &&
				ctx->directory[len - 2] == '/')
			ctx->directory[len - 2] = 0;
	}
}

int pcbtrace_open_next(struct pcbtrace_ctx *ctx) {
	ctx->idx++;
	if (ctx->flags & TRACE_FILE_SEGMENTED) {
		snprintf(ctx->filename, sizeof(ctx->filename), "%s/%s_%d.trc", ctx->directory, ctx->filename_chunk, ctx->idx);
	}
	else {
		snprintf(ctx->filename, sizeof(ctx->filename), "%s/%s.trc", ctx->directory, ctx->filename_chunk);
	}
	
	if ((ctx->flags & TRACE_FILE_OVERWRITE) || (access(ctx->filename, F_OK) != 0)) {
		/* open file and update context */
		ctx->pfile = fopen(ctx->filename, "w");
		if (ctx->pfile)
			return pcbtrace_write_header(ctx, V2_0);
		return -errno;
	}
	else {
		return -EOPNOTSUPP;
	}
}

const char* pcbtrace_get_type(TPCANMsgFD *msg) {
	char* result;
	__u8 is_frame = 0;

	/* check echo frame first (combination of ERRFRAME and STATUS) */
	if ((msg->MSGTYPE & PCAN_MESSAGE_ECHO) == PCAN_MESSAGE_ECHO) {
		is_frame = 1;
	}
	/* check error frame */
	else if ((msg->MSGTYPE & PCAN_MESSAGE_ERRFRAME) == PCAN_MESSAGE_ERRFRAME) {
		result = "ER";
	}	
	/* check status frame */
	else if ((msg->MSGTYPE & PCAN_MESSAGE_STATUS) == PCAN_MESSAGE_STATUS) {
		result = "ST";
	}
	else {	/* msg is valid */
		is_frame = 1;
	}

	if (is_frame) {
			/* check CAN FD */
		if ((msg->MSGTYPE & PCAN_MESSAGE_FD) == PCAN_MESSAGE_FD) {
			/* check BRS & ESI */
			if ((msg->MSGTYPE & PCAN_MESSAGE_BRS) == PCAN_MESSAGE_BRS &&
				(msg->MSGTYPE & PCAN_MESSAGE_ESI) == PCAN_MESSAGE_ESI) {
				result = "BI";
			}
			/* check BRS */
			else if ((msg->MSGTYPE & PCAN_MESSAGE_BRS) == PCAN_MESSAGE_BRS) {
				result = "FB";
			}
			/* check ESI */
			else if ((msg->MSGTYPE & PCAN_MESSAGE_ESI) == PCAN_MESSAGE_ESI) {
				result = "FE";
			}
			else {
				/* classic CAN FD frame */
				result = "FD";
			}
		}
		else { /* CAN frame */
			   /* check RTR */
			if ((msg->MSGTYPE & PCAN_MESSAGE_RTR) == PCAN_MESSAGE_RTR) {
				result = "RR";
			}
			else {
				/* classic CAN frame */
				result = "DT";
			}
		}
	}
	return result;
}

const char* pcbtrace_get_canid(TPCANMsgFD *msg, char* buf, size_t size, int *has_no_canid) {
	char *ptr = buf;
	
	*has_no_canid = ((msg->MSGTYPE & PCAN_MESSAGE_ECHO) != PCAN_MESSAGE_ECHO);
	*has_no_canid &= (
		(msg->MSGTYPE & PCAN_MESSAGE_ERRFRAME) == PCAN_MESSAGE_ERRFRAME ||
		(msg->MSGTYPE & PCAN_MESSAGE_STATUS) == PCAN_MESSAGE_STATUS
		);

	if (*has_no_canid) {
		/* no CAN ID displayed */
		snprintf(buf, size, "        ");
	}
	else {
		/* format CAN ID as hex */
		if ((msg->MSGTYPE & PCAN_MESSAGE_EXTENDED) == PCAN_MESSAGE_EXTENDED)
			snprintf(buf, size, "%08x", msg->ID);
		else
			snprintf(buf, size, "    %04x", msg->ID);
		/* format to upper chars */
		while (*ptr) {
			*ptr = toupper(*ptr);
			ptr++;
		}
	}
	return buf;
}

/* PUBLIC FUNCTIONS */
void pcbtrace_set_defaults(struct pcbtrace_ctx *ctx) {
	if (ctx == NULL)
		return;
	pcbtrace_set_path(ctx, PCBTRACE_DEFAULT_PATH);
	ctx->flags = TRACE_FILE_SINGLE;
	ctx->maxsize = 10;
	ctx->status = PCAN_PARAMETER_OFF;
	ctx->pinfo = NULL;
	ctx->time_start.tv_sec = 0;
	ctx->time_start.tv_nsec = 0;
}

uint8_t pcbtrace_set_path(struct pcbtrace_ctx *ctx, const char* path) {
	uint8_t res = -EINVAL;
	if (ctx == NULL)
		return res;
	if (ctx->directory != NULL) {
		free(ctx->directory);
	}
	if (path == NULL || path[0] == 0)
		path = PCBTRACE_DEFAULT_PATH;
	ctx->directory = realpath(path, NULL);
	if (ctx->directory != NULL) {
		/* Check if directory exists */
		DIR *directory = opendir(ctx->directory);
		if (directory) {
			closedir(directory);
			res = 0;
		}
	}
	if (res != 0) {
		/* path may be invalid, revert to current directory */
		ctx->directory = strdup(PCBTRACE_DEFAULT_PATH);
	}
	return res;
}

void pcbtrace_release(struct pcbtrace_ctx *ctx) {
	pcbtrace_close(ctx);
	if (ctx != NULL && ctx->directory != NULL) {
		free(ctx->directory);
		ctx->directory = NULL;
	}
}


int pcbtrace_open(struct pcbtrace_ctx *ctx, enum pcaninfo_hw hw, unsigned int ch_idx) {
	if (ctx == NULL)
		return -EINVAL;
	pcbtrace_open_init(ctx, hw, ch_idx);
	ctx->idx = 0;
	ctx->msg_cnt = 0;
	return pcbtrace_open_next(ctx);
}

int pcbtrace_close(struct pcbtrace_ctx *ctx) {
	if (ctx == NULL)
		return -EINVAL;
	if (ctx->pfile != NULL) {
		fclose(ctx->pfile);
		ctx->pfile = NULL;
	}
	return 0;
}

int pcbtrace_write_msg(struct pcbtrace_ctx *ctx, TPCANMsgFD *msg, int data_len, struct timeval *tv, int rx) {
	char buf[PCBTRACE_MAX_MSG];
	int i, n, len;
	int has_no_canid, buf_init;
#if DEPRECATED_VERSION_1_1
	char * str = "";
#endif
	if (ctx == NULL)
		return -EINVAL;
	if (!ctx->status)
		return 0;
	if (ctx->pfile == NULL)
		return -EBADF;
		
	ctx->msg_cnt++;
	len = 0;

	/* write: "Frame_Nb   Time_msec.micros   Direction   " */
	int64_t ts = (tv->tv_sec - ctx->time_start.tv_sec) * 1000 + (tv->tv_usec / 1000);
	if (ts < 0) {
		pcanlog_log(LVL_INFO, "Tracing a message that has a timestamp before channel's initialisation: TS=%ld < Start=%ld.\n", tv->tv_sec, ctx->time_start.tv_sec);
	}
	snprintf(buf, PCBTRACE_MAX_MSG, "%7lu   %7" PRId64 ".%03lu ",
		ctx->msg_cnt, ts, (tv->tv_usec % 1000));
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	len += n;
	/* write: "Type " */
	snprintf(buf, PCBTRACE_MAX_MSG, "%s ", pcbtrace_get_type(msg));
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	len += n;
	/* write: "ID" */
	pcbtrace_get_canid(msg, buf, PCBTRACE_MAX_CHAR_SIZE, &has_no_canid);
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	/* write: " Rx/Tx " */
	snprintf(buf, PCBTRACE_MAX_MSG, " %s ", rx ? "Rx" : "Tx");
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	/* write: "DLC " */
	if (has_no_canid)
		snprintf(buf, PCBTRACE_MAX_MSG, "   ");
	else {
		if (ctx->flags & TRACE_FILE_DATA_LENGTH)
			snprintf(buf, PCBTRACE_MAX_MSG, "%-2d ", pcanbasic_get_fd_len(msg->DLC));
		else
			snprintf(buf, PCBTRACE_MAX_MSG, "%-2d ", msg->DLC);
	}
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	len += n;
	/* prepare specific data buffer & extra information: depends on msg's type */
	buf_init = 0;
	if ((msg->MSGTYPE & PCAN_MESSAGE_ECHO) == PCAN_MESSAGE_ECHO) {
		/* same as a CAN/CANFD frame */
		/*	checked first to avoid falling in PCAN_MESSAGE_STATUS */
	}
	else if ((msg->MSGTYPE & PCAN_MESSAGE_STATUS) == PCAN_MESSAGE_STATUS) {
		/* force data_len */
		data_len = 4;
		char bufData[10];
		buf[0] = 0;
		/* force data: data[0..3]=msg[0..3] */
		snprintf(bufData, 10, "%.2X ", (uint8_t)msg->ID);		
		for (i = 0; i < data_len; i++)
		{
			snprintf(bufData, 10, "%.2X ", (i < msg->DLC) ?  msg->DATA[i] : 0);
			strncat(buf, bufData, PCBTRACE_MAX_MSG - strlen(buf));
		}
		buf_init = 1;
#if DEPRECATED_VERSION_1_1
		/* write: state info */
		if(msg->DATA[3] & PCAN_ERROR_BUSLIGHT)
			str = "BUSLIGHT";
		if(msg->DATA[3] & PCAN_ERROR_BUSHEAVY)
			str = "BUSHEAVY";
		if(msg->DATA[3] & PCAN_ERROR_BUSOFF)
			str = "BUSOFF";
#endif
	}
	else if ((msg->MSGTYPE & PCAN_MESSAGE_ERRFRAME) == PCAN_MESSAGE_ERRFRAME) {
		/* force data_len */
		data_len = 5;
		char bufData[10];
		buf[0] = 0;
		/* force data: data[0]=ID */
		snprintf(bufData, 10, "%.2X ", (uint8_t)msg->ID);
		strncat(buf, bufData, PCBTRACE_MAX_MSG - strlen(buf));
		/* force data: data[1..4]=msg[0..3] */
		for (i = 0; i < data_len - 1; i++)
		{
			snprintf(bufData, 10, "%.2X ", (i < msg->DLC) ?  msg->DATA[i] : 0);
			strncat(buf, bufData, PCBTRACE_MAX_MSG - strlen(buf));
		}
		buf_init = 1;
#if DEPRECATED_VERSION_1_1
		/* write: error info */
		switch (msg->DATA[0]) {
		case 1:
			str = "Bit Error";
			break;
		case 2:
			str = "Form Error";
			break;
		case 4:
			str = "Stuff Error";
			break;
		case 8:
			str = "Other Error";
			break;
		default:
			str = "Unknown Error";
			break;
		}		
#endif
	}
	else if ((msg->MSGTYPE & PCAN_MESSAGE_RTR) == PCAN_MESSAGE_RTR) {
		/* force data_len */
		data_len = 0;
		buf[0] = 0;
		buf_init = 1;
#if DEPRECATED_VERSION_1_1
		snprintf(buf, PCBTRACE_MAX_MSG, "RTR");
#endif
	}
	/* prepare generic data buffer */
	if (!buf_init) {
		char bufData[10];
		if (data_len > 0) {
			buf[0] = 0;
			for (i = 0; i < data_len; i++)
			{
				snprintf(bufData, 10, "%.2X ", msg->DATA[i]);
				strncat(buf, bufData, PCBTRACE_MAX_MSG - strlen(buf));
			}
		}
		else {
			buf[0] = ' ';
			buf[1] = 0;
		}
	}
	/* remove trailing space */
	n = strlen(buf);
	if (n > 0 && buf[n - 1] == ' ') {		
		--n;
		buf[n] = 0;		
	}
	/* write: DATA */
	n = fwrite(buf, n, sizeof(char), ctx->pfile);
	if (n < 0)
		return -errno;
	len += n;
#if DEPRECATED_VERSION_1_1
	/* write extra information */
	if (str[0] != 0) {
		n = fwrite(str, strlen(str), sizeof(char), ctx->pfile);
		if (n <= 0)
			return -errno;
		len += n;
	}
#endif
	/* write EOL */
	buf[0] = '\n';
	buf[1] = '\0';
	n = fwrite(buf, strlen(buf), sizeof(char), ctx->pfile);
	if (n <= 0)
		return -errno;
	len += n;
	pcbtrace_size_check(ctx);
	return len;
}

int pcbtrace_write(struct pcbtrace_ctx *ctx, const char * buffer, unsigned int size) {
	int n;

	if (ctx == NULL)
		return -EINVAL;
	if (!ctx->status)
		return 0;
	if (ctx->pfile == NULL)
		return -EBADF;

	n = fwrite(buffer, size, 1, ctx->pfile);
	if (n <= 0)
		return -errno;
	pcbtrace_size_check(ctx);
	return n;
}



