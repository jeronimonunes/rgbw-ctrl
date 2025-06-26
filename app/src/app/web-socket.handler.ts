import {
  AlexaIntegrationSettings,
  encodeAlexaIntegrationSettingsMessage,
  encodeBleStatusMessage,
  encodeColorMessage,
  encodeDeviceNameMessage,
  encodeHttpCredentialsMessage,
  encodeOtaProgressMessage,
  encodeWiFiConnectionDetailsMessage,
  encodeWiFiScanStatusMessage,
  LightState,
  WebSocketMessageType,
  WiFiConnectionDetails
} from "./model";

const AUTO_CLOSE_TIMEOUT_MS = 1500;
const RECONNECT_INTERVAL_MS = 500;

export const webSocketHandlers = new Map<WebSocketMessageType, (data: ArrayBuffer) => void>();

let socket: WebSocket;
let reconnecting = false;
let autoCloseTimeout: ReturnType<typeof setTimeout> | null = null;

export function initWebSocket(url: string, onConnected?: () => void, onDisconnected?: () => void) {
  socket = new WebSocket(url);
  socket.binaryType = "arraybuffer";

  socket.onopen = () => {
    console.info("WebSocket connected");
    reconnecting = false;
    onConnected && onConnected();
    setupAutoClose(onDisconnected);
  };

  socket.onmessage = (event: MessageEvent<ArrayBuffer>) => {
    setupAutoClose(onDisconnected);
    handleMessage(event.data);
  }

  socket.onerror = (err) => {
    console.error("WebSocket error", err);
    tryReconnect(url);
  };

  socket.onclose = () => {
    console.warn("WebSocket closed");
    autoCloseTimeout && clearTimeout(autoCloseTimeout)
    onDisconnected && onDisconnected();
    tryReconnect(url, onConnected, onDisconnected);
  };
}

function setupAutoClose(onclose?: () => void) {
  autoCloseTimeout && clearTimeout(autoCloseTimeout);
  autoCloseTimeout = setTimeout(() => {
    console.warn("WebSocket auto-closing due to inactivity");
    socket.close();
    onclose && onclose();
  }, AUTO_CLOSE_TIMEOUT_MS);
}

function tryReconnect(url: string, onConnected?: () => void, onDisconnected?: () => void) {
  if (reconnecting) return;
  reconnecting = true;

  setTimeout(() => {
    console.info("Reconnecting WebSocket...");
    initWebSocket(url, onConnected, onDisconnected);
  }, RECONNECT_INTERVAL_MS);
}

function send(message: Uint8Array) {
  if (socket?.readyState === WebSocket.OPEN) {
    socket.send(message);
  } else {
    console.warn("WebSocket is not open. Message not sent.");
  }
}

function handleMessage(message: ArrayBuffer) {
  const data = new Uint8Array(message);
  const type = data[0] as WebSocketMessageType;

  const handler = webSocketHandlers.get(type);
  if (handler) {
    handler(message);
  } else {
    console.warn("Unhandled message type", type);
  }
}

export function sendColorMessage(state: [LightState, LightState, LightState, LightState]): void {
  send(encodeColorMessage(state));
}

export function sendDeviceName(name: string): void {
  send(encodeDeviceNameMessage(name));
}

export function sendHttpCredentials(username: string, password: string): void {
  send(encodeHttpCredentialsMessage({username, password}));
}

export function sendWiFiConnectionDetails(details: WiFiConnectionDetails): void {
  send(encodeWiFiConnectionDetailsMessage(details));
}

export function sendBleStatus(status: number): void {
  send(encodeBleStatusMessage(status));
}

export function sendAlexaIntegrationSettings(settings: AlexaIntegrationSettings): void {
  send(encodeAlexaIntegrationSettingsMessage(settings));
}

export function sendWiFiScanRequest(): void {
  send(encodeWiFiScanStatusMessage());
}

export function sendOtaProgressRequest(): void {
  send(encodeOtaProgressMessage());
}
