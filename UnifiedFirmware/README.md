# Pressure Stabiliser - firmware unificato

Firmware single-MCU per la PCB finale. La cartella finale di lavoro e:

`C:\Firmware\Pressure_Stabiliser\Pressure_Stabiliser`

## Sorgenti considerate autorevoli

- Hardware e pin: `Pressure_Stabilizer.ioc`.
- UI e comportamento display: firmware `Display_test_prova`.
- RFID, attuatori e logica prodotto: firmware `RFIDtagReader_LEDdimmer`.
- Sensore MPXV5100DP: implementazione importata dal precedente progetto
  `UnifiedFirmware`, senza calibrazione persistente in questa fase.

Il vecchio `UnifiedFirmware` non e usato come terza architettura applicativa.

## Memorie esterne

- W25Q128JV, SPI3: immagini, font e testi TouchGFX.
- MB85RS256B, SPI2 condivisa con RFID: sessione Bandy versionata con CRC.
- La calibrazione pressione in FRAM e parcheggiata.

Gli asset TouchGFX sono linkati da `0x90000000`; il firmware li legge via
`TouchGFXDataReader` e SPI3 DMA.

## Rigenerazione

1. Aprire `Pressure_Stabilizer.ioc` in STM32CubeIDE/CubeMX.
2. Eseguire `Generate Code`.
3. Generare TouchGFX:

```powershell
& 'C:\TouchGFX\4.26.0\designer\tgfx.exe' generate -p .\TouchGFX -v
```

4. Compilare la configurazione `Release`.
5. Verificare il limite di Flash interna:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\check_memory.ps1
```

6. Estrarre il pacchetto Winbond:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Tools\build_winbond_assets.ps1
```

Il pacchetto produce:

- asset a offset Winbond `0x00000000`;
- manifest versione/dimensione/CRC a offset `0x00FFF000`.

Il firmware verifica JEDEC ID e CRC degli asset prima di avviare TouchGFX.

## Vincoli

- Build Release: Flash interna massima `256 KiB`.
- Nessun bridge Python/UART tra display e prodotto.
- Nessuna simulazione dispositivo nel firmware finale.
- Ogni modifica hardware deve essere fatta prima nell'IOC.
