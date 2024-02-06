
/* 
// INICIALMENTE COMO VOY A COMUNICARME POR I2C TENGO QUE AVERIGUAR EL DIRECCIÓN DEL SENSOR
// en mi caso me da 0x76 y tengo que ir a la libreria en >.pio>libdeps>Adafruit BMP280 Library>Adafruit_BMP280
// y sustituir 0x77 por 0x76

#include <Arduino.h>
#include <Wire.h>

void setup()
{
  Wire.begin();

  Serial.begin(9600);
  while (!Serial);             // Espera al monitor serie
  Serial.println("\nEscaner I2C");
}

void loop()
{
  byte error, address;
  int nDevices;

  Serial.println("Escaneando...");

  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    // El escaner i2c utiliza el valor devuelto por la instrucción 
    // Write.endTransmisstion para reconocer la dirección a la que 
    // está conectado cada dispositivo.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("Dispositivo I2C encontrado en la direccion 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Error desconocido en la direccion 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("Ningun dispositivo I2C encontrado\n");
  else
    Serial.println("Hecho\n");

  delay(5000);           // espera 5 segundos para el siguiente escaneo
}
   */ 

#include <Arduino.h>
#include <Wifi.h> 
#include <HTTPClient.h>      // no es necesario introducirla solo llamarla
#include <WiFiClient.h>
#include <Adafruit_BMP280.h>

#include <Wire.h>            // necesario para sht20
#include <DFRobot_SHT20.h>   // necesario para sht20

#include <AsyncElegantOTA.h>
#include <Hash.h>
#include <elegantWebpage.h>

//objetos
DFRobot_SHT20    sht20;
Adafruit_BMP280 bmp; // I2C
WiFiClient client; 
//COLEGIO
const char* ssid = "CENTRO";
const char* password = "";

// variables entradas/salidas
int UVsensorIn = 32; //Output from the sensor uva
// variables para utilizar fuera de su función
float uvIntensity;
// variables pluviometro
const byte sensor=5;
const int tiempoRebote=500;
volatile int numVuelcos = 0;// numero de vuelcos, comienza en 0
float litrosIntervalo=0;
unsigned long flagVuelcos=500;
unsigned long flagEnvio =0;
int intervaloEnvio=20000;

//FUNCIONES aquí se declaran
void funContadorVuelcos();
void pluviometro();
void lecturaML8511();
void lecturaBMP280();
void lecturaSHT20();
void envioDatos();

int averageAnalogRead(int pinToRead) ;
int averageAnalogRead(int UVsensorIn); 
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max);
//void lecturaML8511();

void setup()
{
  Serial.begin(9600);

  //=======pluviometro====
  pinMode(sensor, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(sensor), funContadorVuelcos, FALLING);
   
  //=======ml8511=========
  pinMode(UVsensorIn, INPUT);

  //=======bmp280=========
  Serial.println(F("BMP280 Forced Mode Test."));

  if (!bmp.begin())
  {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    while (1) delay(10);
  }
 
  bmp.setSampling(Adafruit_BMP280::MODE_FORCED,     
                  Adafruit_BMP280::SAMPLING_X2,     
                  Adafruit_BMP280::SAMPLING_X16,    
                  Adafruit_BMP280::FILTER_X16,      
                  Adafruit_BMP280::STANDBY_MS_500); 

  //=======sht20=========
  Serial.println("SHT20 Example!");                   
  sht20.initSHT20();                                  // Init SHT20 Sensor
  delay(10000);
  sht20.checkSHT20();                                 // Check SHT20 Sensor
  Serial.println("Connecting to WiFi");

  //=======wifi=========
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
  delay(100);
  Serial.print(".");
  }
}

void loop() 
{
  if (millis()-flagEnvio>intervaloEnvio)  // 1h=3600000 sg si el tiempo supera la hora tomo las medidas de todos los vuelcos
  { 
    flagEnvio=millis(); // colocarlo mejor aquí. Si lo colocas al final te va dando retrasos
    lecturaML8511(); // rayos uva
    lecturaBMP280(); // tra,altitud,presión
    lecturaSHT20();  // tra,humedad (elegido por tener mayor precisión)
    pluviometro();
    envioDatos();
  }
}

// aquí van las funciones
void lecturaML8511()
{
  int uvLevel = averageAnalogRead(UVsensorIn);
 
  float outputVoltage = 3.3 * uvLevel/4095;
  float uvIntensity = mapfloat(outputVoltage, 0.99, 2.9, 0.0, 15.0);
 
  Serial.print(" UV Intensity: ");
  Serial.print(uvIntensity);
  Serial.print(" mW/cm^2"); 
  Serial.println(); 
 // delay(200);
}

void lecturaBMP280()
{
  if (bmp.takeForcedMeasurement()) {      // must call this to wake sensor up and get new measurement data it blocks until measurement is complete
    // can now print out the new measurements
    Serial.print(F("Temperature = "));
    Serial.print(bmp.readTemperature());
    Serial.println(" *C");

    Serial.print(F("Pressure = "));
    Serial.print(bmp.readPressure());
    Serial.println(" Pa");

    Serial.print(F("Approx altitude = "));
    Serial.print(bmp.readAltitude(1013.25)); 
    Serial.println(" m");

    Serial.println();
   // delay(2000);
  } else {
    Serial.println("Forced measurement failed!");
  }
}
void lecturaSHT20()
{
    float humd = sht20.readHumidity();                  // Read Humidity
    float temp = sht20.readTemperature();               // Read Temperature
    Serial.print(" Temperature:");
    Serial.print(temp, 1);
    Serial.print("C");
    Serial.print(" Humidity:");
    Serial.print(humd, 1);
    Serial.print("%");
    Serial.println();
   // delay(1000);
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
int averageAnalogRead(int pinToRead)
{
  byte numberOfReadings = 8;
  unsigned int runningValue = 0; 
 
  for(int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
    runningValue /= numberOfReadings;
 
  return(runningValue);  
 
} 

void funContadorVuelcos(){
  if(millis()>(flagVuelcos+tiempoRebote)){ //cuando supera el tiempo de rebote
    numVuelcos++;                             // cuenta y guarda el resultado
    flagVuelcos=millis();                  // prepara el contador para otra medida
    //Serial.println(numVuelcos);// Mejor no ponerlo. Cuando se llama a un controlador de interrupciones, bloquea todo el sistema. 
    //Por lo tanto, solo debe hacer un trabajo mínimo y regresar rápidamente. 
    //Sin embargo, la comunicación en serie es lenta y provoca grandes retrasos
}
}

void pluviometro(){
     
    litrosIntervalo=numVuelcos * 0.2794;     // lectura de litros/hora
    //++++++++++++++++ lectura por horas +++++++++++++++++++++++++++
    if (litrosIntervalo>0)                // si ha pasado la hora AND hay algún vuelco pongo todo a cero
    {                                 // tanto el numVuelcos como
    Serial.println(flagEnvio);  
    Serial.print("litrosIntervalo por m2 caidos en una una Hora (Lm2)" );
    Serial.println(litrosIntervalo);
    numVuelcos= 0;
    //flagEnvio = millis();
    }
    //++++++++++++++++ lectura por días +++++++++++++++++++++++++++

   int litrosDia=litrosIntervalo*24; 

}

void envioDatos(){

if (WiFi.status() == WL_CONNECTED){ 
     HTTPClient http;  // creo el objeto http
     http.begin(client,"http://estacionjac.000webhostapp.com/EspPost.php");
     http.addHeader("Content-Type", "application/x-www-form-urlencoded"); // defino texto plano..
     
     
     //String datos_a_enviar = "temperatura=" +String(10);  

   // String datos_a_enviar = "temperatura=" + String(30) + "&humedad=" + String(30)+ "&presion=" + String(30);  
    String datos_a_enviar = "temperatura=" + String(sht20.readTemperature()) + "&humedad=" + String(sht20.readHumidity())+"&presion=" + String(bmp.readPressure())+"&uv=" + String(uvIntensity)+"&lluvia=" + String(litrosIntervalo);  
     /*  String datos_a_enviar = "temperatura=" + String(sht20.readTemperature());      
           datos_a_enviar=+"&humedad=" + String(sht20.readHumidity());
           datos_a_enviar=+"&presion=" + String(bmp.readPressure());
           datos_a_enviar= +"&uv=" + String(uvIntensity);  */ // ERRORES

     int codigo_respuesta = http.POST(datos_a_enviar);

     if (codigo_respuesta>0){
      Serial.println("Código HTTP: "+ String(codigo_respuesta));
        if (codigo_respuesta == 200){
          String cuerpo_respuesta = http.getString();
          Serial.println("El servidor respondió: ");
          Serial.println(cuerpo_respuesta);
        }
     } else {
        Serial.print("Error enviado POST, código: ");
        Serial.println(codigo_respuesta);
     }

       http.end();  // libero recursos
       
  } else {
     Serial.println("Error en la conexion WIFI");
  }
 // delay(60000); //espera 60s
}
 




