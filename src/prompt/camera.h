#pragma once
#include <QList>
#include <QString>
#include <QVariantMap>

// Professional camera settings the user can set for Marketing Prompts (body, lens, aperture, shutter, ISO, white
// balance, color look, flash, camera height, distance, optics). Unset = "auto": Opus chooses coherently. The choices
// travel to Opus as a block (`block`) that tells it to name the gear AND render the physical consequences.
namespace Camera {

struct Option { QString value, hint; };            // hint: what the choice does to the picture (read by Opus, not shown)
struct Setting { QString id, title; QList<Option> options; };
struct Preset { QString id, label; QVariantMap values; };

const QList<Setting> &settings();
const QList<Preset> &presets();

QString summary(const QVariantMap &values);        // "Sony A7R V · 85mm · f/1.8 · ..." (empty when nothing is set)
QString block(const QVariantMap &values);          // the section added to Opus's request (empty when nothing is set)

}
