# Spec: Affidabilità comunicazione W25Q64 (JEDEC) + verifica parsing firmware

Branch: `fix/w25q64-jedec-comm`

## Objective
La flash esterna W25Q64 (SPI3) deve rispondere al comando JEDIC `0x9F` con
`EF 40 17` **in modo stabile e ripetibile**, così che il flasher UART possa
programmare `assets.bin` e il rendering legga le immagini. Oggi il firmware
legge in modo **ripetibile** `EF 45 54` (manufacturer `EF` giusto, byte 2–3
sbagliati con pattern stile clock `0101`), indipendentemente dal clock SPI
(/2 e /64 danno lo stesso valore).

## Findings (verifica parsing firmware — COMPLETATA)
Codice di lettura JEDEC rivisto: **corretto, nessun bug.**
- `W25Q64_CommandRead`: Select → `HAL_SPI_Transmit(cmd)` → `HAL_SPI_Receive(data)` → Deselect. Sequenza SPI standard.
- `W25Q64_ReadJedecId`: invia `0x9F`, legge 3 byte.
- `W25Q64_IdIsExpected`: confronto con `EF/40/17`.
- `W25Q64_SetSpiMode`: abort+disable+`MODIFY_REG(CPOL/CPHA)`+enable (mode 0 di default).
- SPI3 MspInit: PC10/11/12, AF6, push-pull, very-high speed, MISO no-pull — corretto.

Conclusione: la corruzione **ripetibile e clock-correlata**, con `EF` sempre
giusto, è **fisica/di segnale**, non un errore di parsing software.

## Tech / Commands
- Build: STM32CubeIDE (Debug) → `Display_test_prova_internal.hex` + `assets.bin`.
- Diagnostica (sola lettura): `python Tools\flash_assets.py --port COMx --diagnose`
- Programmazione: `python Tools\flash_assets.py --port COMx assets.bin`
- SPI3 attualmente a prescaler /64 (~656 kHz) per il bring-up.

## Hypotheses (ordinate per probabilità)
1. **Crosstalk CLK→MISO / massa insufficiente** sulla breadboard (i byte errati
   hanno pattern da clock). [molto probabile]
2. **Contatti breadboard marginali** su MISO/CLK/MOSI.
3. SPI mode: escluso lato firmware (mode 0 corretto; `EF` si legge).

## Verification plan
1. `--diagnose`: legge JEDEC 32× in **mode 0 e mode 3** + ID 90h/ABh + status + SFDP.
   - `stable_count` alto ma valore sbagliato → corruzione **deterministica** (crosstalk/massa).
   - SFDP atteso `53 46 44 50` ("SFDP"); se sballato → corruzione dati confermata.
   - mode 3 corretto e mode 0 no (o viceversa) → problema legato al campionamento.
2. Test fisico: massa dedicata corta + GND tra CLK e MISO + fili corti, NO breadboard.
3. Ripetere `--diagnose` dopo ogni intervento di cablaggio.

## Success criteria
- `--diagnose` mostra `EF 40 17` con `stable_count = 32/32` in mode 0.
- SFDP = `53 46 44 50`.
- `flash_assets.py assets.bin` completa fino a `OK`.

## Boundaries
- Sempre: SPI3 e DMA1 separati dal display (SPI1/DMA2); CS attivo basso.
- Mai: toccare il bus/DMA del display; rimuovere i controlli JEDEC/manifest.

## Open questions
- Su breadboard potrebbe non raggiungere mai 32/32; target reale = PCB custom.
