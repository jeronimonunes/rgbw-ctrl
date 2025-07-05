import {Component, OnDestroy} from '@angular/core';
import {MatSnackBar} from '@angular/material/snack-bar';
import {CommonModule, NgOptimizedImage} from '@angular/common';
import {MatButtonModule} from '@angular/material/button';
import {MatIconModule} from '@angular/material/icon';
import {MatCardModule} from '@angular/material/card';
import {FormArray, FormControl, FormGroup, ReactiveFormsModule, ValidatorFn, Validators} from '@angular/forms';
import {
  ALEXA_MAX_DEVICE_NAME_LENGTH,
  AlexaIntegrationMode,
  BufferReader,
  decodeAlexaIntegrationSettings,
  decodeCString,
  decodeEspNowController,
  decodeEspNowDevice,
  decodeHttpCredentials,
  decodeWiFiDetails,
  decodeWiFiScanResult,
  decodeWiFiScanStatus,
  decodeWiFiStatus,
  encodeAlexaIntegrationSettings,
  encodeEspNowController,
  encodeEspNowDevices,
  encodeHttpCredentials,
  encodeWiFiConnectionDetails,
  ESP_NOW_DEVICE_NAME_MAX_LENGTH,
  ESP_NOW_MAX_DEVICES_PER_MESSAGE,
  isEnterprise,
  textEncoder,
  WiFiConnectionDetails,
  WiFiDetails,
  WiFiEncryptionType,
  WiFiNetwork,
  WiFiScanStatus,
  WiFiStatus
} from '../model';
import {MatListModule} from '@angular/material/list';
import {MatLineModule} from '@angular/material/core';
import {MatFormFieldModule} from '@angular/material/form-field';
import {MatExpansionModule} from '@angular/material/expansion';
import {MatInputModule} from '@angular/material/input';
import {MatButtonToggleModule} from '@angular/material/button-toggle';
import {NumberToIpPipe} from '../number-to-ip.pipe';
import {MatDialog, MatDialogModule} from '@angular/material/dialog';
import {LoadingComponent} from '../loading/loading.component';
import {MatTooltipModule} from '@angular/material/tooltip';
import {MatProgressSpinnerModule} from '@angular/material/progress-spinner';
import {EditDeviceNameComponentDialog} from './edit-device-name-dialog/edit-device-name-component-dialog.component';
import {firstValueFrom} from 'rxjs';
import {MatChipsModule} from '@angular/material/chips';
import {
  EnterpriseWiFiConnectDialogComponent
} from './enterprise-wi-fi-connect-dialog/enterprise-wi-fi-connect-dialog.component';
import {SimpleWiFiConnectDialogComponent} from './simple-wi-fi-connect-dialog/simple-wi-fi-connect-dialog.component';
import {CustomWiFiConnectDialogComponent} from './custom-wi-fi-connect-dialog/custom-wi-fi-connect-dialog.component';
import {MAX_HTTP_PASSWORD_LENGTH, MAX_HTTP_USERNAME_LENGTH} from '../http-credentials.model';
import {KilobytesPipe} from '../kb.pipe';
import {MatSliderModule} from '@angular/material/slider';
import {MatSlideToggleModule} from '@angular/material/slide-toggle';
import {RouterLink} from '@angular/router';
import {ColorControlComponent} from './color-control/color-control.component';

const DEVICE_DETAILS_SERVICE = "12345678-1234-1234-1234-1234567890a0";
const DEVICE_RESTART_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0000";
const DEVICE_NAME_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0001";
const FIRMWARE_VERSION_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0002";
const DEVICE_HEAP_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0003";

const HTTP_DETAILS_SERVICE = "12345678-1234-1234-1234-1234567890a1";
const HTTP_CREDENTIALS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0004";

const OUTPUT_SERVICE = "12345678-1234-1234-1234-1234567890a2";
const OUTPUT_COLOR_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0005";

const ALEXA_SERVICE = "12345678-1234-1234-1234-1234567890a3";
const ALEXA_SETTINGS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0006";

const ESP_NOW_CONTROLLER_SERVICE = "12345678-1234-1234-1234-1234567890a4";
const ESP_NOW_REMOTES_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0007";

const ESP_NOW_REMOTE_SERVICE = "12345678-1234-1234-1234-1234567890a5";
const ESP_NOW_CONTROLLER_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0008";

const WIFI_SERVICE = "12345678-1234-1234-1234-1234567890a6";
const WIFI_DETAILS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0009";
const WIFI_STATUS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0010";
const WIFI_SCAN_STATUS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0011";
const WIFI_SCAN_RESULT_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0012";

@Component({
  selector: 'app-rgbw-ctrl',
  standalone: true,
  imports: [
    CommonModule,
    MatButtonModule,
    MatIconModule,
    MatCardModule,
    MatListModule,
    MatLineModule,
    MatFormFieldModule,
    MatExpansionModule,
    MatInputModule,
    MatDialogModule,
    MatButtonToggleModule,
    MatTooltipModule,
    MatProgressSpinnerModule,
    MatSliderModule,
    NumberToIpPipe,
    ReactiveFormsModule,
    MatChipsModule,
    KilobytesPipe,
    NgOptimizedImage,
    MatSlideToggleModule,
    RouterLink,
    ColorControlComponent
  ],
  templateUrl: './rgbw-ctrl.component.html',
  styleUrls: ['./rgbw-ctrl.component.scss']
})
export class RgbwCtrlComponent implements OnDestroy {

  readonly WiFiStatus = WiFiStatus;
  readonly AlexaIntegrationMode = AlexaIntegrationMode;
  readonly ALEXA_MAX_DEVICE_NAME_LENGTH = ALEXA_MAX_DEVICE_NAME_LENGTH - 1;
  readonly MAX_HTTP_USERNAME_LENGTH = MAX_HTTP_USERNAME_LENGTH;
  readonly MAX_HTTP_PASSWORD_LENGTH = MAX_HTTP_PASSWORD_LENGTH;
  readonly ESP_NOW_DEVICE_NAME_MAX_LENGTH = ESP_NOW_DEVICE_NAME_MAX_LENGTH;
  readonly ESP_NOW_MAX_DEVICES_PER_MESSAGE = ESP_NOW_MAX_DEVICES_PER_MESSAGE;

  protected server: BluetoothRemoteGATTServer | null = null;

  protected characteristics: {
    deviceRestart?: BluetoothRemoteGATTCharacteristic,
    deviceName?: BluetoothRemoteGATTCharacteristic,
    firmwareVersion?: BluetoothRemoteGATTCharacteristic,
    httpCredentials?: BluetoothRemoteGATTCharacteristic,
    outputColor?: BluetoothRemoteGATTCharacteristic,
    deviceHeap?: BluetoothRemoteGATTCharacteristic,
    alexaSettings?: BluetoothRemoteGATTCharacteristic,
    espNowRemotes?: BluetoothRemoteGATTCharacteristic,
    espNowController?: BluetoothRemoteGATTCharacteristic,
    wifiDetails?: BluetoothRemoteGATTCharacteristic,
    wifiStatus?: BluetoothRemoteGATTCharacteristic,
    wifiScanStatus?: BluetoothRemoteGATTCharacteristic,
    wifiScanResult?: BluetoothRemoteGATTCharacteristic
  } = {};

  initialized: boolean = false;
  loadingAlexa: boolean = false;
  loadingHttpCredentials = false;
  readingEspNowDevices = false;
  readingEspNowController = false;

  firmwareVersion: string | null = null;
  deviceName: string | null = null;
  deviceHeap: number = 0;

  wifiStatus: WiFiStatus = WiFiStatus.UNKNOWN;
  wifiScanStatus: WiFiScanStatus = WiFiScanStatus.NOT_STARTED;
  wifiScanResult: WiFiNetwork[] = [];
  wifiDetails: WiFiDetails | null = null;

  alexaIntegrationForm = new FormGroup({
    integrationMode: new FormControl<AlexaIntegrationMode>(AlexaIntegrationMode.OFF, {
      nonNullable: true,
      validators: [Validators.required]
    }),
    rDeviceName: new FormControl<string>('', {
      nonNullable: true,
      validators: [Validators.required, Validators.maxLength(ALEXA_MAX_DEVICE_NAME_LENGTH)]
    }),
    gDeviceName: new FormControl<string>('', {
      nonNullable: true,
      validators: [Validators.required, Validators.maxLength(ALEXA_MAX_DEVICE_NAME_LENGTH)]
    }),
    bDeviceName: new FormControl<string>('', {
      nonNullable: true,
      validators: [Validators.required, Validators.maxLength(ALEXA_MAX_DEVICE_NAME_LENGTH)]
    }),
    wDeviceName: new FormControl<string>('', {
      nonNullable: true,
      validators: [Validators.required, Validators.maxLength(ALEXA_MAX_DEVICE_NAME_LENGTH)]
    }),
  });

  httpCredentialsForm = new FormGroup({
    username: new FormControl<string>('', {
      nonNullable: true,
      validators: [Validators.required, Validators.maxLength(MAX_HTTP_USERNAME_LENGTH)]
    }),
    password: new FormControl<string>('', {
      nonNullable: true,
      validators: [Validators.required, Validators.maxLength(MAX_HTTP_PASSWORD_LENGTH)]
    }),
  });

  espNowDevicesForm = new FormArray<FormGroup<{
    name: FormControl<string>,
    address: FormControl<string>
  }>>([], {validators: Validators.maxLength(ESP_NOW_MAX_DEVICES_PER_MESSAGE)});

  espNowControllerForm = new FormControl<string>('', {
    nonNullable: true,
    validators: [Validators.required, Validators.pattern(/^[0-9a-fA-F]{2}(:[0-9a-fA-F]{2}){5}$/)]
  });

  get scanning() {
    return this.wifiScanStatus === WiFiScanStatus.RUNNING;
  }

  get connected(): boolean {
    return this.server?.connected || false;
  }

  constructor(
    private matDialog: MatDialog,
    private snackBar: MatSnackBar
  ) {
  }

  ngOnDestroy() {
    this.disconnect();
  }

  disconnect() {
    this.initialized = false;
    this.server?.disconnect();
    this.server = null;

    // Clear GATT characteristics
    this.characteristics = {};

    // Clear local UI state
    this.deviceName = null;
    this.firmwareVersion = null;
    this.wifiDetails = null;
    this.wifiScanStatus = WiFiScanStatus.NOT_STARTED;
    this.wifiStatus = WiFiStatus.UNKNOWN;
    this.wifiScanResult = [];

    this.loadingAlexa = false;
    this.loadingHttpCredentials = false;
    this.readingEspNowDevices = false;

    this.alexaIntegrationForm.reset();
  }

  async connectBleDevice() {
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true})
    try {
      const device = await navigator.bluetooth.requestDevice({
        filters: [
          {
            manufacturerData: [{
              companyIdentifier: 54321
            }]
          }
        ],
        optionalServices: [DEVICE_DETAILS_SERVICE, HTTP_DETAILS_SERVICE, OUTPUT_SERVICE, ALEXA_SERVICE, ESP_NOW_CONTROLLER_SERVICE, ESP_NOW_REMOTE_SERVICE, WIFI_SERVICE]
      });

      if (!device.gatt) {
        this.snackBar.open('Device does not support GATT', 'Close', {duration: 3000});
        return;
      }

      this.server = await device.gatt.connect();

      await this.initBleWiFiServices();
      await this.initHttpDetailsService();
      await this.initEspNowServices();
      await this.initBleOutputServices();
      await this.initBleAlexaServices();
      await this.initBleDeviceDetailsServices();

      device.addEventListener('gattserverdisconnected', () => {
        this.server = null;
        this.disconnect();
        this.snackBar.open('Device disconnected', 'Reconnect', {duration: 3000})
          .onAction().subscribe(() => this.connectBleDevice());
      });

      await this.readDeviceName();
      await this.readFirmwareVersion();
      await this.readHttpCredentials();
      await this.readWiFiStatus();
      await this.readWiFiDetails();
      await this.readWiFiScanStatus();
      await this.readWiFiScanResult();
      await this.readAlexaIntegration();
      await this.readEspNowDevices();
      await this.readEspNowController();
      this.snackBar.open('Device connected', 'Close', {duration: 3000});
      this.initialized = true;
      if (this.wifiScanResult.length === 0) {
        await this.startWifiScan();
      }
    } catch (error) {
      console.error(error);
      this.disconnect();
      this.snackBar.open('Failed to connect to device', 'Close', {duration: 3000});
    } finally {
      loading.close();
    }
  }

  async startWifiScan() {
    try {
      await this.characteristics.wifiScanStatus!.writeValue(new Uint8Array([0]));
      this.snackBar.open('WiFi scan started', 'Close', {duration: 3000});
    } catch (e) {
      console.error(e);
      this.snackBar.open('Failed to start WiFi scan', 'Close', {duration: 3000});
    }
  }

  async restartDevice() {
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    try {
      await this.characteristics.deviceRestart!.writeValue(textEncoder.encode("RESTART_NOW"));
    } catch (e) {
      console.error('Failed to restart device:', e);
      this.snackBar.open('Failed to restart device', 'Close', {duration: 3000});
    } finally {
      this.disconnect();
      loading.close();
    }
  }

  async connectToNetwork(network: WiFiNetwork) {
    let details: Uint8Array;
    if (network.encryptionType === WiFiEncryptionType.WIFI_AUTH_OPEN) {
      const value: WiFiConnectionDetails = {
        encryptionType: network.encryptionType,
        ssid: network.ssid,
        credentials: {password: ''}
      };
      details = encodeWiFiConnectionDetails(value);
    } else if (isEnterprise(network.encryptionType)) {
      let value = await firstValueFrom(this.matDialog.open(EnterpriseWiFiConnectDialogComponent, {data: network}).afterClosed());
      if (!value) return;
      details = encodeWiFiConnectionDetails(value);
    } else {
      let value = await firstValueFrom(this.matDialog.open(SimpleWiFiConnectDialogComponent, {data: network}).afterClosed());
      if (!value) return;
      details = encodeWiFiConnectionDetails(value);
    }
    await this.sendWifiConfig(details);
  }

  async connectToCustomNetwork() {
    const data: WiFiConnectionDetails = await firstValueFrom(this.matDialog.open(CustomWiFiConnectDialogComponent).afterClosed());
    if (!data) return;
    let details: Uint8Array;
    if (isEnterprise(data.encryptionType)) {
      let value = await firstValueFrom(this.matDialog.open(EnterpriseWiFiConnectDialogComponent, {data}).afterClosed());
      if (!value) return;
      details = encodeWiFiConnectionDetails(value);
    } else {
      let value = await firstValueFrom(this.matDialog.open(SimpleWiFiConnectDialogComponent, {data}).afterClosed());
      if (!value) return;
      details = encodeWiFiConnectionDetails(value);
    }
    await this.sendWifiConfig(details);
  }

  async editDeviceName() {
    const deviceName = await firstValueFrom(this.matDialog.open(EditDeviceNameComponentDialog, {data: this.deviceName}).afterClosed())
    if (deviceName) {
      try {
        const encodedName = textEncoder.encode(deviceName);
        await this.characteristics.deviceName!.writeValue(encodedName);
        this.snackBar.open('Device name updated', 'Close', {duration: 3000});
      } catch (e) {
        console.error('Failed to update device name:', e);
        this.snackBar.open('Failed to update device name', 'Close', {duration: 3000});
      }
    }
  }

  async loadAlexaIntegrationSettings() {
    this.loadingAlexa = true;
    await this.readAlexaIntegration();
    this.loadingAlexa = false;
  }

  async applyAlexaIntegrationSettings() {
    if (this.alexaIntegrationForm.invalid || !this.connected) {
      return;
    }
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    // Encode the form value and write via BLE
    const settings = this.alexaIntegrationForm.value;
    const payload = encodeAlexaIntegrationSettings(settings);
    try {
      await this.characteristics.alexaSettings!.writeValue(payload);
      this.snackBar.open('Alexa settings updated', 'Close', {duration: 2500});
    } catch (e) {
      console.log('Failed to update Alexa settings:', e);
      this.snackBar.open('Failed to update Alexa settings', 'Close', {duration: 3000});
    } finally {
      loading.close();
    }
  }

  async loadHttpCredentials() {
    this.loadingHttpCredentials = true;
    await this.readHttpCredentials();
    this.loadingHttpCredentials = false;
  }

  async applyHttpCredentials() {
    if (this.httpCredentialsForm.invalid || !this.connected) {
      return;
    }
    const credentials = this.httpCredentialsForm.getRawValue();
    const payload = encodeHttpCredentials(credentials);
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    try {
      await this.characteristics.httpCredentials!.writeValue(payload);
      this.snackBar.open('HTTP credentials updated', 'Close', {duration: 3000});
    } catch (e) {
      console.error('Failed to update HTTP credentials:', e);
      this.snackBar.open('Failed to update HTTP credentials', 'Close', {duration: 3000});
    } finally {
      loading.close();
    }
  }

  addEspNowDeviceFormEntry() {
    const formGroup = new FormGroup({
      name: new FormControl<string>('', {
        nonNullable: true,
        validators: [
          Validators.required,
          Validators.maxLength(ESP_NOW_DEVICE_NAME_MAX_LENGTH),
          this.unique('name')
        ]
      }),
      address: new FormControl<string>('', {
        nonNullable: true,
        validators: [
          Validators.required,
          Validators.pattern(/^[0-9a-fA-F]{2}(:[0-9a-fA-F]{2}){5}$/),
          this.unique('address')
        ]
      })
    });
    this.espNowDevicesForm.push(formGroup, {emitEvent: false});
    this.espNowDevicesForm.updateValueAndValidity({emitEvent: false})
  }

  removeEspNowDevice(i: number) {
    this.espNowDevicesForm.removeAt(i);
    this.espNowDevicesForm.markAsDirty();
    this.espNowDevicesForm.markAsTouched();
  }

  unique(controlField: 'address' | 'name'): ValidatorFn {
    return c => {
      for (let i = 0; i < this.espNowDevicesForm.length; i++) {
        const group = this.espNowDevicesForm.at(i);
        if (c.parent !== group && group.value[controlField] === c.value) {
          return {unique: {value: c.value}};
        }
      }
      return null;
    }
  }

  async saveEspNowController() {
    const loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    try {
      const value = this.espNowControllerForm.getRawValue();
      const buffer = encodeEspNowController(value);
      await this.characteristics.espNowController!.writeValue(buffer);
    } catch (e) {
      console.error('Failed to update ESP-NOW controller:', e);
      this.snackBar.open('Failed to update ESP-NOW controller', 'Close', {duration: 3000});
    } finally {
      loading.close();
    }
  }

  async saveEspNowDevices() {
    const loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    try {
      const value = this.espNowDevicesForm.getRawValue();
      const buffer = encodeEspNowDevices(value);
      await this.characteristics.espNowRemotes!.writeValue(buffer);
    } catch (e) {
      console.error('Failed to update ESP-NOW devices:', e);
      this.snackBar.open('Failed to update ESP-NOW devices', 'Close', {duration: 3000});
    } finally {
      loading.close();
    }
  }

  private async initBleWiFiServices() {
    const wifiService = await this.server!.getPrimaryService(WIFI_SERVICE);

    this.characteristics.wifiDetails = await wifiService.getCharacteristic(WIFI_DETAILS_CHARACTERISTIC);
    this.characteristics.wifiDetails.addEventListener('characteristicvaluechanged', (ev: any) => this.wifiDetailsChanged(ev.target.value));
    await this.characteristics.wifiDetails.startNotifications();

    this.characteristics.wifiStatus = await wifiService.getCharacteristic(WIFI_STATUS_CHARACTERISTIC);
    this.characteristics.wifiStatus.addEventListener('characteristicvaluechanged', (ev: any) => this.wifiStatusChanged(ev.target.value));
    await this.characteristics.wifiStatus.startNotifications();

    this.characteristics.wifiScanStatus = await wifiService.getCharacteristic(WIFI_SCAN_STATUS_CHARACTERISTIC);
    this.characteristics.wifiScanStatus.addEventListener('characteristicvaluechanged', (ev: any) => this.wifiScanStatusChanged(ev.target.value));
    await this.characteristics.wifiScanStatus.startNotifications();

    this.characteristics.wifiScanResult = await wifiService.getCharacteristic(WIFI_SCAN_RESULT_CHARACTERISTIC);
    this.characteristics.wifiScanResult.addEventListener('characteristicvaluechanged', (ev: any) => this.wifiScanResultChanged(ev.target.value));
    await this.characteristics.wifiScanResult.startNotifications();
  }

  private async initBleDeviceDetailsServices() {
    const deviceDetailsService = await this.server!.getPrimaryService(DEVICE_DETAILS_SERVICE);

    this.characteristics.deviceRestart = await deviceDetailsService.getCharacteristic(DEVICE_RESTART_CHARACTERISTIC);
    this.characteristics.firmwareVersion = await deviceDetailsService.getCharacteristic(FIRMWARE_VERSION_CHARACTERISTIC);

    this.characteristics.deviceName = await deviceDetailsService.getCharacteristic(DEVICE_NAME_CHARACTERISTIC);
    this.characteristics.deviceName.addEventListener('characteristicvaluechanged', (ev: any) => this.deviceNameChanged(ev.target.value));
    await this.characteristics.deviceName.startNotifications();

    this.characteristics.deviceHeap = await deviceDetailsService.getCharacteristic(DEVICE_HEAP_CHARACTERISTIC);
    this.characteristics.deviceHeap.addEventListener('characteristicvaluechanged', (ev: any) => this.deviceHeapChanged(ev.target.value));
    await this.characteristics.deviceHeap.startNotifications();
  }

  private async initHttpDetailsService() {
    const httpDetailsService = await this.server!.getPrimaryService(HTTP_DETAILS_SERVICE);
    this.characteristics.httpCredentials = await httpDetailsService.getCharacteristic(HTTP_CREDENTIALS_CHARACTERISTIC);
  }

  private async initBleOutputServices() {
    try {
      const service = await this.server!.getPrimaryService(OUTPUT_SERVICE);
      this.characteristics.outputColor = await service.getCharacteristic(OUTPUT_COLOR_CHARACTERISTIC);
    } catch (e) {
      // This device does not support output color service
    }
  }

  private async initBleAlexaServices() {
    try {
      const service = await this.server!.getPrimaryService(ALEXA_SERVICE);
      this.characteristics.alexaSettings = await service.getCharacteristic(ALEXA_SETTINGS_CHARACTERISTIC);
    } catch (e) {
      // This device does not support Alexa integration service
    }
  }

  private async initEspNowServices() {
    try {
      const service = await this.server!.getPrimaryService(ESP_NOW_CONTROLLER_SERVICE);
      this.characteristics.espNowRemotes = await service.getCharacteristic(ESP_NOW_REMOTES_CHARACTERISTIC);
    } catch (e) {
      // This device does not support ESP-NOW remotes service
    }
    try {
      const service = await this.server!.getPrimaryService(ESP_NOW_REMOTE_SERVICE);
      this.characteristics.espNowController = await service.getCharacteristic(ESP_NOW_CONTROLLER_CHARACTERISTIC);
    } catch (e) {
      // This device does not support ESP-NOW controller service:
    }
  }

  private wifiStatusChanged(view: DataView) {
    const buffer = new Uint8Array(view.buffer);
    this.wifiStatus = decodeWiFiStatus(buffer);
  }

  private wifiDetailsChanged(view: DataView) {
    const buffer = new Uint8Array(view.buffer);
    this.wifiDetails = decodeWiFiDetails(new BufferReader(buffer));
  }

  private wifiScanStatusChanged(view: DataView) {
    this.wifiScanStatus = decodeWiFiScanStatus(new Uint8Array(view.buffer));
    if (this.wifiScanStatus === WiFiScanStatus.FAILED) {
      this.snackBar.open(`WiFi scan failed!`, 'Close', {duration: 3000});
    }
  }

  private wifiScanResultChanged(view: DataView) {
    const buffer = new Uint8Array(view.buffer);
    this.wifiScanResult = decodeWiFiScanResult(buffer);
  }

  private deviceNameChanged(view: DataView) {
    const buffer = new Uint8Array(view.buffer);
    this.deviceName = decodeCString(buffer);
  }

  private deviceHeapChanged(view: DataView) {
    this.deviceHeap = view.getUint32(0, true);
  }

  private espNowControllerChanged(view: DataView) {
    const {address} = decodeEspNowController(new Uint8Array(view.buffer));
    this.espNowControllerForm.reset(address);
  }

  private espNowDevicesChanged(view: DataView) {
    const devices = decodeEspNowDevice(new Uint8Array(view.buffer));
    while (this.espNowDevicesForm.length > devices.length) {
      this.espNowDevicesForm.removeAt(0);
    }
    while (this.espNowDevicesForm.length < devices.length) {
      this.addEspNowDeviceFormEntry();
    }
    this.espNowDevicesForm.reset(devices, {emitEvent: false});
  }

  private httpCredentialsChanged(view: DataView) {
    const credentials = decodeHttpCredentials(new Uint8Array(view.buffer));
    this.httpCredentialsForm.reset(credentials);
  }

  private async readHttpCredentials() {
    const value = await this.characteristics.httpCredentials!.readValue();
    this.httpCredentialsChanged(value);
  }

  private async readDeviceName() {
    const view = await this.characteristics.deviceName!.readValue();
    this.deviceNameChanged(view);
  }

  private async readFirmwareVersion() {
    const view = await this.characteristics.firmwareVersion!.readValue();
    this.firmwareVersion = decodeCString(new Uint8Array(view.buffer));
  }

  private async readWiFiStatus() {
    const view = await this.characteristics.wifiStatus!.readValue();
    this.wifiStatusChanged(view);
  }

  private async readWiFiDetails() {
    const view = await this.characteristics.wifiDetails!.readValue();
    this.wifiDetailsChanged(view);
  }

  private async readWiFiScanStatus() {
    const view = await this.characteristics.wifiScanStatus!.readValue();
    this.wifiScanStatusChanged(view);
  }

  private async readWiFiScanResult() {
    const view = await this.characteristics.wifiScanResult!.readValue();
    this.wifiScanResultChanged(view);
  }

  private async readAlexaIntegration() {
    if (!this.characteristics.alexaSettings) return;
    const view = await this.characteristics.alexaSettings.readValue();
    const alexaDetails = decodeAlexaIntegrationSettings(new Uint8Array(view.buffer));
    this.resetAlexaIntegrationForm(alexaDetails.integrationMode);
    this.alexaIntegrationForm.reset(alexaDetails, {emitEvent: true});
  }

  async readEspNowController() {
    if (!this.characteristics.espNowController) return;
    this.readingEspNowController = true;
    try {
      const view = await this.characteristics.espNowController.readValue();
      this.espNowControllerChanged(view);
    } finally {
      this.readingEspNowController = false;
    }
  }

  async readEspNowDevices() {
    if (!this.characteristics.espNowRemotes) return;
    this.readingEspNowDevices = true;
    try {
      const view = await this.characteristics.espNowRemotes.readValue();
      this.espNowDevicesChanged(view);
    } finally {
      this.readingEspNowDevices = false;
    }
  }

  private async sendWifiConfig(details: Uint8Array) {
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    try {
      await this.characteristics.wifiStatus!.writeValue(details);
      this.snackBar.open('WiFi configuration sent', 'Close', {duration: 3000});
    } catch (e) {
      console.error('Failed to send WiFi configuration:', e);
      this.snackBar.open('Failed to send WiFi configuration', 'Close', {duration: 3000});
    } finally {
      loading.close();
    }
  }

  toggleDeviceEnabled(controlName: string) {
    const control = this.alexaIntegrationForm.get(controlName);
    if (!control) return;
    if (control.disabled) {
      control.enable();
    } else {
      control.setValue('');
      control.disable();
    }
  }

  resetAlexaIntegrationForm(integrationMode: AlexaIntegrationMode) {
    const controls = this.alexaIntegrationForm.controls;
    this.alexaIntegrationForm.reset({integrationMode});
    switch (integrationMode) {
      case AlexaIntegrationMode.OFF:
        controls.rDeviceName.disable();
        controls.gDeviceName.disable();
        controls.bDeviceName.disable();
        controls.wDeviceName.disable();
        break;
      case AlexaIntegrationMode.RGBW_DEVICE:
        controls.rDeviceName.enable();
        controls.gDeviceName.disable();
        controls.bDeviceName.disable();
        controls.wDeviceName.disable();
        break;
      case AlexaIntegrationMode.RGB_DEVICE:
        controls.rDeviceName.enable();
        controls.gDeviceName.disable();
        controls.bDeviceName.disable();
        controls.wDeviceName.enable();
        break;
      case AlexaIntegrationMode.MULTI_DEVICE:
        controls.rDeviceName.enable();
        controls.gDeviceName.enable();
        controls.bDeviceName.enable();
        controls.wDeviceName.enable();
        break;
    }
  }
}
