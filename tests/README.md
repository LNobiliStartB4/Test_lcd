# Unit tests — host build for `Model.cpp`

Test della logica pura di `TouchGFX/gui/src/model/Model.cpp` (state machine
Bandy, clamping target, dispatch comandi al bridge). Niente HAL,
niente TouchGFX framework: `display_bridge_rx.h` è sostituito da uno
stub che registra le chiamate e accetta snapshot iniettati.

## Prerequisiti

- **Developer PowerShell for VS 2022** (fornisce `cl.exe`, `cmake`,
  `ninja`)
- Accesso internet alla **prima esecuzione** (CMake FetchContent scarica
  doctest v2.4.11)

## Run (Developer PowerShell)

Dalla cartella `tests\`:

    cd C:\TouchGFXProjects\Display_test_prova\tests
    cmake -B build -G Ninja
    cmake --build build
    ctest --test-dir build --output-on-failure

In alternativa con generator MSBuild se non hai Ninja installato:

    cmake -B build -G "Visual Studio 17 2022"
    cmake --build build --config Debug
    ctest --test-dir build -C Debug --output-on-failure

## Output atteso

```
Test project ...\tests\build
    Start 1: model_unit_tests
1/1 Test #1: model_unit_tests .................   Passed    0.05 sec

100% tests passed, 0 tests failed out of 1
```

## Eseguire singoli test

Il binario `test_model.exe` (in `build\` o `build\Debug\`) accetta i
flag standard di doctest:

    .\build\test_model.exe --list-test-cases
    .\build\test_model.exe -tc="Defaults*"
    .\build\test_model.exe -ts="*"

## Struttura

```
tests/
├── CMakeLists.txt              ← build host con doctest
├── README.md                   ← questo file
├── test_model.cpp              ← test cases (7)
└── stubs/
    ├── display_bridge_rx.h     ← stub header (no HAL)
    └── display_bridge_rx_stub.cpp  ← stub impl + TestStub_* helpers
```

## Cosa è coperto

- Defaults di `BandyState` dopo `initializeBandyDemo`
- Clamping di `targetVacuumMbar` su MIN (290) e MAX (490)
- Mapping comandi pubblici → invocazioni `DisplayBridgeRx_Send*`
  (Start/Pause/Resume/End/EndCancel, Rfid Scan Start/Stop)
- `canOpenBandyScreen()` true solo per AUTHORIZED o RUNNING
- `canOpenPauseScreen()` true solo per PAUSED
- Mapping numerico `snapshot.bandyState=4` → `BandySessionEnding`

## Cosa NON è coperto

- `Screen7View.cpp` / Presenter / FrontendApplication: dipendono dal
  framework TouchGFX (Container, Drawable, ecc.) che non è banale da
  stubbare
- `display_bridge_rx.c` reale: usa HAL UART
- PCB firmware (`RFIDtagReader_LEDdimmer`): test separati non
  inclusi qui

## Tip — esegui da subprocess CMake

Se vuoi un comando unico, da PowerShell:

    cmake -B build -G Ninja `
      ; cmake --build build `
      ; ctest --test-dir build --output-on-failure

(occhio alle backticks PowerShell per il line continuation).
