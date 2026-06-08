/*
 * cryptoRfidTag.h
 *
 *  Created on: Mar 7, 2024
 *      Author: utente
 */

#ifndef INC_CRYPTORFIDTAG_H_
#define INC_CRYPTORFIDTAG_H_

void encryptTag(uint8_t *tagBuffer, uint8_t tagSize, uint8_t *uid);
void decryptTag(uint8_t *tagBuffer, uint8_t tagSize, uint8_t *uid);
int16_t GetCrc(uint8_t *DataBuf, uint8_t SizeOfDataBuf, uint16_t Polynom, uint16_t Initial_Value);

#endif /* INC_CRYPTORFIDTAG_H_ */
