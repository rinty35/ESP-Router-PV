#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <SoftwareSerial.h>
#include "ESP8266WiFi.h"
#include <WiFiManager.h>
#include <ESP8266WebServer.h>



ESP8266WebServer server(80); //server web
SoftwareSerial mySerial;
WiFiManager wifiManager;
String command;


void setup() {
  Serial.begin(9600); //Permet la communication en serial
  mySerial.begin(2400, SWSERIAL_8N1, D1, D2, false, 256);
  Serial.println("Port série initialisé");

  // put your setup code here, to run once:
  //Création du point d'accès
  wifiManager.setConnectTimeout(60);
  wifiManager.setTimeout(60);
  wifiManager.autoConnect("RouteurPV_connecte");
  if (WiFi.status()!= WL_CONNECTED){
    ESP.reset();
  }
  Serial.println("Connection au WIFI OK");
  Serial.println("Démarrage du serveur web");
  server.on("/", result);
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
  
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  ArduinoOTA.handle();
  delay (10);
}

void result () {
  Serial.println ("Appel de la page");
  //server.setContentLength(CONTENT_LENGTH_UNKNOWN);
   String msgweb;
  int i = 0;

    if(mySerial.available()){
      command = mySerial.readStringUntil('\n');
      Serial.println("Trame : " + command);
      String CRC = command.substring(command.indexOf('$')+1);
      Serial.println ("CRC emetteur : " +   CRC);
      command = command.substring(0,command.indexOf('$'));
      Serial.println("Data : " + command);
      Serial.println("CRC Controle : " + (String)CRC8(command));
      msgweb ="";
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
          msgweb += "FireDelay : " + message.substring(message.indexOf('>')+1) + " ";

        }else if (param=="EIB"){
          msgweb += "Energy In Bucket : " + message.substring(message.indexOf('>')+1) + " ";
        }else if (param=="MPC"){
          msgweb += "Min Power Routable : " + message.substring(message.indexOf('>')+1 ) + " ";
        }else if (param=="REE"){
          msgweb += "Real Energy : " + message.substring(message.indexOf('>')+1) + " ";
        }else if (param=="POR"){
          msgweb += "Power Routed : " + message.substring(message.indexOf('>')+1) + " ";
        }else if (param=="SPL"){
          msgweb += "Samples : " + message.substring(message.indexOf('>')+1)  + '\n';
          i++;
        }
        commaIndex = secondcommaIndex + 1;
        secondcommaIndex = command.indexOf(';',commaIndex);
      }
    } else {
      msgweb ="Time out";
    }
  } else {
    msgweb = "Serial non disponible";
  }
  server.send(200, "text/plain", msgweb);
  Serial.println("Fin de l'appel"); 
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
