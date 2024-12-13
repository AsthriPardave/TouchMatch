#include <LiquidCrystal_I2C.h> // librerias para el control del LCD + I2C
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

//String n;  //numero de 4 digitos
#define SER 27
#define RCLK 33
#define SRCLK 32

#define VERDE1 1
#define AMAR1 2
#define ROJO1 4
#define AMAR2 8
#define ROJO2 16
#define VERDE2 32
#define DISPLAY1 64
#define DISPLAY2 128

String n;
String eventData;

struct RGB {  // struct para definir los pines del led RGB
  int red;
  int green;
  int blue;
};

const char* ssid = "LT";
const char* password = "petrikldeidad";
//const char* ssid = "LTR";
//const char* password = "SONSOFAMILIA1234";

const int touchCorrect = 13; // pin de entrada del sensor ttp223
const int touchIncorrect = 12;

const RGB rgb = {25, 26, 34}; // pines del RGB 

LiquidCrystal_I2C lcd(0x27, 16, 2); // display lcd
 
// funcion para imprimir la cantidad de correctas e incorrectas en el lcd
void printInfo(int corrects, int incorrects) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Correctas:");
  lcd.setCursor(11, 0);
  lcd.print(corrects);
  lcd.setCursor(0, 1);
  lcd.print("Incorrectas:");
  lcd.setCursor(12, 1);
  lcd.print(incorrects);
}
 
// funcion para imprimir los resultados finales del juego
void printFinal(int corrects, int incorrects, long avg) {
  printInfo(corrects, incorrects);
  delay(3000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Promedio:");
  lcd.setCursor(0, 1);
  lcd.print(String(avg) + "ms");
  delay(3000);
}

// funcion para encender el color RGB del led en funcion a un parametro

void paintMaster(char master) {
  switch (master) {
    case '0':
      analogWrite(rgb.red, 255);
      analogWrite(rgb.green, 0);
      analogWrite(rgb.blue, 0);
      break;
    case '1':
      analogWrite(rgb.red, 0);
      analogWrite(rgb.green, 255);
      analogWrite(rgb.blue, 0);
      break;
    case '2':
      analogWrite(rgb.red, 255);
      analogWrite(rgb.green, 255);
      analogWrite(rgb.blue, 0);
      break;
    default:
      analogWrite(rgb.red, 0);
      analogWrite(rgb.green, 0);
      analogWrite(rgb.blue, 0);
      break;
  }
}

int suma (){

  int colorLed;
  int numLeds;
  int numDisplay;

  if(n[2] == '1'){  // prender un solo led
    if (n[1] == '0'){  // 0 es rojo
      colorLed = ROJO1;
    } else if (n[1] == '1'){ // 1 es verde
      colorLed = VERDE1;
    } else if (n[1] == '2'){ // 2 es amarillo
      colorLed = AMAR1;
    } else {
      colorLed = 0;
    }
  } else if (n[2] == '2'){  // prender 2 leds
    if (n[1] == '0'){
      colorLed = ROJO1 + ROJO2;
    } else if (n[1] == '1'){
      colorLed = VERDE1 + VERDE2;
    } else if (n[1] == '2'){
      colorLed = AMAR1 + AMAR2;
    } else {
      colorLed = 0;
    }
  }
  if(n[3] == '1'){
    numDisplay = DISPLAY1;
  } else if (n[3] == '2'){
    numDisplay = DISPLAY2;
  }
  return colorLed + numDisplay;
}

// funcion de inicio 
void setup() {

  pinMode(SER, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(SRCLK, OUTPUT);
  
  pinMode(touchCorrect, INPUT); // touch en INPUT
  pinMode(touchIncorrect, INPUT); // touch en INPUT
  Wire.begin(22, 20); // Configura SDA (GPIO22) y SCL (GPIO20)

  // iniciar lcd
  lcd.init();
  lcd.backlight();
  lcd.clear();

  Serial.begin(9600);
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado.");

}

 // Conexión a la red WiFi

// funcion de bucle principal
void loop() {


  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Conectarse al servidor SSE
    //http.begin("https://touchmatch-production.up.railway.app/api/esp32/game/info/TM-7D91");
    http.begin("http://192.168.169.193:8000/api/esp32/game/info/TM-7D91");
    http.addHeader("Accept", "text/event-stream"); // Indicar que es SSE

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
      Serial.println("Conexión establecida con el servidor SSE.");

      // Mantén la conexión abierta para recibir eventos
      WiFiClient* client = http.getStreamPtr();

      while (client->connected()) {
        String line = client->readStringUntil('\n');
        digitalWrite(RCLK, LOW);
        shiftOut(SER, SRCLK, MSBFIRST, 0);
        paintMaster('x');
        digitalWrite(RCLK, HIGH);
        delay(100);

        if (line.startsWith("data: ")) {
          eventData = line.substring(6); // Extraer el contenido después de "data: "
          Serial.println("Evento recibido:");
          Serial.println(eventData);
        } else {
          // acaba el juego
        }
        // Parsear JSON
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, eventData);

        if (error) {
          Serial.print("Error al analizar JSON: ");
          Serial.println(error.c_str());
          return;
        }
        // Extraer valores
        int level = doc["level"];          // Extrae "level"
        const char* sequence = doc["sequence"]; // Extrae "sequence"
        int user = doc["user_registration"]; // Extrae "user_registration"

        n = String(sequence);

        digitalWrite(RCLK, LOW);
        shiftOut(SER, SRCLK, MSBFIRST, suma());
        paintMaster(n[0]);
        digitalWrite(RCLK, HIGH);
        
        bool touchedCorrect = false; // asumimos que no ha tocado nada
        bool touchedIncorrect = false; // asumimos que no ha tocado nada
        long reactTime = 0; // tiempo de reaccion
        
        long begin = millis(); // aqui empieza a contar el tiempo de reacción
        
        while (millis() - begin < (level==1 ? 2000 : 1000)) { // el usuario tiene 1 segundo para reaccionar
          if (digitalRead(touchCorrect) == HIGH && !touchedCorrect) { // si aun no se ha tocado y lo estas tocando ahora (TTP223)
            touchedCorrect = true; // ya se tocó
            reactTime = millis() - begin; // sumamos el tiempo de reaccion
            break;
          }
          if (digitalRead(touchIncorrect) == HIGH && !touchedIncorrect) { // si aun no se ha tocado y lo estas tocando ahora (TTP223)
            touchedIncorrect = true; // ya se tocó
            reactTime = millis() - begin; // sumamos el tiempo de reaccion
            break;
          }
        }
        int punto;
        if (touchedCorrect) { // si llegaste a tocar el correcto
          if (n[0] == n[1]) { // si coincidian los colores aumentamos una correcta
            punto = 1;
          } else { // si no coincidian damos una incorrecta
            punto = -1;
          }
          
        } else if (touchedIncorrect){ // si llegaste a tocar el incorrecto
          if (n[0] == n[1]) { // si coincidian los colores aumentamos una incorrecta
            punto = -1;
          } else { // si no coincidian damos una correcta
            punto = 1;
          }
        
        } else { //si no tocaste el correcto ni el incorrecto
            punto = -1;
        }

        StaticJsonDocument<200> jsonDoc;
        jsonDoc["point"] = punto;
        jsonDoc["react"] = reactTime;

        String requestBody;
        serializeJson(jsonDoc, requestBody);

        // Realizar solicitud HTTP PUT
        HTTPClient http2;
        http2.begin("http://192.168.169.193:8000/api/esp32/game/points/"+ String(user)); // URL del servidor
        http2.addHeader("Content-Type", "application/json"); // Cabecera para JSON

        int http2ResponseCode = http2.PUT(requestBody); // Enviar datos

        if (http2ResponseCode > 0) {
          String responseBody = http2.getString();
          StaticJsonDocument<200> doc2;
          DeserializationError error = deserializeJson(doc2, responseBody);
          if (error) {
            Serial.print("Error al analizar JSON: ");
            Serial.println(error.c_str());
            return;
          }
          // Extraer valores
          int puntosCorrectos = doc2["good_points"];      // Extrae "good_points"
          int puntosIncorrectos = doc2["bad_points"];    // Extrae "bad_points"
          printInfo(puntosCorrectos, puntosIncorrectos);
          
        } else {
          Serial.print("Error en la solicitud HTTP2: ");
          Serial.println(http2ResponseCode);
        }

        http2.end(); // Finalizar conexión HTTP
      }
    
    } else {
      Serial.printf("Error al conectar con el servidor: %d\n", httpCode);
    }

    http.end();
    Serial.println("Se cerro la conexion");
  }

  Serial.print("Se desconecto de la red");

  /* 
    //mostramos en el LCD 
   else { // aquí ya acabaron los 2min por lo que calculamos el tiempo promedio de reaccion y mostramos la información final
    avg = reactCount > 0 ? reactSum / reactCount : 0;
    while (1) {
      printFinal(corrects, incorrects, avg);
      Serial.print("Correctas: ");
      Serial.println(corrects);
      Serial.print("Incorrectas: ");
      Serial.println(incorrects);
      delay(3000);
    }
  } */
  
}
