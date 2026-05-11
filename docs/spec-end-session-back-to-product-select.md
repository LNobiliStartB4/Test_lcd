# Spec: End-Session Confirm → ProductSelect (v2 — explicit RFID scan handshake)

## Objective

Quando l'utente preme "Sì" (confirm) sulla schermata di conferma fine
sessione (Screen7 / EndConfirm), l'app deve tornare alla schermata di
selezione prodotto **Bandy / Hemorflow** (Screen2 / ProductSelect),
senza che il PCB rilegga e auto-approvi un tag RFID ancora presente nel
campo (niente bip parassita).

### Storia del problema

La v1 dello spec cambiava una sola riga su `Screen7View::applyBandyState`
per navigare su `BandySessionWaitRfid` verso ProductSelect. Test sul
campo ha mostrato che la transizione `WAIT_RFID` non arriva mai al
display: dopo `VACEND` il PCB riarma immediatamente lo scan RFID con il
tag ancora in zona, `HandleApprovedTag()` lo riautorizza in pochi ms,
suona il bip e lo stato salta a `AUTHORIZED` saltando del tutto la
finestra `WAIT_RFID`. Screen7 non gestisce `AUTHORIZED` → resta su
EndConfirm.

### Soluzione (approccio A)

Rendere esplicito il **handshake di scansione RFID**: il PCB scansiona
solo quando il display lo dice. Conseguenza diretta: dopo `VACEND` il
PCB non riarma più lo scan automaticamente, lo riarmerà solo quando il
display entra nella schermata RfidWait (Screen4) e invierà
`CMD,SCAN1`.

> Nota architetturale: questo handshake è coerente con il prodotto
> finale (display sulla stessa PCB) dove i comandi seriali diventeranno
> chiamate dirette di funzione. Il bridge USB è solo per la demo.

## Tech Stack

- **Firmware PCB**: STM32 + HAL, C, build STM32CubeIDE.
- **Display**: TouchGFX 4.26 su STM32F401RE, C++, build STM32CubeIDE.
- **Bridge demo**: Python 3 + pyserial (PC).

## Commands

- **Build PCB**: STM32CubeIDE → progetto `RFIDtagReader_LEDdimmer` →
  Build (Debug).
- **Build Display**: STM32CubeIDE → progetto `Display_test_prova` →
  Build (Debug).
- **Run bridge**: `python display_bridge.py --pcb COMx --display COMy`.

## Project Structure (file toccati)

```
RFIDtagReader_LEDdimmer/Core/Src/application_runtime.c   ← PCB: no auto restart RFID dopo VACEND
RFIDtagReader_LEDdimmer/Tools/display_bridge/display_bridge.py  ← whitelist SCAN1/SCAN0
Display_test_prova/Core/Inc/display_bridge_rx.h           ← API SendRfidScan{Start,Stop}Command
Display_test_prova/Core/Src/display_bridge_rx.c           ← impl. CMD,SCAN1 / CMD,SCAN0
Display_test_prova/TouchGFX/gui/include/gui/model/Model.hpp   ← startRfidScan / stopRfidScan
Display_test_prova/TouchGFX/gui/src/model/Model.cpp           ← impl.
Display_test_prova/TouchGFX/gui/src/screen4_screen/Screen4Presenter.cpp  ← activate/deactivate
Display_test_prova/TouchGFX/gui/src/screen7_screen/Screen7View.cpp       ← già modificato in v1
```

## Code Style

Mantenere lo stile esistente di ciascun progetto. Esempi reali del
cambio chiave:

**PCB `application_runtime.c`**:
```c
void ApplicationRuntime_EndVacuumSession(void)
{
  ApplicationFeedback_SetWarningBuzzerActive(false);
  ApplicationRuntime_StopRfidScan();
  ApplicationRuntime_ResetBandySession(false);   // era: true
}
```

**Display `Screen4Presenter.cpp`**:
```cpp
void Screen4Presenter::activate()
{
    if (model != 0)
    {
        model->initializeBandyDemo();
        model->startRfidScan();
    }
}

void Screen4Presenter::deactivate()
{
    if (model != 0)
    {
        model->stopRfidScan();
    }
}
```

## Testing Strategy

Tutto manuale (no unit-test framework in progetto). Scenari:

1. **Path corretto end-session con tag in campo**: Bandy run con tag
   ancora vicino all'antenna → Pause → EndConfirm → "Sì". *Atteso*:
   il display torna a ProductSelect, **nessun bip**, il PCB resta in
   `WAIT_RFID` (B=0) senza ulteriori transizioni.
2. **Riavvio sessione dopo end**: dal ProductSelect appena raggiunto,
   tap "Bandy" → Screen4 (RfidWait) → il display invia `CMD,SCAN1`
   → PCB scansiona il tag (ancora presente) → bip approvazione +
   `AUTHORIZED` → display passa a Bandy main.
3. **Regressione Screen3 timer scaduto**: avvia Bandy con durata
   breve (es. 1 min se possibile), lascia scadere senza usare end-
   session. *Atteso*: display naviga a RfidWait come oggi (Screen3
   continua a chiamare `gotoRfidWaitScreenNoTransition`). Il
   `ResetBandySession(true)` interno alla `ProcessBandySession` è
   invariato (riarma lo scan come prima) — questo path non viene
   modificato.
4. **Cancel su EndConfirm**: EndConfirm → tap "No" → torna a Pause
   (Screen6) come oggi. PCB resta nello stato corrente. Nessun nuovo
   comando inviato.
5. **Buzzer regression**: se accidentalmente il buzzer di warning si
   attivasse dopo VACEND, è bug. *Atteso*:
   `ApplicationFeedback_SetWarningBuzzerActive(false)` continua a
   spegnerlo.

## Boundaries

- **Always do**:
  - Build di entrambi i progetti (PCB + display) prima di chiudere il
    task.
  - Verificare che il PCB esponga `B=0` (WAIT_RFID) stabilmente dopo
    `VACEND` finché il display non manda `CMD,SCAN1`.
- **Ask first**:
  - Toccare il path di timer-expiration (`ProcessBandySession` quando
    `bandyRemainingSeconds == 0`). Per ora resta invariato.
  - Rimuovere `ApplicationRuntime_StartRfidScan()` da `App_init()`
    (avvio del PCB). Resta invariato.
- **Never do**:
  - Modificare i file `.ioc` di nessuno dei due progetti.
  - Introdurre logiche di "ignora stesso UID" o hold-off su PCB
    (approccio B scartato).
  - Modificare la logica di scan attivo durante `RUNNING` o `PAUSED`
    (il PCB già disattiva lo scan in `StartVacuumCycle`).

## Success Criteria

- [ ] Dopo "Sì" su EndConfirm il display mostra Screen2 (ProductSelect)
      entro 1 frame dopo aver ricevuto il frame `VAC?` con `B=0`.
- [ ] **Nessun bip** di approvazione tag tra il "Sì" e ProductSelect.
- [ ] Dopo `VACEND` il PCB risponde a `VAC?` con `B=0` finché non
      riceve `CMD,SCAN1`.
- [ ] Su `CMD,SCAN1` (inviato dall'entrata di Screen4), il PCB riavvia
      lo scan e, se il tag è in campo, genera bip + transita a
      `AUTHORIZED` come da flusso normale.
- [ ] Su `CMD,SCAN0` (uscita di Screen4) il PCB ferma lo scan.
- [ ] Screen3 (timer scaduto) e Screen6 (pausa che riprende) restano
      invariati.
- [ ] Build di entrambi i progetti pulita.

## Open Questions

- In futuro andrebbe valutato se anche il path di timer-expiration
  debba seguire l'handshake esplicito (oggi continua a fare
  `ResetBandySession(true)` che riarma lo scan). Per ora non blocca la
  demo.
- Quando "Hemorflow" sarà implementata, l'handshake esplicito le
  permetterà di iniziare con il tag corretto senza essere intercettata
  da una autorizzazione Bandy residua.
