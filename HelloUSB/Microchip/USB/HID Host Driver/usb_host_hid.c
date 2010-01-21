/******************************************************************************

  USB Host HID Device Driver

This is the Human Interface Device Class driver file for a USB Embedded Host
device. This file should be used in a project with usb_host.c to provided the 
USB hardware interface.

Acronyms/abbreviations used by this class:
    * HID - Human Interface Device

To interface with usb_host.c, the routine USBHostHIDInitialize() should be
specified as the Initialize() function, and USBHostHIDEventHandler() should
be specified as the EventHandler() function in the usbClientDrvTable[] array
declared in usb_config.h.

This driver can be configured to use transfer events from usb_host.c.  Transfer
events require more RAM and ROM than polling, but it cuts down or even
eliminates the required polling of the various USBxxxTasks functions.  For this
class, USBHostHIDTasks() is compiled out if transfer events from usb_host.c
are used.  However, USBHostTasks() still must be called to provide attach,
enumeration, and detach services.  If transfer events from usb_host.c
are going to be used, USB_ENABLE_TRANSFER_EVENT should be defined.  If transfer
status is going to be polled, USB_ENABLE_TRANSFER_EVENT should not be defined.

This driver can also be configured to provide HID transfer events to
the next layer. Generating these events requires a small amount of extra ROM,
but no extra RAM.  The layer above this driver must be configured to receive
and respond to the events.  If HID transfer events are going to be
sent to the next layer, USB_HID_ENABLE_TRANSFER_EVENT should be defined. If
HID transfer status is going to be polled, USB_HID_ENABLE_TRANSFER_EVENT 
should not be defined. In any case transfer event EVENT_HID_RPT_DESC_PARSED
will be sent to interface layer. Application must provide a function
to collect the report descriptor information. Report descriptor information will
be overwritten with new report descriptor(in case multiple interface are present)
information when cotrol returns to HID driver . This is done to avoid using 
extra RAM.

Since HID transfers are performed with interrupt taransfers, 
USB_SUPPORT_INTERRUPT_TRANSFERS must be defined.

* FileName:        usb_host_hid.c
* Dependencies:    None
* Processor:       PIC24/dsPIC30/dsPIC33/PIC32MX
* Compiler:        C30 v2.01/C32 v0.00.18
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
ADG          9-Apr-2008 First release
*******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "GenericTypeDefs.h"
#include "HardwareProfile.h"
#include "usb_config.h"
#include "USB\usb.h"
#include "USB\usb_host_hid.h"
#include "USB\usb_host_hid_parser.h"

//#define DEBUG_MODE
#ifdef DEBUG_MODE
    #include "uart2.h"
#endif


// *****************************************************************************
// *****************************************************************************
// Configuration
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Max Number of Supported Devices

This value represents the maximum number of attached devices this class driver
can support.  If the user does not define a value, it will be set to 1.
Currently this must be set to 1, due to limitations in the USB Host layer.
*/
#ifndef USB_MAX_HID_DEVICES
    #define USB_MAX_HID_DEVICES        1
#endif

// *****************************************************************************
// *****************************************************************************
// Constants
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
// State Machine Constants
// *****************************************************************************

#ifndef USB_ENABLE_TRANSFER_EVENT

#define STATE_MASK                          0x00F0 //
#define SUBSTATE_MASK                       0x000F //
                                                   
#define NEXT_STATE                          0x0010 //
#define NEXT_SUBSTATE                       0x0001 //


#define STATE_DETACHED                      0x0000 //
                                                   
#define STATE_INITIALIZE_DEVICE             0x0010 //
#define SUBSTATE_WAIT_FOR_ENUMERATION       0x0000 //
#define SUBSTATE_DEVICE_ENUMERATED          0x0001 //
                                                   
/* Get Report Descriptor & parse */                //
#define STATE_GET_REPORT_DSC                0x0020 //  
#define SUBSTATE_SEND_GET_REPORT_DSC        0x0000 //
#define SUBSTATE_WAIT_FOR_REPORT_DSC        0x0001 //
#define SUBSTATE_GET_REPORT_DSC_COMPLETE    0x0002 //
#define SUBSTATE_PARSE_REPORT_DSC           0x0003 //
#define SUBSTATE_PARSING_COMPLETE           0x0004 //
                                                   
#define STATE_RUNNING                       0x0030 //
#define SUBSTATE_WAITING_FOR_REQ            0x0000 //
#define SUBSTATE_SEND_READ_REQ              0x0001 //
#define SUBSTATE_READ_REQ_WAIT              0x0002 //
#define SUBSTATE_READ_REQ_DONE              0x0003 //
#define SUBSTATE_SEND_WRITE_REQ             0x0004 //
#define SUBSTATE_WRITE_REQ_WAIT             0x0005 //
#define SUBSTATE_WRITE_REQ_DONE             0x0006 //

#define STATE_HID_RESET_RECOVERY            0x0040 //
#define SUBSTATE_SEND_RESET                 0x0000 //
#define SUBSTATE_WAIT_FOR_RESET             0x0001 //
#define SUBSTATE_RESET_COMPLETE             0x0002 //

#define STATE_HOLDING                       0x0060 //


#else


#define STATE_DETACHED                      0x0000 //

#define STATE_INITIALIZE_DEVICE             0x0001 //
#define STATE_WAIT_FOR_REPORT_DSC           0x0002 //
#define STATE_PARSING_COMPLETE              0x0003 //

#define STATE_RUNNING                       0x0004 //
#define STATE_READ_REQ_WAIT                 0x0005 //
#define STATE_WRITE_REQ_WAIT                0x0006 //

#define STATE_WAIT_FOR_RESET                0x0007 //
#define STATE_RESET_COMPLETE                0x0008 //

#define STATE_HOLDING                       0x0009 //

#endif

// *****************************************************************************
// Other Constants
// *****************************************************************************

#define USB_HID_RESET           (0xFF)  // Device Request code to reset the device.
#define MARK_RESET_RECOVERY     (0x0E)  // Maintain with USB_MSD_DEVICE_INFO

/* Class-Specific Requests */
#define USB_HID_GET_REPORT      (0x01)  //
#define USB_HID_GET_IDLE        (0x02)  //
#define USB_HID_GET_PROTOCOL    (0x03)  //
#define USB_HID_SET_REPORT      (0x09)  //
#define USB_HID_SET_IDLE        (0x0A)  //
#define USB_HID_SET_PROTOCOL    (0x0B)  //

#define USB_HID_INPUT_REPORT    (0x01)  //
#define USB_HID_OUTPUT_REPORT   (0x02)  //
#define USB_HID_FEATURE_REPORT  (0x00)  //


//******************************************************************************
//******************************************************************************
// Data Structures
//******************************************************************************
//******************************************************************************
/*  USB HID Device Information
   This structure is used to hold information of all the interfaces in a device that unique
*/
typedef struct _USB_HID_INTERFACE_DETAILS
{
    struct _USB_HID_INTERFACE_DETAILS   *next;                // Pointer to next interface in the list.
    WORD                                sizeOfRptDescriptor;  // Size of report descriptor of a particular interface.
    WORD                                endpointMaxDataSize;  // Max data size for a interface.
    BYTE                                endpointIN;           // HID IN endpoint for corresponding interface.
    BYTE                                endpointOUT;          // HID OUT endpoint for corresponding interface.
    BYTE                                interfaceNumber;      // Interface number.
    BYTE                                endpointPollInterval; // Polling rate of corresponding interface.
}   USB_HID_INTERFACE_DETAILS;

/*
   This structure is used to hold information about device common to all the interfaces
*/
typedef struct _USB_HID_DEVICE_INFO
{
    WORD                                reportId;              // Report ID of the current transfer.
    BYTE*                               userData;              // Data pointer to application buffer.
    BYTE*                               rptDescriptor;         // Common pointer to report descritor for all the interfaces.
    USB_HID_RPT_DESC_ERROR              HIDparserError;        // Error code incase report descriptor is not in proper format.
    union
    {
        struct
        {
            BYTE                        bfDirection          : 1;   // Direction of current transfer (0=OUT, 1=IN).
            BYTE                        bfReset              : 1;   // Flag indicating to perform HID Reset.
            BYTE                        bfClearDataIN        : 1;   // Flag indicating to clear the IN endpoint.
            BYTE                        bfClearDataOUT       : 1;   // Flag indicating to clear the OUT endpoint.
            BYTE                        breportDataCollected : 1;   // Flag indicationg report data is collected ny application
        };
        BYTE                            val;
    }                                   flags;
    BYTE                                driverSupported;       // If HID driver supports requested Class,Subclass & Protocol.
    BYTE                                deviceAddress;         // Address of the device on the bus.
    BYTE                                errorCode;             // Error code of last error.
    BYTE                                state;                 // State machine state of the device.
    BYTE                                returnState;           // State to return to after performing error handling.
    BYTE                                noOfInterfaces;        // Total number of interfaces in the device.
    BYTE                                interface;             // Interface number of current transfer.
    BYTE                                bytesTransferred;      // Number of bytes transferred to/from the user's data buffer.
    BYTE                                reportSize;            // Size of report currently requested for transfer.
    BYTE                                endpointDATA;          // Endpoint to use for the current transfer.
} USB_HID_DEVICE_INFO;




//******************************************************************************
//******************************************************************************
// Section: Local Prototypes
//******************************************************************************
//******************************************************************************
void _USBHostHID_FreeRptDecriptorDataMem(BYTE deviceAddress);
void _USBHostHID_ResetStateJump( BYTE i );


//******************************************************************************
//******************************************************************************
// Macros
//******************************************************************************
//******************************************************************************

#ifndef USB_ENABLE_TRANSFER_EVENT

#define _USBHostHID_SetNextState()              { deviceInfo[i].state = (deviceInfo[i].state & STATE_MASK) + NEXT_STATE; }
#define _USBHostHID_SetNextSubState()           { deviceInfo[i].state += NEXT_SUBSTATE; }
#define _USBHostHID_TerminateTransfer( error )  {                                                                       \
                                                    deviceInfo[i].errorCode    = error;                                 \
                                                    deviceInfo[i].state        = STATE_RUNNING | SUBSTATE_WAITING_FOR_REQ;\
                                                }
#else
  #ifdef USB_HID_ENABLE_TRANSFER_EVENT
    #define _USBHostHID_TerminateTransfer( error )  {                                                                       \
                                                        deviceInfo[i].errorCode    = error;                                 \
                                                        deviceInfo[i].state        = STATE_RUNNING;\
                                                        usbDeviceInterfaceTable.EventHandler( deviceInfo[i].deviceAddress, EVENT_HID_TRANSFER, NULL, 0 );     \
                                                    }
  #else
    #define _USBHostHID_TerminateTransfer( error )  {                                                                       \
                                                        deviceInfo[i].errorCode    = error;                                 \
                                                        deviceInfo[i].state        = STATE_RUNNING;\
                                                    }
  #endif
#endif

//******************************************************************************
//******************************************************************************
// Section: HID Host Global Variables
//******************************************************************************
//******************************************************************************

USB_HID_DEVICE_INFO                  deviceInfo[USB_MAX_HID_DEVICES] __attribute__ ((aligned));
USB_HID_INTERFACE_DETAILS*           pInterfaceDetails = NULL;
USB_HID_INTERFACE_DETAILS*           pCurrInterfaceDetails = NULL;

//******************************************************************************
//******************************************************************************
// Section: HID Host External Variables
//******************************************************************************
//******************************************************************************

extern BYTE* parsedDataMem;

extern CLIENT_DRIVER_TABLE usbDeviceInterfaceTable;
extern USB_HID_RPT_DESC_ERROR _USBHostHID_Parse_Report(BYTE*, WORD, WORD, BYTE);

// *****************************************************************************
// *****************************************************************************
// Application Callable Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    BYTE    USBHostHIDDeviceStatus( BYTE deviceAddress )

  Summary:

  Description:
    This function determines the status of a HID device.

  Preconditions:  None

  Parameters:
    BYTE deviceAddress - address of device to query

  Return Values:
    USB_HID_DEVICE_NOT_FOUND           -  Illegal device address, or the
                                          device is not an HID
    USB_HID_INITIALIZING               -  HID is attached and in the
                                          process of initializing
    USB_PROCESSING_REPORT_DESCRIPTOR   -  HID device is detected and report 
                                          descriptor is being parsed
    USB_HID_NORMAL_RUNNING             -  HID Device is running normal, 
                                          ready to send and receive reports 
    USB_HID_DEVICE_HOLDING             - 
    USB_HID_DEVICE_DETACHED            -  HID detached.

  Remarks:
    None
*******************************************************************************/
BYTE    USBHostHIDDeviceStatus( BYTE deviceAddress )
{
    BYTE    i;
    BYTE    status;

    // Find the correct device.
    for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfo[i].deviceAddress != deviceAddress); i++);
    if (i == USB_MAX_HID_DEVICES)
    {
        return USB_HID_DEVICE_NOT_FOUND;
    }
    
    status = USBHostDeviceStatus( deviceAddress );
    if (status != USB_DEVICE_ATTACHED)
    {
        return status;
    }
    else
    {
        // The device is attached and done enumerating.  We can get more specific now.    
        #ifndef USB_ENABLE_TRANSFER_EVENT
           switch (deviceInfo[i].state & STATE_MASK)
           {
               case STATE_INITIALIZE_DEVICE:
                   return USB_HID_INITIALIZING;
                   break;
               case STATE_GET_REPORT_DSC:
                   return USB_PROCESSING_REPORT_DESCRIPTOR;
                   break;
      
               case STATE_RUNNING:
                   return USB_HID_NORMAL_RUNNING;
                   break;
               case STATE_HOLDING:
                   return USB_HID_DEVICE_HOLDING;
                   break;

               case STATE_HID_RESET_RECOVERY:
                   return USB_HID_RESETTING_DEVICE;
                   break;
                   
               default:  
                   return USB_HID_DEVICE_DETACHED;  
                   break;
           }
        #else
           switch (deviceInfo[i].state)
           {
               case STATE_INITIALIZE_DEVICE:
               case STATE_PARSING_COMPLETE:
                   return USB_HID_INITIALIZING;
                   break;

               case STATE_RUNNING:
               case STATE_READ_REQ_WAIT:
               case STATE_WRITE_REQ_WAIT:
                   return USB_HID_NORMAL_RUNNING;
                   break;

               case STATE_HOLDING:
                   return USB_HID_DEVICE_HOLDING;
                   break;
                   
               case STATE_WAIT_FOR_RESET:
               case STATE_RESET_COMPLETE:
                   return USB_HID_RESETTING_DEVICE;
                   break;

               default:  
                   return USB_HID_DEVICE_DETACHED;  
                   break;
           }
        #endif
    }    
}
    
/*******************************************************************************
  Function:
    BYTE USBHostHIDResetDevice( BYTE deviceAddress )

  Summary:
    This function starts a HID  reset.

  Description:
    This function starts a HID reset.  A reset can be
    issued only if the device is attached and not being initialized.

  Precondition:
    None

  Parameters:
    BYTE deviceAddress - Device address

  Return Values:
    USB_SUCCESS                 - Reset started
    USB_MSD_DEVICE_NOT_FOUND    - No device with specified address
    USB_MSD_ILLEGAL_REQUEST     - Device is in an illegal state for reset

  Remarks:
    None
*******************************************************************************/
BYTE USBHostHIDResetDevice( BYTE deviceAddress )
{
    BYTE    i;

    // Make sure a valid device is being requested.
    if ((deviceAddress == 0) || (deviceAddress > 127))
    {
        return USB_HID_DEVICE_NOT_FOUND;
    }

    // Find the correct device.
    for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfo[i].deviceAddress != deviceAddress); i++);
    if (i == USB_MAX_HID_DEVICES)
    {
        return USB_HID_DEVICE_NOT_FOUND;
    }

     #ifndef USB_ENABLE_TRANSFER_EVENT
       if (((deviceInfo[i].state & STATE_MASK) != STATE_DETACHED) &&
            ((deviceInfo[i].state & STATE_MASK) != STATE_INITIALIZE_DEVICE))
     #else
         if ((deviceInfo[i].state != STATE_DETACHED) &&
             (deviceInfo[i].state != STATE_INITIALIZE_DEVICE))
     #endif
        {
            deviceInfo[i].flags.val |= MARK_RESET_RECOVERY;
            deviceInfo[i].flags.bfReset = 1;
        #ifndef USB_ENABLE_TRANSFER_EVENT
            deviceInfo[i].returnState = STATE_RUNNING | SUBSTATE_WAITING_FOR_REQ;
        #else
            deviceInfo[i].returnState = STATE_RUNNING;
        #endif
        
            _USBHostHID_ResetStateJump( i );
            return USB_SUCCESS;
        }
        return USB_HID_ILLEGAL_REQUEST;
}


/*******************************************************************************
  Function:
     void USBHostHIDTasks( void )

  Summary:
    This function performs the maintenance tasks required by HID class

  Description:
    This function performs the maintenance tasks required by the HID
    class.  If transfer events from the host layer are not being used, then
    it should be called on a regular basis by the application.  If transfer
    events from the host layer are being used, this function is compiled out,
    and does not need to be called.

  Precondition:
    USBHostHIDInitialize() has been called.

  Parameters:
    None - None

  Returns:
    None

  Remarks:
    None
*******************************************************************************/
void USBHostHIDTasks( void )
{
#ifndef USB_ENABLE_TRANSFER_EVENT
    DWORD   byteCount;
    BYTE    errorCode;
    BYTE    i;

//    BYTE    temp;

    for (i=0; i<USB_MAX_HID_DEVICES; i++)
    {
        if (deviceInfo[i].deviceAddress == 0) /* device address updated by lower layer */
        {
            deviceInfo[i].state = STATE_DETACHED;
        }

        switch (deviceInfo[i].state & STATE_MASK)
        {
            case STATE_DETACHED:
                // No device attached.
                break;

            case STATE_INITIALIZE_DEVICE:
                switch (deviceInfo[i].state & SUBSTATE_MASK)
                {
                    case SUBSTATE_WAIT_FOR_ENUMERATION:
                        if (USBHostDeviceStatus( deviceInfo[i].deviceAddress ) == USB_DEVICE_ATTACHED)
                        {
                            _USBHostHID_SetNextSubState();
                            pCurrInterfaceDetails = pInterfaceDetails; // assign current interface to top of list
                        }
                        break;

                    case SUBSTATE_DEVICE_ENUMERATED:

                            _USBHostHID_SetNextState(); /* need to add sub states to Set Config, Get LANGID & String Descriptors */
                        break;

                    default : 
                        break;
                }
                break;

            case STATE_GET_REPORT_DSC:
                switch (deviceInfo[i].state & SUBSTATE_MASK)
                {
                    case SUBSTATE_SEND_GET_REPORT_DSC:
                            // If we are currently sending a token, we cannot do anything.
                            if (U1CONbits.TOKBUSY)
                                break;

                            if(pCurrInterfaceDetails != NULL) // end of interface list
                            {
                                if(pCurrInterfaceDetails->sizeOfRptDescriptor !=0) // interface must have a Report Descriptor
                                {
                                    if((deviceInfo[i].rptDescriptor = (BYTE *)malloc(pCurrInterfaceDetails->sizeOfRptDescriptor)) == NULL)
                                    {
                                        _USBHostHID_TerminateTransfer(USB_MEMORY_ALLOCATION_ERROR);
                                        break;
                                    }
                                    // send new interface request
                                    if (!USBHostDeviceRequest( deviceInfo[i].deviceAddress, USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_INTERFACE, 
                                         USB_REQUEST_GET_DESCRIPTOR, DSC_RPT, pCurrInterfaceDetails->interfaceNumber,pCurrInterfaceDetails->sizeOfRptDescriptor, deviceInfo[i].rptDescriptor,
                                         USB_DEVICE_REQUEST_GET ))
                                    {           
                                         _USBHostHID_SetNextSubState();
                                    }
                                    else
                                    {
                                        free(deviceInfo[i].rptDescriptor);
                                    }
                                }
                                
                            }
                            else
                            {
                                _USBHostHID_TerminateTransfer(USB_HID_INTERFACE_ERROR);
                            }

                        break;
                    case SUBSTATE_WAIT_FOR_REPORT_DSC:
                            if (USBHostTransferIsComplete( deviceInfo[i].deviceAddress, 0, &errorCode, &byteCount ))
                            {
                                if (errorCode)
                                {
                                    /* Set error code */
                                    _USBHostHID_TerminateTransfer(errorCode);
                                }
                                else
                                {
                                    // Clear the STALL.  Since it is EP0, we do not have to clear the stall.
                                    USBHostClearEndpointErrors( deviceInfo[i].deviceAddress, 0 );
                                }
                                _USBHostHID_SetNextSubState();
                            }
                        break;
                    case SUBSTATE_GET_REPORT_DSC_COMPLETE:
                                _USBHostHID_SetNextSubState();
                                                            
                        break;

                    case SUBSTATE_PARSE_REPORT_DSC:   
                        /* Invoke HID Parser ,, validate for all the errors in report Descriptor */
                              deviceInfo[i].HIDparserError = _USBHostHID_Parse_Report((BYTE*)deviceInfo[i].rptDescriptor , (WORD)pCurrInterfaceDetails->sizeOfRptDescriptor, 
                                                                                       (WORD)pCurrInterfaceDetails->endpointPollInterval, pCurrInterfaceDetails->interfaceNumber);
                              if(deviceInfo[i].HIDparserError)
                                {
                                  /* Report Descriptor is flawed , flag error and free memory , 
                                     retry by requesting again */
                    #ifdef DEBUG_MODE
                                     UART2PrintString("\r\nHID Error Reported :  ");
                                     UART2PutHex(deviceInfo[i].HIDparserError);
                    #endif                     
                                     _USBHostHID_FreeRptDecriptorDataMem(deviceInfo[i].deviceAddress);
                                     _USBHostHID_TerminateTransfer(USB_HID_REPORT_DESCRIPTOR_BAD);
                                }
                              else
                                {
                                    _USBHostHID_SetNextSubState();
                                    /* Inform Application layer of new device attached */
                                }
                          
                    break;

                    case SUBSTATE_PARSING_COMPLETE:
                                usbDeviceInterfaceTable.Initialize( deviceInfo[i].deviceAddress,0 );
                                if(usbDeviceInterfaceTable.EventHandler(deviceInfo[i].deviceAddress, EVENT_HID_RPT_DESC_PARSED, NULL, 0 ))
                                {
                                    deviceInfo[i].flags.breportDataCollected = 1;
                                }
                                else
                                {
                                    // atleast once report must be present if not flag error
                                    if((pCurrInterfaceDetails->interfaceNumber == (deviceInfo[i].noOfInterfaces-1)) &&
                                           (deviceInfo[i].flags.breportDataCollected == 0))
                                    {
                                      _USBHostHID_FreeRptDecriptorDataMem(deviceInfo[i].deviceAddress);
                                      _USBHostHID_TerminateTransfer(USB_ILLEGAL_REQUEST);
                                    }
                                }
                                // free the previous allocated memory, reallocate for new interface if needed
                                free(deviceInfo[i].rptDescriptor);
                                pCurrInterfaceDetails = pCurrInterfaceDetails->next;
                                if(pCurrInterfaceDetails != NULL)
                                {
                                    deviceInfo[i].state = STATE_GET_REPORT_DSC;
                                }
                                else
                                {
                                    if(deviceInfo[i].flags.breportDataCollected == 0)
                                    {
                                        _USBHostHID_FreeRptDecriptorDataMem(deviceInfo[i].deviceAddress);
                                        _USBHostHID_TerminateTransfer(USB_ILLEGAL_REQUEST);
                                    }
                                    else
                                    {
                                        deviceInfo[i].state = STATE_RUNNING;
                                    }
                                }
                        break;

                    default : 
                        break;
                }
                break;
            case STATE_RUNNING:
                switch (deviceInfo[i].state & SUBSTATE_MASK)
                {
                    case SUBSTATE_WAITING_FOR_REQ:   
                              /* waiting for request from application */
                        break;
                    
                    case SUBSTATE_SEND_READ_REQ:   
                                errorCode = USBHostRead( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA, 
                                                         deviceInfo[i].userData, deviceInfo[i].reportSize );
                                if (errorCode)
                                {
                                    if(USB_ENDPOINT_STALLED == errorCode)
                                    {
                                         USBHostClearEndpointErrors( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA );
                                         deviceInfo[i].returnState = STATE_RUNNING | SUBSTATE_WAITING_FOR_REQ;
                                         deviceInfo[i].flags.bfReset = 1;
                                         _USBHostHID_ResetStateJump( i );
                                    }
                                    else
                                    {
                                        /* Set proper error code as per HID guideline */
                                        _USBHostHID_TerminateTransfer(errorCode);
                                    }
                                }
                                else
                                {
                                    // Clear the STALL.  Since it is EP0, we do not have to clear the stall.
                                    _USBHostHID_SetNextSubState();
                                }
                        break;
                        
                    case SUBSTATE_READ_REQ_WAIT:   
                              if (USBHostTransferIsComplete( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA, &errorCode, &byteCount ))
                              {
                                if (errorCode)
                                {
                                    if(USB_ENDPOINT_STALLED == errorCode)
                                    {
                                         USBHostClearEndpointErrors( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA );
                                         deviceInfo[i].returnState = STATE_RUNNING | SUBSTATE_WAITING_FOR_REQ;
                                         deviceInfo[i].flags.bfReset = 1;
                                         _USBHostHID_ResetStateJump( i );
                                    }
                                    else
                                    {
                                        /* Set proper error code as per HID guideline */
                                        _USBHostHID_TerminateTransfer(errorCode);
                                    }
                                }
                                else
                                {
                                    // Clear the STALL.  Since it is EP0, we do not have to clear the stall.
                                    USBHostClearEndpointErrors( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA );
                                    deviceInfo[i].bytesTransferred = byteCount; /* Can compare with report size and flag error ???*/
                                   _USBHostHID_SetNextSubState();
                                }
                              }
#ifdef DEBUG_MODE
                              UART2PrintString("|");
#endif                                
                    break;
                    case SUBSTATE_READ_REQ_DONE:   
                             /* Next transfer */                          
                              deviceInfo[i].state = STATE_RUNNING | SUBSTATE_WAITING_FOR_REQ;
#ifdef USB_HID_ENABLE_TRANSFER_EVENT
                              usbDeviceInterfaceTable.EventHandler(deviceInfo[i].deviceAddress, EVENT_HID_READ_DONE, NULL, 0 );
#endif
#ifdef DEBUG_MODE
                              UART2PrintString("}");
#endif
                    break;

                    case SUBSTATE_SEND_WRITE_REQ:   
                                if(deviceInfo[i].endpointDATA == 0x00)// if endpoint 0 then use control transfer
                                {
                                    errorCode   = USBHostDeviceRequest( deviceInfo[i].deviceAddress, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_CLASS | USB_SETUP_RECIPIENT_INTERFACE, 
                                                                        USB_HID_SET_REPORT, deviceInfo[i].reportId, deviceInfo[i].interface,deviceInfo[i].reportSize, deviceInfo[i].userData,
                                                                        USB_DEVICE_REQUEST_SET );
                                }
                                else
                                {
                                    errorCode = USBHostWrite( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA,
                                                      deviceInfo[i].userData, deviceInfo[i].reportSize );
                                }

                                if (errorCode)
                                {
                                    if(USB_ENDPOINT_STALLED == errorCode)
                                    {
                                         USBHostClearEndpointErrors( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA);
                                         deviceInfo[i].returnState = STATE_RUNNING | SUBSTATE_WAITING_FOR_REQ;
                                         deviceInfo[i].flags.bfReset = 1;
                                         _USBHostHID_ResetStateJump( i );
                                    }
                                    else
                                    {
                                        /* Set proper error code as per HID guideline */
                                        _USBHostHID_TerminateTransfer(errorCode);
                                    }
                                }
                                else
                                {
                                    // TODO assuming only a STALL here
                                    // Clear the STALL.  Since it is EP0, we do not have to clear the stall.
                                    _USBHostHID_SetNextSubState();
                                }

                    break;
                    case SUBSTATE_WRITE_REQ_WAIT:   
                                if (USBHostTransferIsComplete( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA, &errorCode, &byteCount ))
                              {
                                if (errorCode)
                                {
                                    if(USB_ENDPOINT_STALLED == errorCode)
                                    {
                                         USBHostClearEndpointErrors( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA );
                                         deviceInfo[i].returnState = STATE_RUNNING | SUBSTATE_WAITING_FOR_REQ;
                                         deviceInfo[i].flags.bfReset = 1;
                                         _USBHostHID_ResetStateJump( i );
                                    }
                                    else
                                    {
                                        /* Set proper error code as per HID guideline */
                                        _USBHostHID_TerminateTransfer(errorCode);
                                    }
                                }
                                else
                                {
                                    // TODO assuming only a STALL here
                                    // Clear the STALL.  Since it is EP0, we do not have to clear the stall.
                                    USBHostClearEndpointErrors( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA);
                                    _USBHostHID_SetNextSubState();
                                }
                              }
                  break;
                    case SUBSTATE_WRITE_REQ_DONE:   
                              deviceInfo[i].state = STATE_RUNNING | SUBSTATE_WAITING_FOR_REQ;
#ifdef USB_HID_ENABLE_TRANSFER_EVENT
                              usbDeviceInterfaceTable.EventHandler(deviceInfo[i].deviceAddress, EVENT_HID_WRITE_DONE, NULL, 0 );
#endif
                    break;

                }
                break;

            case STATE_HID_RESET_RECOVERY:
                switch (deviceInfo[i].state & SUBSTATE_MASK)
                {
                    case SUBSTATE_SEND_RESET:   /* Not sure of rest request */
                            errorCode = USBHostDeviceRequest( deviceInfo[i].deviceAddress, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_CLASS | USB_SETUP_RECIPIENT_INTERFACE,
                                            USB_HID_RESET, 0, deviceInfo[i].interface, 0, NULL, USB_DEVICE_REQUEST_SET );

                            if (errorCode)
                            {
                                    if(USB_ENDPOINT_STALLED == errorCode)
                                    {
                                         USBHostClearEndpointErrors( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA );
                                         deviceInfo[i].returnState = STATE_RUNNING | SUBSTATE_WAITING_FOR_REQ;
                                         deviceInfo[i].flags.bfReset = 1;
                                         _USBHostHID_ResetStateJump( i );
                                    }
                                    else
                                    {
                                        /* Set proper error code as per HID guideline */
                                        _USBHostHID_TerminateTransfer(errorCode);
                                    }
                            }
                            else
                            {
                                _USBHostHID_SetNextSubState();
                            }
                        break;
                    case SUBSTATE_WAIT_FOR_RESET:
                            if (USBHostTransferIsComplete( deviceInfo[i].deviceAddress, 0, &errorCode, &byteCount ))
                            {
                                if (errorCode)
                                {
                                    if(USB_ENDPOINT_STALLED == errorCode)
                                    {
                                         USBHostClearEndpointErrors( deviceInfo[i].deviceAddress, 0 );
                                         deviceInfo[i].returnState = STATE_RUNNING | SUBSTATE_WAITING_FOR_REQ;
                                         deviceInfo[i].flags.bfReset = 1;
                                         _USBHostHID_ResetStateJump( i );
                                    }
                                    else
                                    {
                                        _USBHostHID_TerminateTransfer(errorCode);
                                    }
                                }
                                else
                                {
                                    deviceInfo[i].flags.bfReset = 0;
                                    _USBHostHID_ResetStateJump( i );
                                }
                            }
                            break;

                    case SUBSTATE_RESET_COMPLETE:
                            _USBHostHID_ResetStateJump( i );
                            break;
                }
                break;

            case STATE_HOLDING:
                break;

            default : 
                break;
        }
    }
#endif
}


/*******************************************************************************
  Function:
    USBHostHIDTransfer( BYTE deviceAddress, BYTE direction, BYTE reportid,
                        BYTE size, BYTE *data)

  Summary:
    This function starts a HID transfer.

  Description:
    This function starts a HID transfer. A read/write wrapper is provided in
    application interface file to access this function.

  Preconditions:
    None

  Parameters:
    BYTE deviceAddress      - Device address
    BYTE direction          - 1=read, 0=write
    BYTE interfaceNum       - Interface number of the device 
    BYTE reportid           - Report ID of the requested report
    BYTE size               - Byte size of the data buffer
    BYTE *data              - Pointer to the data buffer

  Return Values:
    USB_SUCCESS                 - Request started successfully
    USB_HID_DEVICE_NOT_FOUND    - No device with specified address
    USB_HID_DEVICE_BUSY         - Device not in proper state for
                                  performing a transfer
  Remarks:
    None
*******************************************************************************/

BYTE USBHostHIDTransfer( BYTE deviceAddress, BYTE direction, BYTE interfaceNum, WORD reportid, WORD size, BYTE *data)
{
    BYTE    i;
    BYTE    errorCode;

    #ifdef DEBUG_MODE
//        UART2PrintString( "HID: Transfer: " );
        if (direction) 
        {
//            UART2PrintString( "Read, " );
        }
        else
        {
 //           UART2PrintString( "Write, " );
        }
    #endif
            
    // Find the correct device.
    for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfo[i].deviceAddress != deviceAddress); i++);
    if (i == USB_MAX_HID_DEVICES)
    {
        return USB_HID_DEVICE_NOT_FOUND;
    }

    pCurrInterfaceDetails = pInterfaceDetails;
    while((pCurrInterfaceDetails != NULL) && (pCurrInterfaceDetails->interfaceNumber != interfaceNum))
    {
        pCurrInterfaceDetails = pCurrInterfaceDetails->next;        
    }
    // Make sure the device is in a state ready to read/write.
    // Make sure the device is in a state ready to read/write.
    #ifndef USB_ENABLE_TRANSFER_EVENT
        if (deviceInfo[i].state != (STATE_RUNNING) &&
           (deviceInfo[i].state & SUBSTATE_MASK) != (SUBSTATE_WAITING_FOR_REQ))
    #else
        if (deviceInfo[i].state != STATE_RUNNING)
    #endif
        {
            return USB_HID_DEVICE_BUSY;
        }
     
    // Initialize the transfer information.
    deviceInfo[i].bytesTransferred  = 0;
    deviceInfo[i].errorCode         = USB_SUCCESS;
    deviceInfo[i].userData          = data;
    deviceInfo[i].endpointDATA      = pCurrInterfaceDetails->endpointIN;
    deviceInfo[i].reportSize        = size;
    deviceInfo[i].reportId          = reportid;
    deviceInfo[i].interface         = interfaceNum;

    if (!direction) // OUT
    {
        deviceInfo[i].endpointDATA  = pCurrInterfaceDetails->endpointOUT;
        deviceInfo[i].reportId      = (reportid |((WORD)USB_HID_OUTPUT_REPORT<<8));
    }
    #ifdef DEBUG_MODE
//        UART2PrintString( "Data EP: " );
//        UART2PutHex( deviceInfo[i].endpointDATA );
//        UART2PrintString( "\r\n" );
    #endif
    
    #ifndef USB_ENABLE_TRANSFER_EVENT
        // Jump to the transfer state.
        if(!direction)
            {
                /* send write req */
                deviceInfo[i].state             = STATE_RUNNING | SUBSTATE_SEND_WRITE_REQ;
            }
        else
            {
                deviceInfo[i].state             = STATE_RUNNING | SUBSTATE_SEND_READ_REQ;
            }
    #else
        if(!direction)
            {
                if(deviceInfo[i].endpointDATA == 0x00)// if endpoint 0 then use control transfer
                {
                    errorCode   = USBHostDeviceRequest( deviceInfo[i].deviceAddress, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_CLASS | USB_SETUP_RECIPIENT_INTERFACE, 
                                 USB_HID_SET_REPORT, deviceInfo[i].reportId, deviceInfo[i].interface,deviceInfo[i].reportSize, deviceInfo[i].userData,
                                 USB_DEVICE_REQUEST_SET );
                    deviceInfo[i].state         = STATE_WRITE_REQ_WAIT;
                }
                else
                {
                    errorCode                   = USBHostWrite( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA,
                                                            deviceInfo[i].userData, deviceInfo[i].reportSize );
                    deviceInfo[i].state         = STATE_WRITE_REQ_WAIT;
                }
            }
        else
            {
                errorCode                   = USBHostRead( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA, 
                                                           deviceInfo[i].userData, deviceInfo[i].reportSize );
                deviceInfo[i].state         = STATE_READ_REQ_WAIT;
            }

        if(errorCode)
            {
                //TODO Now what??
                _USBHostHID_TerminateTransfer( USB_HID_RESET_ERROR );
            }
        else
            {
                deviceInfo[i].flags.bfReset = 0;
            }
    #endif
    return USB_SUCCESS;
}


/*******************************************************************************
  Function:
    BOOL USBHostHIDTransferIsComplete( BYTE deviceAddress,
                        BYTE *errorCode, DWORD *byteCount )

  Summary:
    This function indicates whether or not the last transfer is complete.

  Description:
    This function indicates whether or not the last transfer is complete.
    If the functions returns TRUE, the returned byte count and error
    code are valid. Since only one transfer can be performed at once
    and only one endpoint can be used, we only need to know the
    device address.

  Precondition:
    None

  Parameters:
    BYTE deviceAddress  - Device address
    BYTE *errorCode     - Error code from last transfer
    DWORD *byteCount    - Number of bytes transferred

  Return Values:
    TRUE    - Transfer is complete, errorCode is valid
    FALSE   - Transfer is not complete, errorCode is not valid
*******************************************************************************/

BOOL USBHostHIDTransferIsComplete( BYTE deviceAddress, BYTE *errorCode, BYTE *byteCount )
{
    BYTE    i;

    // Find the correct device.
    for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfo[i].deviceAddress != deviceAddress); i++);
    if ((i == USB_MAX_HID_DEVICES) || (deviceInfo[i].state == STATE_DETACHED))
    {
        *errorCode = USB_HID_DEVICE_NOT_FOUND;
        *byteCount = 0;
        return TRUE;
    }

    *byteCount = deviceInfo[i].bytesTransferred;
    *errorCode = deviceInfo[i].errorCode;

      #ifndef USB_ENABLE_TRANSFER_EVENT
            if(deviceInfo[i].state == (STATE_RUNNING | SUBSTATE_WAITING_FOR_REQ))
      #else
            if(deviceInfo[i].state == STATE_RUNNING)
      #endif
                {
                    return TRUE;
                }
            else
                {
                    return FALSE;
                }
}


// *****************************************************************************
// *****************************************************************************
// Host Stack Interface Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    BOOL USBHostHIDEventHandler( BYTE address, USB_EVENT event,
                        void *data, DWORD size )

  Precondition:
    The device has been initialized.

  Summary:
    This function is the event handler for this client driver.

  Description:
    This function is the event handler for this client driver.  It is called
    by the host layer when various events occur.

  Parameters:
    BYTE address    - Address of the device
    USB_EVENT event - Event that has occurred
    void *data      - Pointer to data pertinent to the event
    DWORD size       - Size of the data

  Return Values:
    TRUE   - Event was handled
    FALSE  - Event was not handled

  Remarks:
    None
*******************************************************************************/
BOOL USBHostHIDEventHandler( BYTE address, USB_EVENT event, void *data, DWORD size )
{
    BYTE    i;
    switch (event)
    {
        case EVENT_NONE:             // No event occured (NULL event)
            USBTasks();
            return TRUE;
            break;

        case EVENT_DETACH:           // USB cable has been detached (data: BYTE, address of device)
            #ifdef DEBUG_MODE
                UART2PrintString( "HID: Detach\r\n" );
            #endif
            // Find the device in the table.  If found, clear the important fields.
            for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfo[i].deviceAddress != address); i++);
            if (i < USB_MAX_HID_DEVICES)
            {
                /* Free the memory used by the HID device */
                _USBHostHID_FreeRptDecriptorDataMem(address);
                usbDeviceInterfaceTable.EventHandler( address, EVENT_DETACH, NULL, 0 );
                deviceInfo[i].deviceAddress    = 0;
                deviceInfo[i].state            = STATE_DETACHED;
            }

            return TRUE;
            break;

        case EVENT_HUB_ATTACH:       // USB hub has been attached
        case EVENT_TRANSFER:         // A USB transfer has completed - NOT USED
            #if defined( USB_ENABLE_TRANSFER_EVENT )
                #ifdef DEBUG_MODE
//                    UART2PrintString( "HID: transfer event: " );
//                    UART2PutHex( address );
//                    UART2PrintString( "\r\n" );
                #endif

                for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfo[i].deviceAddress != address); i++) {}
                if (i == USB_MAX_HID_DEVICES)
                {
                    #ifdef DEBUG_MODE
                        UART2PrintString( "HID: Unknown device\r\n" );
                    #endif
                    return FALSE;
                }
                #ifdef DEBUG_MODE
 //                   UART2PrintString( "HID: Device state: " );
 //                   UART2PutHex( deviceInfo[i].state );
 //                   UART2PrintString( "\r\n" );
                #endif
                switch (deviceInfo[i].state)
                {
                    case STATE_WAIT_FOR_REPORT_DSC:
                        #ifdef DEBUG_MODE
                            UART2PrintString( "HID: Get Report Descriptor\r\n" );
                        #endif
                             deviceInfo[i].bytesTransferred = ((HOST_TRANSFER_DATA *)data)->dataCount;
                        if ((!((HOST_TRANSFER_DATA *)data)->bErrorCode) && (deviceInfo[i].bytesTransferred == pCurrInterfaceDetails->sizeOfRptDescriptor ))
                        {
                            /* Invoke HID Parser ,, validate for all the errors in report Descriptor */
//                             deviceInfo[i].bytesTransferred = ((HOST_TRANSFER_DATA *)data)->dataCount;
                             deviceInfo[i].HIDparserError = _USBHostHID_Parse_Report((BYTE*)deviceInfo[i].rptDescriptor , (WORD)pCurrInterfaceDetails->sizeOfRptDescriptor,
                                                                                    (WORD)pCurrInterfaceDetails->endpointPollInterval, pCurrInterfaceDetails->interfaceNumber);

                            if(deviceInfo[i].HIDparserError)
                              {
                                /* Report Descriptor is flawed , flag error and free memory , 
                                   retry by requesting again */
                                   #ifdef DEBUG_MODE
                                      UART2PrintString("\r\nHID Error Reported :  ");
                                      UART2PutHex(deviceInfo[i].HIDparserError);
                                   #endif
                                   
                                   _USBHostHID_FreeRptDecriptorDataMem(deviceInfo[i].deviceAddress);
                                   _USBHostHID_TerminateTransfer(USB_HID_REPORT_DESCRIPTOR_BAD);
                              }
                            else
                              {
                                /* Inform Application layer of new device attached */
                                  usbDeviceInterfaceTable.Initialize( deviceInfo[i].deviceAddress,0 );
                                  if(usbDeviceInterfaceTable.EventHandler(deviceInfo[i].deviceAddress, EVENT_HID_RPT_DESC_PARSED, NULL, 0 ))
                                    {
                                        deviceInfo[i].flags.breportDataCollected = 1;
                                    }
                                  else
                                    {
                                       if((pCurrInterfaceDetails->interfaceNumber == (deviceInfo[i].noOfInterfaces-1)) &&
                                           (deviceInfo[i].flags.breportDataCollected == 0))
                                        {
                                          _USBHostHID_FreeRptDecriptorDataMem(deviceInfo[i].deviceAddress);
                                          _USBHostHID_TerminateTransfer(USB_ILLEGAL_REQUEST);
                                        }
                                    }
                                  free(deviceInfo[i].rptDescriptor);
                                  pCurrInterfaceDetails = pCurrInterfaceDetails->next;

                                  if(pCurrInterfaceDetails != NULL)
                                   {
                                       if(pCurrInterfaceDetails->sizeOfRptDescriptor !=0)
                                       {
                                          if((deviceInfo[i].rptDescriptor = (BYTE *)malloc(pCurrInterfaceDetails->sizeOfRptDescriptor)) == NULL)
                                             {
                                                _USBHostHID_TerminateTransfer(USB_MEMORY_ALLOCATION_ERROR);
                                                 break;
                                              }
                                       }
                                      if(USBHostDeviceRequest( deviceInfo[i].deviceAddress, USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_INTERFACE, 
                                            USB_REQUEST_GET_DESCRIPTOR, DSC_RPT, pCurrInterfaceDetails->interfaceNumber, pCurrInterfaceDetails->sizeOfRptDescriptor, deviceInfo[i].rptDescriptor,
                                            USB_DEVICE_REQUEST_GET ))
                                       {           
                                          free(deviceInfo[i].rptDescriptor);
                                           break;
                                       }
                                       deviceInfo[i].state = STATE_WAIT_FOR_REPORT_DSC;
                                   }
                                  else
                                   {
                                       if(deviceInfo[i].flags.breportDataCollected == 0)
                                        {
                                          _USBHostHID_FreeRptDecriptorDataMem(deviceInfo[i].deviceAddress);
                                          _USBHostHID_TerminateTransfer(USB_ILLEGAL_REQUEST);
                                        }
                                        else
                                        {
                                          deviceInfo[i].state = STATE_RUNNING;
                                        }
                                   }

                              }
                        }
                        else
                        {
                            // TODO assuming only a STALL here
                            // Clear the STALL.  Since it is EP0, we do not have to clear the stall.
                            USBHostClearEndpointErrors( deviceInfo[i].deviceAddress, 0 );
                        }
                        break;

                    case STATE_RUNNING:
                        // Shouldn't get any transfer events here.
                        break;
                    case STATE_READ_REQ_WAIT:
                        #ifdef DEBUG_MODE
                            UART2PrintString( "HID: Read Event\r\n" );
                        #endif
                        if (((HOST_TRANSFER_DATA *)data)->bErrorCode)
                        {
                            if (USB_ENDPOINT_STALLED == ((HOST_TRANSFER_DATA *)data)->bErrorCode)
                            {
                                     USBHostClearEndpointErrors( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA );
                                     deviceInfo[i].returnState = STATE_RUNNING;
                                     deviceInfo[i].flags.bfReset = 1;
                                     _USBHostHID_ResetStateJump( i );
                            }
                            else
                            {
                                /* Set proper error code as per HID guideline */
                                _USBHostHID_TerminateTransfer(((HOST_TRANSFER_DATA *)data)->bErrorCode);
                            }
                        }
                        else
                        {
                            USBHostClearEndpointErrors( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA );
                            deviceInfo[i].bytesTransferred = ((HOST_TRANSFER_DATA *)data)->dataCount; /* Can compare with report size and flag error ???*/
                            _USBHostHID_TerminateTransfer(USB_SUCCESS);
                            return TRUE;
                        }
                        break;
                    case STATE_WRITE_REQ_WAIT:
                        #ifdef DEBUG_MODE
                            UART2PrintString( "HID: Write Event\r\n" );
                        #endif
                        if (((HOST_TRANSFER_DATA *)data)->bErrorCode)
                        {
                            if (USB_ENDPOINT_STALLED == ((HOST_TRANSFER_DATA *)data)->bErrorCode)
                            {
                                     USBHostClearEndpointErrors( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA );
                                     deviceInfo[i].returnState = STATE_RUNNING ;
                                     deviceInfo[i].flags.bfReset = 1;
                                     _USBHostHID_ResetStateJump( i );
                            }
                            else
                            {
                                /* Set proper error code as per HID guideline */
                                _USBHostHID_TerminateTransfer(((HOST_TRANSFER_DATA *)data)->bErrorCode);
                            }
                        }
                        else
                        {
                            USBHostClearEndpointErrors( deviceInfo[i].deviceAddress, deviceInfo[i].endpointDATA );
                            deviceInfo[i].bytesTransferred = ((HOST_TRANSFER_DATA *)data)->dataCount; /* Can compare with report size and flag error ???*/
                            _USBHostHID_TerminateTransfer(USB_SUCCESS);
                            return TRUE;
                        }
                        break;
                    case STATE_WAIT_FOR_RESET:
                        #ifdef DEBUG_MODE
                            UART2PrintString( "HID: Reset Event\r\n" );
                        #endif
                        if (((HOST_TRANSFER_DATA *)data)->bErrorCode)
                        {
                            _USBHostHID_TerminateTransfer(((HOST_TRANSFER_DATA *)data)->bErrorCode);
                        }
                        else
                        {
                            deviceInfo[i].flags.bfReset = 0;
                            _USBHostHID_ResetStateJump( i );
                            return TRUE;
                        }
                        break;

                    case STATE_HOLDING:
                            return TRUE;
                        break;
                    default:
                            return FALSE;
                }
            #endif
        case EVENT_SOF:              // Start of frame - NOT NEEDED
        case EVENT_RESUME:           // Device-mode resume received
        case EVENT_SUSPEND:          // Device-mode suspend/idle event received
        case EVENT_RESET:            // Device-mode bus reset received
        case EVENT_STALL:            // A stall has occured
            return TRUE;
            break;

        default:
            return FALSE;
            break;
    }
    return FALSE;
}

/*******************************************************************************
  Function:
    BOOL USBHostHIDInitialize( BYTE address, DWORD flags )

  Summary:
    This function is the initialization routine for this client driver.

  Description:
    This function is the initialization routine for this client driver.  It
    is called by the host layer when the USB device is being enumerated.For a 
    HID device we need to look into HID descriptor, interface descriptor and 
    endpoint descriptor.

  Precondition:
    None

  Parameters:
    BYTE address        - Address of the new device
    DWORD flags          - Initialization flags

  Return Values:
    TRUE   - We can support the device.
    FALSE  - We cannot support the device.

  Remarks:
    None
*******************************************************************************/
BOOL USBHostHIDInitialize( BYTE address, DWORD flags )
{
    BYTE   *descriptor = NULL;
    BOOL    validConfiguration = FALSE;
    BOOL    rptDescriptorfound = FALSE;
    WORD    i                  = 0;
    BYTE    endpointIN         = 0;
    BYTE    endpointOUT        = 0;
    BYTE    device             = 0;
    BYTE    numofinterfaces    = 0;
    BYTE    noOfTPL            = 0;
    BYTE    temp_i             = 0;
    USB_HID_INTERFACE_DETAILS*           pNewInterfaceDetails = NULL;

    #ifdef DEBUG_MODE
        UART2PrintString( "HID: USBHostHIDInitialize(0x" );
        UART2PutHex( flags );
        UART2PrintString( ")\r\n" );
    #endif

    // Find the device in the table.  If we cannot find it, return an error.
    #ifdef DEBUG_MODE
        UART2PrintString("HID: Selecting configuration...\r\n" );
    #endif
    for (device = 0; (device < USB_MAX_HID_DEVICES) ; device++)
    {
        if(deviceInfo[device].deviceAddress == address)
            return TRUE;
    }

    for (device = 0; (device < USB_MAX_HID_DEVICES) && (deviceInfo[device].deviceAddress != 0); device++);
    if (device == USB_MAX_HID_DEVICES)
    {
        #ifdef DEBUG_MODE
            UART2PrintString("HID: Not in the table!\r\n" );
        #endif
        return FALSE;
    }

    descriptor = USBHostGetCurrentConfigurationDescriptor( address );

//    pCurrInterfaceDetails = pInterfaceDetails;
        i = 0;

        #ifdef DEBUG_MODE
            UART2PrintString("HID: Checking descriptor " );
            UART2PutDec( descriptor[i+5] );
            UART2PrintString(" ...\r\n" );
        #endif

        // Total no of interfaces
        deviceInfo[device].noOfInterfaces = descriptor[i+4] ;
        
        // Set current configuration to this configuration.  We can change it later.

        // TODO Check power requirement

        // Find the next interface descriptor.
        while (i < ((USB_CONFIGURATION_DESCRIPTOR *)descriptor)->wTotalLength)
        {
            #ifdef DEBUG_MODE
                UART2PrintString("HID:  Checking interface...\r\n" );
            #endif
            // See if we are pointing to an interface descriptor.
            if (descriptor[i+1] == USB_DESCRIPTOR_INTERFACE)
            {
                // See if the interface is a HID interface.
                if ((descriptor[i+5] == DEVICE_CLASS_HID)||(descriptor[i+5] == 0))
                {
                    // See if the interface subclass and protocol are correct.
                    // TODO What interface subclasses do we accept?
                    while (noOfTPL < NUM_TPL_ENTRIES)
                    {
                        if (usbTPL[noOfTPL].flags.bfIsClassDriver)
                        {
                            // Check for a device-class client driver
                            if ((usbTPL[noOfTPL].device.bClass    == (descriptor[i+5])) &&
                                (usbTPL[noOfTPL].device.bSubClass == (descriptor[i+6])) &&
                                (usbTPL[noOfTPL].device.bProtocol == (descriptor[i+7])))
                            {
                                deviceInfo[device].driverSupported = 1;
                            }
                        }
                        else
                        {
                            if(USBHostDeviceSpecificClientDriver(address))
                                deviceInfo[device].driverSupported = 1; // else VID & PID has already been validated in usb_host.c
                        }
                        noOfTPL++;
                    }
                    noOfTPL = 0;
                   if (deviceInfo[device].driverSupported)
                    {
                        if ((pNewInterfaceDetails = (USB_HID_INTERFACE_DETAILS*)malloc(sizeof(USB_HID_INTERFACE_DETAILS))) == NULL)
                        {
                            return FALSE;
                        }
                        numofinterfaces ++ ;

                        // Create new entry into interface list
                        if(pInterfaceDetails == NULL)
                        {
                            pInterfaceDetails       = pNewInterfaceDetails;
                            pCurrInterfaceDetails   = pNewInterfaceDetails;
                            pInterfaceDetails->next = NULL;
                        }
                        else
                        {
                            pCurrInterfaceDetails->next             = pNewInterfaceDetails;
                            pCurrInterfaceDetails                   = pNewInterfaceDetails;
                            pCurrInterfaceDetails->next             = NULL;
                        }

//                       freezHID( pNewInterfaceDetails );
                       pCurrInterfaceDetails->interfaceNumber   = descriptor[i+2];

                        // Scan for hid descriptors.
                        i += descriptor[i];
                        if(descriptor[i+1] == DSC_HID)
                        {
                            if(descriptor[i+5] == 0)
                            {
                                // Atleast one report descriptor must be present - flag error
                                rptDescriptorfound = FALSE;
                                _USBHostHID_TerminateTransfer( USB_HID_NO_REPORT_DESCRIPTOR );
                            }
                            else
                            {
                                rptDescriptorfound = TRUE;
                                pCurrInterfaceDetails->sizeOfRptDescriptor = ((descriptor[i+7]) |
                                                                                        ((descriptor[i+8]) << 8));
                            
                                // Look for IN and OUT endpoints.
                                // TODO what if there are no endpoints?
                                endpointIN  = 0;
                                endpointOUT = 0;
                                temp_i = 0;
                                // Scan for endpoint descriptors.
                                i += descriptor[i];
                                while(descriptor[i+1] == USB_DESCRIPTOR_ENDPOINT)
                                {
                                    if (descriptor[i+3] == 0x03) // Interrupt
                                    {
                                        if (((descriptor[i+2] & 0x80) == 0x80) && (endpointIN == 0))
                                        {
                                            endpointIN = descriptor[i+2];
                                        }
                                        if (((descriptor[i+2] & 0x80) == 0x00) && (endpointOUT == 0))
                                        {
                                            endpointOUT = descriptor[i+2];
                                        }
                                    }
                                    temp_i = descriptor[i];
                                    i += descriptor[i];
                                }
                
                                if ((endpointIN != 0) || (endpointOUT != 0))
                                {
                                    // Initialize the remaining device information.
                                    deviceInfo[device].deviceAddress        = address;
//              
                                    pCurrInterfaceDetails->endpointIN           = endpointIN;
                                    pCurrInterfaceDetails->endpointOUT          = endpointOUT;
                                    pCurrInterfaceDetails->endpointMaxDataSize  = ((descriptor[i+4]) |
                                                                               (descriptor[i+5] << 8));
                                    pCurrInterfaceDetails->endpointPollInterval = descriptor[i+6];
                                    validConfiguration = TRUE;
                                    
                                    /* By default NAK time out is disabled for HID class */
                                    /* If timeout is required then pass '1' instead '0' in below function call */
//                                    USBHostSetNAKTimeout( address, endpointIN,  0, USB_NUM_INTERRUPT_NAKS );
//                                    USBHostSetNAKTimeout( address, endpointOUT, 0, USB_NUM_INTERRUPT_NAKS );
                                }
                                i -= temp_i;
                            
                            }
                        }
                        else
                        {
                            /* HID Descriptor not found */
                        }
                    }
                }
            }

            // Jump to the next descriptor in this configuration.
            i += descriptor[i];
        }

    if(numofinterfaces != deviceInfo[device].noOfInterfaces)
    {
        validConfiguration = FALSE;
    }
   // We cannot find a valid configuration.  Set it to 0.
   if(validConfiguration)
    {
           
        #ifndef USB_ENABLE_TRANSFER_EVENT
            deviceInfo[device].state                = STATE_INITIALIZE_DEVICE;
        #else
            pCurrInterfaceDetails = pInterfaceDetails;
            if(pCurrInterfaceDetails->sizeOfRptDescriptor !=0)
            {
                if (deviceInfo[device].rptDescriptor != NULL)
                {
                    freezHID( deviceInfo[device].rptDescriptor );
                }

               if((deviceInfo[device].rptDescriptor = (BYTE *)malloc(pCurrInterfaceDetails->sizeOfRptDescriptor)) == NULL)
                  {
                     _USBHostHID_TerminateTransfer(USB_MEMORY_ALLOCATION_ERROR);
                      return FALSE;
                   }
            }
            if (USBHostDeviceRequest( deviceInfo[device].deviceAddress, USB_SETUP_DEVICE_TO_HOST | USB_SETUP_TYPE_STANDARD | USB_SETUP_RECIPIENT_INTERFACE, 
                 USB_REQUEST_GET_DESCRIPTOR, DSC_RPT, pCurrInterfaceDetails->interfaceNumber, pCurrInterfaceDetails->sizeOfRptDescriptor, deviceInfo[device].rptDescriptor,
                 USB_DEVICE_REQUEST_GET ))
            {           
                free(deviceInfo[device].rptDescriptor);
                return FALSE;
            }
            deviceInfo[device].state                = STATE_WAIT_FOR_REPORT_DSC;
        #endif

            #ifdef DEBUG_MODE
                UART2PrintString( "HID: Interrupt endpoint IN: " );
                UART2PutHex( endpointIN );
                UART2PrintString( " Interrupt endpoint OUT: " );
                UART2PutHex( endpointOUT );
                UART2PrintString( "\r\n" );
            #endif

        return TRUE;
    }
   else
    {
       return FALSE;
    }

}


// *****************************************************************************
// *****************************************************************************
// Internal Functions
// *****************************************************************************
// *****************************************************************************

/****************************************************************************
  Function:
    void _USBHostHID_FreeRptDecriptorDataMem(BYTE deviceAddress)

  Summary:


  Description:
    This function is called in case of error is encountered . This function
    frees the memory allocated for report descriptor and interface
    descriptors.

  Precondition:
    None.

  Parameters:
    BYTE address        - Address of the device

  Returns:
    None

  Remarks:
    None
***************************************************************************/
void _USBHostHID_FreeRptDecriptorDataMem(BYTE deviceAddress)
{
    USB_HID_INTERFACE_DETAILS*           ptempInterface;
    BYTE    i;

    // Find the device in the table.  If found, clear the important fields.
    for (i=0; (i<USB_MAX_HID_DEVICES) && (deviceInfo[i].deviceAddress != deviceAddress); i++);
    if (i < USB_MAX_HID_DEVICES)
    {
    /* free memory allocated to HID parser */
        free(parsedDataMem); /* will be indexed once multiple  device support is added */
        parsedDataMem = NULL;
        free(deviceInfo[i].rptDescriptor);
        deviceInfo[i].rptDescriptor = NULL;

    /* free memory allocated to report descriptor in deviceInfo */
       while(pInterfaceDetails != NULL)
        {
            ptempInterface = pInterfaceDetails->next;
//            pCurrInterfaceDetails = pInterfaceDetails;
            freezHID(pInterfaceDetails);
            freezHID(pCurrInterfaceDetails);
            pInterfaceDetails = ptempInterface;
            pCurrInterfaceDetails = pInterfaceDetails;
        }
    }
}

/*******************************************************************************
  Function:
    void _USBHostHID_ResetStateJump( BYTE i )

  Summary:

  Description:
    This function determines which portion of the reset processing needs to
    be executed next and jumps to that state.

Precondition:
    The device information must be in the deviceInfo array.

  Parameters:
    BYTE i  - Index into the deviceInfoMSD structure for the device to reset.

  Returns:
    None

  Remarks:
    None
*******************************************************************************/
void _USBHostHID_ResetStateJump( BYTE i )
{
     #ifdef USB_ENABLE_TRANSFER_EVENT
        BYTE    errorCode;
     #endif
   if (deviceInfo[i].flags.bfReset)
    {
        #ifndef USB_ENABLE_TRANSFER_EVENT
            deviceInfo[i].state = STATE_HID_RESET_RECOVERY;
        #else
             errorCode = USBHostDeviceRequest( deviceInfo[i].deviceAddress, USB_SETUP_HOST_TO_DEVICE | USB_SETUP_TYPE_CLASS | USB_SETUP_RECIPIENT_INTERFACE,
                            USB_HID_RESET, 0, deviceInfo[i].interface, 0, NULL, USB_DEVICE_REQUEST_SET );

            if (errorCode)
            {
                //TODO Now what??
                _USBHostHID_TerminateTransfer( USB_HID_RESET_ERROR );
            }
            else
            {
                deviceInfo[i].state = STATE_WAIT_FOR_RESET;
            }
        #endif
    }
    else
    {
         usbDeviceInterfaceTable.EventHandler(deviceInfo[i].deviceAddress, EVENT_HID_RESET, NULL, 0 );

         deviceInfo[i].state = deviceInfo[i].returnState;
    }
}


