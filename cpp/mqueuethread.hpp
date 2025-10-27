/*
 * Copyright 2025 NXP
 * Copyright (C) 2016 The Qt Company Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <mqueue.h>
#include <QThread>
#include <QString>

#define QUEUE_NAME_APP "/imx-camera-rotation_queue"
#define MAX_SIZE 1024
#define QUEUE_PERMISIIONS 0664
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE 256

class MQueueThread : public QThread
{
    Q_OBJECT
public:

    MQueueThread();

    void initMqueue(void);
    void sendAngle(int angle);

signals:


private:

    struct mq_attr attr;
    mqd_t mq;

};