#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <SoftwareSerial.h>
#include "ESP8266WiFi.h"
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

float CE = 2000; //puissance en W du chauffe eau

ESP8266WebServer server(80); //server web
SoftwareSerial mySerial;
WiFiManager wifiManager;
String msgweb;
const byte tab_Routeur_size = 128;
StaticJsonDocument<tab_Routeur_size> Tab_Routeur;

void setup() {
  WiFi.mode(WIFI_STA);
  Serial.begin(9600); //Permet la communication en serial

  // put your setup code here, to run once:
  //Création du point d'accès
  wifiManager.setConnectTimeout(60);
  wifiManager.setTimeout(60);
  wifiManager.autoConnect("RouteurPV_connecte");
  if (WiFi.status()!= WL_CONNECTED){
    ESP.reset();
  }
  mySerial.begin(9600, SWSERIAL_8N1, D1, D1, false, 256);
  Serial.println("Port série initialisé");
  Serial.println("Connection au WIFI OK");
  Serial.println("Démarrage du serveur web");
  server.on("/", result);
  server.on("/api/routeur", sendRouteur);
  server.begin();

  ArduinoOTA.setHostname("ESPRooterPV");
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Tab_Routeur["FID"]=0;
  Tab_Routeur["EIB"]=0;
  Tab_Routeur["MPC"]=0;
  Tab_Routeur["REE"]=0;
  Tab_Routeur["POR"]=0;
  Tab_Routeur["SPL"]=0;
  Tab_Routeur["VO2"]=0;
  
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  ArduinoOTA.handle();
  //int i = 0;
  String command = "";

  String CRC = "999";
 // while(CRC.toInt()!=CRC8(command) && i< 100){
    if (mySerial.available()>0){
      command = mySerial.readStringUntil('\n');
      Serial.println("Trame : " + command);
      CRC = command.substring(command.indexOf('$')+1);
      //Serial.println ("CRC emetteur : " +   CRC);
      command = command.substring(0,command.indexOf('$'));
      //Serial.println("Data : " + command);
      //Serial.println("CRC Controle : " + (String)CRC8(command));
    }
 //   i++; 
 // }
  if (CRC.toInt()==CRC8(command)){

    msgweb="";
    int commaIndex = 0;
    int secondcommaIndex = command.indexOf(';');
    while( secondcommaIndex!= -1){
      String message = command.substring(commaIndex,secondcommaIndex);
      String param = message.substring(0,message.indexOf('>'));
      String value = message.substring(message.indexOf('>')+1);
      if (param=="FID"){
        msgweb += "FireDelay : " + value + "<BR>";
        Tab_Routeur["FID"]=value.toFloat();
      }else if (param=="EIB"){
        msgweb += "Energy In Bucket : " + value  + "<BR>";
        Tab_Routeur["EIB"]=value.toFloat();
      }else if (param=="MPC"){
        msgweb += "Min Power Routable : " + value + "<BR>";
        Tab_Routeur["MPC"]=value.toFloat();
      }else if (param=="REE"){
        msgweb += "Real Energy : " + value+ "<BR>";
        Tab_Routeur["REE"]=value.toFloat();
      }else if (param=="POR"){
        msgweb += "Power Routed en % : " + (String)(value.toFloat()) + "<BR>Power Routed : " + (String)(value.toFloat()*CE/100) + "<BR>";
        if (value.toFloat()<0){
          Tab_Routeur["POR"]=0;
        } else {
         // Tab_Routeur["POR"]=value.toFloat()*CE/100;
         Tab_Routeur["POR"]=value.toFloat();
        }
      }else if (param=="SPL"){
        msgweb += "Samples : " + value+ "<BR>";
        Tab_Routeur["SPL"]=value.toFloat();
      }else if (param=="VO2"){
        msgweb += "Routage voie 2 : " + value+ "<BR>";
        Tab_Routeur["VO2"]=value.toFloat();
      //i++;
      }
      commaIndex = secondcommaIndex + 1;
      secondcommaIndex = command.indexOf(';',commaIndex);
    }
//  }else{
//    msgweb ="Serial - time out";
  }
 // msgweb ="<html lang=\"fr\"><head><meta http-equiv=\"refresh\" content=\"1\"></head><body>"+ msgweb + "</body></html>";
  /*if (mySerial.available()>0){
    msgweb ="";
    command = mySerial.readStringUntil('\n');
    Serial.println("Trame : " + command);
    String CRC = command.substring(command.indexOf('$')+1);
    Serial.println ("CRC emetteur : " +   CRC);
    command = command.substring(0,command.indexOf('$'));
    Serial.println("Data : " + command);
    Serial.println("CRC Controle : " + (String)CRC8(command));
    while(CRC.toInt()!=CRC8(command) && i< 100){ // tant que l'on n'a pas une trame complète et que l'on n'a pas fait 100 tentatives
      command = mySerial.readStringUntil('\n');
      Serial.println("Trame : " + command);
      CRC = command.substring(command.indexOf('$')+1);
      Serial.println ("CRC emetteur : " +   CRC);
      command = command.substring(0,command.indexOf('$'));
      Serial.println("Data : " + command);
      Serial.println("CRC Controle : " + (String)CRC8(command));
      i++; 
    }
    if (i<100){
     int commaIndex = 0;
     int secondcommaIndex = command.indexOf(';');
     while( secondcommaIndex!= -1){
      String message = command.substring(commaIndex,secondcommaIndex);
      String param = message.substring(0,message.indexOf('>'));
      if (param=="FID"){
        msgweb += "FireDelay : " + message.substring(message.indexOf('>')+1)+ "<BR>";
      }else if (param=="EIB"){
        msgweb += "Energy In Bucket : " + message.substring(message.indexOf('>')+1) + "<BR>";
      }else if (param=="MPC"){
        msgweb += "Min Power Routable : " + message.substring(message.indexOf('>')+1) + "<BR>";
      }else if (param=="REE"){
        msgweb += "Real Energy : " + message.substring(message.indexOf('>')+1)+ "<BR>";
      }else if (param=="POR"){
        msgweb += "Power Routed : " + message.substring(message.indexOf('>')+1)+ "<BR>";
      }else if (param=="SPL"){
        msgweb += "Samples : " + message.substring(message.indexOf('>')+1)+ "<BR>";
        i++;
      }
      commaIndex = secondcommaIndex + 1;
      secondcommaIndex = command.indexOf(';',commaIndex);
    }
  } else {
    msgweb ="Time out";
  }
} else {
msgweb = "Serial non disponible<BR><BR>" + msgweb;
}
delay(100);*/
}

void result () {
 // Serial.println ("Appel de la page");
  //server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  Serial.println ("Appel de la page");

  server.send(200, "text/html","<html lang=\"fr\"><head><meta http-equiv=\"refresh\" content=\"1\"></head><body>" + msgweb + "</body></html>");
  //Serial.println("Fin de l'appel"); 
}

void sendRouteur () {
  Serial.println ("Appel de l'API");
  char buffer[tab_Routeur_size];
  serializeJson(Tab_Routeur, buffer);
  server.send(200, "application/json",buffer);
  //Serial.println("Fin de l'API"); 
}

int CRC8(String stringData) {
  int len = stringData.length();
  int i = 0;
  byte crc = 0x00;
  while (len--) {
    byte extract = (byte) stringData.charAt(i++);
    for (byte tempI = 8; tempI; tempI--) {
      byte sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc;
}
