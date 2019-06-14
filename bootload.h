/*
 * bootload.h
 *
 *  Created on: Jun 15, 2017
 *      Author: tgack
 */

#ifndef SOURCES_BOOTLOAD_H_
#define SOURCES_BOOTLOAD_H_

#include "ihexparser.h"
#include "PE_Types.h"

#define APP_START_ADDRESS		(0x00008000)//4000
#define APP_FLASH_NUM_PAGES		(0x00000400)
#define APP_FLASH_SECTOR_SIZE	(0x00000200)
#define READY					("READY\r")
#define OK						("OK\r")
#define LINE_ERROR				("LE\r")
#define FORMAT_ERROR			("FE\r")
#define CHECKSUM_ERROR			("CE\r")
#define ERASE_ERROR				("EE\r")
#define WRITE_ERROR				("WE\r")
#define ERASE_CONFIRM			("Erase the Application?\r")
#define ERASING 				(" ERASING\r")
#define ERASED	 				("ERASED\r")
#define DUMP_FILE_ERROR			("DFE\r")
#define EOF_ERROR 				("NO_EOF\r")
#define REBOOT 					(" GO TO BOOTLOADER\r")
#define APP_PRESENECE			("APP INSTALLED\r")
#define HERE					("HERE\r")
#define CASE2					("CASE2\r")
#define DEFAULT					("Default\r")

extern void launchTargetApplication(unsigned long l);
void initalizeBootloader(LP_BLOCK block);
void sendResponse(char* response);
void receiveData(void);
STATUS processBootloadRecord(LP_BLOCK block);
STATUS eraseApplicationSpace();
STATUS confirmAppPresence(void);


#endif /* SOURCES_BOOTLOAD_H_ */
