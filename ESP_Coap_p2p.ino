#include <FirebaseESP8266.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include <Queue.h>

//Definition of the WiFi SSID, password and configurations of cloud storage
#ifndef STASSID
#define STASSID "*******"
#define STAPSK  "********"
#define FIREBASE_HOST "https://*******.firebaseio.com/"
#define FIREBASE_AUTH "*************"
#endif
const char* ssid     = STASSID;
const char* password = STAPSK;
IPAddress my_ip(192, 168, 43, 18);
IPAddress gateway(192, 168, 43, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);
//Define FirebaseESP8266 data object
FirebaseData firebaseData;
String FPath = "/Coap";
//Definition of coap classes
WiFiUDP udp;
Coap coap(udp);
const int my_port = 5865;
//Definition of IP and port of the second CoAP server
const int peer_port = 11111;
IPAddress coap_peer(192, 168, 43, 14);
//Timeouts for automatic sensor readings (deactivated by default)
unsigned long timer=0; 
boolean timer_active=false;
int action=-1;
//Class used to define a queue of requests on remote data
class request{
  public:
  IPAddress ip;
  int port; 
  uint8_t messageid;
};
DataQueue<request> requestsQueue(10);//Queue of pending requests up to 10


//Callbacks
void callback_wn(CoapPacket &packet, IPAddress ip, int port){//Callback for /.well-known/core resources
  coap.sendResponse(ip, port, packet.messageid,"loop_update, dht22, lm393, hcsr04, time"); //Gives the resources as a responce to the client (</time> is the default resource on the remote server)
}

void callback_loop_update(CoapPacket &packet, IPAddress ip, int port){//Callback for /loop_update resource
  if(packet.code==3){//PUT request
    // extract payload
    char p[packet.payloadlen + 1];
    memcpy(p, packet.payload, packet.payloadlen);
    p[packet.payloadlen] = '\0';
    String message(p);
    
    if (message.equals("0")){
      timer_active=false;
    }else if(message.equals("1")){
      timer_active=true;
    }
    
  }
  if(timer_active){
    coap.sendResponse(ip, port,packet.messageid, "ON");
  }else{
    coap.sendResponse(ip, port,packet.messageid, "OFF");
  }

}

void callback_dht22(CoapPacket &packet, IPAddress ip, int port) {//0 request to arduino for the DHT22 data
  serialFlush();
  Serial.println("0");
  delay(200);
  if(Serial.available()){
    String result=Serial.readStringUntil('\n');
    String s=result.substring(1);
    char data[s.length() + 1];
    s.toCharArray(data, s.length());
    data[s.length()] = '\0';
    if(result.charAt(0)=='0')
      coap.sendResponse(ip, port,packet.messageid, data);
    else
      coap.sendResponse(ip, port, packet.messageid,"Error");
  }else{
    coap.sendResponse(ip, port, packet.messageid,"Error");
  }
}

void callback_lm393(CoapPacket &packet, IPAddress ip, int port) {//1 request to arduino lm393
  serialFlush();
  Serial.println("1");
  delay(200);
  if(Serial.available()){
    String result=Serial.readStringUntil('\n');
    String s=result.substring(1);
    char data[s.length() + 1];
    s.toCharArray(data, s.length());
    data[s.length()] = '\0';
    if(result.charAt(0)=='1')
      coap.sendResponse(ip, port,packet.messageid, data);
    else
      coap.sendResponse(ip, port, packet.messageid,"Error");
  }else{
    coap.sendResponse(ip, port, packet.messageid, "Error");
  }
}

void callback_hcsr04(CoapPacket &packet, IPAddress ip, int port) {//2 request to arduino hcsr04
  serialFlush();
  Serial.println("2");
  delay(200);
  if(Serial.available()){
    String result=Serial.readStringUntil('\n');
    String s=result.substring(1);
    char data[s.length() + 1];
    s.toCharArray(data, s.length());
    data[s.length()] = '\0';
    if(result.charAt(0)=='2')
      coap.sendResponse(ip, port,packet.messageid, data);
    else
      coap.sendResponse(ip, port, packet.messageid,"Error");
  }else{
    coap.sendResponse(ip, port, packet.messageid, "Error");
  }
}

void callback_time(CoapPacket &packet, IPAddress ip, int port) {
    request req;
    req.ip=ip;
    req.port=port;
    req.messageid=packet.messageid;
    requestsQueue.enqueue(req);
    coap.get(coap_peer, peer_port, "time");
}

void callback_response(CoapPacket &packet, IPAddress ip, int port) { //In response of a request
  //Retrieves payload
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = '\0';
  String message(p);
  while(!requestsQueue.isEmpty()){
    request r=requestsQueue.dequeue();
    coap.sendResponse(r.ip, r.port, r.messageid,p);
  }
  if(timer_active){//send to Firebase
    Firebase.setString(firebaseData,FPath+"/time/data", message);
  }
}

void loop_update(int i){//Function called in loop that when active sends calls to the other CoAP servers and data to Firebase
  if(i>=0){
    serialFlush();
    Serial.println(i);
    delay(200);
    if(Serial.available()){
      String result=Serial.readStringUntil('\n');
      String s=result.substring(1);
      char data[s.length() + 1];
      s.toCharArray(data, s.length());
      data[s.length()] = '\0';
      if(i==0){
        if(result.charAt(0)=='0')
          Firebase.setString(firebaseData,FPath+"/dht22/data/string", String(data));
        serialFlush();
        Serial.println(3);//Only temperature
        delay(200);
        if(Serial.available()){
          result=Serial.readStringUntil('\n');
          s=result.substring(1);
          data[s.length() + 1];
          s.toCharArray(data, s.length());
          data[s.length()] = '\0';
          if(result.charAt(0)=='3')
            Firebase.setString(firebaseData, FPath+"/dht22/data/temp", String(data));
          serialFlush();
          Serial.println(4);//Only humidity
          delay(200);
          if(Serial.available()){
            result=Serial.readStringUntil('\n');
            s=result.substring(1);
            data[s.length() + 1];
            s.toCharArray(data, s.length());
            data[s.length()] = '\0';
            if(result.charAt(0)=='4')
              Firebase.setString(firebaseData, FPath+"/dht22/data/hum", String(data));
          }
        }
      }else if(i==1){
        if(result.charAt(0)=='1')
          Firebase.setString(firebaseData,FPath+"/lm393/data", String(data));
      }else{
        if(result.charAt(0)=='2')
          Firebase.setString(firebaseData, FPath+"/hcsr04/data/string", String(data));
        serialFlush();
        Serial.println(5);//Only distance
        delay(200);
        if(Serial.available()){
          result=Serial.readStringUntil('\n');
          s=result.substring(1);
          data[s.length() + 1];
          s.toCharArray(data, s.length());
          data[s.length()] = '\0';
          if(result.charAt(0)=='5')
            Firebase.setString(firebaseData, FPath+"/hcsr04/data/dist", String(data));
        }
      }
    }
  }else{
    coap.get(coap_peer, peer_port, "time");
  }
}

void serialFlush(){
  while(Serial.available() > 0) {
    String s=Serial.readStringUntil('\n');
  }
} 

void setup() {
  //Serial is used to communicate wih Arduino and retrieve data about its connected sensors
  Serial.begin(115200);
  // We start by connecting to the WiFi network with a static IP
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(false);   // Not connecting by its own
  WiFi.disconnect();  //Prevent connecting to wifi based on previous configuration
  WiFi.config(my_ip, gateway, subnet, dns);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  // Init Firebase data
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.setwriteSizeLimit(firebaseData, "tiny");
  // Init of coap callbacks for the server
  coap.server(callback_wn, ".well-known/core");
  coap.server(callback_loop_update, "loop_update");         
  coap.server(callback_time, "time");
  coap.server(callback_hcsr04, "hcsr04");
  coap.server(callback_dht22, "dht22");
  coap.server(callback_lm393,"lm393");
  coap.response(callback_response);
  coap.start(my_port);

}

void loop() {
  if(WiFi.status() != WL_CONNECTED){//If disconnected from Wifi, retry to connect again
    WiFi.disconnect();
    WiFi.config(my_ip, gateway, subnet, dns);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
  }
  coap.loop();//Coap checks
  if(timer_active){//Update function
    if(millis()>timer){
      timer=millis()+300;//Timer
      loop_update(action);
      action+=1;
      if(action>2){
        action=-1;
      }
    }
  }
  
}
