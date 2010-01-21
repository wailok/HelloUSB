/******************************************************************************
 *
 *               Microchip Memory Disk Drive File System
 *
 ******************************************************************************
 * FileName:        FS Phys Interface Template.c
 * Dependencies:    TEMPLATEFILE.h
 *					string.h
 *                  FSIO.h
 *                  FSDefs.h
 * Processor:       None
 * Compiler:        None
 * Company:         Microchip Technology, Inc.
 * Version:         1.0.0
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
*****************************************************************************/

#include "Compiler.h"
#include "MDD File System\FSIO.h"
#include "MDD File System\FSDefs.h"
#include "string.h"
#include "MDD File System\internal flash.h"
#include "HardwareProfile.h"
#include "FSConfig.h"

/*************************************************************************/
/*  Note:  This file is included as a template of a C file for           */
/*         a new physical layer. It is designed to go with               */
/*         "TEMPLATEFILE.h"                                              */
/*************************************************************************/

/******************************************************************************
 * Global Variables
 *****************************************************************************/

#ifdef USE_PIC18
	#pragma udata
	#pragma code
#endif

/******************************************************************************
 * Prototypes
 *****************************************************************************/
void EraseBlock(ROM BYTE* dest);
void WriteRow(void);
void WriteByte(unsigned char);
BYTE DISKmount( DISK *dsk);
BYTE LoadMBR(DISK *dsk);
BYTE LoadBootSector(DISK *dsk);
extern void Delayms(BYTE milliseconds);
BYTE MediaInitialize(void);

/******************************************************************************
 * Function:        BYTE MediaDetect(void)
 *
 * PreCondition:    InitIO() function has been executed.
 *
 * Input:           void
 *
 * Output:          TRUE   - Card detected
 *                  FALSE   - No card detected
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 *****************************************************************************/
BYTE MDD_IntFlash_MediaDetect()
{
	return TRUE;
}//end MediaDetect

/******************************************************************************
 * Function:        WORD ReadSectorSize(void)
 *
 * PreCondition:    MediaInitialize() is complete
 *
 * Input:           void
 *
 * Output:          WORD - size of the sectors for this physical media.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 *****************************************************************************/
WORD MDD_IntFlash_ReadSectorSize(void)
{
    return MEDIA_SECTOR_SIZE;
}

/******************************************************************************
 * Function:        DWORD ReadCapacity(void)
 *
 * PreCondition:    MediaInitialize() is complete
 *
 * Input:           void
 *
 * Output:          DWORD - size of the "disk"
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 *****************************************************************************/
DWORD MDD_IntFlash_ReadCapacity(void)
{
    return MDD_INTERNAL_FLASH_TOTAL_DISK_SIZE;
}

/******************************************************************************
 * Function:        BYTE InitIO(void)
 *
 * PreCondition:    None
 *
 * Input:           void
 *
 * Output:          TRUE   - Card initialized
 *                  FALSE   - Card not initialized
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 *****************************************************************************/
#if 0
BYTE MDD_IntFlash_InitIO (void)
{
	return  (DISKmount (&gDiskData) == CE_GOOD);
}
#endif


/******************************************************************************
 * Function:        BYTE MediaInitialize(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Returns TRUE if media is initialized, FALSE otherwise
 *
 * Overview:        MediaInitialize initializes the media card and supporting variables.
 *
 * Note:            None
 *****************************************************************************/
BYTE MDD_IntFlash_MediaInitialize(void)
{
	return TRUE;
}//end MediaInitialize


/******************************************************************************
 * Function:        BYTE SectorRead(DWORD sector_addr, BYTE *buffer)
 *
 * PreCondition:    None
 *
 * Input:           sector_addr - Sector address, each sector contains 512-byte
 *                  buffer      - Buffer where data will be stored, see
 *                                'ram_acs.h' for 'block' definition.
 *                                'Block' is dependent on whether internal or
 *                                external memory is used
 *
 * Output:          Returns TRUE if read successful, false otherwise
 *
 * Side Effects:    None
 *
 * Overview:        SectorRead reads 512 bytes of data from the card starting
 *                  at the sector address specified by sector_addr and stores
 *                  them in the location pointed to by 'buffer'.
 *
 * Note:            The device expects the address field in the command packet
 *                  to be byte address. Therefore the sector_addr must first
 *                  be converted to byte address. This is accomplished by
 *                  shifting the address left 9 times.
 *****************************************************************************/
BYTE MDD_IntFlash_SectorRead(DWORD sector_addr, BYTE* buffer)
{
    #if defined(__C30__)
        WORD PSVPageSave;

        PSVPageSave = PSVPAG;
        //TODO: fix this so it is not static but rather based on the requested sector
        PSVPAG = 0x01;
    #endif

    memcpypgm2ram
    (
        (void*)buffer,
        (ROM void*)(MASTER_BOOT_RECORD_ADDRESS + (sector_addr * MEDIA_SECTOR_SIZE)),
        MEDIA_SECTOR_SIZE
    );

    #if defined(__C30__)
        PSVPAG = PSVPageSave;
    #endif

	return TRUE;
}//end SectorRead

/******************************************************************************
 * Function:        BYTE SectorWrite(DWORD sector_addr, BYTE *buffer, BYTE allowWriteToZero)
 *
 * PreCondition:    None
 *
 * Input:           sector_addr - Sector address, each sector contains 512-byte
 *                  buffer      - Buffer where data will be read
 *                  allowWriteToZero - If true, writes to the MBR will be valid
 *
 * Output:          Returns TRUE if write successful, FALSE otherwise
 *
 * Side Effects:    None
 *
 * Overview:        SectorWrite sends 512 bytes of data from the location
 *                  pointed to by 'buffer' to the card starting
 *                  at the sector address specified by sector_addr.
 *
 * Note:            The sample device expects the address field in the command packet
 *                  to be byte address. Therefore the sector_addr must first
 *                  be converted to byte address. This is accomplished by
 *                  shifting the address left 9 times.
 *****************************************************************************/
#if defined(__18CXX)
#pragma udata myFileBuffer
#endif
volatile unsigned char file_buffer[ERASE_BLOCK_SIZE] __attribute__((far));
#if defined(__18CXX)
#pragma udata
#endif

#define INTERNAL_FLASH_PROGRAM_WORD        0x4003
#define INTERNAL_FLASH_ERASE               0x4042
#define INTERNAL_FLASH_PROGRAM_PAGE        0x4001


#if defined(__C32__)
    #define PTR_SIZE DWORD
#else
    #define PTR_SIZE WORD
#endif
ROM BYTE *FileAddress = 0;


BYTE MDD_IntFlash_SectorWrite(DWORD sector_addr, BYTE* buffer, BYTE allowWriteToZero)
{
    #if !defined(INTERNAL_FLASH_WRITE_PROTECT)
        ROM BYTE* dest;
        BOOL foundDifference;
        WORD blockCounter;
        WORD sectorCounter;
    
        #if defined(__18CXX)
            BYTE *p;
        #endif

        #if defined(__C30__)
        WORD PSVPageSave;
    
        PSVPageSave = PSVPAG;
        //TODO: make this not a hardcoded value
        PSVPAG = 0x01;
        #endif
    
        dest = (ROM BYTE*)(MASTER_BOOT_RECORD_ADDRESS + (sector_addr * MEDIA_SECTOR_SIZE));
    
        sectorCounter = 0;
    
        while(sectorCounter < MEDIA_SECTOR_SIZE)
        {
            foundDifference = FALSE;
            for(blockCounter = 0; blockCounter < ERASE_BLOCK_SIZE; blockCounter++)
            {
                if(dest[sectorCounter] != buffer[sectorCounter])
                {
                    foundDifference = TRUE;
                    sectorCounter -= blockCounter;
                    break;
                }
                sectorCounter++;
            }
            if(foundDifference == TRUE)
            {
                BYTE i,j;
                PTR_SIZE address;
    
                #if (ERASE_BLOCK_SIZE >= MEDIA_SECTOR_SIZE)
                    address = ((PTR_SIZE)(dest + sectorCounter) & ~(ERASE_BLOCK_SIZE - 1));
    
                    memcpypgm2ram
                    (
                        (void*)file_buffer,
                        (ROM void*)address,
                        ERASE_BLOCK_SIZE
                    );
    
                    EraseBlock((ROM BYTE*)address);
    
                    address = ((PTR_SIZE)(dest + sectorCounter) & (ERASE_BLOCK_SIZE - 1));
    
                    memcpy
                    (
                        (void*)(&file_buffer[address]),
                        (void*)buffer,
                        MEDIA_SECTOR_SIZE
                    );
    
                #else
    
                    address = ((WORD)(&dest[sectorCounter]) & ~(ERASE_BLOCK_SIZE - 1));
    
                    EraseBlock((ROM BYTE*)address);
    
                    sectorCounter = sectorCounter & ~(ERASE_BLOCK_SIZE - 1);
    
                    memcpy
                    (
                        (void*)file_buffer,
                        (void*)buffer+sectorCounter,
                        ERASE_BLOCK_SIZE
                    );
                #endif
    
                //sectorCounter = sectorCounter & ~(ERASE_BLOCK_SIZE - 1);
    
                i=ERASE_BLOCK_SIZE/WRITE_BLOCK_SIZE;
                j=0;

                #if defined(__18CXX)
                    p = file_buffer;
                #endif

                while(i-->0)
                {
                    //Write the new data
                    for(blockCounter = 0; blockCounter < WRITE_BLOCK_SIZE; blockCounter++)
                    {
                        //Write the data
                        #if defined(__18CXX)
                            TABLAT = *p++;
                            _asm tblwtpostinc _endasm
                            sectorCounter++;
                        #endif
    
                        #if defined(__C30__)
                                __builtin_tblwtl((int)FileAddress, *((WORD*)&file_buffer[sectorCounter]));
                                __builtin_tblwth((int)FileAddress, 0);
                                FileAddress += 2;
                                sectorCounter += 2;
                                blockCounter++;
                        #endif

                        #if defined(__C32__)
                                NVMWriteWord((DWORD*)KVA_TO_PA(FileAddress), *((DWORD*)&file_buffer[sectorCounter]));
                                FileAddress += 4;
                                sectorCounter += 4;
                        #endif
                    }
    
                    j++;
    
                    //write the row
                    #if defined(__18CXX)
                        // Start the write process: reposition tblptr back into memory block that we want to write to.
                         _asm tblrdpostdec _endasm
    
                        // Write flash memory, enable write control.
                        EECON1 = 0x84;
    
                        EECON2 = 0x55;
                        EECON2 = 0xaa;
                        EECON1bits.WR = 1;
                        Nop();
                        EECON1bits.WREN = 0;
    
                        TBLPTR++;
                    #endif
                    #if defined(__C30__)
                        NVMCON = INTERNAL_FLASH_PROGRAM_PAGE;
                    	asm("DISI #16");					//Disable interrupts for next few instructions for unlock sequence
                    	__builtin_write_NVM();
                    #endif
    
                    Nop();
                    Nop();
                }
            }
        }
    
        #if defined(__C30__)
        PSVPAG = PSVPageSave;
        #endif
    
    	return TRUE;
    #else
        return TRUE;
    #endif
} //end SectorWrite

#if !defined(INTERNAL_FLASH_WRITE_PROTECT)
void EraseBlock(ROM BYTE* dest)
{
    #if defined(__18CXX)
        TBLPTR = (unsigned short long)dest;

        //Erase the current block
        EECON1 = 0x94;

        EECON2 = 0x55;
        EECON2 = 0xaa;
        EECON1bits.WR = 1;

        EECON1bits.WREN = 0;
    #endif

    #if defined(__C30__)
    	NVMCON = INTERNAL_FLASH_ERASE;				//Erase page on next WR
       	__builtin_tblwtl((int)dest, 0xFFFF);
    	asm("DISI #16");					//Disable interrupts for next few instructions for unlock sequence
    	__builtin_write_NVM();
        FileAddress = dest;
        NVMCON = INTERNAL_FLASH_PROGRAM_PAGE;
    #endif

    #if defined(__C32__)
        FileAddress = dest;
        NVMErasePage((BYTE *)KVA_TO_PA(dest));
    #endif
}
#endif

/******************************************************************************
 * Function:        BYTE WriteProtectState(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          BYTE    - Returns the status of the "write enabled" pin
 *
 * Side Effects:    None
 *
 * Overview:        Determines if the card is write-protected
 *
 * Note:            None
 *****************************************************************************/

BYTE MDD_IntFlash_WriteProtectState(void)
{
    #if defined(INTERNAL_FLASH_WRITE_PROTECT)
        return TRUE;
    #else
	    return FALSE;
    #endif
}

