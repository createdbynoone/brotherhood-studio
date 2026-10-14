#pragma once
#include <QString>

// Same on-disk locations the Electron and SwiftUI builds use, so history, categories and
// the library carry over between all of them.
namespace Paths {
QString appSupport();            // ~/Library/Application Support
QString bmpDir();                // .../BMP
QString bmpPrefs();
QString bmpMemory();
QString higgsfieldCredentials(); // ~/.config/higgsfield/credentials.json
QString defaultOutput();         // ~/Desktop
QString sorterDir();             // .../Sorter
QString sorterDb();
QString sorterPrefs();
QString sorterLibrary();
QString sorterThumbs();          // own cache, not shared with the other builds
QString studioDir();             // .../BrotherhoodStudioQt
QString studioPrefs();
}
