import {
  AlexaIntegrationSettings,
  encodeAlexaIntegrationSettingsMessage,
  encodeBleStatusMessage,
  encodeColorMessage,
  encodeDeviceNameMessage,
  encodeHttpCredentialsMessage,
  encodeWiFiConnectionDetailsMessage,
  encodeWiFiScanStatusMessage,
  LightState,
  WebSocketMessageType,
  WiFiConnectionDetails
} from "./model";

const AUTO_CLOSE_TIMEOUT_MS = 1500;

export const webSocketHandlers = new Map<WebSocketMessageType, (data: ArrayBuffer) => void>();

let timeoutChecker: ReturnType<typeof setInterval> | null = null;
let lastReceivedMessageTime = Date.now();
let socket: WebSocket | null = null;

export function initWebSocket(url: string, onConnected?: () => void, onDisconnected?: () => void) {
  if (!socket) {
    timeoutChecker && clearInterval(timeoutChecker);
    connectWebSocket(url, onConnected, onDisconnected);
    timeoutChecker = setInterval(() => {
      if (lastReceivedMessageTime + AUTO_CLOSE_TIMEOUT_MS < Date.now()) {
        console.warn("WebSocket connection timed out, closing socket");
        try {
          socket?.close();
        } catch {
        }
        onDisconnected?.();
        connectWebSocket(url, onConnected, onDisconnected);
      }
    }, AUTO_CLOSE_TIMEOUT_MS)
  }
}

export function disconnectWebSocket() {
  socket?.close();
  timeoutChecker && clearInterval(timeoutChecker);
  timeoutChecker = null;
}

function connectWebSocket(url: string, onConnected?: () => void, onDisconnected?: () => void) {
  socket = new WebSocket(url);
  socket.binaryType = "arraybuffer";
  lastReceivedMessageTime = Date.now();

  socket.onopen = () => {
    console.info("WebSocket connected");
    lastReceivedMessageTime = Date.now();
    onConnected?.();
  };

  socket.onmessage = (event: MessageEvent<ArrayBuffer>) => {
    lastReceivedMessageTime = Date.now();
    onConnected?.();
    handleMessage(event.data);
  }

  socket.onerror = (err) => {
    console.error("WebSocket error", err);
  };

  socket.onclose = () => {
    console.warn("WebSocket closed");
    onDisconnected?.();
  };
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
