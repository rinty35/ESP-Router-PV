#include <ArduinoOTA.h>
#include <WiFiUdp.h>

#include "ESP8266WiFi.h"
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <RBDdimmer.h>//librarie pour dimmer
#define outputPin  D0 
#define zerocross  D1 
dimmerLamp dimmer(outputPin, zerocross); //initialase port for dimmer for ESP8266, ESP32, Arduino due boards
int Delaisajustdimmer = 1000;

ESP8266WebServer server(80); //server web

WiFiManager wifiManager;
String msgweb;
StaticJsonDocument<16> filter;

const byte tab_Routeur_size = 48;
StaticJsonDocument<tab_Routeur_size> Tab_Routeur;
int VO2 = 0;

void setup() {
  WiFi.mode(WIFI_STA);
  Serial.begin(9600); //Permet la communication en serial

  dimmer.begin(NORMAL_MODE, ON); //dimmer initialisation: name.begin(MODE, STATE) 
  dimmer.setPower(0);
  dimmer.setState(OFF);

  // put your setup code here, to run once:
  //Création du point d'accès
  wifiManager.setConnectTimeout(60);
  wifiManager.setTimeout(60);
  wifiManager.autoConnect("RouteurPV_connecte_client");
  if (WiFi.status()!= WL_CONNECTED){
    ESP.reset();
  }
  Serial.println("Connection au WIFI OK");
  Serial.println("Démarrage du serveur web");
  //server.on("/", result);
  //server.on("/api/routeur", sendRouteur);
  //server.begin();

  ArduinoOTA.setHostname("ESPRooterPVClient");
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
/*  Tab_Routeur["FID"]=0;
  Tab_Routeur["EIB"]=0;
  Tab_Routeur["MPC"]=0;
  Tab_Routeur["REE"]=0;
  Tab_Routeur["POR"]=0;
  Tab_Routeur["SPL"]=0;*/
  filter["VO2"] = true;
  Tab_Routeur["VO2"]=0;
  
}

void loop() {
  static unsigned long Millisajustdimmer = 0;
  // put your main code here, to run repeatedly:
  //server.handleClient();
  ArduinoOTA.handle();

  WiFiClient client;
  HTTPClient http;
  if (millis()-Millisajustdimmer>=Delaisajustdimmer){
    Millisajustdimmer = millis();
    if (http.begin(client, "http://192.168.1.221/api/routeur")) {  // connexion sur l'esp du routeur tignous
      int httpCode = http.GET(); // récupération des infos du routerur tignous
      if (httpCode > 0) { // httpCode will be negative on error
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          msgweb = http.getString(); // récupération du résultat
          Serial.println("message : " + msgweb);
          DeserializationError error = deserializeJson(Tab_Routeur, msgweb, DeserializationOption::Filter(filter));
          if (error) {
            Serial.println("Impossible de lire le JSON");
            Serial.println(error.c_str());
          }else {
            VO2 = Tab_Routeur["VO2"];
            Serial.println("VO2 : "+ (String) VO2);
            if (VO2 > 0) {
              dimmer.setPower(VO2);
              Serial.println("% à router : " + dimmer.getPower() );
              dimmer.setState(ON);
              Delaisajustdimmer = 1000;
            }else{
              Serial.println("router Off");
              dimmer.setState(OFF);
              Delaisajustdimmer = 10000;
            }
          }
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
    }
    http.end();
  }
}

/*void result () {
  Serial.println ("Appel de la page");
  //server.setContentLength(CONTENT_LENGTH_UNKNOWN);
   

  server.send(200, "text/html","<html lang=\"fr\"><head><meta http-equiv=\"refresh\" content=\"1\"></head><body>" + msgweb + "</body></html>");
  Serial.println("Fin de l'appel"); 
}

void sendRouteur () {
  Serial.println ("Appel de l'API");
  server.send(200, "application/json",msgweb);
  Serial.println("Fin de l'API"); 
}*/
