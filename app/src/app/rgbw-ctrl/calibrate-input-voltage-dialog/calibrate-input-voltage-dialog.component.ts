import {Component, Inject} from '@angular/core';
import {MatButtonModule} from "@angular/material/button";
import {MAT_DIALOG_DATA, MatDialogModule, MatDialogRef,} from "@angular/material/dialog";
import {MatInputModule} from "@angular/material/input";
import {CommonModule} from "@angular/common";
import {FormControl, ReactiveFormsModule, Validators} from "@angular/forms";
import {MatFormFieldModule} from '@angular/material/form-field';

interface SensorData {
  milliVolts: number;
  calibrationFactor: number;
}


@Component({
  selector: 'app-calibrate-input-voltage-dialog',
  imports: [
    CommonModule,
    MatDialogModule,
    MatButtonModule,
    MatFormFieldModule,
    MatInputModule,
    ReactiveFormsModule
  ],
  templateUrl: './calibrate-input-voltage-dialog.component.html',
  styleUrl: './calibrate-input-voltage-dialog.component.scss'
})
export class CalibrateInputVoltageDialogComponent {

  calibrationFactor = new FormControl(0, {
    nonNullable: true,
    validators: [Validators.required, Validators.min(1), Validators.max(100)]
  });

  constructor(
    private matDialogRef: MatDialogRef<CalibrateInputVoltageDialogComponent>,
    @Inject(MAT_DIALOG_DATA) protected data: SensorData
  ) {
    this.calibrationFactor.reset(data.calibrationFactor);
  }

  submit() {
    if (this.calibrationFactor.invalid) return;
    this.matDialogRef.close(this.calibrationFactor.value);
  }

}
