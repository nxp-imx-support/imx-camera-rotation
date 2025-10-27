/*
 * Copyright 2024-2025 NXP
 * Copyright (C) 2016 The Qt Company Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mediastream.hpp"
#include <QtMath>
#include <QRunnable>
#include <QDebug>

#define DEMOPATH "/opt/gopoint-apps/scripts/multimedia/imx-camera-rotation/demos"
#define DEMOG2D "imx-camera-rotation-g2d"
#define DEMOOPENCV "imx-camera-rotation-opencv"
#define DEMOOPENGL "imx-camera-rotation-opengl"

MediaStream::MediaStream()
    : 
      m_isInitialized(false),
      m_playing(false),
      m_angle(0),
      m_width(0),
      m_height(0),
      m_device(""),
      m_backend(""),
      m_resolution(""),
      m_videoModel(nullptr)
{

    init();

    m_mqueue = new MQueueThread();
    m_mqueue->start(QThread::HighPriority);

    m_process = new QProcess();
}
void MediaStream::cleanup()
{
    qDebug() << "[MEDIASTREAM] Cleaning up resources...";
    
    // Stop any ongoing processes
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
        }
    }
    
    // Clear containers
    m_deviceList.clear();
    m_deviceMap.clear();
    m_uniqueDeviceMap.clear();  // Clear the new map
    m_resolutionList.clear();
    
    // Safe VideoDevice cleanup
    if (m_videoModel) {
        try {
            delete m_videoModel;
        } catch (...) {
            qWarning() << "[MEDIASTREAM] Exception during VideoDevice cleanup";
        }
        m_videoModel = nullptr;
    }
    
    // Reset state
    m_device.clear();
    m_resolution.clear();
    m_playing = false;
    m_isInitialized = false;
    
    qDebug() << "[MEDIASTREAM] Cleanup completed";
}
int MediaStream::getAngle()
{
    return m_angle;
}

void MediaStream::setAngle(int angle)
{
    if(angle >= 0 && angle <= 359) {
        m_angle = angle;
        Q_EMIT angleChanged();
    }

}

QString MediaStream::getBackend()
{
    return m_backend;
}

void MediaStream::setBackend(QString backend)
{
    m_backend = backend;
    qDebug() << "[MediaStream] Backend set: " << m_backend;
}

QStringList MediaStream::getDevices()
{
    return m_deviceList;
}

QStringList MediaStream::getResolutions()
{
    return m_resolutionList;
}

QString MediaStream::getSource()
{
    return m_device;
}

void MediaStream::setSource(QString source)
{
    qDebug() << "   Setting source: " << source;
    if(!source.isEmpty()) {
        // Find the device path for the given display name
        QString devicePath;
        
        // Search in our unique device map
        for (auto it = m_uniqueDeviceMap.constBegin(); it != m_uniqueDeviceMap.constEnd(); ++it) {
            if (it.value() == source) {
                devicePath = it.key();
                break;
            }
        }
        
        // Fallback: search in original device map if not found
        if (devicePath.isEmpty()) {
            devicePath = m_deviceMap.key(source);
        }
        
        if (devicePath.isEmpty()) {
            qWarning() << "[MEDIASTREAM] Could not find device path for source:" << source;
            return;
        }
        
        m_device = devicePath;
        qDebug() << "   Device node: " << m_device;
        emit sourceChanged();

        if (m_videoModel) {
            m_resolutionList.clear();
            try {
                m_resolutionList = m_videoModel->deviceResolution(m_device);
                if (!m_resolutionList.isEmpty()) {
                    setResolution(m_resolutionList.first());
                }
                emit resolutionsChanged();
            } catch (const std::exception& e) {
                qWarning() << "[MEDIASTREAM] Failed to get resolutions for device:" << m_device << "Error:" << e.what();
            } catch (...) {
                qWarning() << "[MEDIASTREAM] Unknown exception getting resolutions for device:" << m_device;
            }
        }
    }
}

QString MediaStream::getResolution()
{
        return m_resolution;
}

void MediaStream::setResolution(QString resolution)
{
    if(!resolution.isEmpty()) {
        qDebug() << "   Setting resolution: " << resolution;
        m_resolution = resolution;
    }
}

void MediaStream::pause()
{
    if (m_isInitialized == true && m_playing == true) {
        m_playing = false;
        qDebug() << "[MediaStream] Paused";
    }
}
void MediaStream::play()
{
    if (m_isInitialized == true) {
        m_playing = true;

        char buffer[20];
        QString resolution = m_resolution;
        const char *r = resolution.toStdString().c_str();
        strncpy(buffer, r, sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';

        char *w = strtok(buffer, "x");
        char *h = strtok(NULL, "x");

        if (m_backend == "G2D")
        {
            qDebug() << "   Launching G2D demo...";
            qDebug() << QString(DEMOPATH) + "/" + DEMOG2D << QStringList() << m_device << w << h << QString::number(m_angle);
            m_process->start(QString(DEMOPATH) + "/" + DEMOG2D, QStringList() << m_device << w << h << QString::number(m_angle));
        }
        if (m_backend == "OpenCV")
        {
            qDebug() << "   Launching OpenCV demo...";
            qDebug() << QString(DEMOPATH) + "/" + DEMOOPENCV << QStringList() << m_device << w << h << QString::number(m_angle);
            m_process->start(QString(DEMOPATH) + "/" + DEMOOPENCV, QStringList() << m_device << w << h << QString::number(m_angle));
        }
        if (m_backend == "OpenGL")
        {
            qDebug() << "   Launching OpenGL demo...";
            qDebug() << QString(DEMOPATH) + "/" + DEMOOPENGL << QStringList() << m_device << w << h << QString::number(m_angle);
            m_process->start(QString(DEMOPATH) + "/" + DEMOOPENGL, QStringList() << m_device << w << h << QString::number(m_angle));

        }
    }
}

void MediaStream::stop()
{
    m_process->terminate();
    setAngle(0);
}

void MediaStream::increase()
{
    int increment = 1;

    if(m_angle < 359)
        m_angle += increment;
    else
        m_angle = 0;
    float radians = qDegreesToRadians(m_angle);
    Q_EMIT angleChanged();

    if(m_isInitialized == true){
        qDebug() << "setRotation angle: " << m_angle << " radians: " << radians;
        m_mqueue->sendAngle(m_angle);
        m_playing = true;
    }
}

void MediaStream::decrease()
{
   int increment = 1;
   
   if(m_angle >= increment)
        m_angle -= increment;
    else
        m_angle = 359;
    float radians = qDegreesToRadians(m_angle);
    Q_EMIT angleChanged();

    if(m_isInitialized == true){
        qDebug() << "setRotation angle: " << m_angle << " radians: " << radians;
        m_mqueue->sendAngle(m_angle);
    }
}
void MediaStream::init()
{
    if (m_isInitialized) {
        qDebug() << "[MEDIASTREAM] Already initialized, skipping...";
        return;
    }
    
    qDebug() << "[MEDIASTREAM] Finding video devices...";
    
    // Safe VideoDevice creation with error handling
    try {
        m_videoModel = new VideoDevice();
        if (!m_videoModel) {
            qWarning() << "[MEDIASTREAM] Failed to create VideoDevice instance";
            return;
        }
    } catch (const std::exception& e) {
        qCritical() << "[MEDIASTREAM] Exception creating VideoDevice:" << e.what();
        return;
    } catch (...) {
        qCritical() << "[MEDIASTREAM] Unknown exception creating VideoDevice";
        return;
    }
    
    // Safe device enumeration with validation
    try {
        m_deviceMap = m_videoModel->devices();
        
        // Validate device map is not empty
        if (m_deviceMap.isEmpty()) {
            qWarning() << "[MEDIASTREAM] No video devices found";
            delete m_videoModel;
            m_videoModel = nullptr;
            return;
        }
        
        // Clear existing containers before populating
        m_deviceList.clear();
        m_uniqueDeviceMap.clear();
        
        // Create a map to track device name occurrences
        QMap<QString, int> deviceNameCount;
        
        // First pass: count occurrences of each device name
        for (auto it = m_deviceMap.constBegin(); it != m_deviceMap.constEnd(); ++it) {
            const QString& devicePath = it.key();
            const QString& deviceName = it.value();
            
            // Skip invalid entries
            if (devicePath.isEmpty() || deviceName.isEmpty()) {
                continue;
            }
            
            // Check if device path exists and is accessible
            if (!QFile::exists(devicePath) || !isValidVideoDevice(devicePath)) {
                continue;
            }
            
            deviceNameCount[deviceName]++;
        }
        
        // Second pass: create unique display names and populate lists
        QMap<QString, int> deviceNameIndex;
        
        for (auto it = m_deviceMap.constBegin(); it != m_deviceMap.constEnd(); ++it) {
            const QString& devicePath = it.key();
            const QString& deviceName = it.value();
            
            // Validate device entries
            if (devicePath.isEmpty() || deviceName.isEmpty()) {
                qWarning() << "[MEDIASTREAM] Invalid device entry - Path:" << devicePath << "Name:" << deviceName;
                continue;
            }
            
            // Check if device path exists and is accessible
            if (!QFile::exists(devicePath)) {
                qWarning() << "[MEDIASTREAM] Device path does not exist:" << devicePath;
                continue;
            }
            
            // Check device permissions
            if (!isValidVideoDevice(devicePath)) {
                qWarning() << "[MEDIASTREAM] Device not accessible:" << devicePath;
                continue;
            }
            
            // Create unique display name for identical devices
            QString uniqueDisplayName;
            if (deviceNameCount[deviceName] > 1) {
                // Multiple devices with same name - add index
                deviceNameIndex[deviceName]++;
                uniqueDisplayName = QString("%1 (%2)").arg(deviceName).arg(deviceNameIndex[deviceName]);
            } else {
                // Single device with this name
                uniqueDisplayName = deviceName;
            }
            
            // Add to our containers
            m_deviceList.append(uniqueDisplayName);
            m_uniqueDeviceMap.insert(devicePath, uniqueDisplayName);
            
            qDebug() << "[MEDIASTREAM] Added device:" << devicePath << "as" << uniqueDisplayName;
        }
        
        // Verify we have valid devices after filtering
        if (m_deviceList.isEmpty()) {
            qWarning() << "[MEDIASTREAM] No accessible video devices found after validation";
            delete m_videoModel;
            m_videoModel = nullptr;
            return;
        }
        
    } catch (const std::exception& e) {
        qCritical() << "[MEDIASTREAM] Exception during device enumeration:" << e.what();
        cleanup();
        return;
    } catch (...) {
        qCritical() << "[MEDIASTREAM] Unknown exception during device enumeration";
        cleanup();
        return;
    }
    
    // Debug output for discovered devices
    qDebug() << "[MEDIASTREAM] Found" << m_deviceList.size() << "accessible video devices:";
    for (const auto& deviceName : std::as_const(m_deviceList)) {
        qDebug() << "   [List] Device Name:" << deviceName;
    }
    
    for (auto it = m_deviceMap.constBegin(); it != m_deviceMap.constEnd(); ++it) {
        qDebug() << "   [Map] Device:" << it.key() << "Name:" << it.value();
    }
    
    // Safe default source selection using the first available device
    if (!m_deviceList.isEmpty()) {
        QString defaultDisplayName = m_deviceList.first();
        qDebug() << "   Setting default source:" << defaultDisplayName;
        
        // Use try-catch for setSource in case it throws
        try {
            setSource(defaultDisplayName);
        } catch (const std::exception& e) {
            qWarning() << "[MEDIASTREAM] Failed to set default source:" << e.what();
            // Continue initialization even if default source fails
        } catch (...) {
            qWarning() << "[MEDIASTREAM] Unknown exception setting default source";
        }
    } else {
        qWarning() << "[MEDIASTREAM] No devices available for default source";
    }
    
    // Emit signals safely
    try {
        emit devicesChanged();
    } catch (...) {
        qWarning() << "[MEDIASTREAM] Exception emitting devicesChanged signal";
    }
    
    // Set backend and mark as initialized
    m_backend = "OpenGL";
    m_isInitialized = true;
    
    qDebug() << "[MEDIASTREAM] Initialization completed successfully";
}
void MediaStream::releaseResources()
{
    cleanup();
}

void MediaStream::findDevices()
{
}
bool MediaStream::isValidVideoDevice(const QString& devicePath) const
{
    if (devicePath.isEmpty()) {
        return false;
    }
    
    // Check if device exists
    if (!QFile::exists(devicePath)) {
        return false;
    }
    
    // Check if it's readable
    QFileInfo info(devicePath);
    if (!info.isReadable()) {
        return false;
    }
    
    // Additional check: verify it's a character device (typical for video devices)
    if (!info.exists() || info.size() < 0) {
        return false;
    }
    
    // Try to briefly access the device to ensure it's not just a placeholder
    QFile device(devicePath);
    if (!device.open(QIODevice::ReadOnly)) {
        return false;
    }
    device.close();
    
    return true;
}