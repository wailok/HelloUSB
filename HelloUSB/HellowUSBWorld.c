
#include "GenericTypeDefs.h"
#include "Compiler.h"
#include "usb_config.h"
#include "USB/usb_device.h"
#include "USB/usb.h"
#include "USB/usb_function_cdc.h"
#include "HardwareProfile.h"
#include "HelloUSBWorld.h"

// Let compile time pre-processor calculate the CORE_TICK_PERIOD
#define SYS_FREQ 				(80000000L)
#define TOGGLES_PER_SEC			1000
#define CORE_TICK_RATE	       (SYS_FREQ/2/TOGGLES_PER_SEC)

/** P R I V A T E  P R O T O T Y P E S ***************************************/

/** P R I V A T E  V A R I A B L E S *****************************************/


// Decriments every 1 ms.
volatile static unsigned int OneMSTimer;
int countI = 0;
int boolA = 0;
int boolB = 0;
int direction = 0;
char USB_In_Buffer[64];
char USB_Out_Buffer[64];

/** D E C L A R A T I O N S **************************************************/

/******************************************************************************
 * Function:        void UserInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine should take care of all of the demo code
 *                  initialization that is required.
 *
 * Note:            
 *
 *****************************************************************************/
void UserInit(void)
{
    //Initialize all of the LED pins
	mInitAllLEDs();

	// Open up the core timer at our 1ms rate
	OpenCoreTimer(CORE_TICK_RATE);

    // set up the core timer interrupt with a prioirty of 2 and zero sub-priority
	mConfigIntCoreTimer((CT_INT_ON | CT_INT_PRIOR_2 | CT_INT_SUB_PRIOR_0));

	// set up change notice
	CNCON = 0x8000;
	CNEN = 0xFFFFFFFF;

    // enable multi-vector interrupts
	INTEnableSystemMultiVectoredInt();
	

}//end UserInit

/********************************************************************
 * Function:        void ProcessIO(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is a place holder for other user
 *                  routines. It is a mixture of both USB and
 *                  non-USB tasks.
 *
 * Note:            None
 *******************************************************************/
void ProcessIO()
{
	unsigned char numBytesRead;
	int intBytes;

    //Blink the LEDs according to the USB device status
    BlinkUSBStatus();

    // User Application USB tasks
    if (
    	(USBDeviceState < CONFIGURED_STATE)
    	||
    	(USBSuspendControl == 1)
    )
    {
	    return;
	}

	// Pull in some new data if there is new data to pull in
	numBytesRead = getsUSBUSART (USB_In_Buffer,64);
	
	/*if (!swUser)
	{
		sprintf (USB_Out_Buffer, "Button Pressed. Hello!\r\n");
		putUSBUSART (USB_Out_Buffer, strlen (USB_Out_Buffer));
		mLED_2_Toggle();
		mLED_3_On();
		OneMSTimer = 1000;
	}*/

	/*if (!swUser && numBytesRead != 0)
	{
		sprintf (USB_Out_Buffer, "Button PRESSED. Hello!\r\n");
		putUSBUSART (USB_Out_Buffer, strlen (USB_Out_Buffer));
		mLED_2_Toggle();
		mLED_3_On();
		OneMSTimer = 1000;
	}

	if (swUser && numBytesRead != 0)
	
{
		intBytes = (int)numBytesRead;
		sprintf (USB_Out_Buffer, "%s", USB_In_Buffer);
		putUSBUSART (USB_Out_Buffer, strlen (USB_Out_Buffer));
		mLED_2_Toggle();
		mLED_3_On();
		OneMSTimer = 1000;
	}

	if (!OneMSTimer)
	{
		mLED_3_Off();
	}*/

	/*while (! _RA0);
		
	mLED_1_Off();
	mLED_2_Off();
	mLED_3_Off();
	mLED_4_Off();

	count++;

	sprintf (USB_Out_Buffer, "%i\n",count);
	putUSBUSART (USB_Out_Buffer, strlen (USB_Out_Buffer));
	OneMSTimer = 1000;

	while (_RA0);

	mLED_1_On ();*/
	
	if (boolA)
	{
		mLED_2_On ();
	}
	else
	{
		mLED_2_Off();	
	}
	if (boolB)
	{
		mLED_3_On ();
	}
	else
	{
		mLED_3_Off();	
	}
	if (direction)
	{
		mLED_4_On ();
	}
	else
	{
		mLED_4_Off();	
	}

	OneMSTimer = 1000;
	sprintf (USB_Out_Buffer, "%i, %i, ,%i, %i\n",countI, boolA, boolB,direction);
	putUSBUSART (USB_Out_Buffer, strlen (USB_Out_Buffer));
	mLED_1_Off();
	CDCTxService();
	return;  	

} //end ProcessIO

void __ISR(_CORE_TIMER_VECTOR, ipl2) CoreTimerHandler(void)
{
	asm( "ei" );
    // clear the interrupt flag
    mCTClearIntFlag();

	if (OneMSTimer)
	{
		OneMSTimer--;
	}

    // update the period
    UpdateCoreTimer(CORE_TICK_RATE);
}

void __ISR(_CHANGE_NOTICE_VECTOR, ipl3) ChangeNoticeHandler(void)
{
    // clear the interrupt flag
	//unsigned int value = PORTG;
    mCNClearIntFlag();
	if (_RG7)//value & 0x00000080
	{
		countI++;
		mLED_1_On ();
	}
	else if (_RG8)//value & 0x00000100
	{
		boolA =! boolA;
		if (boolB && boolA)
		{
			direction = 1;
		}
		else if (!boolB && boolA)
		{
			direction = 0;
		}
		else if (boolB && !boolA)
		{
			direction = 0;
		}
		else
		{
			direction = 1;
		}
	}
	else if (_RG9)//value & 0x00000200
	{
		boolB =! boolB;
		if (boolB && boolA)
		{
			direction = 0;
		}
		else if (!boolB && boolA)
		{
			direction = 1;
		}
		else if (boolB && !boolA)
		{
			direction = 1;
		}
		else
		{
			direction = 0;
		}		
	}

}

