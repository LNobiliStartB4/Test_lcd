
 /*********************************************************************************
  * @file           : cryptoRfidTag.c
  * @author			:
  * @brief          : Crypt & decrypt rfid tag
  *********************************************************************************
  *
  * Algorithm supplied by Microlab Elettronica s.a.s.,
  * implemented by StartB4 S.r.l.
  *
  *********************************************************************************/

#include <stdint.h>

void decryptTag(uint8_t *tagBuffer, uint8_t tagSize, uint8_t *uid)
{
// decodifica!
const char K5[]={123,97,55,12,222}; // chiave privata
const char K3[]="THD";
uint8_t i;
char dd1, dd0 = 0, dd = 0;
uint8_t hash = 0;
char key[16];       				// per sicurezza la key viene "assemblata" ogni volta nello stack...
char kk[8];

// compone la chiave privata completa usando la K5 e lo UID del Tag:
kk[0] = K5[0];
kk[1] = K5[3];
kk[2] = K3[2];
kk[3] = K5[2];
kk[4] = K3[0];
kk[5] = K5[1];
kk[6] = K3[1];
kk[7] = K5[4];

for (i = 0; i < 16; i += 2)
	{
	key[i] = uid[i>>1];    			// posizioni pari = UID
	key[i+1] = kk[i>>1];   			// posizioni dispari = chiave privata
	}

// applica l'XTEA per decriptare:
for (i = 0; i < tagSize; i++)
	{
	dd0 = dd;
	dd = tagBuffer[i];
	dd1 = dd ^ (key[hash]^dd0);
	hash = (hash + 1)%16;
	tagBuffer[i]=dd1;
	}
}

void encryptTag(uint8_t *tagBuffer, uint8_t tagSize, uint8_t *uid)
{
// decodifica!
const char K5[]={123,97,55,12,222}; // chiave privata
const char K3[]="THD";
uint8_t i;
char dd0 = 0, dd = 0;
uint8_t hash = 0;
char key[16];       				// per sicurezza la key viene "assemblata" ogni volta nello stack...
char kk[8];

// compone la chiave privata completa usando la K5 e lo UID del Tag:
kk[0] = K5[0];
kk[1] = K5[3];
kk[2] = K3[2];
kk[3] = K5[2];
kk[4] = K3[0];
kk[5] = K5[1];
kk[6] = K3[1];
kk[7] = K5[4];

for (i = 0; i < 16; i += 2)
	{
	key[i] = uid[i>>1];    			// posizioni pari = UID
	key[i+1] = kk[i>>1];   			// posizioni dispari = chiave privata
	}

// applica l'XTEA per decriptare:
for (i = 0; i < tagSize; i++)
	{
	dd0 = dd;
	dd = tagBuffer[i];
	dd = dd ^ (key[hash]^dd0);
	hash = (hash + 1)%16;
	tagBuffer[i]=dd;
	}
}

// this function calculates a CRC16 over a unsigned char Array with, LSB first
// @Param1 (DataBuf): An Array, which contains the Data for Calculation
// @Param2 (SizeOfDataBuf): length of the Data Buffer (DataBuf)
// @Param3 (Polynom): Value of the Generatorpolynom, 0x8408 is recommended
// @Param4 (Initial_Value):load value for CRC16, 0xFFFF is recommended for
// host to reader communication
// return: calculated CRC16
int16_t GetCrc(uint8_t *DataBuf, uint8_t SizeOfDataBuf, uint16_t Polynom, uint16_t Initial_Value)
{
    uint16_t Crc16;
    uint8_t Byte_Counter, Bit_Counter;

    Crc16 = Initial_Value;
    for (Byte_Counter=0; Byte_Counter < SizeOfDataBuf; Byte_Counter++) {
        Crc16^=DataBuf[Byte_Counter];
        for (Bit_Counter=0; Bit_Counter<8; Bit_Counter++) {
            if(( Crc16 & 0x0001)==0) Crc16>>=1;
            else Crc16=(Crc16>>1)^Polynom;
        }
    }
    return Crc16;
}

