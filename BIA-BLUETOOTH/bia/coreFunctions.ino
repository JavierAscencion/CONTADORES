void boletoMas(Boton boton)
{
  int cuenta;
  float dinero;

  if (obtenerEstado(ESTADO_PAPEL) == 18 || 22)
  {
    USB_SERIAL.println("Estable");
    // borrarRenglonOled(0, 16, 110);
    // DibujaEstadoImpresora(true);
    cuenta = boton.GetCuenta();
    cuenta++;
    boton.SetCuenta(cuenta);
    impresionBoleto(cuenta, boton.nombre, boton.costo);
    short cuentaTotal = total.GetCuentaTotal();
    cuentaTotal++;
    total.SetCuentaTotal(cuentaTotal);
    USB_SERIAL.println(cuentaTotal);
    Serial.printf("Pasajes %s: %u ☻\n", boton.nombre.c_str(), cuenta);
    dinero = boton.costo * cuenta;
    boton.SetDinero(dinero);
  }
  else
  {
    USB_SERIAL.println("¡¡¡SIN PAPEL!!!");
    borrarRenglonOled(0, 16, 110);
  }
}

void edicionBoleto(String edicion)
{
  USB_SERIAL.println("Entrando edicion boleto");
  char rutaEscrita[EEPROM.read(123)] = {0};
  char unidadEscrita[EEPROM.read(129)] = {0};
  for (byte i = 0; i < sizeof(rutaEscrita) - 1; i++)
  {
    rutaEscrita[i] = EEPROM.read(100 + i);
  }
  for (byte i = 0; i < sizeof(unidadEscrita) - 1; i++)
  {
    unidadEscrita[i] = EEPROM.read(124 + i);
  }
  String rutaEscritaCadena = String(rutaEscrita);
  rutaEscritaCadena += '\0';
  String unidadEscritaCadena = String(unidadEscrita);
  unidadEscritaCadena += '\0';
  String ruta = encontrarCaracter(edicion, ';', 0);
  String unidad = encontrarCaracter(edicion, ';', ruta.length());
  if (rutaEscritaCadena != ruta)
  {
    for (byte i = 0; i < ruta.length() - 1; i++)
    {
      EEPROM.put(100 + i, ruta[i]);
      EEPROM.commit();
      delay(50);
    }
    EEPROM.put(123, ruta.length());
    EEPROM.commit();
    delay(30);
    USB_SERIAL.println("Ruta editada");
  }
  if (unidadEscritaCadena != unidad)
  {
    for (byte i = 0; i < unidad.length(); i++)
    {
      EEPROM.put(124 + i, unidad[i]);
      EEPROM.commit();
      delay(50);
    }
    EEPROM.put(129, unidad.length());
    EEPROM.commit();
    delay(30);
    USB_SERIAL.println("Unidad editada");
  }
  USB_SERIAL.println("Saliendo edicion boleto");
}

void resetCuenta()
{
  completo.Reset();
  estudiante.Reset();
  terceraEdad.Reset();
  capacidadDiferente.Reset();
  total.SetCuentaTotal(0);
  EEPROM.write(direccionGavetaAbierta, 0);
  EEPROM.commit();
}

bool validarVigencia()
{
  char axoVigencia[5] = {0};
  // String axoVigencia = "";
  for (byte i = 0; i < 4; i++)
  {
    axoVigencia[i] = EEPROM.read(150 + i);
  }
  String axoVi = String(axoVigencia);
  int mesVigencia = EEPROM.read(155);
  int diaVigencia = EEPROM.read(156);
  DateTime tiempoActual = rtc.now();
  USB_SERIAL.println("Año RTC:");
  USB_SERIAL.println(tiempoActual.year());
  USB_SERIAL.println("Año Servidor Nube:");
  USB_SERIAL.println(axoVi);
  if (tiempoActual.year() < axoVi.toInt())
  {
    dibujaEstadoServicio(1);
    USB_SERIAL.println("Subscripcion activa");
    return true;
  }
  else if (tiempoActual.year() > axoVi.toInt())
  {
    dibujaEstadoServicio(0);
    USB_SERIAL.println("SUBSCRIPCION SUSPENDIDA AXO");
    return false;
  }
  else if (tiempoActual.year() == axoVi.toInt())
  {
    if (tiempoActual.month() < mesVigencia)
    {
      dibujaEstadoServicio(1);
      USB_SERIAL.println("Subscripcion activa");
      return true;
    }
    else if (tiempoActual.month() > mesVigencia)
    {
      dibujaEstadoServicio(0);
      USB_SERIAL.println("Subscripcion suspendida mes");
      return false;
    }
    else if (tiempoActual.month() == mesVigencia)
    {
      if (tiempoActual.day() < diaVigencia)
      {
        dibujaEstadoServicio(1);
        USB_SERIAL.println("Subscripcion activa");
        return true;
      }
      else if (tiempoActual.day() > diaVigencia)
      {
        dibujaEstadoServicio(0);
        USB_SERIAL.println("Subscripcion suspendida día");
        return false;
      }
      else if (tiempoActual.day() == diaVigencia)
      {
        dibujaEstadoServicio(1);
        USB_SERIAL.println("Subscripcion activa");
        return true;
      }
    }
  }
}

String serializaSolicitud(TipoSolicitud solicitud)
{
  String json;
  if (solicitud == TOTAL)
  {
    DynamicJsonDocument doc(256);

    doc["request"] = "total";
    doc["numeroSerie"] = numeroSerie;

    JsonObject conteo = doc.createNestedObject("conteo");
    conteo["numeroSerie"] = numeroSerie;
    conteo["completo"] = completo.GetCuenta();
    conteo["estudiante"] = estudiante.GetCuenta();
    conteo["terceraEdad"] = terceraEdad.GetCuenta();
    conteo["capacidadDif"] = capacidadDiferente.GetCuenta();
    conteo["conteoTotal"] = total.GetCuentaTotal();
    conteo["dineroTotal"] = completo.GetDinero() + estudiante.GetDinero() + terceraEdad.GetDinero() + capacidadDiferente.GetDinero();
    int vecesGaveta = EEPROM.read(direccionGavetaAbierta);
    conteo["gaveta"] = vecesGaveta;
    conteo["timestamp"] = "fecha";
    doc["fechaGaveta"] = SensorGaveta.fechaTimestamp;

    serializeJson(doc, json);
    return json;
  }
}

void reconexionWifi()
{
  int redesEncontradas = WiFi.scanNetworks();
  if (redesEncontradas == 0)
  {
    Serial.println("no networks found");
  }
  else
  {
    Serial.print(redesEncontradas);
    Serial.println(" networks found");

    for (int i = 0; i < redesEncontradas; ++i)
    {
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      if (WiFi.SSID(i) == ssid)
      {
        WiFi.begin(ssid, pass);

        Serial.println("listo mDash");
        // digitalWrite(LED_LINK,LOW);
        break;
      }
    }
  }
}
