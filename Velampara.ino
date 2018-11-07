/*
  Velampara:
  Interactúa con un sistema externo (por ejemplo, una bombilla) a través de un relé,
  soplando un microfono para apagar y calentando un termistor para encender.

  El código consiste en el manejo de dos elementos:
  - Microfono:  El cual realizamos un filtrado paso-banda para eliminar componente de DC
                y promediar una media de las muestras. Si la media supera cierto umbral,
                (Se corresponde con el tiempo soplando al microfono), entonces se interpratará
                como soplido.
  - Termistor:  Promediamos la media con cada muestra obtenida y calculamos su diferencial,
                es decir, cuanto ha variado con respecto al valor anterior. Esta diferencial es la derivada,
                una derivada negativa indica termistor calentandose y positiva enfriendose ya que
                nuestro termistor está a tierra.

  Para que este código funcione es necesario la librería "SingleEMAFilter", disponible aqui:
  https://github.com/luisllamasbinaburo/Arduino-SingleEmaFilter

  Por Mickyleitor
  https://github.com/Mickyleitor/Velampara
 */

#define PIN_MICRO A0
#define PIN_TERMO A1
#define PIN_RELE 2
#define THRESHOLD_MICRO 20
#define THRESHOLD_TERMO -10

#include "SingleEMAFilterLib.h"

// Filtro para filtrar solo altas frecuencias
// Y si el soplido se asemeja al ruido blanco, solo nos quedaremos con las altas frecuencias
SingleEMAFilter <float> filtro_micro_AF(0.1);
// Filtro para las bajas frecuencias
// Una vez filtrado nuestro soplido, queremos solo que se activa cuando pasado cierto tiempo,
// Supere cierto umbral y se interprete como soplido, esto se hace realizando un filtro paso bajo
SingleEMAFilter <float> filtro_micro_LF(0.005);
// Filtro paso bajo del termistor y descartar posibles fluctuaciones
SingleEMAFilter <float> filtro_termo_LF(0.01);
// Filtro paso alto para eliminar componente DC del termistor
SingleEMAFilter <float> filtro_termo_AF(0.001);

// Variable de estado para FSM
uint8_t estado = 0;
String SerialMsg;

bool estaMicroEncendido(){  return (abs(filtro_micro_LF.GetLowPass()) > THRESHOLD_MICRO); }
bool estaTermoEncendido(){  return (filtro_termo_AF.GetHighPass() < THRESHOLD_TERMO); }
bool estaReleEncendido(){  return (digitalRead(PIN_RELE) == HIGH); }

void setup() {
  // Configuramos Serial para DEBUG
  Serial.begin(115200);
  // Configuramos pines de entrada y salida
  pinMode(PIN_MICRO,INPUT);
  pinMode(PIN_TERMO,INPUT);
  pinMode(PIN_RELE,OUTPUT);
  // Iniciamos relé apagado
  digitalWrite(PIN_RELE,LOW);
  // Generamos un PWM de 3 Hz para muestreo de debug
  // Muestra en pantalla los datos 3 veces por segundo
  tone(3,3);
  // Inicializa los filtros a 0
  filtro_init();
  // Colocamos interrupción en el Pin del PWM para que llame a la función mostrar
  attachInterrupt(digitalPinToInterrupt(3),mostrar,FALLING);
  // Colocamos interrupción en el Pin del PWM para que compruebe el estado
  // Descomentar funcion si no se desea mostrar la grafica pero sí actuar con el relé
  // attachInterrupt(digitalPinToInterrupt(3),comprobar,FALLING);
}

void loop() {
  filtro_load();
}

void mostrar(){
  // Imprime los filtros e indicador sobre la interpretacion del Micro y Termistor.
  SerialMsg = "";
  // SerialMsg.concat(filtro_micro_AF.GetHighPass());
  // SerialMsg.concat(",");
  SerialMsg.concat(filtro_micro_LF.GetLowPass());
  SerialMsg.concat(",");
  SerialMsg.concat(estaMicroEncendido() ? 50 : 0);
  SerialMsg.concat(",");
  // SerialMsg.concat(filtro_termo_LF.GetLowPass());
  // SerialMsg.concat(",");
  SerialMsg.concat(filtro_termo_AF.GetHighPass());
  SerialMsg.concat(",");
  SerialMsg.concat(estaTermoEncendido() ? 10 : 0);
  Serial.println(SerialMsg);
  comprobar();
}



void comprobar(){
  if(estaMicroEncendido() && !estado){
    digitalWrite(PIN_RELE,LOW);
  }else if(estaTermoEncendido()){
    digitalWrite(PIN_RELE,HIGH);
    estado = 1;
  }else estado = 0;
}

void filtro_init(){
  filtro_micro_AF.AddValue(0);
  filtro_micro_LF.AddValue(0);
  filtro_termo_LF.AddValue(0);
  filtro_termo_AF.AddValue(0);
}

void filtro_load(){
    // Leemos valor del Divisor de tensión del micro al Filtro Paso Alto
  filtro_micro_AF.AddValue(abs(analogRead(PIN_MICRO)));
  // Añadimos el ultimo valor del filtro paso alto al paso bajo
  filtro_micro_LF.AddValue(abs(filtro_micro_AF.GetHighPass()));
  // Leemos valor del Divisor de tensión del termo al Filtro Paso Alto
  filtro_termo_LF.AddValue(analogRead(PIN_TERMO));
  // Añadimos el ultimo valor del filtro paso alto al paso bajo
  filtro_termo_AF.AddValue(filtro_termo_LF.GetLowPass());
}
