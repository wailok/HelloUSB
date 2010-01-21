/******************************************************************************

    USB HAL Local (Header File)
 
This file contains local definitions used by the HAL.

 * Filename:        usb_hal_local.h
 * Dependancies:    See included files, below
 * Processor:       PIC32MX
 * Hardware:        PIC32MX
 * Assembler:       pic32
 * Linker:          pic32
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
BC           04-18-2008 Merged core into HAL

*******************************************************************************/
 
#ifndef _USB_HAL_LOCAL_H_
#define _USB_HAL_LOCAL_H_

#include "usb/usb.h"

#if defined (__18CXX)
    #include "USB PIC18.h"
#elif defined (__C30__)
    #include "USB PIC24.h"
#elif defined (__PIC32MX__)
    #include "p32xxxx.h"
    #include "usb_pic32.h"
#else
    #error "Error!  Unsupported processor"
#endif


// Misc Definitions:

#ifndef USB_DEVICE_ATTACH_DEBOUNCE_TIME
    #define USB_DEVICE_ATTACH_DEBOUNCE_TIME 100     // 100 ms default time
#endif

#ifndef LOCAL_INLINE
    #define LOCAL_INLINE static inline
#endif

#define OUT         0                           //
#define IN          1                           //


/* Endpoint control value and mask bits */
#define EP_HSHK         0x01    // Enable handshake
#define EP_STALL        0x02    // Stall endpoint
#define EP_EP_TX_EN     0x04    // Enable transmitting from endpoint
#define EP_EP_RX_EN     0x08    // Enable receiving from endpoint
#define EP_EP_CTL_DIS   0x10    // Disable control transfers to/from endpoint
#define EP_RETRY_DIS    0x40    // (Host Only) No retry of NAK'd transactions
#define EP_HOST_WOHUB   0x80    // (Host Only) Allow connection to low-speed hub

// Status mask
#if defined(__18CXX)
   
    #define UIR_SOF_TOK (1<<6)
    #define UIR_TOK_DNE (1<<3)
    #define UIR_USB_RST (1<<0)
    #define UIR_UERR (1<<1)
    #define UIR_UIDLE (1<<4)
    #define UIR_STALL (1<<5)

    #define STATUS_MASK (UIR_USB_RST|UIR_UERR|UIR_SOF_TOK|UIR_TOK_DNE| \
                         UIR_UIDLE|UIR_STALL)

#elif defined(__C30__)

    // Status Bits
    #define UIR_USB_RST         0x0001
    #define UIR_UERR            0x0002
    #define UIR_SOF_TOK         0x0004
    #define UIR_TOK_DNE         0x0008
    #define UIR_UIDLE           0x0010
    #define UIR_RESUME          0x0020
    #define UIR_ATTACH          0x0040
    #define UIR_STALL           0x0080

    // Error Status Bits
    #define UEIR_PID_ERR         0x0001
    #define UEIR_CRC5            0x0002
    #define UEIR_HOST_EOF        0x0002
    #define UEIR_CRC16           0x0004
    #define UEIR_DFN8            0x0008
    #define UEIR_BTO_ERR         0x0010
    #define UEIR_DMA_ERR         0x0020
    #define UEIR_BTS_ERR         0x0080

/*    #define STATUS_MASK (UIR_USB_RST|UIR_UERR|UIR_SOF_TOK|UIR_TOK_DNE| \
                         UIR_UIDLE|UIR_RESUME|UIR_ATTACH|UIR_STALL) */
    #define STATUS_MASK (UIR_USB_RST|UIR_UERR|UIR_TOK_DNE|UIR_STALL)

#else

    // Status Bits
    #define UIR_USB_RST          0x00000001
    #define UIR_UERR             0x00000002
    #define UIR_SOF_TOK          0x00000004
    #define UIR_TOK_DNE          0x00000008
    #define UIR_UIDLE            0x00000010
    #define UIR_RESUME           0x00000020
    #define UIR_ATTACH           0x00000040
    #define UIR_STALL            0x00000080
    
    // Error Status Bits
    #define UEIR_PID_ERR         0x00000001
    #define UEIR_CRC5            0x00000002
    #define UEIR_HOST_EOF        0x00000002
    #define UEIR_CRC16           0x00000004
    #define UEIR_DFN8            0x00000008
    #define UEIR_BTO_ERR         0x00000010
    #define UEIR_DMA_ERR         0x00000020
    #define UEIR_BTS_ERR         0x00000080
    
    /*#define STATUS_MASK (UIR_USB_RST|UIR_UERR|UIR_SOF_TOK|UIR_TOK_DNE| \
                         UIR_UIDLE|UIR_RESUME|UIR_ATTACH|UIR_STALL) */
    #define STATUS_MASK (UIR_USB_RST|UIR_UERR|UIR_TOK_DNE|UIR_STALL)
                         
#endif

    

// Assumes HAL error flags are equal to USB OTG UEIR bits 
// (see USBHALGetLastError and ErrorHandler).
#if( (USBHAL_PID_ERR  != UEIR_PID_ERR ) || \
     (USBHAL_CRC5     != UEIR_CRC5    ) || \
     (USBHAL_HOST_EOF != UEIR_HOST_EOF) || \
     (USBHAL_CRC16    != UEIR_CRC16   ) || \
     (USBHAL_DFN8     != UEIR_DFN8    ) || \
     (USBHAL_BTO_ERR  != UEIR_BTO_ERR ) || \
     (USBHAL_DMA_ERR  != UEIR_DMA_ERR ) || \
     (USBHAL_BTS_ERR  != UEIR_BTS_ERR ) )
#error "USBHAL ErrorHandler must translate error flags"
#endif

// Assumes HAL flags shifted left 8 match core control flags (see USBHALSetEpConfiguration).
#if( ((USB_HAL_HANDSHAKE >> 8) != EP_HSHK)      ||  \
     ((USB_HAL_TRANSMIT  >> 8) != EP_EP_TX_EN)  ||  \
     ((USB_HAL_RECEIVE   >> 8) != EP_EP_RX_EN)  ||  \
     ((USB_HAL_NO_RETRY  >> 8) != EP_RETRY_DIS) ||  \
     ((USB_HAL_ALLOW_HUB >> 8) != EP_HOST_WOHUB) )
#error "USBHALSetEpConfiguration must translate control flags"
#endif

#define ERROR_MASK (UEIR_PID_ERR|UEIR_CRC5|UEIR_HOST_EOF|UEIR_CRC16| \
                    UEIR_DFN8|UEIR_BTO_ERR|UEIR_DMA_ERR|UEIR_BTS_ERR)

#define CTRL_MASK (EP_HSHK|EP_EP_TX_EN|EP_EP_RX_EN|EP_RETRY_DIS|EP_HOST_WOHUB)

#define RESISTOR_CTRL_MASK (UOTGCTRL_DM_LOW  | UOTGCTRL_DP_LOW | \
                            UOTGCTRL_DM_HIGH | UOTGCTRL_DP_HIGH)
                                
#define DATA_PTR_SIZE UINT32

// USBHALConfigureDescriptor flags
#define USBHAL_DESC_BSTALL 0x04  // Stall the endpoint.
#define USBHAL_DESC_DTS    0x08  // Require Data Toggle Synchronization
#define USBHAL_DESC_NINC   0x10  // No Incrementing of DMA address
#define USBHAL_DESC_KEEP   0x20  // HW keeps buffer & descriptor
#define USBHAL_DESC_DATA1  0x40  // Indicates data packet 1
#define USBHAL_DESC_DATA0  0x00  // Indicates data packet 0
#define USBHAL_DESC_UOWN   0x80  // USB HW owns buffer & descriptor


/********************************************************************
 * USB - PIC Endpoint Definitions
 * PIC Endpoint Address Format: X:EP3:EP2:EP1:EP0:DIR:PPBI:X
 * This is used when checking the value read from USTAT
 *
 * NOTE: These definitions are not used in the descriptors.
 * EP addresses used in the descriptors have different format.
 *******************************************************************/

#if (USB_PING_PONG_MODE == USB_PING_PONG__NO_PING_PONG)
    #define USB_NEXT_EP0_OUT_PING_PONG 0x0000   // Used in USB Device Mode only
    #define USB_NEXT_EP0_IN_PING_PONG 0x0000    // Used in USB Device Mode only
    #define USB_NEXT_PING_PONG 0x0000           // Used in USB Device Mode only
    #define EP0_OUT_EVEN    0                   // Used in USB Device Mode only
    #define EP0_OUT_ODD     0                   // Used in USB Device Mode only
    #define EP0_IN_EVEN     1                   // Used in USB Device Mode only
    #define EP0_IN_ODD      1                   // Used in USB Device Mode only
    #define EP1_OUT_EVEN    2                   // Used in USB Device Mode only
    #define EP1_OUT_ODD     2                   // Used in USB Device Mode only
    #define EP1_IN_EVEN     3                   // Used in USB Device Mode only
    #define EP1_IN_ODD      3                   // Used in USB Device Mode only
    #define EP2_OUT_EVEN    4                   // Used in USB Device Mode only
    #define EP2_OUT_ODD     4                   // Used in USB Device Mode only
    #define EP2_IN_EVEN     5                   // Used in USB Device Mode only
    #define EP2_IN_ODD      5                   // Used in USB Device Mode only
    #define EP3_OUT_EVEN    6                   // Used in USB Device Mode only
    #define EP3_OUT_ODD     6                   // Used in USB Device Mode only
    #define EP3_IN_EVEN     7                   // Used in USB Device Mode only
    #define EP3_IN_ODD      7                   // Used in USB Device Mode only
    #define EP4_OUT_EVEN    8                   // Used in USB Device Mode only
    #define EP4_OUT_ODD     8                   // Used in USB Device Mode only
    #define EP4_IN_EVEN     9                   // Used in USB Device Mode only
    #define EP4_IN_ODD      9                   // Used in USB Device Mode only
    #define EP5_OUT_EVEN    10                  // Used in USB Device Mode only
    #define EP5_OUT_ODD     10                  // Used in USB Device Mode only
    #define EP5_IN_EVEN     11                  // Used in USB Device Mode only
    #define EP5_IN_ODD      11                  // Used in USB Device Mode only
    #define EP6_OUT_EVEN    12                  // Used in USB Device Mode only
    #define EP6_OUT_ODD     12                  // Used in USB Device Mode only
    #define EP6_IN_EVEN     13                  // Used in USB Device Mode only
    #define EP6_IN_ODD      13                  // Used in USB Device Mode only
    #define EP7_OUT_EVEN    14                  // Used in USB Device Mode only
    #define EP7_OUT_ODD     14                  // Used in USB Device Mode only
    #define EP7_IN_EVEN     15                  // Used in USB Device Mode only
    #define EP7_IN_ODD      15                  // Used in USB Device Mode only
    #define EP8_OUT_EVEN    16                  // Used in USB Device Mode only
    #define EP8_OUT_ODD     16                  // Used in USB Device Mode only
    #define EP8_IN_EVEN     17                  // Used in USB Device Mode only
    #define EP8_IN_ODD      17                  // Used in USB Device Mode only
    #define EP9_OUT_EVEN    18                  // Used in USB Device Mode only
    #define EP9_OUT_ODD     18                  // Used in USB Device Mode only
    #define EP9_IN_EVEN     19                  // Used in USB Device Mode only
    #define EP9_IN_ODD      19                  // Used in USB Device Mode only
    #define EP10_OUT_EVEN   20                  // Used in USB Device Mode only
    #define EP10_OUT_ODD    20                  // Used in USB Device Mode only
    #define EP10_IN_EVEN    21                  // Used in USB Device Mode only
    #define EP10_IN_ODD     21                  // Used in USB Device Mode only
    #define EP11_OUT_EVEN   22                  // Used in USB Device Mode only
    #define EP11_OUT_ODD    22                  // Used in USB Device Mode only
    #define EP11_IN_EVEN    23                  // Used in USB Device Mode only
    #define EP11_IN_ODD     23                  // Used in USB Device Mode only
    #define EP12_OUT_EVEN   24                  // Used in USB Device Mode only
    #define EP12_OUT_ODD    24                  // Used in USB Device Mode only
    #define EP12_IN_EVEN    25                  // Used in USB Device Mode only
    #define EP12_IN_ODD     25                  // Used in USB Device Mode only
    #define EP13_OUT_EVEN   26                  // Used in USB Device Mode only
    #define EP13_OUT_ODD    26                  // Used in USB Device Mode only
    #define EP13_IN_EVEN    27                  // Used in USB Device Mode only
    #define EP13_IN_ODD     27                  // Used in USB Device Mode only
    #define EP14_OUT_EVEN   28                  // Used in USB Device Mode only
    #define EP14_OUT_ODD    28                  // Used in USB Device Mode only
    #define EP14_IN_EVEN    29                  // Used in USB Device Mode only
    #define EP14_IN_ODD     29                  // Used in USB Device Mode only
    #define EP15_OUT_EVEN   30                  // Used in USB Device Mode only
    #define EP15_OUT_ODD    30                  // Used in USB Device Mode only
    #define EP15_IN_EVEN    31                  // Used in USB Device Mode only
    #define EP15_IN_ODD     31                  // Used in USB Device Mode only

    #define EP(ep,dir,pp) (2*ep+dir)            // Used in USB Device Mode only

    #define BD(ep,dir,pp)   ((8 * ep) + (4 * dir))      // Used in USB Device Mode only

#elif (USB_PING_PONG_MODE == USB_PING_PONG__EP0_OUT_ONLY)
    #define USB_NEXT_EP0_OUT_PING_PONG 0x0004
    #define USB_NEXT_EP0_IN_PING_PONG 0x0000
    #define USB_NEXT_PING_PONG 0x0000
    #define EP0_OUT_EVEN    0
    #define EP0_OUT_ODD     1
    #define EP0_IN_EVEN     2
    #define EP0_IN_ODD      2
    #define EP1_OUT_EVEN    3
    #define EP1_OUT_ODD     3
    #define EP1_IN_EVEN     4
    #define EP1_IN_ODD      4
    #define EP2_OUT_EVEN    5
    #define EP2_OUT_ODD     5
    #define EP2_IN_EVEN     6
    #define EP2_IN_ODD      6
    #define EP3_OUT_EVEN    7
    #define EP3_OUT_ODD     7
    #define EP3_IN_EVEN     8
    #define EP3_IN_ODD      8
    #define EP4_OUT_EVEN    9
    #define EP4_OUT_ODD     9
    #define EP4_IN_EVEN     10
    #define EP4_IN_ODD      10
    #define EP5_OUT_EVEN    11
    #define EP5_OUT_ODD     11
    #define EP5_IN_EVEN     12
    #define EP5_IN_ODD      12
    #define EP6_OUT_EVEN    13
    #define EP6_OUT_ODD     13
    #define EP6_IN_EVEN     14
    #define EP6_IN_ODD      14
    #define EP7_OUT_EVEN    15
    #define EP7_OUT_ODD     15
    #define EP7_IN_EVEN     16
    #define EP7_IN_ODD      16
    #define EP8_OUT_EVEN    17
    #define EP8_OUT_ODD     17
    #define EP8_IN_EVEN     18
    #define EP8_IN_ODD      18
    #define EP9_OUT_EVEN    19
    #define EP9_OUT_ODD     19
    #define EP9_IN_EVEN     20
    #define EP9_IN_ODD      20
    #define EP10_OUT_EVEN   21
    #define EP10_OUT_ODD    21
    #define EP10_IN_EVEN    22
    #define EP10_IN_ODD     22
    #define EP11_OUT_EVEN   23
    #define EP11_OUT_ODD    23
    #define EP11_IN_EVEN    24
    #define EP11_IN_ODD     24
    #define EP12_OUT_EVEN   25
    #define EP12_OUT_ODD    25
    #define EP12_IN_EVEN    26
    #define EP12_IN_ODD     26
    #define EP13_OUT_EVEN   27
    #define EP13_OUT_ODD    27
    #define EP13_IN_EVEN    28
    #define EP13_IN_ODD     28
    #define EP14_OUT_EVEN   29
    #define EP14_OUT_ODD    29
    #define EP14_IN_EVEN    30
    #define EP14_IN_ODD     30
    #define EP15_OUT_EVEN   31
    #define EP15_OUT_ODD    31
    #define EP15_IN_EVEN    32
    #define EP15_IN_ODD     32

    #define EP(ep,dir,pp) (2*ep+dir+(((ep==0)&&(dir==0))?pp:2))
    #define BD(ep,dir,pp) (4*(ep+dir+(((ep==0)&&(dir==0))?pp:2)))

#elif (USB_PING_PONG_MODE == USB_PING_PONG__FULL_PING_PONG)
    #define USB_NEXT_EP0_OUT_PING_PONG 0x0004
    #define USB_NEXT_EP0_IN_PING_PONG 0x0004
    #define USB_NEXT_PING_PONG 0x0004
    #define EP0_OUT_EVEN    0
    #define EP0_OUT_ODD     1
    #define EP0_IN_EVEN     2
    #define EP0_IN_ODD      3
    #define EP1_OUT_EVEN    4
    #define EP1_OUT_ODD     5
    #define EP1_IN_EVEN     6
    #define EP1_IN_ODD      7
    #define EP2_OUT_EVEN    8
    #define EP2_OUT_ODD     9
    #define EP2_IN_EVEN     10
    #define EP2_IN_ODD      11
    #define EP3_OUT_EVEN    12
    #define EP3_OUT_ODD     13
    #define EP3_IN_EVEN     14
    #define EP3_IN_ODD      15
    #define EP4_OUT_EVEN    16
    #define EP4_OUT_ODD     17
    #define EP4_IN_EVEN     18
    #define EP4_IN_ODD      19
    #define EP5_OUT_EVEN    20
    #define EP5_OUT_ODD     21
    #define EP5_IN_EVEN     22
    #define EP5_IN_ODD      23
    #define EP6_OUT_EVEN    24
    #define EP6_OUT_ODD     25
    #define EP6_IN_EVEN     26
    #define EP6_IN_ODD      27
    #define EP7_OUT_EVEN    28
    #define EP7_OUT_ODD     29
    #define EP7_IN_EVEN     30
    #define EP7_IN_ODD      31
    #define EP8_OUT_EVEN    32
    #define EP8_OUT_ODD     33
    #define EP8_IN_EVEN     34
    #define EP8_IN_ODD      35
    #define EP9_OUT_EVEN    36
    #define EP9_OUT_ODD     37
    #define EP9_IN_EVEN     38
    #define EP9_IN_ODD      39
    #define EP10_OUT_EVEN   40
    #define EP10_OUT_ODD    41
    #define EP10_IN_EVEN    42
    #define EP10_IN_ODD     43
    #define EP11_OUT_EVEN   44
    #define EP11_OUT_ODD    45
    #define EP11_IN_EVEN    46
    #define EP11_IN_ODD     47
    #define EP12_OUT_EVEN   48
    #define EP12_OUT_ODD    49
    #define EP12_IN_EVEN    50
    #define EP12_IN_ODD     51
    #define EP13_OUT_EVEN   52
    #define EP13_OUT_ODD    53
    #define EP13_IN_EVEN    54
    #define EP13_IN_ODD     55
    #define EP14_OUT_EVEN   56
    #define EP14_OUT_ODD    57
    #define EP14_IN_EVEN    58
    #define EP14_IN_ODD     59
    #define EP15_OUT_EVEN   60
    #define EP15_OUT_ODD    61
    #define EP15_IN_EVEN    62
    #define EP15_IN_ODD     63

    #define EP(ep,dir,pp) (4*ep+2*dir+pp)

    #define BD(ep,dir,pp) (4*(4*ep+2*dir+pp))

#elif (USB_PING_PONG_MODE == USB_PING_PONG__ALL_BUT_EP0)
    #define USB_NEXT_EP0_OUT_PING_PONG 0x0000
    #define USB_NEXT_EP0_IN_PING_PONG 0x0000
    #define USB_NEXT_PING_PONG 0x0004
    #define EP0_OUT_EVEN    0
    #define EP0_OUT_ODD     0
    #define EP0_IN_EVEN     1
    #define EP0_IN_ODD      1
    #define EP1_OUT_EVEN    2
    #define EP1_OUT_ODD     3
    #define EP1_IN_EVEN     4
    #define EP1_IN_ODD      5
    #define EP2_OUT_EVEN    6
    #define EP2_OUT_ODD     7
    #define EP2_IN_EVEN     8
    #define EP2_IN_ODD      9
    #define EP3_OUT_EVEN    10
    #define EP3_OUT_ODD     11
    #define EP3_IN_EVEN     12
    #define EP3_IN_ODD      13
    #define EP4_OUT_EVEN    14
    #define EP4_OUT_ODD     15
    #define EP4_IN_EVEN     16
    #define EP4_IN_ODD      17
    #define EP5_OUT_EVEN    18
    #define EP5_OUT_ODD     19
    #define EP5_IN_EVEN     20
    #define EP5_IN_ODD      21
    #define EP6_OUT_EVEN    22
    #define EP6_OUT_ODD     23
    #define EP6_IN_EVEN     24
    #define EP6_IN_ODD      25
    #define EP7_OUT_EVEN    26
    #define EP7_OUT_ODD     27
    #define EP7_IN_EVEN     28
    #define EP7_IN_ODD      29
    #define EP8_OUT_EVEN    30
    #define EP8_OUT_ODD     31
    #define EP8_IN_EVEN     32
    #define EP8_IN_ODD      33
    #define EP9_OUT_EVEN    34
    #define EP9_OUT_ODD     35
    #define EP9_IN_EVEN     36
    #define EP9_IN_ODD      37
    #define EP10_OUT_EVEN   38
    #define EP10_OUT_ODD    39
    #define EP10_IN_EVEN    40
    #define EP10_IN_ODD     41
    #define EP11_OUT_EVEN   42
    #define EP11_OUT_ODD    43
    #define EP11_IN_EVEN    44
    #define EP11_IN_ODD     45
    #define EP12_OUT_EVEN   46
    #define EP12_OUT_ODD    47
    #define EP12_IN_EVEN    48
    #define EP12_IN_ODD     49
    #define EP13_OUT_EVEN   50
    #define EP13_OUT_ODD    51
    #define EP13_IN_EVEN    52
    #define EP13_IN_ODD     53
    #define EP14_OUT_EVEN   54
    #define EP14_OUT_ODD    55
    #define EP14_IN_EVEN    56
    #define EP14_IN_ODD     57
    #define EP15_OUT_EVEN   58
    #define EP15_OUT_ODD    59
    #define EP15_IN_EVEN    60
    #define EP15_IN_ODD     61

    #define EP(ep,dir,pp) (4*ep+2*dir+((ep==0)?0:(pp-2)))
    #define BD(ep,dir,pp) (4*(4*ep+2*dir+((ep==0)?0:(pp-2))))

#else
    #error "No ping pong mode defined."
#endif


/* Buffer Descriptor Table (BDT) definition
 *************************************************************************
 * These data structures define the buffer descriptor table used by the 
 * USB OTG Core to manage endpoint DMA.
 */
 
/*
 * This is union describes the bitmap of 
 * the Setup & Status entry in the BDT.
 */
typedef union _BDT_SETUP
{
    struct  // Status Entry
    {
        #if defined(__18CXX)
        unsigned short BC_MSB:  2;
        #else
        unsigned short spare:   2;
        #endif
        unsigned short TOK_PID: 4;  // Packit Identifier
        unsigned short DAT01:   1;  // Data-toggle bit
        unsigned short UOWN:    1;  // Descriptor owner: 0=SW, 1=HW
        #if !defined(__18CXX)
        unsigned short resvd:   8;
        #endif
     };

    struct  // Setup Entry
    {
        #if defined(__18CXX)
        unsigned short BC_MSB:  2;
        #else
        unsigned short spare:   2;
        #endif
        unsigned short BSTALL:  1;  // Stalls EP if this descriptor needed
        unsigned short DTS:     1;  // Require data-toggle sync
        unsigned short NINC:    1;  // No Increment of DMA address
        unsigned short KEEP:    1;  // HW Keeps this buffer & descriptor
        unsigned short DAT01:   1;  // Data-toggle number (0 or 1)
        unsigned short UOWN:    1;  // Descriptor owner: 0=SW, 1=HW 
        #if !defined(__18CXX)
        unsigned short resvd:   8;
        #endif
     };
     
     #if !defined(__18CXX)
     WORD Val;
     #else
     BYTE Val;
     #endif

} BDT_SETUP;

/*
 * This union describes the byte-count 
 * entry in the BDT.
 */
typedef union _BYTECOUNT
{
    struct  // Byte-count bitmap
    {
        unsigned short BC0:     1;
        unsigned short BC1:     1; 
        unsigned short BC2:     1;
        unsigned short BC3:     1;
        unsigned short BC4:     1;
        unsigned short BC5:     1;
        unsigned short BC6:     1;
        unsigned short BC7:     1;
        #if !defined(__18CXX)
        unsigned short BC8:     1;
        unsigned short BC9:     1;
        unsigned short resvd:   6;
        #endif
     };

    #if defined(__18CXX)
        //TODO: (DF) - C18 does not allow bitfields of > 8 bits.  made this a word but that does not mask the upper 6 bits from getting set
        struct  // Byte-count field
        {
            unsigned char BC; // Number of bytes in data buffer (really only 10 bits)
        };
    #else
    struct  // Byte-count field
    {
        unsigned short BC:      10; // Number of bytes in data buffer
        unsigned short resvd:   6;
    };
    #endif

} BYTECOUNT;

/*
 * Buffer Descriptor
 *
 * This union describes a single buffer descriptor.  Each descriptor 
 * manages a single buffer.  Each endpoint has 4 such descriptors, 2 
 * for receive and 2 for trasmit.  Having two descriptors (odd and even,
 * or ping and pong) per direction allows the HW to operate on one 
 * while the SW operates on the other.
 */
typedef union _BUFFER_DESCRIPTOR
{
    UINT32  dword[2];       // Double-word access

    UINT16  word[4];        // Word Access
    
    BYTE    byte[8];        // Byte Access

    struct
    {
        BDT_SETUP       setup;      // Setup & status entry
        BYTECOUNT       byte_cnt;   // Byte count entry
        unsigned int    addr;       // Physical address of data buffer
    };

} BUF_DESC, *pBUF_DESC;


/* USB_HAL_PIPE
 *************************************************************************
 * A pipe is a virtual connection between two endpoints, one in the host
 * and one the device.  Data flows through a pipe in a single direction 
 * from one endpoint to the other.  This structure defines the data that
 * the USB HAL must track to manage a pipe.
 */

typedef union
{
    BYTE    bitmap;

    struct
    {
        BYTE zero_pkt:    1; // Ensure transfer ends w/short or zero packet
        BYTE data_toggle: 1; // Data toggle: 0=DATA0/1=DATA1
        BYTE ping_pong:   1; // Current ping pong: 0=even/1=odd
        BYTE send_0_pkt:  1; // Flag indicating when to send a zero-sized packet
        BYTE reserved:    4; // Reserved

    }field;

}PIPE_FLAGS;

typedef struct _USB_HAL_PIPE_DATA
{
    BYTE           *buffer;         // Pointer to the buffer
    unsigned int    max_pkt_size;   // Max packet size of EP
    unsigned int    size;           // Total number of bytes to transfer
    unsigned int    remaining;      // Number of bytes remaining to transfer
    unsigned int    count;          // Actual number of bytes transferred.
    PIPE_FLAGS      flags;

} USB_HAL_PIPE, *PUSB_HAL_PIPE;


/* USB_ROLE
 *************************************************************************
 * This enumeration identifies if the USB controller is currently acting
 * as a USB device or as a USB host.
 */

typedef enum
{
    DEVICE = 0, // USB controller is acting as a USB device
    HOST   = 1  // USB controller is acting as a USB host

} USB_ROLE;


/******************************************************************************
    Function:
        OTGCORE_IdentifyPacket
        
    Summary:
        This provides the endpoint number, direction and ping-pong ID and other
        information identifying the packet that was just completed.
        
    PreCondition:
        Assumes that an UIR_TOK_DNE interrupt has occured.
        
    Parameters:
        None
        
    Return Values:
        flags   Bitmapped flags identifying transfer (see below):
 
                           1 1 1 1 1 1
                           5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
                           | | \_____/ \_/ \_____/ | | \_/
                           | |    |     |     |    | |  +-- reserved1
                           | |    |     |     |    | +----- ping_pong
                           | |    |     |     |    +------- direction
                           | |    |     |     +------------ ep_num
                           | |    |     +------------------ reserved2
                           | |    +------------------------ pid
                           | +----------------------------- data_toggle
                           +------------------------------- reserved3
 
                   size    Size of data actually transferred (in bytes).
        TRUE if successful, FALSE any of the parameters were NULL.
    
    Remarks:
        None
              
 ******************************************************************************/

typedef union
{
    UINT16  bitmap;
    BYTE    byte[2];
    struct
    {
        // Byte 0
        UINT16 reserved1:   2;  // Reserved, ignore
        UINT16 ping_pong:   1;  // Buffer ping pong: 0=even/1=odd
        UINT16 direction:   1;  // Transfer direction: 0=Receive, 1=Transmit
        UINT16 ep_num:      4;  // Endpoint number

        // Byte 1
        UINT16 reserved2:   2;  // Reserved, ignore
        UINT16 pid:         4;  // Packet ID (See USBCh9.h, "Packet IDs")
        UINT16 data_toggle: 1;  // Data toggle: 0=DATA0 packet, 1=DATA1 packet
        UINT16 reserved3:   1;  // Reserved, ignore

    }field;

} TRANSFER_ID_FLAGS;


/* USB Register Macros
 *************************************************************************
 * These macros translate core registers across the different controller
 * families.
 */

#if defined(__18CXX)
    #if defined(__18F4550) || defined(__18F2455) || defined(__18F2550) || defined(__18F4455)
        #error "Please define any necessary register translation macros.
    #endif
    
    #define EnableUsbModule()
    #define SetPingPongMode(m)  U1CNFG1 |= (m)
    
    #error "Please define any necessary register translation macros.
#elif defined(__C30__)

    #define EnableUsbModule()   U1PWRCbits.USBPWR = 1  
    #define SetPingPongMode(m)
    
    // Do we need any others???  #error "Please define any necessary register translation macros.
#else
    #define EnableUsbModule()   U1PWRCbits.USBPWR = 1  
    #define SetPingPongMode(m)
#endif

// Misc bits
#ifndef UPWRC_USB_OP_EN
#define UPWRC_USB_OP_EN     0x01    // USB Operation Enable
#endif

#ifndef UPWRC_SUSPEND
#define UPWRC_SUSPEND       0x02    // Suspend bus activity
#endif



/* USB_HAL_DATA
 *************************************************************************
 * This structure contains the data that is required to manage an 
 * instance of the USB HAL.
 */

/*#if defined( USB_SUPPORT_DEVICE )

    typedef struct _USB_HAL_DATA
    {   
        // Data transfer "pipe" array
        USB_HAL_PIPE    pipe[USB_DEV_HIGHEST_EP_NUMBER+1][2]; // Rx=0, Tx=1
        USB_ROLE        current_role;            // Acting as host or device?
        unsigned long   last_error;              // Last error state detected
    } USB_HAL_DATA, *PUSB_HAL_DATA;
    
    
    // Macro to get a pointer to the desired pipe data.
    #define FindPipe(e,d) &gHALData.pipe[(e)][(d)]

#endif  // defined( USB_SUPPORT_DEVICE )*/


/******************************************************************************
 	Function:
 		USBHALClearStatus
 		
 	Summary:
 		This routine clears the OTG Core's current status
 		
 	PreCondition:
 		None
 		
 	Parameters:
 		status  Bitmap of status bits to clear (caller sets bits it
 		wishes to be cleared).
 		
 	Return Values:
 		None
 		
 	Remarks:
 		The status indicated have been cleared.
 		   
 ******************************************************************************/

#define USBHALClearStatus(s) (U1IR = (s))


/******************************************************************************
    Function:
        USBHALGetStatus
    
    Summary:
        This routine reads the OTG Core's current status.
        
    PreCondition:
        None
 
    Parameters:
        None
        
    Return Values:
        The contents of the USB OTG Core's interrupt status register.
        
    Remarks:
        None
 
 ******************************************************************************/

#define USBHALGetStatus()   U1IR


/******************************************************************************
    Function:
        USBHALGetErrors
        
    Summary:
        This routine reads the current error status.
        
    PreCondition:
        None
        
    Parameters:
        None
        
    Return Values:
        The contents of the USB error-interrupt status register.
        
    Remarks:
        None
 
 ******************************************************************************/
#define USBHALGetErrors()   U1EIR


/******************************************************************************
    Function:
        USBHALClearErrors
        
    Summary:
        This clears given error status.
        
    PreCondition:
        None
        
    Paramters:
        errors - Bitmap of the error status bits (caller sets the bets 
                 it wishes to clear).
           
    Return Values:
        None
        
    Side effects:
        The errors indicated have been cleared.
        
   Remarks:
        None
        
 ******************************************************************************/
#define USBHALClearErrors(e) (U1EIR = (e))


#endif  // _USB_HAL_LOCAL_H_
/*************************************************************************
 * EOF usb_hal.h
 */

