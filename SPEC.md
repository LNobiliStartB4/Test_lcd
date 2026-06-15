# SPEC — Feedback visivo al tocco (puntatore "ripple")

## 1. Obiettivo

Aggiungere un feedback visivo che indichi all'operatore **dove ha toccato lo schermo**: un
cerchio "ripple" giallo THD che appare nel punto premuto, **segue il dito** durante il
trascinamento e **svanisce al rilascio**.

- **Utenti target:** operatori che usano il display (RVA15MD, 320×480) e sviluppatori in fase di
  demo/debug nel simulatore.
- **Perché:** su touch capacitivo senza cursore non è sempre chiaro se/dove il tocco è stato
  registrato; il puntatore dà conferma immediata e rende le demo più leggibili.

### Decisioni di design (confermate)

| Aspetto | Scelta |
|---|---|
| Forma | Cerchio "ripple" morbido che svanisce |
| Comportamento | Compare alla pressione, **segue il dito**, svanisce al rilascio |
| Ambito | **Simulatore + dispositivo reale** (sempre attivo) |
| Colore | **Giallo THD `#f0aa00`** |

## 2. Comportamento funzionale (acceptance criteria)

1. **Pressione (PRESSED):** entro 1 frame appare un cerchio centrato sulle coordinate del tocco,
   raggio ~18 px, giallo THD, semitrasparente (alpha pieno dell'asset ~70%).
2. **Trascinamento (DRAGGED/MOVED):** il cerchio segue le coordinate del dito senza lag percepibile.
3. **Rilascio (RELEASED):** il cerchio **svanisce** con una breve animazione (~250 ms: alpha→0,
   leggero ingrandimento del raggio per l'effetto "ripple"). Terminata l'animazione il widget è
   invisibile e non ridisegnato.
4. **Trasparente all'input:** il puntatore **non** deve catturare/consumare i tocchi — i pulsanti
   e gli slider sottostanti continuano a funzionare identici a prima.
5. **Globale:** funziona su **tutte** le schermate dell'app (incluse le schermate admin), senza
   regressioni di rendering.
6. **Sim + device:** identico nel simulatore e sul target F401 (84 MHz, Partial Framebuffer),
   senza far scendere il frame rate sotto i 60 Hz nominali.
7. **Disattivabile a compile-time:** una macro `TOUCH_FEEDBACK_ENABLED` (default 1) permette di
   escludere tutto il sistema dalla build.

## 3. Architettura e struttura

Vincolo: TouchGFX non offre un overlay globale (`Screen::add` è `protected`, il container dello
screen non è esposto, e con Partial Framebuffer non si disegna direttamente sul framebuffer).
Soluzione: **stato globale aggiornato in un solo punto** + **widget presente su ogni schermata**
che legge lo stato e si anima da solo nel proprio `handleTickEvent`.

```
ClickEvent/DragEvent
   │  (override, chiama prima la base per non alterare il routing)
   ▼
FrontendApplication::handleClickEvent / handleDragEvent
   │  scrive {x, y, phase} nel singleton
   ▼
TouchFeedbackState (singleton)  ──letto ogni tick──▶  TouchFeedback (widget su ogni screen)
                                                         └─ anima il ripple (posizione, alpha, raggio)
```

### Componenti nuovi

| File | Ruolo |
|---|---|
| `TouchGFX/gui/include/gui/common/TouchFeedbackState.hpp` | Singleton header-only: ultime coordinate + fase (`Idle`/`Pressed`/`Released`) + contatore evento. |
| `TouchGFX/assets/images/TouchDot.svg` | Disco giallo THD `#f0aa00` con bordo morbido (radial), usato come ripple. |
| Custom Container **`TouchFeedback`** (TouchGFX Designer) | Contiene l'immagine del dot; nel suo `handleTickEvent` legge `TouchFeedbackState`, segue il dito, gestisce la dissolvenza al rilascio. Si comporta da widget non toccabile. |

### File modificati

| File | Modifica |
|---|---|
| `TouchGFX/gui/include/gui/common/FrontendApplication.hpp` / `.cpp` | Override di `handleClickEvent(const ClickEvent&)` e `handleDragEvent(const DragEvent&)`: chiamano **prima** la base, poi aggiornano `TouchFeedbackState`. |
| `TouchGFX/Display_test.touchgfx` | Aggiunta di un'istanza di `TouchFeedback` come **ultimo** componente (topmost) di ogni schermata con contenuto (Screen1–13, 18). Modifica programmatica come per le icone back. |
| `TouchGFX/gui/include/gui/common/AdminScreens.hpp` | Aggiunta del widget `TouchFeedback` (via `this->add(...)`) nelle schermate admin hardcoded (PIN + menu + diagnostica). |

### Note di rendering

- Il widget è `setTouchable(false)` e occupa la sua sola bounding-box (no full-screen) per ridurre
  l'area invalidata: ad ogni movimento si invalidano solo il rettangolo vecchio e quello nuovo.
- Il ripple usa un `touchgfx::Image`/`SvgImage` con `setAlpha()` per la dissolvenza. L'eventuale
  ingrandimento ("ripple") si ottiene con `TextureMapper`/`ScalableImage` **solo se** non degrada
  le prestazioni; in caso contrario MVP = dot che segue il dito + fade-out di alpha (senza scala).
- Nessun uso di `CanvasWidgetRenderer` se si resta su Image/SvgImage (evita setup buffer dedicato).

## 4. Stile del codice

- C++ conforme al resto del progetto: `touchgfx::` esplicito, membri `camelCase`, costanti
  `kPascalCase` in namespace anonimo, niente eccezioni/RTTI, niente allocazioni dinamiche.
- Colore centralizzato come costante (`kTouchFeedbackColor = Color::getColorFromRGB(240,170,0)`).
- Tutto il sistema sotto `#if TOUCH_FEEDBACK_ENABLED`.
- Le schermate generate non si modificano a mano: le istanze del container si aggiungono via
  Designer/`.touchgfx` e si rigenerano.

## 5. Strategia di test

1. **Verifica visiva nel simulatore** (criterio principale): avviare l'app, fare tap e drag su più
   schermate (dashboard, settings, keypad, admin) e osservare comparsa/inseguimento/dissolvenza.
2. **Regressione input:** confermare che pulsanti, slider e navigazione funzionino come prima
   (il widget non deve intercettare i tocchi).
3. **Unit test** (cartella `tests/`, già presente con CMake/`test_model`): testare la logica pura
   di `TouchFeedbackState` e il calcolo dell'animazione (decadimento alpha nel tempo, transizioni
   di fase) senza dipendenze dal framebuffer.
4. **Sanity build su target:** compilazione Debug in STM32CubeIDE e check che il frame rate resti
   fluido (nessun blocco evidente durante il drag).

## 6. Boundaries

**Fai sempre:**
- Chiamare la base in `handleClickEvent`/`handleDragEvent` (non rompere il routing del touch).
- Mantenere il widget non toccabile e limitato alla sua bounding-box.
- Tenere tutto dietro `TOUCH_FEEDBACK_ENABLED` e a costo nullo quando disattivato.
- Far funzionare il feedback su **tutte** le schermate (designer + admin hardcoded).

**Chiedi prima di:**
- Modificare in massa `Display_test.touchgfx` (diff ampio su molte schermate).
- Aggiungere nuovi asset immagine al progetto.
- Introdurre `TextureMapper`/`ScalableImage` o `CanvasWidgetRenderer` se impattano le prestazioni.

**Non fare mai:**
- Toccare `main.c` / `Display_test_prova.ioc` o periferiche hardware (questa feature è solo UI).
- Modificare a mano i file generati (`*ViewBase`, `generated/`).
- Bloccare o ritardare la consegna degli eventi touch alle schermate.
- Far scendere il frame rate sul target con redraw a tutto schermo ogni frame.

## Decisioni aperte da confermare in implementazione

- Effetto "ripple" con ingrandimento del raggio (TextureMapper) **oppure** sola dissolvenza di
  alpha (più leggero) come MVP.
- Diametro del cerchio (default proposto: ~36 px) e durata del fade-out (default: ~250 ms).

---

## 7. Refinement — il puntatore non deve "congelarsi" sui bottoni di navigazione

### Obiettivo
Sul **dispositivo reale**, toccando un **bottone che cambia schermata**, il pallino resta fisso
nel punto premuto per un istante (poi sparisce). Su un'area non cliccabile il timing è corretto.
Obiettivo: **stesso timing ovunque** — il pallino deve sparire subito anche quando il tap avvia
una navigazione, senza apparire "incantato" durante il cambio schermata.

### Causa (root cause)
Con la Partial Framebuffer Strategy, al cambio schermata si ridisegna **l'intero** display via
SPI (~100 ms a 21 MHz su F401). Il pallino è già nel framebuffer nel punto premuto e resta
fisicamente visibile finché il ridisegno della nuova schermata non sovrascrive quei pixel. Su
un'area non cliccabile non c'è cambio schermata, quindi il pallino svanisce con l'aggiornamento
rapido della sua piccola area. `cancel()` (già presente) nasconde il widget, ma la pulizia fisica
avviene comunque dentro il ridisegno lento → il pallino "resta fisso poi sparisce".

### Soluzione proposta
**Differire la transizione di un solo frame quando il pallino è visibile.** In
`FrontendApplication::handlePendingScreenTransition()`: se c'è una transizione in sospeso **e** il
puntatore è visibile, chiamare `cancel()` (nasconde + invalida la piccola area del dot) e **non**
eseguire la transizione in quel frame (return). Al frame successivo il puntatore è nascosto →
la transizione procede. Così il device pulisce velocemente il pallino e **poi** fa il cambio
schermata, senza pallino congelato.

- Aggiungere `TouchFeedbackController::isPointerVisible()` (espone `animator.isVisible()`).
- Ritardo massimo introdotto alla navigazione: **1 frame (~16 ms)**.

### Opzionale (robustezza sul device)
Soglia di movimento: in `recordMove`, considerare "attività" solo spostamenti oltre ~4–5 px, così
il jitter del touch capacitivo non tiene vivo il pallino mentre si tiene premuto fermo.

### Success criteria
- Sul device, dopo un tap su un bottone di navigazione, il pallino sparisce **prima** che inizi il
  ridisegno della nuova schermata (nessun pallino fisso durante la transizione).
- La navigazione resta percepita come immediata (≤ 1 frame aggiunto).
- Nessun cambiamento per i tap su aree non cliccabili (comportamento già corretto).

### Boundaries
- **Sempre:** non ritardare la navigazione più di 1 frame; eseguire comunque la transizione al
  frame successivo (mai bloccarla).
- **Non fare mai:** scartare/perdere la transizione in sospeso; introdurre un loop che rimanda la
  transizione all'infinito.

### Verifica
- Test su dispositivo reale (criterio principale).
- Unit test per `isPointerVisible()` e la logica di stato già coperta; il differimento di un frame
  è framework-coupled → verifica sul device/simulatore.

### Decisione aperta
- Il ritardo di 1 frame (~16 ms) sulla navigazione è accettabile? (Praticamente impercettibile.)

---

# Spec: Grafica (immagini) TouchGFX in flash esterna Winbond (SPI3)

## Objective
Spostare le **immagini** TouchGFX dalla flash interna dell'F401RE (512 KB, vicina al limite) alla
**flash SPI esterna Winbond** integrata sul modulo display, per **liberare flash interna**. L'F401
**non ha QUADSPI** → niente memory-mapping: si usa il modello TouchGFX **non-memory-mapped
(FlashDataReader)** — i bitmap vengono letti **on-demand** dalla Winbond via **SPI3 + DMA** in una
cache RAM. Esiste già un'implementazione di **riferimento sullo stesso MCU** in `UnifiedFirmware/` — ma il
codice della memoria esterna lì è **presente e MAI verificato sul campo** (NON è garantito
funzionante). Quindi questa feature è un **porting attento e tutto da validare** di quel setup,
adattando il pin CS e il chip.

Successo: il firmware entra in flash interna con ampio margine; le immagini appaiono identiche
(lette dalla Winbond); la diagnostica admin mostra la Winbond presente e il pacchetto asset valido.

## Tech stack / Hardware
- STM32F401RE, TouchGFX 4.26, Partial Framebuffer (invariato). Display su **SPI1** (invariato).
- Winbond **W25Qxx** su **SPI3** (bus separato dal display):
  - SCK=**PC10**, MISO=**PC11**, MOSI=**PC12** (AF6), CS=**PC3** (GPIO out, attivo basso) — *scelta
    utente; PC3 risulta libero nel `.ioc`*.
  - DMA: **DMA1_Stream0** (SPI3_RX), **DMA1_Stream5** (SPI3_TX); prescaler /2 (~21 MHz).
  - Base virtuale asset **0x90000000**; manifest in coda alla flash (magic "ADHT" + CRC32).
- **Chip confermato: Winbond W25Q64 (8 MB)** → region `ASSET_FLASH LENGTH=8M`, base 0x90000000,
  JEDEC capacità **0x17** (manufacturer 0xEF, memtype 0x40), manifest a offset **0x007FF000**
  (8 MB − 4 KB). Il driver di UnifiedFirmware è per W25Q128JV (16 MB, 0x18): va **adattato** a
  8 MB / 0x17 (dimensione + controllo JEDEC + offset manifest).

## Commands
- **TouchGFX Designer → Generate Code**: rigenera il DataReader, mette le immagini in
  `ExtFlashSection` e produce il **binario asset** (es. `Artifacts/Winbond/*_assets.bin`).
- **Build**: STM32CubeIDE (Debug). Flash firmware come ora (ST-Link).
- **Flash asset sulla Winbond**: scrivere il `.bin` all'offset 0 della W25Q tramite
  **STM32CubeProgrammer con external loader** (o programmatore SPI). Il firmware la **legge** soltanto.

## Project Structure (file da portare/modificare da `UnifiedFirmware/` → progetto principale)
- `Core/Inc/w25q*.h`, `Core/Src/w25q*.c` — driver Winbond (su `hspi3`, CS `ASSET_FLASH_CS`=PC3).
- `Core/Src/RVA15MD_DataReader.c` (de-stub) — i `DataReader_*` chiamano il driver w25q.
- `STM32F401RETX_FLASH.ld` — aggiungere region `ASSET_FLASH (rx): ORIGIN=0x90000000, LENGTH=<dim chip>`
  e mappare `ExtFlashSection` (immagini) su `ASSET_FLASH`. (Font/testi restano in flash interna.)
- `Core/Src/main.c` + `Display_test_prova.ioc` (sync obbligatorio): `MX_SPI3_Init` (PC10/11/12, /2),
  GPIO **PC3** = output HIGH (ASSET_FLASH_CS), DMA1 Stream0/Stream5, IRQ `DMA1_Stream0_IRQHandler` →
  `DataReader_DMACallback`. `main.h`: `ASSET_FLASH_CS_Pin=GPIO_PIN_3`, `ASSET_FLASH_CS_GPIO_Port=GPIOC`.
- `TouchGFX/application.config`: `image_configuration.section` ed `extra_section` → `"ExtFlashSection"`
  (font lasciati in `IntFlashSection`).
- Generate produce `TouchGFXGeneratedDataReader.*` + `image_*.cpp` con `LOCATION_ATTRIBUTE("ExtFlashSection")`.
- `Model.cpp` (opz.): popolare la diagnostica Winbond reale (winbondAvailable/Id/SizeBytes/assetPackageValid)
  dal driver invece dei valori hardcoded `false`.

## Code Style
Conforme all'esistente: blocchi `USER CODE BEGIN/END` per il codice in `main.c`; **regola di sync
`.ioc`** per ogni modifica HW; CS attivo basso, HIGH a riposo. Riusare i pattern del driver
`UnifiedFirmware` adattando solo il pin CS (PB1→PC3) e la dimensione/JEDEC del chip.

## Testing Strategy
- **Build**: il `.map` mostra le immagini fuori dalla flash interna; nessun overflow, ampio margine.
- **Funzionale (device)**: scrivere il `.bin` asset sulla Winbond → avvio → **immagini renderizzate
  correttamente** su tutte le schermate; nessun asset mancante/glitch.
- **Diagnostica admin "MEMORY STATUS"**: Winbond **PRESENTE**, ID corretto, asset **VALIDO**.
- **Prestazioni**: rendering fluido (lettura SPI3 @21 MHz + DMA, cache); nessun rallentamento marcato.
- Niente unit test (HW/integrazione); verifica sul dispositivo.

## Boundaries
- **Sempre**: sincronizzare il `.ioc` (SPI3/GPIO/DMA); tenere SPI3 e DMA1 **separati** dal display
  (SPI1 / DMA2_Stream3); CS attivo basso, HIGH a riposo.
- **Chiedi prima di**: spostare anche **font/testi** in esterna; cambiare la dimensione della region;
  cambiare i pin scelti.
- **Non fare mai**: usare il bus/DMA del display per la flash; rompere il rendering del display;
  toccare i pin di display/touch già assegnati.

## Success Criteria
- Firmware entra in flash interna con margine (immagini spostate in esterna).
- Immagini identiche a prima, lette dalla Winbond; diagnostica Winbond OK; nessun overflow.

## Stato / Open Questions
- ✅ **Chip**: Winbond **W25Q64 (8 MB)** — risolto (JEDEC 0xEF/0x40/0x17, manifest 0x007FF000).
- ✅ **Pin + .ioc**: SPI3 PC10/11/12 + CS **PC3** cablati; il `Display_test_prova.ioc` è già stato
  aggiornato (SPI3 + PC3 `ASSET_FLASH_CS` + DMA1_Stream0/Stream5 + NVIC, IPNb=11/PinsNb=30) →
  **rigenerabile**. Verificare aprendolo in CubeMX prima di rigenerare il codice.
- ⚠️ **Riferimento NON collaudato**: l'integrazione esterna di UnifiedFirmware non è mai stata
  provata. Bring-up incrementale obbligatorio: 1) leggere JEDEC ID (atteso EF 40 17); 2) validare il
  manifest; 3) renderizzare UNA immagine da flash esterna; 4) poi tutte. Prevedere fallback se il
  manifest non valida.
- ✅ **Programmazione `.bin`** sulla Winbond: risolto → **flasher UART nel firmware** (nessun
  hardware extra). `Core/Src/asset_flasher.c` riceve il blob su USART2 e programma il chip; host =
  `tools/flash_assets.py`. CRC host (zlib) == CRC firmware (`asset_crc32`), verificato (0xCBF43926).

## Implementazione (in repo)
Tutto committato sul branch corrente. Parti **verificate su host** (doctest, 5/5):
- `Core/{Inc,Src}/asset_manifest.*` — CRC-32 + parse/validazione manifest (HAL-free, testato).
- `Core/{Inc,Src}/asset_flash_layout.*` — aritmetica page/sector per la programmazione (testato).

Parti **HAL-bound (non compilabili qui**, dipendono dai simboli generati da CubeMX — `hspi3`,
`huart2`, `ASSET_FLASH_CS_*`):
- `Core/{Inc,Src}/w25q64.*` — driver W25Q64: read (blocking+DMA) **e** write/erase.
- `Core/Src/RVA15MD_DataReader.c` — de-stubbato, instrada `DataReader_*` → `W25Q64_*`.
- `Core/{Inc,Src}/asset_flasher.*` — flasher UART (knock → erase → streaming → manifest → validate).
- `STM32F401RETX_FLASH.ld` — region `ASSET_FLASH` @0x90000000 (8M); `ExtFlashSection` → ASSET_FLASH
  (solo immagini; font e `IntFlashSection` restano in flash interna).
- `TouchGFX/application.config` — sezione immagini → `ExtFlashSection` (font invariati).
- `TouchGFX/Display_test.touchgfx` — `AvailableSections` include già `ExtFlashSection`.

## Checklist bring-up on-device (passi manuali rimasti)
1. **CubeMX**: apri il `.ioc`, verifica SPI3 (PC10/11/12) + `ASSET_FLASH_CS`=PC3 + DMA1, **Generate
   Code**. Conferma che `main.h` ora definisca `ASSET_FLASH_CS_Pin`/`_GPIO_Port` e `main.c` crei
   `hspi3`/`huart2`.
2. **main.c (USER CODE)**: dopo `MX_SPI3_Init()` aggiungi `W25Q64_Init();` (atteso JEDEC EF 40 17);
   poi, prima di `MX_TouchGFX_Init()`, chiama `AssetFlasher_RunIfRequested(3000);` (3 s di finestra
   knock al boot). Include `w25q64.h` e `asset_flasher.h`.
3. **TouchGFX Designer**: **Generate Code** (crea `TouchGFXGeneratedDataReader` che chiama le
   `DataReader_*`, e il bridge `TouchGFXDataReader`).
4. **Build** in STM32CubeIDE. Verifica che la flash interna ora **non** contenga più le immagini
   (niente overflow) e che `ExtFlashSection` sia a 0x90000000.
5. **Estrai il blob**: `arm-none-eabi-objcopy -O binary --only-section=ExtFlashSection
   Debug/Display_test_prova.elf assets.bin`.
6. **Flash firmware** sulla Nucleo (ST-Link).
7. **Flash asset**: reset board, entro 3 s lancia `python tools/flash_assets.py --port COMxx
   assets.bin` (USART2 = VCP ST-Link). Attendi `OK`.
8. **Verifica**: reset → le immagini sono renderizzate leggendo dalla Winbond. (Opz.) diagnostica
   admin "MEMORY STATUS" = Winbond presente + asset valido.

> Nota: il flasher è **opzionale a runtime** — senza knock l'app parte normale. La validazione
> manifest non blocca il rendering (TouchGFX legge comunque gli offset); serve solo da check di
> integrità.
