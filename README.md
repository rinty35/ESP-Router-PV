# ESP-Router-PV
## ATMEGA :

Version originale : [https://learn.openenergymonitor.org](https://learn.openenergymonitor.org/pv-diversion/mk2/index)

Version routeur Tignous : [routeur Tignous](https://forum-photovoltaique.fr/viewtopic.php?f=110&t=40512&sid=59bafdce37acdfd6e16334a939181397)
	
Evolutions personnelles :

	- Utilisation du code original (suppression de la voie 2)
	- Intégration de l'annulation du chargement du bucket quand la mesure de production ne couvre pas le minimum de consommation du ballon d'eau chaude
	- Communication toutes les secondes d'une trame sur port série avec les informations de routage
	
## ESP :

	- Gestion du wifi avec librairie WiFiManager
	- Intégration de la MAJ OTA via AduinoOTA
	- Serveur Web via ESP8266WebServer
	- Port Série Software via SoftwareSerial
	
	- ESP-Router-PV Récupère les trames de l'ATMEGA et met à dispo l'information en JSON sur l'api httip://@ip/api/routeur le résultat est aussi visible sur http://@ip/api/routeur
	- ESP-Router-PV-Client lit les informations de ESP-Router-PV et pilote un variateur 230v en PWM