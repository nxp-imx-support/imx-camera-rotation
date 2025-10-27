/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import QtCore
import "qml"
import MediaStream 1.0

ApplicationWindow {
    id: mainWindow
    visible: true
    title: qsTr("i.MX Arbitrary Camera Rotation")
    height: Screen.desktopAvailableHeight * 0.25
    width: Screen.desktopAvailableWidth * 0.6
    property int fullscreen: 1
    color: Style.white

    menuBar: MenuBar {
        Menu {
            title: qsTr("&Help")
            Action { 
                text: qsTr("&Readme")
                onTriggered: readme.open()
            }
            Action { 
                text: qsTr("&Licenses")
                onTriggered: licenses.open()
            }
        }
    }

    MediaStream {
        id: mediastream
    }

    Label{
        id: label_backendselector
        text: qsTr("Backend:")
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.topMargin: 15
    }

    ComboBox{
        id: combobox_backendselector
        anchors.margins:10
        anchors.top: parent.top
        anchors.left: label_backendselector.right
        width:150

        model: ["OpenGL", "OpenCV", "G2D"]
        currentIndex: 0
        onActivated: function(index) {
                console.log("Selected Backend:", model[index])
                mediastream.backend = model[index];
        }
    }

    Label{
        id: label_cameraselector
        text: qsTr("Camera Device:")
        anchors.top: parent.top
        anchors.left: combobox_backendselector.right
        anchors.leftMargin: 20
        anchors.topMargin: 15
    }

    ComboBox{
        id: combobox_deviceselector
        width:200
        anchors.left: label_cameraselector.right
        anchors.top: parent.top
        anchors.margins:10
        currentIndex: 0
        model: mediastream.devices

        onCurrentIndexChanged: {
            let current = combobox_deviceselector.model[currentIndex]
            mediastream.source = current
        }
    }

    Label{
        id: label_resolutionselector
        text: qsTr("Resolution:")
        anchors.top: parent.top
        anchors.left: combobox_deviceselector.right
        anchors.leftMargin: 20
        anchors.topMargin: 15
    }

    ComboBox{
        id: combobox_resolutionselector
        width:200
        anchors.left: label_resolutionselector.right
        anchors.top: parent.top
        anchors.margins:10
        currentIndex: 0
        model: mediastream.resolutions

        onCurrentIndexChanged: {
            let current = combobox_resolutionselector.model[currentIndex]
            mediastream.resolution = current
        }
    }

    Label{
        id: label_showangle
        text: qsTr("Current Angle:")
        anchors.top: parent.top
        anchors.left: combobox_resolutionselector.right
        anchors.leftMargin: 20
        anchors.topMargin: 15
    }

    Label{
        id: label_anglevalue
        text: mediastream.angle
        anchors.top: parent.top
        anchors.left: label_showangle.right
        anchors.leftMargin: 20
        anchors.topMargin: 5
        font.pixelSize: 25
        font.bold: true
    }

    MediaControls {
        id: mediacontrols
        stream: mediastream
        width: parent.width
        anchors.top: combobox_backendselector.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 35
    }

    Popup {
        id: readme
        anchors.centerIn: Overlay.overlay
        width: 660
        height: 320
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        Text {
            id: readmeheader
            text: "i.MX Camera Rotation"
            width: parent.width
            height: 40
            font.pointSize: 16.0
            horizontalAlignment: Text.AlignHCenter

        }
        Text {
            id: readmetext
            text: "<br>This example application shows methods to accelerate video rotation on NXP i.MX platforms, enabling more stable 
            and readable video streams from moving cameras, particularly useful in medical or industrial inspection scenarios where 
            cameras are hand-held or rotating.<br> 
            <br>The GUI selects backend to use (OpenGL, OpenCV or G2D), and V4L2 camera input / resolution.
            When launching the application, rotation angle is shown on GUI, and arrow buttons are used to control rotation in 
            clockwise or anticlockwise."

            anchors.top: readmeheader.bottom
            wrapMode: Text.WordWrap
            font.pointSize: 10.0
            width: parent.width
            horizontalAlignment: Text.AlignLeft
        }
    }

    Popup {
        id: licenses
        anchors.centerIn: Overlay.overlay
        width: 660
        height: 180
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        Text {
            id: licensesheader
            text: "Licenses"
            width: parent.width
            height: 40
            font.pointSize: 16.0
            horizontalAlignment: Text.AlignHCenter

        }
        Text {
            id: licenseslist
            text: "- <b>Qt</b>: This application uses the Open Source version of Qt libraries under <b>LGPL-3.0-only</b> license.<br>
                   - <b>UI Icons</b>: This application uses icons from Google Fonts under <b>Apache-2.0</b> license.<br>
                   - <b>Source code</b>: Sources files are provided under <b>BSD 3-Clause</b> license."

            anchors.top: licensesheader.bottom
            font.pointSize: 10.0
            width: parent.width
            horizontalAlignment: Text.AlignLeft
        }
    }
}
