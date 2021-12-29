# ESP-Router-PV
## ATMEGA :

Version originale : [https://learn.openenergymonitor.org](https://learn.openenergymonitor.org/pv-diversion/mk2/index)

Version routeur Tignous : [routeur Tignous](https://forum-photovoltaique.fr/viewtopic.php?f=110&t=40512&sid=59bafdce37acdfd6e16334a939181397)
	
Evolutions personnelles :

	- Utilisation du code original (suppression de la voie 2)
	- Intégration de l'annulation du chargement du bucket quand la mesure de production ne couvre pas le minimum de consommation du ballon d'eau chaude
	- Communication toutes les secondes d'une trame sur port série avec les informations de routage
	- Ajout du calcul de l'ouverture de la voie2 wifi (quand la voie 1 ne suffit pas à consommer tout le surplus ou qu'aucune charge n'est branchée dessus)
	
## ESP :

	- Gestion du wifi avec librairie WiFiManager
	- Intégration de la MAJ OTA via AduinoOTA
	- Serveur Web via ESP8266WebServer
	- Port Série Software via SoftwareSerial
	
##	ESP-Router-PV 
	Récupère les trames de l'ATMEGA et met à dispo l'information en JSON sur l'api httip://@ip/api/routeur
	{"FID":4187,"EIB":2092.77,"MPC":37.5,"REE":-90.43,"POR":784.76,"SPL":57,"VO2":0}

	FID : Fire Delay / Délais de déclanchement
	EIB : Energy in Bucket / Energie virtuel stockée
	MPC : Min Power Routable / Minimum routable en fonction de la puissance du chauffe eau
	REE : Real Energy / Energie mesurée
	POR : Power Routed / Puissance routée
	SPL : Sample / Nombre de mesure dans une onde
	VO2 : Rate for second channel / ratio d'allumage de la voie 2

	le résultat est aussi visible sur http://@ip
	
## ESP-Router-PV-Client

	Toutes les 10 secondes il fait une requete sur l'API de l'ESP-Router-PV (adresse IP codé en dur dans le code).
	Si la consigne lui est donnée de s'allumer il requete toutes les secondes l'API pour ajuster le plus dynamiquement possible sa puissance.
	Il diffuse lui aussi sur son adresse IP en http ou sur /api/routeur les informations le concernant dont le pourcentage d'ouverture
	Il utilise la librairie RBDdimmer pour piloter un dimmer "Robotdyn" (en version 16A pour assurer le fonctionnement d'une source de puissance type chauffage) 
