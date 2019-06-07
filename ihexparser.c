/*
Line  * ihexparser.c
 *
 *  Created on: Jul 5, 2017
 *      Author: tgack
 *
 *
 * Description:
 *  Provides local and global functions for buffering and
 *  parsing Intel Hex data.
 *
 *  String manipulation functions are local to prevent loading
 *  of the larger C String library.
 */


#include <stdint.h>
#include "ihexparser.h"

#define ULONG_MAX			(0xFFFFFFFF)
#define LINE_LENGTH_BYTE 	(2)
#define LINE_LENGTH_WORD 	(4)
#define LINE_START_INDEX 	(0)
#define LINE_LENGTH_INDEX 	(1)
#define LINE_ADDRESS_INDEX 	(3)
#define LINE_TYPE_INDEX		(7)
#define LINE_DATA_INDEX		(9)

static uint16_t lineLength(void);
static STATUS getField(FIELD field, uint32_t *outValue, uint8_t dataIndex, uint8_t *lineBuffer, uint8_t lineLength);
static void copyChars(uint8_t *dest, uint8_t* src, uint8_t length);
static int htoi(const char *s, unsigned long *res);

uint8_t line[LINE_LENGTH];
//uint8_t test[LINE_LENGTH];
uint8_t lineIndex;
uint8_t	lineTerminator;
uint8_t lineNumber = 0;

//lineNumber in the test file
uint8_t getLineNumber(void)
{
	return lineNumber;
}

void initializeParser(void)
{
	clearLine();
}

uint8_t isLineReceived(void)
{
	AS1_TurnRxOn(AS1_DeviceData); //Turn on Rx
	AS1_TurnTxOff(AS1_DeviceData); //Turn off Tx

	uint8_t rValue = 0;
	//Change from 76
	if(lineLength()>=11 && lineLength()<= 76)//are there characters in the buffer?
	{
		rValue = lineTerminator;//has a line terminator been rcv'd?
	} else if(lineLength() >77){
		rValue = 2;
	}
	return rValue;
}
STATUS parseLine(LP_BLOCK block)
{
	STATUS rValue = STATUS_OK;
	uint16_t lineLen = lineLength();
	uint16_t recLen;
	uint32_t value;
	uint16_t dataIndex;

	if(lineLen >= 11)
	{
		if(':' == line[0])
		{
			/* Get record Length */
			if(STATUS_OK == getField(BYTE_COUNT, &value, 0, line, lineLen))
			{
				block->record.byteCount = (uint8_t)value;
			}
			else
			{
				return STATUS_FORMAT_ERROR;
			}

			/* Recover Address */
			if(STATUS_OK == getField(ADDRESS, &value, 0, line, lineLen))
			{
				block->record.address.value = (uint16_t)value;
			}
			else
			{
				return STATUS_FORMAT_ERROR;
			}

			/* Recover Record Type */
			if(STATUS_OK == getField(TYPE, &value, 0, line, lineLen))
			{
				block->record.recordType = (uint8_t)value;
			}
			else
			{
				return STATUS_FORMAT_ERROR;
			}

			/* Recover data from record */
			for(dataIndex=0; dataIndex < (uint16_t)block->record.byteCount; dataIndex++)
			{
				if(STATUS_OK == getField(DATA, &value, dataIndex, line, lineLen))
				{
					block->record.data[dataIndex] = (uint8_t)value;
				}
				else
				{
					return STATUS_FORMAT_ERROR;
				}
			}

			/* Recover the checksum from record */
			if(STATUS_OK == getField(CHECKSUM, &value, 0, line, lineLen))
			{
				block->record.checksum = (uint8_t)value;
			}
			else
			{
				return STATUS_FORMAT_ERROR;
			}
		}
		else
		{
			rValue = STATUS_FORMAT_ERROR;
		}
	}
	else
	{
		rValue = STATUS_LINE_ERROR;
	}
	return rValue;
}

STATUS isRecordValid(LP_BLOCK block)
{
	STATUS rValue = STATUS_OK;
	uint16_t checksum;
	uint8_t index;

	checksum = (uint16_t)block->record.byteCount + (uint16_t)block->record.address.bytes.lsb + (uint16_t)block->record.address.bytes.msb + (uint16_t)block->record.recordType;
	for(index=0; index < block->record.byteCount; index++)
	{
		checksum += (uint16_t)block->record.data[index];
	}
	checksum = ((checksum ^ 0xFFFF) + 1) & 0xFF;
	if((uint8_t)checksum != block->record.checksum)
	{
		rValue = STATUS_CHECKSUM_ERROR;
	}
	return rValue;
}

void enqueueChar(uint8_t value)
{
	if(lineIndex < (LINE_LENGTH-1))
	{
		switch(value)
		{
			case '\r': case '\n':
				if(lineIndex > 0)
				{
					line[lineIndex] = '\0'; //the end of the line
//					test[lineIndex] = '\0';
					lineNumber++;
					lineIndex = 0;
					lineTerminator = 1;
				}
				break;
			default:
				/* Save received character and increment the line buffer index */
				line[lineIndex] = value;
//				test[lineIndex] = (char)value;
				lineIndex++;
				break;
		}
	}
}

static uint16_t lineLength(void)
{
	uint16_t rValue = 0;
	uint8_t *linePtr;
	linePtr = line;
	while(*linePtr)
	{
		rValue = rValue+1;
		linePtr++;
	}
	return rValue;
}

void clearLine(void)
{
	line[0] = '\0';
	lineTerminator = 0;
}

static STATUS getField(FIELD field, uint32_t *outValue, uint8_t dataIndex, uint8_t *lineBuffer, uint8_t lineLength)
{
	uint8_t rValue;
	uint8_t valueBuffer[8];

	rValue = STATUS_OK;
	switch(field)
	{
	case START_CODE:
		if( (lineLength >= 1) && (':' == lineBuffer[LINE_START_INDEX]) )
		{
			*outValue = (uint32_t)lineBuffer[LINE_START_INDEX];
		}
		else
		{
			rValue = STATUS_FORMAT_ERROR;
		}
		break;

	case BYTE_COUNT:
		if (lineLength >= (LINE_LENGTH_INDEX + LINE_LENGTH_BYTE))
		{
			copyChars(valueBuffer, &lineBuffer[LINE_LENGTH_INDEX], LINE_LENGTH_BYTE);
			if(STATUS_OK != htoi( valueBuffer, (unsigned long *)outValue))
			{
				rValue = STATUS_FORMAT_ERROR;
			}
		}
		else
		{
			rValue = STATUS_FORMAT_ERROR;
		}
		break;

	case ADDRESS:
		if (lineLength >= (LINE_ADDRESS_INDEX + LINE_LENGTH_WORD))
		{
			copyChars(valueBuffer, &lineBuffer[LINE_ADDRESS_INDEX], LINE_LENGTH_WORD);
			if(STATUS_OK != htoi( valueBuffer, (unsigned long *)outValue))
			{
				rValue = STATUS_FORMAT_ERROR;
			}
		}
		else
		{
			rValue = STATUS_FORMAT_ERROR;
		}
		break;
	case TYPE:
		if (lineLength >= (LINE_TYPE_INDEX + LINE_LENGTH_BYTE))
		{
			copyChars(valueBuffer, &lineBuffer[LINE_TYPE_INDEX], LINE_LENGTH_BYTE);
			if(STATUS_OK != htoi( valueBuffer, (unsigned long *)outValue))
			{rValue = STATUS_FORMAT_ERROR;}
		}
		else
		{
			rValue = STATUS_FORMAT_ERROR;
		}
		break;

	case DATA:
		if (lineLength >= (LINE_DATA_INDEX + (dataIndex * 2)))
		{
			copyChars(valueBuffer, &lineBuffer[LINE_DATA_INDEX + (dataIndex * 2)], LINE_LENGTH_BYTE);
			if(STATUS_OK != htoi( valueBuffer, (unsigned long *)outValue))
			{
				rValue = STATUS_FORMAT_ERROR;
			}
		}
		else
		{
			rValue = STATUS_FORMAT_ERROR;
		}
		break;

	case CHECKSUM:
		copyChars(valueBuffer, &lineBuffer[lineLength-2], 2);
		if(STATUS_OK != htoi( valueBuffer, (unsigned long *)outValue))
		{
			rValue = STATUS_FORMAT_ERROR;
		}
		break;
	}
	return rValue;
}

static void copyChars(uint8_t *dest, uint8_t* src, uint8_t length)
{
	while(length)
	{
		*dest = *src;
		dest++;
		src++;
		length--;
	}
	*dest = '\0';
}
static int htoi(const char *s, unsigned long *res)
{
	int c;
	unsigned long rc;
	*res = 0L;
	for (rc = 0; '\0' != (c = *s); s++)
	{
		if 		(c >= 'a' && c <= 'f')	{c = c - 'a' + 10;}
		else if (c >= 'A' && c <= 'F')	{c = c - 'A' + 10;}
		else if (c >= '0' && c <= '9')	{c = c - '0';}
		else							{return -1;}
		if 		(rc > (ULONG_MAX/16))	{return -1;}
		rc *= 16;
		rc += (unsigned long) c;
	}
	*res = rc;
	return 0;
}

void softReset(void)
{
	SCB_AIRCR = SCB_AIRCR_VECTKEY(0x5FA) | SCB_AIRCR_SYSRESETREQ_MASK;
	//wait until reset
	for(;;){}
}


