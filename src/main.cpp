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
Data dados_finais; // dados lidos dos sensores

bool dados_Enviados = false;

Data dadosLidos;  // dados lidos do arquivo 

#define DELAY_COLETA 60000  //1 minuto
//#define DELAY_COLETA 300000  //5 minutos
//#define DELAY_COLETA 600000  //10 minutos

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

    if (!Firebase.ready()) {
        Serial.println("Firebase não está pronto.");
        return;
    }

    static bool infoEnviada = false;

    // =========================================================
    // INFO DA ESTAÇÃO
    // =========================================================

    if (!infoEnviada) {

        FirebaseJson infoJson;
        infoJson.set("nome", dados_finais.nomeEstacao);
        infoJson.set("ativo", dados_finais.estacaoAtiva);

        if (Firebase.RTDB.updateNode(
                &firebaseData,
                String(FIREBASE_PATH) + "/info",
                &infoJson)) {

            Serial.println("Info da estação enviada com sucesso!");
            infoEnviada = true;

        } else {
            Serial.print("Erro ao enviar info da estação: ");
            Serial.println(firebaseData.errorReason());
        }
    }

    // =========================================================
    // LÊ TODOS OS REGISTROS PENDENTES PARA MEMÓRIA
    // =========================================================

    File file = LittleFS.open("/dados.bin", FILE_READ);
    if (!file) {
        Serial.println("ERRO: não foi possível abrir dados.bin");
        return;
    }

    int totalRegistros = file.size() / sizeof(Data);

    if (totalRegistros == 0) {
        file.close();
        Serial.println("Nenhum dado pendente para enviar.");
        return;
    }

    Data* registros = new Data[totalRegistros];

    for (int i = 0; i < totalRegistros; i++) {
        file.read((uint8_t*)&registros[i], sizeof(Data));
    }

    file.close();

    // =========================================================
    // ENVIA UM POR UM, EM ORDEM, PARA NO PRIMEIRO ERRO
    // =========================================================

    int enviados = 0;

    for (int i = 0; i < totalRegistros; i++) {

        FirebaseJson registroJson;
        Data &r = registros[i];

        registroJson.set("dataHora/timestamp", r.timestamp);
        registroJson.set("dataHora/ano", r.ano);
        registroJson.set("dataHora/mes", r.mes);
        registroJson.set("dataHora/dia", r.dia);
        registroJson.set("dataHora/hora", r.hora);
        registroJson.set("dataHora/minuto", r.minuto);
        registroJson.set("dataHora/segundo", r.segundo);

        registroJson.set("uv/valor", r.valor);

        registroJson.set("pressao/temperatura/valor", r.temperatura);
        registroJson.set("pressao/temperatura/unidade", "°C");
        registroJson.set("pressao/pressurePa/valor", r.pressurePa);
        registroJson.set("pressao/pressurePa/unidade", "Pa");
        registroJson.set("pressao/pressureBar/valor", r.pressureBar);
        registroJson.set("pressao/pressureBar/unidade", "bar");
        registroJson.set("pressao/altitude/valor", r.altitude);
        registroJson.set("pressao/altitude/unidade", "m");

        registroJson.set("gps/latitude", r.latitude);
        registroJson.set("gps/longitude", r.longitude);

        registroJson.set("chuva/valor", r.volumeChuva);
        registroJson.set("chuva/unidade", "mm");
        registroJson.set("chuva/ativo", r.chuvaAtiva);

        registroJson.set("vento/velocidade/valor", r.velocidadeVento);
        registroJson.set("vento/velocidade/unidade", "km/h");
        registroJson.set("vento/velocidade/ativo", r.velocidadeVentoAtiva);
        registroJson.set("vento/direcao/valor", r.direcaoVento);
        registroJson.set("vento/direcao/unidade", "graus");
        registroJson.set("vento/direcao/ativo", r.direcaoVentoAtiva);

        registroJson.set("luminosidade/valor", r.luminosidade);
        registroJson.set("luminosidade/unidade", "lux");
        registroJson.set("luminosidade/ativo", r.luminosidadeAtiva);

        if (Firebase.RTDB.pushJSON(&firebaseData, String(FIREBASE_PATH) + "/leituras", &registroJson)) {

            Serial.print("Registro enviado com sucesso! Timestamp: ");
            Serial.println(r.timestamp);
            enviados++;

        } else {

            Serial.print("Erro ao enviar registro (timestamp ");
            Serial.print(r.timestamp);
            Serial.print("): ");
            Serial.println(firebaseData.errorReason());
            break; // para aqui, o resto fica pendente
        }
    }

    // =========================================================
    // REGRAVA O ARQUIVO SÓ COM O QUE FALTA ENVIAR
    // =========================================================

    LittleFS.remove("/dados.bin");

    int restantes = totalRegistros - enviados;

    if (restantes > 0) {
        File fileW = LittleFS.open("/dados.bin", FILE_WRITE);
        if (fileW) {
            for (int i = enviados; i < totalRegistros; i++) {
                fileW.write((uint8_t*)&registros[i], sizeof(Data));
            }
            fileW.flush();
            fileW.close();
        }
    }

    Serial.print("Enviados: ");
    Serial.print(enviados);
    Serial.print(" | Pendentes: ");
    Serial.println(restantes);

    delete[] registros;
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

        // teste
        Serial.println("===== DADOS ANTES DE SALVAR =====");

        Serial.print("Timestamp: ");
        Serial.println(dados_finais.timestamp);

        Serial.print("Data: ");
        Serial.print(dados_finais.dia);
        Serial.print("/");
        Serial.print(dados_finais.mes);
        Serial.print("/");
        Serial.println(dados_finais.ano);

        Serial.print("Hora: ");
        Serial.print(dados_finais.hora);
        Serial.print(":");
        Serial.print(dados_finais.minuto);
        Serial.print(":");
        Serial.println(dados_finais.segundo);

        Serial.println("=================================");
}
void lerDados() {

    File file = LittleFS.open("/dados.bin", FILE_READ);

    if (!file) {
        Serial.println("ERRO: não foi possível abrir dados.bin");
        return;
    }

    Serial.print("Tamanho do arquivo: ");
    Serial.println(file.size());

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


         Serial.println("===== DADOS APOS SALVAR =====");

        Serial.print("Timestamp: ");
        Serial.println(dados_finais.timestamp);

        Serial.print("Data: ");
        Serial.print(dados_finais.dia);
        Serial.print("/");
        Serial.print(dados_finais.mes);
        Serial.print("/");
        Serial.println(dados_finais.ano);

        Serial.print("Hora: ");
        Serial.print(dados_finais.hora);
        Serial.print(":");
        Serial.print(dados_finais.minuto);
        Serial.print(":");
        Serial.println(dados_finais.segundo);

        Serial.println("=================================");
    }

    file.close();
}
void limparArquivo() {

if(dados_Enviados == true)
  {
  File file = LittleFS.open("/dados.bin", FILE_WRITE);  // Abre em modo de escrita, o que limpa o conteúdo
  if (!file) {
    Serial.println("Erro ao abrir o arquivo para limpeza");
    return;
  }
  file.close();  // Fecha o arquivo imediatamente para garantir que fique vazio
  Serial.println("Arquivo limpo após envio para o BD");

  dados_Enviados=false;
 }else{
    Serial.print("Dados nao enviados");
 }
}
void light_sleep() {
  esp_sleep_enable_timer_wakeup(DELAY_COLETA * 1000);  
  esp_light_sleep_start();
  delay(500);  
  Serial.println("ESP32 acordou do Light Sleep!");
}
//  funcao que pega a hora na internet
void initTime() {

    Serial.println("InitTime!");

    configTime(-3 * 3600, 0, "pool.ntp.org");

    struct tm timeinfo;
    int tentativas = 0;

    while (!getLocalTime(&timeinfo) && tentativas < 10) {
        Serial.println("Aguardando sincronização NTP...");
        delay(1000);
        tentativas++;
    }

    if (tentativas >= 10) {
        Serial.println("Falha ao obter tempo após 10 tentativas");
    }

    // Salva na struct global dados_finais
    dados_finais.ano     = timeinfo.tm_year + 1900;
    dados_finais.mes     = timeinfo.tm_mon + 1;
    dados_finais.dia     = timeinfo.tm_mday;
    dados_finais.hora    = timeinfo.tm_hour;
    dados_finais.minuto  = timeinfo.tm_min;
    dados_finais.segundo = timeinfo.tm_sec;

    // Timestamp Unix
    dados_finais.timestamp = mktime(&timeinfo);

    Serial.println(&timeinfo, "Horário sincronizado: %d/%m/%Y %H:%M:%S");

}
void SensorVolumeChuva(){}
void SensorVelociadadeVento(){}
void SensorDirecaoVento(){}

void setup() {
  // pinMode(pinoDO, INPUT);
  pinMode(pinoLED, OUTPUT);
  Serial.begin(9600); 
  analogReadResolution(12); // ESP32: 0–4095

  SerialGPS.begin(GPS_BAUD,SERIAL_8N1,GPS_RX,GPS_TX);

    // Monta o sistema de arquivos LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("ERRO: falha ao montar LittleFS!");
        return;
    }

    Serial.println("LittleFS montado com sucesso!");


}

void loop() {

    wifi();    // conexao wifi ok
    delay(1000);
    ConfigFirebase();
    // 2. Obter hora
    delay(1000);
    initTime();

    // 3. Fazer as medições
    // SensorVolumeChuva();
    // SensorVelociadadeVento();
    // SensorDirecaoVento();
    // SensorLuminosidade();
    // SensorUv();
    // SensorPressaoAtm();
    // LerGps();

    // 4. Salvar localmente
    delay(1000);

    salvarDados();
    delay(1000);

    // 5. Ler dados pendentes
    lerDados();
    delay(1000);

    // 6. Enviar para Firebase
    enviarDadosFirebase();
    delay(1000);

    // 7. Limpar somente depois de confirmar envio
    limparArquivo();
    delay(1000);

    // 8. Encerrar/desconectar conexões
    // aqui depende da biblioteca Firebase utilizada

    // 9. Dormir
    light_sleep();
    delay(1000);

    // Não precisa desse delay de 5 segundos
}
