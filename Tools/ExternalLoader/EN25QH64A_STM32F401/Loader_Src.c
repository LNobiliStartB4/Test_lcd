#include <stdint.h>
#include "stm32f401xe.h"

#define LOADER_USED __attribute__((used))

#define EXTERNAL_BASE       0x90000000U
#define EXTERNAL_SIZE       0x00800000U
#define FLASH_PAGE_SIZE     256U
#define FLASH_SECTOR_SIZE   4096U

#define CMD_READ            0x03U
#define CMD_STATUS          0x05U
#define CMD_WRITE_ENABLE    0x06U
#define CMD_PAGE_PROGRAM    0x02U
#define CMD_SECTOR_ERASE    0x20U
#define CMD_CHIP_ERASE      0xC7U
#define CMD_JEDEC_ID        0x9FU

#define EN25_MANUFACTURER   0x1CU
#define EN25_CAPACITY_64M   0x17U
#define BUSY_MASK           0x01U
#define BUSY_TIMEOUT        0x02000000U

static uint32_t normalizeAddress(uint32_t address)
{
    return (address >= EXTERNAL_BASE) ? (address - EXTERNAL_BASE) : address;
}

static void selectFlash(void)
{
    GPIOB->BSRR = (uint32_t)GPIO_BSRR_BR_1;
}

static void deselectFlash(void)
{
    GPIOB->BSRR = GPIO_BSRR_BS_1;
}

static uint8_t transferByte(uint8_t value)
{
    while ((SPI3->SR & SPI_SR_TXE) == 0U)
    {
    }
    *((volatile uint8_t *)&SPI3->DR) = value;
    while ((SPI3->SR & SPI_SR_RXNE) == 0U)
    {
    }
    return *((volatile uint8_t *)&SPI3->DR);
}

static void endTransaction(void)
{
    while ((SPI3->SR & SPI_SR_BSY) != 0U)
    {
    }
    deselectFlash();
}

static void sendAddress(uint32_t address)
{
    transferByte((uint8_t)(address >> 16));
    transferByte((uint8_t)(address >> 8));
    transferByte((uint8_t)address);
}

static uint8_t readStatus(void)
{
    uint8_t status;
    selectFlash();
    transferByte(CMD_STATUS);
    status = transferByte(0xFFU);
    endTransaction();
    return status;
}

static int waitReady(void)
{
    uint32_t timeout = BUSY_TIMEOUT;
    while ((readStatus() & BUSY_MASK) != 0U)
    {
        if (timeout-- == 0U)
        {
            return 0;
        }
    }
    return 1;
}

static int writeEnable(void)
{
    selectFlash();
    transferByte(CMD_WRITE_ENABLE);
    endTransaction();
    return 1;
}

static int readBytes(uint32_t address, uint8_t *buffer, uint32_t size)
{
    uint32_t index;
    address = normalizeAddress(address);
    if ((buffer == 0) || (address >= EXTERNAL_SIZE) ||
        (size > (EXTERNAL_SIZE - address)))
    {
        return 0;
    }

    selectFlash();
    transferByte(CMD_READ);
    sendAddress(address);
    for (index = 0U; index < size; index++)
    {
        buffer[index] = transferByte(0xFFU);
    }
    endTransaction();
    return 1;
}

static int programPage(uint32_t address, const uint8_t *buffer, uint32_t size)
{
    uint32_t index;
    if ((size == 0U) || (size > FLASH_PAGE_SIZE) ||
        (((address & (FLASH_PAGE_SIZE - 1U)) + size) > FLASH_PAGE_SIZE))
    {
        return 0;
    }
    if (!writeEnable())
    {
        return 0;
    }

    selectFlash();
    transferByte(CMD_PAGE_PROGRAM);
    sendAddress(address);
    for (index = 0U; index < size; index++)
    {
        transferByte(buffer[index]);
    }
    endTransaction();
    return waitReady();
}

LOADER_USED int Init(void)
{
    uint8_t manufacturer;
    uint8_t memoryType;
    uint8_t capacity;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
    (void)RCC->APB1ENR;

    GPIOB->MODER = (GPIOB->MODER & ~(3U << (1U * 2U))) |
                   (1U << (1U * 2U));
    GPIOB->OTYPER &= ~(1U << 1U);
    GPIOB->OSPEEDR |= (3U << (1U * 2U));
    deselectFlash();

    GPIOC->MODER = (GPIOC->MODER &
                    ~((3U << (10U * 2U)) |
                      (3U << (11U * 2U)) |
                      (3U << (12U * 2U)))) |
                   (2U << (10U * 2U)) |
                   (2U << (11U * 2U)) |
                   (2U << (12U * 2U));
    GPIOC->OSPEEDR |= (3U << (10U * 2U)) |
                      (3U << (11U * 2U)) |
                      (3U << (12U * 2U));
    GPIOC->AFR[1] = (GPIOC->AFR[1] &
                    ~((0xFU << 8U) | (0xFU << 12U) | (0xFU << 16U))) |
                    (6U << 8U) | (6U << 12U) | (6U << 16U);

    SPI3->CR1 = 0U;
    SPI3->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI;
    SPI3->CR1 |= SPI_CR1_SPE;

    selectFlash();
    transferByte(CMD_JEDEC_ID);
    manufacturer = transferByte(0xFFU);
    memoryType = transferByte(0xFFU);
    capacity = transferByte(0xFFU);
    endTransaction();

    (void)memoryType;
    return (manufacturer == EN25_MANUFACTURER) &&
           (capacity == EN25_CAPACITY_64M);
}

LOADER_USED int Read(uint32_t Address, uint32_t Size, uint8_t *buffer)
{
    return readBytes(Address, buffer, Size);
}

LOADER_USED int Write(uint32_t Address, uint32_t Size, uint8_t *buffer)
{
    uint32_t address = normalizeAddress(Address);
    uint32_t offset = 0U;

    if ((buffer == 0) || (address >= EXTERNAL_SIZE) ||
        (Size > (EXTERNAL_SIZE - address)))
    {
        return 0;
    }

    while (offset < Size)
    {
        uint32_t pageSpace = FLASH_PAGE_SIZE -
                             ((address + offset) & (FLASH_PAGE_SIZE - 1U));
        uint32_t chunk = Size - offset;
        if (chunk > pageSpace)
        {
            chunk = pageSpace;
        }
        if (!programPage(address + offset, &buffer[offset], chunk))
        {
            return 0;
        }
        offset += chunk;
    }
    return 1;
}

LOADER_USED int SectorErase(uint32_t EraseStartAddress,
                            uint32_t EraseEndAddress)
{
    uint32_t current = normalizeAddress(EraseStartAddress) &
                       ~(FLASH_SECTOR_SIZE - 1U);
    uint32_t end = normalizeAddress(EraseEndAddress);

    if ((current >= EXTERNAL_SIZE) || (end >= EXTERNAL_SIZE))
    {
        return 0;
    }

    while (current <= end)
    {
        if (!writeEnable())
        {
            return 0;
        }
        selectFlash();
        transferByte(CMD_SECTOR_ERASE);
        sendAddress(current);
        endTransaction();
        if (!waitReady())
        {
            return 0;
        }
        current += FLASH_SECTOR_SIZE;
    }
    return 1;
}

LOADER_USED int MassErase(uint32_t Parallelism)
{
    (void)Parallelism;
    if (!writeEnable())
    {
        return 0;
    }
    selectFlash();
    transferByte(CMD_CHIP_ERASE);
    endTransaction();
    return waitReady();
}

LOADER_USED uint32_t CheckSum(uint32_t StartAddress,
                              uint32_t Size,
                              uint32_t InitVal)
{
    uint8_t buffer[64];
    uint32_t offset = 0U;
    while (offset < Size)
    {
        uint32_t index;
        uint32_t chunk = Size - offset;
        if (chunk > sizeof(buffer))
        {
            chunk = sizeof(buffer);
        }
        if (!readBytes(StartAddress + offset, buffer, chunk))
        {
            return 0U;
        }
        for (index = 0U; index < chunk; index++)
        {
            InitVal += buffer[index];
        }
        offset += chunk;
    }
    return InitVal;
}

LOADER_USED uint64_t Verify(uint32_t MemoryAddr,
                            uint32_t RAMBufferAddr,
                            uint32_t Size,
                            uint32_t missalignement)
{
    uint8_t buffer[64];
    uint8_t *expected = (uint8_t *)RAMBufferAddr;
    uint32_t byteCount = Size * 4U;
    uint32_t leading = missalignement & 0xFU;
    uint32_t trailing = (missalignement >> 16) & 0xFU;
    uint32_t offset = 0U;
    uint32_t checksum = 0U;

    MemoryAddr += leading;
    if ((leading + trailing) > byteCount)
    {
        return MemoryAddr;
    }
    byteCount -= leading + trailing;

    while (offset < byteCount)
    {
        uint32_t index;
        uint32_t chunk = byteCount - offset;
        if (chunk > sizeof(buffer))
        {
            chunk = sizeof(buffer);
        }
        if (!readBytes(MemoryAddr + offset, buffer, chunk))
        {
            return MemoryAddr + offset;
        }
        for (index = 0U; index < chunk; index++)
        {
            checksum += buffer[index];
            if (buffer[index] != expected[offset + index])
            {
                return (((uint64_t)checksum) << 32) |
                       (uint64_t)(MemoryAddr + offset + index);
            }
        }
        offset += chunk;
    }
    return ((uint64_t)checksum) << 32;
}
