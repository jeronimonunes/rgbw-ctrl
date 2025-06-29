import {AlexaIntegrationSettings} from "./alexa-integration-settings.model";
import {HttpCredentials} from "../http-credentials.model";
import {WiFiDetails, WiFiConnectionDetails, WiFiScanStatus, WiFiStatus} from "./wifi.model";
import {BleStatus} from './ble.model';
import {LightState} from './light.model';
import {OtaState} from './ota.model';
import {EspNowDevice} from "./esp-now.model";

export const WEB_SOCKET_MESSAGE_TYPE_BYTE_SIZE = 1;

export enum WebSocketMessageType {
  ON_HEAP,
  ON_DEVICE_NAME,
  ON_FIRMWARE_VERSION,
  ON_COLOR,
  ON_HTTP_CREDENTIALS,
  ON_BLE_STATUS,
  ON_WIFI_STATUS,
  ON_WIFI_SCAN_STATUS,
  ON_WIFI_DETAILS,
  ON_WIFI_CONNECTION_DETAILS,
  ON_OTA_PROGRESS,
  ON_ALEXA_INTEGRATION_SETTINGS,
  ON_ESP_NOW_DEVICES,
}

export interface WebSocketColorMessage {
  type: WebSocketMessageType.ON_COLOR;
  values: [LightState, LightState, LightState, LightState];
}

export interface WebSocketHttpCredentialsMessage {
  type: WebSocketMessageType.ON_HTTP_CREDENTIALS;
  credentials: HttpCredentials;
}

export interface WebSocketDeviceNameMessage {
  type: WebSocketMessageType.ON_DEVICE_NAME;
  deviceName: string;
}

export interface WebSocketWiFiConnectionDetailsMessage {
  type: WebSocketMessageType.ON_WIFI_DETAILS;
  details: WiFiConnectionDetails;
}

export interface WebSocketBleStatusMessage {
  type: WebSocketMessageType.ON_BLE_STATUS;
  status: BleStatus;
}

export interface WebSocketAlexaIntegrationSettingsMessage {
  type: WebSocketMessageType.ON_ALEXA_INTEGRATION_SETTINGS;
  settings: AlexaIntegrationSettings;
}

export interface WebSocketHeapInfoMessage {
  type: WebSocketMessageType.ON_HEAP;
  freeHeap: number;
}

export interface WebSocketWiFiStatusMessage {
  type: WebSocketMessageType.ON_WIFI_STATUS;
  status: WiFiStatus
}

export interface WebSocketWiFiScanStatusMessage {
  type: WebSocketMessageType.ON_WIFI_SCAN_STATUS;
  status: WiFiScanStatus;
}

export interface WebSocketOtaProgressMessage extends OtaState {
  type: WebSocketMessageType.ON_OTA_PROGRESS;
}

export interface WebSocketEspNowDevicesMessage {
  type: WebSocketMessageType.ON_ESP_NOW_DEVICES;
  devices: EspNowDevice[];
}

export interface WebSocketFirmwareVersionMessage {
  type: WebSocketMessageType.ON_FIRMWARE_VERSION;
  firmwareVersion: string;
}

export interface WebSocketWiFiDetailsMessage {
  type: WebSocketMessageType.ON_WIFI_DETAILS;
  details: WiFiDetails
}

export interface WebSocketWiFiStatusMessage {
  type: WebSocketMessageType.ON_WIFI_STATUS;
  status: WiFiStatus;
}

export interface WebSocketAlexaIntegrationSettingsMessage {
  type: WebSocketMessageType.ON_ALEXA_INTEGRATION_SETTINGS;
  settings: AlexaIntegrationSettings;
}

export type WebSocketMessage =
  | WebSocketColorMessage
  | WebSocketHttpCredentialsMessage
  | WebSocketDeviceNameMessage
  | WebSocketWiFiConnectionDetailsMessage
  | WebSocketBleStatusMessage
  | WebSocketAlexaIntegrationSettingsMessage
  | WebSocketHeapInfoMessage
  | WebSocketWiFiStatusMessage
  | WebSocketWiFiScanStatusMessage
  | WebSocketOtaProgressMessage
  | WebSocketEspNowDevicesMessage
  | WebSocketFirmwareVersionMessage
  | WebSocketWiFiDetailsMessage;
