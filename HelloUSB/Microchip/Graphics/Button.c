/*****************************************************************************
 *  Module for Microchip Graphics Library
 *  GOL Layer 
 *  Button
 *****************************************************************************
 * FileName:        Button.c
 * Dependencies:    Button.h
 * Processor:       PIC24, PIC32
 * Compiler:       	MPLAB C30 Version 3.00, C32
 * Linker:          MPLAB LINK30, LINK32
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

#ifdef USE_BUTTON

/*********************************************************************
* Function: BUTTON  *BtnCreate(WORD ID, SHORT left, SHORT top, SHORT right, 
*                              SHORT bottom, SHORT radius, void *pBitmap, XCHAR *pText, 
*                              GOL_SCHEME *pScheme)
*
*
* Notes: Creates a BUTTON object and adds it to the current active list.
*        If the creation is successful, the pointer to the created Object 
*        is returned. If not successful, NULL is returned.
*
********************************************************************/
BUTTON *BtnCreate(	WORD ID, SHORT left, SHORT top, SHORT right, SHORT bottom, SHORT radius,  
			        WORD state, void *pBitmap, XCHAR *pText, GOL_SCHEME *pScheme)
{
	BUTTON *pB = NULL;
	
	pB = malloc(sizeof(BUTTON));
	if (pB == NULL) 
		return NULL;
	
	pB->hdr.ID      	= ID;					// unique id assigned for referencing
	pB->hdr.pNxtObj 	= NULL;					// initialize pointer to NULL
	pB->hdr.type    	= OBJ_BUTTON;			// set object type
	pB->hdr.left       = left;       	    	// left position
	pB->hdr.top        = top;       	    	// top position
	pB->hdr.right      = right;       	    	// right position
	pB->hdr.bottom     = bottom;      	    	// bottom position
	pB->radius     = radius;				// radius
	pB->pBitmap  	= pBitmap;  			// location of bitmap 
	pB->pText   	= pText;				// location of the text
    pB->hdr.state   	= state; 	            // state

	// Set the color scheme to be used
	if (pScheme == NULL)
		pB->hdr.pGolScheme = _pDefaultGolScheme; 
	else 	
		pB->hdr.pGolScheme = (GOL_SCHEME *)pScheme; 	

	pB->textWidth  	= 0;  
	pB->textHeight 	= 0;
	if (pB->pText != NULL) {
		// Calculate the text width & height  
		pB->textWidth = GetTextWidth(pText, pB->hdr.pGolScheme->pFont);
		pB->textHeight = GetTextHeight(pB->hdr.pGolScheme->pFont);
	}	
    GOLAddObject((OBJ_HEADER*) pB);
    
#ifdef  USE_FOCUS
    if(GetState(pB,BTN_FOCUSED))
        GOLSetFocus((OBJ_HEADER*)pB);
#endif

	return pB;
}

/*********************************************************************
* Function: BtnSetText(BUTTON *pB, XCHAR *pText)
*
*
* Notes: Sets the text used in the button.
*
********************************************************************/
void BtnSetText(BUTTON *pB, XCHAR *pText)
{
	pB->pText      = pText;
	pB->textWidth  = GetTextWidth(pText, pB->hdr.pGolScheme->pFont);     
	pB->textHeight = GetTextHeight(pB->hdr.pGolScheme->pFont);
}

/*********************************************************************
* Function: BtnMsgDefault(WORD translatedMsg, BUTTON *pB, GOL_MSG* pMsg)
*
*
* Notes: This the default operation to change the state of the button.
*		 Called inside GOLMsg() when GOLMsgCallback() returns a 1.
*
********************************************************************/
void  BtnMsgDefault(WORD translatedMsg, BUTTON *pB, GOL_MSG* pMsg){

#ifdef  USE_FOCUS
#ifdef  USE_TOUCHSCREEN
    if(pMsg->type == TYPE_TOUCHSCREEN){
        if(!GetState(pB,BTN_FOCUSED)){
                GOLSetFocus((OBJ_HEADER*)pB);
        }
    }
#endif
#endif

    switch(translatedMsg){

        case BTN_MSG_PRESSED:
            SetState(pB, BTN_PRESSED|BTN_DRAW); 	// set pressed and redraw
            break;

        case BTN_MSG_RELEASED:
            ClrState(pB, BTN_PRESSED);  			// reset pressed
            SetState(pB, BTN_DRAW);   			// redraw
            break;

    }

}

/*********************************************************************
* Function: WORD BtnTranslateMsg(BUTTON *pB, GOL_MSG *pMsg)
*
*
* Notes: Evaluates the message if the object will be affected by the 
*		 message or not.
*
********************************************************************/
WORD BtnTranslateMsg(BUTTON *pB, GOL_MSG *pMsg)
{
	// Evaluate if the message is for the button
    // Check if disabled first
	if (GetState(pB,BTN_DISABLED))
		return OBJ_MSG_INVALID;

#ifdef USE_TOUCHSCREEN

    if(pMsg->type == TYPE_TOUCHSCREEN){
    	// Check if it falls in the button's face
	    if( (pB->hdr.left   < pMsg->param1) &&
   	        (pB->hdr.right  > pMsg->param1) &&
            (pB->hdr.top    < pMsg->param2) &&
            (pB->hdr.bottom > pMsg->param2) ){

			if(GetState(pB, BTN_TOGGLE)) {
				if(pMsg->uiEvent == EVENT_RELEASE) {
					if(GetState(pB, BTN_PRESSED))
                		return BTN_MSG_RELEASED;
					else
                		return BTN_MSG_PRESSED;
                }
			}
			else {
	            if(pMsg->uiEvent == EVENT_RELEASE)
	                return BTN_MSG_RELEASED;
	            if(!GetState(pB,BTN_PRESSED))
	   	            return BTN_MSG_PRESSED;
	   	    }     
	   	            
        }else{
			if(!GetState(pB, BTN_TOGGLE)) {
				if(GetState(pB,BTN_PRESSED))
               		return BTN_MSG_RELEASED;
            }    	
        }
        
	    return OBJ_MSG_INVALID;	
    }

#endif


#ifdef USE_KEYBOARD

    if(pMsg->type == TYPE_KEYBOARD){
	    if(pMsg->param1 == pB->hdr.ID){
            if(pMsg->uiEvent == EVENT_KEYSCAN){
	            if(GetState(pB, BTN_TOGGLE)) {
	                if((pMsg->param2 == SCAN_SPACE_RELEASED)||
	                   (pMsg->param2 == SCAN_CR_RELEASED) ){
						if(GetState(pB, BTN_PRESSED))
    	            		return BTN_MSG_RELEASED;
						else
            	    		return BTN_MSG_PRESSED;
	                }  
		        } else { 
	                if((pMsg->param2 == SCAN_SPACE_PRESSED)||
	                   (pMsg->param2 == SCAN_CR_PRESSED) ){
	                    return BTN_MSG_PRESSED;
	                }
	                if((pMsg->param2 == SCAN_SPACE_RELEASED)||
	                   (pMsg->param2 == SCAN_CR_RELEASED) ){
	                    return BTN_MSG_RELEASED;
	                }  
          		}      
            }
        }
	    return OBJ_MSG_INVALID;	
    }

#endif

	return OBJ_MSG_INVALID;	
}

/*********************************************************************
* Function: WORD BtnDraw(BUTTON *pB)
*
*
* Notes: This is the state machine to draw the button.
*
********************************************************************/
WORD BtnDraw(BUTTON *pB)
{
typedef enum {
	REMOVE,
	BEVEL_DRAW,
    RNDBUTTON_DRAW,
	TEXT_DRAW,
    TEXT_DRAW_RUN,
	FOCUS_DRAW,
} BTN_DRAW_STATES;

static BTN_DRAW_STATES state = REMOVE;
static SHORT width, height, radius;

WORD faceClr, embossLtClr, embossDkClr, xText, yText;

    if(IsDeviceBusy())
        return 0;

    switch(state){

        case REMOVE:

            if(IsDeviceBusy())
                return 0;  

	        if (GetState(pB,BTN_HIDE)) {  				      // Hide the button (remove from screen)
       	        SetColor(pB->hdr.pGolScheme->CommonBkColor);
       	        Bar(pB->hdr.left, pB->hdr.top, pB->hdr.right, pB->hdr.bottom);
		        return 1;
		    }
			/* Note: that width and height adjustment considers the following assumptions:
					1. if circular width = height = radius*2
					2. if vertical capsule width = radius*2
					3. if horizontal capsule height = radius*2
					4. radius must be less than or equal to width if height is greater than width
					5. radius must be less than or equal to height if width is greater than height
					6. if button is cornered, radius must be zero
			*/				
   			radius = pB->radius;								// get radius
	    	width  = (pB->hdr.right - pB->hdr.left) - (radius*2);		// get width
	    	height = (pB->hdr.bottom - pB->hdr.top) - (radius*2);		// get height
            state = BEVEL_DRAW;

        case BEVEL_DRAW:
			if(!GetState(pB,BTN_DISABLED)){
			    if(GetState(pB,BTN_PRESSED)){
			        embossDkClr = pB->hdr.pGolScheme->EmbossLtColor;
			      	embossLtClr = pB->hdr.pGolScheme->EmbossDkColor;
			      	faceClr 	= pB->hdr.pGolScheme->Color1;
			    } else {
			        embossLtClr = pB->hdr.pGolScheme->EmbossLtColor;
			      	embossDkClr = pB->hdr.pGolScheme->EmbossDkColor;
			      	faceClr		= pB->hdr.pGolScheme->Color0;
			    }  
		    }else{    
		        embossLtClr = pB->hdr.pGolScheme->EmbossLtColor;
		      	embossDkClr = pB->hdr.pGolScheme->EmbossDkColor;
		      	faceClr		= pB->hdr.pGolScheme->ColorDisabled;
            } 
   			SetLineThickness(NORMAL_LINE);
			SetLineType(SOLID_LINE);			
            GOLPanelDraw(pB->hdr.left+radius, pB->hdr.top+radius, pB->hdr.right-radius,  	
			   			 pB->hdr.bottom-radius, radius, faceClr, embossLtClr, embossDkClr,  
				  		 pB->pBitmap, GOL_EMBOSS_SIZE);
   			state = RNDBUTTON_DRAW;

        case RNDBUTTON_DRAW:

            if(!GOLPanelDrawTsk()){
                return 0;
            }
   			state = TEXT_DRAW;

        case TEXT_DRAW:
        	if (pB->pText != NULL){
                if (!GetState(pB,BTN_DISABLED)){
        	        if (GetState(pB,BTN_PRESSED)){
	        	        SetColor(pB->hdr.pGolScheme->TextColor1);
	                }else{
		                SetColor(pB->hdr.pGolScheme->TextColor0);
                    }
                }
	            else{
		            SetColor(pB->hdr.pGolScheme->TextColorDisabled);
                }
                SetFont(pB->hdr.pGolScheme->pFont);
                // check text alignment
                if (GetState(pB, BTN_TEXTRIGHT)) 
	                xText = pB->hdr.right-(pB->textWidth+GOL_EMBOSS_SIZE+2);
	            else if (GetState(pB, BTN_TEXTLEFT))    
	                xText = pB->hdr.left+GOL_EMBOSS_SIZE+2;
	            else    
	                xText = (pB->hdr.left+pB->hdr.right-pB->textWidth)>>1;
								
                if (GetState(pB, BTN_TEXTTOP)) 
	                yText = pB->hdr.top+GOL_EMBOSS_SIZE+2;
	            else if (GetState(pB, BTN_TEXTBOTTOM))    
	                yText = pB->hdr.bottom-(pB->textHeight+GOL_EMBOSS_SIZE);
	            else    
	                yText = (pB->hdr.bottom+pB->hdr.top-pB->textHeight)>>1;

           		MoveTo(xText, yText);
           		state = TEXT_DRAW_RUN;
	        }else {
           		state = FOCUS_DRAW;
           		goto rnd_button_draw_focus;
           	}	

        case TEXT_DRAW_RUN:
			if(!OutText(pB->pText))
            	return 0;	
            state = FOCUS_DRAW;
 
        case FOCUS_DRAW:
rnd_button_draw_focus:
            if(IsDeviceBusy())
                return 0;
    
	        if(GetState(pB,BTN_FOCUSED)){
		        SetLineType(FOCUS_LINE);
			    if(GetState(pB,BTN_PRESSED)) {
			        SetColor(pB->hdr.pGolScheme->TextColor1);
			    } else {     
			        SetColor(pB->hdr.pGolScheme->TextColor0);
			    }
			    // check if the object has rounded corners or not    
		        if (!pB->radius) {
					Rectangle(	pB->hdr.left+GOL_EMBOSS_SIZE+2 , pB->hdr.top+GOL_EMBOSS_SIZE+2,
                          		pB->hdr.right-GOL_EMBOSS_SIZE-2, pB->hdr.bottom-GOL_EMBOSS_SIZE-2);
                }    	      
		        else  {
				    // original center is still the same, but radius is reduced
		        	Bevel( pB->hdr.left+radius, pB->hdr.top+radius, 					\
				           pB->hdr.right-radius, pB->hdr.bottom-radius,				\
				           radius - 2 - GOL_EMBOSS_SIZE);
			    }
		        SetLineType(SOLID_LINE);
	        }
	        
            state = REMOVE;
            return 1;
    }
    return 1;
}

#endif // USE_BUTTON
