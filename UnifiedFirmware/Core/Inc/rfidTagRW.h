/**
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/*! \file
 *
 *  \author 
 *
 *  \brief Demo functionality header file
 *
 */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef DEMO_H
#define DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "platform.h"
#include "st_errno.h"
#include "../../Middlewares/ST/rfal/Inc/rfal_nfcv.h"

/* Exported types ------------------------------------------------------------*/
// General struct, var type

typedef struct
{
uint16_t 	examNum;                   	// examination number
uint16_t 	durationMinutes;          	// examination duration [minutes]
int16_t 	a_var;                      // not used - keep for portability
int16_t 	b_var;                      // not used - keep for portability
int16_t 	c_var;                    	// not used - keep for portability
int16_t 	d_var;                      // not used - keep for portability
uint16_t 	firm;                       // firm (type of examination / machine)
uint16_t 	pad1;
uint16_t 	pad2;
uint16_t 	pad3;
uint16_t 	pad4;
uint16_t 	pad5;
uint16_t 	pad6;
uint16_t 	pad7;
uint16_t 	pad8;
uint16_t	proRfidCRC;					// CRC
}PRO_RFID_TAG;
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#define PRO_RFID_TAG_BLOCK_LENGTH	4
#define PRO_RFID_TAG_BLOCK_NUM		8
#define PRO_RFID_TAG_TAG_SIZE		PRO_RFID_TAG_BLOCK_NUM*PRO_RFID_TAG_BLOCK_LENGTH

#define MAX_NUMBER_OF_EXAMINATION	5000
#define TAG_PROGRAM_DEFAULT_EXAM_NUM 1000U
#define PRO_RFID_TAG_MIN_DURATION_MINUTES 1U
#define PRO_RFID_TAG_MAX_DURATION_MINUTES 15U
#define PRO_RFID_TAG_DEFAULT_DURATION_MINUTES 15U

#define APPLICATION_FIRM_MASK	0xFF00

//#define RFID_TAG_CRC_CHECK_ENABLED

typedef enum  	{
				TAG_OK 					= 1,
				TAG_RW_WIP				= 0,
				TAG_ERR_POINTER 		= -1,
				TAG_ERR_CRC 			= -2,
				TAG_ERR_FIRM 			= -3,
				TAG_ERR_MAX_EXAM_NUM	= -4,
				TAG_ERR_READ			= -5,
				TAG_ERR_WRITE			= -6,
				TAG_ERR_VERIFY			= -7,
				TAG_EXPIRED				= -8,
				TAG_ERR_DURATION		= -9
				}TagRWRetVal;

typedef enum	{
				TAG_APPROVED			= 1,
				TAG_SEARCHING			= 0,
				TAG_NOT_APPROVED		= -1
				}TagStatus;

typedef struct
				{
				uint8_t			ledLightIntlevel;
				bool			rfidScanActive;
				TagStatus		rfidTagStatus;
				bool			rfidUpdateTag;
				bool			rfidEraseTag;
				bool			rfidProgramTag;
				uint8_t			rfidTagUid[RFAL_NFCV_UID_LEN];
				PRO_RFID_TAG	rfidTag;
				bool			bepTriggered;
				bool			bopTriggered;
				uint8_t			writeRetryCounter;
				TagRWRetVal		rfidTagRWRetVal;
				TagRWRetVal		lastRfidTagRWRetVal;
				uint16_t		firmToCheck;
				uint32_t		keepAliveTimer;
				}PRO_STATION;

/* Exported functions ------------------------------------------------------- */
bool demoIni( void );
extern TagRWRetVal demoCycle(PRO_STATION* proStationP);

#ifdef __cplusplus
}
#endif

#endif /* DEMO_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
