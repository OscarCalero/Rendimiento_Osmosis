/*
 *********** ARDUINO UNO/NANO

  0
  1
  2 Digital PULL_UP - CAUDALIMETRO IN
  3 Digital PULL_UP - CAUDALIMETRO OUT
  4 Digital PULL_UP - PULSADOR
  5
  6
  7
  8
  9
  10
  11
  12
  13
  14
  15
  16
  17
  18 (A4) - SDA I2C LCD
  19 (A5) - SCL I2C LCD
*/

/*  ***************** CALIBRACION CAUDALIMETRO
  Experimento: 0.5 litros en 15 segundos = 2.00 l/min && 1140 pulsos --> 76 pulsos/seg && k = [pulsos/(60*litros)]=38
*/

// ******* LCD ********
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); // dirección i2C de LCD ( normalmante 0x27 ó 0x3F )
byte retroiluminacion = 30;
#define tiempoRetroiluminacion 60  // segundos que permanece encendida despues de evento

// ******* CAUDAL ********
#define numeroMuestrasCaudalInstantaneo 3  // Número de valores para promedio caudal instantaneo
#define horasCalculoRendimiento 2 // Un cálculo de rendimiento requiere de bastantes horas de promedio
#define alarmaRendimiento 8 // 8 = 8% por debajo del cual se quedara la retroiluminación encendida
float constanteCaudalimetro = 38;  // Constante de caudalimetro (pulsos por segundo para caudal de 1 Litro/min)
const uint8_t caudalPinIN = 2;  // PIN caudalimetro IN
const uint8_t caudalPinOUT = 3;  // PIN caudalimetro OUT
unsigned int numeroPulsosCaudalIN;
unsigned int numeroPulsosCaudalOUT;
float litrosAcumuladosHoraIN = 0;  // Litros acumulados
float litrosAcumuladosHoraOUT = 0;
float mediaCaudalIN[numeroMuestrasCaudalInstantaneo]; //Almacenar valores de caudal instantaneo
float mediaCaudalOUT[numeroMuestrasCaudalInstantaneo];
byte mediaCaudal_TAG;
float litrosAcumuladosRegistroIN[horasCalculoRendimiento + 1]; // Litros acumulados en cada hora
float litrosAcumuladosRegistroOUT[horasCalculoRendimiento + 1];
unsigned int horas_TAG = 0;
byte horasDesdeArranque = 0;

// ********* GENERAL  ****
const uint8_t touchPin = 4;  // PIN para pulsador que enciende pantalla y/o reinicia contadores
boolean habilitarSerie = false;
unsigned long segundo_TAG = 0 ;
boolean Segundo = false;
boolean Minuto = false;
boolean Hora = false;
unsigned int textoHora = 0;
unsigned int textoMinuto = 0;
unsigned int textoSegundo = 0;
unsigned int tempoPulsos = 0;
byte statusDisplay = 0;
unsigned long touch_TAG ;

void pulsoCaudalIN() {
  numeroPulsosCaudalIN++;
}

void pulsoCaudalOUT() {
  numeroPulsosCaudalOUT++;
}

void calculoCaudalInstantaneo () {   // valor instantaneo (promedio de valores y valor por minuto acumulado)
  mediaCaudalIN[mediaCaudal_TAG] = numeroPulsosCaudalIN ;
  numeroPulsosCaudalIN = 0;
  mediaCaudalIN[mediaCaudal_TAG] = mediaCaudalIN[mediaCaudal_TAG] / constanteCaudalimetro ;
  litrosAcumuladosHoraIN += mediaCaudalIN[mediaCaudal_TAG] / 60 ;
  mediaCaudalOUT[mediaCaudal_TAG] = numeroPulsosCaudalOUT ;
  numeroPulsosCaudalOUT = 0;
  mediaCaudalOUT[mediaCaudal_TAG] = mediaCaudalOUT[mediaCaudal_TAG] / constanteCaudalimetro ;
  litrosAcumuladosHoraOUT += mediaCaudalOUT[mediaCaudal_TAG] / 60 ;
  mediaCaudal_TAG++;
  if (mediaCaudal_TAG == numeroMuestrasCaudalInstantaneo) {
    mediaCaudal_TAG = 0;
  }
}

float valorLitrosMinutoIN() {
  float litrosMinuto_ = 0;
  for (int i = 0; i < numeroMuestrasCaudalInstantaneo; i++) { // Promedio de  valores para reportar
    litrosMinuto_ +=  mediaCaudalIN[i];
  }
  litrosMinuto_ = litrosMinuto_ / numeroMuestrasCaudalInstantaneo;
  return litrosMinuto_;
}

float valorLitrosMinutoOUT() {
  float litrosMinuto_ = 0;
  for (int i = 0; i < numeroMuestrasCaudalInstantaneo; i++) { // Promedio de  valores para reportar
    litrosMinuto_ +=  mediaCaudalOUT[i];
  }
  litrosMinuto_ = litrosMinuto_ / numeroMuestrasCaudalInstantaneo;
  return litrosMinuto_;
}

float rendimiento() {
  litrosAcumuladosRegistroIN[horasCalculoRendimiento] = litrosAcumuladosHoraIN;
  litrosAcumuladosRegistroOUT[horasCalculoRendimiento] = litrosAcumuladosHoraOUT;
  for ( unsigned int i = 0 ; i < horasCalculoRendimiento ; i++ ) {
    litrosAcumuladosRegistroIN[horasCalculoRendimiento] += litrosAcumuladosRegistroIN[i];
    litrosAcumuladosRegistroOUT[horasCalculoRendimiento] += litrosAcumuladosRegistroOUT[i];
  }
  float rendimiento_ ;
  if ( litrosAcumuladosRegistroIN[horasCalculoRendimiento] == 0 ) {  // evitar division por cero
    litrosAcumuladosRegistroIN[horasCalculoRendimiento] = 0.000001;
  }
  rendimiento_ = litrosAcumuladosRegistroOUT[horasCalculoRendimiento] / litrosAcumuladosRegistroIN[horasCalculoRendimiento] ;
  return rendimiento_ ;
}

void eventoSegundo () {
  calculoCaudalInstantaneo();
  String mensaje_ ;
  float caudalIN_ = valorLitrosMinutoIN();
  float caudalOUT_ = valorLitrosMinutoOUT();
  float rendimiento_ = rendimiento() * 100;

  if (habilitarSerie) {
    mensaje_ = "IN: " + String(caudalIN_) + " L/min, OUT: " + String(caudalOUT_) + " L/min" ;
    mensaje_ = mensaje_ + " / acum.IN: " + String(litrosAcumuladosRegistroIN[horasCalculoRendimiento]) + " L, acum.OUT: " + String(litrosAcumuladosRegistroOUT[horasCalculoRendimiento]) + " L" ;
    mensaje_ = mensaje_ + " / rendimiento (" + String(horasDesdeArranque) + " horas): " + String(rendimiento_) + " %" ;
    Serial.println( mensaje_ ); //string con caudal agua
  }

  if (rendimiento_ < alarmaRendimiento && rendimiento_ > 0) {
    retroiluminacion = tiempoRetroiluminacion ;
  }
  if (caudalIN_ > 0 || caudalOUT_ > 0 ) {
    retroiluminacion = tiempoRetroiluminacion ;
  }
  if ( retroiluminacion > 0 ) {
    lcd.backlight() ;
    retroiluminacion-- ;
  } else {
    lcd.noBacklight();
  }

  if (caudalIN_ > 0 && statusDisplay < 7) {
    mensaje_ = "I:" + String(caudalIN_) + "    ";
    mensaje_ = mensaje_.substring(0, 6) + "Lm";
  } else {
    mensaje_ = "I:" + String(litrosAcumuladosRegistroIN[horasCalculoRendimiento]) + "    " ;
    mensaje_ = mensaje_.substring(0, 7) + "L";
  }
  if (caudalOUT_ > 0 && statusDisplay < 7) {
    mensaje_ +=  ">O:" + String(caudalOUT_) + "    ";
    mensaje_ = mensaje_.substring(0, 14) + "Lm";
  } else {
    mensaje_ += ">O:" + String(litrosAcumuladosRegistroOUT[horasCalculoRendimiento]) + "    "  ;
    mensaje_ = mensaje_.substring(0, 15) + "L";
  }
  lcd.setCursor(0, 0);
  lcd.print( mensaje_ );

  mensaje_ = "Rend." + String(horasDesdeArranque) + "h: " + String(rendimiento_) + "%" ;
  for ( int i = mensaje_.length() ; i < 16; i++) {
    mensaje_ += " ";
  }
  lcd.setCursor(0, 1);
  lcd.print( mensaje_ );
  
  statusDisplay++;
  if (statusDisplay == 10) {
    statusDisplay = 0 ;
  }
  if ( digitalRead(touchPin) ) {
    touch_TAG = millis();
  }
  if (touch_TAG + 2500 < millis() ) { // borrado de valores
    for ( unsigned int i = 0 ; i < horasCalculoRendimiento + 1 ; i++ ) {
      litrosAcumuladosRegistroIN[i] = 0 ;
      litrosAcumuladosRegistroOUT[i] = 0 ;
      litrosAcumuladosHoraIN = 0;
      litrosAcumuladosHoraOUT = 0;
    }
    touch_TAG = millis();
  }
}


void eventoHora() {
  if (horasDesdeArranque < horasCalculoRendimiento) {
    horasDesdeArranque++;
  }
  litrosAcumuladosRegistroIN[horas_TAG] = litrosAcumuladosHoraIN;
  litrosAcumuladosHoraIN = 0;
  litrosAcumuladosRegistroOUT[horas_TAG] = litrosAcumuladosHoraOUT;
  litrosAcumuladosHoraOUT = 0;
  horas_TAG++;
  if (horas_TAG == horasCalculoRendimiento) {
    horas_TAG = 0;
  }
}

void checkTime() {
  if (millis() > segundo_TAG ) {
    segundo_TAG += 1000;
    Segundo = true;
    // Variables de temps
    textoSegundo++;
    if (textoSegundo > 59) {
      textoSegundo = 0;
      textoMinuto++;
      Minuto = true;
    }
    if (textoMinuto > 59) {
      textoMinuto = 0;
      textoHora++;
      Hora = true;
    }
  }
}

// ************ S E T U P ************

void setup() {

  // ******* Serial ********
  if (habilitarSerie) {
    Serial.begin(115200);
    delay(300);
    Serial.print("Init...\n");
  }

  //*****  I2C  ***************
  Wire.begin();

  // ******* Cristall líquid ********
  lcd.init();                      // initialize the lcd
  lcd.noBacklight();

  // ******* Càlcul caudal ********
  pinMode(caudalPinIN, INPUT_PULLUP );
  pinMode(caudalPinOUT, INPUT_PULLUP );
  pinMode(touchPin, INPUT_PULLUP );
  delay(1);
  attachInterrupt(digitalPinToInterrupt(caudalPinIN), pulsoCaudalIN, RISING);
  attachInterrupt(digitalPinToInterrupt(caudalPinOUT), pulsoCaudalOUT, RISING);
  numeroPulsosCaudalIN = 0;
  numeroPulsosCaudalOUT = 0;
  touch_TAG = millis();
}

void loop(void) {
  if ( !digitalRead(touchPin) ) {  // Enciende retroiluminacion
    retroiluminacion = tiempoRetroiluminacion ;
  }
  checkTime();   // gestió de segons
  if (Segundo) {
    eventoSegundo();
    Segundo = false;
  }
  if (Minuto) {
    Minuto = false;
  }
  if (Hora) {
    eventoHora();
    Hora = false;
  }
}
