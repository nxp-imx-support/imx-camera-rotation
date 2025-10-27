/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

import QtQuick
import QtQuick.Controls

Item {
    id: controls
    height: 80
    property var stream

    Row { // Buttons
        id: button
        height: 60
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: Style.margins
        spacing: 20

        RoundButton {
            id: decreaseAngleButton
            width: 40
            height: 40
            text: "Previous"
            display: AbstractButton.IconOnly
            icon.source: "qrc:/icons/rotate_L"
            onClicked: {
                stream.decrease()
            }
        }

        RoundButton {
            id: increaseAngleButton
            width: 40
            height: 40
            text: "Next"
            display: AbstractButton.IconOnly
            icon.source: "qrc:/icons/rotate_R"
            onClicked: {
                stream.increase()
            }
        }        

        RoundButton  {
            id: playButton
            width: 40
            height: 40
            text: "Play"
            display: AbstractButton.IconOnly
            icon.source: "qrc:/icons/play"
            onClicked: {
                    stream.play()
            } 
        }

        RoundButton {
            id: stopButton
            width: 40
            height: 40
            text: "Stop"
            display: AbstractButton.IconOnly
            icon.source: "qrc:/icons/stop"
            onClicked: {
                stream.stop()
            }
        }
    }
}
