# Brotherhood Studio (Qt 6 / C++)

> Mapa general del proyecto (estado, decisiones, pruebas, publicar): `BLUEPRINT.md`. Este archivo guarda el detalle y las trampas.

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
- Motores (`providers()` en `promptcontroller.cpp`): Seedream 5.0 Pro, Nano Banana Pro y **GPT Sunburst** (= `gpt_image_2_5` con `variant=sunburst`; lleva `quality` low/medium/high/xhigh/max, por defecto high). NB 2.1 ya no existe. Un `Provider` declara `qualities` y `fixed` (parámetros que siempre se envían).

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

## Focus: zoom, pan y pellizco (v1.0.9)
- Rueda de ratón y ⌘/Ctrl+scroll hacen zoom anclado al cursor (`zoomTo` en `FocusView.qml`; la capa escala desde el centro del stage); el scroll del trackpad (`pixelDelta`) solo hace pan; arrastre con ratón = pan (`DragHandler` con `acceptedDevices: Mouse`, solo con zoom > 1; NO referenciar ids borrados en `enabled`: un ReferenceError lo desactiva en silencio). Pan acotado (`setPan`). Zoom 1× a 10×.
- **Pellizco**: sin `PinchHandler`; `Mac::installMagnify` emite `studio.magnified(...)` (ver Canvas CLAUDE.md para el porqué). Zona muerta 0.04, ignorado 200 ms tras una rueda y 250 ms tras un press/release (`clickGuard`).
- **El zoom solo se resetea** al cambiar de imagen (compara `path`, no el objeto: `focusEntry` se re-anuncia al cambiar estado/nota) o con doble click (zoom>1 → 1×; desde 1× → 2.5×). Un click simple nunca lo toca.
- **Semáforo**: `installTitleBarDrag` deja pasar los clics sobre los botones estándar de la ventana.
- Arrastrar una card muestra su miniatura (`Drag.imageSource`) en vez del URL como texto. Márgenes: grid `kPad = 20`, botón del inspector a 16 pt del borde.
- `--open focus` abre el primer item en Focus (capturas con `--grab --delay 9000`: las imágenes grandes tardan en decodificar).

## Cambios 1.1.x (publicados en 1.1.0 a 1.1.3)
- **Exportar eliminado** (diálogo, menú, botón de Focus, atajo ⌘E, `exportEntry`, `composeCover`).
- **Costo en el botón de lanzar**: `Higgsfield::cost` (`higgsfield generate cost`, sin crear job) por imagen × variaciones, con debounce de 300 ms al cambiar motor/ratio/resolución/calidad. **Créditos en vivo**: `refreshCredits` cada 15 s con la app al frente, al volver a primer plano, 3 s después de enviar un job y al terminar cualquiera (cobro al crear, reembolso al fallar).
- **Arrastre**: el `DragHandler` de la card (umbral 6) debe cruzar su umbral ANTES que el del marquee del grid (12); con el marquee en 4 ganaba casi siempre y la card no se levantaba. URLs con `Theme.fileUrl` (`encodeURI` deja pasar `#` y `?`). El drag lleva toda la selección y la clave `application/x-brotherhood-card` para que el grid no lo tome por una importación. NO verificado con un arrastre real (no hay forma de simularlo sin tocar la pantalla).
- **Activity** dibujado como terminal (`ActivityLog.qml`): verde fósforo sobre casi negro, mono, prefijo por tipo (`$ ✓ ✗ ! >`), cursor de bloque con Timer lento. `--open log` carga líneas de muestra.
- **Escala de interfaz** (Ajustes > Interface, 100 a 50 %): `QT_SCALE_FACTOR` se fija en `main()` antes de crear la app leyendo `uiScale` de `studio-prefs.json`; cambiarla pide reiniciar (`StudioController::restart`). Las coordenadas que llegan de AppKit en puntos (pellizco, arrastre de la barra) se dividen por el factor. Ventana mínima 900x560; la barra superior y el filtro del Sorter se compactan al estrechar.
- Menú contextual: "Use as reference/product in Prompt" (antes "BMP").
- **Memoria**: `MemTrim` (`src/platform/memtrim.*`): con la app 45 s en segundo plano suelta lo reconstruible (copias decodificadas de imágenes, caché de componentes QML, GC, texturas sin uso, páginas libres del heap); en uso solo devuelve páginas libres cada 3 min. `BS_TRIM_NOW=1` lo dispara a los 8 s (pruebas). Reposo medido: ≈140 MB (huella física); lo grande son los thumbnails vivos (≈16 MB), ColorSync (≈7 MB) y QML; no hay fugas visibles con `heap`.
- **Clave de acceso rotada el 2026-10-15** (clave aleatoria de 20 caracteres, ~99 bits): solo cambian `kSaltHex`/`kHashHex` en `src/core/lock.cpp`. El hash anterior sigue en el historial público de git, así que la clave vieja debe darse por quemada. Cambiar el hash invalida los tokens de "ya desbloqueado", y cada Mac pide la clave una vez.
- **Escala: el reinicio hereda el entorno** (`open` lo pasa): `main()` descarta `QT_SCALE_FACTOR` si viene con el marcador `*_SCALE_APPLIED` de la ejecución anterior; sin eso la escala vieja ganaba y no se podía volver a 100 %. Probado midiendo el ancho real de la ventana (100/80/50/100).
- **Paneles flotantes no filtran eventos**: el panel del inspector lleva un `MouseArea` (todos los botones, hover y rueda) bajo su contenido, y el botón redondo también; sin él las cards o la imagen de Focus recibían los clics, arrastres y la rueda. El pellizco (AppKit, global) se ignora sobre el inspector (`FocusView.overInspector`). NO verificado con un clic real (no hay driver en Studio).
- **Inspector: escudo `Theme.shield`**: los clics sobre los BOTONES del inspector (items con TapHandler) además llegaban a las cards de atrás aunque el panel llevara un MouseArea; ahora cards, encabezados de grupo y la imagen de Focus preguntan `Theme.shielded(x, y)` y ignoran todo lo que cae dentro del panel (clic, doble clic, clic derecho, arrastre, rueda, hover). Reproducido con eventos reales vía `QWindowSystemInterface` (ver historial del chat; el sondeo era temporal). CUIDADO al probar: los clics simulados modifican la base real del Sorter, usar una copia.
- **Inspector con varias imágenes**: Category y Product se aplican a TODA la selección (`TriageController::toggleCategories`: si todas ya la tienen se quita de todas, si no se pone en todas; el chip se ve activo solo si todas la tienen). Los botones de Status del inspector siguen aplicándose solo a la imagen ancla (K/M/D del teclado sí usan toda la selección).
- **Modelo en el inspector** (casilla "Model"): `EntryItem.model` se guarda al generar (`PromptController::saved(path, label)` → `ingest`); para imágenes anteriores `ModelDetect` lee los metadatos C2PA del PNG/JPEG sin tocar los píxeles (Seedream: "seedream"; Nano Banana Pro: Google/Gemini/SynthID; GPT: OpenAI, sin ejemplos reales aún); si no hay nada y el ratio es 3:4 se muestra "Seedream 5.0 Pro (est.)", y si no, "Unknown". De 88 imágenes: 21 NB Pro, 14 Seedream, 53 sin metadatos.
- Pruebas con clics reales: `QWindowSystemInterface::handleMouseEvent` (cabeceras privadas de QtGui) y datos en una COPIA con `CFFIXED_USER_HOME=/tmp/fakehome`; nunca contra la base real.
- **Activity**: un `ListView` resetea la `x` de su delegate a 0, así que el margen lateral va en un `Row` interior (antes las horas quedaban pegadas al borde en el diseño de una columna). El botón "Toggle inspector" de la cabecera del Sorter se quitó (queda la flecha redonda, la tecla I y la paleta).
