import {from, fromEvent, mergeMap, tap, throttleTime} from "rxjs";
import {
  perceptualMap,
  inversePerceptualMap
} from "../../app/src/app/color-utils.ts";
import {
  initWebSocket,
  sendBleStatus,
  sendColorMessage,
  webSocketHandlers
} from "../../app/src/app/websocket.handler.ts";
import {
  BleStatus,
  decodeDeviceNameMessage,
  decodeFirmwareVersionMessage,
  decodeWebSocketAlexaMessage,
  decodeWebSocketEspNowDevicesMessage,
  decodeWebSocketEspNowControllerMessage,
  decodeWebSocketHeapInfoMessage,
  decodeWebSocketOnBleStatusMessage,
  decodeWebSocketOnColorMessage,
  decodeWebSocketOnOtaProgressMessage,
  decodeWebSocketWiFiDetailsMessage,
  decodeWebSocketWiFiStatusMessage,
  EspNowDevice,
  LightState,
  otaStatusToString,
  WebSocketMessageType,
  WiFiDetails,
  wifiStatusToString
} from "../../app/src/app/model"
import {resetSystem, restartSystem} from "../../app/src/app/http-requests.ts";
import {numberToIp} from "../../app/src/app/model/decode.utils.ts";
import {alexaModeToString} from "../../app/src/app/model/alexa-integration-settings.model.ts";

const sliders = Array.from(document.querySelectorAll<HTMLInputElement>('label.slider input[type="range"]'));
const switches = Array.from(document.querySelectorAll<HTMLInputElement>('label.switch input[type="checkbox"]'));
const resetButton = document.querySelector<HTMLButtonElement>("#restart-btn")!;
const restartButton = document.querySelector<HTMLButtonElement>("#reset-btn")!;
const bluetoothButton = document.querySelector<HTMLButtonElement>("#bluetooth-toggle")!;
const espNowHeader = document.querySelector("#esp-now-info>h2") as HTMLHeadElement;
const espNowTable = document.querySelector("#esp-now-info>table") as HTMLTableElement;
const espNowNoDevices = document.querySelector("#esp-now-info>p") as HTMLParagraphElement;
const overlay = document.getElementById('loading-overlay') as HTMLDivElement;

resetButton.addEventListener("click", async (e) => {
  e.preventDefault();
  if (confirm("Restart the device?")) {
    await restartSystem();
  }
});

restartButton.addEventListener("click", async (e) => {
  e.preventDefault();
  if (confirm("Factory reset the device?")) {
    await resetSystem();
  }
});

bluetoothButton.addEventListener("click", async () => {
  const current = bluetoothButton.dataset.enabled === "true";
  bluetoothButton.disabled = true;
  sendBleStatus(current ? BleStatus.OFF : BleStatus.ADVERTISING);
});

switches.forEach((switchEl, index) => {
  switchEl.addEventListener("change", () => {
    const slider = sliders[index];
    const value = parseInt(slider.value, 10);
    const on = switchEl.checked;
    if (on && value <= 14) {
      slider.value = "255";
      slider.dispatchEvent(new Event('input'));
    } else {
      sendColorMessage(getOutputState());
    }
  });
});

from(sliders).pipe(
  mergeMap(slider => fromEvent(slider, 'input')),
  tap(event => {
    const slider = event.target as HTMLInputElement;
    const switchEl = switches[sliders.indexOf(slider)];
    const value = updateSliderVisual(slider);
    if (value > 0) {
      switchEl.checked = true;
    }
  }),
  throttleTime(200, undefined, {leading: true, trailing: true}),
).subscribe(() => sendColorMessage(getOutputState()));

webSocketHandlers.set(WebSocketMessageType.ON_BLE_STATUS, (message: ArrayBuffer) => {
  const {status} = decodeWebSocketOnBleStatusMessage(message);
  updateBluetoothButton(status);
  bluetoothButton.disabled = false;
});

webSocketHandlers.set(WebSocketMessageType.ON_COLOR, (message: ArrayBuffer) => {
  document.getElementById("light-controls")!.style.display = "block";
  const {values} = decodeWebSocketOnColorMessage(message);
  values.forEach(({on, value}, index) => {
    const slider = sliders[index];
    const switchEl = switches[index];
    slider.value = inversePerceptualMap(value).toString();
    switchEl.checked = on;
    updateSliderVisual(slider);
  })
});

webSocketHandlers.set(WebSocketMessageType.ON_DEVICE_NAME, (message: ArrayBuffer) => {
  const {deviceName} = decodeDeviceNameMessage(message);
  updateText("device-name", deviceName);
});

webSocketHandlers.set(WebSocketMessageType.ON_OTA_PROGRESS, (message: ArrayBuffer) => {
  const {status, totalBytesExpected, totalBytesReceived} = decodeWebSocketOnOtaProgressMessage(message);
  const percentage = totalBytesExpected > 0
    ? Math.round((totalBytesReceived / totalBytesExpected) * 100)
    : 0;
  const text = otaStatusToString(status);
  updateText("ota-status", percentage > 0 ? `${text} (${percentage}%)` : text);
});

webSocketHandlers.set(WebSocketMessageType.ON_HEAP, (message: ArrayBuffer) => {
  const {freeHeap} = decodeWebSocketHeapInfoMessage(message);
  updateText("heap", `${freeHeap}`);
});

webSocketHandlers.set(WebSocketMessageType.ON_ESP_NOW_DEVICES, (message: ArrayBuffer) => {
  const {devices} = decodeWebSocketEspNowDevicesMessage(message);
  updateEspNowDevices(devices);
});

webSocketHandlers.set(WebSocketMessageType.ON_ESP_NOW_CONTROLLER, (message: ArrayBuffer) => {
  const {address} = decodeWebSocketEspNowControllerMessage(message);
  updateEspNowController(address);
});

webSocketHandlers.set(WebSocketMessageType.ON_FIRMWARE_VERSION, (message: ArrayBuffer) => {
  const {firmwareVersion} = decodeFirmwareVersionMessage(message);
  updateText("firmware-version", firmwareVersion);
});

webSocketHandlers.set(WebSocketMessageType.ON_WIFI_DETAILS, (message: ArrayBuffer) => {
  const {details} = decodeWebSocketWiFiDetailsMessage(message);
  updateWiFiDetails(details);
})

webSocketHandlers.set(WebSocketMessageType.ON_WIFI_STATUS, (message: ArrayBuffer) => {
  const {status} = decodeWebSocketWiFiStatusMessage(message);
  updateText("wifi-status", wifiStatusToString(status));
});

webSocketHandlers.set(WebSocketMessageType.ON_WIFI_STATUS, (message: ArrayBuffer) => {
  const {status} = decodeWebSocketWiFiStatusMessage(message);
  updateText("wifi-status", wifiStatusToString(status));
});

webSocketHandlers.set(WebSocketMessageType.ON_ALEXA_INTEGRATION_SETTINGS, (message: ArrayBuffer) => {
  const {
    settings: {
      integrationMode,
      rDeviceName,
      gDeviceName,
      bDeviceName,
      wDeviceName
    }
  } = decodeWebSocketAlexaMessage(message);
  document.getElementById("alexa-info")!.style.display = "block";
  updateText("alexa-mode", alexaModeToString(integrationMode));
  updateText("alexa-names", [rDeviceName, gDeviceName, bDeviceName, wDeviceName].filter(Boolean).join(", "));
});

function updateWiFiDetails(details: WiFiDetails) {
  updateText("wifi-ssid", details.ssid);
  updateText("wifi-mac", details.mac);
  updateText("wifi-ip", numberToIp(details.ip));
  updateText("wifi-gateway", numberToIp(details.gateway));
  updateText("wifi-subnet", numberToIp(details.subnet));
  updateText("wifi-dns", numberToIp(details.dns));
}

function updateText(id: string, value: string): void {
  const el = document.getElementById(id);
  if (el) el.textContent = value;
}

function getOutputState(): [LightState, LightState, LightState, LightState] {
  const states: LightState[] = [];
  for (let i = 0; i < sliders.length; i++) {
    const slider = sliders[i];
    const switchEl = switches[i];
    const originalValue = parseInt(slider.value, 10);
    const value = perceptualMap(originalValue);
    const on = switchEl.checked;
    states.push({on, value});
  }
  return states as [LightState, LightState, LightState, LightState];
}

function updateBluetoothButton(status: BleStatus): void {
  const statusString = status === BleStatus.OFF
    ? "OFF"
    : status === BleStatus.ADVERTISING
      ? "ADVERTISING"
      : "CONNECTED";
  bluetoothButton.dataset.enabled = status !== BleStatus.OFF ? "true" : "false";
  bluetoothButton.textContent = `Bluetooth: ${statusString}`;
  bluetoothButton.classList.toggle("active", status !== BleStatus.OFF);
  bluetoothButton.disabled = false;
}

function updateSliderVisual(slider: HTMLInputElement) {
  const label = slider.parentElement!;
  const color = getComputedStyle(label!).getPropertyValue('--color').trim();
  const [_, r, g, b] = color.match(/rgb\((\d+),\s*(\d+),\s*(\d+)/)!;
  const value = parseInt(slider.value, 10) - 14;
  const percentage = Math.round((value / (255 - 14)) * 100);
  slider.style.background = `linear-gradient(to right, ${color} 0%, ${color} ${percentage}%, rgba(${r}, ${g}, ${b}, 0.3) ${percentage}%, rgba(${r}, ${g}, ${b}, 0.3) 100%)`;
  const labelSpan = slider.parentElement?.querySelector("span");
  if (labelSpan) labelSpan.textContent = `${percentage}%`;
  return value;
}

function onConnected() {
  hideLoadingOverlay();
}

function onDisconnected() {
  showLoadingOverlay("Reconnecting...");
}

function showLoadingOverlay(text = "Loading...") {
  overlay.querySelector("p")!.textContent = text;
  overlay.classList.remove('hidden');
}

function hideLoadingOverlay() {
  overlay.classList.add('hidden');
}

function updateEspNowDevices(devices: EspNowDevice[]) {
  espNowHeader.textContent = "ESP NOW Remotes";
  espNowTable.innerHTML = "";
  if (devices.length === 0) {
    espNowNoDevices.innerText = "No devices allowed."
    return;
  }

  espNowNoDevices.innerText = "";
  createEspNowListHeader(espNowTable);

  const tBody = espNowTable.createTBody();
  devices.forEach(device => {
    createEspNowListItem(tBody, device);
  });
}

function updateEspNowController(address: string) {
  espNowHeader.textContent = "ESP NOW Controller";
  espNowTable.innerHTML = "";
  espNowNoDevices.style.display = "none";
  const tr = espNowTable.insertRow();
  const td = tr.insertCell();
  td.style.textAlign = "center";
  td.innerText = address;
}

function createEspNowListHeader(espNowTable: HTMLTableElement) {
  const tHead = espNowTable.createTHead();
  const tr = tHead.insertRow();
  const thName = tr.insertCell();
  thName.textContent = "Device Name";

  const thMac = tr.insertCell();
  thMac.textContent = "MAC Address";
}

function createEspNowListItem(tBody: HTMLTableSectionElement, device: EspNowDevice) {
  const tr = tBody.insertRow();
  const tdName = tr.insertCell();
  tdName.textContent = device.name;

  const tdMac = tr.insertCell();
  tdMac.textContent = device.address;
}

initWebSocket(`ws://${location.host}/ws`, onConnected, onDisconnected);
