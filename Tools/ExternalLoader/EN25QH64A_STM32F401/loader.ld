ENTRY(Init)

MEMORY
{
  RAM (xrw) : ORIGIN = 0x20000004, LENGTH = 0x00017FFC
}

SECTIONS
{
  .text :
  {
    KEEP(*(.text.Init))
    KEEP(*(.text.Read))
    KEEP(*(.text.Write))
    KEEP(*(.text.SectorErase))
    KEEP(*(.text.MassErase))
    KEEP(*(.text.CheckSum))
    KEEP(*(.text.Verify))
    *(.text*)
    *(.rodata*)
  } > RAM

  .data :
  {
    *(.data*)
  } > RAM

  .bss (NOLOAD) :
  {
    *(.bss*)
    *(COMMON)
  } > RAM

  .Dev_Info :
  {
    KEEP(*(.Dev_Info))
  } > RAM

  /DISCARD/ :
  {
    *(.comment*)
    *(.ARM.attributes*)
  }
}
