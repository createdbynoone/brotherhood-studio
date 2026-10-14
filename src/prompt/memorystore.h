#pragma once
#include <QList>
#include <QString>

struct MemoryEntry {
    QString id;
    double timestamp = 0;       // ms since epoch (shared format with the other builds)
    QString description;
    QString prompt;
    bool fired = false;
    QString aspectRatio;
};

// Prompt memory: every generated prompt is stored; fired ones are the strongest signal
// and get injected back into the system prompt as calibration examples.
// Thread-safe (workers add entries, the UI reads history).
namespace MemoryStore {
QList<MemoryEntry> entries();                // newest first
void stats(int *total, int *fired);
MemoryEntry add(const QString &description, const QString &prompt);
void markFired(const QString &id, const QString &aspectRatio);
QString context();
}
