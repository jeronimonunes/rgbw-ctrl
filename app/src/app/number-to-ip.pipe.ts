import {Pipe, PipeTransform} from '@angular/core';
import {numberToIp} from './model';

@Pipe({
  name: 'numberToIp'
})
export class NumberToIpPipe implements PipeTransform {

  transform(value: number): string | null {
    return numberToIp(value);
  }

}
