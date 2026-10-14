#include "lock.h"

#include "../platform/mac.h"
#include "paths.h"
#include "prefs.h"

#include <QDateTime>
#include <QFile>
#include <QSaveFile>
#include <QtMath>

// Same salt/hash as the SwiftUI BMP build, so the same passphrase opens this app.
// Only the hash lives here, never the passphrase. Regenerate with:
//   openssl kdf -keylen 64 -kdfopt digest:SHA512 -kdfopt pass:'NEW' -kdfopt hexsalt:<salt> -kdfopt iter:200000 PBKDF2
namespace AppLock {

static const char *kSaltHex = "1b2b0cb131d133d28ff17a07cf8fc82a";
static const char *kHashHex = "4ccbf4703f3ede32ebd7f31a914ad62a156a6ae1d8bbfcfb4b14ec254ff2d48225ffbcdfefc0016f794e8ac4f5c4bb51d03cafa4c2442fb276f3f2ad6aabc3e0";
static const quint32 kIterations = 200000;
static const QString kService = QStringLiteral("com.brotherhood.studio.qt");

static bool constantTimeEqual(const QByteArray &a, const QByteArray &b) {
    if (a.size() != b.size()) return false;
    quint8 diff = 0;
    for (int i = 0; i < a.size(); ++i) diff |= quint8(a[i]) ^ quint8(b[i]);
    return diff == 0;
}

bool verify(const QString &attempt) {
    QByteArray derived;
    if (!Mac::pbkdf2Sha512(attempt.toUtf8(), QByteArray::fromHex(kSaltHex), kIterations, 64, &derived)) return false;
    return constantTimeEqual(derived, QByteArray::fromHex(kHashHex));
}

double lockUntil() { return Prefs::authLockUntil(); }

double registerFailure() {
    const int count = Prefs::authFailCount() + 1;
    const double now = QDateTime::currentMSecsSinceEpoch();
    const double until = count >= 3 ? now + qMin(5000.0 * qPow(2, count - 3), 5 * 60 * 1000.0) : 0;
    Prefs::setAuth(count, until);
    return until;
}

void clearFailures() {
    Prefs::setAuth(0, 0);
    remember();
}

// "Unlocked once per Mac": a small token file in the app's support folder, HMAC-SHA256(machine UUID, key hash +
// service). Bound to this Mac (a copy elsewhere fails) and to the key hash (changing the passphrase re-locks every
// Mac). It deliberately does NOT use the Keychain: an item there is tied to the signing identity of the app that
// created it, and every ad-hoc signed release has a new one, so macOS asked for the login password again after each
// update. A 0600 file survives updates and never prompts. Any read problem falls back to asking for the key.
static QByteArray expectedToken() {
    return Mac::hmacSha256(Mac::machineId().toUtf8(), (QString(kHashHex) + kService).toUtf8());
}

static QString tokenPath() { return Paths::studioDir() + "/unlock.token"; }

bool isRemembered() {
    QFile f(tokenPath());
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QByteArray stored = QByteArray::fromHex(f.readAll().trimmed());
    return !stored.isEmpty() && constantTimeEqual(stored, expectedToken());
}

void remember() {
    QSaveFile f(tokenPath());
    f.setDirectWriteFallback(false);
    if (!f.open(QIODevice::WriteOnly)) return;
    f.write(expectedToken().toHex());
    if (f.commit()) QFile::setPermissions(tokenPath(), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

void forget() { QFile::remove(tokenPath()); }

}  // namespace AppLock
