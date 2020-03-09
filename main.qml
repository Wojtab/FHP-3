import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.14

Window {
    visible: true
    width: 640
    height: 480
    id: root
    title: qsTr("FHP-3")


    Image {
        id: preview
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: startButton.top
        source: "image://images/d"
        property var counter: 0
    }
    Button {
        id: startButton
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        text: "START"
        onClicked: {
            sim.start()
            refreshTimer.start()
        }
    }
    CheckBox {
        id: typeCheckbox
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        onCheckedChanged: {
            if(checked)
            {
                preview.source = "image://images/v" + preview.counter
                preview.counter += 1
            }
            else
            {
                preview.source = "image://images/d" + preview.counter
                preview.counter += 1
            }
        }
    }

    Button {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        text: "STOP"
        onClicked: {
            sim.stop()
            refreshTimer.stop()
        }
    }
    Timer {
        id: refreshTimer
        running: false
        interval: 100
        repeat: true
        onTriggered: {
            if(typeCheckbox.checked)
            {
                preview.source = "image://images/v" + preview.counter
                preview.counter += 1
            }
            else
            {
                preview.source = "image://images/d" + preview.counter
                preview.counter += 1
            }
        }
    }

    //show all incoming/outgoing possibilities
    /*
    property var mdata: [[0,0,0],[1,1,1],[2,2,2],[3,3,3],[4,4,4],[5,66,66],[6,6,6],[7,7,7],[8,8,8],[9,36,18],[10,68,68],[11,38,69],[12,12,12],[13,74,22],[14,14,14],[15,15,15],[16,16,16],[17,96,96],[18,9,36],[19,98,37],[20,72,72],[21,42,42],[22,13,74],[23,102,75],[24,24,24],[25,52,104],[26,84,44],[27,45,54],[28,28,28],[29,90,108],[30,30,30],[31,110,110],[32,32,32],[33,33,33],[34,65,65],[35,35,35],[36,18,9],[37,19,98],[38,69,11],[39,39,39],[40,80,80],[41,81,50],[42,21,21],[43,83,101],[44,26,84],[45,54,27],[46,77,86],[47,87,87],[48,48,48],[49,49,49],[50,41,81],[51,51,51],[52,104,25],[53,105,114],[54,27,45],[55,107,107],[56,56,56],[57,57,57],[58,116,89],[59,117,117],[60,60,60],[61,122,122],[62,93,93],[63,63,63],[64,64,64],[65,34,34],[66,5,5],[67,67,67],[68,10,10],[69,11,38],[70,70,70],[71,71,71],[72,20,20],[73,100,82],[74,22,13],[75,23,102],[76,76,76],[77,86,46],[78,78,78],[79,79,79],[80,40,40],[81,50,41],[82,73,100],[83,101,43],[84,44,26],[85,106,106],[86,46,77],[87,47,47],[88,88,88],[89,58,116],[90,108,29],[91,109,118],[92,92,92],[93,62,62],[94,94,94],[95,95,95],[96,17,17],[97,97,97],[98,37,19],[99,99,99],[100,82,73],[101,43,83],[102,75,23],[103,103,103],[104,25,52],[105,114,53],[106,85,85],[107,55,55],[108,29,90],[109,118,91],[110,31,31],[111,111,111],[112,112,112],[113,113,113],[114,53,105],[115,115,115],[116,89,58],[117,59,59],[118,91,109],[119,119,119],[120,120,120],[121,121,121],[122,61,61],[123,123,123],[124,124,124],[125,125,125],[126,126,126],[127,127,127],[128,128,128],[129,136,136],[130,144,144],[131,152,152],[132,160,160],[133,168,168],[134,176,176],[135,184,184],[136,129,129],[137,137,137],[138,145,145],[139,153,153],[140,161,161],[141,169,169],[142,177,177],[143,185,185],[144,130,130],[145,138,138],[146,146,146],[147,154,154],[148,162,162],[149,170,170],[150,178,178],[151,186,186],[152,131,131],[153,139,139],[154,147,147],[155,155,155],[156,163,163],[157,171,171],[158,179,179],[159,187,187],[160,132,132],[161,140,140],[162,148,148],[163,156,156],[164,164,164],[165,172,172],[166,180,180],[167,188,188],[168,133,133],[169,141,141],[170,149,149],[171,157,157],[172,165,165],[173,173,173],[174,181,181],[175,189,189],[176,134,134],[177,142,142],[178,150,150],[179,158,158],[180,166,166],[181,174,174],[182,182,182],[183,190,190],[184,135,135],[185,143,143],[186,151,151],[187,159,159],[188,167,167],[189,175,175],[190,183,183],[191,191,191],[192,192,192],[193,200,200],[194,208,208],[195,216,216],[196,224,224],[197,232,232],[198,240,240],[199,248,248],[200,193,193],[201,201,201],[202,209,209],[203,217,217],[204,225,225],[205,233,233],[206,241,241],[207,249,249],[208,194,194],[209,202,202],[210,210,210],[211,218,218],[212,226,226],[213,234,234],[214,242,242],[215,250,250],[216,195,195],[217,203,203],[218,211,211],[219,219,219],[220,227,227],[221,235,235],[222,243,243],[223,251,251],[224,196,196],[225,204,204],[226,212,212],[227,220,220],[228,228,228],[229,236,236],[230,244,244],[231,252,252],[232,197,197],[233,205,205],[234,213,213],[235,221,221],[236,229,229],[237,237,237],[238,245,245],[239,253,253],[240,198,198],[241,206,206],[242,214,214],[243,222,222],[244,230,230],[245,238,238],[246,246,246],[247,254,254],[248,199,199],[249,207,207],[250,215,215],[251,223,223],[252,231,231],[253,239,239],[254,247,247],[255,255,255]]
    ScrollView {
        anchors.fill: parent
        Column {
            width: root.width
            Repeater {
                model: root.mdata
                Item {
                    id: repeated
                    width: root.width
                    height: 100
                    property var d: modelData
                    Row {
                        anchors.fill: parent
                        Repeater {
                            model: 3
                            Item {
                                id: graphics
                                width: parent.height
                                height: parent.height
                                property var i: index
                                Repeater {
                                    model: 6
                                    Rectangle {
                                        anchors.left: parent.horizontalCenter
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 40
                                        height: 2
                                        visible: graphics.i != 2 || repeated.d[1] != repeated.d[2]
                                        color: repeated.d[graphics.i] & (1 << index) ? "#ff0000" : "#c0c0c0"
                                        Rectangle {
                                            visible: repeated.d[graphics.i] & (1 << index)
                                            color: parent.color
                                            height: 2
                                            width: 10
                                            x: graphics.i == 0 ? 0 : parent.width-width
                                            anchors.verticalCenter: parent.verticalCenter
                                            transform: Rotation {
                                                origin.x: graphics.i == 0 ? 0 : 10
                                                origin.y: 1
                                                angle: 20
                                            }
                                        }
                                        Rectangle {
                                            visible: repeated.d[graphics.i] & (1 << index)
                                            color: parent.color
                                            height: 2
                                            width: 10
                                            x: graphics.i == 0 ? 0 : parent.width-width
                                            anchors.verticalCenter: parent.verticalCenter
                                            transform: Rotation {
                                                origin.x: graphics.i == 0 ? 0 : 10
                                                origin.y: 1
                                                angle: -20
                                            }
                                        }

                                        transform: Rotation {
                                            origin.x: 0
                                            origin.y: 1
                                            angle: (graphics.i == 0 ? 60 : -120) + 60*index
                                        }
                                    }
                                }
                                Rectangle {
                                    width: 20
                                    height: 20
                                    anchors.centerIn: parent
                                    border.color: "#000000"
                                    color: "transparent"
                                    visible: (repeated.d[graphics.i] & (1 << 7)) && (graphics.i != 2 || repeated.d[1] != repeated.d[2])
                                }
                                Rectangle {
                                    width: 20
                                    height: 20
                                    anchors.centerIn: parent
                                    border.color: "#0000ff"
                                    color: "transparent"
                                    visible: (repeated.d[graphics.i] & (1 << 6)) && (graphics.i != 2 || repeated.d[1] != repeated.d[2])
                                    radius: 10
                                }
                            }
                        }
                    }
                }
            }
        }
    }*/
}
