#pragma once
#include <QString>
#include <QStringList>

// Dropped files keep their original names (camera IDs, accented Spanish text). Claude reads the
// filename as text next to the image, so a loaded name can bias the description. Staging copies
// each file under a neutral "imageN" name and normalizes it for Claude's Read tool, which only
// understands jpg/png/gif/webp: HEIC, AVIF, TIFF, BMP are re-encoded as JPEG, EXIF rotation is
// baked in, and anything above 2560 px is downscaled. Blocking: call from worker threads.
namespace Staging {
void reset();
QStringList stage(const QStringList &originals);   // throws Err
}
