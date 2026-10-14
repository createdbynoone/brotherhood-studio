#include <QCommandLineParser>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>

#include "app/studiocontroller.h"
#include "platform/mac.h"
#include "prompt/promptcontroller.h"
#include "triage/thumbprovider.h"
#include "triage/triagecontroller.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName("Brotherhood Studio");
    QGuiApplication::setOrganizationName("Brotherhood");
    // BS_VERSION_OVERRIDE pretends to be an older build so the update flow can be tested against real releases
    QGuiApplication::setApplicationVersion(qEnvironmentVariable("BS_VERSION_OVERRIDE", QStringLiteral(APP_VERSION)));
    QQuickStyle::setStyle("Basic");
    Mac::setDarkAppearance();

    // --grab <png>   render the window to a PNG and quit (design review without screen access)
    QCommandLineParser cli;
    QCommandLineOption grab("grab", "Save a screenshot and quit.", "file");
    QCommandLineOption delay("delay", "Milliseconds to wait before the screenshot.", "ms", "1800");
    QCommandLineOption layout("layout", "prompt | split | triage", "mode");
    QCommandLineOption size("size", "Window size WxH.", "WxH", "1560x940");
    QCommandLineOption transition("transition", "Capture frames of a layout change: from:to:prefix", "spec");
    QCommandLineOption openOpt("open", "Open a dialog at startup (settings | palette), for design review.", "what");
    cli.addOptions({grab, delay, layout, size, transition, openOpt});
    cli.process(app);

    for (const char *f : {"Geist-Regular", "Geist-Medium", "Geist-SemiBold", "Geist-Bold", "GeistMono-Regular", "GeistMono-Medium",
                          "Phosphor", "Phosphor-Bold", "Phosphor-Fill"})
        QFontDatabase::addApplicationFont(QStringLiteral(":/assets/fonts/%1.ttf").arg(f));
    // Brand font (Sequel 100 Black 75): optional, falls back to Geist Bold in the wordmark
    QString brandFamily;
    const int brandId = QFontDatabase::addApplicationFont(QStringLiteral(":/assets/fonts/Sequel100Black-75.ttf"));
    if (brandId >= 0) brandFamily = QFontDatabase::applicationFontFamilies(brandId).value(0);
    QFont base("Geist", 13);
    QGuiApplication::setFont(base);

    StudioController studio;
    PromptController prompt;
    TriageController triage;
    QObject::connect(&prompt, &PromptController::saved, &triage, &TriageController::ingest);
    QObject::connect(&prompt, &PromptController::outputPathChanged, &triage, &TriageController::rescan);
    QObject::connect(&app, &QGuiApplication::aboutToQuit, &triage, &TriageController::flushNow);
    if (cli.isSet(layout)) studio.setLayout(cli.value(layout));

    QQmlApplicationEngine engine;
    engine.addImageProvider("thumb", new ThumbProvider(false));
    engine.addImageProvider("full", new ThumbProvider(true));
    engine.rootContext()->setContextProperty("studio", &studio);
    engine.rootContext()->setContextProperty("prompt", &prompt);
    engine.rootContext()->setContextProperty("triage", &triage);
    engine.rootContext()->setContextProperty("grabSize", cli.value(size));
    engine.rootContext()->setContextProperty("brandFamily", brandFamily);
    engine.rootContext()->setContextProperty("startOpen", cli.value(openOpt));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("Studio", "Main");

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().value(0));
    if (!window) return 1;
    QTimer::singleShot(0, [window] {
        Mac::styleWindow(window);
        // QML decides which points of the title bar are empty space (see Main.qml: titleBarHit)
        Mac::installTitleBarDrag(window, [window](double x, double y) {
            QVariant hit;
            QMetaObject::invokeMethod(window, "titleBarHit", Q_RETURN_ARG(QVariant, hit), Q_ARG(QVariant, x), Q_ARG(QVariant, y));
            return hit.toBool();
        });
    });

    // --transition split:triage:/tmp/frames   settle on `from`, switch to `to`, save frames while it animates
    if (cli.isSet(transition)) {
        const QStringList parts = cli.value(transition).split(':');
        if (parts.size() == 3) {
            const QString original = studio.layout(), from = parts[0], to = parts[1], prefix = parts[2];
            studio.setLayout(from);
            QTimer::singleShot(2200, &app, [=, &studio, &app] {
                studio.setLayout(to);
                static const int marks[] = {60, 300, 600, 900, 1250, 1700, 2400};
                for (int i = 0; i < 7; ++i)
                    QTimer::singleShot(marks[i], &app, [=] { window->grabWindow().save(QStringLiteral("%1_%2.png").arg(prefix).arg(i)); });
                QTimer::singleShot(3000, &app, [=, &studio, &app] { studio.setLayout(original); app.quit(); });
            });
        }
    }
    if (cli.isSet(grab)) {
        const QString out = cli.value(grab);
        QTimer::singleShot(cli.value(delay).toInt(), &app, [window, out, &app] {
            window->grabWindow().save(out);
            app.quit();
        });
    }
    return app.exec();
}
