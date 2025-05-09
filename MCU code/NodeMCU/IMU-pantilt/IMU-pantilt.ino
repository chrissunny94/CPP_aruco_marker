

/*
Pin numbers written on the board itself do not correspond to ESP8266 GPIO pin numbers. We have constants defined to make using this board easier:

static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;
 *  Enjoy your kids
 *########################################################### */

/*
 * Definições de alguns endereços mais comuns do MPU6050
 * os registros podem ser facilmente encontrados no mapa de registros do MPU6050
 */

#include <Servo.h>
int PanservoPin = 13;
int Panangle = 90;
Servo Panservo;
int TiltservoPin = 15;
int Tiltangle = 90;
Servo Tiltservo; 

  
 
  
 

#include <WiFiUdp.h>
WiFiUDP Udp;
unsigned int localPort = 8080;



#include <ESP8266WiFi.h>  // biblioteca para usar as funções de Wifi do módulo ESP8266
#include <Wire.h>         // biblioteca de comunicação I2C
#include <ArduinoJson.h>  // biblioteca JSON para sistemas embarcados

const int MPU_ADDR =      0x68; // definição do endereço do sensor MPU6050 (0x68)
const int WHO_AM_I =      0x75; // registro de identificação do dispositivo
const int PWR_MGMT_1 =    0x6B; // registro de configuração do gerenciamento de energia
const int GYRO_CONFIG =   0x1B; // registro de configuração do giroscópio
const int ACCEL_CONFIG =  0x1C; // registro de configuração do acelerômetro
const int ACCEL_XOUT =    0x3B; // registro de leitura do eixo X do acelerômetro

const int sda_pin = D5; // definição do pino I2C SDA
const int scl_pin = D6; // definição do pino I2C SCL

bool led_state = false;

// variáveis para armazenar os dados "crus" do acelerômetro
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ; 

// Definições da rede Wifi
const char* SSID = "Vanora Robots";
const char* PASSWORD = "Livingspaces8";

// endereço IP local do Servidor Web instalado na Raspberry Pi 3
// onde será exibida a página web
const char* rpiHost = "192.168.86.101";  

// servidor que disponibiliza serviço de geolocalização via IP    
const char* IpApiHost = "ip-api.com";   

WiFiClient client;

// construindo o objeto JSON que irá armazenar os dados do acelerômetro na função populateJSON()
StaticJsonBuffer<300> jsonBuffer;
JsonObject& object = jsonBuffer.createObject();
JsonObject& data = object.createNestedObject("data");
  
JsonObject& accel = data.createNestedObject("accel");
JsonObject& temp = data.createNestedObject("temp");
JsonObject& gyro = data.createNestedObject("gyro");
  
JsonObject& accelX = accel.createNestedObject("accelX");
JsonObject& accelY = accel.createNestedObject("accelY");
JsonObject& accelZ = accel.createNestedObject("accelZ");

JsonObject& gyroX = gyro.createNestedObject("gyroX");
JsonObject& gyroY = gyro.createNestedObject("gyroY");
JsonObject& gyroZ = gyro.createNestedObject("gyroZ");

/*
 * função que configura a I2C com os pinos desejados 
 * sda_pin -> D5
 * scl_pin -> D6
 */
void initI2C() 
{
  //Serial.println("---inside initI2C");
  Wire.begin(sda_pin, scl_pin);
}

/*
 * função que escreve um dado valor em um dado registro
 */
void writeRegMPU(int reg, int val)
{
  Wire.beginTransmission(MPU_ADDR);     // inicia comunicação com endereço do MPU6050
  Wire.write(reg);                      // envia o registro com o qual se deseja trabalhar
  Wire.write(val);                      // escreve o valor no registro
  Wire.endTransmission(true);           // termina a transmissão
}

/*
 * função que lê de um dado registro
 */
uint8_t readRegMPU(uint8_t reg)
{
  uint8_t data;
  Wire.beginTransmission(MPU_ADDR);     // inicia comunicação com endereço do MPU6050
  Wire.write(reg);                      // envia o registro com o qual se deseja trabalhar
  Wire.endTransmission(false);          // termina transmissão mas continua com I2C aberto (envia STOP e START)
  Wire.requestFrom(MPU_ADDR, 1);        // configura para receber 1 byte do registro escolhido acima
  data = Wire.read();                   // lê o byte e guarda em 'data'
  return data;                          //retorna 'data'
}

/*
 * função que procura pelo sensor no endereço 0x68
 */
void findMPU(int mpu_addr)
{
  Wire.beginTransmission(MPU_ADDR);
  int data = Wire.endTransmission(true);

  if(data == 0)
  {
    Serial.print("Dispositivo encontrado no endereço: 0x");
    Serial.println(MPU_ADDR, HEX);
  }
  else 
  {
    Serial.println("Dispositivo não encontrado!");
  }
}

/*
 * função que verifica se o sensor responde e se está ativo
 */
void checkMPU(int mpu_addr)
{
  findMPU(MPU_ADDR);
    
  int data = readRegMPU(WHO_AM_I); // Register 117 – Who Am I - 0x75
  
  if(data == 104) 
  {
    Serial.println("MPU6050 Dispositivo respondeu OK! (104)");

    data = readRegMPU(PWR_MGMT_1); // Register 107 – Power Management 1-0x6B

    if(data == 64) Serial.println("MPU6050 em modo SLEEP! (64)");
    else Serial.println("MPU6050 em modo ACTIVE!"); 
  }
  else Serial.println("Verifique dispositivo - MPU6050 NÃO disponível!");
}

/*
 * função de inicialização do sensor
 */
void initMPU()
{
  setSleepOff();
  setGyroScale();
  setAccelScale();
}

/* 
 *  função para configurar o sleep bit  
 */
void setSleepOff()
{
  writeRegMPU(PWR_MGMT_1, 0); // escreve 0 no registro de gerenciamento de energia(0x68), colocando o sensor em o modo ACTIVE
}

/* função para configurar as escalas do giroscópio
   registro da escala do giroscópio: 0x1B[4:3]
   0 é 250°/s

    FS_SEL  Full Scale Range
      0        ± 250 °/s      0b00000000
      1        ± 500 °/s      0b00001000
      2        ± 1000 °/s     0b00010000
      3        ± 2000 °/s     0b00011000
*/
void setGyroScale()
{
  writeRegMPU(GYRO_CONFIG, 0);
}

/* função para configurar as escalas do acelerômetro
   registro da escala do acelerômetro: 0x1C[4:3]
   0 é 250°/s

    AFS_SEL   Full Scale Range
      0           ± 2g            0b00000000
      1           ± 4g            0b00001000
      2           ± 8g            0b00010000
      3           ± 16g           0b00011000
*/
void setAccelScale()
{
  writeRegMPU(ACCEL_CONFIG, 0);
}

/* função que lê os dados 'crus'(raw data) do sensor
   são 14 bytes no total sendo eles 2 bytes para cada eixo e 2 bytes para temperatura:

  0x3B 59 ACCEL_XOUT[15:8]
  0x3C 60 ACCEL_XOUT[7:0]
  0x3D 61 ACCEL_YOUT[15:8]
  0x3E 62 ACCEL_YOUT[7:0]
  0x3F 63 ACCEL_ZOUT[15:8]
  0x40 64 ACCEL_ZOUT[7:0]

  0x41 65 TEMP_OUT[15:8]
  0x42 66 TEMP_OUT[7:0]

  0x43 67 GYRO_XOUT[15:8]
  0x44 68 GYRO_XOUT[7:0]
  0x45 69 GYRO_YOUT[15:8]
  0x46 70 GYRO_YOUT[7:0]
  0x47 71 GYRO_ZOUT[15:8]
  0x48 72 GYRO_ZOUT[7:0]
   
*/
void readRawMPU()
{  
  Wire.beginTransmission(MPU_ADDR);       // inicia comunicação com endereço do MPU6050
  Wire.write(ACCEL_XOUT);                       // envia o registro com o qual se deseja trabalhar, começando com registro 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);            // termina transmissão mas continua com I2C aberto (envia STOP e START)
  Wire.requestFrom(MPU_ADDR, 14);         // configura para receber 14 bytes começando do registro escolhido acima (0x3B)

  AcX = Wire.read() << 8;                 // lê primeiro o byte mais significativo
  AcX |= Wire.read();                     // depois lê o bit menos significativo
  AcY = Wire.read() << 8;
  AcY |= Wire.read();
  AcZ = Wire.read() << 8;
  AcZ |= Wire.read();

  Tmp = Wire.read() << 8;
  Tmp |= Wire.read();

  GyX = Wire.read() << 8;
  GyX |= Wire.read();
  GyY = Wire.read() << 8;
  GyY |= Wire.read();
  GyZ = Wire.read() << 8;
  GyZ |= Wire.read(); 

  Serial.print("AcX = "); Serial.print(AcX);
  Serial.print(" | AcY = "); Serial.print(AcY);
  Serial.print(" | AcZ = "); Serial.print(AcZ);
  Serial.print(" | Tmp = "); Serial.print(Tmp/340.00+36.53);
  Serial.print(" | GyX = "); Serial.print(GyX);
  Serial.print(" | GyY = "); Serial.print(GyY);
  Serial.print(" | GyZ = "); Serial.println(GyZ);

  led_state = !led_state;
  digitalWrite(LED_BUILTIN, led_state);         // pisca LED do NodeMCU a cada leitura do sensor
  delay(50);                                        
}

/*
 * função que conecta o NodeMCU na rede Wifi
 * SSID e PASSWORD devem ser indicados nas variáveis
 */
void reconnectWiFi() 
{
  if(WiFi.status() == WL_CONNECTED)
    return;

  WiFi.begin(SSID, PASSWORD);

  while(WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Succefully connected to wifi ");
  Serial.println(SSID);
  Serial.print("IP obtained: ");
  Serial.println(WiFi.localIP());  
}

void initWiFi()
{
  delay(10);
  Serial.print("Conecting to the wifi: ");
  Serial.println(SSID);
  Serial.println("letsee");

  reconnectWiFi();
}

/*
 * função que armazena cada dado do sensor em um objeto JSON
 * utiliza a biblioteca ArduinoJson
 */
void populateJSON()
{
  object["nodeID"] = "mcu1";

  accel["accelX"] = AcX;
  accel["accelY"] = AcY;
  accel["accelZ"] = AcZ;

  data["temp"] = Tmp/340.00+36.53;

  gyro["gyroX"] = GyX;
  gyro["gyroY"] = GyY;
  gyro["gyroZ"] = GyZ;
}

/*
 * função que envia os dados do sensor para o servidor em formato JSON
 * faz um POST request ao servidor 
 */
void makePOST()
{
  if(!client.connect(rpiHost, 3000))     // aqui conectamos ao servidor
  {
    Serial.println("Não foi possível conectar ao servidor!\n");
  }
  else
  {    
    Serial.println("Conectado ao servidor");
    // Make HTTP POST request    
    client.println("POST /accel HTTP/1.1");
    client.println("Host: 192.168.0.24");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(object.measureLength());
    client.println();
    object.printTo(client);    // enviamos os dados em JSON    
  }
}

/*
 * Função que realiza GET request no site ip-api.com
 * Esse site disponibiliza uma API de geolocalização via IP
 * A função retorna um JSON com dados de geolocalização
 * Os dados de geolocalização são exibidos na página web em um Google Maps
 */
String makeGETlocation()
{
  if ( !client.connect(IpApiHost, 80) ) {
    Serial.println("connection to ip-api.com failed");
    return "connection failed";
  }
  
  // Realiza HTTP GET request
  client.println("GET /json/?fields=8405 HTTP/1.1");
  client.print("Host: ");
  client.println(IpApiHost);
  client.println("Connection: close");
  client.println();

  // recebe o Header de resposta
  while (client.connected()) {
    String data = client.readStringUntil('\n');
    if (data == "\r") {
      break;
    }
  }
  // recebe os dados de geolocalização em formato JSON e guarda na variável data
  String data = client.readStringUntil('\n');
  Serial.println("your current Geo location :\n");
  Serial.println(data);  
  return data; 
}


void makePOSTlocation()
{
  String location = makeGETlocation(); // guarda o JSON de geolocalização na variável location
  if(!client.connect(rpiHost, 3000))     // aqui conectamos ao servidor
  {
    Serial.print("Could not connect to host: \n");
    Serial.print(rpiHost);
  }
  else
  {
    // realiza HTTP POST request    
    client.println("POST /location HTTP/1.1");
    client.println("Host: 192.168.0.24");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(location.length());
    client.println();
    client.println(location);    // enviamos o JSON ao servidor
  }
}









void Pandrive(int value){
  Panangle = Panangle + value;
  
  if(Panangle>180)
    Panangle = 180;
  else if (Panangle <0)
    Panangle = 0;
  Serial.println("Panangle=");
  Serial.print(Panangle);
  Panservo.write(Panangle);
  }

 void Tiltdrive(int value){
  Tiltangle = Tiltangle + value;
  
  if(Tiltangle>180)
    Tiltangle = 180;
  else if (Tiltangle <0)
    Tiltangle = 0;
  Serial.println("Titleangle=");
  Serial.print(Tiltangle);

  Tiltservo.write(Tiltangle);
  }



// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  //buffer to hold incoming packet,
char  ReplyBuffer[] = "acknowledged";       // a string to send back


void checkcommand()
 {
   // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    IPAddress remote = Udp.remoteIP();
    for (int i = 0; i < 4; i++) {
      Serial.print(remote[i], DEC);
      if (i < 3) {
        Serial.print(".");
      }
    }
    Serial.print(", port ");
    Serial.println(Udp.remotePort());

    // read the packet into packetBufffer
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    Serial.println("Contents:");
    Serial.println(packetBuffer);
    action(packetBuffer);
    // send a reply to the IP address and port that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer);
    Udp.endPacket();
  }
  delay(10);
}


void action(String content){

  String inByte = content.substring(0,1);
  

  //char cm[1];
  //inByte.toCharArray(cm,1);

  //int firstVal = -1;
  //int secondVal = 0;
  
  //for (int i = 0; i < content.length(); i++) {
  //if (content.substring(i, i+1) == ":") {
   // firstVal = content.substring(0, i).toInt();
   // secondVal = content.substring(i+1).toInt();
   // break;
  // }
  //}


  //Serial.println("\nCommand entered:");
  //Serial.print(firstVal);
  //Serial.println("\nvalue entered:");
  //Serial.print(secondVal);
  //Serial.println("\n");
  //if (firstVal >=0)
  //actcommand(firstVal,secondVal);

  actcommand(inByte.toInt(),0);
  }


void actcommand(int cm , int value)
 {
   
   
   switch(cm){
    case 0:Pandrive(1) ; break;
    case 1:Pandrive(-1) ; break;
    case 2:Tiltdrive(1); break;
    case 3:Tiltdrive(-1); break;
    case 4:readRawMPU(); populateJSON(); makePOST(); break; 
    default: break;
   }

 }














void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);

  Serial.println("\nInitiating the  WiFi....\n");
  initWiFi();

  Serial.println("\nInitiating  MPU6050....\n");
  initI2C();
  initMPU();
  checkMPU(MPU_ADDR);

  Serial.println("Checking for location settings\n");
  makePOSTlocation();

  Serial.println("\nConfiguring the pan and tilt servo\n");

  Panservo.attach(PanservoPin);
  
  Panservo.write(Panangle);
  Tiltservo.attach(TiltservoPin);
  
  Tiltservo.write(Tiltangle);

  Udp.begin(localPort);
}


void loop() {
  checkcommand();
  
  
}


