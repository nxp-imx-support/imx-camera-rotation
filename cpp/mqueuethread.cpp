/*
 * Copyright 2025 NXP
 * Copyright (C) 2016 The Qt Company Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <QDebug>
#include "mqueuethread.hpp"

MQueueThread::MQueueThread()
{
    initMqueue();

}

void MQueueThread::initMqueue()
{
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs =0;

    //Create the message queue
    if((mq = mq_open (QUEUE_NAME_APP, O_WRONLY | O_CREAT, QUEUE_PERMISIIONS, &attr)) == -1)
    {
        qDebug() << "ERROR Server: mq_open " << QUEUE_NAME_APP;
    }
}

void MQueueThread::sendAngle(int angle)
{
    char buffer[MAX_MSG_SIZE];
    snprintf(buffer, sizeof(buffer), "%d", angle);

    if (mq_send(mq, buffer, strlen(buffer), 0) == -1) {
        qDebug() << "ERROR Client: Not able to send message to queue";
    } else {
        qDebug() << "Client: Message sent to queue: " << buffer;
    }
}