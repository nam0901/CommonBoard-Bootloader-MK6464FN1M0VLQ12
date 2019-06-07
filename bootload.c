/*
 * bootload.c
 *
 *  Created on: Jun 15, 2017
 *      Author: tgack
 */

#include <stdint.h>
#include "bootload.h"
#include "AS1.h"
#include "FLASH1.h"
#include "PE_Error.h"
#include "string.h"
#include "bootstrap.h"
#include "UART_PDD.h"
#include "GPIO1.h"
#include "Events.h"

static void updateAddressMSW(LP_BLOCK block);
static STATUS processDataRecord(LP_BLOCK block);
static STATUS processEOFRecord(LP_BLOCK block);

volatile uint8_t rxValue;
volatile bool RxFlag = FALSE;
volatile bool TxFlag = FALSE;
LDD_TError Error;
LDD_FLASH_TOperationStatus OpStatus;

/*!
 * @brief Application entry point.
 */
extern void launchTargetApplication(unsigned long l)
{
	__asm("ldr sp, [r0,#0]");	// load the stack pointer value from the program's reset vector
	__asm("ldr pc, [r0,#4]");	// load the program counter value from the program's reset vector to cause operation to continue from there
}
void initalizeBootloader(LP_BLOCK block)
{
	block->mswAddress = 0x00000000;
}
STATUS processBootloadRecord(LP_BLOCK block)
{
	STATUS rValue = STATUS_OK;
	//IMPORTANT: MK64F12 needs to write in sets of 8 bytes minimum.
	//Writing less than 8 bytes to an 8 byte set is OK as long as nothing else is written to the 8 byte set.
	//To rewrite, an erase must be executed first.
	if(isRecordValid(block) == STATUS_OK)
	{
		switch(block->record.recordType)
		{
			case DATA_REC:				rValue = processDataRecord(block);
				break;
			case EOF_REC:				rValue = processEOFRecord(block);
				break;
			case EXT_SEG_ADDR_REC:		updateAddressMSW(block);
				break;
			case START_SEG_ADDR_REC:
				break;
			case EXT_LINEAR_ADDR_REC:	updateAddressMSW(block);
				break;
			case START_LINEAR_ADDR_REC:
				break;
			default:
				break;
		}
	}
	else
	{
		rValue = STATUS_CHECKSUM_ERROR;
	}
	return rValue;
}
STATUS eraseApplicationSpace(void)
{
	STATUS rValue = STATUS_OK;
	Error = FLASH1_Erase(FLASH1_DeviceData, APP_START_ADDRESS, 0xF8000); //100K - start address (0x410) = size
	do
	{
		FLASH1_Main(FLASH1_DeviceData);
		OpStatus = FLASH1_GetOperationStatus(FLASH1_DeviceData);
	} while(!((OpStatus == LDD_FLASH_IDLE) | (OpStatus == LDD_FLASH_FAILED)));
	if(OpStatus == LDD_FLASH_FAILED)
	{
		rValue = STATUS_ERASE_ERROR;
	}
	return rValue;
}

static STATUS processEOFRecord(LP_BLOCK block)
{
	STATUS rValue = STATUS_END;
//	uint8_t code = LAUNCH_CODE;
//	Error = FLASH1_Write(FLASH1_DeviceData, &code, LAUNCH_CODE_SEGMENT, 1);
//	do
//	{
//		FLASH1_Main(FLASH1_DeviceData);
//		OpStatus = FLASH1_GetOperationStatus(FLASH1_DeviceData);
//	} while(!((OpStatus == LDD_FLASH_IDLE) | (OpStatus == LDD_FLASH_FAILED)));
//	if(OpStatus == LDD_FLASH_FAILED)
//	{
//		rValue = STATUS_WRITE_ERROR;
//	}
	return rValue;
}

static STATUS processDataRecord(LP_BLOCK block)
{
	STATUS rValue = STATUS_OK;
	uint32_t destinationAddress;
	uint32_t index;

	destinationAddress = (block->mswAddress | (uint32_t)block->record.address.value);
	if(destinationAddress >= APP_START_ADDRESS)
	{
		//write line to flash
		for(index = 0; index < (uint32_t)block->record.byteCount; index += 8)
		{
			Error = FLASH1_Write(FLASH1_DeviceData, &block->record.data[index], destinationAddress+index, 8);
			do
			{
				FLASH1_Main(FLASH1_DeviceData);
				OpStatus = FLASH1_GetOperationStatus(FLASH1_DeviceData);
			} while(!((OpStatus == LDD_FLASH_IDLE) | (OpStatus == LDD_FLASH_FAILED)));
			if(OpStatus == LDD_FLASH_FAILED)
			{
				rValue = STATUS_WRITE_ERROR;
			}
		}
		//read back line and store in dataCheck buffer
		for(index = 0; index < (uint32_t)block->record.byteCount; index += 8)
		{
			Error = FLASH1_Read(FLASH1_DeviceData, destinationAddress+index, &block->record.dataCheck[index], 8);
			do
			{
				FLASH1_Main(FLASH1_DeviceData);
				OpStatus = FLASH1_GetOperationStatus(FLASH1_DeviceData);
			} while(!((OpStatus == LDD_FLASH_IDLE) | (OpStatus == LDD_FLASH_FAILED)));
			if(OpStatus == LDD_FLASH_FAILED)
			{
				rValue = STATUS_WRITE_ERROR;
			}
		}
		//compare buffers to confirm successful write
		for(index = 0; index < (uint32_t) block->record.byteCount; index++)
		{
			if(block->record.dataCheck[index] != block->record.data[index])
			{
				rValue = STATUS_WRITE_ERROR;
			}
		}
	}
	else
	{
		rValue = STATUS_WRITE_ERROR;
	}
	return rValue;
}
static void updateAddressMSW(LP_BLOCK block)
{
	uint8_t *ptr;
	uint16_t value;
	uint32_t address;

	ptr = block->record.data;
	switch(block->record.recordType)
	{
		case EXT_SEG_ADDR_REC:
			value = ((uint16_t)*ptr) << 8;
			ptr++;
			value |= (uint16_t)*ptr;
			address = value * 16;
			break;
		case EXT_LINEAR_ADDR_REC:
			address = ((uint16_t)*ptr) << 16;
			ptr++;
			address |= (uint16_t)*ptr;
			address = ((uint16_t)*ptr) << 16;
			break;
		default:
			break;
	}
	block->mswAddress = address;
}
STATUS confirmAppPresence(void)
{
	STATUS rValue = STATUS_OK;
	uint32_t buffer;
	Error = FLASH1_Read(FLASH1_DeviceData, APP_START_ADDRESS, &buffer, 8);
	do
	{
		FLASH1_Main(FLASH1_DeviceData);
		OpStatus = FLASH1_GetOperationStatus(FLASH1_DeviceData);
	} while(!((OpStatus == LDD_FLASH_IDLE) | (OpStatus == LDD_FLASH_FAILED)));
	if(OpStatus == LDD_FLASH_FAILED)
	{
		rValue = STATUS_WRITE_ERROR; //not an error, but using the integer value
	}
	if(buffer == 0xFFFFFFFF)
	{
		rValue = STATUS_WRITE_ERROR;
	}
	return rValue;
}
void sendResponse(char* response)
{
//	for(int i=0;i<3000;i++)
//	{
//		__asm("nop");
//	}
	AS1_CancelBlockReception(AS1_DeviceData);
	GPIO1_SetFieldValue(NULL, PTE, 0b1); //enable Tx
	AS1_TurnRxOff(AS1_DeviceData); //Turn off Rx
	AS1_TurnTxOn(AS1_DeviceData); //Turn on Tx
	AS1_SendBlock(AS1_DeviceData, response, strlen(response));
	while(!AS1_GetTxCompleteStatus(AS1_DeviceData));
	TxFlag = FALSE;

}

void receiveData(void)
{
	AS1_CancelBlockTransmission(AS1_DeviceData);
	GPIO1_SetFieldValue(NULL, PTE, 0b0); //disable Tx, enable Rx
	AS1_TurnRxOn(AS1_DeviceData); //Turn on Rx
	AS1_TurnTxOff(AS1_DeviceData); //Turn off Tx
	AS1_ReceiveBlock(AS1_DeviceData, (LDD_TData*)&rxValue, 0x1U);
	while(!RxFlag);
	RxFlag = FALSE;

}

