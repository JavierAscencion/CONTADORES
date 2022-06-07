
void IRAM_ATTR onTimer(){
  portENTER_CRITICAL_ISR(&muxTimer);
    contadorIntentaReconexion++;
    contadorPublicaDatos++;
    banderaWDT = true;
  portEXIT_CRITICAL_ISR(&muxTimer);
}
void startTimer() {
  timer = timerBegin(0, 80, true); // timer_id = 0; divider=80; countUp = true;
  timerAttachInterrupt(timer, &onTimer, true); // edge = true
  timerAlarmWrite(timer, 1e6, true);  //1000 ms
  timerAlarmEnable(timer);
}

void heartBeatPrint()
{ 
  
   if(toggle_activity==1){
    digitalWrite(LED_STATUS,HIGH);
    toggle_activity=0;
   }
   else{
    digitalWrite(LED_STATUS,LOW);
    toggle_activity=1;
   }
   

    /*
    if(toggle_status==1){
    digitalWrite(LED_STATUS,HIGH);
    toggle_status=0;
   }
   else{
    digitalWrite(LED_STATUS,LOW);
    toggle_status=1;
   }
   */
  
}

void status_led_datos(){

  if(actividad_datos==true){
  for (int deg = 0; deg < 20; deg = deg + 1){
    //dacWrite(25, int(128 + 127 * (sin(deg*PI/180))));
    //valor = analogRead(23);
    //Serial.println(valor);
    //digitalWrite(LED_STATUS,127 * (sin(deg*PI/180)));
    digitalWrite(LED_ACTIVITY,HIGH);
    delay(5);
    digitalWrite(LED_ACTIVITY,LOW);
    delay(40);
    }
     actividad_datos=0;
  }

 /* if(actividad_datos==true){
   
  if(toggle_status==1){
    digitalWrite(LED_ACTIVITY,HIGH);
    toggle_status=0;
    delay(5);
   }
   else{
    digitalWrite(LED_ACTIVITY,LOW);
    toggle_status=1;
    delay(10);
   }
   actividad_datos=0;
   
   //ledcWrite(PWM1_Ch, PWM1_DutyCycle);
 // }*/
  
  
}
