import { Routes } from '@angular/router';

export const routes: Routes = [
  { path: '', title: 'Welcome', loadComponent: () => import('./welcome/welcome.component').then(m => m.WelcomeComponent) },
  { path: 'configure', title: 'Configure rgbw-ctrl', loadComponent: () => import('./rgbw-ctrl/rgbw-ctrl.component').then(m => m.RgbwCtrlComponent) },
];
