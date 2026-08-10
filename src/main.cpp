#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#include "FS.h"
#include <LittleFS.h>  
#include <WiFi.h>
#include <HardwareSerial.h>
#include "esp_sleep.h"
#include <time.h>
#include <TinyGPSPlus.h>


// config gps
TinyGPSPlus gps;

HardwareSerial SerialGPS(2);

#define GPS_RX 16
#define GPS_TX 17
#define GPS_BAUD 9600


// const char* ssid = "REDE20";  // Colocar o nome da rede Wi-Fi
// const char* password = "20#UERGSNET99";     // Colocar a senha da rede Wi-Fi


const char* ssid = "Camuflado STG";  // Colocar o nome da rede Wi-Fi
const char* password = "@veialoka#rumo60@";     // Colocar a senha da rede Wi-Fi

// const int pinoDO = 4;     // trocado


#define FIREBASE_DATABASE_URL "https://bancodedados-a7591-default-rtdb.firebaseio.com"
#define FIREBASE_API_KEY "AIzaSyCG6pcJI9JV8G6gW8F8HAhfEGJvw8vhXDY"
#define FIREBASE_AUTH "09fFbaRrhJkNPoDVwRE3TszPG2m7TeUZKWuoAJUF"

#define FIREBASE_ESTACAO_ID "ESTACAO_001"
#define FIREBASE_PATH "/estacao/" FIREBASE_ESTACAO_ID

FirebaseJson json;
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;
bool firebase_flag = true;

#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

// a pensar 
#define TAM_BUFFER 5
typedef struct {

    // INFO DA ESTAÇÃO

    char nomeEstacao[50];
    char firmware[20];
    bool estacaoAtiva;

    // DATA / HORA

    uint32_t timestamp;

    uint16_t ano;
    uint8_t mes;
    uint8_t dia;
    uint8_t hora;
    uint8_t minuto;
    uint8_t segundo;

    // SENSOR - LUMINOSIDADE

    float luminosidade;
    bool luminosidadeAtiva;

    // SENSOR - UV

    float valor;


    // SENSOR - PRESSÃO ATMOSFÉRICA

    float temperatura;
    float pressurePa;
    float pressureBar;
    float altitude;


    // SENSOR - CHUVA

    float volumeChuva;
    bool chuvaAtiva;

    // SENSOR - VENTO

    float velocidadeVento;
    bool velocidadeVentoAtiva;

    float direcaoVento;
    bool direcaoVentoAtiva;

    // SENSOR - GPS
  
    double latitude;
    double longitude;
    bool gpsAtivo;

} Data;
Data dados_finais[TAM_BUFFER];


// const int pinoDO = 34;     // saída digital do sensor
const int pinoLED = 13;   // LED
const int pinoAO = 34;    // pino analógico (ADC)
Adafruit_BMP280 bmp; // I2C
//Adafruit_BMP280 bmp(BMP_CS); // hardware SPI
//Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO,  BMP_SCK);


void SensorLuminosidade() {     // precisa ser revisada
  // int estado = digitalRead(pinoDO);
    int valorLuz = analogRead(pinoAO);

  // Serial.println(estado);
     Serial.println(valorLuz);
  delay(200);
}


void SensorUv() {
  float sensorVoltage; 
  float sensorValue;
 
  sensorValue = analogRead(36);
  sensorVoltage = sensorValue/4095*3.3;
  Serial.print("sensor reading = ");
  Serial.print(sensorValue);
  Serial.println("");
  Serial.print("sensor voltage = ");
  Serial.print(sensorVoltage);
  Serial.println(" V");
  dados_finais->valor = sensorVoltage;
  delay(1000);
}
void ConfigSensorPressao(){

  while ( !Serial ) delay(100);   // wait for native usb
  Serial.println(F("BMP280 test"));
  unsigned status;
  //status = bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);
  status = bmp.begin(0x76);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }

  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
}
void SensorPressaoAtm(){
  ConfigSensorPressao();
  float temperatura = bmp.readTemperature();
  float pressurePa = bmp.readPressure();
  float pressureBar = pressurePa * 1e-5;
  float altitude = bmp.readAltitude(1011.9);

    Serial.print(F("Temperature = "));
    Serial.print(bmp.readTemperature());
    Serial.println(" *C");
    

    Serial.print(F("Pressure = "));
    Serial.print(bmp.readPressure());
  
    Serial.print("Pressure = ");
    Serial.print(pressureBar, 6);  // 6 decimal places
    Serial.println(" atm");

    Serial.print(F("Approx altitude = "));
    Serial.print(bmp.readAltitude(1011.9)); /* Adjusted to local forecast! */
    Serial.println(" m");

    Serial.println();
    delay(2000);
}
// ///////////////////////////////////////////
void ConfigFirebase() {

    config.api_key = FIREBASE_API_KEY;
    config.database_url = FIREBASE_DATABASE_URL;

    // Autenticação via legacy token (database secret)
    config.signer.tokens.legacy_token = FIREBASE_AUTH;

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}

void enviarDadosFirebase() {

    // =========================================================
    // VERIFICA FIREBASE
    // =========================================================

    if (!Firebase.ready()) {
        Serial.println("Firebase não está pronto.");
        return;
    }


    // =========================================================
    // JSON PRINCIPAL
    // =========================================================

    FirebaseJson json;


    // =========================================================
    // INFORMAÇÕES DA ESTAÇÃO
    // =========================================================

    json.set("info/nome",dados_finais->nomeEstacao);
    json.set("info/ativo",dados_finais->estacaoAtiva);


    // =========================================================
    // LUMINOSIDADE
    // =========================================================

    // json.set("sensores/luminosidade/valor", dados.luminosidade);
    // json.set("sensores/luminosidade/unidade", "lux");
    // json.set("sensores/luminosidade/ativo", dados.luminosidadeAtiva);
    // json.set("sensores/luminosidade/timestamp", dados.timestamp);


    // =========================================================
    // UV
    // =========================================================

    json.set("sensores/uv/valor",dados_finais->valor);
    // json.set("sensores/uv/unidade", "indice");
    // json.set("sensores/uv/ativo", dados.uvAtivo);
    // json.set("sensores/uv/timestamp", dados.timestamp);


    // =========================================================
    // PRESSÃO ATMOSFÉRICA
    // =========================================================

    json.set("sensores/pressao/temperatura/valor",dados_finais->temperatura);
    json.set("sensores/pressao/temperatura/unidade", "°C");

    json.set("sensores/pressao/pressurePa/valor",dados_finais->pressurePa);
    json.set("sensores/pressao/pressurePa/unidade", "Pa");

    json.set("sensores/pressao/pressureBar/valor",dados_finais->pressureBar);
    json.set("sensores/pressao/pressureBar/unidade", "bar");

    json.set("sensores/pressao/altitude/valor",dados_finais->altitude);
    json.set("sensores/pressao/altitude/unidade", "m");


    // =========================================================
    // CHUVA
    // =========================================================

    // json.set("sensores/chuva/valor", dados.volumeChuva);
    // json.set("sensores/chuva/unidade", "mm");
    // json.set("sensores/chuva/ativo", dados.chuvaAtiva);
    // json.set("sensores/chuva/timestamp", dados.timestamp);


    // =========================================================
    // VENTO - VELOCIDADE
    // =========================================================

    // json.set("sensores/vento/velocidade/valor",dados.velocidadeVento);

    // json.set("sensores/vento/velocidade/unidade","km/h");

    // json.set("sensores/vento/velocidade/ativo",dados.velocidadeVentoAtiva);

    // json.set("sensores/vento/velocidade/timestamp",dados.timestamp);


    // // =========================================================
    // // VENTO - DIREÇÃO
    // // =========================================================

    // json.set("sensores/vento/direcao/valor",dados.direcaoVento);

    // json.set("sensores/vento/direcao/unidade","graus");

    // json.set("sensores/vento/direcao/ativo",dados.direcaoVentoAtiva);

    // json.set("sensores/vento/direcao/timestamp",dados.timestamp);


    // =========================================================
    // GPS
    // =========================================================

    json.set("sensores/gps/latitude",dados_finais->latitude);

    json.set("sensores/gps/longitude",dados_finais->longitude);

    // json.set("sensores/gps/timestamp",dados_finais->timestamp);


    // =========================================================
    // ENVIO
    // =========================================================

      String caminho = FIREBASE_PATH;
    Serial.println("Enviando dados para Firebase...");

    if (Firebase.RTDB.updateNode(&firebaseData, caminho, &json)) {
        Serial.println("Dados enviados com sucesso!");
    } else {
        Serial.print("Erro ao enviar para Firebase: ");
        Serial.println(firebaseData.errorReason());
    }
}
// //////////////////////////////////////////

void wifi() {
  Serial.println("Tentando conectar ao WiFi...");
  
  WiFi.begin(ssid, password);  // Chama apenas uma vez

  byte count_wifi_attempt = 0;
  while (count_wifi_attempt < 20 && WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
    count_wifi_attempt++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado ao WiFi!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha ao tentar conectar ao WiFi.");
  }
}

///////////////////////////////////////////////////////////////
  // Lê todos os caracteres disponíveis no GPS
void LerGps(){
    while (SerialGPS.available()) {
        gps.encode(SerialGPS.read());
    }
    // Latitude
    if (gps.location.isValid()) {
        dados_finais->latitude = gps.location.lat();
        Serial.print("Latitude: ");
        Serial.println(dados_finais->latitude, 6);

        dados_finais->longitude = gps.location.lng();
        Serial.print("Longitude: ");
        Serial.println(dados_finais->longitude, 6);

    } else {
        dados_finais->latitude = 0.0;
        dados_finais->longitude = 0.0;
    }

  }

void SensorVolumeChuva(){}
void SensorVelociadadeVento(){}
void SensorDirecaoVento(){}

void setup() {
  // pinMode(pinoDO, INPUT);
  pinMode(pinoLED, OUTPUT);
  Serial.begin(9600); // ESP32 usa melhor 115200
  analogReadResolution(12); // ESP32: 0–4095

  SerialGPS.begin(
        GPS_BAUD,
        SERIAL_8N1,
        GPS_RX,
        GPS_TX
    );

  wifi();    // conexao wifi ok
  ConfigFirebase();
}

void loop() {

  // SensorVolumeChuva(); -> precisa desenvolver a funcao ainda
  // SensorVelociadadeVento(); -> precisa desenvolver a funcao ainda
  // SensorDirecaoVento(); -> precisa desenvolver a funcao ainda
  // SensorLuminosidade(); -> preciso da funcao que tem
  // SensorUv();  conexao ok
  // SensorPressaoAtm(); conexao ok
  // LerGps(); precisa testar na placa 
  if (WiFi.status() != WL_CONNECTED) {
    wifi();
  }
  enviarDadosFirebase(); // esta funcionando corretamente
  delay(5000);
}
