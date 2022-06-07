String encontrarCaracter(String cadena, char caracter, byte inicioCadena){
  byte largoCadena = cadena.length();
  String cadenaModificada="";
  //USB_SERIAL.println(cadena);
  for(byte i=inicioCadena; i < largoCadena; i++){
    if(cadena[i] == caracter){
      cadena[i] = '\0';
      cadenaModificada += '\0';
      return cadenaModificada;
    }else{
      cadenaModificada += cadena[i];
      cadena[i] = '\0';
    }
  }
}

bool sincronizarRTC(String payloadReloj)
{
  String axo = encontrarCaracter(payloadReloj, '-', 0);
  String mes = encontrarCaracter(payloadReloj, '-', axo.length());
  String dia = encontrarCaracter(payloadReloj, ' ', axo.length()+mes.length());
  String hora = encontrarCaracter(payloadReloj, ':', axo.length() + mes.length() + dia.length());
  String minuto = encontrarCaracter(payloadReloj, ':', axo.length() + mes.length() + dia.length() +  hora.length());      
  DateTime horaActual = rtc.now();
  if(axo.toInt() == horaActual.year()){
    if(mes.toInt() == horaActual.month()){
      if(dia.toInt() == horaActual.day()){
        if(hora.toInt() == horaActual.hour()){
          if(minuto.toInt() == horaActual.minute()){
            USB_SERIAL.println("Hora correcta");
            return true;
          }else{
            rtc.adjust(DateTime(axo.toInt(), mes.toInt(), dia.toInt(), hora.toInt(), minuto.toInt()));
            USB_SERIAL.println("Cambiar minuto");
            return false;
          }
        }else{
          rtc.adjust(DateTime(axo.toInt(), mes.toInt(), dia.toInt(), hora.toInt(), minuto.toInt()));
          USB_SERIAL.println("Cambiar hora");
          return false;
        }
      }else{
        rtc.adjust(DateTime(axo.toInt(), mes.toInt(), dia.toInt(), hora.toInt(), minuto.toInt()));
        USB_SERIAL.println("Cambiar dia");
        return false;
      }
    }else{
      rtc.adjust(DateTime(axo.toInt(), mes.toInt(), dia.toInt(), hora.toInt(), minuto.toInt()));
      USB_SERIAL.println("Cambiar mes");
      return false;
    }
  }else{
    rtc.adjust(DateTime(axo.toInt(), mes.toInt(), dia.toInt(), hora.toInt(), minuto.toInt()));
    USB_SERIAL.println(horaActual.year());
    USB_SERIAL.println("Cambiar año");
    return false;
  }
}

bool escribeVigenciaEEPROM(String anio, String mes, String dia)
{
  char axoVigencia[5] = {0};
  for(byte i=0; i<4; i++){
    axoVigencia[i] = EEPROM.read(150+i);
  }
  String axoVi = String(axoVigencia);
  String mesVigencia = String(EEPROM.read(155));
  String diaVigencia = String(EEPROM.read(156));
  axoVi += '\0';
  mesVigencia += '\0';
  diaVigencia += '\0';
  if((axoVi != anio) || (mesVigencia != mes) || (diaVigencia != dia)){
    USB_SERIAL.println("Cambios en vigencia");
    for(byte i=0; i<anio.length(); i++){
      EEPROM.put(150+i, anio[i]);
      EEPROM.commit();
    }
    EEPROM.put(155, mes.toInt());
    EEPROM.commit();
    EEPROM.put(156, dia.toInt());
    EEPROM.commit();
    return true;
  }
  else{
    USB_SERIAL.println("Vigencia sin cambios");
    return false;
  }
}
