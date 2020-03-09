#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "simrunner.h"
#include "provider.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    QSharedPointer<SimRunner> simRunner(new SimRunner());
    engine.addImageProvider(QLatin1String("images"), new Provider(simRunner));
    engine.rootContext()->setContextProperty("sim", simRunner.get());

    engine.load(url);

    return app.exec();
}
