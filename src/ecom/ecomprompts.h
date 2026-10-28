#pragma once
#include <QString>

namespace EcomPrompts {
extern const QString system;
QString preamble(bool layout, int photos, bool back, bool backFromPhoto);
}
