/*****************************************************************************
 *  Module for Microchip Graphics Library 
 *  GOL Layer 
 *****************************************************************************
 * FileName:        GOL.c
 * Dependencies:    Graphics.h 
 * Processor:       PIC24, PIC32
 * Compiler:       	MPLAB C30 V3.00, MPLAB C32
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
 * Anton Alkhimenok and
 * Paolo A. Tamayo
 *                      11/12/07    Version 1.0 release
 *****************************************************************************/
#include "Graphics\Graphics.h"

#ifdef USE_GOL

// Pointer to the current linked list of objects displayed and receiving messages
OBJ_HEADER  *_pGolObjects        = NULL;

// Pointer to the default GOL scheme (created in GOLInit())
GOL_SCHEME  *_pDefaultGolScheme  = NULL;

// Pointer to the object receiving keyboard input
OBJ_HEADER  *_pObjectFocused     = NULL;

#ifdef USE_FOCUS

/*********************************************************************
* Function: OBJ_HEADER *GOLGetFocusNext(void)
*
* PreCondition: none
*
* Input: none
*
* Output: pointer to the object receiving messages from keyboard.
*         NULL if there are no objects supporting keyboard focus.
*
* Side Effects: none
*
* Overview: moves keyboard focus to the next object
*           in the current link list
*
* Note: none
*
********************************************************************/
OBJ_HEADER *GOLGetFocusNext(void){
OBJ_HEADER *pNextObj;

    if(_pObjectFocused == NULL){
        pNextObj = _pGolObjects;
    }else{
        pNextObj = _pObjectFocused;
    }

    do{
        pNextObj = pNextObj->pNxtObj;

        if(pNextObj == NULL)
            pNextObj = _pGolObjects;

        if(GOLCanBeFocused(pNextObj))
            break;

    }while(pNextObj != _pObjectFocused);
         
    
    return pNextObj;
}

/*********************************************************************
* Function: void GOLSetFocus(OBJ_HEADER* object)
*
* PreCondition: none
*
* Input: pointer to the object to be focused
*
* Output: 
*
* Side Effects: none
*
* Overview: moves keyboard focus to the object
*
* Note: none
*
********************************************************************/
void GOLSetFocus(OBJ_HEADER* object){

	if(!GOLCanBeFocused(object)) 
		return;
		
    if(_pObjectFocused != NULL){
        ClrState(_pObjectFocused, FOCUSED);
        SetState(_pObjectFocused, DRAW_FOCUS);
    }

     SetState(object, DRAW_FOCUS|FOCUSED);

    _pObjectFocused = object;
}

/*********************************************************************
* Function: WORD GOLCanBeFocused(OBJ_HEADER* object)
*
* PreCondition: none
*
* Input: pointer to the object 
*
* Output: non-zero if the object supports keyboard focus, zero if not
*
* Side Effects: none
*
* Overview: checks if object support keyboard focus
*
* Note: none
*
********************************************************************/
WORD GOLCanBeFocused(OBJ_HEADER* object){

    if((object->type == OBJ_BUTTON) || (object->type == OBJ_CHECKBOX) ||
        (object->type == OBJ_RADIOBUTTON) || (object->type == OBJ_LISTBOX) ||
        (object->type == OBJ_SLIDER)) 
    {

        if(!GetState(object,DISABLED))            
            return 1;
    }
    return 0;
}

#endif

/*********************************************************************
* Function: GOL_SCHEME *GOLCreateScheme(void)
*
* PreCondition: none
*
* Input: none
*
* Output: pointer to scheme object
*
* Side Effects: none
*
* Overview: creates a color scheme object and assign default colors
*
* Note: none
*
********************************************************************/
GOL_SCHEME *GOLCreateScheme(void)
{
	GOL_SCHEME *pTemp;
	pTemp = (GOL_SCHEME*) malloc(sizeof(GOL_SCHEME));
	if (pTemp != NULL)
	{
		pTemp->EmbossDkColor 		= EMBOSSDKCOLORDEFAULT;
		pTemp->EmbossLtColor   		= EMBOSSLTCOLORDEFAULT;
		pTemp->TextColor0			= TEXTCOLOR0DEFAULT;
		pTemp->TextColor1	  		= TEXTCOLOR1DEFAULT;
		pTemp->TextColorDisabled	= TEXTCOLORDISABLEDDEFAULT;
		pTemp->Color0				= COLOR0DEFAULT; 
		pTemp->Color1				= COLOR1DEFAULT; 
        pTemp->ColorDisabled		= COLORDISABLEDDEFAULT; 
		pTemp->CommonBkColor 		= COMMONBACKGROUNDCOLORDEFAULT;
		pTemp->pFont				= (void*)&FONTDEFAULT;		
	}
	return pTemp;
}	

/*********************************************************************
* Function: void GOLInit()
*
* PreCondition: none
*
* Input: none
*
* Output: none
*
* Side Effects: none
*
* Overview: initializes GOL
*
* Note: none
*
********************************************************************/
void  GOLInit(){
	// Initialize display
    InitGraph();
	// Initialize the default GOL scheme
	_pDefaultGolScheme  = GOLCreateScheme();
}

/*********************************************************************
* Function: void GOLFree()
*
* PreCondition: none
*
* Input: none
*
* Output: none
*
* Side Effects: none
*
* Overview: frees memory of all objects in the current linked list
*           and starts a new linked list
*
* Note: drawing and messaging must be suspended
*
********************************************************************/
void GOLFree(){
OBJ_HEADER * pNextObj;
OBJ_HEADER * pCurrentObj;


    pCurrentObj = _pGolObjects;
    while(pCurrentObj != NULL){
        pNextObj = pCurrentObj->pNxtObj;

#ifdef USE_LISTBOX
        if(pCurrentObj->type == OBJ_LISTBOX)
            LbDelItemsList((LISTBOX*)pCurrentObj);
#endif

#ifdef USE_GRID
        if(pCurrentObj->type == OBJ_GRID)
            GridFreeItems((GRID*)pCurrentObj);
#endif
        free(pCurrentObj);
        pCurrentObj = pNextObj;
    }

    GOLNewList();
}
/*********************************************************************
* Function: BOOL GOLDeleteObject(OBJ_HEADER * object)
*
* PreCondition: none
*
* Input: pointer to the object
*
* Output: none
*
* Side Effects: none
*
* Overview: deletes an object to the linked list objects for the current screen.
*
* Note: none
*
********************************************************************/
BOOL  GOLDeleteObject(OBJ_HEADER * object)
{
    if(!_pGolObjects)
        return FALSE;

    if(object == _pGolObjects)
    {
        _pGolObjects = object->pNxtObj;        
    }else
    {
        OBJ_HEADER  *curr;
        OBJ_HEADER  *prev;

        curr = _pGolObjects->pNxtObj;
        prev = _pGolObjects;

        while(curr)
        {
            if(curr == object)
                break;

            prev = curr;
            curr = curr->pNxtObj;
        }

        if(!curr)
            return FALSE;

        prev->pNxtObj = curr->pNxtObj;
    }

#ifdef USE_LISTBOX
    if(object->type == OBJ_LISTBOX)
        LbDelItemsList((LISTBOX*)object);
#endif

#ifdef USE_GRID
    if(object->type == OBJ_GRID)
        GridFreeItems((GRID*)object);
#endif

    free(object);

    return TRUE;
}
/*********************************************************************
* Function: BOOL GOLDeleteObject(OBJ_HEADER * object)
*
* PreCondition: none
*
* Input: pointer to the object
*
* Output: none
*
* Side Effects: none
*
* Overview: deletes an object to the linked list objects for the current screen.
*
* Note: none
*
********************************************************************/
BOOL  GOLDeleteObjectByID(WORD ID)
{
    OBJ_HEADER * object;

    object = GOLFindObject(ID);

    if(!object)
        return FALSE;

    return GOLDeleteObject(object);

}
/*********************************************************************
* Function: OBJ_HEADER* GOLFindObject(WORD ID)
*
* PreCondition: none
*
* Input: object ID
*
* Output: pointer to the object
*
* Side Effects: none
*
* Overview: searches for the object by its ID in the current objects linked list,
*           returns NULL if the object is not found
*
* Note: none
*
********************************************************************/
OBJ_HEADER* GOLFindObject(WORD ID){
OBJ_HEADER * pNextObj;

    pNextObj = _pGolObjects;
    while(pNextObj != NULL){
        if(pNextObj->ID == ID){
            return pNextObj;
        }
        pNextObj = pNextObj->pNxtObj;            
    }
    return NULL;
}

/*********************************************************************
* Function: void GOLAddObject(OBJ_HEADER * object)
*
* PreCondition: none
*
* Input: pointer to the object
*
* Output: none
*
* Side Effects: none
*
* Overview: adds an object to the linked list objects for the current screen.
*           Object will be drawn and will receive messages.
*
* Note: none
*
********************************************************************/
void  GOLAddObject(OBJ_HEADER * object){
OBJ_HEADER * pNextObj;

    if(_pGolObjects ==  NULL){
        _pGolObjects = object;
    }else{
        pNextObj = _pGolObjects;
        while(pNextObj->pNxtObj != NULL){
            pNextObj = pNextObj->pNxtObj;            
        }
        pNextObj->pNxtObj = object;
    }
    object->pNxtObj = NULL;
}
/*********************************************************************
* Function: WORD GOLDraw()
*
* PreCondition: none
*
* Input: none
*
* Output: non-zero if drawing is complete
*
* Side Effects: none
*
* Overview: redraws objects in the current linked list
*
* Note: none
*
********************************************************************/
WORD GOLDraw(){
static OBJ_HEADER *pCurrentObj = NULL;
SHORT done;

    if(pCurrentObj == NULL){
        if(GOLDrawCallback()){
            // It's last object jump to head
            pCurrentObj = _pGolObjects;
        }else{
            return 0;  // drawing is not done
        }
    }

    done = 0;
    while(pCurrentObj != NULL){
        
        if(IsObjUpdated(pCurrentObj)){

            switch(pCurrentObj->type){
                #ifdef USE_BUTTON
	            case OBJ_BUTTON:
                    done = BtnDraw((BUTTON*)pCurrentObj);
                    break;
                #endif
                #ifdef USE_WINDOW
	            case OBJ_WINDOW:
                    done = WndDraw((WINDOW*)pCurrentObj);
                    break;
                #endif
                #ifdef USE_CHECKBOX
	            case OBJ_CHECKBOX:
                    done = CbDraw((CHECKBOX*)pCurrentObj);
                    break;
                #endif
                #ifdef USE_RADIOBUTTON
	            case OBJ_RADIOBUTTON:
                    done = RbDraw((RADIOBUTTON*)pCurrentObj);
                    break;
                #endif
                #ifdef USE_EDITBOX
	            case OBJ_EDITBOX:
                    done = EbDraw((EDITBOX*)pCurrentObj);
                    break;
                #endif
                #ifdef USE_LISTBOX
	            case OBJ_LISTBOX:
                    done = LbDraw((LISTBOX*)pCurrentObj);
                    break;
                #endif
                #ifdef USE_SLIDER
	            case OBJ_SLIDER:
                    done = SldDraw((SLIDER*)pCurrentObj);
                    break;
                #endif
                #ifdef USE_PROGRESSBAR
	            case OBJ_PROGRESSBAR:
                    done = PbDraw((PROGRESSBAR*)pCurrentObj);
                    break;
                #endif
                #ifdef USE_STATICTEXT
	            case OBJ_STATICTEXT:
                    done = StDraw((STATICTEXT*)pCurrentObj);
                    break;
                #endif
                #ifdef USE_PICTURE
	            case OBJ_PICTURE:
                    done = PictDraw((PICTURE*)pCurrentObj);
                    break;
                #endif
                #ifdef USE_GROUPBOX
	            case OBJ_GROUPBOX:
                    done = GbDraw((GROUPBOX*)pCurrentObj);
                    break;
                #endif
                #ifdef USE_ROUNDDIAL
	            case OBJ_ROUNDDIAL:
                    done = RdiaDraw((ROUNDDIAL*)pCurrentObj);
                    break;
                #endif
                #ifdef USE_METER
	            case OBJ_METER:
                    done = MtrDraw((METER*)pCurrentObj);
                    break;
                #endif
                #ifdef USE_CUSTOM
	            case OBJ_CUSTOM:
                    done = CcDraw((CUSTOM*)pCurrentObj);
                    break;
                #endif
                #ifdef USE_GRID
                case OBJ_GRID:
                    done = GridDraw( (GRID *)pCurrentObj );
                    break;
                #endif
                #ifdef USE_CHART
                case OBJ_CHART:
                    done = ChDraw( (CHART *)pCurrentObj );
                    break;
                #endif
                default:
                    break;
            }
            if(done){
                GOLDrawComplete(pCurrentObj);
            }else{
                return 0; // drawing is not done
            }
        }
        pCurrentObj = pCurrentObj->pNxtObj;
    }
    return 1;   // drawing is completed
}

/*********************************************************************
* Function: void GOLRedrawRec(SHORT left, SHORT top, SHORT right, SHORT bottom)
*
* PreCondition: none
*
* Input: left,top,right,bottom - rectangle borders
*
* Output: none
*
* Side Effects: none
*
* Overview: marks objects with parts in the rectangle to be redrawn
*
* Note: none
*
********************************************************************/
void  GOLRedrawRec(SHORT left, SHORT top, SHORT right, SHORT bottom){
OBJ_HEADER *pCurrentObj;

    pCurrentObj = _pGolObjects;

    while(pCurrentObj != NULL){
        
        if( !( (pCurrentObj->left > right) ||
             (pCurrentObj->right < left) ||       
             (pCurrentObj->top > bottom) ||
             (pCurrentObj->bottom < top) ) )

		if( ( (pCurrentObj->left >= left)&&
              (pCurrentObj->left <= right) ) ||

            ( (pCurrentObj->right >= left)&&
              (pCurrentObj->right <= right) ) ||

            ( (pCurrentObj->top >= top)&&
              (pCurrentObj->top <= bottom) )||

            ( (pCurrentObj->bottom >= top)&&
              (pCurrentObj->bottom <= bottom) ) ){

                GOLRedraw(pCurrentObj);

        }

        pCurrentObj = pCurrentObj->pNxtObj;            

    }//end of while
}

/*********************************************************************
* Function: void GOLMsg(GOL_MSG *pMsg)
*
* PreCondition: none
*
* Input: pointer to the message
*
* Output: none
*
* Side Effects: none
*
* Overview: processes message for all objects in the liked list
*
* Note: none
*
********************************************************************/
void  GOLMsg(GOL_MSG *pMsg){
OBJ_HEADER *pCurrentObj;
WORD   translatedMsg;

    if(pMsg->uiEvent == EVENT_INVALID)
        return;

    pCurrentObj = _pGolObjects;

    while(pCurrentObj != NULL){
        switch(pCurrentObj->type){
            #ifdef USE_BUTTON
	        case OBJ_BUTTON:
                translatedMsg = BtnTranslateMsg((BUTTON*)pCurrentObj, pMsg);
                if(translatedMsg == OBJ_MSG_INVALID)
                    break;
                if(GOLMsgCallback(translatedMsg,pCurrentObj,pMsg))
                    BtnMsgDefault(translatedMsg,(BUTTON*)pCurrentObj, pMsg);
                break;
            #endif
            #ifdef USE_WINDOW
	        case OBJ_WINDOW:
                translatedMsg = WndTranslateMsg((WINDOW*)pCurrentObj, pMsg);
                if(translatedMsg == OBJ_MSG_INVALID)
                    break;
                GOLMsgCallback(translatedMsg,pCurrentObj,pMsg);
                break;
            #endif
            #ifdef USE_CHECKBOX
	        case OBJ_CHECKBOX:
                translatedMsg = CbTranslateMsg((CHECKBOX*)pCurrentObj, pMsg);
                if(translatedMsg == OBJ_MSG_INVALID)
                    break;
                if(GOLMsgCallback(translatedMsg,pCurrentObj,pMsg))
                    CbMsgDefault(translatedMsg,(CHECKBOX*)pCurrentObj, pMsg);
                break;
            #endif
            #ifdef USE_RADIOBUTTON
	        case OBJ_RADIOBUTTON:
                translatedMsg = RbTranslateMsg((RADIOBUTTON*)pCurrentObj, pMsg);
                if(translatedMsg == OBJ_MSG_INVALID)
                    break;
                if(GOLMsgCallback(translatedMsg,pCurrentObj,pMsg))
                    RbMsgDefault(translatedMsg,(RADIOBUTTON*)pCurrentObj, pMsg);
                break;
            #endif
            #ifdef USE_EDITBOX
	        case OBJ_EDITBOX:
                translatedMsg = EbTranslateMsg((EDITBOX*)pCurrentObj, pMsg);
                if(translatedMsg == OBJ_MSG_INVALID)
                    break;
                if(GOLMsgCallback(translatedMsg,pCurrentObj,pMsg))
                    EbMsgDefault(translatedMsg,(EDITBOX*)pCurrentObj, pMsg);
                break;
            #endif
            #ifdef USE_LISTBOX
	        case OBJ_LISTBOX:
                translatedMsg = LbTranslateMsg((LISTBOX*)pCurrentObj, pMsg);
                if(translatedMsg == OBJ_MSG_INVALID)
                    break;
                if(GOLMsgCallback(translatedMsg,pCurrentObj,pMsg))
                    LbMsgDefault(translatedMsg,(LISTBOX*)pCurrentObj, pMsg);
                break;
            #endif
            #ifdef USE_SLIDER
	        case OBJ_SLIDER:
                translatedMsg = SldTranslateMsg((SLIDER*)pCurrentObj, pMsg);
                if(translatedMsg == OBJ_MSG_INVALID)
                    break;
                if(GOLMsgCallback(translatedMsg,pCurrentObj,pMsg))
                    SldMsgDefault(translatedMsg,(SLIDER*)pCurrentObj, pMsg);
                break;
            #endif
            #ifdef USE_GROUPBOX
	        case OBJ_GROUPBOX:
                translatedMsg = GbTranslateMsg((GROUPBOX*)pCurrentObj, pMsg);
                if(translatedMsg == OBJ_MSG_INVALID)
                    break;
                GOLMsgCallback(translatedMsg,pCurrentObj,pMsg);
                break;
            #endif
            #ifdef USE_PROGRESSBAR
	        case OBJ_PROGRESSBAR:
                break;
            #endif
            #ifdef USE_STATICTEXT
	        case OBJ_STATICTEXT:
                translatedMsg = StTranslateMsg((STATICTEXT*)pCurrentObj, pMsg);
                if(translatedMsg == OBJ_MSG_INVALID)
                    break;
                GOLMsgCallback(translatedMsg,pCurrentObj,pMsg);
                break;
            #endif
            #ifdef USE_PICTURE
	        case OBJ_PICTURE:
                translatedMsg = PictTranslateMsg((PICTURE*)pCurrentObj, pMsg);
                if(translatedMsg == OBJ_MSG_INVALID)
                    break;
                GOLMsgCallback(translatedMsg,pCurrentObj,pMsg);
                break;
            #endif
           
            #ifdef USE_ROUNDDIAL
            case OBJ_ROUNDDIAL:
                translatedMsg = RdiaTranslateMsg((ROUNDDIAL*)pCurrentObj, pMsg);
                if(translatedMsg == OBJ_MSG_INVALID)
                    break;
                if(GOLMsgCallback(translatedMsg,pCurrentObj,pMsg))
                    RdiaMsgDefault(translatedMsg,(ROUNDDIAL*)pCurrentObj, pMsg);
                break;
            #endif            
            
            #ifdef USE_METER
            case OBJ_METER:
                translatedMsg = MtrTranslateMsg((METER*)pCurrentObj, pMsg);
                if(translatedMsg == OBJ_MSG_INVALID)
                    break;
                if(GOLMsgCallback(translatedMsg,pCurrentObj,pMsg))
                    MtrMsgDefault(translatedMsg,(METER*)pCurrentObj, pMsg);
                break;
            #endif            

            #ifdef USE_CUSTOM
	        case OBJ_CUSTOM:
                translatedMsg = CcTranslateMsg((CUSTOM*)pCurrentObj, pMsg);
                if(translatedMsg == OBJ_MSG_INVALID)
                    break;
                if(GOLMsgCallback(translatedMsg,pCurrentObj,pMsg))
                    CcMsgDefault((CUSTOM*)pCurrentObj,pMsg);
                break;
            #endif

            #ifdef USE_GRID
	        case OBJ_GRID:
                translatedMsg = GridTranslateMsg((GRID*)pCurrentObj, pMsg);
                if(translatedMsg == OBJ_MSG_INVALID)
                    break;
                if(GOLMsgCallback(translatedMsg,pCurrentObj,pMsg))
                    GridMsgDefault( translatedMsg, (GRID*)pCurrentObj, pMsg );
                break;
            #endif

            #ifdef USE_CHART
	        case OBJ_CHART:
                translatedMsg = ChTranslateMsg((CHART*)pCurrentObj, pMsg);
                if(translatedMsg == OBJ_MSG_INVALID)
                    break;
                GOLMsgCallback(translatedMsg,pCurrentObj,pMsg);
                break;
            #endif

            
            default:
                break;
        }
        pCurrentObj = pCurrentObj->pNxtObj;
    }
}

/*********************************************************************
* Variables for rounded panel drawing. Used by GOLRndPanelDraw and GOLRndPanelDrawTsk
********************************************************************/
SHORT _rpnlX1,				// Center x position of upper left corner
      _rpnlY1,				// Center y position of upper left corner
      _rpnlX2,				// Center x position of lower right corner
      _rpnlY2,				// Center y position of lower right corner
      _rpnlR;				// radius
WORD  _rpnlFaceColor,		// face color
      _rpnlEmbossLtColor,	// emboss light color
      _rpnlEmbossDkColor,	// emboss dark color
	  _rpnlEmbossSize;      // emboss size 
	  
void* _pRpnlBitmap = NULL;      

/*********************************************************************
* Function: WORD GOLPanelDrawTsk(void) 
*
* PreCondition: parameters must be set with
*               GOLRndPanelDraw(x,y,radius,width,height,faceClr,embossLtClr,
*								embossDkClr,pBitmap,embossSize)
*
* Input: None
*
* Output: Output: non-zero if drawing is completed
*
* Overview: draws a rounded panel on screen. Must be called repeatedly. Drawing is done
*           when it returns non-zero. 
*
* Note: none
*
********************************************************************/
WORD GOLPanelDrawTsk(void) {

//#define SIN45 46341

#ifndef USE_NONBLOCKING_CONFIG

	WORD counter;

	if (_rpnlR) {
		// draw upper left portion of the embossed area
		SetColor(_rpnlEmbossLtColor);
		Arc(_rpnlX1, _rpnlY1, _rpnlX2, _rpnlY2, _rpnlR-_rpnlEmbossSize, _rpnlR, 0xE1);	
		// draw lower right portion of the embossed area
		SetColor(_rpnlEmbossDkColor);
		Arc(_rpnlX1, _rpnlY1, _rpnlX2, _rpnlY2, _rpnlR-_rpnlEmbossSize, _rpnlR, 0x1E);	
	}
	else {
		// object is rectangular panel draw the embossed areas
		counter = 1;
	    SetColor(_rpnlEmbossLtColor);
    	while(counter < _rpnlEmbossSize){
   			Bar( _rpnlX1+counter, _rpnlY1+counter, _rpnlX2-counter, _rpnlY1+counter); 	// draw top
        	Bar( _rpnlX1+counter, _rpnlY1+counter, _rpnlX1+counter, _rpnlY2-counter);	// draw left
           	counter++;
       	}
       	counter = 1;
	    SetColor(_rpnlEmbossDkColor);
    	while(counter < _rpnlEmbossSize){
        	Bar( _rpnlX1+counter, _rpnlY2-counter, _rpnlX2-counter, _rpnlY2-counter);	// draw bottom
        	Bar( _rpnlX2-counter, _rpnlY1+counter, _rpnlX2-counter, _rpnlY2-counter);	// draw right	
           	counter++;
       	}
	}

	// draw bitmap
	if(_pRpnlBitmap != NULL) {
		
		PutImage(((_rpnlX2+_rpnlX1)-(GetImageWidth((void*)_pRpnlBitmap))>>1)+1, 
				 ((_rpnlY2+_rpnlY1)-(GetImageHeight((void*)_pRpnlBitmap))>>1)+1,
				 _pRpnlBitmap, IMAGE_NORMAL);

    } else {
	    // draw the face color
	    SetColor(_rpnlFaceColor);
		if (_rpnlR) 
		    FillBevel(_rpnlX1, _rpnlY1, _rpnlX2, _rpnlY2, _rpnlR-_rpnlEmbossSize);
		else    
		    Bar(_rpnlX1+_rpnlEmbossSize, _rpnlY1+_rpnlEmbossSize, _rpnlX2-_rpnlEmbossSize, _rpnlY2-_rpnlEmbossSize);	     
#ifdef USE_MONOCHROME
		if (_rpnlFaceColor == BLACK) {
			SetColor(WHITE);
			if (_rpnlR) 
	   			Bevel(_rpnlX1, _rpnlY1, _rpnlX2, _rpnlY2, _rpnlR-(_rpnlEmbossSize-1));	
	   		else	
    			Bevel(_rpnlX1+(_rpnlEmbossSize-1), _rpnlY1+(_rpnlEmbossSize-1), 
    				  _rpnlX2-(_rpnlEmbossSize-1), _rpnlY2-(_rpnlEmbossSize-1), 0);	
		}
#endif
	} 
    
	// check if we need to draw the frame
	if ((_pRpnlBitmap == NULL) || (((_rpnlX2-_rpnlX1+_rpnlR)>=GetImageWidth((void*)_pRpnlBitmap)) &&
								   ((_rpnlY2-_rpnlY1+_rpnlR)>=GetImageHeight((void*)_pRpnlBitmap)))) {
		// draw the outline 
    	SetColor(_rpnlEmbossDkColor);
    	Bevel(_rpnlX1, _rpnlY1, _rpnlX2, _rpnlY2, _rpnlR);
	}			 
    return 1;

#else

typedef enum {
BEGIN,
ARC1,
DRAW_EMBOSS1,
DRAW_EMBOSS2,
DRAW_EMBOSS3,
DRAW_EMBOSS4,
DRAW_FACECOLOR,
#ifdef USE_MONOCHROME
DRAW_INNERFRAME,
#endif
DRAW_FRAME,
} ROUND_PANEL_DRAW_STATES;

static ROUND_PANEL_DRAW_STATES state = BEGIN;
static WORD counter;

while(1){
    if(IsDeviceBusy())
        return 0;
    switch(state){
        case BEGIN:      
			if (_rpnlR) {
				// draw upper left portion of the embossed area
				SetColor(_rpnlEmbossLtColor);
				if (!Arc(_rpnlX1, _rpnlY1, _rpnlX2, _rpnlY2, _rpnlR-_rpnlEmbossSize, _rpnlR, 0xE1))	
					return 0;
				state = ARC1;
	        } 
	        else {
        		state = DRAW_EMBOSS1;
        		goto rnd_panel_draw_emboss;
        	}

        case ARC1:      
			// draw upper left portion of the embossed area
			SetColor(_rpnlEmbossDkColor);
			if (!Arc(_rpnlX1, _rpnlY1, _rpnlX2, _rpnlY2, _rpnlR-_rpnlEmbossSize, _rpnlR, 0x1E))
				return 0;	
        	state = DRAW_FACECOLOR;
        	goto rnd_panel_draw_facecolor;

		// now draw the upper portion of the embossed area
        case DRAW_EMBOSS1:
rnd_panel_draw_emboss:        
	    	SetColor(_rpnlEmbossLtColor);
            counter = 1;
            while(counter < _rpnlEmbossSize){
   	            if(IsDeviceBusy())
	       	            return 0;
	       	    // draw top        
				Bar( _rpnlX1+counter, _rpnlY1+counter,  \
				     _rpnlX2-counter, _rpnlY1+counter);
              	counter++;
           	}
            counter = 1;
           	state = DRAW_EMBOSS2;
           	break;

		case DRAW_EMBOSS2:
           	while(counter < _rpnlEmbossSize){
               	if(IsDeviceBusy())
                   	return 0;
                // draw left   	
	        	Bar( _rpnlX1+counter, _rpnlY1+counter,  \
	        	     _rpnlX1+counter, _rpnlY2-counter);	
               	counter++;
           	}
           	counter = 1;
           	state = DRAW_EMBOSS3;
           	break;

		// now draw the lower portion of the embossed area
		case DRAW_EMBOSS3:
		    SetColor(_rpnlEmbossDkColor);
	        while(counter < _rpnlEmbossSize){
                if(IsDeviceBusy())
       	            return 0;
       	        // draw bottom    
		       	Bar( _rpnlX1+counter, _rpnlY2-counter,   \
		       	     _rpnlX2-counter, _rpnlY2-counter);
             	counter++;
           	}
           	counter = 1;
           	state = DRAW_EMBOSS4;
           	break;

		case DRAW_EMBOSS4:
			while(counter < _rpnlEmbossSize){
               	if(IsDeviceBusy())
                   	return 0;
                // draw right	   	
	        	Bar( _rpnlX2-counter, _rpnlY1+counter,  \
	        		 _rpnlX2-counter, _rpnlY2-counter);	
    	       	counter++;
			}
            state = DRAW_FACECOLOR;
            break;

		// draw the face color 
		case DRAW_FACECOLOR:
rnd_panel_draw_facecolor:		

			if(_pRpnlBitmap != NULL) {
				
				PutImage(((_rpnlX2+_rpnlX1- GetImageWidth((void*)_pRpnlBitmap))>>1)+1, 
						 ((_rpnlY2+_rpnlY1-GetImageHeight((void*)_pRpnlBitmap))>>1)+1,
						 _pRpnlBitmap, IMAGE_NORMAL);

		    } else {
			    SetColor(_rpnlFaceColor);
				if (_rpnlR) {
				    if (!FillBevel(_rpnlX1, _rpnlY1, _rpnlX2, _rpnlY2, _rpnlR-_rpnlEmbossSize))
				    	return 0;
				}
				else  {  
				    Bar(_rpnlX1+_rpnlEmbossSize, _rpnlY1+_rpnlEmbossSize,    \
				    	_rpnlX2-_rpnlEmbossSize, _rpnlY2-_rpnlEmbossSize);
				}			    
	       	    #ifdef USE_MONOCHROME
	       	    	state = DRAW_INNERFRAME;
            		break;
	       	    #endif
				
			}
            state = DRAW_FRAME;
            break;

		#ifdef USE_MONOCHROME
		case DRAW_INNERFRAME:
   			if (_rpnlFaceColor == BLACK) {
   				SetColor(WHITE);
				if (_rpnlR) 
	    			Bevel(_rpnlX1, _rpnlY1, _rpnlX2, _rpnlY2, _rpnlR-(_rpnlEmbossSize-1));	
	    		else	
    				Bevel(_rpnlX1+(_rpnlEmbossSize-1), _rpnlY1+(_rpnlEmbossSize-1), 
    					  _rpnlX2-(_rpnlEmbossSize-1), _rpnlY2-(_rpnlEmbossSize-1), 0);	
	    			
    		}
            state = DRAW_FRAME;
            break;
		#endif	

		case DRAW_FRAME:
			// check if we need to draw the frame
			if ((_pRpnlBitmap == NULL) || (((_rpnlX2-_rpnlX1+_rpnlR)>=GetImageWidth((void*)_pRpnlBitmap)) &&
										   ((_rpnlY2-_rpnlY1+_rpnlR)>=GetImageHeight((void*)_pRpnlBitmap)))) {
				// draw the outline frame
				#ifdef USE_MONOCHROME
			    	SetColor(WHITE);
			    #else	
			    	SetColor(_rpnlEmbossDkColor);
			    #endif
			    Bevel(_rpnlX1, _rpnlY1, _rpnlX2, _rpnlY2, _rpnlR);
			}			 
            state = BEGIN;
            return 1;
            
    }// end of switch
}// end of while
#endif //#ifndef USE_NONBLOCKING_CONFIG   
}

#endif // USE_GOL
