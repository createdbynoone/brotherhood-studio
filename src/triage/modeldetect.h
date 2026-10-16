#pragma once
#include <QString>

// Which generator made an image, read from the metadata the generators embed (C2PA content credentials in a PNG
// caBX chunk or a JPEG APP11 segment). Only metadata blocks are read, never the pixel data, so it is cheap.
// Empty when the file carries nothing recognisable (re-saved, compressed or imported images lose it).
namespace ModelDetect {
QString fromMetadata(const QString &path);
}
