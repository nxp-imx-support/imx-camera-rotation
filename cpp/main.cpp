/*
 * Copyright 2024-2025 NXP
 * Copyright (C) 2022 The Qt Company Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QtQuick/QQuickView>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    // Application
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // Load QML
    const QUrl url(QStringLiteral("qrc:/qml/main"));
    QObject::connect(
            &engine, &QQmlApplicationEngine::objectCreated, &app,
            [url](QObject *obj, const QUrl &objUrl) {
                if (!obj && url == objUrl)
                    QCoreApplication::exit(-1);
            },
            Qt::QueuedConnection);
    engine.load(url);

    // QML window
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QQuickWindow *window = qobject_cast<QQuickWindow *>(engine.rootObjects().at(0));

    return app.exec();
}
