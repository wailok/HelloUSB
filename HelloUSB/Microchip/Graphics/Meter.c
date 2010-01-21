/*****************************************************************************
 *  Module for Microchip Graphics Library
 *  GOL Layer 
 *  Meter
 *****************************************************************************
 * FileName:        Meter.c
 * Dependencies:    Meter.h
 * Processor:       PIC24, PIC32
 * Compiler:       	MPLAB C30 Version 3.00, MPLAB C32
 * Linker:          MPLAB LINK30, MPLAB LINK32
 * Company:         Microchip Technology Incorporated
 *
 * Software License Agreement
 *
 * Copyright © 2008 Microchip Technology Inc.  All rights reserved.
 * Microchip licenses to you the right to use, modify, copy and distribute
 * Software only when embedded on a Microchip microcontroller or digital
 * signal controller, which is integrated into your product or third party
 * product (pursuant to the sublicense terms in the accompanying license
 * agreement).  
 *
 * You should refer to the license agreement accompanying this Software
 * for additional information regarding your rights and obligations.
 *
 * SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS” WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY
 * OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR
 * PURPOSE. IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR
 * OBLIGATED UNDER CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION,
 * BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT
 * DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL,
 * INDIRECT, PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA,
 * COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY
 * CLAIMS BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF),
 * OR OTHER SIMILAR COSTS.
 *
 * Author               Date        Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* Paolo A. Tamayo		11/12/07	Version 1.0 release
 *****************************************************************************/

#include "Graphics\Graphics.h"
#include <math.h>
#include <stdio.h>

#ifdef USE_METER

/*********************************************************************
* This is the sine lookup table used to draw the meter and update
* the position of the needle. 
*********************************************************************/
const char _sineTable[] __attribute__  ((aligned(2))) = {
	0x00,0x03,0x06,0x09,0x0c,0x10,0x13,0x16, 
	0x19,0x1c,0x1f,0x22,0x25,0x28,0x2b,0x2e,
	0x31,0x33,0x36,0x39,0x3c,0x3f,0x41,0x44,
	0x47,0x49,0x4c,0x4e,0x51,0x53,0x55,0x58,
	0x5a,0x5c,0x5e,0x60,0x62,0x64,0x66,0x68,
	0x6a,0x6b,0x6d,0x6f,0x70,0x71,0x73,0x74,
	0x75,0x76,0x78,0x79,0x7a,0x7a,0x7b,0x7c,
	0x7d,0x7d,0x7e,0x7e,0x7e,0x7f,0x7f,0x7f,
	0x7f
};

/*********************************************************************
* Function: METER  *MtrCreate(WORD ID, SHORT left, SHORT top, SHORT right, 
*							  SHORT bottom, WORD state, SHORT value, 
*							  SHORT range, XCHAR *pText, GOL_SCHEME *pScheme)				 
*
*
* Notes: Creates a METER object and adds it to the current active list.
*        If the creation is successful, the pointer to the created Object 
*        is returned. If not successful, NULL is returned.
*
********************************************************************/
METER  *MtrCreate(WORD ID, SHORT left, SHORT top, SHORT right, SHORT bottom,
				  WORD state, SHORT value, SHORT range, 
				  XCHAR *pText, GOL_SCHEME *pScheme)				 
{
	METER *pMtr = NULL;
	SHORT tempHeight, tempWidth;
	XCHAR tempChar[2] = {'8',0};
	
	pMtr = malloc(sizeof(METER));
	if (pMtr == NULL) 
		return NULL;
	
	pMtr->hdr.ID      	= ID;					// unique id assigned for referencing
	pMtr->hdr.pNxtObj 	= NULL;					// initialize pointer to NULL
	pMtr->hdr.type    	= OBJ_METER;	 	    // set object type
	pMtr->hdr.left      = left;    	  	    	// left,top coordinate
	pMtr->hdr.top       = top;    	  	    	// 
	pMtr->hdr.right     = right;   	  	    	// right,bottom coordinate
	pMtr->hdr.bottom    = bottom;  	  	    	// 
	pMtr->range		= range;
	pMtr->value		= value;
    pMtr->hdr.state   	= state; 	            // state
    pMtr->pText 	= pText;
    
    // set the default scale colors
    pMtr->normalColor = BRIGHTGREEN; 
	pMtr->criticalColor = BRIGHTYELLOW; 
	pMtr->dangerColor = BRIGHTRED; 

	// get the text width reference. This is used to scale the meter
	if (pText != NULL) {
		tempHeight = (GOL_EMBOSS_SIZE)+GetTextHeight(pScheme->pFont);
	}
	else {
		tempHeight = GOL_EMBOSS_SIZE;
	}
	tempWidth = (GOL_EMBOSS_SIZE)+(GetTextWidth(tempChar,pScheme->pFont)*SCALECHARCOUNT);
		
	// Meter size is dependent on the width or height.
	// The radius is also adjusted to add space for the scales 

#if (METER_TYPE == MTR_WHOLE_TYPE) 
	// choose the radius 
	if ((right-left-tempWidth) > (bottom-top-tempHeight)) {
		pMtr->radius = ((bottom-top-(tempHeight<<1))>>1) - ((tempHeight+bottom-top)>>3);
	}
	else	
		pMtr->radius = ((right-left)>>1) - (tempWidth+((right-left)>>3));

	// center the meter on the given dimensions
	pMtr->xCenter = (left+right)>>1;
	pMtr->yCenter = ((bottom+top)>>1) - (tempHeight>>1);

#elif (METER_TYPE == MTR_HALF_TYPE)
	// choose the radius
	if ((right-left)>>1 > (bottom-top)) {
		pMtr->radius = (bottom-top) - ((tempHeight<<1)+((bottom-top)>>3));
		pMtr->yCenter = ((bottom+top)>>1) + ((pMtr->radius+((bottom-top)>>3))>>1);
	}
	else {
		pMtr->radius = ((right-left)>>1) - (tempWidth + ((right-left)>>3));
		pMtr->yCenter = ((bottom+top)>>1) + ((pMtr->radius+((right-left)>>3))>>1);
	}

	// center the meter on the given dimensions
	pMtr->xCenter = (left+right)>>1;

#elif (METER_TYPE == MTR_QUARTER_TYPE)
	// choose the radius
	if ((right-left-tempWidth) > (bottom-top-(GetTextHeight(pScheme->pFont)<<1))) {
		pMtr->radius = (bottom-top-(GetTextHeight(pScheme->pFont)+tempHeight+GOL_EMBOSS_SIZE));
	}
	else {
		pMtr->radius = right-left-(GetTextWidth(tempChar,pScheme->pFont)*(SCALECHARCOUNT+1))-GOL_EMBOSS_SIZE;
	}
	pMtr->radius -= (((pMtr->radius)>>2) + GOL_EMBOSS_SIZE); 
	
	// center the meter on the given dimensions
	pMtr->xCenter = ((left+right)>>1) - ((pMtr->radius+tempWidth+(pMtr->radius>>2)) >> 1);
	pMtr->yCenter = ((top+bottom)>>1) + ((pMtr->radius+(pMtr->radius>>2)) >> 1);
    
#endif

	// Set the color scheme to be used
	if (pScheme == NULL)
		pMtr->hdr.pGolScheme = _pDefaultGolScheme; 
	else 	
		pMtr->hdr.pGolScheme = (GOL_SCHEME *)pScheme; 	

    GOLAddObject((OBJ_HEADER*) pMtr);
    
	return pMtr;
}

/*********************************************************************
* Function: MtrSetVal(METER *pMtr, SHORT newVal)
*
* Notes: Sets the value of the meter to newVal. If newVal is less
*		 than 0, 0 is assigned. If newVal is greater than range,
*		 range is assigned.
*
********************************************************************/
void MtrSetVal(METER *pMtr, SHORT newVal)
{	
	if (newVal < 0) {
		pMtr->value = 0;
		return;
	}
	if (newVal > pMtr->range) {
		pMtr->value = pMtr->range;
		return;
	}
	pMtr->value = newVal;
		
}

/*********************************************************************
* Function: MtrMsgDefault(WORD translatedMsg, METER *pMtr, GOL_MSG* pMsg)
*
* Notes: This the default operation to change the state of the meter.
*		 Called inside GOLMsg() when GOLMsgCallback() returns a 1.
*
********************************************************************/
void  MtrMsgDefault(WORD translatedMsg, METER *pMtr, GOL_MSG* pMsg)
{
	if (translatedMsg == MTR_MSG_SET) {
		MtrSetVal(pMtr, pMsg->param2);			// set the value	
		SetState(pMtr, MTR_DRAW_UPDATE);		// update the meter
    }	

}	

/*********************************************************************
* Function: WORD MtrTranslateMsg(METER *pMtr, GOL_MSG *pMsg)
*
* Notes: Evaluates the message if the object will be affected by the 
*		 message or not.
*
********************************************************************/
WORD MtrTranslateMsg(METER *pMtr, GOL_MSG *pMsg)
{

	// Evaluate if the message is for the meter
    // Check if disabled first
	if (GetState(pMtr,MTR_DISABLED))
		return OBJ_MSG_INVALID;
		
	if(pMsg->type == TYPE_SYSTEM) {	
		if(pMsg->param1 == pMtr->hdr.ID) {
            if(pMsg->uiEvent == EVENT_SET) {
				return MTR_MSG_SET;
            }
        }
    }
	return OBJ_MSG_INVALID;	
}

/*********************************************************************
* Function: WORD MtrDraw(METER *pMtr)
*
* Notes: This is the state machine to draw the meter.
*
********************************************************************/
WORD MtrDraw(METER *pMtr)
{
typedef enum {
	IDLE,
	FRAME_DRAW,
    NEEDLE_DRAW,
    NEEDLE_ERASE,
	TEXT_DRAW,
	TEXT_DRAW_RUN,
	ARC0_DRAW,
	ARC1_DRAW,
	ARC2_DRAW,
	SCALE_COMPUTE,
	SCALE_LABEL_DRAW,
    SCALE_DRAW,
    VALUE_ERASE,
    VALUE_DRAW,
    VALUE_DRAW_RUN,
} MTR_DRAW_STATES;

	static MTR_DRAW_STATES state = IDLE;
	static SHORT x1, y1, x2, y2;
	static SHORT critical, danger;
	static SHORT temp, j, i, angle;
	static XCHAR strVal[SCALECHARCOUNT+1];
	static XCHAR tempXchar[4] = {'8','8','8',0};
	static float radian;
	static DWORD_VAL dTemp, dRes;
		
    if(IsDeviceBusy())
        return 0;

    switch(state) {
    	case IDLE:

			if (GetState(pMtr,MTR_HIDE)) {  				      	// Hide the meter (remove from screen)
		    	SetColor(pMtr->hdr.pGolScheme->CommonBkColor);
		       	Bar(pMtr->hdr.left, pMtr->hdr.top,pMtr->hdr.right, pMtr->hdr.bottom);
			    return 1;
			}
	
			// Check if we need to draw the whole object
			SetLineThickness(NORMAL_LINE);
			SetLineType(SOLID_LINE);
			if (GetState(pMtr,MTR_DRAW)) {  				      
				// set parameters to draw the frame
				GOLPanelDraw(pMtr->hdr.left, pMtr->hdr.top, pMtr->hdr.right, pMtr->hdr.bottom, 0, 
							 pMtr->hdr.pGolScheme->Color0, 
							 pMtr->hdr.pGolScheme->EmbossLtColor, 
							 pMtr->hdr.pGolScheme->EmbossDkColor, 
							 NULL, GOL_EMBOSS_SIZE-1);
				state = FRAME_DRAW;
			} else {
				state = NEEDLE_ERASE;
				goto mtr_needle_draw_st;
				
			}
			
		case FRAME_DRAW:
	        if(!GOLPanelDrawTsk()){
    			return 0;
        	}
        	state = TEXT_DRAW;
        	
        case TEXT_DRAW:	

			// set scale and label color code boundaries
			critical = CRITICAL_DEGREE; danger = DANGER_DEGREE;
	
			// draw the meter title
			SetColor(pMtr->hdr.pGolScheme->TextColor1);
			SetFont(pMtr->hdr.pGolScheme->pFont);
			temp = GetTextWidth(pMtr->pText, pMtr->hdr.pGolScheme->pFont);
			
			// set the start location of the meter title
			#if (METER_TYPE == MTR_WHOLE_TYPE) 
		
				MoveTo( pMtr->xCenter-(temp>>1), pMtr->yCenter+pMtr->radius+GetTextHeight(pMtr->hdr.pGolScheme->pFont));
				 		
			#elif (METER_TYPE == MTR_HALF_TYPE)			 		
	
				MoveTo(	pMtr->xCenter-(temp>>1), pMtr->yCenter+3);
				
			#elif (METER_TYPE == MTR_QUARTER_TYPE)			 		
	
				MoveTo(((pMtr->hdr.right+pMtr->hdr.left)>>1)-(temp>>1), pMtr->yCenter+(GetTextHeight(pMtr->hdr.pGolScheme->pFont)>>2));

			#endif

            state = TEXT_DRAW_RUN;

		case TEXT_DRAW_RUN:
			// render the title of the meter
        	if(!OutText(pMtr->pText))
            	return 0;
            state = ARC0_DRAW;

		case ARC0_DRAW:
			// check if we need to draw the arcs 
			if (!GetState(pMtr, MTR_RING)) {
				// if meter is not RING type, for scale label colors use
				// the three colors (normal, critical and danger)
				SetColor(pMtr->normalColor);
				i = DEGREE_END;
				state = SCALE_COMPUTE;
				goto mtr_scale_compute;
			} else {
				SetColor(pMtr->normalColor);
			
				// set the arc radii: x1 smaller radius and x2 as the larger radius
				x1 = pMtr->radius+2;
				x2 = pMtr->radius+(pMtr->radius>>2)+2;

			
				// draw the arcs
			#if (METER_TYPE == MTR_WHOLE_TYPE) 
				if (!Arc( pMtr->xCenter, pMtr->yCenter, pMtr->xCenter, pMtr->yCenter, x1, x2, 0xE1))	
					return 0;
			#elif (METER_TYPE == MTR_HALF_TYPE) 
				if (!Arc( pMtr->xCenter, pMtr->yCenter, pMtr->xCenter, pMtr->yCenter, x1, x2, 0xC0))
					return 0;
			#elif (METER_TYPE == MTR_QUARTER_TYPE) 
				if (!Arc( pMtr->xCenter, pMtr->yCenter, pMtr->xCenter, pMtr->yCenter, x1, x2, 0x01))
					return 0;
			#endif
				SetColor(pMtr->criticalColor);
				state = ARC1_DRAW;
			} 
				
		case ARC1_DRAW:
			// draw the arcs
			#if (METER_TYPE == MTR_WHOLE_TYPE) 
				if (!Arc( pMtr->xCenter, pMtr->yCenter, pMtr->xCenter, pMtr->yCenter, x1, x2, 0x02))	
					return 0;
			#elif (METER_TYPE == MTR_HALF_TYPE) 
				if (!Arc( pMtr->xCenter, pMtr->yCenter, pMtr->xCenter, pMtr->yCenter, x1, x2, 0x01))
					return 0;
			#elif (METER_TYPE == MTR_QUARTER_TYPE) 
				if (!Arc( pMtr->xCenter, pMtr->yCenter, pMtr->xCenter, pMtr->yCenter, x1, x2, 0x02))
					return 0;
			#endif
			SetColor(pMtr->dangerColor);
			state = ARC2_DRAW;

		case ARC2_DRAW:

				// draw the arcs
			#if (METER_TYPE == MTR_WHOLE_TYPE) 
				if (!Arc( pMtr->xCenter, pMtr->yCenter, pMtr->xCenter, pMtr->yCenter, x1, x2, 0x04))
					return 0;
			#elif (METER_TYPE == MTR_HALF_TYPE) 
				if (!Arc( pMtr->xCenter, pMtr->yCenter, pMtr->xCenter, pMtr->yCenter, x1, x2, 0x02))
					return 0;
			#endif
			// set the color for the scale labels
			SetColor(pMtr->hdr.pGolScheme->Color1);
			i = DEGREE_END;
			state = SCALE_COMPUTE;

		case SCALE_COMPUTE:
mtr_scale_compute:		

			if (i >= DEGREE_START) {
				
				radian = i*.0175;
	
				if (!GetState(pMtr, MTR_RING)) {
					if (i < danger) {
						SetColor(pMtr->dangerColor);
					}
					else if (i <= critical) {
						SetColor(pMtr->criticalColor);
					}
				}
				
				// compute for the effective radius of the scales
				if ((i%45) == 0) 			
					x2 = pMtr->radius+(pMtr->radius>>2)+2;
				else	
					x2 = pMtr->radius+(pMtr->radius>>3)+3;
				
				// compute the starting x1 and y1 position of the scales
				// x2 here is the distance from the center to the x1, y1
				// position. Sin and cos is used here since computation speed in initial
				// drawing is not yet critical.
				x1 = x2*cos(radian);
				y1 = (-1)*(x2*sin(radian));
				
				// using ratio and proportion we get the x2,y2 position
				dTemp.Val = 0;
				dTemp.w[1] = (pMtr->radius+3);
				dTemp.Val /= x2;
				
				dRes.Val = dTemp.Val*x1;
				x2 = dRes.w[1] + pMtr->xCenter;				// adjusted to center
				
				dRes.Val = dTemp.Val*y1;
				y2 = dRes.w[1] + pMtr->yCenter;				// adjusted to center
				
				x1 += pMtr->xCenter; y1 += pMtr->yCenter;	// adjust x1, y1 to the center 
        		state = SCALE_DRAW;
			} else {
        		state = NEEDLE_ERASE;
        		goto mtr_needle_draw_st;
        	}
			
		case SCALE_DRAW:
			Line(x1,y1,x2,y2);							// now draw the scales
			if(IsDeviceBusy())
       			return 0;

			if ((i%45) == 0) {

				// draw the scale labels
				// reusing radian, x2 and y2
	
				// compute for the actual angle of needle to be shown in screen				
				radian = (DEGREE_END - DEGREE_START) - (i-(DEGREE_START));
				// compute the values of the label to be shown per 45 degree
				temp = pMtr->range*(radian/(DEGREE_END - DEGREE_START));
				
				// this implements sprintf(strVal, "%d", temp); faster
				// note that this is just for values >= 0, while sprintf covers negative values.

				j = 1;
				do {
					strVal[SCALECHARCOUNT-j] = (temp%10) + '0';
					if ((temp/= 10) == 0)
						break;
					j++;
				} while(j <= SCALECHARCOUNT);
				
				// the (&strVal[SCALECHARCOUNT-j]) removes the leading zeros.
				// if leading zeroes will be printed change (&strVal[SCALECHARCOUNT-j])
				// to simply strVal and remove the break statement above in the do-while loop
				x2 = GetTextWidth((&strVal[SCALECHARCOUNT-j]), pMtr->hdr.pGolScheme->pFont);
				y2 = GetTextHeight(pMtr->hdr.pGolScheme->pFont);
	
				if (i == -45) {
					MoveTo(x1+3, y1);
				} else if (i == 0) {
					MoveTo(x1+3, y1-(y2>>1));
				} else if (i == 45) {
					MoveTo(x1+3, y1-(y2>>1)-3);
				} else if (i == 90) {
					MoveTo(x1-(x2>>1), y1-(y2)-3);
				} else if (i == 135) {
					MoveTo(x1-(x2), y1-(y2));
				} else if (i == 180) {
					MoveTo(x1-(x2)-3, y1-(y2>>1));
				} else if (i == 225) {
					MoveTo(x1-(x2+3), y1);
				}
				state = SCALE_LABEL_DRAW;
			} else {
				i -= DEGREECOUNT;
				state = SCALE_LABEL_DRAW;
				goto mtr_scale_compute;
			}
			
		case SCALE_LABEL_DRAW:
			if(!OutText(&strVal[SCALECHARCOUNT-j]))
       			return 0;
			i -= DEGREECOUNT;
			state = SCALE_COMPUTE;
			goto mtr_scale_compute;

		case NEEDLE_ERASE:
mtr_needle_draw_st:


			if (GetState(pMtr,MTR_DRAW_UPDATE)) {  				      
				// to update the needle, redraw the old position with background color
				SetColor(pMtr->hdr.pGolScheme->Color0);
				SetLineThickness(THICK_LINE);
				Line(pMtr->xCenter, pMtr->yCenter, pMtr->xPos, pMtr->yPos);
			}

			state = NEEDLE_DRAW;
	
		case NEEDLE_DRAW:

			if(IsDeviceBusy())
       			return 0;

			// At this point, pMtr->value is assumed to contain the new value of the meter.
			// calculate the new angle
			dTemp.Val   = 0;
			dTemp.w[1]  = pMtr->value;
			dTemp.Val  /= pMtr->range;
			dTemp.Val  *= (DEGREE_END - DEGREE_START);
			
			angle = DEGREE_END-(dTemp.w[1]);
			temp  = angle;
	
			/* The method uses a lookup table of pre-calculated sine values. The table
			   is derived from calculating sine values from 0 degrees to 90 degrees.
			   To save space, the entries are just a byte size. So 360/255 = 1.40625 degrees
			   increments is used for each entry. (i.e entry 0 is zero, entry i is 1.40625,
			   entry 2 is 2*1.40625,... entry n is n*1.40625. 
			   Get the sine of the entries yields a float value. Since we only use a 
			   byte for storage we can shift the values by 127 without overflowing the 
			   storage. Thus we need to shift the computed values by 7 bits. Shifting now 
			   permits us to work with integers which greatly reduces the execution time.
			   With this in mind, the input angles must be translated to 0-90 range with the
			   quadrant of the original angle stored to translate back the value from the 
			   table to the correct quadrant. Also the quadrant number will determine if 
			   the calculated x and y positions are to be swapped. 
			   In summary:
			   	Swap x and y when calculating points in quadrant 2 and 4
			   	Negate x when in quadrant 2 and 3
			   	Negate y when in quadrant 1 and 2
			*/

			// translate the angle to 0-90 range
			while (temp < 0)								// this is needed for negative
				temp += 90;									// for negative values
			while(temp > 90)		
				temp -= 90;

			// determine which quadrant the angle is located
			// i determines if x and y are swapped (0 no swapping, 1 swapping needed)
			// y2 and x2 are the multiplier to negate or not the x and y values
			if ((180 < angle ) && (angle <= 270)) {			// quadrant 3
				i  =  0;
				y2 =  1;
				x2 = -1;
			} else if ((90 < angle) && (angle <= 180)) {	// quadrant 2
				i  =  1;
				y2 = -1;
				x2 = -1;
			} else if ((0 <= angle) && (angle <= 90)) {		// quadrant 1
				i  =  0;								
				y2 = -1;
				x2 =  1;
			} else if ((-90 < angle) && (angle < 0)) {		// quadrant 4
				i  =  1;
				y2 =  1;
				x2 =  1;
			} 
			
			// Find Sine value from look up table 
			temp *= .71111;									// value is derived from 
															// 360/256 = 1.40625. To avoid
															// division, the inverse is used
			x1 = _sineTable[64-temp] * pMtr->radius;
			y1 = _sineTable[temp] 	 * pMtr->radius;
			
			// calculate new positions, check if we need to reverse x and y values
			pMtr->xPos = ((x2)*(((i==0) ? x1:y1) >> 7)) + pMtr->xCenter;
			pMtr->yPos = ((y2)*(((i==0) ? y1:x1) >> 7)) + pMtr->yCenter;
		
			// now draw the needle with the new position	
			SetColor(pMtr->dangerColor);
			SetLineThickness(THICK_LINE);
			Line(pMtr->xCenter, pMtr->yCenter, pMtr->xPos, pMtr->yPos);	
			SetLineThickness(NORMAL_LINE);	
			#ifdef METER_DISPLAY_VALUES_ENABLE
				state = VALUE_ERASE;
			#else			 
				// reset the line to normal 
				SetLineThickness(NORMAL_LINE);
				state = IDLE;
				return 1;
			#endif	

		#ifdef METER_DISPLAY_VALUES_ENABLE
		case VALUE_ERASE:
			if(IsDeviceBusy())
       			return 0;

			// reset the line to normal 
			SetLineThickness(NORMAL_LINE);

			// display the value
			// erase previous value first
			
			temp = GetTextWidth(tempXchar, pMtr->hdr.pGolScheme->pFont);
			
			SetColor(pMtr->hdr.pGolScheme->Color0);
		
			#if (METER_TYPE == MTR_WHOLE_TYPE) 
		
				Bar(    pMtr->xCenter-(temp>>1),
		 				pMtr->yCenter+pMtr->radius,
				    	pMtr->xCenter+(temp>>1),
		 				pMtr->yCenter+pMtr->radius+GetTextHeight(pMtr->hdr.pGolScheme->pFont));
			 		
			#elif (METER_TYPE == MTR_HALF_TYPE) 
		
				Bar(    pMtr->xCenter-(temp>>1),
			 			pMtr->yCenter-GetTextHeight(pMtr->hdr.pGolScheme->pFont),
					    pMtr->xCenter+(temp>>1),
					    pMtr->yCenter);

			#elif (METER_TYPE == MTR_QUARTER_TYPE) 
				
				Bar(	pMtr->xCenter-1, 
						pMtr->yCenter-GetTextHeight(pMtr->hdr.pGolScheme->pFont),
						pMtr->xCenter+temp,
						pMtr->yCenter+1);
											    		
			#endif
			state = VALUE_DRAW;
		
		case VALUE_DRAW:
			if(IsDeviceBusy())
       			return 0;
		
			if (angle > critical) {
				SetColor(pMtr->normalColor);
			}
			else if (angle > danger) {
				SetColor(pMtr->criticalColor);
			}
			else {
				SetColor(pMtr->dangerColor);
			}
			
			// display the current value
			SetFont(pMtr->hdr.pGolScheme->pFont);
			
			// this implements sprintf(strVal, "%03d", pMtr->value); faster
			// note that this is just for values >= 0, while sprintf covers negative values.
			i = pMtr->value;
			
			j = 1;
			do {
				strVal[SCALECHARCOUNT-j] = (i%10) + '0';
				if ((i/= 10) == 0)
					break;
				j++;
			} while(j <= SCALECHARCOUNT);			
			
			
			temp = GetTextWidth(&strVal[SCALECHARCOUNT-j], pMtr->hdr.pGolScheme->pFont);
						
			#if (METER_TYPE == MTR_WHOLE_TYPE) 
			
				MoveTo( pMtr->xCenter-(temp>>1), pMtr->yCenter+pMtr->radius);
					 		
			#elif (METER_TYPE == MTR_HALF_TYPE) 
			
				MoveTo(	pMtr->xCenter-(temp>>1),
			 			pMtr->yCenter-GetTextHeight(pMtr->hdr.pGolScheme->pFont));

			#elif (METER_TYPE == MTR_QUARTER_TYPE) 
			
				MoveTo(pMtr->xCenter, pMtr->yCenter-GetTextHeight(pMtr->hdr.pGolScheme->pFont));
						
			#endif
			
			state = VALUE_DRAW_RUN;
			
		case VALUE_DRAW_RUN:
			if(!OutText(&strVal[SCALECHARCOUNT-j]))
            	return 0;	
            state = IDLE;	
			return 1;
		#endif //METER_DISPLAY_VALUES_ENABLE
	}
	return 1;
}

#endif // USE_METER




