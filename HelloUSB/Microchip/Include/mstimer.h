/*********************************************************************
 *
 *                  MS Timer
 *
 *********************************************************************
 * FileName:        mstimer.c
 * Processor:       PIC24 /  Daytona
 * Complier:        MPLAB C30/C32
 *
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement:
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the �Company�) for its dsPIC30F and PICmicro� Microcontroller is intended 
 * and supplied to you, the Company�s customer, for use solely and
 * exclusively on Microchip's dsPIC30F and PICmicro Microcontroller products. 
 * The software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN �AS IS� CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 * Author               Date      Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * 
 *
 ********************************************************************/

#ifndef _MS_TIMER_HEADER_FILE
#define _MS_TIMER_HEADER_FILE

#include "GenericTypeDefs.h"

void MSTimerInit(unsigned int pbclock);
void MSTimerClear(void);
UINT MSTimerGetTime(void);
void MSTimerWait(UINT ms_wait);

void MSTimer32Clear(void);
UINT32 MSTimer32GetTime(void);

#endif
