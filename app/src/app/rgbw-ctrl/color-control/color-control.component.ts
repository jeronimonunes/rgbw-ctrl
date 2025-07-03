import {Component, Input, OnDestroy} from '@angular/core';
import {FormControl, FormGroup, ReactiveFormsModule} from "@angular/forms";
import {MatCard, MatCardContent, MatCardHeader, MatCardTitle} from "@angular/material/card";
import {MatSlideToggle} from "@angular/material/slide-toggle";
import {MatSlider, MatSliderThumb} from "@angular/material/slider";
import {asyncScheduler, Subscription, throttleTime} from 'rxjs';
import {inversePerceptualMap, perceptualMap} from '../../color-utils';
import {decodeOutputState, encodeOutputState} from '../../model';

export const OUTPUT_SERVICE = "12345678-1234-1234-1234-1234567890ad";
const OUTPUT_COLOR_CHARACTERISTIC = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeee0005";

@Component({
  selector: 'app-color-control',
  imports: [
    MatCard,
    MatCardContent,
    MatCardHeader,
    MatCardTitle,
    MatSlideToggle,
    MatSlider,
    MatSliderThumb,
    ReactiveFormsModule
  ],
  templateUrl: './color-control.component.html',
  styleUrl: './color-control.component.scss'
})
export class ColorControlComponent implements OnDestroy {


  @Input({required: true}) set server(server: BluetoothRemoteGATTServer) {
    this.initBleOutputServices(server)
      .then(() => this.readOutputColor());
  }

  private colorSubscription: Subscription;
  readingOutputColor = false;

  private outputColorCharacteristic: BluetoothRemoteGATTCharacteristic | null = null;

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

  constructor() {
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

  private async initBleOutputServices(server: BluetoothRemoteGATTServer) {
    const service = await server.getPrimaryService(OUTPUT_SERVICE);
    this.outputColorCharacteristic = await service.getCharacteristic(OUTPUT_COLOR_CHARACTERISTIC);
    this.outputColorCharacteristic.addEventListener('characteristicvaluechanged', (ev: any) => this.outputColorChanged(ev.target.value));
    await this.outputColorCharacteristic.startNotifications();
  }

  async readOutputColor() {
    this.readingOutputColor = true;
    const view = await this.outputColorCharacteristic!.readValue();
    this.outputColorChanged(view);
    this.readingOutputColor = false;
  }

  ngOnDestroy() {
    this.colorSubscription.unsubscribe();
    this.outputColorCharacteristic = null;
    this.readingOutputColor = false;
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

}
