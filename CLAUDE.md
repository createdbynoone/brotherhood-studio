# Brotherhood Studio (Qt 6 / C++)

BMP (Brotherhood Marketing Prompts) y Sorter (triage de generaciones) en una sola app de macOS con dos paneles. Lógica en C++20, interfaz en QML (Qt Quick, controles propios), capa nativa de Apple en Objective-C++. Reemplaza a BMP Native y Sorter Native (SwiftUI) y a las apps Electron.

**Build:** `brew install qt cmake ninja` · `cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=$(brew --prefix qt) && cmake --build build` → `build/Brotherhood Studio.app`.
**Release:** `tools/release.sh` (build Release, `macdeployqt`, firma ad-hoc, zip `Brotherhood-Studio-X.Y.Z-arm64-mac.zip`).
**Fuente de marca:** `resources/fonts/Sequel100Black-75.ttf` NO está en git (licencia). Copiarla desde `Brotherhood/Branding/Brand Resources/TIPOGRAFÍAS/`. Sin ella el wordmark cae a Geist Bold.
**Ícono:** `resources/AppIcon.icon` (Icon Composer, hecho a medida con apariencias clara, oscura y tintable) se compila con `actool` en cada build (Assets.car + .icns). La carpeta DEBE llamarse `AppIcon.icon` (actool busca ese nombre). No aplicar desplazamientos propios: el centrado ya viene en el diseño.

## Estructura
```
src/core/      JsonFile (escritura atómica), Paths, Prefs (solo toca sus claves), Lock (PBKDF2 + token en archivo)
src/platform/  mac.mm: CommonCrypto, ImageIO, AVFoundation, Papelera, ventana, arrastre de la barra
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
- Lock: misma passphrase (PBKDF2-SHA512 200k). El token de desbloqueo es un archivo 0600 (`BrotherhoodStudioQt/unlock.token`, HMAC atado a la Mac y al hash de la clave): la clave se pide una vez por Mac. NO usar Keychain: su ACL queda atada a la firma de cada release (ad-hoc) y macOS pide la contraseña tras cada actualización.
- Resultados de BMP: `bmp_<ms>.<ext>` en `outputPath`; el Sorter vigila ese patrón. `PromptController::saved` → `TriageController::ingest` los muestra al instante.
- NB 2.1 siempre lleva `thinking_level=high` (fijo en `Provider`).

## Diseño
- Una paleta cálida-oscura, UN acento naranja `#F54F1B` (texto sobre él: `#0B0B0A`, AA). Radios: control 8, card 12, panel 16. Tres tonos de texto, todos AA.
- Títulos (paneles, tarjetas, secciones, selector de modo, botones principales): `Txt` variante `heading`/`overline` = fuente de marca (Sequel), MAYÚSCULAS, sin tracking; fallback Geist Bold. Nombres visibles: "Marketing Prompts" y "Generation Sorter" (sin subtítulos). Ancho mínimo de ventana 1360 (selector de modo + logo + controles).
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

GitHub responde 403 a peticiones sin `User-Agent` (Qt no lo envía solo): `StudioController` lo pone en el chequeo y en la descarga. `BS_VERSION_OVERRIDE=1.0.0` hace que la app se crea una versión anterior para probar el flujo contra releases reales.
Prueba local del actualizador sin publicar nada: servir un `latest.json` (formato de la API de releases: `tag_name`, `html_url`, `assets[].name/browser_download_url`) con `python3 -m http.server` y arrancar con `BS_UPDATE_API=http://localhost:8765/latest.json`. La app reemplaza su propio .app por el zip, quita cuarentena y se reabre.
Cada release está firmado ad-hoc: macOS puede volver a pedir el permiso de Documentos tras actualizar. 

## Publicar una versión
1. Subir `VERSION` en `CMakeLists.txt`. 2. `tools/release.sh` (≈6 a 9 min). 3. `git commit` + `git tag -a vX.Y.Z` + push. 4. `gh release create vX.Y.Z dist/Brotherhood-Studio-X.Y.Z-arm64-mac.zip -R createdbynoone/brotherhood-studio`. La app instalada la ofrece al abrir.

## Focus: zoom y pan (v1.0.8)
- Rueda de ratón y ⌘/Ctrl+scroll hacen zoom anclado al cursor (`zoomTo` en `FocusView.qml`, la capa escala desde el centro del stage); el scroll del trackpad (`pixelDelta`) solo hace pan; pellizco = zoom; doble click alterna 1× / 2.5×. El pan se limita (`setPan`) y el arrastre solo lo hace el ratón.
- macOS mezcla eventos de magnificación diminutos en un scroll de dos dedos: antes eso cambiaba el zoom al mover. El pellizco se ignora 220 ms tras cualquier rueda y exige |escala−1| ≥ 0.04 para activarse.
- Arrastrar una card muestra su miniatura (`Drag.imageSource`) en lugar del URL como texto. `--open focus` abre el primer item en Focus (para capturas con `--grab`).
- Márgenes: grid con `kPad = 20`, botón del inspector a 16 pt del borde.
- (post 1.0.8, sin publicar) El pellizco ya no usa `PinchHandler`: `Mac::installMagnify` (NSEvent local monitor) emite `studio.magnified(x, y, mag, phase)` y se traga el evento; `FocusView` solo lo acepta como gesto deliberado (zona muerta 0.04, no durante scroll, no 250 ms tras un click). El zoom solo se resetea al cambiar de imagen (por `path`) o con doble click; un click simple nunca lo toca.
