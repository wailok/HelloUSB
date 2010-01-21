/******************************************************************************

    USB Generic Function Driver (Local Header File)
    
This file contains declarations local to the generic USB device function.

 * Filename:        usb_generic_local.h
 * Dependancies:    See included files, below.
 * Processor:       TBD
 * Hardware:        TBD
 * Assembler:       TBD
 * Linker:          TBD
 * Company:         Microchip Technology, Inc.
 
Software License Agreement

The software supplied herewith by Microchip Technology Incorporated
(the “Company”) for its PICmicro® Microcontroller is intended and
supplied to you, the Company’s customer, for use solely and
exclusively on Microchip PICmicro Microcontroller products. The
software is owned by the Company and/or its supplier, and is
protected under applicable copyright laws. All rights are reserved.
Any use in violation of the foregoing restrictions may subject the
user to criminal sanctions under applicable laws, as well as to
civil liability for the breach of the terms and conditions of this
license.

THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

Author          Date    Comments
--------------------------------------------------------------------------------
BC           11-10-2006 Initial Creation

*******************************************************************************/
 
#ifndef _USBGEN_LOCAL_H_
#define _USBGEN_LOCAL_H_

#include <usb\usb.h>


/* Generic USB Function Data
 *************************************************************************
 * This structure maintains the data necessary to manage the generic USB
 * device function.
 */
 
typedef struct _generic_usb_fun_data
{
    BYTE    flags;      // Current state flags.
    BYTE    rx_size;    // Number of bytes received.
    BYTE    ep_num;     // Endpoint number.

} GEN_FUNC, *PGEN_FUNC;

// Generic USB Function State Flags:
#define GEN_FUNC_FLAG_TX_BUSY       0x01    // Tx is currently busy
#define GEN_FUNC_FLAG_RX_BUSY       0x02    // Rx is currently busy
#define GEN_FUNC_FLAG_RX_AVAIL      0x04    // Data has been received
#define GEN_FUNC_FLAG_INITIALIZED   0x80    // Function initialized


#endif  // _USBGEN_LOCAL_H_
/*************************************************************************
 * EOF usbgen_local.h
 */

