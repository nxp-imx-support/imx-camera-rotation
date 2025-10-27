/*
 * Copyright 2025 NXP
 * Copyright (C) 2016 The Qt Company Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "videodevice.hpp"
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <QDir>
#include <QString>
#include <iostream>

namespace {
    QStringList commonResolutions = {
        "1024x768",
        "1280x720",
        "1920x1080",
        "2304x1296",
        "3840x2160",
        "4096x2160"
    };
}

VideoDevice::VideoDevice(QObject *parent)
    : QObject(parent)
{
    udev = udev_new();
    if (!udev) {
        qFatal("Failed to create udev");
    }
    monitor = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(monitor, "video4linux", nullptr);
    udev_monitor_enable_receiving(monitor);

    int fd = udev_monitor_get_fd(monitor);
    enumerateDevices();
}

VideoDevice::~VideoDevice()
{
    udev_monitor_unref(monitor);
    udev_unref(udev);
}

QMap<QString, QString> VideoDevice::devices() const
{
    return m_devices;
}
 
void VideoDevice::handleUdevEvent() {
    struct udev_device *dev = udev_monitor_receive_device(monitor);
    if (dev) {
        QString action = udev_device_get_action(dev);
        QString devNode = udev_device_get_devnode(dev);
        QString name = udev_device_get_property_value(dev, "ID_V4L_PRODUCT");

        qDebug() << "Action:" << action << "Device:" << devNode << "Name:" << name;
            udev_device_unref(dev);
        }
}

void VideoDevice::enumerateDevices() {
    struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "video4linux");
    udev_enumerate_scan_devices(enumerate);
    QStringList cameras;
    int cam = 1;

    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;

    qDebug() << "[VIDEODEVICE] Device enumeration started..."; 
    udev_list_entry_foreach(entry, devices) {
        const char *path = udev_list_entry_get_name(entry);
        struct udev_device *dev = udev_device_new_from_syspath(udev, path);

        QString devNode = udev_device_get_devnode(dev);
        QString name = udev_device_get_property_value(dev, "ID_V4L_PRODUCT");
        QString cap = udev_device_get_property_value(dev, "ID_V4L_CAPABILITIES");
        QString serial = udev_device_get_property_value(dev, "ID_SERIAL_SHORT");
        devNode.remove(QChar('"'));
        name.remove(QChar('"'));
        cap.remove(QChar('"'));
        cap.remove(QChar(':'));
        serial.remove(QChar(':'));

        if(!m_devices.contains(devNode) && cap == "capture"){
            if(!cameras.contains(name)){
                m_devices.insert(devNode,name);
                qDebug() << "   devNode: " << devNode << " name: " << name;
            }
            else{
                name = name + "-" + QString::number(cam);
                qDebug() << "devNode: " << devNode << " name: " << name;
                m_devices.insert(devNode,name);
                cam++;
            }
        }

        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
}

QStringList VideoDevice::deviceResolution(QString device)
{
    QStringList resolutions;
    try {
        qDebug() << "   Getting device resolution for: " << device;
        const char *dev = device.toStdString().c_str();
        int fd = open(dev, O_RDWR);
        if (fd < 0) {
            throw std::runtime_error("Error opening device: " + device.toStdString());
        }

        // Get current device format
        struct v4l2_format fmt = {};
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
            close(fd);
            throw std::runtime_error("VIDIOC_G_FMT failed for device: " + device.toStdString());
        }

        m_deviceResolution.clear();
        struct v4l2_frmsizeenum frmsize = {};
        frmsize.pixel_format = fmt.fmt.pix.pixelformat;

        for (frmsize.index = 0; ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0; frmsize.index++) {
            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
                if(frmsize.discrete.width < 640 || frmsize.discrete.height < 480)
                    continue;
                QString currentRes = QString::number(frmsize.discrete.width) + "x" + QString::number(frmsize.discrete.height);
                m_deviceResolution.append(currentRes);
            } else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE) {
                qDebug() << "   Stepwise: " << frmsize.stepwise.min_width << "x" << frmsize.stepwise.min_height
                        << " to " << frmsize.stepwise.max_width << "x" << frmsize.stepwise.max_height
                        << " with step " << frmsize.stepwise.step_width << "x" << frmsize.stepwise.step_height;
                m_deviceResolution = commonResolutions;
                break;
            }
        }

        close(fd);
        resolutions = m_deviceResolution;
    } catch (std::exception &e) {
        qDebug() << "Exception getting device resolution";
    }

    return resolutions;
}