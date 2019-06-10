/* ###################################################################
**     Filename    : main.c
**     Project     : Commboard_Bootloader
**     Processor   : MK64FN1M0VLQ12
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2018-12-12, 10:27, # CodeGen: 0
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 01.01
** @brief
**         Main module.
**         This module contains user's application code.
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */

/* Including needed modules to compile this module/procedure */
#include "Cpu.h"
#include "Events.h"
#include "AS1.h"
#include "FLASH1.h"
#include "TI1.h"
#include "TU1.h"
#include "GPIO1.h"
/* Including shared modules, which are used for whole project */
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "PDD_Includes.h"
#include "Init_Config.h"
/* User includes (#include below this line is not maintained by Processor Expert) */
#include "bootload.h"
#include "bootstrap.h"
#include "ihexparser.h"
#include <stdio.h>
#include <string.h>
/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)

//When it says COMPLETE, it means we're in test application

/*lint -restore Enable MISRA rule (6.3) checking. */
{
	/* Write your local variable definition here */
	BLOCK block;
	STATUS state;
	STATUS errorCatch;
    char temp[4];
    uint8_t flag  = 0;
    LDD_TError Error;

	/*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
	PE_low_level_init();


	/*** End of Processor Expert internal initialization.                    ***/
	/* Write your code here */
	/* For example: for(;;) { } */
	/*---Application Presence Check---*/
//	if((getBootstrapConfig() == LAUNCH_CODE))
//    {
//		launchTargetApplication(APP_START_ADDRESS);
//    }

	/*---Clear Application Space---*/
//    if(eraseApplicationSpace() != STATUS_OK)
//    {
//    	while(1);	// Unable to erase flash. TODO: Handle fatal error, discuss with P.M.
//    }

    /*---Bootloader Mode---*/
    initalizeBootloader(&block);
    initializeParser();

    TI1_Init(TI1_DeviceData);
    TI1_Enable(TI1_DeviceData);
	AS1_TurnTxOff(AS1_DeviceData); //Turn off Tx
	AS1_TurnRxOff(AS1_DeviceData); //Turn off Rx
    sendResponse(ERASE_CONFIRM);

    /*---Bootloader Check ---*/
    AS1_TurnRxOn(AS1_DeviceData); //Turn on Rx

	           /* Start reception of one character */
	while (!RxFlag && counter < 10000 ) {                                      /* Wait until characters is received */
		AS1_TurnRxOn(AS1_DeviceData); //Turn on Rx
		GPIO1_SetFieldValue(NULL, PTE, 0b0); //disable Tx, enable Rx
		AS1_ReceiveBlock(AS1_DeviceData, line, 4);
		if( (counter%1000) == 999)
				{
					time = (counter/1000)+1;
					sprintf(temp, "%d\r",time);
					sendResponse(temp);
				}
		if(strcmp((char*)line, "YES") == 0 || strcmp((char*)line,"yes") == 0){
			TI1_Disable(TI1_DeviceData);


			sendResponse(ERASING);
			flag = 1;
			break;


		}
	}

	if(eraseApplicationSpace() != STATUS_OK)
	{
		sendResponse(ERASE_ERROR);
		while(1);
	}else{
		sendResponse(ERASED);
	}

	if(!flag){
			AS1_CancelBlockReception(AS1_DeviceData);
		}


	clearLine();


	if(confirmAppPresence() == STATUS_OK)
		{
			launchTargetApplication(APP_START_ADDRESS);
		}
	sendResponse(READY);


	//Cannot send "YES" during dumping the file
		//If so, erase all the application space and do the reset


	for(;;)
	{
		receiveData();	//wait for a character

		switch(isLineReceived()){
		case 1:
//				sendResponse(test);
				state = parseLine(&block);
				if(state == STATUS_OK)
				{
					state = processBootloadRecord(&block);
					switch(state)
					{
						case STATUS_OK:
							break;
						case STATUS_END:
							switch(errorCatch)
							{
								case STATUS_CHECKSUM_ERROR:
									sendResponse(CHECKSUM_ERROR);
									break;
								case STATUS_ERASE_ERROR:
									sendResponse(ERASE_ERROR);
									break;
								case STATUS_WRITE_ERROR:
									sendResponse(WRITE_ERROR);
									break;
								case STATUS_LINE_ERROR:
									sendResponse(LINE_ERROR);
									break;
								case STATUS_FORMAT_ERROR:
									sendResponse(FORMAT_ERROR);
									break;
								default:
									sendResponse(OK);
									if(confirmAppPresence() == STATUS_OK)
									{
										launchTargetApplication(APP_START_ADDRESS);
									}
									break;
							}
						default:
							errorCatch = state;
							break;
					}
				}else if(state == STATUS_REBOOT){
					sendResponse(REBOOT);
					softReset();
				}
				else
				{
					errorCatch = state;
				}

				clearLine();	//use function here instead of inside parseLine to cover parseLine fail

		break;
		case 2:
			// AS1 On Block Send: realterm to processor (TxFlag)
			// AS1 On Receive: processor to realterm (RxFlag)
			sendResponse(DUMP_FILE_ERROR);
			softReset();
			break;
		default:

			break;
	}

}

  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/



/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.5 [05.21]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
