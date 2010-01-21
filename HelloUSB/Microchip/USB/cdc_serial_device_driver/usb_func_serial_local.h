/******************************************************************************

    USB CDC Serial Function Driver (Local Header File)
    
This file contains declarations local to the USB CDC-Serial device function.

 * Filename:        usb_func_serial_local.h
 * Dependancies:    TBD
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
 
#ifndef _USBSERIAL_LOCAL_H_
#define _USBSERIAL_LOCAL_H_

#include <usb\usb.h>


/* CDC Serial Emulation Function Data
 *************************************************************************
 * This structure contains the data necessary to manage the USB CDC Serial 
 * emulation function driver.
 */

typedef struct _usb_cdcserial_data
{
    unsigned long flags;        // Current state flags.

    void         *rx_buff;      // Rx data pointer

    BYTE          rx_size;      // Number of bytes received.

    LINE_CODING   line_coding;  // Buffer to store line coding information

    void         *tx_buff;      // Tx Buffer pointer.

    BYTE          tx_size;      // Tx Data size.

} CDCSER_DAT, *PCDCSER_DAT;

// Flags Definitions.
#define CDC_FLAGS_INITIALIZED       0x80000000
#define CDC_FLAGS_TX_BUSY           0x40000000
#define CDC_FLAGS_RX_BUSY           0x20000000
#define CDC_FLAGS_NOTIFY_BUSY       0x10000000
#define CDC_FLAGS_LINE_CTRL_BUSY    0x08000000
#define CDC_FLAGS_GET_CMD_BUSY      0x04000000
#define CDC_FLAGS_SEND_RESP_BUSY    0x02000000
#define CDC_FLAGS_WAITING_STATUS    0x01000000
#define CDC_FLAGS_SENT_DATA         0x00800000
#define CDC_FLAGS_INIT_MASK         0x000FFFFF



#endif  // _USBSERIAL_LOCAL_H_
/*************************************************************************
 * EOF usbserial_local.h
 */

