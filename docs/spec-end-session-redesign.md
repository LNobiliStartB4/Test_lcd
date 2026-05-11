# Spec: End-Session Redesign + Tag-Burn-on-End

## Objective

Sostituire la schermata Screen7 (EndConfirm) con un layout più ricco
che comunichi all'operatore l'irreversibilità dell'azione, e
implementare il **burn effettivo del tag RFID** lato PCB quando
l'operatore conferma "End". Tag = one-shot per visita: dopo End il tag
non deve più autorizzare alcuna sessione.

### Storia del problema

Il design attuale di Screen7 ha tre limiti:
1. Non comunica visivamente che l'azione è distruttiva (tag bruciato).
2. I bottoni "No"/"Yes" sono simmetrici, mentre l'azione pericolosa
   (Yes) andrebbe minimizzata visivamente.
3. La conferma End non invalida realmente il tag — il modello "one-
   shot per visita" non è enforced. Un tag può autorizzare più sessioni
   se l'operatore non lo distrugge fisicamente.

## Tech Stack

- **PCB firmware**: STM32 + HAL, C. Toolchain STM32CubeIDE Debug.
- **Display firmware**: TouchGFX 4.26, C++ su STM32F401RE @ 84 MHz.
  Display 480×320 RGB565 (ST7789).
- **Bridge demo**: Python 3 + pyserial.

## Commands

- **Build PCB**: STM32CubeIDE → progetto `RFIDtagReader_LEDdimmer` →
  Build (Debug).
- **Build Display**: STM32CubeIDE → progetto `Display_test_prova` →
  Build (Debug).
- **Run bridge**: `python display_bridge.py --pcb COMx --display COMy`.

## Project Structure (file toccati)

```
RFIDtagReader_LEDdimmer/Core/Inc/application_runtime.h    ← nuovo stato ENDING
RFIDtagReader_LEDdimmer/Core/Src/application_runtime.c    ← burn-on-end logic
RFIDtagReader_LEDdimmer/Core/Src/serial_comm.c            ← (forse) nuovo VACENDBURN o riuso VACEND
RFIDtagReader_LEDdimmer/Tools/display_bridge/display_bridge.py ← nessun cambio

Display_test_prova/TouchGFX/assets/images/
  ├ alert_triangle_white.png  (NEW, 24×24)
  ├ trash_red.png             (NEW, 24×24)
  └ arrow_left_black.png      (NEW, 18×18)

Display_test_prova/TouchGFX/gui/include/gui/screen7_screen/Screen7View.hpp  ← nuovi membri
Display_test_prova/TouchGFX/gui/src/screen7_screen/Screen7View.cpp           ← layout completo
Display_test_prova/TouchGFX/gui/include/gui/model/DashboardTypes.hpp         ← nuovo BandySessionEnding
Display_test_prova/TouchGFX/gui/src/model/Model.cpp                          ← mappatura stato 4
```

## Code Style

### Display — Screen7View (code-only override)

I widget esistenti generati in `Screen7ViewBase` vengono nascosti
(`setVisible(false)` + `invalidate()`) in `Screen7View::setupScreen()`
prima di aggiungere i nuovi widget. La Designer mostrerà ancora il
layout vecchio, ma a runtime sarà coperto da quello nuovo. Pattern:

```cpp
void Screen7View::setupScreen()
{
    Screen7ViewBase::setupScreen();
    hideInheritedWidgets();
    buildOuterFrame();
    buildHeader();
    buildTimeBox();
    buildActionButtons();
    applyBandyState(presenter ? presenter->getBandyState() : BandyState());
}
```

### PCB — burn-on-end

`ApplicationRuntime_EndVacuumSession()` non chiama più
`ResetBandySession(false)` direttamente. Invece entra in stato
`APPLICATION_BANDY_STATE_ENDING`, attiva i flag erase e lascia che il
loop principale completi il burn.

```c
void ApplicationRuntime_EndVacuumSession(void)
{
  PRO_STATION *proStation = ApplicationContext_GetProStation();

  ApplicationFeedback_SetWarningBuzzerActive(false);
  ApplicationRuntime_StopVacuumCycleInternal(false);

  proStation->rfidUpdateTag = true;
  proStation->rfidEraseTag = true;
  proStation->rfidScanActive = true;

  bandyState = APPLICATION_BANDY_STATE_ENDING;
}
```

Nuovo handler quando il burn completa (in `ProcessRfidScan` o
`HandleRfidError` filtrato per `ENDING`):

```c
if (bandyState == APPLICATION_BANDY_STATE_ENDING &&
    proStation->rfidTagRWRetVal == TAG_EXPIRED)
{
  /* successo del burn */
  proStation->bepTriggered = true;        // beep "fine sessione"
  ApplicationRuntime_StopRfidScan();
  ApplicationRuntime_ResetBandySession(false);
}
```

## Visual Spec (Screen7 nuovo layout, 480×320)

Tutte le coordinate in pixel. Colori RGB888 (TouchGFX converte a
RGB565 col bayer dithering al runtime).

### Layout overview

```
┌─────────────────────────────────────────────┐  outer red border 2 px
│  ┌──────────────────────────────────────┐  │
│  │   ⚠ IRREVERSIBLE ACTION              │  │  pill (22 px)
│  │                                       │  │
│  │   End session?                        │  │  title  (~26 px)
│  │   The RFID tag will be burned         │  │
│  │   and remaining time will be lost     │  │  subtitle (2 lines)
│  │                                       │  │
│  │  ┌────────────────────────────────┐  │  │
│  │  │ TIME YOU'LL LOSE                 🗑 │  │  time box (60 px)
│  │  │ 12:45                              │  │
│  │  └────────────────────────────────┘  │  │
│  │                                       │  │
│  │  ┌──────────────┐  ┌──────────┐     │  │  buttons (60 px)
│  │  │ ← Continue   │  │   End    │     │  │
│  │  └──────────────┘  └──────────┘     │  │
│  └──────────────────────────────────────┘  │
└─────────────────────────────────────────────┘
```

### Widget posizionamento e proprietà

| # | Widget | Tipo | X, Y, W, H | Colori / note |
|---|---|---|---|---|
| 1 | `outerBorder` | BoxWithBorder | 0, 0, 480, 320 | Border #E24B4A, size 2, fill black |
| 2 | `pillBg` | Box | 140, 20, 200, 22 | Fill #E24B4A |
| 3 | `pillIcon` | Image | 150, 23, 16, 16 | `alert_triangle_white.png` |
| 4 | `pillText` | TextArea | 170, 22, 168, 16 | "IRREVERSIBLE ACTION", white, 10–11 pt, letter-spaced |
| 5 | `newTitle` | TextArea | 16, 52, 448, 26 | "End session?", white #FFFFFF, ~17 pt, centered |
| 6 | `newSubtitle` | TextArea | 16, 86, 448, 36 | "The RFID tag will be burned\nand remaining time will be lost", pink #F7C1C1, ~11 pt, centered, line-spacing 2 |
| 7 | `timeBoxBg` | Box | 16, 140, 448, 60 | Fill #1B0909 (≈ red 12% on black). Alpha 255 (no transparency) |
| 8 | `timeBoxBorder` | BoxWithBorder | 16, 140, 448, 60 | Border #E24B4A, size 1, fill transparent (alpha 0) |
| 9 | `timeLabel` | TextArea | 28, 150, 200, 12 | "TIME YOU'LL LOSE", pink #F7C1C1, 8 pt, letter-spaced |
| 10 | `timeValue` | TextArea | 28, 164, 200, 32 | wildcard MM:SS, white #FFFFFF, ~20 pt — **riusa endVisitValueBuffer del base** |
| 11 | `trashIcon` | Image | 422, 158, 24, 24 | `trash_red.png` |
| 12 | `continueButton` | FlexButton | 16, 240, 264, 60 | Fill white #FFFFFF, pressed #DDDDDD. Callback → `cancelClicked()` |
| 13 | `continueArrow` | Image | 60, 264, 18, 18 | `arrow_left_black.png` |
| 14 | `continueLabel` | TextArea | 82, 256, 180, 28 | "Continue", black #000000, ~14 pt, left-aligned |
| 15 | `endButton` | FlexButton | 288, 240, 176, 60 | BoxWithBorder: fill black #000000, border #E24B4A size 1. Callback → `confirmClicked()` |
| 16 | `endLabel` | TextArea | 288, 256, 176, 28 | "End", red #E24B4A, ~14 pt, centered |

### Stato "Holding tag" (durante burn)

Quando l'utente preme `endButton` e bandyState transita a `ENDING`,
sostituire il contenuto del time box e dei bottoni:

- Sostituire trash + label + value con: testo grande centrato "Hold
  tag near reader…" e un small icon `rfid_contactless.png` (già nei
  bitmap), pulsante a sx "Cancel" (re-usa `cancelClicked` per abortire
  il burn), nessun bottone a destra.
- L'abort lato PCB: introdurre `ApplicationRuntime_CancelEndSession()`
  che resetta i flag erase/scan e torna allo stato precedente
  (`RUNNING` o `PAUSED`). Comando seriale: nuovo `VACENDCANCEL`.

## Testing Strategy

Manuale. Scenari da coprire dopo build di PCB + Display + bridge:

1. **Layout visivo**: avvio Bandy → Pause → EndConfirm. *Atteso*: il
   nuovo layout corrisponde al mockup (pill rossa con triangolo,
   titolo, sottotitolo rosa, time box rossastro, bottoni asimmetrici
   bianco/trasparente-rosso).
2. **Continue button**: tap Continue (largo, bianco). *Atteso*: torna
   a Screen6 (Pause), nessun comando seriale alla PCB. Equivale al
   vecchio "No".
3. **End con tag in campo**: tap End con tag vicino all'antenna.
   *Atteso*: schermo passa a stato "Holding tag…", PCB scansiona il
   tag, scrive examNum=0, CRC aggiornata, ritorna TAG_EXPIRED. Beep
   conferma, display naviga a Screen2 (ProductSelect). Tag rimesso a
   un nuovo scan non autorizza più (rifiutato da `examNum == 0`).
4. **End senza tag in campo**: tap End senza tag. *Atteso*: schermo
   "Holding tag…" persiste. L'operatore può posare il tag → burn
   parte → conclusione come scenario 3. Oppure può premere Cancel →
   torna alla EndConfirm precedente, sessione resta attiva.
5. **Verifica tag bruciato**: dopo scenario 3, fai una nuova sessione
   con lo stesso tag. *Atteso*: il PCB rifiuta il tag (errore beep
   `bopTriggered`), display resta su Screen4 (RfidWait) senza
   AUTHORIZED.
6. **Regressione**: tutti gli altri schermi (1-6) invariati. Tutti i
   bottoni con i nuovi ID di BitmapDatabase compilano senza warning.

## Boundaries

- **Always do**:
  - Aggiungere i 3 PNG in `TouchGFX/assets/images/` e Generate Code
    da Designer **prima** di scrivere il codice (altrimenti
    BitmapDatabase non ha gli ID).
  - Build + flash dei due target prima di chiudere.
  - Riusare `endVisitValueBuffer` esistente per il display tempo
    (già wired via `T_VALUE_TIMEREMAINING`).
- **Ask first**:
  - Sostituire i font esistenti con nuovi (per ora usiamo quelli
    già in progetto).
  - Modificare lo schema di durata del tag (`examNum`,
    `durationMinutes`).
  - Introdurre nuovi comandi seriali oltre a `VACENDCANCEL` (e
    eventualmente `VACENDBURN` se l'analisi suggerisce di separare il
    flusso).
- **Never do**:
  - Editare `Screen7ViewBase.cpp` (file generato).
  - Editare manualmente `Display_test.touchgfx` in questo round
    (troppo rischio di corruzione su 12+ widget aggiunti).
  - Toccare il flusso vacuum/pump per implementare il burn — il burn
    è solo logica RFID.

## Success Criteria

- [ ] I 3 nuovi PNG sono in `assets/images/` e compaiono in
      `BitmapDatabase.hpp` come ID validi.
- [ ] Il nuovo layout di Screen7 rispecchia il mockup (5 scenari di
      test passano).
- [ ] Tap End con tag in campo → tag invalidato (`examNum=0`,
      `TAG_EXPIRED`) → display naviga a ProductSelect.
- [ ] Tap End senza tag → display mostra "Holding tag…" finché tag
      non arriva o user preme Cancel.
- [ ] Tap Cancel durante "Holding tag…" → ripristina lo stato
      precedente (RUNNING o PAUSED), nessun cambio sul tag.
- [ ] Il vecchio Screen7 layout è completamente coperto a runtime
      (nessun widget vecchio visibile).
- [ ] Build pulita di entrambi i progetti.

## Decisioni chiuse (post discussione)

- **Icone**: rinominate ai nomi `alert_triangle_white.png`,
  `trash_red.png`, `arrow_left_black.png`. Ricolorate via Pillow
  script `_recolor.py` (presente in `TouchGFX/assets/images/`).
- **Triangolo**: utente scarica la variante filled
  (`alert-triangle-filled`) da Tabler — il recolor script la usa
  automaticamente se trova `alert-triangle-filled.png` nella cartella
  immagini, altrimenti rimane l'outline ricolorata a bianco.
- **Pill**: bitmap dedicato `pill_red.png` (200×22, rosso #E24B4A,
  angoli arrotondati r=11). Si compone con `Image` widget (non Box).
- **Testi**: l'utente aggiunge in Designer (Text Editor) le nuove
  voci. Lista TextId esatti vedi sezione "Text IDs da aggiungere".
- **Font**: si riusano le typography già in progetto. Mapping
  tentativo: titolo → `T_VALUE_METRIC` style, sottotitolo →
  `T_TEXT_TIMELEFT` style, bottoni → `T_TEXT_YES` style. Verifica
  visiva durante Fase 2; se troppo grosso/piccolo, si aggiunge una
  nuova typography in Designer.

## Text IDs da aggiungere in Designer

Apri Designer → Text Editor → aggiungi 7 voci (Tab "Single Use" o
"Default"):

| TextId | Stringa (English) |
|---|---|
| `Text_PillIrreversible` | `IRREVERSIBLE ACTION` |
| `Text_EndSessionQuestion` | `End session?` |
| `Text_EndSessionSubtitle` | `The RFID tag will be burned\nand remaining time will be lost` |
| `Text_TimeYoullLose` | `TIME YOU'LL LOSE` |
| `Text_ButtonContinue` | `Continue` |
| `Text_ButtonEnd` | `End` |
| `Text_HoldTagNearReader` | `Hold tag near reader to end session` |

Dopo l'aggiunta, **Generate Code (Ctrl+G)** rigenera
`texts/TextKeysAndLanguages.hpp` con i symboli
`T_TEXT_PILLIRREVERSIBLE`, ecc., usabili nel codice.

## Implementation Phases

**Fase 1 — Asset & Generated Code**:
1. Scaricare i 3 PNG da Tabler Icons.
2. Copiarli in `TouchGFX/assets/images/`.
3. Designer → Generate Code → verifica `BitmapDatabase.hpp`.
4. Aggiungere in Designer le nuove entry Text (vedi Open Questions).

**Fase 2 — UI redesign Screen7 (display)**:
1. Aggiornare `Screen7View.hpp` con i nuovi membri widget.
2. Implementare `setupScreen()` con il nuovo layout (16 widget).
3. Implementare `applyBandyState()` per gestire i 4 stati incluso
   `ENDING`.
4. Build + flash.
5. Verifica visiva manuale (scenari 1, 2 del Testing).

**Fase 3 — Burn-on-end (PCB + display protocol)**:
1. PCB: nuovo stato `APPLICATION_BANDY_STATE_ENDING` in
   `application_runtime.h`.
2. PCB: modificare `EndVacuumSession()` per attivare i flag erase e
   transitare a `ENDING` senza reset immediato.
3. PCB: handler post-burn in `HandleRfidError`/`ProcessRfidScan` per
   completare la transizione su `TAG_EXPIRED`.
4. PCB: nuovo comando seriale `VACENDCANCEL` per abortire il burn.
5. Display: estendere `BandySessionState` con `BandySessionEnding`.
6. Display: gestire stato `ENDING` in `applyBandyState` (mostra
   overlay "Holding tag…").
7. Verifica end-to-end (scenari 3, 4, 5, 6).

Le fasi sono in dipendenza stretta: 1 → 2 → 3. Si può iniziare 2
appena 1 è completo e dimostrare visivamente il layout, poi
aggiungere 3 come iterazione successiva.
