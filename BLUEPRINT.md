# Brotherhood Studio: blueprint

Foto del proyecto para retomarlo o rehacerlo. El detalle técnico y las trampas ya resueltas viven en `CLAUDE.md` (leerlo primero); aquí está el mapa.
Estado al 2026-10-28: **v1.2.0 publicada** (`createdbynoone/brotherhood-studio`): Ecommerce, tarjeta Camera, Background/LOOKBOOK y Split como botón. v1.1.3 fue la anterior. El zip público NO incluye la biblioteca de estilo de Ecommerce (fotos de la marca).

## Qué es
BMP (Brotherhood Marketing Prompts), Ecommerce (fotos de catálogo con prompts JSON) y Sorter (clasificador de generaciones) en una sola app de macOS (Prompts, Ecommerce, Sorter; el botón naranja Split muestra Prompts + Sorter). Qt 6 / C++20, interfaz QML, capa nativa Objective-C++. Reemplazó a BMP Native y Sorter Native (SwiftUI) y a las apps Electron.

## Flujo de BMP
Referencia + producto → Sonnet escribe el brief → Opus escribe el prompt (`prompts.cpp`, mantenido a mano) → **Fire** a Higgsfield con el motor elegido → la imagen cae en la carpeta de salida como `bmp_<ms>.<ext>` → el Sorter la muestra al instante (`saved(path, modelo)`).

Motores (`providers()` en `promptcontroller.cpp`): Seedream 5.0 Pro · Nano Banana Pro · **GPT Sunburst** (= `gpt_image_2_5` con `variant=sunburst`; calidad low…max). El botón de lanzar muestra el costo de la tanda (`higgsfield generate cost`) y los créditos de la esquina se actualizan solos (poll de 15 s y tras cada job).

## Fondo y realismo (publicado en 1.2.0)
Prompts tiene una tarjeta **Background** opcional (recomendada): foto de la ubicación real que el motor recibe como última referencia. Sin ella, el prompt exige un lugar real y fotografiable con look de cine (bloque REALISM STANDARD). El bloque **LOOKBOOK** destila 17 referencias de streetwear real (garajes, pasos peatonales, mallas, flash, grano de película). Esas fotos están en `~/Library/Application Support/BrotherhoodStudioQt/look-library/` (no empaquetadas: son de terceros) y el botón "Library" del Background las ofrece; las de lugares vacíos llevan prefijo `plate-`. Detalle en `CLAUDE.md`.

## Cámara (publicada en 1.2.0)
Tarjeta **Camera** en Prompts: cámara, lente, apertura, obturador, ISO, balance de blancos, look de color/película, flash, altura, distancia y óptica, con 6 presets y Auto. Viaja a Opus como bloque `CAMERA SETTINGS` que exige la línea [CAMERA] exacta y las consecuencias físicas descritas (bokeh, grano, color de luz, flash...) para generaciones más realistas. Datos en `src/prompt/camera.cpp`; se guarda en `studio-prefs.json`. Pendiente: comparar generaciones reales con y sin ajustes.

## Ecommerce (publicado en 1.2.0)
**Solo GPT Sunburst y solo 1:1** (sin selector de motor ni de ratio). Fotos del producto (hasta 8: frente, espalda, ángulos y detalles; + referencia de layout opcional + notas) → Opus lee todas, detecta el tipo, mide la **silueta y cada costura** (`product.silhouette`, `product.seam_map`, `photo_views`) y la empareja con la **biblioteca de estilo de casa** (33 fotos de los drops 1 y 2: fondo blanco, maniquí invisible, encuadre fijo por categoría; el lienzo cuadrado es el 4:5 de la casa ensanchado con blanco, así el producto conserva su tamaño de catálogo) → JSON con layout y objetivos de retoque (sin arrugas, costuras perfectas, gráficos exactos) → disparo como edición. Casilla **"Shoot the back too"** (solo prendas, no gorras/gorros/bolsos): el JSON gana `back_spec` (medida de las fotos o inferida del frente) y cada variación dispara frente + espalda (`ecom_<ms>.jpg` y `ecom_<ms>_back.jpg`, costo x2). Las imágenes llegan al Sorter al instante. Salida en `<salida>/Ecommerce/` (con el JSON al lado). Las fotos de la biblioteca viven en `resources/ecom-style/` (fuera de git) y se copian a `~/Library/Application Support/BrotherhoodStudioQt/ecom-style/`, donde se añaden los drops nuevos. Pendiente: **probar un disparo real a Higgsfield** (solo se probó el análisis con la CLI y el compilado; ver si la trucker mejora silueta y costuras, y probar con y sin foto de espalda), historial de JSON.

## Arquitectura en una pantalla
```
src/core/      JsonFile, Paths, Prefs (solo toca sus claves), Lock (PBKDF2-SHA512 200k + token en archivo)
src/platform/  mac.mm (ImageIO, AVFoundation, Papelera, ventana, arrastre de barra, pellizco), memtrim
src/prompt/    Shell, Higgsfield (CLI), ClaudeCli, MemoryStore, Staging, Downloader, PromptController, LogModel, Camera
src/ecom/      EcomController, ecomprompts (JSON de catálogo), ecomstyle (biblioteca de estilo de casa)
src/triage/    Store, EntryItem (incluye `model`), GalleryModel (virtualizada), FolderWatcher, ThumbProvider, ModelDetect, TriageController
src/app/       StudioController (layout, clave, actualizaciones, escala de UI, reinicio)
qml/prompt     PromptPane, InputsColumn, OutputColumn, FireDock, ActivityLog (terminal)…
qml/ecom       EcomPane, EcomInputs, EcomOutput
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
- **Split es un botón aparte** (naranja, a la izquierda del selector de modo), no una opción del selector.
- **Ecommerce es un panel que CUBRE** el área de Prompts/Sorter (velo del color de fondo, nada de animar opacidad de los paneles); debajo conservan su layout. Split sigue siendo Prompts + Sorter.
- Las fotos de terceros (look-library) no se empaquetan ni se suben; las de la marca (ecom-style) se empaquetan en local pero no se commitean (repo público).
- Los prompts (`prompts.cpp`, `ecomprompts.cpp`) se escriben en positivo: los motores toman las negaciones literalmente. Ambos exigen que silueta, paneles, costuras y filas de puntada salgan de las fotos del producto, no de la versión típica de la prenda.
- El JSON de Ecommerce se manipula como TEXTO (`cutKey`), nunca con QJson, para no perder el orden de las claves.

## Cómo se prueba
- `--grab out.png [--layout prompt|ecom|split|triage] [--open settings|palette|focus|log] [--size WxH] [--delay ms]` (la ventana se renderiza sin tocar la pantalla; las capturas frenan las animaciones).
- Clics reales simulados con `QWindowSystemInterface::handleMouseEvent` (cabeceras privadas; se añadían temporalmente, no están en el árbol). **Siempre contra una COPIA de los datos** (`CFFIXED_USER_HOME=/tmp/fakehome` con los JSON copiados): una vez los clics simulados cambiaron estados y categorías de la base real del Sorter.
- Para capturas de Ecommerce/Prompts: `--layout ecom --open ecom` con una carpeta de usuario falsa que tenga `Library/Application Support/BrotherhoodStudioQt/unlock.token` copiado y `.config` enlazado al real (sin él la app abre el login de Higgsfield en el navegador).
- Nada de `tools/uitest.swift` si el usuario trabaja en otras apps.

## Publicar
1. Subir `VERSION` en `CMakeLists.txt`. 2. `tools/release.sh` (~7 min, en background; apaga `BS_BUNDLE_ECOM_STYLE`; si la ruta lleva `&`, `BS_BUILD_DIR` apuntando a otra carpeta). 3. Validar el zip (firma, versión, hash nuevo, sin libs de Homebrew, sin `SKIP_LOCK`, sin secretos). 4. commit + tag + push + `gh release create`. **Solo con visto bueno del usuario.**

## Seguridad
Repo público sin claves de API, tokens ni referencias a MCP (repos, historial y zips escaneados). El hash del lock es público; el anterior sigue en el historial y esa clave está quemada.

## Pendiente / ideas
- El arrastre de cards se corrigió por análisis y no se probó con un arrastre real.
- Los botones de Status del inspector solo afectan a la imagen ancla (K/M/D del teclado sí usan toda la selección): decidir si deben afectar a todas.
- La detección de GPT por metadatos no tiene ejemplos reales todavía.
- Decidir si se borran los releases 1.1.1 y 1.1.2.
