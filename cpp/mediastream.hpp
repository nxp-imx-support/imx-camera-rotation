/*
 * Copyright 2024-2025 NXP
 * Copyright (C) 2016 The Qt Company Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QQuickItem>
#include <QQuickWindow>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include "mqueuethread.hpp"
#include "videodevice.hpp"

class MediaStream : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(int angle READ getAngle WRITE setAngle NOTIFY angleChanged)
    Q_PROPERTY(QString backend READ getBackend WRITE setBackend NOTIFY backendChanged)
    Q_PROPERTY(QString resolution READ getResolution WRITE setResolution NOTIFY resolutionChanged)
    Q_PROPERTY(QString source READ getSource WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QStringList devices READ getDevices NOTIFY devicesChanged)
    Q_PROPERTY(QStringList resolutions READ getResolutions NOTIFY resolutionsChanged)

    QML_ELEMENT

public:
    MediaStream();

public Q_SLOTS:
    int getAngle();
    void setAngle(int angle);

    QString getBackend();
    void setBackend(QString backend);

    QString getSource();
    void setSource(QString source);

    QString getResolution();
    void setResolution(QString res);

    QStringList getDevices();
    QStringList getResolutions();

    void play();
    void pause();
    void stop();

    // Used for increment or decrease m_angle
    void increase();
    void decrease();

Q_SIGNALS:
    void angleChanged();
    void backendChanged();
    void resolutionChanged();
    void sourceChanged();
    void devicesChanged();
    void resolutionsChanged();

private:
    void init();
    void cleanup();
    void findDevices();
    void releaseResources();
    bool isValidVideoDevice(const QString& devicePath) const;

    bool m_isInitialized;
    bool m_playing;
    
    int m_angle;
    int m_width;
    int m_height;

    QString m_device;
    QString m_backend;
    QString m_resolution;

    QStringList m_deviceList;
    QStringList m_resolutionList;
    QMap<QString, QString> m_deviceMap;
    QMap<QString, QString> m_uniqueDeviceMap;  // Maps device path to unique display name

    QProcess *m_process;
    MQueueThread *m_mqueue;
    VideoDevice *m_videoModel;
};