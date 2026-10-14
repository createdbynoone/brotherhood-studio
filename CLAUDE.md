# Brotherhood Studio (Qt 6 / C++)

BMP (Brotherhood Marketing Prompts) y Sorter (triage de generaciones) en una sola app de macOS con dos paneles. Lógica en C++20, interfaz en QML (Qt Quick, controles propios), capa nativa de Apple en Objective-C++. Reemplaza a BMP Native y Sorter Native (SwiftUI) y a las apps Electron.

**Build:** `brew install qt cmake ninja` · `cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=$(brew --prefix qt) && cmake --build build` → `build/Brotherhood Studio.app`.
**Release:** `tools/release.sh` (build Release, `macdeployqt`, firma ad-hoc, zip `Brotherhood-Studio-X.Y.Z-arm64-mac.zip`).
**Fuente de marca:** `resources/fonts/Sequel100Black-75.ttf` NO está en git (licencia). Copiarla desde `Brotherhood/Branding/Brand Resources/TIPOGRAFÍAS/`. Sin ella el wordmark cae a Geist Bold.
**Ícono:** `resources/AppIcon.icon` (Icon Composer) se compila con `actool` en cada build (Assets.car + .icns).

## Estructura
```
src/core/      JsonFile (escritura atómica), Paths, Prefs (solo toca sus claves), Lock (PBKDF2 + Keychain)
src/platform/  mac.mm: Keychain, CommonCrypto, ImageIO, AVFoundation, Papelera, ventana, arrastre de la barra
src/prompt/    BMP: Shell, Higgsfield, ClaudeCli, MemoryStore, Staging, Downloader, PromptController, LogModel
src/triage/    Sorter: Store, EntryItem, GalleryModel, FolderWatcher, ThumbProvider, TriageController
src/app/       StudioController (layout, clave, updates, helpers de SO)
qml/components Tokens (Theme), tipografía (Txt), Btn, Segment, Field, Wordmark, BrandMark, …
qml/prompt     PromptPane, InputsColumn, OutputColumn, FireDock, History…
qml/triage     TriagePane, GridArea (galería), MediaCard, FocusView, InspectorOverlay…
tools/         gen_prompts.py, gen_icons.py, uitest.swift, release.sh
```

## Compatibilidad (no romper)
- Mismos archivos que las apps anteriores: `~/Library/Application Support/BMP/{bmp-prefs,bmp-memory}.json`, `.../Sorter/{sorter-db,sorter-prefs}.json` + `library/`. `Prefs` y `Store` conservan las claves que no conocen. Layout propio en `BrotherhoodStudioQt/studio-prefs.json`.
- `src/prompt/prompts.cpp` se GENERA con `tools/gen_prompts.py` desde `Prompts.swift`; `system` debe ser byte-idéntico al SYSTEM_PROMPT de Electron.
- Lock: misma passphrase (PBKDF2-SHA512 200k). Keychain service `com.brotherhood.studio.qt`: la clave se pide una vez por Mac.
- Resultados de BMP: `bmp_<ms>.<ext>` en `outputPath`; el Sorter vigila ese patrón. `PromptController::saved` → `TriageController::ingest` los muestra al instante.
- NB 2.1 siempre lleva `thinking_level=high` (fijo en `Provider`).

## Diseño
- Una paleta cálida-oscura, UN acento naranja `#F54F1B` (texto sobre él: `#0B0B0A`, AA). Radios: control 8, card 12, panel 16. Tres tonos de texto, todos AA.
- Tipografía Geist / Geist Mono; wordmark `BROTHERHOOD®` en Sequel 100 Black 75 (mayúsculas). Íconos Phosphor (fuente), nunca SVG a mano. Sin guiones largos en textos visibles.
- `Theme.reduceMotion` sigue "Reducir movimiento" de macOS; toda duración pasa por `Theme.ms()`.

## Reglas aprendidas (rendimiento y render)
- **Nada de animaciones infinitas**: `Pulse.qml` voltea la opacidad con un Timer lento (≈3 fps), no con una animación continua. Reposo medido: ≈0 % CPU.
- **No animar `opacity` de subárboles con el `Shape` de esquinas redondeadas**: Qt oscurece TODA la ventana. Usar un velo del color de fondo (`GridArea`, `StudioView`).
- **No usar Behavior que persiga un valor que cambia cada frame** (ancho de panel): provoca "persecución". Usar un único valor de progreso (`InspectorOverlay.prog`) o congelar el layout mientras el panel se mueve (`GridArea.freeze`).
- Galería: `GalleryModel` calcula x/y/w/h en C++ y expone solo las tarjetas cercanas a la vista (virtualizada); los nodos persisten entre re-layouts y QML anima x/y/w/h **solo** mientras `reflowing` (≈700 ms tras cambio de columnas / filtro / orden).
- Cada pane vive en una caja con `clip` y nunca baja de su ancho mínimo (efecto cortina, sin deformarse).
- Tipos QML de `Basic` (Button, Menu…) chocan con nombres propios: el botón propio se llama `Btn`. `Statuses` y `Theme`/`Icons` deben marcarse singleton en `CMakeLists.txt`.
- `font.pixelSize` solo admite enteros.
- Arrastre de la ventana: monitor local de AppKit (`Mac::installTitleBarDrag`), QML decide en `titleBarHit` qué es espacio vacío. El arrastre de Qt no sirve (llega tarde).

## Pruebas de interfaz
- `--grab out.png [--delay ms] [--layout prompt|split|triage] [--size WxH]` captura la ventana y sale. `--transition from:to:/ruta/prefijo` guarda fotogramas de un cambio de layout.
- `tools/uitest.swift` simula clics, arrastres y teclas sobre la app (solo actúa si Brotherhood Studio está al frente). Probar con una copia de `sorter-db.json`.
- Las capturas de `--grab` frenan la animación: la fluidez se juzga a ojo.

## Actualizaciones
Al abrir, `StudioController::checkForUpdate` consulta `releases/latest` de `createdbynoone/brotherhood-studio`. Si hay versión nueva, la barra superior ofrece "Install": descarga el zip del release, y un script reemplaza el .app y lo reabre.
