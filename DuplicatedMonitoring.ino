#include <SoftwareSerial.h>
#include <MQUnifiedsensor.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Wire.h>

#define RX 12 // TX of esp8266 in connected with Arduino pin 2
#define TX 3 // RX of esp8266 in connected with Arduino pin 3
#define placa "Arduino UNO"
#define Voltage_Resolution 5
#define pin A0 //mq135 input
#define pin1 A1 //dustSensor
#define dhtpin 7 //dhtdigital pin
#define type "MQ-135" //MQ135
#define ADC_Bit_Resolution 10 // For arduino UNO/MEGA/NANO
#define RatioMQ135CleanAir 3.6//RS / R0 = 3.6 ppm  
#define LED 4


String WIFI_SSID = "KUHP";
String WIFI_PASS = "Banyumanik23"; 
String API = "";// fill with your thingspeak api
String HOST = "api.thingspeak.com";
String PORT = "80";
int countTrueCommand;
int countTimeCommand; 
boolean found = false;
byte countArray = 0;
int arr[30];
//Variabel Anemometer
volatile byte half_revolutions;//variabel tipe data byte
unsigned int rpmku;
unsigned long timeold;
int median;
int kalibrasi;
//Variabel dustsensor
float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;
//Variabel dht22
float temp;
float hum;

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial esp8266(RX,TX);
MQUnifiedsensor MQ135(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);
DHT dht(dhtpin, DHT22);

char jenisgas[6][10] = {"CO","Alcohol","CO2","Tolueno","NH4","Aceton"};
float gasA[6] = {605.18, 77.255, 110.47, 44.947, 102.2, 34.668};
float gasB[6] = {-3.937, -3.18, -2.862, -3.445, -2.473, -3.369};
int itemcheck = 0;//0 for carbon monoxide

void setup() {
  
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("starting.. -_-");
  attachInterrupt(digitalPinToInterrupt(2), rpm_fun, RISING); //mengambil sinyal high pada pin 2
  half_revolutions = 0;
  rpmku = 0;
  timeold = 0;
  kalibrasi = 0;
  Serial.begin(9600);
  dht.begin();
  pinMode(LED, OUTPUT);
  MQ135.setRegressionMethod(1); //_PPM =  a*ratio^b
  MQ135.setA(gasA[itemcheck]); MQ135.setB(gasB[itemcheck]); // Configurate the ecuation values to get CO concentration
  MQ135.init();
  MQ135.setR0(12.25);//hasil pengukuran R0 setelah preheating
  Serial.println("  done!.");
  // if(isinf(calcR0)) {Serial.println("Warning: Conection issue founded, R0 is infite (Open circuit detected) please check your wiring and supply"); while(1);}
  // if(calcR0 == 0){Serial.println("Warning: Conection issue founded, R0 is zero (Analog pin with short circuit to ground) please check your wiring and supply"); while(1);}
  // /*****************************  MQ CAlibration ********************************************/ 
  MQ135.serialDebug(false);
  
  esp8266.begin(115200);
  sendCommand("AT",5,"OK");
  sendCommand("AT+CWMODE=1",5,"OK");
  sendCommand("AT+CWJAP=\""+ WIFI_SSID +"\",\""+ WIFI_PASS +"\"",20,"OK");
  lcd.clear();
}
void loop() {

  if(countArray >= 0 && countArray <= 29){
    arr[countArray] = windSpeed();
    delay(500);
    Serial.print("kecepatan: ");
    Serial.println(arr[countArray]);
    countArray++;
  }
  else if(countArray >= 30){
    MQ135.update();
    float co = MQ135.readSensor();

    quickSort(arr, 0, 30 - 1);
    median = (arr[30/2-1] + arr[30/2]) / 2;

    hum = dht.readHumidity();
    temp = dht.readTemperature();
    voMeasured = analogRead(pin1);
    calcVoltage = voMeasured * (5.0 / 1024.0);
    dustDensity = 170 * calcVoltage - 0.1;
    
    delay(5000);
    sendCommand("AT+CIPMUX=1",5,"OK");
    sendCommand("AT+CIPSTART=0,\"TCP\",\""+ HOST +"\","+ PORT,30,"OK");

    String getData="GET /update?api_key="+ API+"&field3="+temp +"&field4="+hum 
    +"&field2="+dustDensity +"&field1="+co +"&field5="+median;
    sendCommand("AT+CIPSEND=0," +String(getData.length()+4),6,">");
    esp8266.println(getData);
    delay(1500);
    countTrueCommand++;

    sendCommand("AT+CIPCLOSE=0",5,"OK");
    lcd.setCursor(0,1);
    lcd.print("CO: ");
    lcd.print(co);
    lcd.setCursor(0,0);
    lcd.print("pm2.5: ");
    lcd.print(median);

    clearArray(arr,30);
    countArray = 0;
    
  }
}
void clearArray(int arr[], int size) {
    for (int i = 0; i < size; ++i) {
        arr[i] = 0; // Atur nilai setiap elemen array ke 0.0
    }
}

void sendCommand(String command, int maxTime, char readReplay[]) {
  Serial.print(countTrueCommand);
  Serial.print(". at command => ");
  Serial.print(command);
  Serial.print(" ");
  while(countTimeCommand < (maxTime*1))
  {
    esp8266.println(command);//at+cipsend
    if(esp8266.find(readReplay))//ok
    {
      found = true;
      break;
    }
    countTimeCommand++;
  } 
  if(found == true)
  {
    Serial.println("OK");
    countTrueCommand++;
    countTimeCommand = 0;
  }
  if(found == false)
  {
    Serial.println("Fail");
    countTrueCommand = 0;
    countTimeCommand = 0;
  } 
  found = false;
 }

int windSpeed(){
  rpmku = 30*1000/(millis() - timeold)*half_revolutions; //mengaktifkan counter millis
  timeold = millis(); //hasil counter dimasukkan ke variabel timeold
  half_revolutions = 0; //reset variabel

  kalibrasi = (rpmku - 150)/109; //rumus kalibrasi

  if((kalibrasi > 590)&&(kalibrasi < 605)){
    kalibrasi = 0;
  }
  return kalibrasi;
}

void swap(int& a, int& b){
  int temp = a;
  a = b;
  b = temp;
}

int partition(int arr[], int low, int high){
  int pivot = arr[high];
  int i = (low - 1);

  for (int j = low; j <= high -1; j++){
    if(arr[j] <= pivot){
      i++;
      swap(arr[i], arr[j]);
    }
    swap(arr[i + 1], arr[high]);
    return (i+1);
  }
}

void quickSort(int arr[], int low, int high){
  if(low < high){
    int pi = partition(arr, low, high);
    quickSort(arr, low, pi - 1);
    quickSort(arr, pi + 1, high);
  }
}

void rpm_fun(){
   half_revolutions++; //counter interupt
}
