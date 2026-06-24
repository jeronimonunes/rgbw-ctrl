import {Component, Inject} from '@angular/core';
import {DecimalPipe, NgForOf, NgIf} from '@angular/common';
import {MatButton} from '@angular/material/button';
import {
  MAT_DIALOG_DATA,
  MatDialogActions,
  MatDialogClose,
  MatDialogContent,
  MatDialogRef,
  MatDialogTitle
} from '@angular/material/dialog';
import {MatError, MatFormField, MatHint, MatInput} from '@angular/material/input';
import {FormControl, FormGroup, ReactiveFormsModule, Validators} from '@angular/forms';
import {MatOption, MatSelect} from '@angular/material/select';

@Component({
  selector: 'app-configure-safety-shutdown-dialog',
  imports: [
    MatButton,
    MatDialogActions,
    MatDialogClose,
    MatDialogContent,
    MatDialogTitle,
    MatError,
    MatFormField,
    MatInput,
    NgIf,
    ReactiveFormsModule,
    MatSelect,
    MatOption,
    NgForOf
  ],
  templateUrl: './configure-safety-shutdown-dialog.component.html',
  styleUrl: './configure-safety-shutdown-dialog.component.scss'
})
export class ConfigureSafetyShutdownDialogComponent {

  protected shutdownModes = [
    { value: 0, label: 'Disabled' },
    { value: 1, label: 'Full' },
    { value: 2, label: 'Phased' }
  ]

  form = new FormGroup({
    mode: new FormControl(0),
    voltage: new FormControl(0, [Validators.required, Validators.min(0), Validators.max(36)])
  });

  constructor(
    private matDialogRef: MatDialogRef<ConfigureSafetyShutdownDialogComponent>,
    @Inject(MAT_DIALOG_DATA) protected data: { voltage: number, mode: number }
  ) {
    this.form.reset(data);
  }

  submit() {
    if (this.form.invalid) return;
    this.matDialogRef.close(this.form.value);
  }

}
