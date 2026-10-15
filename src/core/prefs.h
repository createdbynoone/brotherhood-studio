#pragma once
#include <QString>

// Each accessor reads/writes only its own key and keeps every other key in the file untouched,
// so the Electron and SwiftUI builds never lose settings this app doesn't know about.
namespace Prefs {

// bmp-prefs.json
QString outputPath();                      // falls back to ~/Desktop
void setOutputPath(const QString &path);
int authFailCount();
double authLockUntil();                    // ms since epoch
void setAuth(int failCount, double lockUntil);

// sorter-prefs.json
double sorterGridSize(double fallback);
bool sorterInspectorOpen(bool fallback);
void setSorterGridSize(double v);
void setSorterInspectorOpen(bool v);

// studio-prefs.json (this app only)
QString studioLayout();                    // "prompt" | "split" | "triage"
double studioSplitFraction();
void setStudioLayout(const QString &layout);
void setStudioSplitFraction(double v);
/// Interface scale in percent (100, 90, 80, 70, 60, 50). Read before QGuiApplication exists: it sets QT_SCALE_FACTOR.
int studioUiScale();
void setStudioUiScale(int percent);

}
