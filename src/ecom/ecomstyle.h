#pragma once
#include <QString>

// The house-style library: the finished catalog photos of past drops (pure white background, invisible mannequin,
// fixed framing per product type). They are bundled in the app, copied once to Application Support, and the user
// adds new drops there. Every photo is measured (where the product sits in the canvas) so Opus can match a new
// product to the closest existing shot instead of guessing numbers.
//   <dir>/<category>/<name>__<front|back>.jpg      category: tee, hoodie, pants, shorts, headwear, ...
namespace EcomStyle {

QString dir();
/// Copies the bundled photos that are not in the folder yet (never overwrites or deletes what the user changed).
void seed();
int count();
/// One line per photo for the model: absolute path, category, view and the measured frame (fractions of the canvas).
QString index();
/// Absolute path of a library photo given its path relative to the library (or absolute); empty when it is not inside the library.
QString resolve(const QString &relativeOrAbsolute);

}
