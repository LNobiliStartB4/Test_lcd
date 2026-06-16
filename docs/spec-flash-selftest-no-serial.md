# Spec: Test W25Q64 senza seriale (isolamento glitch + render immagine ridotta)

Branch: `fix/w25q64-jedec-comm`
Stato: **DA APPROVARE** (Phase 1 — Specify)

## 1. Objective
Capire se i "glitch" del programmatore vengono dalla **seriale (UART)** o dalla
**SPI/flash**, e contemporaneamente mostrare una **vera immagine piccola** dalla
W25Q64 sul display — **senza usare la seriale**. Tutto on-device, esito a schermo.

Utente: sviluppatore (bring-up prototipo). Successo:
- Sappiamo con certezza dove nasce il glitch.
- Vediamo un'immagine reale renderizzata dalla flash esterna senza UART.

## 2. Approccio (due test, nessuna UART)
### Test A — Self-test SPI a volume grande (isolamento)
- Il firmware **genera** un pattern deterministico (es. LCG/contatore a 32 bit) →
  nessun dato da archiviare, quindi posso coprire un volume grande.
- Volume configurabile, default **262144 byte (256 KB)** sulla W25Q64.
- Flusso: erase settori → write a pagine da 256 B → read-back → confronto byte-a-byte.
- Misura: `errori`, `primo offset errato`, `byte atteso/letto`.
- Interpretazione:
  - **0 errori** → SPI/flash affidabile → i glitch sono nella **seriale**.
  - **>0 errori** → SPI/flash ancora marginale (cablaggio) → si lavora lì.

### Test B — Vera immagine piccola dalla flash (senza seriale, senza TouchGFX)
- Una **piccola** immagine reale in **RGB565 raw** è inclusa nella **flash interna**
  (array `const`).
- Al boot il firmware la **scrive nella W25Q64**, la **rilegge dalla W25Q64**, e la
  **disegna direttamente sul display** (blit via `Display_Set_Area` +
  `touchgfxDisplayDriverTransmitBlock`). **Niente TouchGFX, niente offset asset.**
- Dimostra il percorso completo flash-write → flash-read → display **senza UART**.
- Verdetto **visivo**: se vedi l'immagine corretta → tutto il percorso funziona.

## 3. Report a schermo (scelta: colore pieno)
- **Test A**: a fine self-test il display diventa **VERDE = PASS** / **ROSSO = FAIL**
  (via `DisplayDriver_Clear`), tenuto ~2 s. Nessun TouchGFX, nessun font.
- **Test B**: il verdetto è l'**immagine stessa** disegnata a schermo.
Sequenza al boot: Test A (verde/rosso) → Test B (immagine) → resta visibile.

## 4. Attivazione (come si entra in modalità test)
Opzione scelta: **macro di compile-time** `FLASH_SELFTEST_ENABLED` (default 0).
- Build normale → comportamento attuale.
- Build con `FLASH_SELFTEST_ENABLED=1` → al boot esegue Test A + Test B e mostra il report,
  **senza** entrare nel flasher UART e senza richiedere asset validi.
Così il test è isolato, non tocca il flusso di produzione, e si attiva/disattiva da un solo punto.

## 5. Tech / Commands
- Build: STM32CubeIDE (Debug). Per il test: definire `FLASH_SELFTEST_ENABLED=1`.
- Flash firmware: ST-Link (`Display_test_prova_internal.hex`).
- Nessuno strumento host, nessuna `flash_assets.py`, nessuna UART.

## 6. File coinvolti (stima)
- `Core/Src/w25q64.c/.h` — riuso `W25Q64_Init/Write/Read/EraseSector` (già presenti);
  aggiunta `W25Q64_SelfTest(volume, *result)` (pattern write+readback+confronto).
- `Core/Src/main.c` — sotto `#if FLASH_SELFTEST_ENABLED`: esegue self-test + copia
  immagine embeddata, salva risultati in un global, salta il flasher.
- `Core/{Inc,Src}/flash_selftest_image.*` — immagine piccola reale come array `const`
  (generata da un PNG/bitmap ridotto) + offset di destinazione.
- TouchGFX: una schermata di report testuale (o riuso di una esistente) che legge i risultati.

## 7. Code style
Conforme: `W25Q64_` prefix, niente malloc, `#if FLASH_SELFTEST_ENABLED`, costanti `kPascalCase`/define.

## 8. Testing / verifica
- Manuale sul device: leggere il report a schermo.
- A) atteso PASS con 0 errori se il cablaggio diretto regge i 256 KB.
- B) atteso immagine visibile + "image copy: PASS".

## 9. Boundaries
- **Sempre:** nessuna UART nel percorso di test; SPI3/DMA separati dal display.
- **Mai:** rompere il flusso di produzione (tutto dietro la macro, default off);
  toccare i pin display/touch.

## 10. Success criteria
- Con `FLASH_SELFTEST_ENABLED=1`: il display mostra l'esito di A e B.
- Test A dà un verdetto chiaro (errori=0 o >0) → isola seriale vs SPI.
- Test B mostra una vera immagine letta dalla flash esterna, **senza** seriale.

## Decisioni (confermate)
1. Volume Test A: **237616 byte** (= dimensione esatta del blob reale `assets.bin`) → confronto 1:1.
2. Immagine Test B: **scelgo io** una piccola immagine reale esistente, convertita in RGB565 raw.
3. Report: **colore pieno** (verde=PASS / rosso=FAIL) per Test A; Test B = immagine a schermo.
   Niente TouchGFX nel percorso di test (più semplice e robusto).
