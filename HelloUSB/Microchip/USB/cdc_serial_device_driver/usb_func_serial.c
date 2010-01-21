/*********************************************************************
 *
 *             Microchip USB Firmware -  CDC Serial Emulation
 *
 *********************************************************************
 * FileName:        usb_cdcserial.c
 * Dependencies:    See INCLUDES section below
 * Processor:       TBD
 * Compiler:        TBD
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the “Company”) for its PICmicro® Microcontroller is intended and
 * supplied to you, the Company’s customer, for use solely and
 * exclusively on Microchip PICmicro Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN “AS IS” CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 * Author               Date        Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Rawin Rojvanit       11/19/04    Original. RS-232 Emulation Subset
 * Bud Caldwell         11/5/06     Ported to new USB SW stack.
 ********************************************************************/

/** I N C L U D E S **********************************************************/

#include "GenericTypeDefs.h"
#include "USB/usb.h"
#include "USB/usb_device_cdc_serial.h"

#include "usb_func_serial_local.h"


/** G L O B A L   D A T A ****************************************************/

// CDC Serial Emulation State Data
CDCSER_DAT gCdcSer;


/** L O C A L   R O U T I N E S **********************************************/


/******************************************************************************
 * Function:        void USBCheckCDCRequest(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine checks the setup data packet to see if it
 *                  knows how to handle it
 *
 * Note:            None
 *****************************************************************************/
BOOL USBCheckCDCRequest( PSETUP_PKT pkt )
{
    /*
     * If request recipient is not an interface then return
     */
    if(pkt->requestInfo.recipient != USB_SETUP_RECIPIENT_INTERFACE) 
        return FALSE;

    /*
     * If request type is not class-specific then return
     */
    if(pkt->requestInfo.type != (USB_SETUP_TYPE_CLASS >> 5)) 
        return FALSE;

    /*
     * Interface ID must match interface numbers associated with
     * CDC class, else return
     */
    if(pkt->wIndex != CDC_COMM_INTF_ID) 
        return FALSE;

    
    switch(pkt->bRequest)
    {
    case SEND_ENCAPSULATED_COMMAND:
        // Encapsulated command about to be sent, notify user.
        return CDC_APP_EVENT_HANDLING_FUNC(EVENT_CDC_CMD, &pkt->wLength, sizeof(UINT16));

    case GET_ENCAPSULATED_RESPONSE:
        // Encapsulated response requested, notify user.
        return CDC_APP_EVENT_HANDLING_FUNC(EVENT_CDC_RESP, &pkt->wLength, sizeof(UINT16));

    case SET_LINE_CODING:
        // Start a Rx transaction on EP0 to get the line coding.
        gCdcSer.flags |= CDC_FLAGS_LINE_CTRL_BUSY;
        USBDEVTransferData(XFLAGS(USB_EP0|USB_RECEIVE), &gCdcSer.line_coding, LINE_CODING_LENGTH);
        return FALSE;   // Don't want to start a new setup packet yet
    
    case GET_LINE_CODING:
        // Start a Tx transaction on EP0 to send the line coding.
        if (USBDEVTransferData(XFLAGS(USB_SETUP_DATA|USB_TRANSMIT), &gCdcSer.line_coding, LINE_CODING_LENGTH))
        {
            // Prepare now to receive the status packet in case host cuts us short.
            gCdcSer.flags |= CDC_FLAGS_WAITING_STATUS;
            USBHALTransferData(XFLAGS(USB_SETUP_STATUS|USB_RECEIVE), NULL, 0);
        }
        gCdcSer.flags |= CDC_FLAGS_SENT_DATA;
        return FALSE;   // Don't want to start a new setup packet yet

    case SET_CONTROL_LINE_STATE:

        // We ignore this request, assuming that the host is always 
        // present and that we control our own carrier.  However, we 
        // need to send a zero-byte packet to satisfy the status stage.
        gCdcSer.flags |= CDC_FLAGS_WAITING_STATUS;
        USBDEVTransferData(XFLAGS(USB_SETUP_DATA|USB_TRANSMIT), NULL, 0);
        return FALSE; // Don't want to start a new setup packet yet.

    // Optional, but not Supported 
    // (as reported in descriptors).
    case SET_COMM_FEATURE:
    case GET_COMM_FEATURE:
    case CLEAR_COMM_FEATURE:
    case SEND_BREAK:
    default:
        break;
    }

    return FALSE;

} //end USBCheckCDCRequest


/* HandleTransferDone
 *************************************************************************
 * This routine sets the appropriate state flags and data when a Tx or Rx
 * transfer finishes.
 */

PRIVATE inline BOOL HandleTransferDone( USB_TRANSFER_EVENT_DATA * xfer )
{
    #ifdef USB_SAFE_MODE
    
    if (xfer == NULL) {
        return FALSE;
    }

    #endif
    
    // Was it one of our endpoints?
    switch(xfer->flags.field.ep_num) 
    {
    case 0: // EP0

        // Identify transfer that finished
        if (gCdcSer.flags & CDC_FLAGS_LINE_CTRL_BUSY)
        {
            // New line-control data has been received, send status...
            gCdcSer.flags &= ~CDC_FLAGS_LINE_CTRL_BUSY;
            gCdcSer.flags |= CDC_FLAGS_WAITING_STATUS;
            USBHALTransferData(XFLAGS(USB_SETUP_STATUS|USB_TRANSMIT), NULL, 0);

            // ...and notify the user
            CDC_APP_EVENT_HANDLING_FUNC(EVENT_CDC_LINE_CTRL, NULL, 0);
            return FALSE;
        }
        else if (gCdcSer.flags & CDC_FLAGS_GET_CMD_BUSY)
        {
            // New encapsulated command received, notify user.
            gCdcSer.flags &= ~CDC_FLAGS_GET_CMD_BUSY;
            CDC_APP_EVENT_HANDLING_FUNC(EVENT_CDC_CMD_RCVD, xfer->size, sizeof(xfer->size));
            return TRUE;
        }
        else if (gCdcSer.flags & CDC_FLAGS_SEND_RESP_BUSY)
        {
            // Encapsulated response sent.
            gCdcSer.flags &= ~CDC_FLAGS_SEND_RESP_BUSY;
            return TRUE;
        }
        else if (gCdcSer.flags & CDC_FLAGS_SENT_DATA)
        {
            // Data has been sent, clear the flag.
            gCdcSer.flags &= ~CDC_FLAGS_SENT_DATA;
            return FALSE;   // Don't want to start new setup packet yet
        }
        else if (gCdcSer.flags & CDC_FLAGS_WAITING_STATUS)
        {
            // Clear the state flag
            gCdcSer.flags &= ~CDC_FLAGS_WAITING_STATUS;
            return TRUE;
        }
        break;

    case CDC_INT_EP_NUM: // Notification EP

        // Clear the notification endpoint busy flag
        gCdcSer.flags &= ~CDC_FLAGS_NOTIFY_BUSY;
        return TRUE;

    case CDC_BULK_EP_NUM: // Data EP

        // Did a transmit transfer finish?
        if ( xfer->flags.field.direction == 1)
        {
            // Yes, clear the Tx flag.
            gCdcSer.flags &= ~CDC_FLAGS_TX_BUSY;
            return TRUE;
        }
        else   // A receive transfer finished.
        {
            // Yes, Clear the the Rx-data-available flag & record the size.
            gCdcSer.flags &= ~CDC_FLAGS_RX_BUSY;
            gCdcSer.rx_size = (BYTE)xfer->size;
            return TRUE;
        }
        break;

    // Default & fall-through
    }

    // It's not ours.
    return FALSE;
}


/**************************
 * Device-layer Interface *
 **************************/

/*************************************************************************
 * Function:        USBUARTInit
 *
 * Precondition:    USBDEVInitialize has been called and the system has 
 *                  been enumerated as a CDC Serial Emulation device on 
 *                  the USB.
 *
 * Input:           flags       Initialization flags (see below)
 *
 * Output:          none
 *
 * Returns:         TRUE if successful, FALSE if not.
 *
 * Side Effects:    The USB function driver called has been initialized.
 *
 * Overview:        This routine is a call out from the device layer to a
 *                  USB function driver.  It is called when the system 
 *                  has been configured as a USB peripheral device by the
 *                  host.  Its purpose is to initialize and activate the
 *                  function driver.
 *
 * Note:            There may be multiple function drivers.  If so, the
 *                  USB device layer will call the initialize routine
 *                  for each one of the function that are in the selected
 *                  configuration.
 *************************************************************************/

PUBLIC BOOL USBUARTInit ( unsigned long flags )
{
    // Clear the state data
    memset(&gCdcSer, 0, sizeof(gCdcSer));

    // Initialize an state necessary
    gCdcSer.flags  = flags & CDC_FLAGS_INIT_MASK;

    // Initialize default line coding
    gCdcSer.line_coding.dwDTERate   = CDC_DEFAULT_BPS;
    gCdcSer.line_coding.bCharFormat = CDC_DEFAULT_FORMAT;
    gCdcSer.line_coding.bParityType = CDC_DEFAULT_PARITY;
    gCdcSer.line_coding.bDataBits   = CDC_DEFAULT_NUM_BITS;

    // Indicate that the CDC driver has been initialized
    gCdcSer.flags |= CDC_FLAGS_INITIALIZED;

    return TRUE;
}


/*************************************************************************
 * Function:        USBUARTEventHandler
 *
 * Preconditions:   1. USBInitialize must have been called to initialize 
 *                  the USB SW Stack.
 *
 *                  2. The host must have configured the system as a USB
 *                  CDC Serial Emulation Device. 
 *                  
 * Input:           event       Identifies the bus event that occured
 *
 *                  data        Pointer to event-specific data
 *
 *                  size        Size of the event-specific data
 *
 * Output:          none
 *
 * Returns:         TRUE if the event was handled, FALSE if not
 *
 * Side Effects:    Event-specific actions have been taken.
 *
 * Overview:        This routine is called by the "device" layer to notify
 *                  the general function of events that occur on the USB.
 *                  If the event is recognized, it is handled and the 
 *                  routine returns TRUE.  Otherwise, it is ignored and 
 *                  the routine returns FALSE.
 *************************************************************************/

PUBLIC BOOL USBUARTEventHandler ( USB_EVENT event, void *data, unsigned int size )
{
    // Abort if not initialized.
    if ( !(gCdcSer.flags & CDC_FLAGS_INITIALIZED) ) {
        return FALSE;
    }

    // Handle specific events.
    switch (event)
    {
    case EVENT_TRANSFER:  // A USB transfer has completed.

        #ifdef USB_SAFE_MODE
        if (size == sizeof(USB_TRANSFER_EVENT_DATA)) {
            return HandleTransferDone((USB_TRANSFER_EVENT_DATA *)data);
        } else {
            return FALSE;
        }
        #else
        return HandleTransferDone((USB_TRANSFER_EVENT_DATA *)data);
        #endif

    case EVENT_SUSPEND:   // Device-mode suspend/idle event received
    case EVENT_DETACH:    // USB cable has been detached
        
        // De-initialize the function driver.
        gCdcSer.flags &= ~CDC_FLAGS_INITIALIZED;
        return TRUE;

    case EVENT_RESUME:    // Device-mode resume received, re-initialize
        return USBUARTInit(gCdcSer.flags);
        
    case EVENT_SETUP:     // Class-specific setup packet received
        #ifdef USB_SAFE_MODE
        if (size == sizeof(SETUP_PKT)) {
            return USBCheckCDCRequest(data);
        } else {
            return FALSE;
        }
        #else
        return USBCheckCDCRequest(data);
        #endif

    case EVENT_BUS_ERROR: // Error on the bus, call USBDEVGetLastError()
        USBDEVGetLastError();
        // To Do: Capture the error and do something about it.
        return TRUE;

    }

    // Unknown event
    return FALSE;

} // USBUARTEventHandler


/**************************************
 * CDC Serial Emulation API Functions *
 **************************************/

/*************************************************************************
 * Function:        USBUSARTTxIsReady
 *
 * Precondition:    System has been enumerated as a CDC Serial emulation
 *                  device on the USB and the CDC function driver has been
 *                  initialized.
 *
 * Input:           none
 *
 * Output:          none.
 *
 * Returns:         TRUE if the system is ready to transmit data on the
 *                  USB, FALSE if not.
 *
 * Side Effects:    none
 *
 * Overview:        This function determines if the CDC class is ready to 
 *                  send data on the USB.
 *
 * Notes:           none
 *************************************************************************/

PUBLIC inline BOOL USBUSARTTxIsReady ( void )
{
    if ( !(gCdcSer.flags & CDC_FLAGS_INITIALIZED) ) {
        return FALSE;
    }

    if ( gCdcSer.flags & CDC_FLAGS_TX_BUSY )

        return FALSE;

    else

        return TRUE;

} // USBUSARTTxIsReady


/*************************************************************************
 * Function:        USBUSARTTxIsBusy
 *
 * Precondition:    System has been enumerated as a CDC Serial emulation
 *                  device on the USB and the CDC function driver has been
 *                  initialized.
 *
 * Input:           none
 *
 * Output:          none.
 *
 * Returns:         TRUE if the system is busy transmitting data on the
 *                  USB, FALSE if not.
 *
 * Side Effects:    none
 *
 * Overview:        This function determines if the CDC class is currently
 *                  busy sending data on the USB.
 *
 * Notes:           none
 *************************************************************************/

PUBLIC inline BOOL USBUSARTTxIsBusy ( void )
{
    if ( !(gCdcSer.flags & CDC_FLAGS_INITIALIZED) ) {
        return FALSE;
    }

    if ( gCdcSer.flags & CDC_FLAGS_TX_BUSY )

        return TRUE;

    else

        return FALSE;

} // USBUSARTTxIsBusy


/*************************************************************************
 * Function:        USBUSARTRxIsReady
 *
 * Precondition:    System has been enumerated as a CDC Serial emulation
 *                  device on the USB and the CDC function driver has been
 *                  initialized.
 *
 * Input:           none
 *
 * Output:          none.
 *
 * Returns:         TRUE if the system is ready to receive data on the
 *                  USB, FALSE if not.
 *
 * Side Effects:    none
 *
 * Overview:        This function determines if the CDC class is ready to 
 *                  receive data on the USB.
 *
 * Notes:           none
 *************************************************************************/

PUBLIC BOOL USBUSARTRxIsReady ( void )
{
    if ( !(gCdcSer.flags & CDC_FLAGS_INITIALIZED) ) {
        return FALSE;
    }

    if (gCdcSer.flags & CDC_FLAGS_RX_BUSY)

        return FALSE;

    else

        return TRUE;

} // USBUSARTRxIsReady


/*************************************************************************
 * Function:        USBUSARTRxIsBusy
 *
 * Precondition:    System has been enumerated as a CDC Serial emulation
 *                  device on the USB and the CDC function driver has been
 *                  initialized.
 *
 * Input:           none
 *
 * Output:          none.
 *
 * Returns:         TRUE if the system is busy receiving data on the
 *                  USB, FALSE if not.
 *
 * Side Effects:    none
 *
 * Overview:        This function determines if the CDC class is currently
 *                  busy receiving data on the USB.
 *
 * Notes:           none
 *************************************************************************/

PUBLIC inline BOOL USBUSARTRxIsBusy ( void )
{
    if ( !(gCdcSer.flags & CDC_FLAGS_INITIALIZED) ) {
        return FALSE;
    }

    if (gCdcSer.flags & CDC_FLAGS_RX_BUSY)

        return TRUE;

    else

        return FALSE;

} // USBUSARTRxIsBusy


/*************************************************************************
 * Function:        USBUSARTRxGetLength
 *
 * Precondition:    System has been enumerated as a CDC Serial emulation
 *                  device on the USB and the CDC function driver has been
 *                  initialized.
 *
 * Input:           none
 *
 * Output:          none.
 *
 * Returns:         The current number of bytes available in the caller's 
 *                  buffer.
 *
 * Side Effects:    none
 *
 * Overview:        This function provides the current number of bytes 
 *                  that have been received from the USB and placed into 
 *                  the caller's buffer.
 *
 * Notes:           none
 *************************************************************************/

PUBLIC inline BYTE USBUSARTRxGetLength ( void )
{
    if ( !(gCdcSer.flags & CDC_FLAGS_INITIALIZED) ) {
        return 0;
    }

    return gCdcSer.rx_size;

} // USBUSARTRxGetLength


/*************************************************************************
 * Function:        USBUSARTTx
 *
 * Precondition:    System has been enumerated as a CDC Serial emulation
 *                  device on the USB and the CDC function driver has been
 *                  initialized.
 *
 *                  USBUSARTTxIsReady must have returned TRUE.
 *
 * Input:           pData   Pointer to the data to transmit.
 *
 *                  len     Length of the data, in bytes.
 *
 * Output:          none
 *
 * Returns:         none
 *
 * Side Effects:    The USB Tx transfer will have been started.
 *
 * Overview:        This function starts a USB Tx Transfer on the CDC data
 *                  IN endpoint.
 *
 * Notes:           This function is non-blocking, so the transfer will 
 *                  only have been started, not completed.  The caller 
 *                  will have to monitor USBUSARTTxIsReady or 
 *                  USBUSARTTxIsBusy to determine when the transfer has 
 *                  completed.
 *************************************************************************/

PUBLIC void USBUSARTTx( BYTE *pData, BYTE len )
{
    // Abort if not initialized.
    if ( !(gCdcSer.flags & CDC_FLAGS_INITIALIZED) ) {
        return;
    }


    // If there's not currently a Tx transfer in progress
    if ( !(gCdcSer.flags & CDC_FLAGS_TX_BUSY) )
    {    
        // Mark Tx as busy
        gCdcSer.flags |= CDC_FLAGS_TX_BUSY;

        // Call the device layer to start the data transfer.
        USBDEVTransferData(XFLAGS(CDC_BULK_EP_NUM|USB_TRANSMIT), pData, (unsigned int)len);    
    }
    
} // USBUSARTTx


/*************************************************************************
 * Function:        USBUSARTRx
 *
 * Precondition:    System has been enumerated as a CDC Serial emulation
 *                  device on the USB and the CDC function driver has been
 *                  initialized.
 *
 *                  USBUSARTRxIsReady must have returned TRUE.
 *
 *                  The length of the buffer specified must be at least as
 *                  large as the size specified in the Data OUT endpoint
 *                  descriptor and configuration structures.
 *
 * Input:           pData   Pointer to the buffer in which to receive data.
 *
 *                  len     Length of the buffer, in bytes.
 *
 * Output:          none
 *
 * Returns:         none
 *
 * Side Effects:    The CDC will become ready to receive data into the 
 *                  caller's buffer from the serial Data In endpoint.
 *
 * Overview:        This function sets the CDC Data-RX (USB OUT) endpoint's
 *                  buffer to the one specified by the caller and starts a 
 *                  USB Rx Transfer on the CDC data OUT endpoint.
 *
 * Notes:           This function is non-blocking, so the transfer will 
 *                  only have been started, not completed.  The caller 
 *                  will have to monitor USBUSARTRxIsReady or
 *                  USBUSARTRxIsBusy to determine when the transfer has 
 *                  completed.
 *************************************************************************/

PUBLIC void USBUSARTRx( BYTE *pData, BYTE len )
{
    // Abort if not initialized.
    if ( !(gCdcSer.flags & CDC_FLAGS_INITIALIZED) ) {
        return;
    }

    // If the Rx is not busy...
    if ( !(gCdcSer.flags & CDC_FLAGS_RX_BUSY) )
    {    
        // Set busy flag & clear the old size.
        gCdcSer.flags   |= CDC_FLAGS_RX_BUSY;
        gCdcSer.rx_size  = 0;

        // Call the device layer to start the data transfer.
        USBDEVTransferData(XFLAGS(CDC_BULK_EP_NUM|USB_RECEIVE), pData, (unsigned int)len);
    }

} // USBUSARTRx


/******************************************************************************
 * Function:        USBUSARTGets
 *
 * Precondition:    System has been enumerated as a CDC Serial emulation
 *                  device on the USB and the CDC function driver has been
 *                  initialized.
 *
 * Input:           len     : The number of bytes expected.
 *
 * Output:          buffer  : Pointer to where received bytes are to be stored
 *
 * Returns:         The number of bytes copied to buffer.
 *
 * Side Effects:    An internal state variable that maintains the size of data
 *                  copied into the caller's buffer may have been updated. 
 *                  the caller may obtain this value by calling 
 *                  USBUSARTRxGetLength.
 *
 * Overview:        USBUSARTGets copies a string of bytes received through
 *                  USB CDC Bulk OUT endpoint to a user's specified location. 
 *
 * Note:            It is a non-blocking function. It does not wait
 *                  for data if there is no data available. Instead it returns
 *                  '0' to notify the caller that there is no data available.
 *****************************************************************************/
PUBLIC BYTE USBUSARTGets ( char *buffer, BYTE len )
{    
    if ( !(gCdcSer.flags & CDC_FLAGS_INITIALIZED) ) {
        return 0;
    }

    // If not busy, start the transfer
    if ( !(gCdcSer.flags & CDC_FLAGS_RX_BUSY) )
    {
        USBUSARTRx(buffer, len);    // Clears rx_size & sets CDC_FLAGS_RX_BUSY
    }

    // Report current size
    return gCdcSer.rx_size;

} // USBUSARTGets


/******************************************************************************
 * Function:        USBUSARTPuts
 *
 * Precondition:    System has been enumerated as a CDC Serial emulation
 *                  device on the USB and the CDC function driver has been
 *                  initialized.
 *
 *                  USBUSARTTxIsReady must have returned TRUE.
 *
 * Input:           data    : Pointer to a null-terminated string of data.
 *                            If a null character is not found, 255 bytes
 *                            of data will be transferred to the host.
 *
 * Output:          none
 *
 * Side Effects:    A USB IN (Tx) transfer has been started.
 *
 * Overview:        Writes a string of data to the USB Data IN endpoint, 
 *                  including the null character.
 *
 * Note:           none
 *****************************************************************************/

PUBLIC inline void USBUSARTPuts ( char *data )
{
    if ( !(gCdcSer.flags & CDC_FLAGS_INITIALIZED) ) {
        return;
    }

    // If there's not currently a Tx transfer in progress
    if ( !(gCdcSer.flags & CDC_FLAGS_TX_BUSY) )
    {
        USBUSARTTx(data, strlen(data)+1);
    }

} // USBUSARTPuts


/******************************************************************************
 * Function:        USBUSARTGetLineCoding
 *
 * Precondition:    System has been enumerated as a CDC Serial emulation
 *                  device on the USB and the CDC function driver has been
 *                  initialized.
 *
 * Input:           none
 *
 * Output:          pLC     : Pointer to a LINE_CODING data structure to
 *                            receive the current line coding data.
 *
 * Returns:         TRUE if successful, FALSE if the line-coding data was in
 *                  the process of being updated.
 *
 * Side Effects:    none
 *
 * Overview:        This function provides the current line-coding (BPS, 
 *                  # stop bits, parity, & data bits-per-word) information.
 *
 * Note:            If the line coding data is currently being updated by the
 *                  host, this function will return FALSE and the caller 
 *                  should retry it later.
 *****************************************************************************/

PUBLIC BOOL USBUSARTGetLineCoding ( LINE_CODING *pLC )
{
    if ( !(gCdcSer.flags & CDC_FLAGS_INITIALIZED) ) {
        return FALSE;
    }

    // Make sure we're not currently updating line control
    if (gCdcSer.flags & CDC_FLAGS_LINE_CTRL_BUSY) {
        return FALSE;
    }

    // Copy the line coding data to the user's buffer
    memcpy(pLC, &gCdcSer.line_coding, sizeof(LINE_CODING));

    return TRUE;

} // USBUSARTGetLineCoding


/******************************************************************************
 * Function:        USBUSARTGetCmdStr
 *
 * Precondition:    System has been enumerated as a CDC Serial emulation
 *                  device on the USB and the CDC function driver has been
 *                  initialized.
 *
 *                  The client has received a EVT_CDC_CMD event notification in
 *                  response to the host sending a SEND_ENCAPSULATED_COMMAND
 *                  request.
 *
 * Input:           len     : The number of bytes expected
 *
 * Output:          buffer  : Pointer to where received bytes are to be stored
 *
 * Returns:         TRUE if able to start the transfer, FALSE if not.
 *
 * Side Effects:    The next len bytes read from EP0 will be read into the 
 *                  caller's buffer.
 *
 * Overview:        This routine retrieves a len byte command string from 
 *                  EP0 into the caller's buffer.  
 *
 * Note:            It should only ever be called when a EVT_CDC_CMD event 
 *                  notification has been received or the caller may interfere
 *                  with EP0 request processing.
 *****************************************************************************/
PUBLIC BOOL USBUSARTGetCmdStr ( char *buffer, BYTE len )
{    
    if ( !(gCdcSer.flags & CDC_FLAGS_INITIALIZED) ) {
        return 0;
    }

    // Make sure we're not currently busy receiving a command string
    if ( !(gCdcSer.flags & CDC_FLAGS_GET_CMD_BUSY) )
    {
        // Set cmd busy flag.
        gCdcSer.flags    |= CDC_FLAGS_GET_CMD_BUSY;

        // Call the device layer to start the data transfer.
        return USBDEVTransferData(XFLAGS(USB_EP0|USB_RECEIVE), buffer, (unsigned int)len);
    }

    return FALSE;

} // USBUSARTGetCmdStr


/******************************************************************************
 * Function:        USBUSARTSendRespStr
 *
 * Precondition:    System has been enumerated as a CDC Serial emulation
 *                  device on the USB and the CDC function driver has been
 *                  initialized.
 *
 *                  The client musthave received a EVT_CDC_RESP event in 
 *                  response the host sending a GET_ENCAPSULATED_RESPONSE 
 *                  request must have been received.
 *
 * Input:           data    : Pointer to a null-terminated string of response
 *                            data.
 *
 * Output:          none
 *
 * Side Effects:    A USB IN (Tx) transfer has been started on EP0.
 *
 * Overview:        Writes a string of data to USB EP0, in response to a 
 *                  GET_ENCAPSULATED_RESPONSE request from the host.
 *
 * Note:            This routine should only be called once per 
 *                  GET_ENCAPSULATED_RESPONSE request.
 *
 *                  We do not care when the response transfer finishes, since
 *                  we will find out when the host sends another request 
 *                  packet, so there is no way for the caller to determine 
 *                  this.  
 *****************************************************************************/

PUBLIC BOOL USBUSARTSendRespStr ( char *data )
{
    if ( !(gCdcSer.flags & CDC_FLAGS_INITIALIZED) ) {
        return FALSE;
    }

    // Make sure we're not currently sending a response.
    if (gCdcSer.flags & CDC_FLAGS_SEND_RESP_BUSY)
    {
        // Set the send-response-busy flag
        gCdcSer.flags |= CDC_FLAGS_SEND_RESP_BUSY;

        // Call the device layer to start the data transfer.
        USBDEVTransferData(XFLAGS(USB_EP0|USB_TRANSMIT), data, strlen(data));

        return TRUE;
    }

    return FALSE;


} // USBUSARTSendRespStr


/******************************************************************************
 * Function:        USBUSARTSendNotification
 *
 * Precondition:    System has been enumerated as a CDC Abstract Control Model
 *                  device on the USB and the CDC function driver has been
 *                  initialized.
 *
 * Input:           notification    Pointer to the notification data to the host.
 *
 * Output:          none
 *
 * Side Effects:    A USB IN (Tx) transfer has been started on the notification
 *                  endpoint with the given data packet.
 *
 * Overview:        Sends a notification packet to the host through the
 *                  notification endpoint.
 *
 * Note:            This routine should only be called once per 
 *                  SEND_ENCAPSULATED_COMMAND request and then only when the 
 *                  response has been prepared.
 *
 *                  We do not care when the response transfer finishes, 
 *                  so there is no way for the caller to determine this..
 *****************************************************************************/

PUBLIC BOOL USBUSARTSendNotification ( CDC_NOTIFICATION *notification )
{
    if ( !(gCdcSer.flags & CDC_FLAGS_INITIALIZED) ) {
        return FALSE;
    }

    // Make sure we're not currently transferring a previous notification.
    if ( !(gCdcSer.flags & CDC_FLAGS_NOTIFY_BUSY) )
    {
        // Set busy flag
        gCdcSer.flags |= CDC_FLAGS_NOTIFY_BUSY;

        // Call the device layer to start the data transfer.
        USBDEVTransferData(XFLAGS(CDC_INT_EP_NUM|USB_TRANSMIT), notification, 
                           sizeof(CDC_NOTIFICATION));

        return TRUE;
    }

    return FALSE;


} // USBUSARTSendNotification


/** EOF usb_cdcserial.c ****************************************************************/
