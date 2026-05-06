import { getSnapshot } from "../core/api.js";
//import { sendData } from "../ws.js";

initPage();

async function initPage() {
  const initialPage = window.location.hash.substring(1) || "schedule";
  console.log(initialPage);
  showPage(initialPage);

  const data = await getSnapshot();

  if (data?.wifi) {
  renderWifi(data.wifi);
  } else {
    console.warn("No wifi data found in response from /api/snapshot");
  }
  if (data?.valve) {
  renderValves(data.valve);
  } else {
    console.warn("No valve data found in response from /api/snapshot");
  }
  if (data?.relays) {
  renderRelays(data.relays);
  } else {
    console.warn("No relays data found in response from /api/snapshot");
  }
  if (data?.zones) {
  renderZones(data.zones);
  } else {
    console.warn("No zones data found in response from /api/snapshot");
  }

  initRelayToggles();
}

// live updates
//initWS((msg) => {
//  if (msg.type === "status") {
//    renderZones(msg.data.zones);
//    renderRelays(msg.data.relays);
//  }
//});

function w3_open() {
  document.getElementById("mySidebar").style.width = "100%";
  document.getElementById("mySidebar").style.display = "block";
}

function w3_close() {
  document.getElementById("mySidebar").style.display = "none";
}

function renderWifi(wifi) {
  const container = document.getElementById("wifi");

  container.innerHTML = `
    <div class='w3-content w3-half'>
      <p style='margin: 15px 0 7px 0 '><i class='fa-solid fa-network-wired' style='width: 20px;'></i>&nbsp;${wifi.ssid}</p>
      <p style='margin: 7px 0'><i class='fa-solid fa-wifi' style='width: 20px;'></i>&nbsp;${wifi.ip}</p>
    </div>
    <div class='w3-content w3-half'>
      <p style='margin: 7px 0'><i class='fa-solid fa-signal' style='width: 20px;'></i>&nbsp;${wifi.rssi}&nbsp;dBm</p>
      <p style='margin: 7px 0 15px 0'><i class='fa-regular fa-clock' style='width: 20px;'></i>
        <span id='relayTime'></span>
      </p>
    </div>
  `;
}

function renderValves(valve) {
  const container = document.getElementById("valve");

  container.innerHTML = `
    <p style="margin: 15px 0 7px 0">
      <i class="fa-solid fa-hourglass-half" style="width: 20px;"></i>
      &nbsp;Manual duration&nbsp;<b>${valve.duration}</b>&nbsp;min
    </p>

    <p style="margin: 7px 0 15px 0">
      <i class="fa-solid fa-stopwatch" style="width: 20px;"></i>
      &nbsp;Main valve open time&nbsp;<b>${valve.openTime}</b>&nbsp;s
    </p>
  `;
}

function renderRelays(relays) {
  const container = document.getElementById("relay_toggles");

  let html = "";
  for (let i = 0; i < relays.length; i++) {
    const is_checked = relays[i].state === "ON" ? "checked" : "";
    html += `
      <div class='w3-col' style='width: 33%; margin: 15px 0 0 0'>
        <h6><b>Relay #${relays[i].n}</b></h6>
        <h6><small>${relays[i].name}</small></h6>
        <label class='switch'>
          <input name='checkbox_test' type='checkbox' id='btn_${relays[i].name}' ${is_checked}><span class='slider'></span>
        </label>
      </div>
    `;
  }
  container.innerHTML = html;
}

function renderZones(zones) {
  const container = document.getElementById("watering_table");

  let html = "";
  zones.forEach((zone) => {
    html += `
      <div class='w3-card w3-border w3-round w3-margin-top-7'>
        <header class='w3-container w3-border-bottom w3-theme' style='border-radius: 3px 3px 0px 0px; height: 22px'>
          <h6 style='margin: 2px;'><b>Zone ${zone.name}</b></h6>
        </header>
    `;
    zone.ways.forEach((way) => {
      html += `
        <div class='w3-container w3-border-top w3-theme-l4'>
          <div class='w3-bar w3-small' style='height: 49px; padding: 8px 0px;'>
            <p class='w3-left'>
              <a href='/add?way=${way.name}'>
                <i class='fa-solid fa-circle-plus fa-xl w3-text-theme' style='margin-left:0px; width:30px;'></i>
              </a>
              <b class='w3-small'>${way.shortName}</b>
            </p>

            <button name='manual_mode' id='btn_${way.name}' type='btn' class='w3-right w3-btn w3-theme w3-round'
              style='height: 33px; width: 50px;'>
              ${
                !way.manual.started
                  ? `<i class='fa-solid fa-play' aria-hidden='true'></i>`
                  : `<i class='fa-solid fa-stop' aria-hidden='true'></i>`
              }
            </button>

            <div class='input-icons w3-right w3-margin-right-8' style='width: 100px;'>
              <i class=' fa-solid fa-stopwatch icon'></i>
              <input id='m_duration_${way.name}' class='w3-input w3-border w3-round input-field' type='number' step='5'
                value='${way.manual.duration}'>
            </div>
            <p id='lbl_${way.name}' class='w3-right w3-margin-right-8'></p>
          </div>
        </div>

        <div class='w3-container w3-border-top'>
          <p class='w3-small w3-text-gray'><b><i id='lbl_next_${way.name}'>&nbsp;</i></b></p>
        </div>
      `;
      way.waterings.forEach((watering) => {
        html += `
          <div class='w3-container w3-border-top'>
            <a href='/edit?way=${way.name}&schedule=${watering.index}'>
              <p class='w3-small w3-left' style='margin-bottom: 0px;'>
                <i class='fa-solid fa-clock'></i>
                <b>&nbsp;Schedule&nbsp;${watering.index}</b>
              </p>
            </a>

            <label class='switch w3-right w3-margin-top-6'>
              <input name='checkbox_enable' type='checkbox' id='btn_enable_${way.name}_${watering.index}' ${watering.enabled ? "checked" : ""}>
              <span class='slider'></slider>
            </label>

            <div class='w3-row'>
              <div class='w3-col' style='width:80px'>
                <p class='w3-margin-4 w3-text-gray'><span class='w3-small'>Start time :</span></p>
                <p class='w3-margin-4 w3-text-gray'><span class='w3-small'>Duration :</span></p>
                <p class='w3-margin-4 w3-text-gray'><span class='w3-small'>Frequency :</span></p>
              </div>

              <div class='w3-rest'>
                <p class='w3-margin-4'>${watering.time}</p>
                <p class='w3-margin-4'>${watering.duration}&nbsp;min</p>
                <p class='w3-margin-4'>${watering.freq}</p>
              </div>
            </div>
          </div>
        `;
      });
    });
  });

  html += "</div>";

  container.innerHTML = html;
}

// Test relays - Buttons update
function initRelayToggles() {
  // Test relays
  let checkbox_test = document.getElementsByName("checkbox_test");
  for (let i = 0; i < checkbox_test.length; i++) {
    let gpio = checkbox_test[i].id;
    document.getElementById(gpio).addEventListener("click", function () {
      sendData({ update: "test_relay", relay: gpio.substring(4) });
    });
  }

  // Enable buttons (to enable/disable a watering)
  let checkbox_enable = document.getElementsByName("checkbox_enable");
  for (let n = 0; n < checkbox_enable.length; n++) {
    let id = checkbox_enable[n].id;
    const arr = id.split("_");
    document.getElementById(id).addEventListener("click", function () {
      sendData({ update: "enable_button", way: arr[2], index: arr[3] });
    });
  }

  // Manual start
  let manual_button = document.getElementsByName("manual_mode");
  for (let i = 0; i < manual_button.length; i++) {
    let wayName = manual_button[i].id;
    document.getElementById(wayName).addEventListener("click", function () {
      let duration_id = "m_duration_" + wayName.substring(4);
      let duration = document.getElementById(duration_id).value;
      let operation =
        document.getElementById(wayName).innerHTML ==
        '<i class="fa-solid fa-play" aria-hidden="true"></i>'
          ? "start"
          : "stop";
      sendData({
        update: "manual_mode",
        way: wayName.substring(4),
        duration: duration,
        operation: operation,
      });
    });
  }
}
