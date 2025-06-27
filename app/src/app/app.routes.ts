import { Routes } from '@angular/router';

export const routes: Routes = [
  { path: '', title: 'Configure rgbw-ctrl', loadComponent: () => import('./rgbw-ctrl/rgbw-ctrl.component').then(m => m.RgbwCtrlComponent) },
];
