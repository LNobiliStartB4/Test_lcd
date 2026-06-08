#ifndef W25Q128JV_H
#define W25Q128JV_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define W25Q128JV_SIZE_BYTES 0x01000000UL
#define W25Q128JV_VIRTUAL_BASE 0x90000000UL
#define W25Q128JV_ASSET_MANIFEST_OFFSET 0x00FFF000UL

bool W25Q128JV_Init(void);
bool W25Q128JV_IsReady(void);
bool W25Q128JV_ValidateAssetPackage(void);
bool W25Q128JV_Read(uint32_t address, uint8_t *data, uint32_t length);
bool W25Q128JV_StartDmaRead(uint32_t address, uint8_t *data, uint32_t length);
bool W25Q128JV_IsDmaReadActive(void);
void W25Q128JV_WaitForDmaRead(void);
void W25Q128JV_DmaCompleteCallback(void);

#ifdef __cplusplus
}
#endif

#endif /* W25Q128JV_H */
