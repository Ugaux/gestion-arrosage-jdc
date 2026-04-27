
// Run time function
setInterval(function () {
    getESPTime();
    getSensorsValues();
    getStatus();
    manualInProgress();
    getNextWatering();
}, 1000);

/********** WebSocket **********/
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onLoad);

// OnLoad
function onLoad(event) {
    initWebSocket();
    initButton();
}

// InitWebSocket
function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

// OnOpen
function onOpen(event) {
    console.log('Connection opened');
}

// OnClose
function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

// OnMessage
function onMessage(event) {
    let data = JSON.parse(event.data);

    // Test relays
    if (data.update == "test_relay") {
        let id = "btn_" + data.relay;
        document.getElementById(id).checked = !data.state;
    }

    // Enable buttons
    else if (data.update == "enable_button") {
        console.log(data);
        let id = "btn_enable_" + data.way + "_" + data.index;
        document.getElementById(id).checked = data.state;
    }

    // Date and time
    else if (data.update == "date_time") {
        document.getElementById("mainTime").innerHTML = data.time;
        document.getElementById("relayTime").innerHTML = data.date + " " + data.time;
    }

    // Sensors
    else if (data.update == "sensors") {
        // Moisture
        if (data.moisture != "No sensor") {
            document.getElementById("moistureSensor").innerHTML = " Humidity <b>" + data.moisture + "</b>%";
        } else {
            document.getElementById("moistureSensor").innerHTML = " No sensor";
        }
        
        // Flow
        if (data.flow != "No sensor") {
            document.getElementById("flowSensor").innerHTML = " Flow <b>" + data.flow + "</b> l/min";
        } else {
            document.getElementById("flowSensor").innerHTML = " No sensor";
        }

        // Seasonal adjustment
        document.getElementById("seasonAdjustment").innerHTML = " Seasonnal adj. <b>" + data.adjustment + "</b>%";
    }

    // Manual mode -> Update buttons START or STOP
    else if (data.update == "manual_mode") {
        let id_btn = "btn_" + data.way;
        let id_lbl = "lbl_" + data.way;
        document.getElementById(id_btn).innerHTML = data.operation;
        if (data.state == "stop") {
            document.getElementById(id_lbl).innerHTML = ""; // Hide timer before end of countdown
        }
    }

    // Manual mode -> Update timers
    else if (data.update == "manual_in_progress") {
        let ways = data.ways.split(",");
        for (let i = 0; i < ways.length; i++) {
            let token = ways[i].split("=");
            let id_btn = "btn_" + token[0];
            let id_lbl = "lbl_" + token[0];
            if (token[1] != "00:00") {
                document.getElementById(id_lbl).innerHTML = token[1];
            } else {
                document.getElementById(id_lbl).innerHTML = "";
                document.getElementById(id_btn).innerHTML = '<i class="fa-solid fa-play" aria-hidden="true"></i>'; // Toggle button to START
            }
        }
    }

    // Watering status -> Next irrigation / Watering in progress / Manual mode / No scheduled watering
    else if (data.update == "watering_status") {
        // DEBUG console.log(data);
        document.getElementById("wateringStatus").innerHTML = data.status;
        
        // Manual & Auto watering
        if (data.status == "Manual watering" || data.status == "Watering in progress") {
            let infos = data.infos.split(","); // Lawn.House=00:10,Dripline.Flowers=00:23
            for (let i = 0; i < infos.length; i++) {
                let token = infos[i].split("="); // Lawn.House=00:10
                let name = token[0].split("."); // Dripline.Flowers
                document.getElementById("wateringLine1").innerHTML = name[1];
                document.getElementById("wateringLine2").innerHTML = token[1] + " min remaining";
                document.getElementById("wateringLine3").innerHTML = infos.length > 1 ? "<i><small>" + (infos.length - 1) + " more in progress</small></i>" : "";     
            }
        // Next watering and no scheduled irrigation
        } else {
            document.getElementById("wateringLine1").innerHTML = data.line_1 != "" ? data.line_1 : "";
            document.getElementById("wateringLine2").innerHTML = data.line_2 != "" ? data.line_2 : "";
            document.getElementById("wateringLine3").innerHTML = data.line_3 != "" ? data.line_3 : "";     
        }  
    }

    // Next watering for each way
    else if(data.update == "next_watering") {
        console.log(data);
        let infos = data.infos.split(",");
        for (let i = 0; i < infos.length; i++) {
            let token = infos[i].split("=");
            let id_lbl = "lbl_next_" + token[0];
            document.getElementById(id_lbl).innerHTML = token[1];
        }
    }


}

// Test relays - Buttons update
function initButton() {    
    // Test relays
    let checkbox_test = document.getElementsByName("checkbox_test");
    for (let i = 0; i < checkbox_test.length; i++) {
        let gpio = checkbox_test[i].id;
        document.getElementById(gpio).addEventListener('click', function() {
            sendData({update: "test_relay", relay: gpio.substring(4)});
        });
    }

    // Enable buttons (to enable/disable a watering)
    let checkbox_enable = document.getElementsByName("checkbox_enable");
    for (let n = 0; n < checkbox_enable.length; n++) {
        let id = checkbox_enable[n].id;
        const arr = id.split("_");
        document.getElementById(id).addEventListener('click', function() {
            sendData({update: "enable_button", way: arr[2], index: arr[3]});
        });
    }

    // Manual start
    let manual_button = document.getElementsByName("manual_mode");
    for (let i = 0; i < manual_button.length; i++) {
        let wayName = manual_button[i].id;
        document.getElementById(wayName).addEventListener('click', function() {
            let duration_id = "m_duration_" + wayName.substring(4);
            let duration = document.getElementById(duration_id).value;
            let operation = document.getElementById(wayName).innerHTML == '<i class="fa-solid fa-play" aria-hidden="true"></i>' ? "start" : "stop";
            sendData({update: "manual_mode", way: wayName.substring(4), duration: duration, operation: operation});
        });
    }
}

// Websocket send JSON data
function sendData(objString) {
    const obj = objString;
    let objJSON = JSON.stringify(obj);
    websocket.send(objJSON);
}

// Get ESP32 Sytem time
function getESPTime() {
    sendData({update: "date_time"});
}

// Get flow and humidity values
function getSensorsValues() {
    sendData({update: "sensors"});
}

// Get remain time in manual mode
function manualInProgress() {
    sendData({update: "manual_in_progress"});
}

// Get waterings status (webpage's top closed to sensors infos)
function getStatus() {
    sendData({update: "watering_status"});
}

// Get next watering times for each way
function getNextWatering() {
    sendData({update: "next_watering"});
}