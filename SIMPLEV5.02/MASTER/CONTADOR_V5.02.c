//2021/05/13 *V5.01 Se recalcula frecuencia de IR
//2020/03/31 *V4.A1 Se agrega etiqueta para identificar por LCD si hay comunicacion entre los postes
//2020/02/28 *V04.A Se modifica codigo para evitar que postes cuenten solos
//2019/12/09 Se modifica la forma de contar con sensores bloqueados
//2019/11/19 Se modifica codigo para detectar una recepcion serial de dato poste posterior erronea, asi como identificar error de conexion tambien,
//           Se quita funcion de boton silenciador que nunca se uso
//2019/11/14 Se modifica la escucha de la cuenta del poste secundario, se cambia limite a 20 para dato posterior recibido.
//D01.2 2019/03/28 Se agrega cambios para confirmar reset a nuevo GSM y enter al final de envio de cuenta
//                 Se discriminan los primneros 2 caracteres de validacion serial
// ultima modificacion;
/*
VER.  FECHA
D02.6 2019/12/09 Cambia la forma de contar
- Poste de datos esta identificado como 1, el esclavo como 2
*/

#include <18F4580.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#use delay (clock=20000000)
#use rs232(uart1, baud=9600,TIMEOUT=10,stream=monitor)//pic to pic
//#use rs232(BAUD=9600, BITS=8, PARITY=N, XMIT=PIN_C0,rcv=PIN_C1,TIMEOUT=40,stream=GPS,DISABLE_INTS)//
#use rs232(BAUD=9600, BITS=8, PARITY=N, XMIT=PIN_C1,rcv=PIN_C0,TIMEOUT=40,stream=GPS,DISABLE_INTS)//
#fuses HS,NOLVP,PUT,PROTECT,NODEBUG,NOWDT,WDT4096//pin D5 rx, tx_d0
#priority int_rda
#include "LCD_4x20.c"
#include "24256_eeprom.c"
//******************************************************************************
//entradas
#define entrada01  PIN_B4////SENSOR 1              2  1
#define entrada02  PIN_D4////SENSOR 2              4  3
#define entrada03  PIN_B2////SENSOR 3              6  5
#define entrada04  PIN_B3////SENSOR 4
#define entrada05  PIN_B0////SENSOR 5
#define entrada06  PIN_B1////SENSOR 6

#define rst_cta PIN_D3////REINICIAR CONTADOR
#define frente  PIN_D2///INDICADOR DE MAESTRO
#define B_silencio  PIN_D1///BOTON PARA SILENCIAR (eliminado)
//salidas
#define salida01  PIN_C3//Buzzer
#define salida02  PIN_C4//Indicador de Sensado
#define salida03  PIN_C5//Relay

#define HB PIN_E0  //habilita recepcion de datos en gprs  pin 25->D39
#define EN PIN_E1  //monitor de "envio listo" desde gprs  pin 26->D38


const int s_cta=5;//numero de veces para verificar obsturbcion (sensibilidad de deteccion)
int size_s;//Tama�o de trama que espera leer el puerto serial (se utiliza para agilizar la captura de datos)
///////VARIABLES DE CONEXION/////
int error,transmitir;//variable para indicar un error al escuchar la cuenta del poste trasero, bandera para inciar transmision a gprs
///////GENERALES//////////////////////
int sumae1,sumas1;//Variables temporales de suma de entradas y salidas
int lim_b;//cuenta de sensores bloqueados
int fse1,fse2,fse3,fss1,fss2,fss3;//banderas que indican si los pares de sensores detectaron entrada o salida
unsigned int16 temporal1,temporal2,tempo_tn,sub_atras,baj_atras;//variables temporales para hacer calculos.
int16 entran1,salen1;//variables temporales para hacer calculos.
///////--------------------///////////
int16 salian,time_clr;//variable temporal para guardar salidas
int ini,i,ii,j=0,a,master=0,flag_enviar,f_clr;
int16 b,n,cta_bloqueo,cta_enviar,total2_t;//,envia_ent,envia_sal;
int cta_bloq1=0,cta_bloq2=0,cta_bloq3=0;//variables para contar bloqueos por pares
int ent1=0,ent2=0,ent3=0,ent4=0,ent5=0,ent6=0;//variables de estado de sensores
int sen_ent=0,sen_sal=0;//variables para definir el sentido (subio o bajo)
int16 sub_cta1=0,sub_cta2=0,sub_cta3=0,sub_cta4=0,sub_cta5=0,sub_cta6=0;//variables para calculos de cuentas
int detec1,detec2,detec3,detec4,detec5,detec6;//variables para determinar el cambio de estado de los sensores
int sub_d1,sub_d2,sub_d3,sub_d4,sub_d5,sub_d6;//variables para determinar el cambio de estado de los sensores
int entra_act1=0,edo1=0,entra1=0,sale1=0,edoa1=0,enable1=0,entra_temp1=0;//variables para la maquina de estados de sensores par1
int entra_act2=0,edo2=0,entra2=0,sale2=0,edoa2=0,enable2=0,entra_temp2=0;//variables para la maquina de estados de sensores par2
int entra_act3=0,edo3=0,entra3=0,sale3=0,edoa3=0,enable3=0,entra_temp3=0;//variables para la maquina de estados de sensores par3
int16 entraront=0,salieront=0,pasaje=0,pasajet=0;//cuenta total de entradas y salidas
int16 tem_entraront=0,tem_salieront=0,temporal3;//cuenta total de entradas y salidas recibidas de cuentas del poste trasero.
int des1=0,des2=0,des3=0;
int bloq_p1,bloq_p2,bloq_p3,en_b1,en_b2,en_b3;
int16 tim_p1,tim_p2,tim_p3,segundoen,time_envio;
const int32 set_timer=59286;//10ms
const int16 time_bloqueo=500;//5seg. 100=1seg.
int clear_lcd1,time_lcd1;
int y=0;
int bandera_revision=0;
int16 cntbnd=0;

// variables temporales para la conversion de string a entero de datos seriales de la cuenta de pasajeros del poste trasero///
char term[3], *ptr;
char data1[14];
char data2[10];
char data3[10];
char data4[10];
char data5[10];
int caso_valor=0;
int solicitud_reset_gps=0;

//lista de comandos para contadores ///
char *textosearch[11] =    { "V5.01     \r",
                             "BORRAR_ALL\r",
                             "GRABAR_DAT\r",
                             "LEERCUENTA\r",//
                             "SOLICITADO\r",
                             "PASAJEROS:\r",
                             "SERIALTEST\r",
                             "SERIAL_TOK\r",
                             "SUBIENDOC2\r",
                             "SINFUNCION\r",
                             "          \r"                           
                           };

char version[]="V5.02";//<---Version de codigo de contador

/////EEPROM///////////
const int word_size =10;//TAMA�O DE LOS DATOS EN EEPROM//antes 8
const int data_set=9;
//char XX[word_size+19];// ARREGLO SERIAL
char XX[word_size+22];// ARREGLO SERIAL
char entran[5];
char salen[5];
char bloqueado[5];
char memory[word_size];
//*******************************//
char tempo_tc[5];
/////////////FUNCIONES/////////////
void rev_suben();
void enviop1();
void finsuma();
void leer_conta2();
void graba_conta2();

int convertir_to_entero(char *cadena);
int evaluar_string(char *command[]);
void envia2();
void solicitar();
void envio_master();
void envio();
void limpiar();
void detecta_suma();
void write_ent();
void write_sal();
void entraron_total();
void salieron_total();
void rd_eeprom();
void rd_eeprom_inicio();
//void llaves();

void lcd_cuentas();
void sentidos();
void sensores();
void sensando1();
void sensando2();
void sensando3();
void contando1();
void contando2();
void contando3();
void dar_dato();
void reset();
//------------------------------------------------------------------------------
// Rutina de gesti�n de interrupciones
//------------------------------------------------------------------------------
#int_TIMER1 //se utiliza timer 1 porque el 0 esta asignado al wdt
void TIMER1_isr(void){
   time_clr++;
   segundoen++;
   if(segundoen>=100) {
      segundoen=0;
      time_envio++;
      time_lcd1++;
   }
//****Se definen los estados de los sensores (deteccion)***//
   //sensor1
   ent1=input(entrada01);
   if(ent1==0) sub_cta1++;//ent1 =0 (no pulso)
   //sensor2
   ent2=input(entrada02);
   if(ent2==0) sub_cta2++;
   //sensor3
   ent3=input(entrada03);
   if(ent3==0) sub_cta3++;
   //sensor4
   ent4=input(entrada04);
   if(ent4==0) sub_cta4++;
   //sensor5
   ent5=input(entrada05);
   if(ent5==0) sub_cta5++;
   //sensor6
   ent6=input(entrada06);
   if(ent6==0) sub_cta6++;
//****************************************//
   tim_p1++;
   tim_p2++;
   tim_p3++;
   cta_enviar++;
set_timer1(set_timer);// 10ms
}
//****************************************************************************//

//-----------------------------------------------------------------------------
//timer para la el tiempo de espera del puerto serial emulado, datos del gprs//
#int_TIMER3
void  TIMER3_isr(void) 
{
   cntbnd++;  //bandera de conteo de timepo de espera de para puerto serial emlado
   set_timer3(65036); 
   
}


//-----------------------------------------------------------------------------
// captura de datos seriales entre postes, delantero y trasero /////////////
#int_rda
void isr_rs232(){ //escucha segundo poste
disable_interrupts(GLOBAL);//deshabilita interrupciones globales
i=0; // CONTADOR DE ARREGLO SERIAL INICIADO
   while(i<=30){
   XX[i] = fgetc(monitor);
   i++;
   if(XX[i]==13)break;
   };//se captura el dato del serial monitor(comunicacion entre pics)
j=1;
y=1;
//sub_cta1=sub_cta2=sub_cta3=sub_cta4=sub_cta5=sub_cta6=0;//reinicia la cuenta para detectar sensores
}

#ZERO_RAM
void main(){
//Apaga entradas analogicas
setup_adc_ports(NO_ANALOGS);
setup_adc(ADC_OFF);
lcd_init();//inicia LCD
init_ext_eeprom();//Inicia EEPROM
setup_timer_1(T1_INTERNAL|T1_DIV_BY_8);//Configura timer1:Reloj interno, preescaler=8
enable_interrupts(INT_TIMER1);//Habilito interrupci�n particular del TIMER1
set_timer1(set_timer);//Setea cuenta para la interrupcion.
disable_interrupts(INT_EXT);//desabilita interrupciones externas
enable_interrupts(GLOBAL);//habilita interrupciones globales
///**************************************************************************///
////////Confuguracion de PWM////////
output_low(PIN_C2);//CCP1
setup_ccp1(CCP_PWM);
setup_timer_2(t2_div_By_1,131,1);//frecuencia 38khz
//set_pwm1_duty(26);//(5%) alto
set_pwm1_duty(279);//(10%) Parte alta del ciclo 
////////////////////////////////////
ini=read_eeprom(100);//Carga estado de inicio
if(ini!=1){//Verifica si es la primera vez que se enciende el poste y graba configuracion inicial
   write_eeprom(100,1);//Graba inicio
   limpiar();//pone en 0 el registro de cuentas almacenadas.
}
/////////////////////////////////////////////////////////////////////////////////////////////////
entraron_total();//carga el total de entradas registradas
salieron_total();//carga el total de salidas registradas
ent1=ent2=ent3=ent4=ent5=ent6=0;//inicia en 0 variables de estado de sensores
tim_p1=tim_p2=tim_p3=0;//inicia en 0 las cuentas para detectar sensores bloqueados
en_b1=en_b2=en_b3=0;//bandera de pares bloqueados
time_envio=0;//inicia en 0 cuenta para transmitir datos
if(input(frente) ) {//revisa si es maestro (1) o escalvo (2)
   master=1;
   size_s=29;
}
else {
   master=0;
   size_s=15;
}
lcd_cuentas();//Escribe texto base en LCD
cta_bloq1=read_ext_eeprom(9);//carga bloqueos acumulados en par1
cta_bloq2=read_ext_eeprom(10);//carga bloqueos acumulados en par2
cta_bloq3=read_ext_eeprom(11);//carga bloqueos acumulados en par3
int tem_res;
tem_res=input(rst_cta);//inicializa variable de reset de cuenta fisico
j=0;
flag_enviar=0;
salian=0;
enable_interrupts(INT_RDA);//habilita interrupcion serial
setup_wdt(WDT_ON);//habilita watch dog
sub_cta1=sub_cta2=sub_cta3=sub_cta4=sub_cta5=sub_cta6=0;
tem_entraront=tem_salieront=0;
error=0;
reset();//Revisa causa de inicio del pic
transmitir=0;
output_low(salida02);
sub_atras=baj_atras=0;
clear_lcd1=0;

output_low(HB); //esperamos a que se aliste un dato del puerto serial emulado
bandera_revision=1;
caso_valor=0;

while(TRUE){
//solo para reinicializar interrupciones despues de obtener datos por puerto serial nato
 if(y==1){
  enable_interrupts(GLOBAL);//habilita interrupciones globales
  y=0;
 }
//////////////////////boton para reset cuenta////////////////////////////////////////////
   if(tem_res!=input(rst_cta) ){//BOTON DE RESET DE CUENTA
      delay_ms(50);
      if(tem_res!=input(rst_cta) ){
         tem_res=input(rst_cta);
         if( (master==1)&&(tem_res==0) ) {
            fprintf(monitor,"BORRAR_ALL\r");
            printf(lcd_putc,"\fBORRAR_ALL");
            limpiar();
         }
      }
   }
//-------pines de monitoreo de datos seriales por puerto emulado----------------------------
if(input(EN) == 1  && bandera_revision == 1){ //1
     output_high(HB); //avisa que ya est� listo para recibir dato serial
     cntbnd=0; 
     //i=0;
     bandera_revision=0;
     output_high(salida03);
     //fprintf(monitor,"relay-");  
      while (kbhit(GPS)!=1 || cntbnd >= 200) /// espera a que se complete dato serial o timer se agote
      //while (XX[0]==0 || cntbnd >= 200) /// espera a que se complete dato serial o timer se agote
      {
        bandera_revision=1;
        //fprintf(monitor,"wh-");
      } 
      XX[0] = fgetc(GPS);
     /*
      while(i<=30){
      XX[i] = fgetc(GPS);
      fprintf(monitor,".");
      i++;
      if(XX[i]==13)break;
      };//se captura el dato del serial monitor(comunicacion entre pics)
     */
     // si el comando es ?, es coomando valido de solicitud de borrar cuenta
    // fprintf(monitor,"<");
    // fprintf(monitor,"%c",XX[0]);
    // fprintf(monitor,">");
     if(XX[0]==63)
     {
     output_low(salida03);
         j=1;
         sub_cta1=sub_cta2=sub_cta3=sub_cta4=sub_cta5=sub_cta6=0; //se inicializa cuenta pasajeros
         solicitud_reset_gps=1;
         /*
         XX[0] = 'B';
         XX[1] = 'O';
         XX[2] = 'R';
         XX[3] = 'R';
         XX[4] = 'A';
         XX[5] = 'R';
         XX[6] = '_';
         XX[7] = 'A';
         XX[8] = 'L';
         XX[9] = 'L';
         XX[10] = '\n';
         */
         strcpy(XX,"BORRAR_ALL\r");  //BORRAR_ALL\r  
         output_low(HB); // se reinicia la espera de nuevo dato serial
         bandera_revision=1;
     }
 
 
}
 //------------------------------------------------------------------------------ 

   if(j==1){//Revisa si hubo datos por cualquiera de los 2 puertos seriales
      rd_eeprom();//valida dato recibido para ver si es una palabra de control
      caso_valor = evaluar_string(XX);
      lcd_gotoxy(9,1);
      printf(lcd_putc,"%d ",caso_valor);//pinta en lcd si el numero de caso detectado (9 significa que no es un dato valido)

      switch (caso_valor) {
          case 1: {//Se pone en 0 la cuenta
                  limpiar();
                  if(master==1) fprintf(monitor,"BORRAR_ALL\r");//4
                  if(solicitud_reset_gps==1 && master==1){
                  solicitud_reset_gps=0;
                  fprintf(GPS,"RESET_OK\r\n");//confirma a gsm
                  }
                  break;}
          case 2: {//Se hace solicitud de cuenta del poste trasero
                  solicitar();
                  break;}
          case 3: {//Se pone en 0 la cuenta por peticion del GPRS
                  if(master==1) fprintf(GPS,"RESET_OK\r\n");//confirma a gsm 
                  delay_ms(1000);
                  limpiar();
                  if(master==1) fprintf(monitor,"BORRAR_ALL\r");//4
                  break;}
          case 4: {//ver pasaje total de este acceso (contador atras)
                  envio_master();
                  break;}
          case 5: {//envia a GPS pasaje total actual de ambos accesos (recibe cuenta de atras y suma)
                  flag_enviar=0;
                  enviop1();
                  transmitir=1;
                  lcd_gotoxy(9,4);
                  lcd_putc("C2OK");
                  
                  break;}
          case 6: {//TEST DE COMUNICACION SERIAL
                  clear_lcd1=1;
                  time_lcd1=0;
                  lcd_gotoxy(9,3);
                  lcd_putc("C1OK");
                  fprintf(monitor,"SERIAL_TOK\r");//
                  break;}
          case 7: {//CONFIRMACION DE COMUNICACION SERIAL
                  clear_lcd1=1;
                  time_lcd1=0;
                  lcd_gotoxy(9,4);
                  lcd_putc("C2OK");
                  break;}
          case 8: {//Suben por atras
                  clear_lcd1=1;
                  time_lcd1=0;
                  lcd_gotoxy(9,4);
                  lcd_putc("Sub2");
                  break;}
          default:{
                  lcd_gotoxy(9,2);
                  printf(lcd_putc,"c=0");
                  }
      }//end switch

      caso_valor=0;
      memset(XX, 0, sizeof(XX) );
      time_clr=0;
      f_clr=1;
      j=0;
   }
    if( (time_envio>=180)&&(master) ){//Revisa tiempo para transmitir por GPRS (time_envio esta en segundos, 180s/60 = 3min)
         solicitar();
        
   }
   if( (clear_lcd1)&&(time_lcd1>=10) ){//cronometro para limpiar notificaciones LCD
      clear_lcd1=0;
      lcd_gotoxy(9,1);
      lcd_putc("    ");
      lcd_gotoxy(9,3);
      lcd_putc("    ");
      lcd_gotoxy(9,4);
      lcd_putc("    ");
   }
   
   if( (flag_enviar)&&(cta_enviar>=2000)&&(master) ) envia2();//Si el poste esclavo no contesta enviar sin conexion
   if(transmitir) envio();//cuando el esclavo reporto su cuenta, maestro prepara paquete para mandar a la nube
   
   if( (bloq_p1)&&(tim_p1>=time_bloqueo)&&(!en_b1) ) {//bloqueo de par 1 por m�s de X segundos
       bloq_p1=0;//bandera de par de sensores bloqueados detectando, no significa contador bloqueado
       if( (!en_b1)&&(!en_b2)&&(!en_b3) ){//si no habia un bloqueo antes
            lcd_gotoxy(10,1);//cronometro para limpiar
            lcd_putc("BL");
            cta_bloq1++;
            write_ext_eeprom(9,cta_bloq1);//bloq1
            if( (master==1)&&(j==0) ) solicitar();
            else if(j==0) envio_master();
       }
       en_b1=1;//bandera de bloqueo detectado en par 1
   }
   if( (bloq_p2)&&(tim_p2>=time_bloqueo)&&(!en_b2) ) {//bloqueo de par 2 por m�s de X segundos
       bloq_p2=0;
       if( (!en_b1)&&(!en_b2)&&(!en_b3) ){
           lcd_gotoxy(10,1);//cronometro para limpiar
           lcd_putc("BL");
           cta_bloq2++;
           write_ext_eeprom(10,cta_bloq2);//bloq2
           if( (master==1)&&(j==0) ) solicitar();
           else if(j==0) envio_master();
       }
       en_b2=1;
   }
   if( (bloq_p3)&&(tim_p3>=time_bloqueo)&&(!en_b3) ) {//bloqueo de par 3 por m�s de X segundos
       bloq_p3=0;
       if( (!en_b1)&&(!en_b2)&&(!en_b3) ){
            lcd_gotoxy(10,1);//cronometro para limpiar
            lcd_putc("BL");
            cta_bloq3++;
            write_ext_eeprom(11,cta_bloq3);//bloq3
            if( (master==1)&&(j==0) ) solicitar();
            else if(j==0) envio_master();
       }
       en_b3=1;
   }//end deteccion
   if( (!bloq_p3)&&(!bloq_p2)&&(!bloq_p1) ){//Si todos los pares estan desbloqueados limpia estado de bloqueo LCD
      lcd_gotoxy(10,1);
      lcd_putc("  ");
   }
   
   if( (time_clr>=900)&&(f_clr) ){//refresca LCD
        lcd_cuentas();
         f_clr=0;
   }
   sensores();//Revisa estado de los sensores
      
   restart_wdt();
 }//end true
}//end main

void solicitar(){//Solicita cuenta al poste esclavo
   fprintf(monitor,"SOLICITADO\r");//4
   time_envio=0;
   flag_enviar=1;
   cta_enviar=0;
}

void envio_master(){//reporta al maestro
    int16 envit;
    envit=(entraront + salieront)/2;
    cta_bloqueo=cta_bloq1+cta_bloq2+cta_bloq3;
    clear_lcd1=1;
    time_lcd1=0;
    lcd_gotoxy(9,3);//cronometro para limpiar
    lcd_putc(0xAB);
    //fprintf(monitor,"PASAJEROS:%04Ld/%04Ld/%03Ld/%04Lu/\r",entraront,salieront,cta_bloqueo,envit );
    fprintf(monitor,"PASAJEROS:\r");
}

int convertir_to_entero(char *cadena) {
   int valor = 0;
        if(cadena=='0') valor=0;
   else if(cadena=='1') valor=1;
   else if(cadena=='2') valor=2;
   else if(cadena=='3') valor=3;
   else if(cadena=='4') valor=4;
   else if(cadena=='5') valor=5;
   else if(cadena=='6') valor=6;
   else if(cadena=='7') valor=7;
   else if(cadena=='8') valor=8;
   else if(cadena=='9') valor=9;
   return valor;
}

void enviop1(){//Convierte a enteros la cadena recibida por el poste esclavo
   disable_interrupts(INT_RDA);
   entran1=0;
   salen1=0;
   temporal1=0;
   temporal2=0;
   temporal3=0;
   tempo_tn=0;
   pasajet=0;
   memset(entran, 0, sizeof(entran));//entran=0;
   memset(salen, 0, sizeof(salen));//salen=0;
   memset(bloqueado, 0, sizeof(bloqueado));//bloqueado=0;
   memset(bloqueado, 0, sizeof(tempo_tc));//total
   i=10;
   ii=0;
   strcpy(term,":/");
   ptr = strtok(XX, term);

    int conteo_wl=0;
   
         strcpy(data1,ptr);                        // 1ra Palabra
         strcpy(data2, strtok(NULL , term));       // 2ra Palabra
         strcpy(data3, strtok(NULL , term));       // 3ra Palabra
         strcpy(data4, strtok(NULL , term));       // 4ra Palabra
         strcpy(data5, strtok(NULL , term));       // 5ra Palabra
         ptr = strtok(0, term);
         
         
         conteo_wl++;
         sub_atras = atoi32(data2);
         baj_atras = atoi32(data3);
         temporal3 = atoi32(data4);
         tempo_tn  = atoi32(data5);
         /*
         printf(lcd_putc,"\f");
         lcd_gotoxy(1,1);
         printf(lcd_putc,"%s->%ld",data2,sub_atras);
         lcd_gotoxy(1,2);
         printf(lcd_putc,"%s->%ld",data3,baj_atras);
         lcd_gotoxy(1,3);
         printf(lcd_putc,"%s->%ld",data4,temporal3);
         lcd_gotoxy(1,4);
         printf(lcd_putc,"%s->%ld",data5,tempo_tn);
        delay_ms(4000);
        */
         
         lcd_gotoxy(14,3);
         printf(lcd_putc,"S2:%Ld ",sub_atras);
         lcd_gotoxy(14,4);
         printf(lcd_putc,"B2:%Ld ",baj_atras);
         

}

void envio(){//crea paquete para enviar al modulo GPRS
   ////////////// Detectando envio total
   /*
   if(convertir_to_entero(tempo_tc[0]) >0) for(i=0;i< convertir_to_entero(tempo_tc[0]);i++) tempo_tn=tempo_tn+1000;
   if(convertir_to_entero(tempo_tc[1]) >0) for(i=0;i< convertir_to_entero(tempo_tc[1]);i++) tempo_tn=tempo_tn+100;
   if(convertir_to_entero(tempo_tc[2]) >0) for(i=0;i< convertir_to_entero(tempo_tc[2]);i++) tempo_tn=tempo_tn+10;
   tempo_tn=tempo_tn+convertir_to_entero(tempo_tc[3]);
   */
   ////////////////////////////////////////////////////
   /*
   if(convertir_to_entero(entran[0]) >0) for(i=0;i< convertir_to_entero(entran[0]);i++) temporal1=temporal1+1000;
   if(convertir_to_entero(entran[1]) >0) for(i=0;i< convertir_to_entero(entran[1]);i++) temporal1=temporal1+100;
   if(convertir_to_entero(entran[2]) >0) for(i=0;i< convertir_to_entero(entran[2]);i++) temporal1=temporal1+10;
   sub_atras=temporal1=temporal1+convertir_to_entero(entran[3]);
   
   if(convertir_to_entero(salen[0]) >0) for(i=0;i< convertir_to_entero(salen[0]);i++) temporal2=temporal2+1000;
   if(convertir_to_entero(salen[1]) >0) for(i=0;i< convertir_to_entero(salen[1]);i++) temporal2=temporal2+100;
   if(convertir_to_entero(salen[2]) >0) for(i=0;i< convertir_to_entero(salen[2]);i++) temporal2=temporal2+10;
   baj_atras=temporal2=temporal2+convertir_to_entero(salen[3]);
   */
   /*
   if(convertir_to_entero(bloqueado[0]) >0) for(i=0;i< convertir_to_entero(bloqueado[0]);i++) temporal3=temporal3+100;
   if(convertir_to_entero(bloqueado[1]) >0) for(i=0;i< convertir_to_entero(bloqueado[1]);i++) temporal3=temporal3+10;
   temporal3=temporal3+convertir_to_entero(bloqueado[2]);
   */
   
   temporal1 = sub_atras;
   temporal2 = baj_atras;
   entran1=(entraront+salieront)/2;
   
   total2_t=(temporal1+temporal2)/2;

   leer_conta2();
   if( total2_t== tempo_tn ) {//dato recibido correcto
         salian=salen1=total2_t;
         graba_conta2();
         error=0;
   }
   else error=2;
   /////////////
   pasajet=entran1+salen1;
   cta_bloqueo=cta_bloq1+cta_bloq2+cta_bloq3;
   //fprintf(GPS,"ACC+01:%04Lu,%04Lu,%04Lu,%04Lu,%04Lu,%03Lu,%03Lu,%02u,\r\n",pasajet,entraront,salieront,temporal1,temporal2,cta_bloqueo,temporal3,error);
   fprintf(GPS,"ACC+01:%04Lu,%04Lu,%04Lu,%03Lu,%03Lu,%02u,\r\n",pasajet,entran1,salen1,cta_bloqueo,temporal3,error);
   lcd_cuentas();

   lcd_gotoxy(9,3);//cronometro para limpiar
   lcd_putc(0xAB);

   clear_lcd1=1;
   time_lcd1=0;
   /////////////////////////////
   time_envio=0;//reinicia el tiempo para el siguiente envio
   transmitir=0;
   enable_interrupts(INT_RDA);
}

void envia2(){//Crea paquete para enviar a modulo GPRS cuando no hay conexion con esclavo
   disable_interrupts(INT_RDA);
   cta_bloqueo=cta_bloq1+cta_bloq2+cta_bloq3;
   memset(entran, 0, sizeof(entran));//entran=0;
   memset(salen, 0, sizeof(salen));//salen=0;
   memset(bloqueado, 0, sizeof(bloqueado));//bloqueado=0;
   i=10;
   ii=0;
   entran1=(entraront+salieront)/2;
   leer_conta2();
   pasajet=entran1+salian;
   error=1;
   //fprintf(GPS,"ACC+01:%04Lu,%04Lu,%04Lu,DESC,DESC,%03Lu,%03Lu,%02u,\r\n",pasajet,entraront,salieront,cta_bloqueo,temporal3,error);
   fprintf(GPS,"ACC+01:%04Lu,%04Lu,DESC,%03Lu,000,%02u,\r\n",pasajet,entran1,cta_bloqueo,error);
//!   lcd_gotoxy(1,4);
//!   printf(lcd_putc,"T:%04Lu BS:%03Lu ",pasajet,cta_bloqueo);
   lcd_gotoxy(9,3);//cronometro para limpiar
   lcd_putc(0xAB);
   lcd_gotoxy(9,4);//cronometro para limpiar
   lcd_putc("DESC");
   clear_lcd1=1;
   time_lcd1=0;
   /////////////////////////////
   time_envio=0;//reinicia el tiempo para el siguiente envio
   flag_enviar=0;
   enable_interrupts(INT_RDA);
}

void limpiar(){//borra cuentas de subidas, bajadas y bloqueos
   tem_entraront=0;
   tem_salieront=0;
   salian=0;
   graba_conta2();
   leer_conta2();
   pasaje=0;
   pasajet=0;
   salieront=0;
   entraront=0;
   write_ent();
   write_sal();
   cta_bloqueo=0;
   cta_bloq1=0;
   cta_bloq2=0;
   cta_bloq3=0;
   write_ext_eeprom(9,cta_bloq1);//bloq1
   write_ext_eeprom(10,cta_bloq2);//bloq2
   write_ext_eeprom(11,cta_bloq3);//bloq3
   lcd_putc("\f");
   lcd_cuentas();
}

void detecta_suma(){//Realiza suma de entradas o salidas
    if( (!detec1)&&(!detec2)&&(!detec3)&&(!detec4)&&(!detec5)&&(!detec6) ){//suma sin bloqueos
      sumae1=fse1+fse2+fse3;
      sumas1=fss1+fss2+fss3;
//!      lcd_gotoxy(9,4);//habilitar unicamente para hacer diagnosticos
//!      printf(lcd_putc,"Se:%u Ss:%u",sumae1,sumas1);//habilitar unicamente para hacer diagnosticos
      if( (sumae1>=2)||(sumas1>=2) ){
         if(sumae1>sumas1) {
            entraront++;
            write_ent();
         }
         else{
            salieront++;
            write_sal();
         }
      }
      sumae1=fse1=fse2=fse3=0;
      sumas1=fss1=fss2=fss3=0;
      //envio_PC();
   }//fin suma sin bloqueos
   else if( (en_b1)||(en_b2)||(en_b3) ){//contar con bloqueos
            lim_b= en_b1+ en_b2+ en_b3;
   
            if(lim_b==1){//un solo bloqueo
               if(en_b1){
                  if( (!detec3)&&(!detec4)&&(!detec5)&&(!detec6) ){
                     if(sen_ent==1){
                        entraront++;
                        write_ent();
                     }
                     else{
                        salieront++;
                        write_sal();
                     }
                  }//fin sensores 0
               }//fin bloqueo par 1
               else if(en_b2){//bloqueo en par 2
                  if( (!detec1)&&(!detec2)&&(!detec5)&&(!detec6) ){
                     if(sen_ent){
                        entraront++;
                        write_ent();
                     }
                     else{
                        salieront++;
                        write_sal();
                     }
                  }//fin sensores 0
               }//fin par 2
               else if(en_b3){//bloqueao en par 3
                  if( (!detec3)&&(!detec4)&&(!detec1)&&(!detec2) ){
                     if(sen_ent){
                        entraront++;
                        write_ent();
                     }
                     else{
                        salieront++;
                        write_sal();
                     }
                  }
               }//fin par 3
            }//fin un solo bloqueo
//-----------------------------------------------------------------------------
            else{//mas de un bloqueo
               sumae1=fse1+fse2+fse3;
               sumas1=fss1+fss2+fss3;
               //lcd_gotoxy(9,3);//habilitar unicamente para hacer diagnosticos
               //printf(lcd_putc,"Se:%u Ss:%u",sumae1,sumas1);//habilitar unicamente para hacer diagnosticos
               if( (sumae1>=1)||(sumas1>=1) ){
                  if(sumae1>sumas1) {
                     entraront++;
                     write_ent();
                  }
                  else{
                     salieront++;
                     write_sal();
                  }
               }
            }//fin mas de un bloqueo
               sumae1=fse1=fse2=fse3=0;
               sumas1=fss1=fss2=fss3=0;
//-----------------------------------------------------------------------------
   }//end bloqueos
}

void finsuma(){//Pinta en LCD la nueva cifra de entrada o salida

      pasaje=(entraront+salieront)/2;
      lcd_cuentas();
      sen_ent=0;
      sen_sal=0;
      fse1=fse2=fse3=0;
      fss1=fss2=fss3=0;
}

void contando1(){//Revisa maquina de estados para par 1
   if(entra_temp1!=edo1) {
      entra_temp1=edo1;
      enable1=1;
   }
   if (enable1==1){
         switch (edo1) {
            case 1: {//LLEGA PERSONA EN ENTRADA1
                     break;}
            case 5: {//PERSONA A ENTRADO
                     if(!fss1) fse1=1;//si no ha salido antes
                     else fss1=0;
                     detecta_suma();
                     break;}
            case 6: {//entraba y se regresa
                     detecta_suma();
                     break;}
            case 7: {//PERSONA DETECTADO -SALIENDO
                     break;}
            case 11: {//PERSONA A SALIDO
                     if(!fse1) fss1=1;//si no ha entrado antes
                     else fse1=0;
                     detecta_suma();
                     break;}
            case 12: {//SALIA y se regresa
                     detecta_suma();
                     break;}
         }
      enable1=0;
      entra_act1=0;
   }
}

void contando2(){//Revisa maquina de estados para par 2
   if(entra_temp2!=edo2) {
      entra_temp2=edo2;
      enable2=1;
   }
   if (enable2==1){
         switch (edo2) {
            case 1: {//LLEGA PERSONA EN ENTRADA1
                     break;}
            case 5: {//PERSONA A ENTRADO
                     if(!fss2) fse2=1;
                     else fss2=0;
                     detecta_suma();
                     break;}
            case 6: {//entraba y se regresa
                     detecta_suma();
                     break;}
            case 7: {//PERSONA DETECTADO -SALIENDO
                     break;}
            case 11: {//PERSONA A SALIDO
                     if(!fse2) fss2=1;
                     else fse2=0;
                     detecta_suma();
                     break;}
            case 12: {//SALIA y se regresa
                     detecta_suma();
                     break;}
         }
      enable2=0;
      entra_act2=0;
   }
}

void contando3(){//Revisa maquina de estados para par 3
   if(entra_temp3!=edo3) {
      entra_temp3=edo3;
      enable3=1;
   }
   if (enable3==1){
         switch (edo3) {
            case 1: {//LLEGA PERSONA EN ENTRADA1
                     break;}
            case 5: {//PERSONA A ENTRADO
                     if(!fss3) fse3=1;
                     else fss3=0;
                     detecta_suma();
                     break;}
            case 6: {//entraba y se regresa
                     detecta_suma();
                     break;}
            case 7: {//PERSONA DETECTADO -SALIENDO
                     break;}
            case 11: {//PERSONA A SALIDO
                     if(!fse3) fss3=1;
                     else fse3=0;
                     detecta_suma();
                     break;}
            case 12: {//SALIA y se regresa
                     detecta_suma();
                     break;}
         }
      enable3=0;
      entra_act3=0;
   }
}

void sensando1(){//Define estados para la maquina de estados del par 1
/////normal///////
if((entra_act1==0)&&(detec1==1)&&(detec2==0)&&(entra1==0)&&(sale1==0)){//LLEGA PERSONA EN ENTRADA1
   if( (sen_ent==0)&&(sen_sal==0) ) sentidos();
   lcd_gotoxy(8,2);
   lcd_putc(0x7F);//flecha del sentido
   des1=1;
   ////
   edo1=1;
   entra1=1;
   edoa1=1;
   entra_act1=1;}
///prioridad
else if((entra_act1==0)&&(detec1==1)&&(detec2==1)&&(entra1==1)&&(sale1==0)){//PERSONA ENTRANDO
   edo1=3;
   edoa1=0;
   entra_act1=1;}
else if((entra_act1==0)&&(detec1==0)&&(detec2==1)&&(entra1==1)&&(sale1==0)){//PERSONA CASI TERMINA DE ENTRAR
   edo1=4;
   entra_act1=1;}
else if((entra_act1==0)&&(detec1==0)&&(detec2==0)&&(edoa1==0)&&(entra1==1)&&(sale1==0)){//PERSONA A ENTRADO
   lcd_gotoxy(8,2);
   lcd_putc(" ");
   des1=0;
   if(entra_temp1==4){
      edo1=5;
      entra1=0;
      entra_act1=1;
   }
   else {//entraba y se regresa
      des1=0;
      edo1=6;
      entra1=0;
      edoa1=0;
      entra_act1=1;  
   }
}
else if((entra_act1==0)&&(detec1==0)&&(detec2==0)&&(edoa1==1)&&(entra1==1)&&(sale1==0)){//entraba y se regresa
   lcd_gotoxy(8,2);
   lcd_putc(" ");
   des1=0;
   edo1=6;
   entra1=0;
   edoa1=0;
   entra_act1=1;}
/////
else if((entra_act1==0)&&(detec1==1)&&(detec2==0)&&(entra1==1)&&(sale1==0)){
   edo1=2;
   edoa1=1;
   entra_act1=1;}//checar prioridad
//////sentido inverso///
else if((entra_act1==0)&&(detec1==0)&&(detec2==1)&&(entra1==0)&&(sale1==0)){//PERSONA DETECTADO -SALIENDO POR ENTRADA
   if( (sen_ent==0)&&(sen_sal==0) ) sentidos();
   lcd_gotoxy(8,2);
   lcd_putc(0x7E);//flecha del sentido
   des1=1;
   edo1=7;
   sale1=1;
   entra_act1=1;}
else if((entra_act1==0)&&(detec1==0)&&(detec2==1)&&(entra1==0)&&(sale1==1)){//SALIENDO POR ENTRADA PARTE 1
   edo1=8;
   edoa1=1;
   entra_act1=1;}
else if((entra_act1==0)&&(detec1==1)&&(detec2==1)&&(entra1==0)&&(sale1==1)){//SALIENDO POR ENTRADA PARTE 2
   edo1=9;
   edoa1=0;
   entra_act1=1;}
else if((entra_act1==0)&&(detec1==1)&&(detec2==0)&&(entra1==0)&&(sale1==1)){//CASI TERMINA DE SALIR
   edo1=10;
   entra_act1=1;}
else if((entra_act1==0)&&(detec1==0)&&(detec2==0)&&(edoa1==0)&&(entra1==0)&&(sale1==1)){//PERSONA A SALIDO POR ENTRADA
   lcd_gotoxy(8,2);
   lcd_putc(" ");
   if(entra_temp1==10) {
      des1=0;
      edo1=11;
      sale1=0;
      entra_act1=1;}
   else {
      des1=0;
      edo1=12;
      sale1=0;
      edoa1=0;
      entra_act1=1;
   }
}
else if((entra_act1==0)&&(detec1==0)&&(detec2==0)&&(edoa1==1)&&(entra1==0)&&(sale1==1)){//SALIA y se regresa
   lcd_gotoxy(8,2);
   lcd_putc(" ");
   des1=0;
   edo1=12;
   sale1=0;
   edoa1=0;
   entra_act1=1;}
}

void sensando2(){//Define estados para la maquina de estados del par 2
/////normal///////
if((entra_act2==0)&&(detec3==1)&&(detec4==0)&&(entra2==0)&&(sale2==0)){//LLEGA PERSONA EN ENTRADA1
   if( (sen_ent==0)&&(sen_sal==0) ) sentidos();
   lcd_gotoxy(8,3);
   lcd_putc(0x7F);
   des2=1;
   edo2=1;
   entra2=1;
   edoa2=1;
   entra_act2=1;}
///prioridad
else if((entra_act2==0)&&(detec3==1)&&(detec4==1)&&(entra2==1)&&(sale2==0)){//PERSONA ENTRANDO
   edo2=3;
   edoa2=0;
   entra_act2=1;}
else if((entra_act2==0)&&(detec3==0)&&(detec4==1)&&(entra2==1)&&(sale2==0)){//PERSONA CASI TERMINA DE ENTRAR
   edo2=4;
   entra_act2=1;}
else if((entra_act2==0)&&(detec3==0)&&(detec4==0)&&(edoa2==0)&&(entra2==1)&&(sale2==0)){//PERSONA A ENTRADO
   lcd_gotoxy(8,3);
   lcd_putc(" ");
   if(entra_temp2==4){
      des2=0;
      edo2=5;
      entra2=0;
      entra_act2=1;}
   else{
      des2=0;
      edo2=6;
      entra2=0;
      edoa2=0;
      entra_act2=1;}
}
else if((entra_act2==0)&&(detec3==0)&&(detec4==0)&&(edoa2==1)&&(entra2==1)&&(sale2==0)){//entraba y se regresa
   lcd_gotoxy(8,3);
   lcd_putc(" ");
   des2=0;
   edo2=6;
   entra2=0;
   edoa2=0;
   entra_act2=1;}
/////
else if((entra_act2==0)&&(detec3==1)&&(detec4==0)&&(entra2==1)&&(sale2==0)){
   edo2=2;
   edoa2=1;
   entra_act2=1;}//checar prioridad
//////sentido inverso///
else if((entra_act2==0)&&(detec3==0)&&(detec4==1)&&(entra2==0)&&(sale2==0)){//PERSONA DETECTADO -SALIENDO POR ENTRADA
   if( (sen_ent==0)&&(sen_sal==0) ) sentidos();
   lcd_gotoxy(8,3);
   lcd_putc(0x7E);//flecha del sentido
   des2=1;
   edo2=7;
   sale2=1;
   entra_act2=1;}
else if((entra_act2==0)&&(detec3==0)&&(detec4==1)&&(entra2==0)&&(sale2==1)){//SALIENDO POR ENTRADA PARTE 1
   edo2=8;
   edoa2=1;
   entra_act2=1;}
else if((entra_act2==0)&&(detec3==1)&&(detec4==1)&&(entra2==0)&&(sale2==1)){//SALIENDO POR ENTRADA PARTE 2
   edo2=9;
   edoa2=0;
   entra_act2=1;}
else if((entra_act2==0)&&(detec3==1)&&(detec4==0)&&(entra2==0)&&(sale2==1)){//CASI TERMINA DE SALIR
   edo2=10;
   entra_act2=1;}
else if((entra_act2==0)&&(detec3==0)&&(detec4==0)&&(edoa2==0)&&(entra2==0)&&(sale2==1)){//PERSONA A SALIDO POR ENTRADA
   lcd_gotoxy(8,3);
   lcd_putc(" ");
   if(entra_temp2==10){
      des2=0;
      edo2=11;
      sale2=0;
      entra_act2=1;}
   else{
      des2=0;
      edo2=12;
      sale2=0;
      edoa2=0;
      entra_act2=1;}
}
else if((entra_act2==0)&&(detec3==0)&&(detec4==0)&&(edoa2==1)&&(entra2==0)&&(sale2==1)){//SALIA y se regresa
   lcd_gotoxy(8,3);
   lcd_putc(" ");
   des2=0;
   edo2=12;
   sale2=0;
   edoa2=0;
   entra_act2=1;}
}

void sensando3(){//Define estados para la maquina de estados del par 3
/////normal///////
if((entra_act3==0)&&(detec5==1)&&(detec6==0)&&(entra3==0)&&(sale3==0)){//LLEGA PERSONA EN ENTRADA1
   if( (sen_ent==0)&&(sen_sal==0) ) sentidos();
   lcd_gotoxy(8,4);
   lcd_putc(0x7F);
   des3=1;
   edo3=1;
   entra3=1;
   edoa3=1;
   entra_act3=1;}
///prioridad
else if((entra_act3==0)&&(detec5==1)&&(detec6==1)&&(entra3==1)&&(sale3==0)){//PERSONA ENTRANDO
   edo3=3;
   edoa3=0;
   entra_act3=1;}
else if((entra_act3==0)&&(detec5==0)&&(detec6==1)&&(entra3==1)&&(sale3==0)){//PERSONA CASI TERMINA DE ENTRAR
   edo3=4;
   entra_act3=1;}
else if((entra_act3==0)&&(detec5==0)&&(detec6==0)&&(edoa3==0)&&(entra3==1)&&(sale3==0)){//PERSONA A ENTRADO
   lcd_gotoxy(8,4);
   lcd_putc(" ");
   if(entra_temp3==4){
      des3=0;
      edo3=5;
      entra3=0;
      entra_act3=1;}
   else{
      des3=0;
      edo3=6;
      entra3=0;
      edoa3=0;
      entra_act3=1;}
}
else if((entra_act3==0)&&(detec5==0)&&(detec6==0)&&(edoa3==1)&&(entra3==1)&&(sale3==0)){//entraba y se regresa
   lcd_gotoxy(8,4);
   lcd_putc(" ");
   des3=0;
   edo3=6;
   entra3=0;
   edoa3=0;
   entra_act3=1;}
/////
else if((entra_act3==0)&&(detec5==1)&&(detec6==0)&&(entra3==1)&&(sale3==0)){
   //sentidos();
   edo3=2;
   edoa3=1;
   entra_act3=1;}//checar prioridad
//////sentido inverso///
else if((entra_act3==0)&&(detec5==0)&&(detec6==1)&&(entra3==0)&&(sale3==0)){//PERSONA DETECTADO -SALIENDO POR ENTRADA
   if( (sen_ent==0)&&(sen_sal==0) ) sentidos();
   lcd_gotoxy(8,4);
   lcd_putc(0x7E);//flecha del sentido
   des3=1;
   edo3=7;
   sale3=1;
   entra_act3=1;}
else if((entra_act3==0)&&(detec5==0)&&(detec6==1)&&(entra3==0)&&(sale3==1)){//SALIENDO POR ENTRADA PARTE 1
   edo3=8;
   edoa3=1;
   entra_act3=1;}
else if((entra_act3==0)&&(detec5==1)&&(detec6==1)&&(entra3==0)&&(sale3==1)){//SALIENDO POR ENTRADA PARTE 2
   edo3=9;
   edoa3=0;
   entra_act3=1;}
else if((entra_act3==0)&&(detec5==1)&&(detec6==0)&&(entra3==0)&&(sale3==1)){//CASI TERMINA DE SALIR
   edo3=10;
   entra_act3=1;}
else if((entra_act3==0)&&(detec5==0)&&(detec6==0)&&(edoa3==0)&&(entra3==0)&&(sale3==1)){//PERSONA A SALIDO POR ENTRADA
   lcd_gotoxy(8,4);
   lcd_putc(" ");
   if(entra_temp3==10){
      des3=0;
      edo3=11;
      sale3=0;
      entra_act3=1;}
   else{
      des3=0;
      edo3=12;
      sale3=0;
      edoa3=0;
      entra_act3=1;}
}
else if((entra_act3==0)&&(detec5==0)&&(detec6==0)&&(edoa3==1)&&(entra3==0)&&(sale3==1)){//SALIA y se regresa
   lcd_gotoxy(8,4);
   lcd_putc(" ");
   des3=0;
   edo3=12;
   sale3=0;
   edoa3=0;
   entra_act3=1;}
}

void sensores(){//Detecta estado (bloqueado o libre) de los sensores
//sensor1
ent1=input(entrada01);
if(ent1==1){
   sub_cta1=0;
   detec1=0;
   bloq_p1=0;//deshabilita bloqueo par1
   tim_p1=0;
   if(en_b1) {//pregunta si esta sonando el par1
      output_low(salida01);//apaga alarma bloqueo
      en_b1=0;
   }
}
else {
   if(sub_cta1>=s_cta) detec1=1;//revisa si se interumpio la luz en (5ms*4)
   if(sub_d1!=detec1) {//si, cambia de estado el sensor
      sub_d1=detec1;
      enable1=1;
      lcd_gotoxy(5,2);
      printf(lcd_putc,"1:%d",sub_d1);
      sensando1();
      contando1();
   }
}
//sensor2
ent2=input(entrada02);
if(ent2==1){
   output_high(salida02);//
   sub_cta2=0;
   detec2=0;
   bloq_p1=0;//deshabilita bloqueo
   tim_p1=0;
   if(en_b1) {
      output_low(salida01);//apaga alarma bloqueo
      en_b1=0;
   }
}
else {
   output_low(salida02);
   if(sub_cta2>=s_cta) detec2=1;
   if(sub_d2!=detec2) {
      sub_d2=detec2;
      enable1=1;
      lcd_gotoxy(1,2);
      printf(lcd_putc,"2:%d ",sub_d2);
      sensando1();
      contando1();
   }
}
//sensor3
ent3=input(entrada03);
if(ent3==1){
   sub_cta3=0;
   detec3=0;
   bloq_p2=0;//deshabilita bloqueo
   tim_p2=0;
   if(en_b2) {
      output_low(salida01);//apaga alarma bloqueo
      en_b2=0;
   }
}
else {
   if(sub_cta3>=s_cta) detec3=1;
   if(sub_d3!=detec3) {
      sub_d3=detec3;
      enable2=1;
      lcd_gotoxy(5,3);
      printf(lcd_putc,"3:%d",sub_d3);
      sensando2();
      contando2();
   }
}
//sensor4
ent4=input(entrada04);
if(ent4==1){
   output_high(salida02);//
   sub_cta4=0;
   detec4=0;
   bloq_p2=0;//deshabilita bloqueo
   tim_p2=0;
   if(en_b2) {
      output_low(salida01);//apaga alarma bloqueo
      en_b2=0;
   }
}
else {
    output_low(salida02);
    if(sub_cta4>=s_cta) detec4=1;
    if(sub_d4!=detec4) {
      sub_d4=detec4;
      enable2=1;
      lcd_gotoxy(1,3);
      printf(lcd_putc,"4:%d ",sub_d4);
      sensando2();
      contando2();
   }
}
//sensor5
ent5=input(entrada05);
if(ent5==1){
   sub_cta5=0;
   detec5=0;
   bloq_p3=0;//deshabilita bloqueo
   tim_p3=0;
   if(en_b3) {
      output_low(salida01);//apaga alarma bloqueo
      en_b3=0;
   }
}
else {
    if(sub_cta5>=s_cta) detec5=1;
    if(sub_d5!=detec5) {
      sub_d5=detec5;
      enable3=1;
      lcd_gotoxy(5,4);
      printf(lcd_putc,"5:%d",sub_d5);
      sensando3();
      contando3();
   }
}
//sensor6
ent6=input(entrada06);
if(ent6==1){
   output_high(salida02);//
   sub_cta6=0;
   detec6=0;
   bloq_p3=0;//deshabilita bloqueo
   tim_p3=0;
   if(en_b3) {
      output_low(salida01);//apaga alarma bloqueo
      en_b3=0;
   }
}
else {
   output_low(salida02);//
   if(sub_cta6>=s_cta)   detec6=1;
   if(sub_d6!=detec6) {
      sub_d6=detec6;
      enable3=1;
      lcd_gotoxy(1,4);
      printf(lcd_putc,"6:%d ",sub_d6);
      sensando3();
      contando3();
   }
}
//////////////////////////
if( (detec1==1)&&(detec2==1)&&(bloq_p1==0) ){
         bloq_p1=1;
         tim_p1=0;
      }
if( (detec3==1)&&(detec4==1)&&(bloq_p2==0) ){
         bloq_p2=1;
         tim_p2=0;
      }
if( (detec5==1)&&(detec6==1)&&(bloq_p3==0) ){
         bloq_p3=1;
         tim_p3=0;
      }
}//end sensores

void sentidos(){//Define si la persona esta entrando o saliendo
 if( (!sen_ent)&&(!sen_sal) ){
   if ( ( (detec1==1)&&(detec3==1) )|| ( (detec1==1)&&(detec5==1) )|| ( (detec3==1)&&(detec5==1) ) ) {
      sen_ent=1;
//!      lcd_gotoxy(17,4);
//!      lcd_putc("Sub");
   }
   if ( ( (detec2==1)&&(detec4==1) )|| ( (detec2==1)&&(detec6==1) )|| ( (detec4==1)&&(detec6==1) ) ) {
      sen_sal=1;
//!      lcd_gotoxy(17,4);
//!      lcd_putc("Baj");
   }
 }
}

void rd_eeprom(){//Revisa si cadena recibida es un instruccion
   a=b=0;
   n=word_size;//
   //i=0;
   //i=2;
   while ((b <=data_set)&&(a==0)){//NUMERO TOTAL DE INSTRUCCIONES
       i=0;
       b++;
       while (i < word_size) {//word_size=10
           memory[i] = read_eeprom(n+i);
           
           if (memory[i] != XX[i])
               break;
           i++;
           if (i==word_size) a=1;
       }
       //fprintf(monitor,"memory:%s\r",memory);
       n=n+word_size;//WORD_SIZE=30
       restart_wdt();
   }
}

void rd_eeprom_inicio(){//
   a=b=0;
   n=word_size;//
   
   while (b <=data_set){//NUMERO TOTAL DE INSTRUCCIONES
       i=0;
       b++;
       while (i < word_size) {//word_size=10
           memory[i] = read_eeprom(n+i);
           i++;
           if (i==word_size) a=1;
       }
       fprintf(monitor,"memory:%s\r",memory);
       n=n+word_size;//WORD_SIZE=30
   }
}

void lcd_cuentas(){//Escribe las cuenta en LCD 
    lcd_gotoxy(1,1);
    if(master) {
       printf(lcd_putc,"1 %c%c%c%c%c ",version[0],version[1],version[2],version[3],version[4]);
       lcd_gotoxy(14,1);
       printf(lcd_putc,"S1:%Ld ",entraront);
       lcd_gotoxy(14,2);
       printf(lcd_putc,"B1:%Ld ",salieront);
       lcd_gotoxy(14,3);
       printf(lcd_putc,"S2:%Ld ",sub_atras);
       lcd_gotoxy(14,4);
       printf(lcd_putc,"B2:%Ld ",baj_atras);
    }
    else{
       printf(lcd_putc,"2 %c%c%c%c%c ",version[0],version[1],version[2],version[3],version[4]);
       lcd_gotoxy(14,1);
       lcd_putc("S1:    ");
       lcd_gotoxy(14,2);
       lcd_putc("B1:    ");
       lcd_gotoxy(14,3);
       printf(lcd_putc,"S2:%Ld ",entraront);
       lcd_gotoxy(14,4);
       printf(lcd_putc,"B2:%Ld ",salieront);
    }
    
}

void write_ent(){//Graba en eeprom las subidas del poste
   disable_interrupts(GLOBAL);
   write_ext_eeprom(2,entraront);//parte baja
   write_ext_eeprom(1,(entraront>>8) );//parte alta
   enable_interrupts(GLOBAL);
   finsuma();
}

void write_sal(){//Graba en eeprom las bajadas del poste
   disable_interrupts(GLOBAL);
   write_ext_eeprom(4,salieront);//parte baja
   write_ext_eeprom(3,(salieront>>8) );//parte alta
   enable_interrupts(GLOBAL);
   finsuma();
}

void entraron_total(){//Graba en eeprom el total de subidas
   disable_interrupts(GLOBAL);
   unsigned int reg_h=0,reg_l=0;
   unsigned int16 total=0;
   
   reg_h=read_ext_eeprom(1);
   reg_l=read_ext_eeprom(2);
   total=reg_h;
   total=(total<<8)|reg_l;
   entraront=total;
   enable_interrupts(GLOBAL);
}

void salieron_total(){//Graba en eeprom el total de bajadas
   disable_interrupts(GLOBAL);
   unsigned int reg_h=0,reg_l=0;
   unsigned int16 total=0;
   
   reg_h=read_ext_eeprom(3);
   reg_l=read_ext_eeprom(4);
   total=reg_h;
   total=(total<<8)|reg_l;
   salieront=total;
   enable_interrupts(GLOBAL);
}

void graba_conta2(){//Graba en eeprom el total de pasajes de poste esclavo
   disable_interrupts(GLOBAL);
   unsigned int reg_hc=0,reg_lc=0;
   unsigned int16 totalc=0;
   totalc=0;
   reg_lc=0;
   reg_hc=0;
   
   totalc=salian;
   reg_lc=totalc;
   reg_hc=totalc>>8;
   write_ext_eeprom(20,reg_hc);
   write_ext_eeprom(21,reg_lc);
   enable_interrupts(GLOBAL);
}

void leer_conta2(){//Carga de eeprom el total de pasajes de poste esclavo
   disable_interrupts(GLOBAL);
   unsigned int reg_hc=0,reg_lc=0;
   unsigned int16 totalc=0;
   
   reg_hc=read_ext_eeprom(20);
   reg_lc=read_ext_eeprom(21);
   totalc=reg_hc;
   totalc=totalc<<8;
   totalc=totalc|reg_lc;
   salian=totalc;
   enable_interrupts(GLOBAL);
}

void reset(){
 switch ( restart_cause() ) {
      case WDT_TIMEOUT:
      {  //lcd_putc("REINICIO-WD");//
         break;}
      case MCLR_FROM_RUN://avisa que reinicio por master clear
      {  
         fprintf(monitor,"SERIALTEST\r\n");
         break;}
      case BROWNOUT_RESTART://avisa que el pic reinicio por un voltaje menor a 4v
      {
         break;
      }
      case NORMAL_POWER_UP:{
         break;
      }//END MODO
   }
}

//// evalua el comando obetenido del serial nato ///////////////////
int evaluar_string(char *command[]){
   int conteo_command=0;
   int iresult;
//// recorremos los 11 comandos definidos en textosearxch[] ////////
   while(conteo_command<=10){
   iresult=strcoll(textosearch[conteo_command],command);
//// si se encontr� el comando en la lista entonces deja de buscar //
   if(iresult==0){
   break;
   }
   conteo_command++;
   }

//// si parte del comando es o empieza con "ASAJEROS" entra caso 5 //
      if(XX[1]=='A' && XX[2]=='S' && XX[3]=='A' && XX[4]=='J' && XX[5]=='E' && XX[6]=='R' && XX[7]=='O' && XX[8]=='S')
      {
         conteo_command = 5;
      }
/// retorna valor de comando para validar ///////////////////////////
   return conteo_command;

   }
   
