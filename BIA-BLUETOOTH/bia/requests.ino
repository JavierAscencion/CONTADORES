void sendConteoRequest()
{
  static bool requestOpenResult;

  if (ConteoRequest.readyState() == readyStateUnsent || ConteoRequest.readyState() == readyStateDone)
  {
    requestOpenResult = ConteoRequest.open("POST", "http://tia-api.accesa.me/api/v1/breakdowns");
    ConteoRequest.setReqHeader("Content-type", "application/json");
    if (requestOpenResult)
    {
      // Only send() if open() returns true, or crash
      TipoSolicitud solicitud = TOTAL;
      // randomNumber = random(2,6);
      // dacWrite(25, E);
      // int valor=0;
      // valor = analogRead(23);
      // digitalWrite(LED_STATUS, valor);
      actividad_datos = 1;
      USB_SERIAL.println(serializaSolicitud(solicitud));
      ConteoRequest.send(serializaSolicitud(solicitud));
    }
    else
    {
      Serial.println("Can't send bad conteo request");
    }
  }
  else
  {
    Serial.println("Can't send conteo request");
  }
}

void ConteoCB(void *optParm, AsyncHTTPRequest *request, int readyState)
{
  if (readyState == readyStateDone)
  {
    USB_SERIAL.print("Código de respuesta HTTP: ");
    responseHttpCode = request->responseHTTPcode();
    USB_SERIAL.println(responseHttpCode);
    payloadConteo = request->responseText();
    USB_SERIAL.println("\nConteo**************************************");
    USB_SERIAL.println(payloadConteo);
    // delay(100);
    // digitalWrite(LED_ACTIVITY,LOW);
    // actividad_datos=0;
    USB_SERIAL.println("Conteo**************************************");
    if (request->responseHTTPcode() >= 200 && request->responseHTTPcode() < 300)
    {
      handleResponseFlag = true;
    }
    handleHttpCodeFlag = true;
    request->setDebug(false);
  }
}
