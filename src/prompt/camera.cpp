#include "camera.h"

namespace Camera {

const QList<Setting> &settings() {
    static const QList<Setting> s = {
        {"body", "Camera", {
            {"Sony A7R V", "high-resolution mirrorless: crisp, neutral, modern digital rendering"},
            {"Canon EOS R5", "clean modern digital with warm, natural skin and fabric tones"},
            {"Leica M11", "rangefinder rendering: understated contrast, rich micro-detail, gentle highlight roll-off"},
            {"Hasselblad X2D 100C", "medium format: extreme detail, smooth tonal transitions, shallower depth of field than the aperture suggests"},
            {"Contax T2", "35mm film compact with a Zeiss lens: soft vignette, saturated film color, honest grain"},
            {"Canon AE-1", "35mm film SLR: classic grain, soft highlight roll-off, slightly warm cast"},
            {"Pentax 67", "medium format film: large-negative detail, creamy depth falloff, fine grain"},
            {"Disposable flash camera", "plastic lens: soft corners, heavy vignette, harsh direct flash, slightly off color, visible grain"},
            {"iPhone Pro", "smartphone: deep depth of field, computational HDR, crisp sharpening, wide natural perspective"},
            {"ARRI Alexa 35", "cinema camera: organic highlight roll-off, filmic color, wide dynamic range, cinematic texture"},
        }},
        {"lens", "Lens", {
            {"16mm ultra-wide", "strong perspective stretch, converging verticals, edge distortion, near objects exaggerated"},
            {"24mm wide", "wide environmental view, mild perspective stretch, the place is part of the picture"},
            {"35mm", "classic street field of view: natural perspective with context around the subject"},
            {"50mm", "natural eye-like perspective, no distortion"},
            {"85mm", "portrait telephoto: flattering compression, clear subject isolation"},
            {"135mm", "compressed perspective, stacked background layers, strong subject isolation"},
            {"200mm", "strong telephoto compression, flattened depth, heavy background melt"},
            {"Anamorphic 40mm", "cinema anamorphic: oval bokeh, horizontal flares, subtle edge falloff, widescreen feel"},
        }},
        {"aperture", "Aperture", {
            {"f/1.2", "razor-thin plane of focus, very large creamy bokeh, soft glow wide open"},
            {"f/1.4", "very shallow depth of field, large soft bokeh, background fully melted"},
            {"f/1.8", "shallow depth of field, soft background separation"},
            {"f/2.8", "moderate depth of field: subject sharp, background clearly soft but readable"},
            {"f/4", "subject sharp with a gently soft background, readable surroundings"},
            {"f/5.6", "deeper focus: subject and nearby surroundings sharp"},
            {"f/8", "deep focus: subject and background both sharp, maximum lens sharpness"},
            {"f/11", "very deep focus, everything sharp from foreground to background"},
            {"f/16", "everything sharp front to back, slight diffraction softness, sun stars on bright points"},
        }},
        {"shutter", "Shutter", {
            {"1/30", "slow handheld shutter: hands, fabric and passers-by carry soft motion blur, handheld softness"},
            {"1/60", "slight motion blur on fast movement, otherwise sharp"},
            {"1/125", "everything sharp at normal handheld speed"},
            {"1/250", "sharp, a walking subject frozen"},
            {"1/500", "frozen action, crisp edges"},
            {"1/1000", "fast action frozen, very crisp edges"},
            {"1/2000", "extremely crisp, any motion frozen"},
        }},
        {"iso", "ISO", {
            {"100", "clean and smooth, no visible noise"},
            {"200", "clean, minimal noise"},
            {"400", "fine grain, barely visible"},
            {"800", "visible fine grain in the shadows"},
            {"1600", "clear grain, slight color noise in the shadows"},
            {"3200", "heavy grain, lifted blacks, low-light look"},
            {"6400", "very grainy, gritty night-time character"},
        }},
        {"wb", "White balance", {
            {"3200K", "balanced for tungsten: warm interior light looks neutral, daylight turns blue"},
            {"4000K", "balanced for fluorescent tubes: cool-green garage light looks neutral, daylight shifts slightly warm"},
            {"5000K", "balanced for mixed light: neutral, slightly warm daylight"},
            {"5600K", "balanced for daylight: neutral whites and true colors"},
            {"6500K", "balanced for overcast sky: whites stay neutral, warm light shifts golden"},
            {"7500K", "balanced for open shade: a warm golden cast over everything"},
        }},
        {"grade", "Color look", {
            {"Natural digital", "faithful neutral color, true-to-life tones, no stylized grade"},
            {"Portra 400", "Kodak Portra 400: soft pastel color, gentle contrast, warm skin, fine grain"},
            {"Gold 200", "Kodak Gold 200: warm yellow cast, nostalgic saturated reds and yellows, visible grain"},
            {"Ektar 100", "Kodak Ektar 100: vivid saturated color, punchy contrast, very fine grain"},
            {"Fuji 400H", "Fujifilm Pro 400H: cool pastel greens, teal shadows, airy soft highlights"},
            {"CineStill 800T", "tungsten cine stock: cyan-teal shadows, red halation glow around bright lights, grain"},
            {"Vision3 500T", "cinema negative: rich shadows, warm tungsten highlights, filmic grain"},
            {"Tri-X 400 B&W", "high-contrast black and white, gritty grain, deep blacks"},
        }},
        {"flash", "Flash", {
            {"No flash", "available light only"},
            {"Direct flash", "on-camera direct flash: hard frontal light, flat shine on the fabric, sharp shadow edge behind the subject, fast falloff into a darker background, specular highlights on skin, zippers and prints"},
            {"Fill flash", "subtle flash opening the shadows under strong daylight, soft catchlights"},
            {"Ring flash", "even frontal light with a ring-shaped catchlight and a thin halo shadow around the subject"},
        }},
        {"angle", "Camera height", {
            {"Ground level", "camera on the floor looking up: monumental and heroic, ground texture in the foreground"},
            {"Low angle", "knee to waist height looking up slightly: imposing garment, sky or ceiling visible"},
            {"Eye level", "neutral eye height: documentary, straight verticals"},
            {"High angle", "above eye level looking down on the subject"},
            {"Overhead", "straight top-down view"},
        }},
        {"distance", "Distance", {
            {"Detail", "about half a meter away: fabric, stitching and print in close-up, extremely shallow depth"},
            {"Close", "about 1.5 meters away: half to three-quarter body"},
            {"Medium", "about 3 meters away: full body with surroundings"},
            {"Wide", "6 meters or more: a small figure in a large space, the environment dominates"},
        }},
        {"optics", "Optics", {
            {"Clean", "modern lens: very sharp, minimal vignette, high micro-contrast"},
            {"Natural", "slight vignette, natural micro-contrast, gentle softness at the edges"},
            {"Vintage", "vintage glass: softer corners, glow in the highlights, flare, light vignette, slight chromatic fringing"},
        }},
    };
    return s;
}

const QList<Preset> &presets() {
    static const QList<Preset> p = {
        {"street-film", "Street film", {{"body", "Contax T2"}, {"lens", "35mm"}, {"aperture", "f/2.8"}, {"shutter", "1/250"}, {"iso", "400"}, {"wb", "5600K"},
                                        {"grade", "Portra 400"}, {"flash", "Direct flash"}, {"angle", "Eye level"}, {"distance", "Medium"}, {"optics", "Vintage"}}},
        {"editorial", "Editorial", {{"body", "Sony A7R V"}, {"lens", "85mm"}, {"aperture", "f/1.8"}, {"shutter", "1/500"}, {"iso", "200"}, {"wb", "5600K"},
                                    {"grade", "Natural digital"}, {"flash", "No flash"}, {"angle", "Eye level"}, {"distance", "Close"}, {"optics", "Clean"}}},
        {"flash-night", "Flash night", {{"body", "Disposable flash camera"}, {"lens", "35mm"}, {"aperture", "f/8"}, {"shutter", "1/125"}, {"iso", "800"}, {"wb", "5000K"},
                                        {"grade", "Gold 200"}, {"flash", "Direct flash"}, {"angle", "Eye level"}, {"distance", "Close"}, {"optics", "Vintage"}}},
        {"cinema", "Cinema", {{"body", "ARRI Alexa 35"}, {"lens", "Anamorphic 40mm"}, {"aperture", "f/2.8"}, {"shutter", "1/60"}, {"iso", "800"}, {"wb", "3200K"},
                              {"grade", "Vision3 500T"}, {"flash", "No flash"}, {"angle", "Low angle"}, {"distance", "Medium"}, {"optics", "Vintage"}}},
        {"medium-format", "Medium format", {{"body", "Hasselblad X2D 100C"}, {"lens", "85mm"}, {"aperture", "f/4"}, {"shutter", "1/250"}, {"iso", "100"}, {"wb", "5600K"},
                                            {"grade", "Natural digital"}, {"flash", "No flash"}, {"angle", "Eye level"}, {"distance", "Close"}, {"optics", "Clean"}}},
        {"phone", "Phone candid", {{"body", "iPhone Pro"}, {"lens", "24mm wide"}, {"aperture", "f/1.8"}, {"shutter", "1/125"}, {"iso", "200"}, {"wb", "5600K"},
                                    {"grade", "Natural digital"}, {"flash", "No flash"}, {"angle", "Eye level"}, {"distance", "Close"}, {"optics", "Natural"}}},
    };
    return p;
}

QString summary(const QVariantMap &v) {
    QStringList parts;
    for (const char *id : {"body", "lens", "aperture", "shutter", "iso", "wb", "grade", "flash"}) {
        const QString s = v.value(QLatin1String(id)).toString();
        if (!s.isEmpty()) parts << s;
    }
    return parts.join(QStringLiteral(" · "));
}

QString block(const QVariantMap &v) {
    bool any = false;
    for (const auto &s : settings()) if (!v.value(s.id).toString().isEmpty()) any = true;
    if (!any) return {};

    QStringList lines;
    for (const auto &s : settings()) {
        const QString value = v.value(s.id).toString();
        if (value.isEmpty()) { lines << QStringLiteral("- %1: AUTO (choose what fits the scene and stays coherent with the rest)").arg(s.title); continue; }
        QString hint;
        for (const auto &o : s.options) if (o.value == value) hint = o.hint;
        lines << QStringLiteral("- %1: %2%3").arg(s.title, value, hint.isEmpty() ? QString() : QStringLiteral(" (%1)").arg(hint));
    }
    return QStringLiteral(
        "## CAMERA SETTINGS (chosen by the user: they outrank the reference's camera feel, angle and lens, and your own defaults):\n%1\n\n"
        "CAMERA PROTOCOL: the picture must look like a real photograph taken with exactly this equipment and these settings.\n"
        "- Write the [CAMERA] section as the real shooting line: the body, the focal length and the aperture, the shutter speed, the ISO, the white balance and the film stock or color look, naming every chosen value exactly\n"
        "- Then render the PHYSICAL CONSEQUENCES of the settings in plain visual prose inside the other sections: depth of field and the character of the background blur (from aperture, focal length and distance together), motion or frozen action (shutter), grain or noise (ISO or film), the color of the light against the white balance (a balance that differs from the scene light gives a deliberate color cast), film color rendition and halation, the look of flash light and where its shadows fall, the optical character (vignette, flare, edge softness), and the perspective that the camera height and focal length produce\n"
        "- Image engines respond to pictures of an effect, not to numbers: state each setting AND describe what it looks like in this scene\n"
        "- Keep every consequence coherent with the others and physically possible. When the choices conflict (for example ISO 100 at f/16 and 1/2000 at dusk), keep the user's lens, aperture, film and look, and let the scene's light level adapt\n"
        "- Items marked AUTO are yours to choose, in harmony with the chosen ones and the scene\n"
        "- Realism: render the natural character of real optics and sensors or film (micro-contrast, slight vignette, honest grain, real highlight roll-off), with true skin texture, natural edge sharpness and a photographic finish\n\n").arg(lines.join('\n'));
}

}
