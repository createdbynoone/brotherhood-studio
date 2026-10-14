#include "shell.h"

#include "../core/err.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>

namespace Shell {

QStringList searchPath() {
    static const QStringList dirs = [] {
        const QString home = QDir::homePath();
        QStringList d = {home + "/.local/bin",  // claude CLI
                         "/usr/local/bin", "/opt/homebrew/bin", "/opt/homebrew/sbin", "/usr/bin", "/bin"};
        const QString inherited = qEnvironmentVariable("PATH");
        if (!inherited.isEmpty()) d += inherited.split(':', Qt::SkipEmptyParts);
        return d;
    }();
    return dirs;
}

QProcessEnvironment environment() {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PATH", searchPath().join(':'));
    // A leftover ANTHROPIC_API_KEY would make the claude CLI bill per-token
    // instead of using the logged-in subscription session.
    env.remove("ANTHROPIC_API_KEY");
    return env;
}

QString resolve(const QString &bin) {
    auto executable = [](const QString &p) { const QFileInfo fi(p); return fi.isFile() && fi.isExecutable(); };
    if (bin.startsWith('/')) return executable(bin) ? bin : QString();
    for (const QString &dir : searchPath()) {
        const QString candidate = dir + "/" + bin;
        if (executable(candidate)) return candidate;
    }
    return {};
}

Result run(const QString &bin, const QStringList &args, int timeoutMs) {
    const QString exe = resolve(bin);
    if (exe.isEmpty())
        throw Err(QStringLiteral("`%1` no está instalado o no está en el PATH (%2)").arg(bin, searchPath().join(':')));

    QProcess p;
    p.setProcessEnvironment(environment());
    p.setProgram(exe);
    p.setArguments(args);
    p.setStandardInputFile(QProcess::nullDevice());
    p.start();
    if (!p.waitForStarted(10000)) throw Err(QStringLiteral("No se pudo iniciar %1").arg(bin));

    // waitForFinished keeps draining both pipes while it waits, so large outputs can't deadlock.
    if (!p.waitForFinished(timeoutMs)) {
        p.kill();
        p.waitForFinished(2000);
        throw Err(QStringLiteral("%1 excedió el tiempo de espera").arg(bin));
    }
    Result r{QString::fromUtf8(p.readAllStandardOutput()), QString::fromUtf8(p.readAllStandardError()), p.exitCode()};
    if (p.exitStatus() != QProcess::NormalExit || r.status != 0) {
        const QString msg = r.err.trimmed();
        throw Err(msg.isEmpty() ? QStringLiteral("%1 salió con código %2").arg(bin).arg(r.status) : msg);
    }
    return r;
}

}  // namespace Shell
