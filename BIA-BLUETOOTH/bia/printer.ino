// COMANDO POST IMPRESORA TERMICA
char IMPT_INIT[] = {27, 64, 0};      // BORRA BUFER Y RESETEA AJUSTES DE IMPRESION
char IMPT_MODO[] = {27, 33, 0};      // MODO DE IMPRESION, ***
char IMPT_ESP_PROG[] = {27, 51, 0};  // ESPACIADO ENTRE LINEAS n[0-255], MAX 1016mm
char IMPT_JUST[] = {27, 97, 0};      // JUSTIFICACION IZQ,CENT,DER n[0-2 || 48-50]
char IMPT_SIZE_CHAR[] = {29, 33, 0}; // ALTURA POR ANCHO DEL CARACTER n = swap[0-7, 0-7]
char IMPT_NEGRITA[] = {27, 69, 0};   // ACTIVA O DESACTIVA LA LETRA NEGRITA n[0,1]
char IMPT_CORTE_POS[] = {29, 86, 0}; // POSICION DEL CORTE
char IMPT_STAT[] = {16, 4, 0};       // POSICION DEL CORTE
char CUT_POS_OFFSET = 66;            // CORTE PARCIAL CON OFFSET
char CUT_POS_ACT = 49;               // CORTA EN LA POSICION ACTUAL
char FONTB = 0x01;
char FONTA = 0x00;
char interlineado = 40;
char JUST_LEFT = 0;
char JUST_CENTER = 1;

void impresionBoleto(int cuenta, String id, float costo)
{
  /*
  char ruta[EEPROM.read(123)-1]={0};
  char unidad[EEPROM.read(129)-1]={0};
  for(byte i=0; i<sizeof(ruta);i++){
    ruta[i] = EEPROM.read(100+i);
  }
  for(byte i=0; i<sizeof(unidad);i++){
    unidad[i] = EEPROM.read(124+i);
  }
  */

  USB_SERIAL.println(ruta);
  USB_SERIAL.println(unidad);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_SIZE_CHAR[i]);
  }
  serialImpresora.write(0x11);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_JUST[i]);
  }
  serialImpresora.write(JUST_CENTER);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_NEGRITA[i]);
  }
  serialImpresora.write(0x01);
  serialImpresora.printf("%s", id.c_str());
  serialImpresora.println("");
  serialImpresora.printf("COSTO: $%.2f\r\n", costo);
  serialImpresora.println("");
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_SIZE_CHAR[i]);
  }
  serialImpresora.write(0x00);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_JUST[i]);
  }
  serialImpresora.write(JUST_CENTER);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_NEGRITA[i]);
  }
  serialImpresora.write(0x00);
  serialImpresora.printf("FOLIO: %i\r\n", cuenta);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_SIZE_CHAR[i]);
  }
  serialImpresora.write(0x11);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_JUST[i]);
  }
  serialImpresora.write(JUST_CENTER);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_NEGRITA[i]);
  }
  serialImpresora.write(0x00);
  serialImpresora.print("RUTA: ");
  for (byte i = 0; i < sizeof(ruta); i++)
  {
    serialImpresora.print(ruta[i]);
  }
  serialImpresora.println("");
  serialImpresora.print("UNIDAD: ");
  for (byte i = 0; i < 3; i++)
  {
    serialImpresora.print(unidad[i]);
  }
  serialImpresora.println("");
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_SIZE_CHAR[i]);
  }
  serialImpresora.write(0x00);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_JUST[i]);
  }
  serialImpresora.write(JUST_CENTER);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_NEGRITA[i]);
  }
  serialImpresora.write(0x00);
  DateTime now = rtc.now();
  serialImpresora.print(now.hour());
  serialImpresora.print(":");
  serialImpresora.print(now.minute());
  serialImpresora.print(":");
  serialImpresora.print(now.second());
  serialImpresora.print(" ");
  serialImpresora.print(now.year());
  serialImpresora.print("/");
  serialImpresora.print(now.month());
  serialImpresora.print("/");
  serialImpresora.println(now.day());
  Serial.print(now.year());
  Serial.print("/");
  Serial.print(now.month());
  Serial.print("/");
  Serial.print(now.day());
  Serial.print(" ");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.print(now.second());
  Serial.println();
  // IMPRESION DEL QR: Ancho
  serialImpresora.write('\x1D');
  serialImpresora.write('\x28');
  serialImpresora.write('\x6B');
  serialImpresora.write('\x03');
  serialImpresora.write('\x00');
  serialImpresora.write('\x31');
  serialImpresora.write('\x43');
  serialImpresora.write('\x04'); // Ancho del QR
  // QR: Memoria
  serialImpresora.write('\x1D');
  serialImpresora.write('\x28');
  serialImpresora.write('\x6B');
  serialImpresora.write(strlen("www.accesa.me") + 3); // LongitudQR+3
  serialImpresora.write('\x00');
  serialImpresora.write('\x31');
  serialImpresora.write('\x50');
  serialImpresora.write('\x30');
  // QR:Data and imprimir
  serialImpresora.print("www.accesa.me");
  serialImpresora.write('\x1D');
  serialImpresora.write('\x28');
  serialImpresora.write('\x6B');
  serialImpresora.write('\x03');
  serialImpresora.write('\x00');
  serialImpresora.write('\x31');
  serialImpresora.write('\x51');
  serialImpresora.write('\x30');
  /*
  for(byte i=0; i<2; i++){
  serialImpresora.write(IMPT_SIZE_CHAR[i]);
  }
  serialImpresora.write(0x11);
  for(byte i=0; i<2; i++){
    serialImpresora.write(IMPT_JUST[i]);
  }
  serialImpresora.write(JUST_CENTER);
  for(byte i=0; i<2; i++){
    serialImpresora.write(IMPT_NEGRITA[i]);
  }
  serialImpresora.write(0x01);
  serialImpresora.println("GRATIS $500 PESOS");
  */
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_SIZE_CHAR[i]);
  }
  serialImpresora.write(0x00);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_JUST[i]);
  }
  serialImpresora.write(JUST_CENTER);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_NEGRITA[i]);
  }
  serialImpresora.write(0x00);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_MODO[i]);
  }
  serialImpresora.write(FONTA);
  serialImpresora.println("SI NO RECIBE SU BOLETO O");
  serialImpresora.println("NO CORRESPONDE A SU TARIFA");
  serialImpresora.println("REPORTE AL CEL: " + REPORTES + "\r\n");
  serialImpresora.println("Ventas: " + VENTAS);
  serialImpresora.println("www.accesa.me | automatizacion@accesa.me\r\n");
  serialImpresora.println("Conserve este boleto,\r\n es su seguro de viajero.");
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_CORTE_POS[i]);
  }
  serialImpresora.write(CUT_POS_OFFSET);
  serialImpresora.write(20);
}

void iniciaImpresora()
{
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_INIT[i]);
  }
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_MODO[i]);
  }
  serialImpresora.write(FONTA);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_ESP_PROG[i]);
  }
  serialImpresora.write(interlineado);
}

void ejecutaCorte()
{
  /*
  char ruta[EEPROM.read(123)-1]={0};
  char unidad[EEPROM.read(129)-1]={0};
  for(byte i=0; i<sizeof(ruta);i++){
    ruta[i] = EEPROM.read(100+i);
  }
  for(byte i=0; i<sizeof(unidad);i++){
    unidad[i] = EEPROM.read(124+i);
  }
  */
  SensorGaveta.vecesAbierta = EEPROM.read(direccionGavetaAbierta);
  // imprime corte doble
  for (byte i = 0; i < 2; i++)
  {
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_INIT[i]);
    }
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_MODO[i]);
    }
    serialImpresora.write(FONTA);
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_ESP_PROG[i]);
    }
    serialImpresora.write(20);
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_JUST[i]);
    }
    serialImpresora.write(JUST_LEFT);
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_SIZE_CHAR[i]);
    }
    serialImpresora.write(0x33);
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_SIZE_CHAR[i]);
    }
    serialImpresora.write(0x11);
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_JUST[i]);
    }
    serialImpresora.write(JUST_CENTER);
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_NEGRITA[i]);
    }
    serialImpresora.write(0x01);
    serialImpresora.print("RUTA: ");
    for (byte i = 0; i < sizeof(ruta); i++)
    {
      serialImpresora.print(ruta[i]);
    }
    serialImpresora.println("");
    serialImpresora.print("UNIDAD: ");
    for (byte i = 0; i < sizeof(unidad); i++)
    {
      serialImpresora.print(unidad[i]);
    }
    serialImpresora.println("");
    serialImpresora.println("BOLETOS EMITIDOS");
    DateTime now = rtc.now();
    serialImpresora.print(now.hour());
    serialImpresora.print(":");
    serialImpresora.print(now.minute());
    serialImpresora.print(":");
    serialImpresora.print(now.second());
    serialImpresora.print(" ");
    serialImpresora.print(now.year());
    serialImpresora.print("/");
    serialImpresora.print(now.month());
    serialImpresora.print("/");
    serialImpresora.println(now.day());
    imprimeBoletoCorte(completo);
    imprimeBoletoCorte(estudiante);
    imprimeBoletoCorte(terceraEdad);
    imprimeBoletoCorte(capacidadDiferente);
    // Imprime veces que la gaveta ha sido abierta
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_SIZE_CHAR[i]);
    }
    serialImpresora.write(0x11);
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_JUST[i]);
    }
    serialImpresora.write(JUST_LEFT);
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_NEGRITA[i]);
    }
    serialImpresora.write(0x01);
    serialImpresora.println("Gaveta");
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_SIZE_CHAR[i]);
    }
    serialImpresora.write(0x00);
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_JUST[i]);
    }
    serialImpresora.write(JUST_LEFT);
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_NEGRITA[i]);
    }
    serialImpresora.write(0x00);
    serialImpresora.printf("Abierta: %i\r\n", SensorGaveta.vecesAbierta);
    // Imprime veces que la gaveta ha sido abierta
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_SIZE_CHAR[i]);
    }
    serialImpresora.write(0x11);
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_JUST[i]);
    }
    serialImpresora.write(JUST_LEFT);
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_NEGRITA[i]);
    }
    serialImpresora.write(0x01);
    serialImpresora.printf("TOTAL RECAUDADO: $%.2f\r\n", (completo.GetDinero() + estudiante.GetDinero() + terceraEdad.GetDinero() + capacidadDiferente.GetDinero()));
    serialImpresora.printf("TOTAL PERSONAS: %i\r\n", total.GetCuentaTotal());
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_SIZE_CHAR[i]);
    }
    serialImpresora.write(0x11);
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_JUST[i]);
    }
    serialImpresora.write(JUST_LEFT);
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_NEGRITA[i]);
    }
    serialImpresora.write(0x01);
    serialImpresora.println("NOMBRE:_________________\r\n\r\n");
    serialImpresora.println("\r\n");
    serialImpresora.println("FIRMA:__________________\r\n\r\n");
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_SIZE_CHAR[i]);
    }
    serialImpresora.write(0x11);
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_JUST[i]);
    }
    serialImpresora.write(JUST_CENTER);
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_NEGRITA[i]);
    }
    serialImpresora.write(0x01);
    serialImpresora.println("Ventas: " + VENTAS + "\r\n");
    for (byte i = 0; i < 2; i++)
    {
      serialImpresora.write(IMPT_CORTE_POS[i]);
    }
    serialImpresora.write(CUT_POS_OFFSET);
    serialImpresora.write(20);
  }
  banderaCorte = false;
}

void imprimeBoletoCorte(Boton boton)
{
  short cuenta = boton.GetCuenta();

  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_SIZE_CHAR[i]);
  }
  serialImpresora.write(0x11);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_JUST[i]);
  }
  serialImpresora.write(JUST_LEFT);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_NEGRITA[i]);
  }
  serialImpresora.write(0x01);
  serialImpresora.println(boton.nombre);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_SIZE_CHAR[i]);
  }
  serialImpresora.write(0x00);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_JUST[i]);
  }
  serialImpresora.write(JUST_LEFT);
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_NEGRITA[i]);
  }
  serialImpresora.write(0x00);
  serialImpresora.printf("SUBEN:%i\r\n", cuenta);
  serialImpresora.printf("TOTAL:%.2f\r\n", cuenta * boton.costo);
}

byte obtenerEstado(byte solicitud)
{
  byte j = 0, retorno;
  for (byte i = 0; i < 2; i++)
  {
    serialImpresora.write(IMPT_STAT[i]);
  }
  serialImpresora.write(solicitud);
  while (serialImpresora.available())
  {
    byte estado = serialImpresora.read();
    retorno = estado;
    switch (solicitud)
    {
    case 1:
      USB_SERIAL.print("Estado Impresora: ");
      USB_SERIAL.println(estado, DEC);
      return estado;
      break;
    case 2:
      USB_SERIAL.print("Causa Offline: ");
      USB_SERIAL.println(estado, DEC);
      return retorno;
      break;
    case 3:
      USB_SERIAL.print("Tipo Error: ");
      USB_SERIAL.println(estado, DEC);
      return retorno;
      break;
    case 4:
      USB_SERIAL.print("Estado Papel: ");
      USB_SERIAL.println(estado, DEC);
      return retorno;
      break;
    }
  }
  return 0;
}
