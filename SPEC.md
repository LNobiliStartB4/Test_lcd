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
