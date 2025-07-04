import {Component, Input, OnDestroy} from '@angular/core';
import {FormControl, FormGroup, ReactiveFormsModule} from "@angular/forms";
import {MatCard, MatCardContent, MatCardHeader, MatCardTitle} from "@angular/material/card";
import {MatSlideToggle} from "@angular/material/slide-toggle";
import {MatSlider, MatSliderThumb} from "@angular/material/slider";
import {asyncScheduler, Subscription, throttleTime} from 'rxjs';
import {inversePerceptualMap, perceptualMap} from '../../color-utils';
import {decodeOutputState, encodeOutputState} from '../../model';

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

  private colorSubscription: Subscription;
  readingOutputColor = false;

  private _outputColorCharacteristic!: BluetoothRemoteGATTCharacteristic;

  @Input({required: true})
  set outputColorCharacteristic(characteristic: BluetoothRemoteGATTCharacteristic) {
    this._outputColorCharacteristic = characteristic;
    characteristic.addEventListener('characteristicvaluechanged', (ev: any) => this.outputColorChanged(ev.target.value));
    characteristic.startNotifications()
      .then(() => this.readOutputColor())
      .catch(console.error);
  }

  get outputColorCharacteristic(): BluetoothRemoteGATTCharacteristic | null {
    return this._outputColorCharacteristic;
  }

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

  async readOutputColor() {
    this.readingOutputColor = true;
    const view = await this.outputColorCharacteristic!.readValue();
    this.outputColorChanged(view);
    this.readingOutputColor = false;
  }

  ngOnDestroy() {
    this.colorSubscription.unsubscribe();
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
