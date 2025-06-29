import { Injectable } from '@angular/core';
import { BehaviorSubject } from 'rxjs';
import {
  AlexaIntegrationSettings,
  decodeAlexaIntegrationSettings,
  decodeCString,
  decodeEspNowDevice,
  decodeHttpCredentials,
  decodeOutputState,
  decodeWiFiDetails,
  decodeWiFiScanResult,
  decodeWiFiScanStatus,
  decodeWiFiStatus,
  encodeAlexaIntegrationSettings,
  encodeEspNowMessage,
  encodeHttpCredentials,
  encodeOutputState,
  encodeWiFiConnectionDetails,
  textEncoder,
  WiFiConnectionDetails,
  WiFiDetails,
  WiFiNetwork,
  WiFiScanStatus,
  WiFiStatus
} from '../model';

const BLE_NAME = 'rgbw-ctrl';

const DEVICE_DETAILS_SERVICE = '12345678-1234-1234-1234-1234567890ac';
const DEVICE_RESTART_CHARACTERISTIC = 'aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0000';
const DEVICE_NAME_CHARACTERISTIC = 'aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0001';
const FIRMWARE_VERSION_CHARACTERISTIC = 'aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0002';
const HTTP_CREDENTIALS_CHARACTERISTIC = 'aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0003';
const DEVICE_HEAP_CHARACTERISTIC = 'aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0004';
const OUTPUT_COLOR_CHARACTERISTIC = 'aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0005';
const ALEXA_SETTINGS_CHARACTERISTIC = 'aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0006';
const ESP_NOW_DEVICES_CHARACTERISTIC = 'aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0007';

const WIFI_SERVICE = '12345678-1234-1234-1234-1234567890ab';
const WIFI_DETAILS_CHARACTERISTIC = 'aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0008';
const WIFI_STATUS_CHARACTERISTIC = 'aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0009';
const WIFI_SCAN_STATUS_CHARACTERISTIC = 'aaaaaaaa-bbbb-cccc-dddd-eeeeeeee000a';
const WIFI_SCAN_RESULT_CHARACTERISTIC = 'aaaaaaaa-bbbb-cccc-dddd-eeeeeeee000b';

@Injectable({
  providedIn: 'root'
})
export class BluetoothService {
  private server: BluetoothRemoteGATTServer | null = null;

  private deviceRestartCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private deviceNameCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private firmwareVersionCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private httpCredentialsCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private deviceHeapCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private outputColorCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private alexaSettingsCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private espNowDevicesCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;

  private wifiDetailsCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private wifiStatusCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private wifiScanStatusCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;
  private wifiScanResultCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;

  private initializedSubject = new BehaviorSubject<boolean>(false);
  initialized$ = this.initializedSubject.asObservable();
  private connectedSubject = new BehaviorSubject<boolean>(false);
  connected$ = this.connectedSubject.asObservable();

  private firmwareVersionSubject = new BehaviorSubject<string | null>(null);
  firmwareVersion$ = this.firmwareVersionSubject.asObservable();
  private deviceNameSubject = new BehaviorSubject<string | null>(null);
  deviceName$ = this.deviceNameSubject.asObservable();
  private deviceHeapSubject = new BehaviorSubject<number>(0);
  deviceHeap$ = this.deviceHeapSubject.asObservable();

  private wifiStatusSubject = new BehaviorSubject<WiFiStatus>(WiFiStatus.UNKNOWN);
  wifiStatus$ = this.wifiStatusSubject.asObservable();
  private wifiScanStatusSubject = new BehaviorSubject<WiFiScanStatus>(WiFiScanStatus.NOT_STARTED);
  wifiScanStatus$ = this.wifiScanStatusSubject.asObservable();
  private wifiScanResultSubject = new BehaviorSubject<WiFiNetwork[]>([]);
  wifiScanResult$ = this.wifiScanResultSubject.asObservable();
  private wifiDetailsSubject = new BehaviorSubject<WiFiDetails | null>(null);
  wifiDetails$ = this.wifiDetailsSubject.asObservable();

  private alexaSettingsSubject = new BehaviorSubject<AlexaIntegrationSettings | null>(null);
  alexaSettings$ = this.alexaSettingsSubject.asObservable();
  private httpCredentialsSubject = new BehaviorSubject<{username: string, password: string} | null>(null);
  httpCredentials$ = this.httpCredentialsSubject.asObservable();
  private outputColorSubject = new BehaviorSubject<ReturnType<typeof decodeOutputState> | null>(null);
  outputColor$ = this.outputColorSubject.asObservable();
  private espNowDevicesSubject = new BehaviorSubject<{name: string, address: string}[]>([]);
  espNowDevices$ = this.espNowDevicesSubject.asObservable();

  async connect(): Promise<void> {
    const device = await navigator.bluetooth.requestDevice({
      filters: [{ namePrefix: BLE_NAME }],
      optionalServices: [WIFI_SERVICE, DEVICE_DETAILS_SERVICE]
    });
    if (!device.gatt) throw new Error('Device does not support GATT');
    this.server = await device.gatt.connect();
    await this.initBleWiFiServices(this.server);
    await this.initBleDeviceDetailsServices(this.server);
    device.addEventListener('gattserverdisconnected', () => {
      this.disconnect();
    });
    await this.readDeviceName();
    await this.readFirmwareVersion();
    await this.readHttpCredentials();
    await this.readWiFiStatus();
    await this.readWiFiDetails();
    await this.readWiFiScanStatus();
    await this.readWiFiScanResult();
    await this.readAlexaIntegration();
    await this.readOutputColor();
    await this.readEspNowDevices();
    this.connectedSubject.next(true);
    this.initializedSubject.next(true);
  }

  disconnect(): void {
    this.initializedSubject.next(false);
    this.connectedSubject.next(false);
    this.server?.disconnect();
    this.server = null;
    this.deviceRestartCharacteristic = null;
    this.deviceNameCharacteristic = null;
    this.firmwareVersionCharacteristic = null;
    this.outputColorCharacteristic = null;
    this.alexaSettingsCharacteristic = null;
    this.espNowDevicesCharacteristic = null;
    this.wifiDetailsCharacteristic = null;
    this.wifiStatusCharacteristic = null;
    this.wifiScanStatusCharacteristic = null;
    this.wifiScanResultCharacteristic = null;
    this.deviceNameSubject.next(null);
    this.firmwareVersionSubject.next(null);
    this.wifiDetailsSubject.next(null);
    this.wifiScanStatusSubject.next(WiFiScanStatus.NOT_STARTED);
    this.wifiStatusSubject.next(WiFiStatus.UNKNOWN);
    this.wifiScanResultSubject.next([]);
  }

  async startWifiScan(): Promise<void> {
    if (!this.wifiScanStatusCharacteristic) return;
    await this.wifiScanStatusCharacteristic.writeValue(new Uint8Array([0]));
  }

  async restartDevice(): Promise<void> {
    await this.deviceRestartCharacteristic?.writeValue(textEncoder.encode('RESTART_NOW'));
  }

  async applyAlexaIntegration(settings: AlexaIntegrationSettings): Promise<void> {
    const payload = encodeAlexaIntegrationSettings(settings);
    await this.alexaSettingsCharacteristic?.writeValue(payload);
  }

  async applyHttpCredentials(credentials: {username: string, password: string}): Promise<void> {
    const payload = encodeHttpCredentials(credentials);
    await this.httpCredentialsCharacteristic?.writeValue(payload);
  }

  async setDeviceName(name: string): Promise<void> {
    const data = textEncoder.encode(name);
    await this.deviceNameCharacteristic?.writeValue(data);
  }

  async sendWifiConfig(details: WiFiConnectionDetails): Promise<void> {
    const payload = encodeWiFiConnectionDetails(details);
    await this.wifiStatusCharacteristic?.writeValue(payload);
  }

  async saveEspNowDevices(devices: {name: string, address: string}[]): Promise<void> {
    const buffer = encodeEspNowMessage(devices);
    await this.espNowDevicesCharacteristic?.writeValue(buffer);
  }

  async setOutputColor(state: Parameters<typeof encodeOutputState>[0]): Promise<void> {
    const buffer = encodeOutputState(state);
    await this.outputColorCharacteristic?.writeValue(buffer);
  }

  private async initBleWiFiServices(server: BluetoothRemoteGATTServer) {
    const wifiService = await server.getPrimaryService(WIFI_SERVICE);

    this.wifiDetailsCharacteristic = await wifiService.getCharacteristic(WIFI_DETAILS_CHARACTERISTIC);
    this.wifiDetailsCharacteristic.addEventListener('characteristicvaluechanged', (ev: any) => this.wifiDetailsChanged(ev.target.value));
    await this.wifiDetailsCharacteristic.startNotifications();

    this.wifiStatusCharacteristic = await wifiService.getCharacteristic(WIFI_STATUS_CHARACTERISTIC);
    this.wifiStatusCharacteristic.addEventListener('characteristicvaluechanged', (ev: any) => this.wifiStatusChanged(ev.target.value));
    await this.wifiStatusCharacteristic.startNotifications();

    this.wifiScanStatusCharacteristic = await wifiService.getCharacteristic(WIFI_SCAN_STATUS_CHARACTERISTIC);
    this.wifiScanStatusCharacteristic.addEventListener('characteristicvaluechanged', (ev: any) => this.wifiScanStatusChanged(ev.target.value));
    await this.wifiScanStatusCharacteristic.startNotifications();

    this.wifiScanResultCharacteristic = await wifiService.getCharacteristic(WIFI_SCAN_RESULT_CHARACTERISTIC);
    this.wifiScanResultCharacteristic.addEventListener('characteristicvaluechanged', (ev: any) => this.wifiScanResultChanged(ev.target.value));
    await this.wifiScanResultCharacteristic.startNotifications();
  }

  private async initBleDeviceDetailsServices(server: BluetoothRemoteGATTServer) {
    const deviceDetailsService = await server.getPrimaryService(DEVICE_DETAILS_SERVICE);

    this.deviceRestartCharacteristic = await deviceDetailsService.getCharacteristic(DEVICE_RESTART_CHARACTERISTIC);
    this.firmwareVersionCharacteristic = await deviceDetailsService.getCharacteristic(FIRMWARE_VERSION_CHARACTERISTIC);
    this.httpCredentialsCharacteristic = await deviceDetailsService.getCharacteristic(HTTP_CREDENTIALS_CHARACTERISTIC);

    this.deviceNameCharacteristic = await deviceDetailsService.getCharacteristic(DEVICE_NAME_CHARACTERISTIC);
    this.deviceNameCharacteristic.addEventListener('characteristicvaluechanged', (ev: any) => this.deviceNameChanged(ev.target.value));
    await this.deviceNameCharacteristic.startNotifications();

    this.deviceHeapCharacteristic = await deviceDetailsService.getCharacteristic(DEVICE_HEAP_CHARACTERISTIC);
    this.deviceHeapCharacteristic.addEventListener('characteristicvaluechanged', (ev: any) => this.deviceHeapChanged(ev.target.value));
    await this.deviceHeapCharacteristic.startNotifications();

    this.outputColorCharacteristic = await deviceDetailsService.getCharacteristic(OUTPUT_COLOR_CHARACTERISTIC);
    this.outputColorCharacteristic.addEventListener('characteristicvaluechanged', (ev: any) => this.outputColorChanged(ev.target.value));
    await this.outputColorCharacteristic.startNotifications();

    this.alexaSettingsCharacteristic = await deviceDetailsService.getCharacteristic(ALEXA_SETTINGS_CHARACTERISTIC);
    this.espNowDevicesCharacteristic = await deviceDetailsService.getCharacteristic(ESP_NOW_DEVICES_CHARACTERISTIC);
  }

  private wifiStatusChanged(view: DataView) {
    const buffer = new Uint8Array(view.buffer);
    this.wifiStatusSubject.next(decodeWiFiStatus(buffer));
  }

  private wifiDetailsChanged(view: DataView) {
    const buffer = new Uint8Array(view.buffer);
    this.wifiDetailsSubject.next(decodeWiFiDetails(new BufferReader(buffer)));
  }

  private wifiScanStatusChanged(view: DataView) {
    const status = decodeWiFiScanStatus(new Uint8Array(view.buffer));
    this.wifiScanStatusSubject.next(status);
  }

  private wifiScanResultChanged(view: DataView) {
    const buffer = new Uint8Array(view.buffer);
    this.wifiScanResultSubject.next(decodeWiFiScanResult(buffer));
  }

  private deviceNameChanged(view: DataView) {
    const buffer = new Uint8Array(view.buffer);
    this.deviceNameSubject.next(decodeCString(buffer));
  }

  private deviceHeapChanged(view: DataView) {
    this.deviceHeapSubject.next(view.getUint32(0, true));
  }

  private outputColorChanged(view: DataView) {
    const buffer = new Uint8Array(view.buffer);
    this.outputColorSubject.next(decodeOutputState(buffer));
  }

  private espNowDevicesChanged(view: DataView) {
    const devices = decodeEspNowDevice(new Uint8Array(view.buffer));
    this.espNowDevicesSubject.next(devices);
  }

  private httpCredentialsChanged(view: DataView) {
    const credentials = decodeHttpCredentials(new Uint8Array(view.buffer));
    this.httpCredentialsSubject.next(credentials);
  }

  async readHttpCredentials() {
    const value = await this.httpCredentialsCharacteristic!.readValue();
    this.httpCredentialsChanged(value);
  }

  private async readDeviceName() {
    const view = await this.deviceNameCharacteristic!.readValue();
    this.deviceNameChanged(view);
  }

  private async readFirmwareVersion() {
    const view = await this.firmwareVersionCharacteristic!.readValue();
    this.firmwareVersionSubject.next(decodeCString(new Uint8Array(view.buffer)));
  }

  async readWiFiStatus() {
    const view = await this.wifiStatusCharacteristic!.readValue();
    this.wifiStatusChanged(view);
  }

  async readWiFiDetails() {
    const view = await this.wifiDetailsCharacteristic!.readValue();
    this.wifiDetailsChanged(view);
  }

  async readWiFiScanStatus() {
    const view = await this.wifiScanStatusCharacteristic!.readValue();
    this.wifiScanStatusChanged(view);
  }

  async readWiFiScanResult() {
    const view = await this.wifiScanResultCharacteristic!.readValue();
    this.wifiScanResultChanged(view);
  }

  async readAlexaIntegration() {
    const view = await this.alexaSettingsCharacteristic!.readValue();
    const alexaDetails = decodeAlexaIntegrationSettings(new Uint8Array(view.buffer));
    this.alexaSettingsSubject.next(alexaDetails);
  }

  async readOutputColor() {
    const view = await this.outputColorCharacteristic!.readValue();
    this.outputColorChanged(view);
  }

  async readEspNowDevices() {
    const view = await this.espNowDevicesCharacteristic!.readValue();
    this.espNowDevicesChanged(view);
  }
}

