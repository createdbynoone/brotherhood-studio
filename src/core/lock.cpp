#include "lock.h"

#include "../platform/mac.h"
#include "prefs.h"

#include <QDateTime>
#include <QtMath>

// Same salt/hash as the SwiftUI BMP build, so the same passphrase opens this app.
// Only the hash lives here, never the passphrase. Regenerate with:
//   openssl kdf -keylen 64 -kdfopt digest:SHA512 -kdfopt pass:'NEW' -kdfopt hexsalt:<salt> -kdfopt iter:200000 PBKDF2
namespace AppLock {

static const char *kSaltHex = "1b2b0cb131d133d28ff17a07cf8fc82a";
static const char *kHashHex = "4ccbf4703f3ede32ebd7f31a914ad62a156a6ae1d8bbfcfb4b14ec254ff2d48225ffbcdfefc0016f794e8ac4f5c4bb51d03cafa4c2442fb276f3f2ad6aabc3e0";
static const quint32 kIterations = 200000;
static const QString kService = QStringLiteral("com.brotherhood.studio.qt");
static const QString kAccount = QStringLiteral("unlock-token");

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

// HMAC-SHA256(machine UUID, key hash + service): bound to this Mac (copying the item elsewhere
// fails), bound to the key hash (changing the passphrase re-locks every Mac) and ThisDeviceOnly
// (never syncs). Any Keychain failure falls back to asking for the key, never to opening.
static QByteArray expectedToken() {
    return Mac::hmacSha256(Mac::machineId().toUtf8(), (QString(kHashHex) + kService).toUtf8());
}

bool isRemembered() {
    const QByteArray stored = Mac::keychainGet(kService, kAccount);
    return !stored.isEmpty() && constantTimeEqual(stored, expectedToken());
}

void remember() { Mac::keychainSet(kService, kAccount, expectedToken()); }
void forget() { Mac::keychainDelete(kService, kAccount); }

}  // namespace AppLock
