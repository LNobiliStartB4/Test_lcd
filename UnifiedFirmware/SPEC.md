# Spec: Pressure Stabilizer — Firmware Unificato (PCB finale)

**Stato**: DRAFT v0.1 — in attesa di approvazione umana prima di passare a Phase 2 (Plan).
**Autore**: Lorenzo Nobili + Claude
**Data**: 2026-05-20
**Scope**: nuovo firmware single-MCU che sostituisce il sistema demo (PCB main + display Nucleo + bridge Python) con un unico binario su STM32F401RE quando arriva la PCB finale.

---

## 1. Objective

### Cosa stiamo costruendo
Un **firmware unificato** per il Pressure Stabilizer (dispositivo medico per vuoto controllato attivato via tag RFID) che gira su una **sola MCU STM32F401RE** della PCB finale e contiene:
- la logica applicativa attualmente nel firmware PCB main (`RFIDtagReader_LEDdimmer`),
- l'interfaccia TouchGFX attualmente nel firmware display (`Display_test_prova`),
- nessun bridge esterno (il protocollo UART tra PCB e display sparisce perché entrambi vivono nella stessa MCU).

### Perché
La PCB finale integra display + controllo in un singolo modulo. Il bridge Python e la doppia Nucleo erano artefatti di sviluppo. Vogliamo iniziare la convergenza **adesso**, mentre la PCB finale è ancora in produzione, in modo che all'arrivo dell'hardware ci sia già un firmware target-ready, testato off-target sui moduli pure-logic.

### Utente / contesto operativo
- **Operatore clinico**: scansiona tag, avvia/mette in pausa/termina sessione Bandy, legge pressione e tempo dal display touch. **Esegue la calibrazione del sensore** dal display: schermata dedicata che guida la procedura zero atmosferica + span 500 mbar via siringa inversa a pressione controllata.
- **Tecnico factory**: usa UART di servizio (USART1) per comandi diagnostici (`VER`, `KA`, `PCAL0`, `PCAL500`, `PCAL?`, `BEP`, ecc.). Stesso modulo `pressure_calib` invocato sia da UART che da UI.
- **Service**: collega via UART per calibrazione MPXV5100DP (`PCAL*`) e diagnostica come backup del flusso UI.

### Success criteria (testabili)
- [ ] Build CubeIDE warning-free su Pressure_Stabilizer.ioc spostato sotto `UnifiedFirmware/`.
- [ ] TouchGFX rigenerabile dal `.touchgfx` project incluso (no edit manuali a file generati).
- [ ] Display 480×320 RGB565 rendering ≥ 50 FPS misurati con TIM11 VSync.
- [ ] Bandy session FSM passa tutti i test off-target (transizioni, pause timer 30s, double-tap reset).
- [ ] Pressure: lettura PC5 + conversione MPXV5100DP coerente con datasheet (0 mbar a P1=P2=atm, 500 mbar a riferimento esterno).
- [ ] `PCAL0` → `PCAL500` → `PCAL?` sequence completa funziona end-to-end via UART.
- [ ] `PCAL500` prima di `PCAL0` ritorna `NACK\r`.
- [ ] RFID: scan tag valido → AUTHORIZED, examNum decrement persistito, burn a `examNum=0`.
- [ ] Hemorflow leak timeout funziona come oggi (commit 4145abb porting integrale).
- [ ] FRAM MB85RS256B legge/scrive su SPI2 condiviso con RFID senza race (test stress).
- [ ] Loop principale ≤ 200 ms in worst case (vincolo storico bridge — qui non serve, ma manteniamo come SLA UX).
- [ ] Flash usage ≤ 90% (480 KB su 512 KB). Lasciare margine per future feature.

### Vincolo hardware durante lo sviluppo (importante)

- **Prototipo PCB attuale** (questo repo, `RFIDtagReader_LEDdimmer.ioc`): ha pin mapping diverso (display su Nucleo separata + bridge Python). **NON compatibile** con il firmware unified.
- **PCB finale**: non ancora esistente fisicamente. Quando arriverà, avrà display integrato e pin mapping della SPEC §3.
- **Conseguenza**: durante M0..M5 il firmware unified si sviluppa **senza target hardware reale**. La validazione è:
  - **M1 (L3 pure-logic)**: TDD off-target completo + coverage ≥ 90% → confidence alta.
  - **M2 (BSP) / M3 (Drivers)**: solo compilation check (warning-free `-Werror`) + static analysis (cppcheck) + review codice vs datasheet/driver legacy → confidence media.
  - **M5 (TouchGFX UI)**: TouchGFX simulator su Windows per validazione visuale completa → confidence alta per UI logic.
- **M6 Bring-up hardware**: fase critica di validazione end-to-end **eseguita all'arrivo della PCB finale**. Tutti i bench check di M2/M3 si consolidano qui.
- **Firmware legacy** del prototipo: **intoccabile** durante lo sviluppo. La demo con bridge continua a girare.

### Non-goals (esplicitamente fuori scope)
- ❌ Mantenere protocollo bridge USB↔UART (sparirà del tutto).
- ❌ Persistenza in EEPROM emulata su flash interna (sostituita da FRAM esterna).
- ❌ Comandi factory distruttivi `ERASE` e `TAGRESET` (rimossi totalmente, non più gated).
- ❌ Porting "byte-per-byte" del codice attuale: il codice viene **riscritto da zero** con architettura pulita, mantenendo invariata la semantica funzionale.
- ❌ Implementare ora la persistenza FRAM della calibrazione pressione (RAM-only nella prima milestone, FRAM in milestone successiva quando driver portato e validato).

---

## 2. Tech Stack

| Componente | Versione / Spec | Note |
|---|---|---|
| MCU | **STM32F401RETx** LQFP64 | 84 MHz · 512 KB Flash · 96 KB RAM · stesso del prototipo |
| Toolchain | STM32CubeIDE 1.19 + GCC ARM 13.3 | C99 (`__STDC_VERSION__ >= 199901L`) |
| HAL | STM32Cube FW_F4 (latest CubeMX gen) | Non modificare driver HAL |
| Middleware RFID | **RFAL** (X-CUBE-NFC5) | Per ST25R3911, ISO15693 |
| Middleware UI | **X-CUBE-TOUCHGFX 4.26.0** | Rigenerabile, no edit a file gen |
| Linguaggio | C99 (app) + C++ (TouchGFX) | TouchGFX View/Presenter/Model in C++ |
| Test framework off-target | **doctest** + **CMake** | Pattern già usato in `Display_test_prova/tests/` |
| Host build | MinGW / MSVC su Windows | Solo per logica pure (no HAL stub deep) |

### Stack TouchGFX (recepito)
- Driver display: SPI1 prescaler /16 (PA5/6/7), CS=PD2, DC=PB10, RST=PA1, BL=PA8
- Driver touch: I2C1 @ 400 kHz (PB8/9), IRQ=PB7 EXTI falling pull-up, RST=PB5
- VSync: TIM11 (prescaler 8399, period 165 ≈ 60 Hz)
- Framebuffer: **da definire nel Plan** — vincolo 96 KB RAM, 480×320×2 = 307 KB non entra → strategia partial framebuffer / line buffer (verifica progetto display attuale)

---

## 3. Pin mapping (canonico — da `.ioc` Pressure_Stabilizer)

### Display SPI1
| Pin | Function | GPIO Label |
|---|---|---|
| PA5 | SPI1_SCK | — |
| PA6 | SPI1_MISO | — |
| PA7 | SPI1_MOSI | — |
| PD2 | GPIO_Output | `DISP_CS` |
| PB10 | GPIO_Output | `DISP_DC` |
| PA1 | GPIO_Output | `DISP_RST` |
| PA8 | GPIO_Output | `BACKLIGHT` |

### Touch I2C1 @ 400 kHz
| Pin | Function | GPIO Label |
|---|---|---|
| PB8 | I2C1_SCL | — |
| PB9 | I2C1_SDA | — |
| PB7 | EXTI falling, pull-up | `TOUCH_IRQ` |
| PB5 | GPIO_Output | `TOUCH_RST` |

### RFID + FRAM SPI2 (bus condiviso, CS distinti)
| Pin | Function | GPIO Label |
|---|---|---|
| PB13 | SPI2_SCK | — |
| PB14 | SPI2_MISO | — |
| PB15 | SPI2_MOSI | — |
| PB12 | GPIO_Output | `RFID_CS` |
| PC4 | GPIO_Input | `RFID_IRQ_OUT` |
| PB4 | GPIO_Output | `RFID_IRQ_IN` |
| PB6 | GPIO_Output | `FRAM_CS` |

### ADC1 (riconfigurazione canale prima di ogni read)
| Pin | Function | GPIO Label | Channel |
|---|---|---|---|
| PC5 | ADCx_IN15 | `MPXV5100DP` | ADC_CHANNEL_15 |
| PC0 | ADCx_IN10 | `PUMP_CURRENT` | ADC_CHANNEL_10 |

> **Nota**: nel `.ioc` solo CH15 è dichiarato come regular conversion. Allineare l'`.ioc` aggiungendo formalmente `ADC_CHANNEL_10` per rigenerabilità pulita (task di foundation).

### PWM TIM3 (prescaler 83, period 999 → ~1 kHz)
| Pin | Function | GPIO Label |
|---|---|---|
| PC6 | TIM3_CH1 | `Valve1` |
| PC7 | TIM3_CH2 | `Valve2` |
| PB0 | TIM3_CH3 | `Pump` |
| PC9 | TIM3_CH4 | `Valve3` |

### Discrete I/O
| Pin | Function | GPIO Label |
|---|---|---|
| PA0 | (timer da assegnare) | `BUZZER` — TIM2_CH1 o TIM5_CH1 (decidere nel Plan) |
| PC8 | GPIO_Output | `STATUS_LED` |
| PA4 | GPIO_Output | `CAM_DISABLE` |

### Seriale di servizio
| Pin | Function | Note |
|---|---|---|
| PA9 | USART1_TX | 115200 8N1 |
| PA10 | USART1_RX | IRQ enabled |

### Timer UI
- **TIM11**: prescaler 8399 / period 165 → ~60 Hz VSync TouchGFX

### Riservati
- PA11/PA12 = USB DM/DP riservati input
- PA13/PA14 = SWDIO/SWCLK debug

---

## 4. Project Structure

```
UnifiedFirmware/
├── Pressure_Stabilizer.ioc          # File CubeMX UNICO — sorgente di verità pin/clock/periferiche
├── SPEC.md                          # questo documento
├── PLAN.md                          # piano implementazione (Phase 2)
├── TASKS.md                         # task list ordinata (Phase 3)
├── Core/
│   ├── Inc/                         # Header pubblici moduli applicativi
│   └── Src/                         # Sorgenti applicazione + main.c (CubeMX-gen)
├── Drivers/                         # HAL ST + CMSIS (CubeMX-gen, NO EDIT)
│   ├── STM32F4xx_HAL_Driver/
│   └── CMSIS/
├── Middlewares/                     # RFAL + X-CUBE-TOUCHGFX (NO EDIT)
│   ├── ST/rfal/
│   └── ST/touchgfx/
├── TouchGFX/                        # Progetto TouchGFX rigenerabile
│   ├── App.touchgfx                 # File progetto editabile
│   ├── assets/                      # Asset originali (immagini, font sources)
│   ├── generated/                   # Output `Generate Code` — NO EDIT
│   ├── gui/                         # View/Presenter custom
│   │   ├── include/gui/
│   │   └── src/
│   ├── target/                      # BSP TouchGFX target-specific
│   └── simulator/                   # Build simulatore Windows TouchGFX
├── Tests/
│   └── host/                        # Test off-target doctest + CMake
│       ├── CMakeLists.txt
│       ├── test_pressure_mpxv5100dp.cpp
│       ├── test_pressure_calibration.cpp
│       ├── test_bandy_session_fsm.cpp
│       ├── test_pump_fsm.cpp
│       ├── test_rfid_tag_logic.cpp
│       ├── test_serial_parser.cpp
│       ├── test_hemorflow.cpp
│       └── fakes/                   # HAL fakes (gpio, tim, adc, spi, uart)
└── Docs/
    ├── architecture.md              # Diagramma layer + dipendenze
    ├── pin_map.md                   # Estratto/derivato da .ioc
    └── adr/                         # Architecture Decision Records
```

### Architettura a layer

```
┌───────────────────────────────────────────────────────┐
│ L5 UI       TouchGFX (View / Presenter / Model)       │  C++
├───────────────────────────────────────────────────────┤
│ L4 App      application_runtime · application_context │  C
│             application_feedback · serial_comm        │
├───────────────────────────────────────────────────────┤
│ L3 Services rfid_tag · bandy_fsm · pump_ctrl ·        │  C, PURE LOGIC,
│             pressure_calib · hemorflow_leak · persist │  TESTABLE OFF-TARGET
├───────────────────────────────────────────────────────┤
│ L2 Drivers  st25r3911(via RFAL) · mb85rs256b(FRAM) ·  │  C
│             mpxv5100dp · display_drv · touch_drv      │
├───────────────────────────────────────────────────────┤
│ L1 BSP      bsp_gpio · bsp_adc · bsp_pwm · bsp_uart · │  C, thin wrappers
│             bsp_spi1 · bsp_spi2(shared+mutex) · bsp_tim│
├───────────────────────────────────────────────────────┤
│ L0 HAL      STM32 HAL + CMSIS (CubeMX-generated)      │  C, NO EDIT
└───────────────────────────────────────────────────────┘
```

**Regole**:
- L5 chiama solo L4 (mai L2/L1/L0 direttamente).
- L4 chiama L3 e L2.
- L3 è **pure-logic**: nessun include di HAL, nessun side-effect non testabile.
- L2 chiama L1.
- L1 incapsula HAL e re-esporta API minime (es. `Bsp_Spi2_TransferLocked(...)` con mutex).
- Nessuna chiamata diretta a `HAL_*` da L3/L4. Eccezione tollerata: tick `HAL_GetTick()` astratto come `Bsp_Timer_GetTickMs()`.

### Bridge TouchGFX ↔ Application
- `TouchGFX/gui/src/Model.cpp` è l'**unico** punto di contatto tra TouchGFX e L4.
- `Model::tick()` → chiamato da TouchGFX a ogni frame; query a `ApplicationContext_*` o `ApplicationRuntime_*` per stato.
- Eventi UI (tap "Start", "Pause", "End") → `Presenter` → `Model` → `ApplicationRuntime_*` API.
- Nessun include di HAL/RFAL dentro `gui/`.

---

## 5. Code Style

Coerente con CLAUDE.md del repo. Riassunto applicabile al nuovo firmware:

### Esempio canonico (modulo pure-logic L3)

```c
// Core/Inc/pressure_calib.h
#ifndef PRESSURE_CALIB_H
#define PRESSURE_CALIB_H

#include <stdbool.h>
#include <stdint.h>

#define PRESSURE_CALIB_REF_SPAN_MBAR  500u

typedef enum {
    PRESSURE_CALIB_STATE_FACTORY = 0,  /* gain/zero from datasheet */
    PRESSURE_CALIB_STATE_ZERO    = 1,  /* PCAL0 done, awaiting PCAL500 */
    PRESSURE_CALIB_STATE_FULL    = 2,  /* PCAL0 + PCAL500 done */
} pressure_calib_state_t;

typedef struct {
    pressure_calib_state_t state;
    uint16_t  zero_adc_raw;     /* ADC reading at P=0 */
    float     gain_mbar_per_lsb;
    uint16_t  ref_span_mbar;
    /* FRAM-ready layout: pad to multiple of 16 bytes for future persist */
    uint8_t   _reserved[4];
} pressure_calib_t;

void PressureCalib_Init(pressure_calib_t *self);

/* Returns true on success, false if procedure violated (e.g. PCAL500 before PCAL0). */
bool PressureCalib_ApplyZero(pressure_calib_t *self, uint16_t adc_now);
bool PressureCalib_ApplySpan(pressure_calib_t *self, uint16_t adc_now);
void PressureCalib_Clear(pressure_calib_t *self);

/* Pure conversion: ADC count → mbar. Returns INT32_MIN if input out of plausible range. */
int32_t PressureCalib_AdcToMbar(const pressure_calib_t *self, uint16_t adc);

#endif
```

### Convenzioni
- **Function naming**: `ModuleName_FunctionName()` PascalCase + PascalCase (es. `BandyFsm_Tick`, `PressureCalib_AdcToMbar`). Coerente con codebase attuale.
- **Types**: `*_t` suffix (`pressure_calib_t`, `bandy_state_t`).
- **Constants**: `UPPER_SNAKE_CASE` via `#define` (o `static const`).
- **Module private**: `static`.
- **Indentation**: **2 spazi** (uniformiamo tutto il nuovo codice — niente più il mix tab/spazi del codebase legacy).
- **Header guards**: `#ifndef MODULE_H / #define MODULE_H / #endif`.
- **Boolean**: `bool` da `<stdbool.h>`.
- **Integer width**: `uint8/16/32_t`, `int16/32_t` espliciti.
- **`const`** dove non-modificabile. **`volatile`** solo registri / flag ISR.
- **No `malloc`/`free`** in runtime. Allocazioni statiche o stack.
- **No ricorsione, no `goto`/`continue`** salvo motivo forte commentato.
- **Magic numbers**: vietati. Sempre `#define` con unità nel nome (`_MBAR`, `_MS`, `_HZ`).
- **Parentesi esplicite** in espressioni complesse. **`{}`** sempre, anche per single-statement.
- **Commenti**: spiegano il **perché**, non il cosa. Documenta unità (mbar, mA, ms, °C), range validi, scaling.

### Build flags target
- `-Wall -Wextra -Wshadow -Wdouble-promotion -Werror`
- `-fdata-sections -ffunction-sections -Wl,--gc-sections` (per controllo Flash)
- Static analysis raccomandato: `cppcheck --enable=warning,performance,portability` su `Core/Src/`

### Build flags host test
- `-Wall -Wextra -Wshadow -Wpedantic -fprofile-arcs -ftest-coverage` (coverage opzionale)

---

## 6. Testing Strategy

### TDD obbligatorio per ogni modulo pure-logic (L3)
Workflow per ogni unità:
1. **Test list** scritta prima del codice (acceptance criteria → casi di test).
2. **Test rosso**: scrivere prima il test, vederlo fallire (compile o assertion).
3. **Implementation**: minimo codice per passare il test.
4. **Test verde**: tutti i test del modulo passano.
5. **Refactor**: pulizia senza rompere test.
6. **Solo allora** si passa al modulo successivo.

> Vincolo esplicito dall'utente: **"passare al successivo solo quando tutti i test di quella precedente sono passati con successo"**.

### Livelli di test

| Livello | Cosa | Dove | Quando |
|---|---|---|---|
| **Unit off-target** | L3 services + utility (pure C, no HAL) | `Tests/host/` doctest+CMake | Continuamente, gate per commit |
| **Module off-target con fake** | L2 driver con HAL fake (SPI/ADC/GPIO) | `Tests/host/fakes/` | Per logica driver non puramente IO |
| **Integration on-target** | L4 + L3 + L2 + L1 real | Su Nucleo F401 + bench | Per ogni milestone |
| **Hardware-in-the-loop** | Sistema completo con pompa/valvole/RFID/MPXV5100DP | Su PCB finale | Pre-release |
| **Acceptance** | Scenari operatore reali | Su PCB finale + manichino test | Pre-release clinico |

### Test list iniziale (vivi nel TASKS.md, riassunti qui)

**`pressure_calib`** (primo modulo da implementare con TDD):
- [ ] `Init` → state FACTORY, gain factory da datasheet, zero factory `0.04 * Vs * partitore`.
- [ ] `AdcToMbar(adc_factory_zero)` ≈ 0 mbar tolleranza ±20 mbar (sensor noise).
- [ ] `AdcToMbar(adc_for_500mbar)` ≈ 500 mbar tolleranza ±20 mbar.
- [ ] `AdcToMbar(adc_for_1000mbar)` ≈ 1000 mbar tolleranza ±30 mbar (near full-scale).
- [ ] `AdcToMbar(adc=0)` → INT32_MIN (implausible).
- [ ] `AdcToMbar(adc=4095)` → INT32_MIN (implausible / out of partitore range).
- [ ] `ApplySpan` prima di `ApplyZero` → false, state INVARIATO.
- [ ] `ApplyZero(adc)` → state ZERO, `zero_adc_raw = adc`.
- [ ] `ApplySpan(adc)` dopo ApplyZero → state FULL, `gain` ricalcolato.
- [ ] `Clear` → state FACTORY, zero/gain factory restore.
- [ ] Pressione negativa post-conversione clampata a 0 per consumer (regola: il controllo non vede valori negativi).

**`bandy_fsm`**:
- [ ] Init → state WAIT_RFID.
- [ ] `OnTagScanned(valid)` da WAIT_RFID → AUTHORIZED.
- [ ] `OnStartPressed` da AUTHORIZED → RUNNING.
- [ ] `OnPausePressed` da RUNNING → PAUSED, timer 30s armed.
- [ ] Tick a 30001 ms da PAUSED → RUNNING (auto-resume).
- [ ] `OnEndPressed` da RUNNING/PAUSED → WAIT_RFID + StopRfidScan signal.
- [ ] Wrap-around tick HAL_GetTick() gestito (unsigned subtraction).

**`pump_ctrl`** (FSM vuoto):
- [ ] IDLE → VACUUM quando `target_active` && `pressure < target_mbar`.
- [ ] VACUUM → IDLE quando `pressure >= target_mbar`.
- [ ] Riavvio con isteresi: IDLE → VACUUM solo se `pressure < target - HYSTERESIS_MBAR`.
- [ ] Current limit: VACUUM → FAULT se `current_ma > CURRENT_LIMIT_MA` per N tick.
- [ ] FAULT non riparte automaticamente.

**`rfid_tag` (logica)**:
- [ ] `DecodeTag(blob, uid)` con chiave UID-keyed corretta → struct decoded.
- [ ] `DecodeTag(blob, uid)` con UID errato → INVALID.
- [ ] `EncodeTag(struct, uid)` → blob deterministico.
- [ ] Roundtrip Encode→Decode invariante.
- [ ] `DecrementExamNum`: 5 → 4 → ... → 0; sotto a 0 saturato a 0.
- [ ] `IsBurned(tag)` true sse `examNum == 0`.

**`hemorflow_leak`**:
- [ ] Timeout perdite scatta dopo N ms di pressione fuori target.
- [ ] Reset timeout al rientro in target.
- [ ] (Porting integrale logica commit 4145abb)

**`serial_parser`**:
- [ ] Comandi noti → enum corretto + ACK.
- [ ] Comando sconosciuto → NACK.
- [ ] `VAC?` → frame VAC formattato.
- [ ] `PCAL0\r` → invoke calib zero.
- [ ] `PCAL500\r` → invoke calib span.
- [ ] `PCAL?\r` → frame stato calibrazione.
- [ ] `PCALCLR\r` → invoke calib clear.
- [ ] Parser ignora `\n`, accetta solo `\r` terminator.
- [ ] Buffer overflow protetto.
- [ ] ERASE e TAGRESET non più riconosciuti (entrambi → NACK).

### Coverage target
- L3 services: **≥ 90% line coverage** sui moduli pure-logic.
- L2 drivers con fake: ≥ 70%.
- L4 application: best-effort (molti side-effect HAL).

---

## 7. Boundaries

### Always
- Spec-driven: ogni nuovo modulo passa per SPEC.md → PLAN.md → TASKS.md prima del codice.
- TDD off-target prima del codice target per L3 services.
- Build warning-free con flag indicati.
- Coerenza con pin mapping definito qui (la canonical source è il `.ioc`).
- Documentare unità di misura nei commenti (mbar, ms, mA, °C).
- `\r` come terminator seriale (mai `\n`).
- ISR brevi: zero printf/malloc/HAL_Delay/logica.
- Controllare return di tutte le chiamate HAL/RFAL.

### Ask first
- Modifiche al pin mapping (PCB schematic-impacting).
- Aggiunta di font/bitmap TouchGFX (vincolo Flash 512 KB).
- Cambi al clock tree o periferiche in `.ioc`.
- Cambi semantica Bandy session FSM o `VAC?` frame format (impatto display).
- Cambi al protocollo seriale (impatto factory/service tooling).
- Scelta esatta del timer per buzzer (PA0 può andare su TIM2_CH1 o TIM5_CH1 — proporrò nel Plan).
- Strategia framebuffer TouchGFX (partial vs line buffer — dipende dal driver display esistente).

### Never
- Editare file in `Drivers/STM32F4xx_HAL_Driver/` o `Middlewares/ST/`.
- Editare file in `TouchGFX/generated/` (sovrascritti da `Generate Code`).
- Commitare con test rossi.
- Saltare TDD per "fretta" su un modulo L3.
- Includere HAL/RFAL in L3 o L5.
- Re-introdurre EEPROM emulata su flash interna.
- Re-introdurre ERASE / TAGRESET (eliminati definitivamente).
- Re-introdurre il bridge USB↔UART o il protocollo a frame `VAC?` polling (la UART di servizio risponde a comando, non polla).

---

## 8. Open Questions (da chiudere nel PLAN.md)

1. **Timer buzzer**: PA0 → TIM2_CH1 o TIM5_CH1? Il vecchio firmware usava TIM2_CH1 con Period 56000. Frequenze richieste: BEP 1500 Hz, BOP 300 Hz → period a 84 MHz richiede prescaler. Decisione nel Plan.
2. **SPI2 sharing (RFID + FRAM)**: strategia mutex? Per ora bare-metal no RTOS — atomicità via disabilitazione preempt o flag busy? Definire pattern.
3. **Framebuffer TouchGFX strategy**: dipende dal driver display in `Display_test_prova` — partial frames o line buffer? Da ispezionare nel Plan.
4. **VAC? frame format**: lo manteniamo identico per compatibilità con eventuale strumento factory? O lo semplifichiamo (campi del bridge ora inutili)?
5. **Authentication token flow**: come si integra con Bandy FSM al boot (auth all'avvio? per ogni tag scan?).
6. **Tre valvole vs due**: il vecchio firmware aveva Valve1/Valve2. Il nuovo .ioc ha Valve1/Valve2/Valve3 su TIM3_CH1/2/4. Cosa fa Valve3? Da chiarire (release vent? Bypass? Air inlet?).
7. **Persistenza calibrazione**: proposta — FRAM persistence dopo che il driver MB85RS256B è portato e validato (Task in mid-stream). RAM-only nella prima milestone.

---

## 9. Verification (gate verso Phase 2)

Prima di passare al PLAN.md:
- [ ] Spec coperto tutte le 6 aree (Objective / Tech / Structure / Style / Test / Boundaries).
- [ ] Pin mapping completo e privo di conflitti.
- [ ] Test list iniziale per i primi 3 moduli (pressure_calib, bandy_fsm, pump_ctrl).
- [ ] Success criteria specifici e testabili.
- [ ] Open questions enumerate e tracciate.
- [ ] **Approvazione esplicita dell'utente** ("ok va bene", "procedi al plan", o feedback puntuale).

---

## Changelog

- **v0.1 — 2026-05-20**: prima draft. In attesa di review.
