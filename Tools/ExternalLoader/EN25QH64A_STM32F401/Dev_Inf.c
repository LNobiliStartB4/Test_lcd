#include "Dev_Inf.h"

__attribute__((used, section(".Dev_Info")))
const struct StorageInfo StorageInfo =
{
    "EN25QH64A_STM32F401RE_SPI3",
    NOR_FLASH,
    0x90000000U,
    0x00800000U,
    0x00000100U,
    0xFFU,
    {
        {0x00000800U, 0x00001000U},
        {0U, 0U}
    }
};
