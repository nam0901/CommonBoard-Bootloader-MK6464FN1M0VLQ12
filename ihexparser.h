/*
 * ihexparser.h
 *
 *  Created on: Jul 5, 2017
 *      Author: tgack
 */

#ifndef SOURCES_IHEXPARSER_H_
#define SOURCES_IHEXPARSER_H_

#define LINE_LENGTH					(128)
//char test[LINE_LENGTH];
//#include "bootload.h"
#include "MK64F12.h"
#include <stdio.h>
#include <string.h>
/**
 * Intel Hex record type definitions
 *
 */
typedef enum
{
	DATA_REC = 0x00,				// Data
	EOF_REC = 0x01,					// Last line of Intel Hex File

	EXT_SEG_ADDR_REC = 0x02,		// 16-bit segment base address.
									// Current Value is multiplied by 16 and added
									// to the data record address before writing.
	START_SEG_ADDR_REC = 0x03,		// For 80x86 processors.
	EXT_LINEAR_ADDR_REC = 0x04,		// Big endian value. Upper 16 bits of the
									// absolute 32 bit address.
	START_LINEAR_ADDR_REC = 0x05	// For 80x86 processors.
}RECORD_TYPES;


typedef enum
{
	START_CODE,
	BYTE_COUNT,
	ADDRESS,
	TYPE,
	DATA,
	CHECKSUM
}FIELD;

typedef enum
{
	STATUS_OK,
	STATUS_END,
	STATUS_REBOOT,
	STATUS_DUMP_FILE_ERROR,
	STATUS_LINE_ERROR,
	STATUS_FORMAT_ERROR,
	STATUS_CHECKSUM_ERROR,
	STATUS_ERASE_ERROR,
	STATUS_WRITE_ERROR,
	STATUS_ERASE
}STATUS;

/**
 * Used to compose/decompose the word address based
 * on individual bytes in the ASCII text record.
 */
typedef union
{
	uint16_t value;
	struct
	{
		uint8_t lsb: 8;
		uint8_t msb: 8;
	}bytes;
}WORD_TYPE;

/**
 * Individual Intel Hex Line Record binary data
 */
typedef struct __RECORD__
{
	uint8_t byteCount;
	WORD_TYPE address;
	uint8_t recordType;
	uint8_t data[LINE_LENGTH];
	uint8_t dataCheck[LINE_LENGTH];//added to read from flash and confirm successful write
	uint8_t checksum;
}RECORD_TYPE;

/**
 * Composite Intel Hex record includes extended address
 */
typedef struct __BLOCK__
{
	uint32_t mswAddress;
	RECORD_TYPE record;
}BLOCK, *LP_BLOCK;

extern uint8_t line[LINE_LENGTH];
extern uint8_t lineNumber;
extern uint16_t maxLength;

void enqueueChar(uint8_t value);
void initializeParser(void);
void clearLine(void);
void softReset(void);
uint8_t getLineNumber(void);
uint8_t isLineReceived(void); //Check whether any data from the file is received
STATUS parseLine(LP_BLOCK block);
STATUS isRecordValid(LP_BLOCK block);

#endif /* SOURCES_IHEXPARSER_H_ */
