
/******************************************************************************

    USB Device Abstraction Layer (Local Header File)
    
This file defines macros, prototypes, and constants used by the USB device 
abstraction.

 * Filename:        usb_device_local.h
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
BC           11-8-2006  Initial Creation

*******************************************************************************/
 
#ifndef _USB_DEVICE_LOCAL_H_
#define _USB_DEVICE_LOCAL_H_

#include "usb/usb.h"


/* EP0_STATE
 *************************************************************************
 * This enumeration identifies the current state of endpoint 0.
 */
 
typedef enum
{
    EP0_UNINITIALIZED = 0,  // Device layer not yet initialized.
    EP0_WAITING_SETUP,      // Rx started, waiting for a setup packet.
    EP0_WAITING_IN_XFER,    // Tx started, waiting to finish data transfer.
    EP0_WAITING_OUT_XFER,   // Rx started, waiting to finish data transfer.
    EP0_WAITING_SET_ADDR,   // Have received a new address, waiting ACK.
    EP0_SENDING_DESC,       // Currently sending descriptor data to host.
    EP0_WAITING_RX_STATUS,  // Have sent data, waiting status from host.
    EP0_WAITING_TX_STATUS,  // Have received data, sending status to host.
    EP0_WAITING_FUNC,       // Waiting for a function to handle a request.
    EP0_STALLED             // Unidentified request received.

} EP0_STATE;


/* USB_DEVICE_DATA
 *************************************************************************
 * This data structure contains all the state data necessary for the USB
 * device layer.
 */

#ifndef USB_DEV_EP0_MAX_PACKET_SIZE
    #define USB_DEV_EP0_MAX_PACKET_SIZE 8
#endif
 
typedef struct _USB_DEVICE_DATA
{
    // Receive buffer for endpoint zero
    BYTE        ep0_buffer[USB_DEV_EP0_MAX_PACKET_SIZE];

    // Function driver map.
    UINT32      function_map;

    // Current state of EP0
    EP0_STATE   ep0_state;

    // Device-layer flags (see below)
    UINT16      flags;

    // Initialization flags
    unsigned long init_flags;

    // Function driver table.
    BYTE        func_drv[USB_DEV_HIGHEST_EP_NUMBER];

    // Current device configuration.
    BYTE        dev_config;
    
    #ifdef USB_DEV_SUPPORTS_ALT_INTERFACES
    
    // Interface number per endpoint
    BYTE        interface[USB_DEV_HIGHEST_EP_NUMBER];

    // Alternate interface setting per endpoint
    BYTE        alt_intf[USB_DEV_HIGHEST_EP_NUMBER];

    #endif
    
} USB_DEVICE_DATA, *PUSB_DEVICE_DATA;


/* Device-layer flags bits
 *************************************************************************
 * These bits are used in the device layer to track the current status of
 * the device and certain configuration options.
 */

#define USB_DEVICE_FLAGS_SELF_PWR       0x0100
#define USB_DEVICE_FLAGS_REMOTE_WAKE    0x0200
#define USB_DEVICE_FLAGS_SUSPENDED      0x0400
#define USB_DEVICE_FLAGS_ATTACHED       0x0800
#define USB_DEVICE_FLAGS_ADDR_MASK      0x00FF


/*
 * Endpoint configuration logic assumes that the following device-layer
 * endpoint config flags are equal to the HAL config flags.  If not,
 * the device layer will need to translate them.
 */
#if ( (USB_EP_TRANSMIT  != USB_HAL_TRANSMIT ) || \
      (USB_EP_RECEIVE   != USB_HAL_RECEIVE  ) || \
      (USB_EP_HANDSHAKE != USB_HAL_HANDSHAKE) )
#error "Error!  Device layer needs to translate endpoint configuration flags."
#endif

// EP0 Initialization flags
#define EP0_FLAGS (USB_HAL_TRANSMIT|USB_HAL_RECEIVE|USB_HAL_HANDSHAKE)


// Definition of min
#ifndef min
#define min(a,b) ( ( (a) < (b) ) ? (a) : (b) )
#endif


#endif  // _USB_DEVICE_LOCAL_H_
/*************************************************************************
 * EOF
 */

