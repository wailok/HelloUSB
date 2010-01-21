/******************************************************************************

  USB Host HID Application Interface

This is the Human Interface Device Class application interface file for a USB
Embedded Host device. This file should be used in a project with usb_host_hid.c
to provided the functional interface.

Acronyms/abbreviations used by this class:
    * HID - Human Interface Device

This file provides interface between HID application and driver. This file
also provides interface functions of HID parser. These interfaces must be used
to access the data structures populated by HID parser. These data structures
hold crucial information required by application to understand the nature of
device connected on the bus. The file also contains interface functions to
send/recieve Reports to/from device.

* FileName:        usb_host_hid_appl_interface.c
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
#include "GenericTypeDefs.h"
#include "HardwareProfile.h"
#include "usb_config.h"
#include "USB\usb.h"
#include "USB\usb_host_hid.h"
#include "USB\usb_host_hid_parser.h"
#include "USB\usb_host_hid_appl_interface.h"
#include <stdlib.h>
#include <string.h>

//#define DEBUG_MODE
#ifdef DEBUG_MODE
    #include "uart2.h"
#endif

//#define DEBUG_MODE
#ifdef DEBUG_MODE
    #include "uart2.h"
#endif

// *****************************************************************************
// *****************************************************************************
// Section: Constants
// *****************************************************************************
// *****************************************************************************


//******************************************************************************
//******************************************************************************
// Section: Data Structures
//******************************************************************************
//******************************************************************************

//******************************************************************************
//******************************************************************************
// Section: Local Prototypes
//******************************************************************************
//******************************************************************************
BOOL APPL_COLLECT_PARSED_DATA(void);

//******************************************************************************
//******************************************************************************
// Section: HID Host Global Variables
//******************************************************************************
//******************************************************************************

BYTE HIDdeviceAddress = 0;

// *****************************************************************************
// *****************************************************************************
// HID Host Stack Callback Functions
// *****************************************************************************
// *****************************************************************************

/****************************************************************************
  Function:
    BOOL USBHostHID_APIInitialize( BYTE address, DWORD flags )

  Description:
    This function is called when a USB HID device is being 
    enumerated.

  Precondition:
    None

  Parameters:
    BYTE address    -   Address of the new device
    DWORD flags     -   Initialization flags


  Return Values:
    TRUE    -   We can support the device.
    FALSE   -   We cannot support the device.

  Remarks:
    None
***************************************************************************/
BOOL USBHostHID_APIInitialize( BYTE address, DWORD flags )
{
    #ifdef DEBUG_MODE
        UART2PrintString( "HID: Device attached.\r\n" );
    #endif

    if (HIDdeviceAddress == 0)
    {
        // Save the address of the new device.
        HIDdeviceAddress = address;
        return TRUE;
    }
    else
    {
        // We can currently only handle one device.
        return FALSE;
    }        
}    

/****************************************************************************
  Function:
    BOOL USBHostHID_APIEventHandler( BYTE address, USB_HID_EVENT event, 
                                     void *data, DWORD size )

  Description:
    This function is called when various events occur in the
    USB HID Host layer.

  Precondition:
    The device has been initialized.

  Parameters:
    BYTE address    -   Address of the device
    USB_EVENT event -   Event that has occurred
    void *data      -   Pointer to data pertinent to the event
    DWORD size      -   Size of the data

  Return Values:
    TRUE    -   Event was handled
    FALSE   -   Event was not handled

  Remarks:
    None
***************************************************************************/
BOOL USBHostHID_APIEventHandler( BYTE address, USB_EVENT event, void *data, DWORD size )
{
    if (HIDdeviceAddress == address)
    {
        switch( event )
        {
            case EVENT_HID_NONE:
            case EVENT_HID_TRANSFER:                 // A HID transfer has completed
                return TRUE;
                break;
            case EVENT_HID_WRITE_DONE:
                                     // Applicaton can insert a function call to perform tasks after
                                     // WRITE request is done.
                return TRUE;
                break;
            case EVENT_HID_READ_DONE:
                                     // Applicaton can insert a function call to perform tasks after
                                     // READ request is done.
                return TRUE;
                break;

            case EVENT_HID_RPT_DESC_PARSED: // Application function call to collect data from parsed report descriptor
                                          #ifndef APPL_COLLECT_PARSED_DATA
                                              return TRUE;  // Driver assumes application is aware of report format 
                                          #else
                                              if(APPL_COLLECT_PARSED_DATA())
                                                 return TRUE;
                                              else
                                                 return FALSE;
                                          #endif
                break;
            
            case EVENT_HUB_ATTACH:               // USB hub has been attached
                #ifdef DEBUG_MODE
                    UART2PrintString( "API: Hub use is not supported.\r\n" );
                #endif
                return TRUE;
                break;

            case EVENT_HID_RESET:
                #ifdef DEBUG_MODE
                    UART2PrintString( "API: HID Reset performed.\r\n" );
                #endif
                return TRUE;
                break;
            
            case EVENT_DETACH:                   // USB cable has been detached (data: BYTE, address of device)
                #ifdef DEBUG_MODE
                    UART2PrintString( "API: Device detached.\r\n" );
                #endif
                HIDdeviceAddress = 0;
                return TRUE;
                break;
                
            default:         
                return FALSE;
                break;
        }
    }
    return FALSE;
}

// *****************************************************************************
// *****************************************************************************
// HID Application Callable Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    BOOL USBHostHID_ApiDeviceDetect( void )

  Description:
    This function determines if a HID device is attached
    and ready to use.

  Precondition:
    None

  Parameters:
    None

  Return Values:
    TRUE   -  HID present and ready
    FALSE  -  HID not present or not ready

  Remarks:
    Since this will often be called in a loop while waiting for
    a device, we'll make sure the tasks are executed.
*******************************************************************************/
BOOL USBHostHID_ApiDeviceDetect( void )
{
    USBHostTasks();
    USBHostHIDTasks();

    #ifdef DEBUG_MODE
//        UART2PrintString( "HID: State = " );
//        UART2PutHex( USBHostHIDDeviceStatus(HIDdeviceAddress) );
//        UART2PrintString( "  Address = " );
//        UART2PutHex( HIDdeviceAddress );
//        UART2PrintString( "\r\n" );
    #endif
    
    if ((USBHostHIDDeviceStatus(HIDdeviceAddress) == USB_HID_NORMAL_RUNNING) &&
        (HIDdeviceAddress != 0))
    {
        return TRUE;
    }
    
    return FALSE;
}

/*******************************************************************************
  Function:
    BYTE USBHostHID_ApiGetReport(BYTE reportid, BYTE interfaceNum, BYTE size,
                                BYTE* data)

  Description:
    This function is called by application to receive Input report from device

  Precondition:
    None

  Parameters:
    BYTE reportid           - Report ID of the requested report
    BYTE interfaceNum       - Interface number of the device 
    BYTE size               - Byte size of the data buffer
    BYTE *data              - Pointer to the data buffer

  Return Values:
    USB_SUCCESS                 - Request started successfully
    USB_HID_DEVICE_NOT_FOUND    - No device with specified address
    USB_HID_DEVICE_BUSY         - Device not in proper state for
                                  performing a transfer

  Remarks:
    Device must be connected, host application is responsible for scheduling 
    messages at required rate.
*******************************************************************************/
BYTE USBHostHID_ApiGetReport(WORD reportid, BYTE interfaceNum, BYTE size,BYTE* data)
{
    return(USBHostHIDRead(HIDdeviceAddress,reportid,interfaceNum,size,data));
}

/*******************************************************************************
  Function:
    BYTE USBHostHID_ApiSendReport(BYTE reportid, BYTE interfaceNum, BYTE size,
                                BYTE* data)

  Description:
    This function is called by application to send Output report to device.

  Precondition:
    None

  Parameters:
    BYTE reportid           - Report ID of the requested report
    BYTE interfaceNum       - Interface number of the device
    BYTE size               - Byte size of the data buffer
    BYTE *data              - Pointer to the data buffer

  Return Values:
    USB_SUCCESS                 - Request started successfully
    USB_HID_DEVICE_NOT_FOUND    - No device with specified address
    USB_HID_DEVICE_BUSY         - Device not in proper state for
                                  performing a transfer

  Remarks:
    Device must be connected, host application is responsible for scheduling
    messages at required rate.
*******************************************************************************/
BYTE USBHostHID_ApiSendReport(WORD reportid, BYTE interfaceNum, BYTE size,BYTE* data)
{
    return(USBHostHIDWrite(HIDdeviceAddress,reportid,interfaceNum,size,data));
}

/****************************************************************************
  Function:
    BOOL USBHostHID_ApiResetDevice( void  )

  Summary:
    This function resets the HID device.

  Description:
    This function resets the HID device attached on the bus.  It is called if
    an operation returns an error or the application can call it in case of
    error.

  Precondition:
    None

  Parameters:
    None - None

  Return Values:
    USB_SUCCESS                 - Reset successful
    USB_HID_DEVICE_NOT_FOUND    - No device with specified address
    USB_ILLEGAL_REQUEST         - Device is in an illegal USB state
                                  for reset

  Remarks:
    None
  ***************************************************************************/

BYTE USBHostHID_ApiResetDevice( void  )
{
    DWORD   byteCount;
    BYTE    errorCode;

    errorCode = USBHostHIDResetDevice( HIDdeviceAddress );
    if (errorCode)
    {
        return errorCode;
    }

    do
    {
        USBTasks();
        errorCode = USBHostHIDDeviceStatus( HIDdeviceAddress );
    } while (errorCode == USB_HID_RESETTING_DEVICE);


    if (USBHostHIDTransferIsComplete( HIDdeviceAddress, &errorCode, &byteCount ))
    {
        return errorCode;
    }

    return USB_HID_RESET_ERROR;
}

/*******************************************************************************
  Function:
    BOOL USBHostHID_ApiTransferIsComplete(BYTE *errorCodeDriver, 
                                          DWORD *byteCount )

  Summary:
    This function indicates whether or not the last transfer is complete.

  Description:
    This function indicates whether or not the last transfer is complete.
    If the functions returns TRUE, the returned byte count and error
    code are valid.

  Precondition:
    None

  Parameters:
    BYTE *errorCode     - Error code from last transfer
    DWORD *byteCount    - Number of bytes transferred

  Return Values:
    TRUE    - Transfer is complete, errorCode is valid
    FALSE   - Transfer is not complete, errorCode is not valid

  Remarks:
    None
*******************************************************************************/
BOOL USBHostHID_ApiTransferIsComplete(BYTE* errorCodeDriver, BYTE* byteCount )
{       
    return(USBHostHIDTransferIsComplete(HIDdeviceAddress,errorCodeDriver,byteCount));
}

/*******************************************************************************
  Function:
    BYTE USBHostHID_ApiGetCurrentInterfaceNum(void)

  Description:
    This function reurns the interface number of the cuurent report descriptor
    parsed. This function must be called to fill data interface detail data
    structure and passed as parameter when requesinf for report transfers.

  Precondition:
    None

  Parameters:
    BYTE *errorCode     - Error code from last transfer
    DWORD *byteCount    - Number of bytes transferred

  Return Values:
    TRUE    - Transfer is complete, errorCode is valid
    FALSE   - Transfer is not complete, errorCode is not valid

  Remarks:
    None
*******************************************************************************/
BYTE USBHostHID_ApiGetCurrentInterfaceNum(void)
{
    return(deviceRptInfo.interfaceNumber);
}

/*******************************************************************************
  Function:
    BOOL USBHostHID_ApiFindBit(WORD usagePage,WORD usage,HIDReportTypeEnum type,
                          BYTE* Report_ID, BYTE* Report_Length, BYTE* Start_Bit)

  Description:
    This function is used to locate a specific button or indicator.
    Once the report descriptor is parsed by the HID layer without any error,
    data from the report descriptor is stored in pre defined dat structures.
    This function traverses these data structure and exract data required
    by application

  Precondition:
    None

  Parameters:
    WORD usagePage         - usage page supported by application
    WORD usage             - usage supported by application
    HIDReportTypeEnum type - report type Input/Output for the particular
                             usage
    BYTE* Report_ID        - returns the report ID of the required usage
    BYTE* Report_Length    - returns the report length of the required usage
    BYTE* Start_Bit        - returns  the start bit of the usage in a
                             particular report

  Return Values:
    TRUE    - If the required usage is located in the report descriptor
    FALSE   - If the application required usage is not supported by the 
              device(i.e report descriptor).

  Remarks:
    Application event handler with event 'EVENT_HID_RPT_DESC_PARSED' is called.
    Application is suppose to fill in data details structure 'HID_DATA_DETAILS'
    This function can be used to the get the details of the required usages.
*******************************************************************************/
BOOL USBHostHID_ApiFindBit(WORD usagePage,WORD usage,HIDReportTypeEnum type,BYTE* Report_ID,
                    BYTE* Report_Length, BYTE* Start_Bit)
{
    WORD iR;
    WORD index;
    WORD reportIndex;
    HID_REPORTITEM *reportItem;
    BYTE* count;

//  Disallow Null Pointers

    if((Report_ID == NULL)|(Report_Length == NULL)|(Start_Bit == NULL))
        return FALSE;

//  Search through all the report items

    for (iR=0; iR < deviceRptInfo.reportItems; iR++)
        {
            reportItem = &itemListPtrs.reportItemList[iR];

//      Search only reports of the proper type

            if ((reportItem->reportType==type))// && (reportItem->globals.reportsize == 1))
                {
                    if (USBHostHID_HasUsage(reportItem,usagePage,usage,(WORD*)&index,(BYTE*)&count))
                        {
                             reportIndex = reportItem->globals.reportIndex;
                             *Report_ID = itemListPtrs.reportList[reportIndex].reportID;
                             *Start_Bit = reportItem->startBit + index;
                             if (type == hidReportInput)
                                 *Report_Length = (itemListPtrs.reportList[reportIndex].inputBits + 7)/8;
                             else if (type == hidReportOutput)
                                 *Report_Length = (itemListPtrs.reportList[reportIndex].outputBits + 7)/8;
                             else
                                 *Report_Length = (itemListPtrs.reportList[reportIndex].featureBits + 7)/8;
                             return TRUE;
                         }
                }
        }
    return FALSE;
}

/*******************************************************************************
  Function:
    BOOL USBHostHID_ApiFindValue(WORD usagePage,WORD usage,
                HIDReportTypeEnum type,BYTE* Report_ID,BYTE* Report_Length,BYTE*
                Start_Bit, BYTE* Bit_Length)

  Description:
    Find a specific Usage Value. Once the report descriptor is parsed by the HID
    layer without any error, data from the report descriptor is stored in
    pre defined dat structures. This function traverses these data structure and
    exract data required by application.

  Precondition:
    None

  Parameters:
    WORD usagePage         - usage page supported by application
    WORD usage             - usage supported by application
    HIDReportTypeEnum type - report type Input/Output for the particular
                             usage
    BYTE* Report_ID        - returns the report ID of the required usage
    BYTE* Report_Length    - returns the report length of the required usage
    BYTE* Start_Bit        - returns  the start bit of the usage in a
                             particular report
    BYTE* Bit_Length       - returns size of requested usage type data in bits

  Return Values:
    TRUE    - If the required usage is located in the report descriptor
    FALSE   - If the application required usage is not supported by the 
              device(i.e report descriptor).

  Remarks:
    Application event handler with event 'EVENT_HID_RPT_DESC_PARSED' is called.
    Application is suppose to fill in data details structure 'HID_DATA_DETAILS'
    This function can be used to the get the details of the required usages.
*******************************************************************************/
BOOL USBHostHID_ApiFindValue(WORD usagePage,WORD usage,HIDReportTypeEnum type,BYTE* Report_ID,
                    BYTE* Report_Length,BYTE* Start_Bit, BYTE* Bit_Length)
{
    WORD index;
    WORD reportIndex;
    BYTE iR;
    BYTE count;
    HID_REPORTITEM *reportItem;

//  Disallow Null Pointers

     if((Report_ID == NULL)|(Report_Length == NULL)|(Start_Bit == NULL)|(Bit_Length == NULL))
        return FALSE;

//  Search through all the report items

    for (iR=0; iR < deviceRptInfo.reportItems; iR++)
    {
        reportItem = &itemListPtrs.reportItemList[iR];

//      Search only reports of the proper type

        if ((reportItem->reportType==type)&& ((reportItem->dataModes & HIDData_ArrayBit) != HIDData_Array)
             && (reportItem->globals.reportsize != 1))
        {
            if (USBHostHID_HasUsage(reportItem,usagePage,usage,&index,&count))
            {
                 reportIndex = reportItem->globals.reportIndex;
                 *Report_ID = itemListPtrs.reportList[reportIndex].reportID;
                 *Bit_Length = reportItem->globals.reportsize;
                 *Start_Bit = reportItem->startBit + index * (reportItem->globals.reportsize);
                 if (type == hidReportInput)
                     *Report_Length = (itemListPtrs.reportList[reportIndex].inputBits + 7)/8;
                 else if (type == hidReportOutput)
                     *Report_Length = (itemListPtrs.reportList[reportIndex].outputBits + 7)/8;
                 else
                     *Report_Length = (itemListPtrs.reportList[reportIndex].featureBits + 7)/8;
                 return TRUE;
             }
        }
    }
    return FALSE;
}




/*******************************************************************************
  Function:
    BOOL USBHostHID_ApiImportData(BYTE *report, WORD reportLength, 
                     HID_USER_DATA_SIZE *buffer,HID_DATA_DETAILS *pDataDetails)
  Description:
    This function can be used by application to extract data from the input 
    reports. On receiving the input report from the device application can call
    the function with required inputs 'HID_DATA_DETAILS'.

  Precondition:
    None

  Parameters:
    BYTE *report                    - Input report received from device
    WORD reportLength               - Length of input report report
    HID_USER_DATA_SIZE *buffer      - Buffer into which data needs to be
                                      populated
    HID_DATA_DETAILS *pDataDetails  - data details extracted from report
                                      descriptor
  Return Values:
    TRUE    - If the required data is retrieved from the report
    FALSE   - If required data is not found.

  Remarks:
    None
*******************************************************************************/
BOOL USBHostHID_ApiImportData(BYTE *report, WORD reportLength, HID_USER_DATA_SIZE *buffer, HID_DATA_DETAILS *pDataDetails)
{
    WORD data;
    WORD signBit;
    WORD mask;
    WORD extendMask;
    WORD start;
    WORD startByte;
    WORD startBit;
    WORD lastByte;
    WORD i;

//  Report must be ok

    if (report == NULL) return FALSE;

//  Must be the right report

    if ((pDataDetails->reportID != 0) && (pDataDetails->reportID != report[0])) return FALSE;

//  Length must be ok

    if (pDataDetails->reportLength != reportLength) return FALSE;
    lastByte = (pDataDetails->bitOffset + (pDataDetails->bitLength * pDataDetails->count) - 1)/8;
    if (lastByte > reportLength) return FALSE;

//  Extract data one count at a time

    start = pDataDetails->bitOffset;
    for (i=0; i<pDataDetails->count; i++) {
        startByte = start/8;
        startBit = start&7;
        lastByte = (start + pDataDetails->bitLength - 1)/8;

//      Pick up the data bytes backwards

        data = 0;
        do {
            data <<= 8;
            data |= (int) report[lastByte];
        }
        while (lastByte-- > startByte);

//      Shift to the right to byte align the least significant bit

        if (startBit > 0) data >>= startBit;

//      Done if 16 bits long

        if (pDataDetails->bitLength < 16) {

//          Mask off the other bits

            mask = 1 << pDataDetails->bitLength;
            mask--;
            data &= mask;

//          Sign extend the report item

            if (pDataDetails->signExtend) {
                signBit = 1;
                if (pDataDetails->bitLength > 1) signBit <<= (pDataDetails->bitLength-1);
                extendMask = (signBit << 1) - 1;
                if ((data & signBit)==0) data &= extendMask;
                else data |= ~extendMask;
            }
        }

//      Save the value

        *buffer++ = data;

//      Next one

        start += pDataDetails->bitLength;
    }
    return TRUE;
}
