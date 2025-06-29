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
  ESP_NOW_DEVICE_NAME_MAX_LENGTH,
  ESP_NOW_MAX_DEVICES_PER_MESSAGE,
  isEnterprise,
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
import {BluetoothService} from './bluetooth.service';

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

  connected = false;

  constructor(
    private matDialog: MatDialog,
    private snackBar: MatSnackBar,
    private ble: BluetoothService
  ) {
    this.colorSubscription = this.colorForm.valueChanges.pipe(
      throttleTime(300, asyncScheduler, {
        leading: true,
        trailing: true
      })
    ).subscribe(value => {
      const {r, g, b, w} = value!;
      const pr = perceptualMap(r!.value!);
      const pg = perceptualMap(g!.value!);
      const pb = perceptualMap(b!.value!);
      const pw = perceptualMap(w!.value!);
      this.ble.setOutputColor({
        values: [
          {value: pr, on: r!.on!},
          {value: pg, on: g!.on!},
          {value: pb, on: b!.on!},
          {value: pw, on: w!.on!}
        ]
      }).catch(console.error);
    });

    this.ble.connected$.subscribe(v => this.connected = v);
    this.ble.initialized$.subscribe(v => this.initialized = v);
    this.ble.firmwareVersion$.subscribe(v => this.firmwareVersion = v);
    this.ble.deviceName$.subscribe(v => this.deviceName = v);
    this.ble.deviceHeap$.subscribe(v => this.deviceHeap = v);
    this.ble.wifiStatus$.subscribe(v => this.wifiStatus = v);
    this.ble.wifiDetails$.subscribe(v => this.wifiDetails = v);
    this.ble.wifiScanStatus$.subscribe(v => this.wifiScanStatus = v);
    this.ble.wifiScanResult$.subscribe(v => this.wifiScanResult = v);
    this.ble.outputColor$.subscribe(state => {
      if (!state) return;
      this.colorForm.setValue({
        r: { value: inversePerceptualMap(state.values[0].value), on: state.values[0].on },
        g: { value: inversePerceptualMap(state.values[1].value), on: state.values[1].on },
        b: { value: inversePerceptualMap(state.values[2].value), on: state.values[2].on },
        w: { value: inversePerceptualMap(state.values[3].value), on: state.values[3].on },
      }, { emitEvent: false });
    });
    this.ble.alexaSettings$.subscribe(settings => {
      if (!settings) return;
      this.resetAlexaIntegrationForm(settings.integrationMode);
      this.alexaIntegrationForm.reset(settings, { emitEvent: false });
    });
    this.ble.httpCredentials$.subscribe(credentials => {
      if (credentials) this.httpCredentialsForm.reset(credentials);
    });
    this.ble.espNowDevices$.subscribe(devices => {
      while (this.espNowDevicesForm.length > devices.length) {
        this.espNowDevicesForm.removeAt(0);
      }
      while (this.espNowDevicesForm.length < devices.length) {
        this.addEspNowDeviceFormEntry();
      }
      this.espNowDevicesForm.reset(devices, { emitEvent: false });
    });

  }

  ngOnDestroy() {
    this.colorSubscription.unsubscribe();
    this.disconnect();
  }

  disconnect() {
    this.ble.disconnect();
  }

  async connectBleDevice() {
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    try {
      await this.ble.connect();
      this.snackBar.open('Device connected', 'Close', {duration: 3000});
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
    await this.ble.startWifiScan();
  }

  async restartDevice() {
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    try {
      await this.ble.restartDevice();
    } catch (e) {
      console.error('Failed to restart device:', e);
      this.snackBar.open('Failed to restart device', 'Close', {duration: 3000});
    } finally {
      this.disconnect();
      loading.close();
    }
  }

  async connectToNetwork(network: WiFiNetwork) {
    let details: WiFiConnectionDetails;
    if (network.encryptionType === WiFiEncryptionType.WIFI_AUTH_OPEN) {
      const value: WiFiConnectionDetails = {
        encryptionType: network.encryptionType,
        ssid: network.ssid,
        credentials: {password: ''}
      };
      details = value;
    } else if (isEnterprise(network.encryptionType)) {
      let value = await firstValueFrom(this.matDialog.open(EnterpriseWiFiConnectDialogComponent, {data: network}).afterClosed());
      if (!value) return;
      details = value;
    } else {
      let value = await firstValueFrom(this.matDialog.open(SimpleWiFiConnectDialogComponent, {data: network}).afterClosed());
      if (!value) return;
      details = value;
    }
    await this.ble.sendWifiConfig(details);
  }

  async connectToCustomNetwork() {
    const data: WiFiConnectionDetails = await firstValueFrom(this.matDialog.open(CustomWiFiConnectDialogComponent).afterClosed());
    if (!data) return;
    let details: WiFiConnectionDetails;
    if (isEnterprise(data.encryptionType)) {
      let value = await firstValueFrom(this.matDialog.open(EnterpriseWiFiConnectDialogComponent, {data}).afterClosed());
      if (!value) return;
      details = value;
    } else {
      let value = await firstValueFrom(this.matDialog.open(SimpleWiFiConnectDialogComponent, {data}).afterClosed());
      if (!value) return;
      details = value;
    }
    await this.ble.sendWifiConfig(details);
  }

  async editDeviceName() {
    const deviceName = await firstValueFrom(this.matDialog.open(EditDeviceNameComponentDialog, {data: this.deviceName}).afterClosed())
    if (deviceName) {
      await this.ble.setDeviceName(deviceName);
      this.snackBar.open('Device name updated', 'Close', {duration: 3000});
    }
  }

  async loadAlexaIntegrationSettings() {
    this.loadingAlexa = true;
    await this.ble.readAlexaIntegration();
    this.loadingAlexa = false;
  }

  async applyAlexaIntegrationSettings() {
    if (this.alexaIntegrationForm.invalid || !this.connected) {
      return;
    }
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    // Encode the form value and write via BLE
    const settings = this.alexaIntegrationForm.value;
    try {
      await this.ble.applyAlexaIntegrationSettings(settings as any);
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
    await this.ble.readHttpCredentials();
    this.loadingHttpCredentials = false;
  }

  async applyHttpCredentials() {
    if (this.httpCredentialsForm.invalid || !this.connected) {
      return;
    }
    const credentials = this.httpCredentialsForm.getRawValue();
    let loading = this.matDialog.open(LoadingComponent, {disableClose: true});
    try {
      await this.ble.applyHttpCredentials(credentials);
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
      await this.ble.saveEspNowDevices(value);
    } catch (e) {
      console.error('Failed to update ESP-NOW devices:', e);
      this.snackBar.open('Failed to update ESP-NOW devices', 'Close', {duration: 3000});
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
