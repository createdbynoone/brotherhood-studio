# Brotherhood Studio: blueprint

Foto del proyecto para retomarlo o rehacerlo. El detalle técnico y las trampas ya resueltas viven en `CLAUDE.md` (leerlo primero); aquí está el mapa.
Estado al 2026-10-16: **v1.1.3 publicada** (`createdbynoone/brotherhood-studio`).

## Qué es
BMP (Brotherhood Marketing Prompts) y Sorter (clasificador de generaciones) en una sola app de macOS con dos paneles (Prompts, Split, Sorter). Qt 6 / C++20, interfaz QML, capa nativa Objective-C++. Reemplazó a BMP Native y Sorter Native (SwiftUI) y a las apps Electron.

## Flujo de BMP
Referencia + producto → Sonnet escribe el brief → Opus escribe el prompt (`prompts.cpp` generado, byte-idéntico al de Electron) → **Fire** a Higgsfield con el motor elegido → la imagen cae en la carpeta de salida como `bmp_<ms>.<ext>` → el Sorter la muestra al instante (`saved(path, modelo)`).

Motores (`providers()` en `promptcontroller.cpp`): Seedream 5.0 Pro · Nano Banana Pro · **GPT Sunburst** (= `gpt_image_2_5` con `variant=sunburst`; calidad low…max). El botón de lanzar muestra el costo de la tanda (`higgsfield generate cost`) y los créditos de la esquina se actualizan solos (poll de 15 s y tras cada job).

## Arquitectura en una pantalla
```
src/core/      JsonFile, Paths, Prefs (solo toca sus claves), Lock (PBKDF2-SHA512 200k + token en archivo)
src/platform/  mac.mm (ImageIO, AVFoundation, Papelera, ventana, arrastre de barra, pellizco), memtrim
src/prompt/    Shell, Higgsfield (CLI), ClaudeCli, MemoryStore, Staging, Downloader, PromptController, LogModel
src/triage/    Store, EntryItem (incluye `model`), GalleryModel (virtualizada), FolderWatcher, ThumbProvider, ModelDetect, TriageController
src/app/       StudioController (layout, clave, actualizaciones, escala de UI, reinicio)
qml/prompt     PromptPane, InputsColumn, OutputColumn, FireDock, ActivityLog (terminal)…
qml/triage     TriagePane, GridArea, MediaCard, FocusView, InspectorOverlay (cristal) + InspectorPanel
```

## Compatibilidad (no romper)
Mismos archivos que las apps anteriores: `~/Library/Application Support/BMP/{bmp-prefs,bmp-memory}.json`, `…/Sorter/{sorter-db,sorter-prefs}.json` + `library/`; las claves desconocidas sobreviven. Layout propio en `BrotherhoodStudioQt/studio-prefs.json` (incluye `uiScale`).

## Decisiones que no hay que deshacer
- **Sorter**: galería virtualizada calculada en C++; nada de animar `opacity` de subárboles con esquinas redondeadas (usar velos); un solo valor de progreso para paneles que se mueven.
- **Inspector flotante**: bloquea todo lo que cae sobre él con un `MouseArea` Y con `Theme.shield` (los clics sobre sus BOTONES atravesaban a las cards de atrás); el pellizco (global, AppKit) se ignora sobre él. Category y Product se aplican a toda la selección.
- **Modelo de la imagen**: se registra al generar; para las anteriores `ModelDetect` lee los metadatos C2PA del PNG/JPEG sin leer píxeles (Seedream y Nano Banana se reconocen; las reencodificadas quedan "Unknown"; 3:4 sin metadatos = "Seedream (est.)").
- **Arrastre**: el umbral del `DragHandler` de la card (6) debe ser menor que el del marquee del grid (12); URLs con `Theme.fileUrl` (encodeURI deja pasar `#` y `?`); el drag lleva toda la selección y la clave `application/x-brotherhood-card`.
- **Escala de UI**: `QT_SCALE_FACTOR` en `main()` (clave `uiScale`), reinicio con `StudioController::restart()`; el entorno heredado se limpia con el marcador `BS_SCALE_APPLIED`. Las coordenadas de AppKit se dividen por el factor.
- **Memoria**: `MemTrim` suelta lo reconstruible tras 45 s en segundo plano. Reposo ≈140 MB.
- **Lock**: PBKDF2-SHA512, token 0600 atado a la Mac (no Keychain). Clave rotada el 2026-10-15; solo sal + hash en el código.
- Exportar se eliminó a propósito.

## Cómo se prueba
- `--grab out.png [--layout prompt|split|triage] [--open settings|palette|focus|log] [--size WxH] [--delay ms]` (la ventana se renderiza sin tocar la pantalla; las capturas frenan las animaciones).
- Clics reales simulados con `QWindowSystemInterface::handleMouseEvent` (cabeceras privadas; se añadían temporalmente, no están en el árbol). **Siempre contra una COPIA de los datos** (`CFFIXED_USER_HOME=/tmp/fakehome` con los JSON copiados): una vez los clics simulados cambiaron estados y categorías de la base real del Sorter.
- Nada de `tools/uitest.swift` si el usuario trabaja en otras apps.

## Publicar
1. Subir `VERSION` en `CMakeLists.txt`. 2. `tools/release.sh` (~7 min, en background). 3. Validar el zip (firma, versión, hash nuevo, sin libs de Homebrew, sin `SKIP_LOCK`, sin secretos). 4. commit + tag + push + `gh release create`. **Solo con visto bueno del usuario.**

## Seguridad
Repo público sin claves de API, tokens ni referencias a MCP (repos, historial y zips escaneados). El hash del lock es público; el anterior sigue en el historial y esa clave está quemada.

## Pendiente / ideas
- El arrastre de cards se corrigió por análisis y no se probó con un arrastre real.
- Los botones de Status del inspector solo afectan a la imagen ancla (K/M/D del teclado sí usan toda la selección): decidir si deben afectar a todas.
- La detección de GPT por metadatos no tiene ejemplos reales todavía.
- Decidir si se borran los releases 1.1.1 y 1.1.2.
