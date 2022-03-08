#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>


int Rele1A = 2;
int Rele2A = 4;
int detectorDeHumedadA = A0;

int contadorAuxiliar = 0;

long int tiempoDetectorA = 0;
long int tiempoComprobado = 0;
long int ultimoRiegoA = 0;
int resultadoA = 0;

bool regarA = false;
bool archivoAbierto = false;
bool comprobado = false;
bool primerGuardadoDatos = true;

const int dry = 584;
const int wet = 240;

File myFile;

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    myFile.print('0');
  }
  myFile.print(number);
}


void activarRiego(int pinAlimentarRele, int pinActivarRele, int tiempoRiego){
  digitalWrite(pinActivarRele, LOW);
  digitalWrite(pinAlimentarRele, HIGH);
  
  delay (tiempoRiego);

  digitalWrite(pinActivarRele, HIGH);
  digitalWrite(pinAlimentarRele, LOW);
}

int humedad(int sensor){
  int vectorLocal[16];
  int resultadoLocal = 0;
  int contador = 0;
  long int sumaLocal = 0;
  long int humedadLocal = 0;
  long int sumaReal = 0;
  long int humedadReal = 0;
    for (int i = 0; i < 16; i++){
      vectorLocal[i] = analogRead(sensor);
      delay(100);
    }

    for(int i = 0; i < 16; i++){
      sumaLocal += vectorLocal[i];
    }
    humedadLocal = sumaLocal / 16;

    for(int i = 0; i < 16; i++){
      if (vectorLocal[i] - humedadLocal > 0){
        if(vectorLocal[i] - humedadLocal < 100){
          sumaReal += vectorLocal[i];
          contador++;
        }
      }
      else{
        if(vectorLocal[i] - humedadLocal > -100){
          sumaReal += vectorLocal[i];
          contador++;
        }
      }
    }
    humedadReal = sumaReal / contador;
    float porcentaje = humedadReal - 240; 
    porcentaje = porcentaje / 344;
    porcentaje = 100 * porcentaje;
    porcentaje = 100 - porcentaje;
    resultadoLocal = porcentaje;
    return resultadoLocal;
    
}

void setup() {
  tmElements_t tm;
  pinMode(2, OUTPUT);
  pinMode(4, OUTPUT);
  Serial.begin(9800);
  if(!SD.begin(10)){
    comprobado = true;
    Serial.println("No se ha encontrado una tarjeta SD");
  }
  else{
    Serial.println("Tarjeta SD detectada");
  }
}
// the loop function runs over and over again forever
void loop() {
  tmElements_t tm;
  if(!comprobado && !SD.begin(10)){
    Serial.println("Tarjeta SD desconectada.");
    comprobado = true;
  }
  // Si ha pasado un segundo actualizamos la humedad del sensor, cada vez que se actualice ejecutamos todo el programa de regado.
  if((millis() / 1000 - tiempoDetectorA) >= 300){
    
    resultadoA = humedad(detectorDeHumedadA);
    Serial.print(resultadoA);
    Serial.println("%");
    

    
    // Si la humedad está por debajo del X(20%) 40% y no le hemos ordenado que riege, lo hacemos.
    if(resultadoA <= 40 && !regarA){
      regarA = true;
    }
    // Si la humedad está por debajo del 60% y se le ha ordenador regar, lo regamos y ajustamos el ultimo riego a ahora mismo, así volverá
    // a regar cuando haya pasado el tiempo necesario (unos 30 minutos).
    if(resultadoA < 60 && regarA){
      if(contadorAuxiliar == 0) {

        // !Como ya no  voy a utilizar el relé tengo que redefinir la funcion activar riego
      activarRiego(Rele2A,Rele1A,4400);
      ultimoRiegoA = millis() / 1000;
      Serial.println("Encendiendo la placa");
      contadorAuxiliar++;
      }
      else if(millis() / 1000 - ultimoRiegoA > 1600){
      Serial.println("Encendiendo la placa");
      activarRiego(Rele2A,Rele1A,4400);
      ultimoRiegoA = millis() / 1000;
      }
    }
    // Finalmente si la humedad está por encima del 60% hacemos que deje de regar.
    else if(resultadoA > 60){
      regarA = false;
    }
    // Esto volverá a repetirse cada vez que la humedad baje del 20%.    
    tiempoDetectorA = millis() / 1000;
    Serial.println(tiempoDetectorA);
  if(!comprobado){
    myFile = SD.open("Riego.txt", FILE_WRITE);
      if(primerGuardadoDatos){
        delay(300);
        RTC.read(tm);
        myFile.println();
        myFile.print("Fecha de inicio (D/M/Y): ");
        myFile.print(tm.Day);
        myFile.print('/');
        myFile.print(tm.Month);
        myFile.print('/');
        myFile.print(tmYearToCalendar(tm.Year));
        myFile.println();
        primerGuardadoDatos = false;
    }
    if (RTC.read(tm)) {
      print2digits(tm.Hour);
      myFile.print(':');
      print2digits(tm.Minute);
      myFile.print(':');
      print2digits(tm.Second);
      myFile.print(", (D/M/Y) = ");
      myFile.print(tm.Day);
      myFile.print('/');
      myFile.print(tm.Month);
      myFile.print('/');
      myFile.print(tmYearToCalendar(tm.Year));
      myFile.println();
      Serial.println("Datos Cargados");
    } else {
      if (RTC.chipPresent()) {
        Serial.println("The DS1307 is stopped.  Please run the SetTime");
        Serial.println("example to initialize the time and begin running.");
        Serial.println();
      } else {
        Serial.println("DS1307 read error!  Please check the circuitry.");
        Serial.println();
            }
          }
      myFile.print(resultadoA);
      myFile.println("%");
      myFile.println();  
      myFile.close();    
        }
}
}
