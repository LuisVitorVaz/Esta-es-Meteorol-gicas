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

    uint16_t ano = 0;
    uint8_t mes = 0;
    uint8_t dia = 0;
    uint8_t hora = 0;
    uint8_t minuto = 0;
    uint8_t segundo = 0;

    // SENSOR - LUMINOSIDADE

    float luminosidade;
    bool luminosidadeAtiva;

    // SENSOR - UV

    float valor = 0;


    // SENSOR - PRESSÃO ATMOSFÉRICA

    float temperatura = 0;
    float pressurePa = 0;
    float pressureBar = 0;
    float altitude = 0;


    // SENSOR - CHUVA

    float volumeChuva = 0.0;
    bool chuvaAtiva;

    // SENSOR - VENTO

    float velocidadeVento = 0.0;
    bool velocidadeVentoAtiva;

    float direcaoVento = 0.0;
    bool direcaoVentoAtiva;

    // SENSOR - GPS
  
    double latitude = 0;
    double longitude = 0;
    bool gpsAtivo;

} Data;
Data dados_finais;


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
  dados_finais.valor = sensorVoltage;
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
    // INFORMAÇÕES DA ESTAÇÃO (estado atual, não é histórico)
    // =========================================================

    static bool infoEnviada = false;

    if (!infoEnviada) {
        FirebaseJson infoJson;
        infoJson.set("nome", dados_finais.nomeEstacao);
        infoJson.set("ativo", dados_finais.estacaoAtiva);

        if (Firebase.RTDB.updateNode(&firebaseData, String(FIREBASE_PATH) + "/info", &infoJson)) {
            Serial.println("Info da estação enviada com sucesso!");
            infoEnviada = true;
        } else {
            Serial.print("Erro ao enviar info da estação: ");
            Serial.println(firebaseData.errorReason());
        }
    }

    bool ok = true;

    // =========================================================
    // LUMINOSIDADE
    // =========================================================

    // FirebaseJson luzJson;
    // luzJson.set("timestamp/.sv", "timestamp");
    // luzJson.set("valor", dados_finais->luminosidade);
    // luzJson.set("unidade", "lux");
    // luzJson.set("ativo", dados_finais->luminosidadeAtiva);
    // if (!Firebase.RTDB.pushJSON(&firebaseData, String(FIREBASE_PATH) + "/sensores/luminosidade/leituras", &luzJson)) {
    //     Serial.print("Erro luminosidade: ");
    //     Serial.println(firebaseData.errorReason());
    //     ok = false;
    // }


    // =========================================================
    // UV
    // =========================================================

    FirebaseJson uvJson;
    uvJson.set("timestamp/.sv", "timestamp");
    uvJson.set("valor", dados_finais.valor);
    // uvJson.set("unidade", "indice");
    // uvJson.set("ativo", dados_finais->uvAtivo);

    if (!Firebase.RTDB.pushJSON(&firebaseData, String(FIREBASE_PATH) + "/sensores/uv/leituras", &uvJson)) {
        Serial.print("Erro ao enviar UV: ");
        Serial.println(firebaseData.errorReason());
        ok = false;
    }


    // =========================================================
    // PRESSÃO ATMOSFÉRICA
    // =========================================================

    FirebaseJson pressaoJson;
    pressaoJson.set("timestamp/.sv", "timestamp");
    pressaoJson.set("temperatura/valor", dados_finais.temperatura);
    pressaoJson.set("temperatura/unidade", "°C");
    pressaoJson.set("pressurePa/valor", dados_finais.pressurePa);
    pressaoJson.set("pressurePa/unidade", "Pa");
    pressaoJson.set("pressureBar/valor", dados_finais.pressureBar);
    pressaoJson.set("pressureBar/unidade", "bar");
    pressaoJson.set("altitude/valor", dados_finais.altitude);
    pressaoJson.set("altitude/unidade", "m");

    if (!Firebase.RTDB.pushJSON(&firebaseData, String(FIREBASE_PATH) + "/sensores/pressao/leituras", &pressaoJson)) {
        Serial.print("Erro ao enviar pressão: ");
        Serial.println(firebaseData.errorReason());
        ok = false;
    }


    // =========================================================
    // CHUVA
    // =========================================================

    // FirebaseJson chuvaJson;
    // chuvaJson.set("timestamp/.sv", "timestamp");
    // chuvaJson.set("valor", dados_finais->volumeChuva);
    // chuvaJson.set("unidade", "mm");
    // chuvaJson.set("ativo", dados_finais->chuvaAtiva);
    // if (!Firebase.RTDB.pushJSON(&firebaseData, String(FIREBASE_PATH) + "/sensores/chuva/leituras", &chuvaJson)) {
    //     Serial.print("Erro chuva: ");
    //     Serial.println(firebaseData.errorReason());
    //     ok = false;
    // }


    // =========================================================
    // VENTO - VELOCIDADE E DIREÇÃO
    // =========================================================

    // FirebaseJson ventoJson;
    // ventoJson.set("timestamp/.sv", "timestamp");
    // ventoJson.set("velocidade/valor", dados_finais->velocidadeVento);
    // ventoJson.set("velocidade/unidade", "km/h");
    // ventoJson.set("velocidade/ativo", dados_finais->velocidadeVentoAtiva);
    // ventoJson.set("direcao/valor", dados_finais->direcaoVento);
    // ventoJson.set("direcao/unidade", "graus");
    // ventoJson.set("direcao/ativo", dados_finais->direcaoVentoAtiva);
    // if (!Firebase.RTDB.pushJSON(&firebaseData, String(FIREBASE_PATH) + "/sensores/vento/leituras", &ventoJson)) {
    //     Serial.print("Erro vento: ");
    //     Serial.println(firebaseData.errorReason());
    //     ok = false;
    // }


    // =========================================================
    // GPS
    // =========================================================

    FirebaseJson gpsJson;
    gpsJson.set("timestamp/.sv", "timestamp");
    gpsJson.set("latitude", dados_finais.latitude);
    gpsJson.set("longitude", dados_finais.longitude);

    if (!Firebase.RTDB.pushJSON(&firebaseData, String(FIREBASE_PATH) + "/sensores/gps/leituras", &gpsJson)) {
        Serial.print("Erro ao enviar GPS: ");
        Serial.println(firebaseData.errorReason());
        ok = false;
    }

    if (ok) {
        Serial.println("Todos os dados enviados com sucesso!");
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
        dados_finais.latitude = gps.location.lat();
        Serial.print("Latitude: ");
        Serial.println(dados_finais.latitude, 6);

        dados_finais.longitude = gps.location.lng();
        Serial.print("Longitude: ");
        Serial.println(dados_finais.longitude, 6);

    } else {
        dados_finais.latitude = 0.0;
        dados_finais.longitude = 0.0;
    }

  }
  // verificar essa funcao 
void salvarDados() {

    File file = LittleFS.open("/dados.bin", FILE_APPEND);

    if (!file) {
        Serial.println("Erro ao abrir /dados.bin para escrita");
        return;
    }

    size_t bytesGravados = file.write((uint8_t*)&dados_finais,sizeof(Data));

    file.flush();
    file.close();

    if (bytesGravados == sizeof(Data)) {
        Serial.println("Dados salvos com sucesso!");
    } else {
        Serial.println("Erro ao salvar os dados!");
    }
}
void lerDados() {

    File file = LittleFS.open("/dados.bin", FILE_READ);

    if (!file) {
        Serial.println("ERRO: não foi possível abrir dados.bin");
        return;
    }

    Serial.print("Tamanho do arquivo: ");
    Serial.println(file.size());

    Data dadosLidos;

    while (file.available()) {

        size_t bytesLidos = file.read(
            (uint8_t*)&dadosLidos,
            sizeof(Data)
        );

        if (bytesLidos != sizeof(Data)) {
            Serial.println("ERRO: registro incompleto!");
            break;
        }

        Serial.println("-------------------------");
        Serial.println("REGISTRO ENCONTRADO");

        Serial.print("Nome: ");
        Serial.println(dadosLidos.nomeEstacao);

        Serial.print("Firmware: ");
        Serial.println(dadosLidos.firmware);

        Serial.print("Temperatura: ");
        Serial.println(dadosLidos.temperatura);

        Serial.print("Pressao Pa: ");
        Serial.println(dadosLidos.pressurePa);

        Serial.print("Pressao Bar: ");
        Serial.println(dadosLidos.pressureBar);

        Serial.print("Altitude: ");
        Serial.println(dadosLidos.altitude);

        Serial.print("Luminosidade: ");
        Serial.println(dadosLidos.luminosidade);

        Serial.print("UV: ");
        Serial.println(dadosLidos.valor);

        Serial.print("Latitude: ");
        Serial.println(dadosLidos.latitude, 6);

        Serial.print("Longitude: ");
        Serial.println(dadosLidos.longitude, 6);

        Serial.print("Velocidade vento: ");
        Serial.println(dadosLidos.velocidadeVento);

        Serial.print("Direcao vento: ");
        Serial.println(dadosLidos.direcaoVento);

        Serial.print("Chuva: ");
        Serial.println(dadosLidos.volumeChuva);

        Serial.print("Timestamp: ");
        Serial.println(dadosLidos.timestamp);
    }

    file.close();
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

    // Monta o sistema de arquivos LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("ERRO: falha ao montar LittleFS!");
        return;
    }

    Serial.println("LittleFS montado com sucesso!");

  wifi();    // conexao wifi ok
  ConfigFirebase();
}

void loop() {
  salvarDados();  // funcao ok salvando os dados falta testar por horas
  lerDados(); // esta recuperando os dados corretamente
  // SensorVolumeChuva(); -> precisa desenvolver a funcao ainda
  // SensorVelociadadeVento(); -> precisa desenvolver a funcao ainda
  // SensorDirecaoVento(); -> precisa desenvolver a funcao ainda
  // SensorLuminosidade(); -> preciso da funcao que tem
  // SensorUv();  conexao ok
  // SensorPressaoAtm(); conexao ok
  // LerGps(); precisa testar na placa 
  // if (WiFi.status() != WL_CONNECTED) {
  //   wifi();
  // }
  // enviarDadosFirebase(); // esta funcionando corretamente
  // delay(5000);
}
