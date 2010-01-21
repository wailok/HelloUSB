/*********************************************************************

             Microchip USB Firmware -  CDC Serial Emulation

This file defines the CDC Serial Emulation driver's interface.
*******************************************************************************/
//DOM-IGNORE-BEGIN
/******************************************************************************

 * FileName:        usb_cdcserial.h
 * Dependencies:    See INCLUDES section below
 * Processor:       TBD
 * Compiler:        TBD
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

Author               Date       Comment
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Rawin Rojvanit       7/21/04    Original.
Bud Caldwell         12/5/06    Updated for PIC32 USB Device stack.
********************************************************************/
#ifndef _USB_CDCSERIAL_H_
#define _USB_CDCSERIAL_H_
//DOM-IGNORE-END


/** I N C L U D E S **********************************************************/
#include <GenericTypeDefs.h>
#include <USB\usb.h>


/** D E F I N I T I O N S ****************************************************/

/* Class-Specific Requests */
#define SEND_ENCAPSULATED_COMMAND   0x00
#define GET_ENCAPSULATED_RESPONSE   0x01
#define SET_COMM_FEATURE            0x02
#define GET_COMM_FEATURE            0x03
#define CLEAR_COMM_FEATURE          0x04
#define SET_LINE_CODING             0x20
#define GET_LINE_CODING             0x21
#define SET_CONTROL_LINE_STATE      0x22
#define SEND_BREAK                  0x23

/* Notifications

 Note: Notifications are polled over
 Communication Interface (Interrupt Endpoint)
*/
#define NETWORK_CONNECTION          0x00
#define RESPONSE_AVAILABLE          0x01
#define SERIAL_STATE                0x20


/* Device Class Code */
#define CDC_DEVICE                  0x02

/* Communication Interface Class Code */
#define COMM_INTF                   0x02

/* Communication Interface Class SubClass Codes */
#define ABSTRACT_CONTROL_MODEL      0x02

/* Communication Interface Class Control Protocol Codes */
#define V25TER                      0x01    // Common AT commands ("Hayes(TM)")


/* Data Interface Class Codes */
#define DATA_INTF                   0x0A

/* Data Interface Class Protocol Codes */
#define NO_PROTOCOL                 0x00    // No class specific protocol required


/* Communication Feature Selector Codes */
#define ABSTRACT_STATE              0x01
#define COUNTRY_SETTING             0x02

/* Functional Descriptors */
/* Type Values for the bDscType Field */
#define CS_INTERFACE                0x24
#define CS_ENDPOINT                 0x25

/* bDscSubType in Functional Descriptors */
#define DSC_FN_HEADER               0x00
#define DSC_FN_CALL_MGT             0x01
#define DSC_FN_ACM                  0x02    // ACM - Abstract Control Management
#define DSC_FN_UNION                0x06
/* more.... see Table 25 in USB CDC Specification 1.1 */

/* CDC Bulk IN transfer states */
#define CDC_TX_READY                0
#define CDC_TX_BUSY                 1
#define CDC_TX_BUSY_ZLP             2       // ZLP: Zero Length Packet
#define CDC_TX_COMPLETING           3


/** S T R U C T U R E S ******************************************************/

/* Line Coding Structure */
#define LINE_CODING_LENGTH          0x07

/* Line Coding Data
 
This structure contains the data needed to specify the current coding
of the serial port line.  It is the structure used by the USB CDC 
class-specific GET_LINE_CODING and SET_LINE_CODING requests.
*/

typedef union _LINE_CODING
{
    struct
    {
        BYTE _byte[LINE_CODING_LENGTH];
    };
    struct
    {
        DWORD   dwDTERate;          // Complex data structure
        BYTE    bCharFormat;
        BYTE    bParityType;
        BYTE    bDataBits;
    };
} LINE_CODING;

// Line Coding Definitions:
//
// dwDTERate = number of bits per second
//
// bCharFormat
#define USBCDC_1_STOP_BIT           0
#define USBCDC_1_POINT_5_STOP_BITS  1
#define USBCDC_2_STOP_BITS          2
//
// bParityType
#define USBCDC_NO_PARITY            0
#define USBCDC_ODD_PARITY           1
#define USBCDC_EVEN_PARITY          2
#define USBCDC_MARK_PARITY          3
#define USBCDC_SPACE_PARITY         4
//
// bDataBits = number of bits per word (5, 6, 7, 8, or 16)


typedef union _CONTROL_SIGNAL_BITMAP
{
    BYTE _byte;
    struct
    {
        unsigned DTE_PRESENT;       // [0] Not Present  [1] Present
        unsigned CARRIER_CONTROL;   // [0] Deactivate   [1] Activate
    };
} CONTROL_SIGNAL_BITMAP;


/* Functional Descriptor Structure - See CDC Specification 1.1 for details */

#pragma pack(push,1)  // Must not have any padding.

/* Header Functional Descriptor */
typedef struct _USB_CDC_HEADER_FN_DSC
{
    BYTE bFNLength;
    BYTE bDscType;
    BYTE bDscSubType;
    WORD bcdCDC;
} USB_CDC_HEADER_FN_DSC;

/* Abstract Control Management Functional Descriptor */
typedef struct _USB_CDC_ACM_FN_DSC
{
    BYTE bFNLength;
    BYTE bDscType;
    BYTE bDscSubType;
    BYTE bmCapabilities;
} USB_CDC_ACM_FN_DSC;

/* Union Functional Descriptor */
typedef struct _USB_CDC_UNION_FN_DSC
{
    BYTE bFNLength;
    BYTE bDscType;
    BYTE bDscSubType;
    BYTE bMasterIntf;
    BYTE bSlaveIntf0;
} USB_CDC_UNION_FN_DSC;

/* Call Management Functional Descriptor */
typedef struct _USB_CDC_CALL_MGT_FN_DSC
{
    BYTE bFNLength;
    BYTE bDscType;
    BYTE bDscSubType;
    BYTE bmCapabilities;
    BYTE bDataInterface;
} USB_CDC_CALL_MGT_FN_DSC;

#pragma pack(pop)


/* CDC Notifications */
typedef SETUP_PKT CDC_NOTIFICATION;

/* bmRequestType for CDC Notifications */
#define CDC_NOTIFICATION_REQ    (USB_SETUP_DEVICE_TO_HOST | \
                                 USB_SETUP_TYPE_CLASS     | \
                                 USB_SETUP_RECIPIENT_INTERFACE)

/* Notification Number */
#define NETWORK_CONNECTION      0x00
#define RESPONSE_AVAILABLE      0x01
#define SERIAL_STATE            0x20

// Serial State Bits
#define SER_STATE_TX_CARRIER    0x02
#define SER_STATE_RX_CARRIER    0x01

/* Macro for initializing notifications

 * n = Notification number (above)
 * v = wValue  (notification specific)
 * i = wIndex  (notification specific)
 * l = wLength (notification specific) Length of data to follow, if any.
 */
#define CDC_INIT_NOTIFICATION(n,v,i,l) {CDC_NOTIFICATION_REQ,(n),(v),(i),(l)}



// *****************************************************************************
/* Section: USB_EVENT - CDC Serial Function Events

These definitions extend the USB_EVENT enumeration that identifies USB events 
that occur.  They are specific to the USB Serial Function driver.  An 
application can implement a event-handling (USB_EVENT_HANDLER) routine if it
needs to receive them.
*/
 
#ifndef EVENT_CDC_OFFSET      // The application can add a non-zero offset 
#define EVENT_CDC_OFFSET 100  // to the CDC events to resolve conflicts
#endif                        // in event number.
 
#define EVENT_CDC_LINE_CTRL (EVENT_USER_BASE+EVENT_CDC_OFFSET+0)
        // A change in the line-control status occured
        // data: NULL
        // size: 0

#define EVENT_CDC_CMD       (EVENT_USER_BASE+EVENT_CDC_OFFSET+1)
        // An encapsulated command is about to be sent
        // data: Length of command to be sent
        // size: sizeof(UINT16)

#define EVENT_CDC_CMD_RCVD  (EVENT_USER_BASE+EVENT_CDC_OFFSET+2)
        // An encapsulated command has been received
        // data: Length of command received
        // size: sizeof(UINT32)

#define EVENT_CDC_RESP      (EVENT_USER_BASE+EVENT_CDC_OFFSET+3)
        // An encapsulated response has been requested
        // data: Length of response
        // size: sizeof(UINT16)
    


/*****************************************************************************
Communications Class Device: Abstract Control Model, Serial Emulation API
                       AKA: USB COM Port Emulation
*****************************************************************************/
 
/*************************************************************************
 Function:        USBUSARTTxIsReady

 Precondition:    System has been enumerated as a CDC Serial emulation
                  device on the USB and the CDC function driver has been
                  initialized.

 Input:           none

 Output:          none.

 Returns:         TRUE if the system is ready to transmit data on the
                  USB, FALSE if not.

 Side Effects:    none

 Overview:        This function determines if the CDC class is ready to 
                  send data on the USB.

 Notes:           none
 *************************************************************************/

BOOL USBUSARTTxIsReady ( void );


/*************************************************************************
 Function:        USBUSARTTxIsBusy

 Precondition:    System has been enumerated as a CDC Serial emulation
                  device on the USB and the CDC function driver has been
                  initialized.

 Input:           none

 Output:          none.

 Returns:         TRUE if the system is busy transmitting data on the
                  USB, FALSE if not.

 Side Effects:    none

 Overview:        This function determines if the CDC class is currently
                  busy sending data on the USB.

 Notes:           none
*************************************************************************/

BOOL USBUSARTTxIsBusy ( void );

/*************************************************************************
 Function:        USBUSARTRxIsReady

 Precondition:    System has been enumerated as a CDC Serial emulation
                  device on the USB and the CDC function driver has been
                  initialized.

 Input:           none

 Output:          none.

 Returns:         TRUE if the system is ready to receive data on the
                  USB, FALSE if not.

 Side Effects:    none

 Overview:        This function determines if the CDC class is ready to 
                  receive data on the USB.

 Notes:           none
*************************************************************************/

BOOL USBUSARTRxIsReady ( void );


/*************************************************************************
 Function:        USBUSARTRxIsBusy

 Precondition:    System has been enumerated as a CDC Serial emulation
                  device on the USB and the CDC function driver has been
                  initialized.

 Input:           none

 Output:          none.

 Returns:         TRUE if the system is busy receiving data on the
                  USB, FALSE if not.

 Side Effects:    none

 Overview:        This function determines if the CDC class is currently
                  busy receiving data on the USB.

 Notes:           none
 *************************************************************************/

BOOL USBUSARTRxIsBusy ( void );


/*************************************************************************
 Function:        USBUSARTRxGetLength

 Precondition:    System has been enumerated as a CDC Serial emulation
                  device on the USB and the CDC function driver has been
                  initialized.

 Input:           none

 Output:          none.

 Returns:         The current number of bytes available in the caller's 
                  buffer.

 Side Effects:    none

 Overview:        This function provides the current number of bytes 
                  that have been received from the USB and placed into 
                  the caller's buffer.

 Notes:           none
 *************************************************************************/

BYTE USBUSARTRxGetLength ( void );


/*************************************************************************
 Function:        USBUSARTTx

 Precondition:    System has been enumerated as a CDC Serial emulation
                  device on the USB and the CDC function driver has been
                  initialized.

                  USBUSARTTxIsReady must have returned TRUE.

 Input:           pData   Pointer to the data to transmit.

                  len     Length of the data, in bytes.

 Output:          none

 Returns:         none

 Side Effects:    The USB Tx transfer will have been started.

 Overview:        This function starts a USB Tx Transfer on the CDC data
                  IN endpoint.

 Notes:           This function is not-blocking, so the transfer will 
                  only have been started, not completed.  The caller 
                  will have to monitor USBUSARTTxIsReady or 
                  USBUSARTTxIsBusy to determine when the transfer has 
                  completed.
 *************************************************************************/

void USBUSARTTx( BYTE *pData, BYTE len );


/*************************************************************************
 Function:        USBUSARTRx

 Precondition:    System has been enumerated as a CDC Serial emulation
                  device on the USB and the CDC function driver has been
                  initialized.

                  USBUSARTRxIsReady must have returned TRUE.

                  The length of the buffer specified must be at least as
                  large as the size specified in the Data OUT endpoint
                  descriptor and configuration structures.

 Input:           pData   Pointer to the buffer in which to receive data.

                  len     Length of the buffer, in bytes.

 Output:          none

 Returns:         none

 Side Effects:    The CDC will become ready to receive data into the 
                  caller's buffer from the serial Data In endpoint.

 Overview:        This function sets the CDC Data-RX (USB OUT) endpoint's
                  buffer to the one specified by the caller and starts a 
                  USB Rx Transfer on the CDC data OUT endpoint.

 Notes:           This function is not-blocking, so the transfer will 
                  only have been started, not completed.  The caller 
                  will have to monitor USBUSARTRxIsReady or
                  USBUSARTRxIsBusy to determine when the transfer has 
                  completed.
 *************************************************************************/

void USBUSARTRx( BYTE *pData, BYTE len );


/******************************************************************************
 Function:        USBUSARTGets

 Precondition:    System has been enumerated as a CDC Serial emulation
                  device on the USB and the CDC function driver has been
                  initialized.

 Input:           len     : The number of bytes expected.

 Output:          buffer  : Pointer to where received bytes are to be stored

 Returns:         The number of bytes copied to buffer.

 Side Effects:    An internal state variable that maintains the size of data
                  copied into the caller's buffer may have been updated. 
                  the caller may obtain this value by calling 
                  USBUSARTRxGetLength.

 Overview:        USBUSARTGets copies a string of bytes received through
                  USB CDC Bulk OUT endpoint to a user's specified location. 

 Note:            It is a non-blocking function. It does not wait
                  for data if there is no data available. Instead it returns
                  '0' to notify the caller that there is no data available.
 *****************************************************************************/

BYTE USBUSARTGets ( char *buffer, BYTE len );


/******************************************************************************
 Function:        USBUSARTPuts

 Precondition:    System has been enumerated as a CDC Serial emulation
                  device on the USB and the CDC function driver has been
                  initialized.

                  USBUSARTTxIsReady must have returned TRUE.

 Input:           data    : Pointer to a null-terminated string of data.
                            If a null character is not found, 255 bytes
                            of data will be transferred to the host.

 Output:          none

 Side Effects:    A USB IN (Tx) transfer has been started.

 Overview:        Writes a string of data to the USB Data IN endpoint, 
                  including the null character.

 Note:           none
 *****************************************************************************/

void USBUSARTPuts ( char *data );


/******************************************************************************
 Function:        USBUSARTGetLineCoding

 Precondition:    System has been enumerated as a CDC Serial emulation
                  device on the USB and the CDC function driver has been
                  initialized.

 Input:           none

 Output:          pLC     : Pointer to a LINE_CODING data structure to
                            receive the current line coding data.

 Returns:         TRUE if successful, FALSE if the line-coding data was in
                  the process of being updated.

 Side Effects:    none

 Overview:        This function provides the current line-coding (BPS, 
                  # stop bits, parity, & data bits-per-word) information.

 Note:            If the line coding data is currently being updated by the
                  host, this function will return FALSE and the caller 
                  should retry it later.
 *****************************************************************************/

BOOL USBUSARTGetLineCoding ( LINE_CODING *pLC );


/******************************************************************************
 Function:        USBUSARTGetCmdStr

 Precondition:    System has been enumerated as a CDC Serial emulation
                  device on the USB and the CDC function driver has been
                  initialized.

                  The client has received a EVT_CDC_CMD event notification in
                  response to the host sending a SEND_ENCAPSULATED_COMMAND
                  request.

 Input:           len     : The number of bytes expected

 Output:          buffer  : Pointer to where received bytes are to be stored

 Returns:         TRUE if able to start the transfer, FALSE if not.

 Side Effects:    The next len bytes read from EP0 will be read into the 
                  caller's buffer.

 Overview:        This routine retrieves a len byte command string from 
                  EP0 into the caller's buffer.  

 Note:            It should only ever be called when a EVT_CDC_CMD event 
                  notification has been received or the caller may interfere
                  with EP0 request processing.
 *****************************************************************************/
BOOL USBUSARTGetCmdStr ( char *buffer, BYTE len );


/******************************************************************************
 Function:        USBUSARTSendRespStr

 Precondition:    System has been enumerated as a CDC Serial emulation
                  device on the USB and the CDC function driver has been
                  initialized.

                  The client musthave received a EVT_CDC_RESP event in 
                  response the host sending a GET_ENCAPSULATED_RESPONSE 
                  request must have been received.

 Input:           data    : Pointer to a null-terminated string of response
                            data.

 Output:          none

 Side Effects:    A USB IN (Tx) transfer has been started on EP0.

 Overview:        Writes a string of data to USB EP0, in response to a 
                  GET_ENCAPSULATED_RESPONSE request from the host.

 Note:            This routine should only be called once per 
                  GET_ENCAPSULATED_RESPONSE request.

                  We do not care when the response transfer finishes, since
                  we will find out when the host sends another request 
                  packet, so there is no way for the caller to determine 
                  this.  
 *****************************************************************************/

BOOL USBUSARTSendRespStr ( char *data );


/******************************************************************************
 Function:        USBUSARTSendNotification

 Precondition:    System has been enumerated as a CDC Abstract Control Model
                  device on the USB and the CDC function driver has been
                  initialized.

 Input:           notification    Pointer to the notification data to the host.

 Output:          none

 Side Effects:    A USB IN (Tx) transfer has been started on the notification
                  endpoint with the given data packet.

 Overview:        Sends a notification packet to the host through the
                  notification endpoint.

 Note:            This routine should only be called once per 
                  SEND_ENCAPSULATED_COMMAND request and then only when the 
                  response has been prepared.

                  We do not care when the response transfer finishes, 
                  so there is no way for the caller to determine this..
 *****************************************************************************/

BOOL USBUSARTSendNotification ( CDC_NOTIFICATION *notification );


/***********************************
 Device-layer Interface Routines
 ***********************************/

/*************************************************************************
 Function:        USBUARTInit

 Precondition:    USBDEVInitialize has been called and the system has 
                  been enumerated as a CDC Serial Emulation device on 
                  the USB.

 Input:           flags       Initialization flags

 Output:          none

 Returns:         TRUE if successful, FALSE if not.

 Side Effects:    The USB function driver called has been initialized.

 Overview:        This routine is a call out from the device layer to a
                  USB function driver.  It is called when the system 
                  has been configured as a USB peripheral device by the
                  host.  Its purpose is to initialize and activate the
                  function driver.

 Note:            There may be multiple function drivers.  If so, the
                  USB device layer will call the initialize routine
                  for each one of the function that are in the selected
                  configuration.
 *************************************************************************/

BOOL USBUARTInit ( unsigned long flags );


/*************************************************************************
 Function:        USBUARTEventHandler

 Preconditions:   1. USBInitialize must have been called to initialize 
                  the USB SW Stack.

                  2. The host must have configured the system as a USB
                  CDC Serial Emulation Device. 
                  
 Input:           event       Identifies the bus event that occured

                  data        Pointer to event-specific data

                  size        Size of the event-specific data

 Output:          none

 Returns:         TRUE if the event was handled, FALSE if not

 Side Effects:    Event-specific actions have been taken.

 Overview:        This routine is called by the "device" layer to notify
                  the general function of events that occur on the USB.
                  If the event is recognized, it is handled and the 
                  routine returns TRUE.  Otherwise, it is ignored and 
                  the routine returns FALSE.
 *************************************************************************/

BOOL USBUARTEventHandler ( USB_EVENT event, void *data, unsigned int size );


/********************************
 Backward Compatiblity Macros
 ********************************

 These interface macros have have been depreciated and are only provided 
 for backward compatiblity.  They should not be used by new applications.
 */

#define mUSBUSARTIsTxTrfReady()   USBUSARTTxIsReady()
#define mCDCUsartRxIsBusy()       ((unsigned)USBUSARTRxIsBusy())
#define mCDCUsartTxIsBusy()       ((unsigned)USBUSARTTxIsBusy())
#define mCDCGetRxLength()         USBUSARTRxGetLength()
#define mUSBUSARTTxRam(p,l)       USBUSARTTx((p),(l))
#define mUSBUSARTTxRom(p,l)       USBUSARTTx((p),(l))
#define CDCInitEP()
#define CDCTxService()
#define getsUSBUSART(p,l)         USBUSARTGets((p),(l))
#define putsUSBUSART(p)           USBUSARTPuts((p))
#define putrsUSBUSART(p)          USBUSARTPuts((char *)(p))


#endif //_USB_CDCSERIAL_H_

/******************************************************************************
 * EOF usb_cdcserial.h
 */

