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
import {asyncScheduler, firstValueFrom, Subscription, throttleTime} from 'rxjs';
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
import {inversePerceptualMap, perceptualMap} from '../color-utils';

const BLE_NAME = "rgbw-ctrl";

const DEVICE_DETAILS_SERVICE = "12345678-1234-1234-1234-1234567890ac";
const DEVICE_RESTART_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0000";
const DEVICE_NAME_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0001";
const FIRMWARE_VERSION_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0002";
const HTTP_CREDENTIALS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0003";
const DEVICE_HEAP_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0004";
const OUTPUT_COLOR_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0005";
const ALEXA_SETTINGS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0006";
const ESP_NOW_DEVICES_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0007";

const WIFI_SERVICE = "12345678-1234-1234-1234-1234567890ab";
const WIFI_DETAILS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0008";
const WIFI_STATUS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0009";
const WIFI_SCAN_STATUS_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee000a";
const WIFI_SCAN_RESULT_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee000b";

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
    MatSlideToggleModule
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


  private colorSubscription: Subscription;

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

  initialized: boolean = false;
  loadingAlexa: boolean = false;
  loadingHttpCredentials = false;
  readingOutputColor = false;
  readingEspNowDevices = false;

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

  colorForm = new FormGroup({
    r: new FormGroup({
      value: new FormControl<number>(0, {
        nonNullable: true
      }),
      on: new FormControl(false, {
        nonNullable: true
      })
    }),
    g: new FormGroup({
      value: new FormControl<number>(0, {
        nonNullable: true
      }),
      on: new FormControl(false, {
        nonNullable: true
      })
    }),
    b: new FormGroup({
      value: new FormControl<number>(0, {
        nonNullable: true
      }),
      on: new FormControl(false, {
        nonNullable: true
      })
    }),
    w: new FormGroup({
      value: new FormControl<number>(0, {
        nonNullable: true
      }),
      on: new FormControl(false, {
        nonNullable: true
      })
    })
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
    this.colorSubscription = this.colorForm.valueChanges.pipe(
      throttleTime(300, asyncScheduler, {
        leading: true,
        trailing: true
      })
    ).subscribe(value => {
      const {r, g, b, w} = value!;
      if (this.outputColorCharacteristic) {
        const pr = perceptualMap(r!.value!);
        const pg = perceptualMap(g!.value!);
        const pb = perceptualMap(b!.value!);
        const pw = perceptualMap(w!.value!);
        const buffer = encodeOutputState({
          values: [
            {value: pr, on: r!.on!},
            {value: pg, on: g!.on!},
            {value: pb, on: b!.on!},
            {value: pw, on: w!.on!}
          ]
        });
        this.outputColorCharacteristic.writeValue(buffer)
          .catch(console.error);
      }
    });

  }

  ngOnDestroy() {
    this.colorSubscription.unsubscribe();
    this.disconnect();
  }

  disconnect() {
    this.initialized = false;
    this.server?.disconnect();
    this.server = null;

    // Clear GATT characteristics
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


    // Clear local UI state
    this.deviceName = null;
    this.firmwareVersion = null;
    this.wifiDetails = null;
    this.wifiScanStatus = WiFiScanStatus.NOT_STARTED;
    this.wifiStatus = WiFiStatus.UNKNOWN;
    this.wifiScanResult = [];

    this.loadingAlexa = false;
    this.loadingHttpCredentials = false;
    this.readingOutputColor = false;
    this.readingEspNowDevices = false;

    this.alexaIntegrationForm.reset();
  }

  async connectBleDevice() {
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true})
    try {
      const device = await navigator.bluetooth.requestDevice({
        filters: [{namePrefix: BLE_NAME}],
        optionalServices: [WIFI_SERVICE, DEVICE_DETAILS_SERVICE]
      });

      if (!device.gatt) {
        this.snackBar.open('Device does not support GATT', 'Close', {duration: 3000});
        return;
      }

      this.server = await device.gatt.connect();
      await this.initBleWiFiServices(this.server);
      await this.initBleDeviceDetailsServices(this.server);
      device.addEventListener('gattserverdisconnected', () => {
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
      await this.readOutputColor();
      await this.readEspNowDevices();
      this.snackBar.open('Device connected', 'Close', {duration: 3000});
      this.initialized = true;
      if (this.wifiScanResult.length === 0) {
        await this.startWifiScan();
      }
    } catch (error) {
      console.error(error);
      this.snackBar.open('Failed to connect to device', 'Close', {duration: 3000});
    } finally {
      loading.close();
    }
  }

  async startWifiScan() {
    if (!this.wifiScanStatusCharacteristic) return;
    await this.wifiScanStatusCharacteristic.writeValue(new Uint8Array([0]));
  }

  async restartDevice() {
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    try {
      await this.deviceRestartCharacteristic!.writeValue(textEncoder.encode("RESTART_NOW"));
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
      const encodedName = textEncoder.encode(deviceName);
      await this.deviceNameCharacteristic!.writeValue(encodedName);
      this.snackBar.open('Device name updated', 'Close', {duration: 3000});
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
      await this.alexaSettingsCharacteristic!.writeValue(payload);
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
      await this.httpCredentialsCharacteristic!.writeValue(payload);
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
          Validators.pattern(/^[0-9a-fA-F]{2}([:-][0-9a-fA-F]{2}){5}$/),
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

  async saveEspNowDevices() {
    const loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    try {
      const value = this.espNowDevicesForm.getRawValue();
      const buffer = encodeEspNowMessage(value);
      await this.espNowDevicesCharacteristic?.writeValue(buffer);
    } catch (e) {
      console.error('Failed to update ESP-NOW devices:', e);
      this.snackBar.open('Failed to update ESP-NOW devices', 'Close', {duration: 3000});
    } finally {
      loading.close();
    }
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

  private outputColorChanged(view: DataView) {
    const state = decodeOutputState(new Uint8Array(view.buffer));
    this.colorForm.setValue({
      r: {
        value: inversePerceptualMap(state.values[0].value),
        on: state.values[0].on
      },
      g: {
        value: inversePerceptualMap(state.values[1].value),
        on: state.values[1].on
      },
      b: {
        value: inversePerceptualMap(state.values[2].value),
        on: state.values[2].on
      },
      w: {
        value: inversePerceptualMap(state.values[3].value),
        on: state.values[3].on
      }
    }, {emitEvent: false});
  }

  private espNowDevicesChangedChanged(view: DataView) {
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
    const value = await this.httpCredentialsCharacteristic!.readValue();
    this.httpCredentialsChanged(value);
  }

  private async readDeviceName() {
    const view = await this.deviceNameCharacteristic!.readValue();
    this.deviceNameChanged(view);
  }

  private async readFirmwareVersion() {
    const view = await this.firmwareVersionCharacteristic!.readValue();
    this.firmwareVersion = decodeCString(new Uint8Array(view.buffer));
  }

  private async readWiFiStatus() {
    const view = await this.wifiStatusCharacteristic!.readValue();
    this.wifiStatusChanged(view);
  }

  private async readWiFiDetails() {
    const view = await this.wifiDetailsCharacteristic!.readValue();
    this.wifiDetailsChanged(view);
  }

  private async readWiFiScanStatus() {
    const view = await this.wifiScanStatusCharacteristic!.readValue();
    this.wifiScanStatusChanged(view);
  }

  private async readWiFiScanResult() {
    const view = await this.wifiScanResultCharacteristic!.readValue();
    this.wifiScanResultChanged(view);
  }

  private async readAlexaIntegration() {
    const view = await this.alexaSettingsCharacteristic!.readValue();
    const alexaDetails = decodeAlexaIntegrationSettings(new Uint8Array(view.buffer));
    this.resetAlexaIntegrationForm(alexaDetails.integrationMode);
    this.alexaIntegrationForm.reset(alexaDetails, {emitEvent: true});
  }

  async readOutputColor() {
    this.readingOutputColor = true;
    const view = await this.outputColorCharacteristic!.readValue();
    this.outputColorChanged(view);
    this.readingOutputColor = false;
  }

  async readEspNowDevices() {
    this.readingEspNowDevices = true;
    try {
      const view = await this.espNowDevicesCharacteristic!.readValue();
      this.espNowDevicesChangedChanged(view);
    } finally {
      this.readingEspNowDevices = false;
    }
  }

  private async sendWifiConfig(details: Uint8Array) {
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    try {
      await this.wifiStatusCharacteristic!.writeValue(details);
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
