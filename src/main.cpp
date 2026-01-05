#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

// Declare the type registration function
extern void qml_register_types_QMeshCore();

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    app.setApplicationName(QStringLiteral("QMeshCore"));
    app.setOrganizationName(QStringLiteral("QMeshCore"));
    app.setOrganizationDomain(QStringLiteral("meshcore.dev"));

    // Register QML types
    qml_register_types_QMeshCore();

    // Use Material style for modern look
    QQuickStyle::setStyle(QStringLiteral("Material"));

    QQmlApplicationEngine engine;

    // Connect to handle object creation failure
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("QMeshCore", "Main");

    return app.exec();
}
