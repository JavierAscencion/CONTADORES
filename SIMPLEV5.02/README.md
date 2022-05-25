Código de contadores de camiones, con versión para PIC con encapsulado QFN-44, PCB montaje superficial modelo: CRUV1.3

Es necesario montar el módulo GPRS para que puedan operar lo sensores porque sino hay un bloqueo y no contaria (sub_cta1=sub_cta2=sub_cta3=sub_cta4=sub_cta5=sub_cta6=0; //se inicializa cuenta pasajeros).

Se hicieron ajustes al código fuente original para que pudiera cominicarse sin errores por medio del puerto serial PIC-PIC, se cambiaron lista de comandos que se escribian y leian en memoria eeprom, ahora se graban en código y se validan por medio de funciones del stdio.h, asi se vuelve seguro las validaciones, se recibe dato serial de GPRS por medio de banderas como interrupciones, es decir hay pines dedicados en el GPRS y PIC para sincronizar la captura serial y confirmación en PIC y GPRS.

Para la programación de los PIC´s es necesario saber cual es el maestro y cual el esclavo de los postes para cargar el firmware correcto
