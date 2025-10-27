/*
 * Copyright 2025 NXP
 * Copyright (C) 2016 The Qt Company Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <QAbstractListModel>
#include <QDir>
#include <QMap>
#include <libudev.h>
#include <utility>
 
class VideoDevice : public QObject {
    Q_OBJECT

public:
    VideoDevice(QObject *parent = nullptr);
    ~VideoDevice();

    QMap<QString, QString> devices() const;
    QStringList deviceResolution(QString device);
 

private slots:
    void handleUdevEvent();

private:

    void enumerateDevices();

    struct udev *udev;
    struct udev_monitor *monitor;
    QMap <QString, QString> m_devices;
    QList<QString> m_deviceResolution;
};
