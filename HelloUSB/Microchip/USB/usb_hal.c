/******************************************************************************

    USB Hardware Abstraction Layer
    
This file implements the USB Hardware Abstraction Layer.

 * Filename:        usb_hal.c
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
BC           10-11-2006 Initial Creation
BC           04-18-2008 Merged core into HAL

*******************************************************************************/
 
#include <string.h>

#include "plib.h"
#include "GenericTypeDefs.h"
#include "USB/usb.h"
#include "usb_hal_local.h"


/* Global Data
 *************************************************************************
 * Data necessary to manage the USB HAL.
 */
 
USB_HAL_DATA gHALData;              // Main HAL Data Structure
unsigned int attach_counter;        // Attach debounce counter

/* Buffer Descriptor Table */
BUF_DESC __attribute__ ((aligned(512))) g_BDT[USB_DEV_HIGHEST_EP_NUMBER+1][2][2];
//
// Index as: g_BDT[endpoint_number][direction][ping_pong]
//
// endpoint_number: 0-USB_DEV_HIGHEST_EP_NUMBER
// direction:       0=Receive, 1=Transmit
// ping_pong:       0=Even,    1=Odd

#define FindDescriptor(e,d,p) &g_BDT[(e)][(d)][(p)]


/******************
 * Local Routines *
 ******************/

/******************************************************************************
 * Function:        SetupBDTP
 *
 * PreCondition:    None
 *
 * Input:           Physical address of the BDT
 *                  
 * Output:          None
 *
 * Side Effects:    The BDT base address has been initialized in the USB 
 *                  OTG Core.
 *
 * Overview:        Internal function to setup the USB Buffer Descriptor
 *                  Table registers, UBDTP1, UBDTP2, and UBDTP3.
 *
 * Note:            Inline local function. 
 *****************************************************************************/
static inline void SetupBDTP( UINT32 BTD_phys_addr )
{
   DWORD_VAL pa;
   
   pa.Val = BTD_phys_addr;

   U1BDTP1 = pa.v[1];
   U1BDTP2 = pa.v[2];
   U1BDTP3 = pa.v[3];

} // SetupBDTP()


/* NotifyHigherLayerOfEvent
 *************************************************************************
 * This function notifies the appropriate higher SW layer (host or device,
 * depending on configuration and/or current role of the controller) of 
 * bus event.
 */

LOCAL_INLINE BOOL NotifyHigherLayerOfEvent ( USB_EVENT    event, 
                                               void        *data, 
                                               int          size   )
{

    #if defined( USB_SUPPORT_DEVICE )
    
        #if defined( USB_SUPPORT_HOST )
        
            #if defined( USB_SUPORT_OTG )
            
                // True USB OTG Device
                #error "USB OTG is not yet supported."
                
            #else
            
                // Identify current role & notify appropriate layer
                if (gHALData.current_role == HOST)
                {
                    if (USB_HOST_EVENT_HANDLER != NULL) {
                        return USB_HOST_EVENT_HANDLER(event, data, size);
                    } else {
                        return FALSE;
                    }
                }
                else
                {
                    if (USB_DEV_EVENT_HANDLER != NULL) {
                        return USB_DEV_EVENT_HANDLER(event, data, size);
                    } else {
                        return FALSE;
                    }
                }
                        
            #endif
            
        #else
        
            // Notify device-layer
            if (USB_DEV_EVENT_HANDLER != NULL) {
                return USB_DEV_EVENT_HANDLER(event, data, size);
            } else {
                return FALSE;
            }
            
        #endif
        
    #else
    
        #if defined( USB_SUPPORT_HOST )
        
            // Notify host layer
            if (USB_HOST_EVENT_HANDLER != NULL) {
                return USB_HOST_EVENT_HANDLER(event, data, size);
            } else {
                return FALSE;
            }
            
        #else
        
                #error "Application must define support mode in usb_config.h"
                
        #endif
        
    #endif
    
    
} // NotifyHigherLayerOfEvent


/******************************************************************************
 * Function:        USBHALIdentifyPacket
 *
 * PreCondition:    Assumes that an UIR_TOK_DNE interrupt has occured.
 *
 * Input:           None 
 *
 * Output:          flags   Bitmapped flags identifying transfer (see below):
 *
 *                          1 1 1 1 1 1
 *                          5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *                          | | \_____/ \_/ \_____/ | | \_/
 *                          | |    |     |     |    | |  +-- reserved1
 *                          | |    |     |     |    | +----- ping_pong
 *                          | |    |     |     |    +------- direction
 *                          | |    |     |     +------------ ep_num
 *                          | |    |     +------------------ reserved2
 *                          | |    +------------------------ pid
 *                          | +----------------------------- data_toggle
 *                          +------------------------------- reserved3
 *
 *                  size    Size of data actually transferred (in bytes).
 *
 * Returns:         TRUE if successful, FALSE any of the parameters were NULL.
 *
 * Side Effects:    None
 *
 * Overview:        This provides the endpoint number, direction and ping-pong
 *                  ID and other information identifying the packet that was 
 *                  just completed.
 ******************************************************************************/

LOCAL_INLINE BOOL USBHALIdentifyPacket( TRANSFER_ID_FLAGS *flags,
                                        UINT16            *size)
{
    pBUF_DESC     desc; // Pointer to 1st descriptor in BDT


    #ifdef USB_SAFE_MODE
    // Validate parameters.
    if (flags == NULL || size == NULL) {
        return FALSE;
    }
    #endif

    // Grab the EP#, direction & ping pong data from the USB Status register.
    flags->byte[0] = (BYTE)U1STAT;

    // Index into the BDT and get the desired descriptor
    desc = &g_BDT[flags->field.ep_num][flags->field.direction][flags->field.ping_pong];
    if (desc->setup.UOWN) {
        return FALSE;       // We don't own the descriptor right now, don't touch it!
    }

    // Grab the PID & Data Toggle from the descriptor
    flags->byte[1] = desc->byte[0];
    
    // Grab the size of the packet from the descriptor
    *size = desc->byte_cnt.BC;
    
    // Clear the TXD_SUSPEND bit for setup transfers
    if (desc->setup.TOK_PID == 0xD)
    {
	    U1CON &= ~UCTRL_TXD_SUSPND;
    }

    return TRUE;

} // USBHALIdentifyPacket


/******************************************************************************
 * Function:        USBHALStartPacket
 *
 * PreCondition:    Assumes USBHALDeviceEnable has been called to initialize
 *                  the BDT and that USBHALConfigureDescriptor has been 
 *                  called on the given descriptor.
 *
 * Input:           flags   Transfer flags giving the endpoint number and 
 *                          direction.
 *
 *                  p_Pipe  Pointer to the HAL pipe data structure maintaining
 *                          state for the transfer.
 *                  
 * Output:          None
 *
 * Returns:         TRUE if successful, FALSE if not.
 *
 * Side Effects:    The descriptor for the given buffer has been initialized
 *                  and passed to the HW and the pipe has been updated to
 *                  account for the transfer.
 *
 * Overview:        This function initializes the data buffer address and size
 *                  parameters in the DMA descriptor for the endpoint, direction
 *                  and buffer (even or odd) given.  It then, passes control of
 *                  the descriptor and buffer off to the HW for data transfer
 *                  on the USB.
 ******************************************************************************/

static BOOL USBHALStartPacket (TRANSFER_FLAGS flags, PUSB_HAL_PIPE p_Pipe)
{
    pBUF_DESC desc;
    WORD      bdt_flags;


    // Range check the BDT indices
    #ifdef USB_SAFE_MODE
    if (flags.field.ep_num > USB_DEV_HIGHEST_EP_NUMBER) {
        return FALSE;
    }
    #endif
    
    // Index into the desired descriptor.
    desc = FindDescriptor(flags.field.ep_num, flags.field.direction, p_Pipe->flags.field.ping_pong);
    if (desc->setup.Val & USBHAL_DESC_UOWN) {
        return FALSE; // We don't own the descriptor right now, don't touch it!
    }

    // Set the buffer address.
    desc->addr = (DATA_PTR_SIZE)KVA_TO_PA((UINT32)p_Pipe->buffer);

    // Choose the size of the packet
    if (p_Pipe->remaining > p_Pipe->max_pkt_size)
    {
        // Set the DMA descriptor size
        desc->byte_cnt.BC  = p_Pipe->max_pkt_size;
        
        // Advance the pipe
        p_Pipe->buffer    += p_Pipe->max_pkt_size;
        p_Pipe->remaining -= p_Pipe->max_pkt_size;
    }
    else
    {
        // Set the DMA descriptor size
        desc->byte_cnt.BC  = p_Pipe->remaining;
        
        // Advance the pipe
        p_Pipe->buffer    += p_Pipe->remaining;
        p_Pipe->remaining  = 0;
    }

    // Determing the new BDT flags value
    bdt_flags = USBHAL_DESC_UOWN|USBHAL_DESC_DTS|(p_Pipe->flags.field.data_toggle << 6);

    // Update the data toggle & hand off the descriptor (& buffer) to the HW.
    desc->setup.Val = bdt_flags;

    // Update pipe's ping-pong & data toggle.
    p_Pipe->flags.field.ping_pong   ^= 1;
    p_Pipe->flags.field.data_toggle ^= 1;

    return TRUE;

} // USBHALStartPacket


/*************************************
 * Interrupt Service Routine Support *
 *************************************/

/* ServiceEndpoint
 *************************************************************************
 * This routine services an endpoint when a token done interrupt occurs.
 * It identifies which endpoint and buffer needs servicing, calls the 
 * higher layer callback or request handler if the transfer is done, 
 * or schedules the next DMA transaction if the transfer is incomplete.
 */

LOCAL_INLINE void ServiceEndpoint ( void )
{
    TRANSFER_ID_FLAGS       pkt_id;         // Packet ID data.
    PUSB_HAL_PIPE           p_Pipe;         // Pointer to transfer pipe data
    unsigned int            new_xfer_size;  // Size of new transfer (if any)
    USB_TRANSFER_EVENT_DATA xfer_data;      // Data passed to XFer event notice
    UINT16                  pkt_size;       // Number of bytes of data in this packet


    // Identify the transfer that just completed.
    if (!USBHALIdentifyPacket(&pkt_id, &pkt_size)) {
        gHALData.last_error |= USBHAL_XFER_ID;
        NotifyHigherLayerOfEvent(EVENT_BUS_ERROR, NULL, 0);
        return; // Error!
    }

    // Find the transfer pipe data.
    p_Pipe = FindPipe(pkt_id.field.ep_num, pkt_id.field.direction);

    #ifdef USB_SAFE_MODE
    // Did it exist?
    if (p_Pipe == NULL) {
        gHALData.last_error |= USBHAL_NO_EP;
        NotifyHigherLayerOfEvent(EVENT_BUS_ERROR, NULL, 0);
        return; // Error!
    }
    #endif
    
    // Accumulate the count of actual data transferred.
    p_Pipe->count += pkt_size;
    
    // If it's a setup packet...
    if (pkt_id.field.pid == 0xD)
    {
        // Release the pipe. 
        p_Pipe->buffer = NULL;

        // Call the device layer's setup handler directly.
        HandleDeviceRequest();
        return;
    }
    
    // Assemble transfer flags for endpoint number and direction
    xfer_data.flags.field.ep_num    = pkt_id.field.ep_num;
    xfer_data.flags.field.direction = pkt_id.field.direction;

    // We're done if there's no data remaining or we've received a short packet.
    if ( (p_Pipe->count == p_Pipe->size) || (pkt_size < p_Pipe->max_pkt_size) )
	{
        // Unless we need a zero-sized packet to finish.
        if (p_Pipe->flags.field.send_0_pkt)
        {
            p_Pipe->flags.field.send_0_pkt = 0;
        }
        else
        {
            // Assemble the rest of the transfer data
            xfer_data.flags.field.dts = pkt_id.field.data_toggle;
            xfer_data.pid             = pkt_id.field.pid;
            xfer_data.size            = p_Pipe->count;

            // Null out the buffer pointer to make this pipe available
            p_Pipe->buffer = NULL;

            // Notify higher-level layer.
            NotifyHigherLayerOfEvent(EVENT_TRANSFER, &xfer_data, sizeof(xfer_data));

            // Exit
            return;
        }
    }

    // Do we need to schedule a zero-sized packet next time?
    if (  p_Pipe->flags.field.zero_pkt && 
         (p_Pipe->remaining == p_Pipe->max_pkt_size) )
    {
        p_Pipe->flags.field.send_0_pkt = 1;
    }
    
    // Start the next transfer, if necessary:
    if ( (p_Pipe->remaining > 0) || p_Pipe->flags.field.send_0_pkt )
    {
        if (!USBHALStartPacket (xfer_data.flags, p_Pipe)){
            gHALData.last_error |= USBHAL_DMA_ERR2;
            NotifyHigherLayerOfEvent(EVENT_BUS_ERROR, NULL, 0);
            return; // Error!
        }    
    }

    // Done!
    return;

} // ServiceEndpoint


/* SOFHandler
 ******************************************************************************
 * Internal function to handle USB Start-Of-Frame Token interrupt.
 */

LOCAL_INLINE void SOFHandler( void )
{
    // Notify the host or device layer (as appropriate).
    NotifyHigherLayerOfEvent(EVENT_SOF, NULL, 0);

}   // SOFHandler


/* UsbResume
 ******************************************************************************
 * Internal function to handle USB resume interrupt.
 */
LOCAL_INLINE void UsbResume( void )
{
    // Notify the host or device layer (as appropriate).
    NotifyHigherLayerOfEvent(EVENT_RESUME, NULL, 0);

}   // UsbResume


/* UsbReset
 ******************************************************************************
 * Internal function to handle USB reset interrupt.
 */

LOCAL_INLINE void UsbReset( void )
{
    // Notify the host or device layer (as appropriate).
    NotifyHigherLayerOfEvent(EVENT_RESET, NULL, 0);

}   // UsbReset


/* ErrorHandler
 ******************************************************************************
 * Internal function to handle USB Error Condition interrupt.
 */

LOCAL_INLINE void ErrorHandler( void )
{
    UINT32 error_status;

    // Get the error status
    error_status = ERROR_MASK & USBHALGetErrors();

    // Record the error.
    gHALData.last_error |= error_status;

    // Clear/Acknowledge the individual USB Error 
    USBHALClearErrors(error_status);

    // Notify higher layer.
    NotifyHigherLayerOfEvent(EVENT_BUS_ERROR, NULL, 0);
    // Unfortunately, we can't identify the endpoint to stall it, so we're
    // pretty limited on how we can handle errors.  We will probably have to
    // reset the controller on any error.  That decision is left to the 
    // higher-layer of SW.
   
}   // ErrorHandler


/* UsbSuspend
 ******************************************************************************
 * Internal function to handle USB suspend interrupt.
 */

LOCAL_INLINE void UsbSuspend( void )
{

    // Call out to device module to prepare it to enter suspend state.
    NotifyHigherLayerOfEvent(EVENT_SUSPEND, NULL, 0);

}   // UsbSuspend


/* DetachHandler
 ******************************************************************************
 * Internal function to handle USB cable detachment.
 */

LOCAL_INLINE void DetachHandler( void )
{
    // Detach from the bus
    U1CONbits.USBEN = 0;

    // Notify the host or device layer (as appropriate).
    NotifyHigherLayerOfEvent(EVENT_DETACH, NULL, 0);

}   // DetachHandler


/* AttachHandler
 ******************************************************************************
 * Internal function to handle USB Attach interrupt.
 */
LOCAL_INLINE void AttachHandler( void )
{
    // Attach to the bus
    U1CONbits.USBEN = 1;

    // Notify the host or device layer (as appropriate).
    NotifyHigherLayerOfEvent(EVENT_ATTACH, NULL, 0);

}   // AttachHandler


/* StallHandler
 ******************************************************************************
 * Internal function to handle USB Stall Handshake interrupt.
 */

LOCAL_INLINE void StallHandler( void )
{
    // Notify the host or device layer (as appropriate).
    NotifyHigherLayerOfEvent(EVENT_STALL, NULL, 0);

}   // StallHandler


/*************************************************************************
 * Function:        USBHALHandleBusEvent
 *
 * Precondition:    USBInitialize must have been called to initialize the 
 *                  USB SW stack.
 *
 * Input:           none
 *
 * Output:          none
 *
 * Returns:         none
 *
 * Side Effects:    Depend on the event that may have occured.
 *
 * Overview:        This routine checks the USB for any events that may
 *                  have occured and handles them appropriately.  It may
 *                  be called directly to poll the USB and handle events 
 *                  or it may be called in response to an interrupt.
 *
 * Note:            This routine is the core of the USB HAL state machine.
 *************************************************************************/

PUBLIC void USBHALHandleBusEvent ( void )
{
    UINT32 status;

    //
    // Note: Order of checking events defines an implicit priority.
    //

    // Watch VBus for an attach.
    if (attach_counter < USB_DEVICE_ATTACH_DEBOUNCE_TIME) {
        if ( USBHALSessionIsValid() ) {
            if (U1OTGIRbits.T1MSECIF) {
                U1OTGIRbits.T1MSECIF = 1;
                if (++attach_counter >= USB_DEVICE_ATTACH_DEBOUNCE_TIME)
                    AttachHandler();
            }
        }
    }

    // Watch VBus for a detach.
    if (!USBHALSessionIsValid()) {
        if (attach_counter >= USB_DEVICE_ATTACH_DEBOUNCE_TIME)
            DetachHandler();
        attach_counter = 0;
    }


    // Get status of enabled interrupts.
    if( (status = STATUS_MASK & USBHALGetStatus()) == 0 )
    {
       return;  // Early exit when no state change.
    }

    // Service USB Start-Of-Frame Token Interrupt
    #if defined(USB_DEVICE_ENABLE_SOF_EVENTS)
        if( status & UIR_SOF_TOK ) 
        {
            SOFHandler();
        }
    #endif

    // Service USB Token Processing Complete Interrupt
    if( status & UIR_TOK_DNE ) 
    {
        ServiceEndpoint();
    }

    // Service USB Bus Reset Interrupt
    if( status & UIR_USB_RST ) 
    {
        UsbReset();
    }

    // Service USB Error Condition Interrupt
    if( status & UIR_UERR ) 
    {
        ErrorHandler();
    }
 
    // Service USB Idle Interrupt
    if( status & UIR_UIDLE ) 
    {
        UsbSuspend();
    }

    // Service USB Resume Interrupt
    if( status & UIR_RESUME ) 
    {
        UsbResume();
    }

    // Service USB Stall Handshake Interrupt
    if( status & UIR_STALL ) 
    {
        StallHandler();
    }


    // Clear the interrupt status
    USBHALClearStatus(status);


} // USBHALHandleBusEvent


/********************************
 * HAL Interface Implementation *
 ********************************/

/*
 * Note: USBHALHandleBusEvent is defined above in the Interrupt Service 
 * Routine Support section.
 */

/*************************************************************************
 Function:        USBHALSetBusAddress

 Preconditions:   1. USBHALInitialize must have been called to
                     initialize the USB HAL.
                  2. Endpoint zero (0) must be configured as appropriate
                     by calls to USBHALSetEpConfiguration.
                  3. The system must have been enumerated on the USB (as
                     a device).

 Input:           addr    Desired address of this device on the USB.

 Output:          none

 Returns:         none

 Side Effects:    The bus address has been set.

 Overview:        This routine sets the address of the system on the USB
                  when acting as a peripheral device.

 Notes:           The address is assigned by the host and is received in
                  a SET_ADDRESS setup request.
 *************************************************************************/
void USBHALSetBusAddress( BYTE addr )
{
    U1ADDR = (U1ADDR & ~0xFF) | (addr & 0x7F);
}


/*************************************************************************
 Function:        USBHALStallPipe

 Preconditions:   USBHALInitialize must have been called to initialize
                  the USB HAL.

 Input:           pipe    Uses the TRANSFER_FLAGS (see USBCommon.h) format to
                          identify the endpoint and direction making up the
                          pipe to stall.

                          Note: Only ep_num and direction fields are
                          required.

 Output:          None

 Returns:         TRUE if able to stall endpoint, FALSE if not.

 Side Effects:    The endpoint will stall if additional data transfer is
                  attempted.

 Side Effects:    Given endpoint has been stalled.

 Overview:        This routine stalls the given endpoint.

 Notes:           Starting another data transfer automatically
                  "un-stalls" the endpoint.
 *************************************************************************/

PROTECTED BOOL USBHALStallPipe( TRANSFER_FLAGS pipe )
{
    pBUF_DESC desc;
    BOOL	  success = FALSE;

    // Range check the BDT index
    #ifdef USB_SAFE_MODE
    if (pipe.field.ep_num > USB_DEV_HIGHEST_EP_NUMBER) {
        return FALSE;
    }
    #endif

    // Index into the even descriptor of the desired endpoint.
    desc = FindDescriptor(pipe.field.ep_num, pipe.field.direction, 0);

    // If software owns the descriptor, set the stall bit.
    if ( !(desc->word[0] & USBHAL_DESC_UOWN) ) {
        desc->word[0] = USBHAL_DESC_UOWN|USBHAL_DESC_BSTALL;
        success = TRUE;
    }

    // Advance to the odd descriptor.
    desc++;

    // If software owns the descriptor, set the stall bit.
    if ( !(desc->word[0] & USBHAL_DESC_UOWN) ) {
        desc->word[0] = USBHAL_DESC_UOWN|USBHAL_DESC_BSTALL;
        success = TRUE;
    }

    // Fails if we were unable to stall either descriptor.
    return success;

}   // USBHALStallPipe


/******************************************************************************
 * Function:        USBHALEndpointHasBeenStalled
 *
 Preconditions:     USBHALInitialize must have been called to initialize
                    the USB HAL.
 *
 * Input:           endpoint - Number and direction of the endpoint to check.
 *                  
 * Output:          None
 *
 * Returns:         TRUE  if the endpoint has been stalled,
 *                  FALSE if not.
 *
 * Side Effects:    None
 *
 * Overview:        This routine checks to see if the given endpoint has been
 *                  stalled.
 *
 * Note:            This routine will report a stall if the endpoint has been 
 *                  prepared to be stalled, even if the actual stall condition
 *                  was not actually triggered (by a packet being sent to the 
 *                  endpoint).  
 ******************************************************************************/

PROTECTED BOOL USBHALEndpointHasBeenStalled ( TRANSFER_FLAGS endpoint )
{
    pBUF_DESC desc;
    BOOL	  stalled = FALSE;

    // Range check the BDT index
    #ifdef USB_SAFE_MODE
    if (endpoint.field.ep_num > USB_DEV_HIGHEST_EP_NUMBER) {
        return FALSE;
    }
    #endif

    // Index into the even descriptor of the desired endpoint.
    desc = FindDescriptor(endpoint.field.ep_num, endpoint.field.direction, 0);

    // Has the stall bit been set?
    if ( desc->word[0] & USBHAL_DESC_BSTALL ) {
        stalled = TRUE;
    }

    // Advance to the odd descriptor.
    desc++;

    // Has the stall bit been set?
    if ( desc->word[0] & USBHAL_DESC_BSTALL ) {
        stalled = TRUE;
    }

    // If either stall bit was set, the endpoint has been stalled
    return stalled;

} // USBHALEndpointHasBeenStalled


/******************************************************************************
 Function:        USBHALUnstallPipe

 Preconditions:   USBHALInitialize must have been called to initialize
                  the USB HAL and USBHALStallPipe has been called on the 
                  given pipe.

 Input:           pipe    Uses the TRANSFER_FLAGS (see USBCommon.h) format to
                          identify the endpoint and direction making up the
                          pipe to unstall.

 Output:          None

 Returns:         TRUE if able to stall the pipe, FALSE if not.

 Side Effects:    The BSTALL and UOWN bits (and all other control bits) in
                  the BDT for the given pipe will be cleared.

 Overview:        This routine clears the stall condition for the given pipe.

 *****************************************************************************/

PROTECTED BOOL USBHALUnstallPipe( TRANSFER_FLAGS pipe )
{
    pBUF_DESC desc;

    // Range check the BDT index
    #ifdef USB_SAFE_MODE
    if (pipe.field.ep_num > USB_DEV_HIGHEST_EP_NUMBER) {
        return FALSE;
    }
    #endif

    // Index into the even descriptor of the desired endpoint.
    desc = FindDescriptor(pipe.field.ep_num, pipe.field.direction, 0);

    // Clear the control bits in both BDT entries for the given pipe.
    desc->word[0] = 0;
    desc++;
    desc->word[0] = 0;

    return TRUE;

}   // USBHALUnstallPipe


/*************************************************************************
 Function:        USBHALControlUsbResistors

 Precondition:    USBInitialize must have been called to initialize the
                  USB SW stack.

 Input:           flags   This is a bit-mapped flags value indicating
                          which resistors to enable or disable (see
                          below).

 Output:          none

 Returns:         TRUE if successful, FALSE if not.

 Side Effects:    The resistors are enabled as requested.

 Overview:        This routine enables or disables the USB pull-up or
                  pull-down resistors as requested.

 Note:            Used for USB peripheral control to connect to or
                  disconnect from the bus.  Otherwise, used for OTG
                  SRP/HNP and host support.
 *************************************************************************/

PROTECTED inline void USBHALControlUsbResistors( BYTE flags )
{
#ifdef USB_SUPPORT_DEVICE
    
    // Enable the USB module (automatically connects as a device)
    U1CON = UCTRL_USB_EN;

#else
    
    BYTE reg_val;

    reg_val  = U1OTGCON & ~RESISTOR_CTRL_MASK;
    reg_val |= (flags & RESISTOR_CTRL_MASK) | UOTGCTRL_OTG_EN;

    U1OTGCON = reg_val;

#endif
    
} // USBHALControlUsbResistors


/*************************************************************************
 Function:        USBHALSessionIsValid

 Precondition:    USBInitialize must have been called to initialize the
                  USB SW stack.

 Input:           none

 Output:          none

 Returns:         TRUE if the session is currently valid, FALSE if not.

 Side Effects:    none

 Overview:        This routine determines if there is currently a valid
                  USB session or not.
 *************************************************************************/

BOOL USBHALSessionIsValid( void )
{
    if (U1OTGSTATbits.SESVD)

        return TRUE;

    else

        return FALSE;
}


/*************************************************************************
 * Function:        USBHALGetLastError
 *
 * Precondition:    USBInitialize must have been called to initialize the 
 *                  USB SW stack.
 *
 * Input:           none
 *
 * Output:          none
 *
 * Returns:         Bitmap indicating the most recent error condition(s).
 *
 * Side Effects:    Error record is cleared.
 *
 * Overview:        This routine provides a bitmap of the most recent
 *                  error conditions to occur.
 *
 * Note:            Although record of the error state is cleared, nothing
 *                  is done to fix the condition or recover from the
 *                  error.  The client must take appropriate steps.
 *************************************************************************/

PUBLIC unsigned long USBHALGetLastError( void )
{
    unsigned long error_status;

    // Clear the error state.
    error_status = gHALData.last_error;
    gHALData.last_error = 0;

    // Return the last error state
    return error_status;

}   // USBHALGetLastError


/*************************************************************************
 * Function:        USBHALTransferData
 *
 * Preconditions:   1. USBHALInitialize must have been called to 
 *                     initialize the USB HAL.  
 *                  2. The endpoint through which the data will be 
 *                     transferred must be configured as appropriate by a 
 *                     call to USBHALSetEpConfiguration.
 *                  3. The bus must have been enumerated (either as a host
 *                     or device).  Except for EP0 transfers.
 *
 * Input:           flags       Flags consists of the endpoint number and
 *                              several flags indicating transfer
 *                              direction and such.
 *
 *                              See USBCommon.h, "Data Transfer Flags"
 *
 *                  buffer      Address of the buffer to receive data.
 *
 *                  size        Number of bytes of data to transfer.
 *
 * Output:          none
 *
 * Returns:         TRUE if the HAL was able to successfully start the
 *                  data transfer, FALSE if not.
 *
 * Side Effects:    The HAL has prepared to transfer the data on the USB.
 *
 * Overview:        This routine prepares to transfer data on the USB.  
 *                  If the system is in device mode, the actual transfer 
 *                  will not occur until the host peforms an OUT request 
 *                  to the given endpoint.  If the system is in host mode,
 *                  the transfer will not start until the token has been 
 *                  sent on the bus.  
 *
 * Note:            The HAL will continue the data transfer, keeping track
 *                  of the buffer address, data remaining, and ping-pong
 *                  buffer details internally when USBHALHandleBusEvent is
 *                  called (either polled or in response to an interrupt).
 *                  The caller will receive notification that the transfer
 *                  has completed when the EVENT_TRANSFER event is passed into 
 *                  the USBHALBusEventCallout call-out function.
 *************************************************************************/

PUBLIC BOOL USBHALTransferData ( TRANSFER_FLAGS    flags, 
                                 void         *buffer, 
                                 unsigned int  size )
{
    PUSB_HAL_PIPE   p_Pipe;                 // Pointer to pipe data


    // Find the pipe data.
    p_Pipe = FindPipe(flags.field.ep_num, flags.field.direction);

    #ifdef USB_SAFE_MODE
        // Did it exist?
        if (p_Pipe == NULL) {
            return FALSE;
        }
    
        // Make sure it's not currently in use.
        if (p_Pipe->buffer != NULL) {
            return FALSE;
        }
    
        // Flush Rx buffers before starting transfer.
        #ifdef FLUSH_RX_BUFFERS
        if (flags.direction == 0) {
            memset(caller_buffer, size, 0);
        }
        #endif
    #endif
    

    // Preserve the flags
    p_Pipe->flags.field.zero_pkt = flags.field.zero_pkt;
    
    if (flags.field.force_dts)
    {
        p_Pipe->flags.field.data_toggle = flags.field.dts;
    }

    // Preserve the transfer data.
    p_Pipe->size      = size;
    p_Pipe->remaining = size;
    p_Pipe->count     = 0;
    p_Pipe->buffer    = (BYTE *)buffer;

    // If we have an exact multiple of 2 packets, then our 3rd may be a zero packet.
    if (size == 2 * p_Pipe->max_pkt_size) {
        p_Pipe->flags.field.send_0_pkt = flags.field.zero_pkt;
    }

    // Do we have enough data to start 2 packets?
    if (size >= p_Pipe->max_pkt_size)
    {
        // If we have exactly 1 packet, only start 2 
        // packets if a zero packet was requested. (Note: The
        // logic below is the DeMorgan inverse of this statement).
        if (flags.field.zero_pkt || size != p_Pipe->max_pkt_size)
        {
            if (!USBHALStartPacket(flags, p_Pipe)) {  // Updates pipe
                return FALSE;
            }
        }
    }

    // Always start at least 1 packet, even if it's zero sized.
    if (!USBHALStartPacket(flags, p_Pipe)) {  // Updates pipe
        return FALSE;
    }


    return TRUE;

} // USBHALTransferData


/******************************************************************************
 * Function:        USBHALFlushPipe
 *
 * Preconditions:   USBHALInitialize must have been called to initialize the 
 *                  USB HAL.  
 *
 *                  The caller must ensure that there is no possible way for
 *                  hardware to be currently accessing the pipe (see notes).
 *
 * Input:           pipe    Uses the TRANSFER_FLAGS (see USBCommon.h) format to 
 *                          identify the endpoint and direction making up the
 *                          pipe to flush.
 *
 * Output:          None
 *
 * Returns:         TRUE if successful, FALSE if not.
 *
 * Side Effects:    Transfer data for this pipe has been zero'd out.
 *
 * Overview:        This routine clears any pending transfers on the given 
 *                  pipe.
 *
 * Note:            This routine ignores the normal HW protocol for ownership
 *                  of the pipe data and flushes the pipe, even if it is in 
 *                  process.  Thus, the caller must ensure that data transfer
 *                  cannot be in process.  This situation occurs when a 
 *                  transfer has been terminated early by the host.
 *****************************************************************************/

PUBLIC BOOL USBHALFlushPipe( TRANSFER_FLAGS pipe )
{
    PUSB_HAL_PIPE   p_Pipe;     // Pointer to pipe data
    pBUF_DESC       desc;
    BYTE            Val;


    // Range check the BDT index
    #ifdef USB_SAFE_MODE
    if (pipe.field.ep_num > USB_DEV_HIGHEST_EP_NUMBER) {
        return FALSE;
    }
    #endif

    // Find the pipe data.
    p_Pipe = FindPipe(pipe.field.ep_num, pipe.field.direction);

    // Index into the even descriptor of the desired endpoint.
    desc = FindDescriptor(pipe.field.ep_num, pipe.field.direction, 0);

    // Grab the current BDT flags value
    Val =desc->setup.Val;

    // Clear the setup bits and byte count.
    desc->setup.Val    = 0;
    desc->byte_cnt.BC  = 0;

    // Advance to the odd descriptor.
    desc++;

    // Determine if none, one, or both buffers were in use.
    Val ^= desc->setup.Val;

    // Clear the setup bits and byte count.
    desc->setup.Val    = 0;
    desc->byte_cnt.BC  = 0;

    // Update the ping-pong tracking if needed
    if (Val & USBHAL_DESC_UOWN)
    {
        p_Pipe->flags.field.ping_pong ^= 1;
    }
    
    // Did it exist?
    #ifdef USB_SAFE_MODE
    if (p_Pipe == NULL) {
        return FALSE;
    }
    #endif

    // Clear out any current transfer data.
    p_Pipe->size      = 0;
    p_Pipe->remaining = 0;
    p_Pipe->count     = 0;
    p_Pipe->buffer    = NULL;

    return TRUE;

}   // USBHALFlushPipe


/*************************************************************************
 * Function:        USBHALSetEpConfiguration
 *
 * Precondition:    USBHALInitialize has been called.
 *
 * Input:           ep_num       Number of endpoint to configure, Must be
 *                               (ep_num >=0) && 
 *                               (ep_num <= USB_DEV_HIGHEST_EP_NUMBER)
 *
 *                  max_pkt_size Size of largest packet this enpoint can
 *                               transfer.
 *
 *                  flags        Configuration flags (see below)
 *
 * Output:          none
 *
 * Returns:         TRUE if successful, FALSE if not.
 *
 * Side Effects:    The endpoint has been configured as desired.
 *
 * Overview:        This routine allows the caller to configure various
 *                  options (see "Flags for USBHALSetEpConfiguration", 
 *                  below) and set the behavior for the given endpoint.
 *
 * Note:            The base address and size of the buffer is not set by
 *                  this routine.  Those features of an endpoint are 
 *                  dynamically managed by the USBHALTransferData routine.
 *
 *                  An endpoint can be "de-configured" by setting its max
 *                  packet size to 0.  When doing this, you should also
 *                  set all flags to 0.
 *************************************************************************/

PUBLIC BOOL USBHALSetEpConfiguration ( BYTE ep_num, UINT16 max_pkt_size, UINT16 flags )
{
    BYTE            ctrl_flags; // EP control flags
    PUSB_HAL_PIPE   p_Pipe;     // Pipe data pointer
    unsigned long   reg_val;


    #ifdef USB_SAFE_MODE
    if (ep_num > USB_DEV_HIGHEST_EP_NUMBER) {
        return FALSE;   // Illegal!
    }
    #endif
    
    // Find the first (Rx) Pipe for the given endpoint.
    p_Pipe = FindPipe(ep_num, 0);

    // Configure the receive pipe?
    if (flags & USB_HAL_RECEIVE)
    {
        // Preserve max packet size
        p_Pipe->max_pkt_size = max_pkt_size;

        // Clear the pipe?
        if (max_pkt_size == 0) {
            memset(p_Pipe, 0, sizeof(USB_HAL_PIPE));
        }
    }

    // Advance to the next (Tx) pipe
    p_Pipe++;
        
    // Configure the transmit pipe?
    if (flags & USB_HAL_TRANSMIT)
    {
        // Preserve max packet size
        p_Pipe->max_pkt_size = max_pkt_size;

        // Clear the pipe?
        if (max_pkt_size == 0) {
            memset(p_Pipe, 0, sizeof(USB_HAL_PIPE));
        }
    }
    
    //
    // Configure the endpoint control register.
    //

    // Shift & mask flags to match core.
    ctrl_flags = (flags >> 8) & CTRL_MASK;

    // Get the reg value and mask off the bits to preserve
    switch(ep_num)
    {
        case 0:  reg_val = U1EP0  & (0xFFFFFF00|CTRL_MASK); break;
        case 1:  reg_val = U1EP1  & (0xFFFFFF00|CTRL_MASK); break;
        case 2:  reg_val = U1EP2  & (0xFFFFFF00|CTRL_MASK); break;
        case 3:  reg_val = U1EP3  & (0xFFFFFF00|CTRL_MASK); break;
        case 4:  reg_val = U1EP4  & (0xFFFFFF00|CTRL_MASK); break;
        case 5:  reg_val = U1EP5  & (0xFFFFFF00|CTRL_MASK); break;
        case 6:  reg_val = U1EP6  & (0xFFFFFF00|CTRL_MASK); break;
        case 7:  reg_val = U1EP7  & (0xFFFFFF00|CTRL_MASK); break;
        case 8:  reg_val = U1EP8  & (0xFFFFFF00|CTRL_MASK); break;
        case 9:  reg_val = U1EP9  & (0xFFFFFF00|CTRL_MASK); break;
        case 10: reg_val = U1EP10 & (0xFFFFFF00|CTRL_MASK); break;
        case 11: reg_val = U1EP11 & (0xFFFFFF00|CTRL_MASK); break;
        case 12: reg_val = U1EP12 & (0xFFFFFF00|CTRL_MASK); break;
        case 13: reg_val = U1EP13 & (0xFFFFFF00|CTRL_MASK); break;
        case 14: reg_val = U1EP14 & (0xFFFFFF00|CTRL_MASK); break;
        case 15: reg_val = U1EP15 & (0xFFFFFF00|CTRL_MASK); break;
        default: return FALSE;
    }

    // OR-in the desired bits
    reg_val |= (unsigned long)ctrl_flags;

    // Disable control in all but EP0
    if (ep_num == 0)
    {
        reg_val &= ~EP_EP_CTL_DIS;
    }
    else
    {
        reg_val |= EP_EP_CTL_DIS;
    }

    // Change the EP control register
    switch(ep_num)
    {
        case 0:  U1EP0  = reg_val; break;
        case 1:  U1EP1  = reg_val; break;
        case 2:  U1EP2  = reg_val; break;
        case 3:  U1EP3  = reg_val; break;
        case 4:  U1EP4  = reg_val; break;
        case 5:  U1EP5  = reg_val; break;
        case 6:  U1EP6  = reg_val; break;
        case 7:  U1EP7  = reg_val; break;
        case 8:  U1EP8  = reg_val; break;
        case 9:  U1EP9  = reg_val; break;
        case 10: U1EP10 = reg_val; break;
        case 11: U1EP11 = reg_val; break;
        case 12: U1EP12 = reg_val; break;
        case 13: U1EP13 = reg_val; break;
        case 14: U1EP14 = reg_val; break;
        case 15: U1EP15 = reg_val; break;
        default: return FALSE;
    }

    return TRUE;

} // USBHALSetEpConfiguration


/*************************************************************************
 * Function:        USBHALInitialize
 *
 * Precondition:    The system has been initialized.
 *
 * Input:           flags       Initialization flags
 *
 * Output:          none
 *
 * Returns:         TRUE if successful, FALSE if not.
 *
 * Side Effects:    The USB HAL SW stack was initialized.
 *
 * Overview:        This call performs the basic initialization of the USB 
 *                  HAL.
 *
 * Note:            This routine can be called to reset the controller.
 *************************************************************************/

PUBLIC BOOL USBHALInitialize ( unsigned long flags )
{
    UINT32  val;

    // Initialize the HAL data
    //
    // Note: Sets state to USB_DEV_INITIALIZED & 
    //       current_role to DEVICE.
    memset(&gHALData, 0, sizeof(gHALData));

    // Reset & enable the USB module, clearing suspend.
    EnableUsbModule();
    
    SetPingPongMode(USB_FULL_PING_PONG);

    // Disable all further USB interrupts
    U1IE     = 0;
    U1EIE    = 0;

    // Clear all existing USB interrupts.
    //OTG module requires write 1 to clear the interrupt
    U1IR     = 0xFF;
    U1EIR    = 0xFF;

    // Initialize the BDT
    memset(&g_BDT, 0, sizeof(g_BDT));
    SetupBDTP(KVA_TO_PA((UINT32)&g_BDT));
    // Note: Initializing the BDT to all zeros (0) is OK at this time since it keeps
    // the buffers under SW control.  All we need to do now is initialize EP 0 before 
    // connecting to the bus.

    // Enable or disable OTG, depending on the configuration.
    #ifdef USB_SUPPORT_DEVICE
        #if !defined(__18CXX)
            #ifdef USB_A0_SILICON_WORK_AROUND
                U1OTGCON = 0x84;
            #else
                U1OTGCON = 0;
            #endif
        #endif
    #else
        #ifdef USB_SUPPORT_HOST
            U1OTGCON = 0;
        #else
            U1OTGCON = UOTGCTRL_OTG_EN;
        #endif
    #endif

    // Reset ping-pong & Enable the USB. 
    val = U1CON & UCTRL_USB_EN;
    U1CON = val | UCTRL_ODD_RST;    
    U1CON = val;

    return TRUE;

} // UsbDevInitialize


/*************************************************************************
 * EOF usb_hal.c
 */

