#include <SoftwareSerial.h>
#include <SimpleDHT.h>

#define pinRXesp 10
#define pinTXesp 11
#define pinDHT22 2
#define trigHCPin 7
#define echoHCPin 6
#define ldrPin 4

//variables HC-SR04
long duration;
int distance;
//variables DHT-22
float temperature = 0;
float humidity = 0;
//variable LDR
boolean light;

SimpleDHT22 dht22(pinDHT22);

SoftwareSerial Esp_serial(pinTXesp,pinRXesp); // TX,RX

void getDHT22Data(String type){//0 string, 3 temp, 4 hum
  float temperature_l = 0;
  float humidity_l = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht22.read2(&temperature_l, &humidity_l, NULL)) == SimpleDHTErrSuccess) {
      temperature=temperature_l;
      humidity=humidity_l;
  }
  if(type.charAt(0)=='0'){
    Esp_serial.print("0");Esp_serial.print((float)temperature); Esp_serial.print(" *C, ");Esp_serial.print((float)humidity); Esp_serial.println(" RH%");
  }else if(type.charAt(0)=='3'){
    Esp_serial.print("3");Esp_serial.println((float)temperature);
  }else if(type.charAt(0)=='4'){
    Esp_serial.print("4");Esp_serial.println((float)humidity);
  }
}

void getlLDR393Data(){
  light=digitalRead(ldrPin);
  if(light==HIGH){
    Esp_serial.print("1");Esp_serial.println("LOW Light");
  }else{
    Esp_serial.print("1");Esp_serial.println("HIGH Light");
  }
}

void getHCS04Data(String type){
  // Clears the trigPin
  digitalWrite(trigHCPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigHCPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigHCPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoHCPin, HIGH);
  // Calculating the distance
  distance = duration*0.034/2;
  if(type.charAt(0)=='2'){
    Esp_serial.print("2");Esp_serial.print("Distance: ");Esp_serial.print(distance); Esp_serial.println(" cm");  
  }else if(type.charAt(0)=='5'){
    Esp_serial.print("5");Esp_serial.println(distance);
  }
}

void setup() {
  pinMode(trigHCPin, OUTPUT);
  pinMode(echoHCPin, INPUT);
  pinMode(ldrPin, INPUT);
  Esp_serial.begin(115200);
  Serial.begin(115200);
}

void loop() {
    if(Esp_serial.available()){
      String s=Esp_serial.readStringUntil('\n');
      Serial.println(s);
      if(s.charAt(0)=='0' || s.charAt(0)=='3' || s.charAt(0)=='4'){
        getDHT22Data(s);
      } else if(s.charAt(0)=='1'){
        getlLDR393Data();
      } else if(s.charAt(0)=='2' || s.charAt(0)=='5'){
        getHCS04Data(s);
      }
    }
}
