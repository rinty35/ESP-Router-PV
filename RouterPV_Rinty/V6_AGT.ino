//  V6 cours VERSION FRANCAISE SIMPLIFIEE UNIQUEMENT EN COMMANDE DE PHASE. du 27 / 06 /19 


 /* Pour dériver du surplus solaire vers un chauffe eau utilisant un relai statique non passage à zero.
  
 PRINCIPE =Calculer une cinquantaine de fois par période la puissance instantannée
 au noend de raccordement;moyenner sur le cycle puis en déduire le paquet d'énergie à ajouter ou soustraire
 au "bucket".
  A l'intérieur d'une fourchette de 100 à 1000 joules ,un triac est activé plus ou moins tard
 dérivant vers une charge extérieure la quantité exacte d'énergie de façon à importer ou exporter un minimum.
 Les échantillons doivent être décales (offset)et filtrés pour être numérisés par l'Atmel

 
 */
 
#include <avr/wdt.h>
#define POSITIVE 1
#define NEGATIVE 0
#define ON 1  // commande positive du relai statique ou triac
#define OFF 0
#define version "Rinty35"
#define Fireimediatly 0
 

#define CdeCh1 2// pin4 micro cde SSR1
#define CdeCh2 4// 8 pin 14 micro cde SSR2 (ou 4 pin 6)
#define voltageSensorPin A3
#define currentSensorPin A5




long cycleCount = 0;
long cptperiodes = 0;
int  samplesDuringThisMainsCycle = 0;
byte nextStateOfTriac;
float cyclesPerSecond = 50; // flottant pour plus de précision
float halfsin = 1000000 / cyclesPerSecond / 2; //temps pour une 1/2 onde
byte polarityNow;
boolean beyondStartUpPhase = false;
float energyInBucket = 0;                                              

int sampleV,sampleI;   // Les valeurs de tension et courant sont des entiers dans l'espace 0 ...1023 
int lastSampleV;     // valeur à la boucle précédente.         
float lastFilteredV,filteredV;  //  tension après filtrage pour retirer la composante continue
float prevDCoffset;          // <<--- pour filtre passe bas
float DCoffset;              // <<--- idem 
float cumVdeltasThisCycle;   // <<--- idem 
float sampleVminusDC;         // <<--- idem
float sampleIminusDC;         // <<--- idem
float lastSampleVminusDC;     // <<--- idem
float sumP = 0;   //  Somme cumulée des puissances à l'intérieur d'un cycle.
// realpower est la puissance moyenne à l'intérieur d'une période

float PHASECAL; //correction de phase inutilisé = 1
float POWERCAL;  // pour conversion des valeur brutes V et I en joules.  
float VOLTAGECAL; // Pour determiner la tension mini de déclenchement du triac.                  // the trigger device can be safely armed

boolean firstLoopOfHalfCycle;
boolean phaseAngleTriggerActivated;
unsigned long timeAtStartOfHalfCycleInMicros; // = To machine à chaque début de 1/2 periode 
unsigned long firingDelayInMicros; // firing delay 
unsigned long timeNowInMicros;
float  phaseShiftedVminusDC;
float instP;
float realPower;
float realEnergy;
//++++++++ PARAMETRES A MODIFIER SUIVANT INSTALL++++++
#define CE 2000   // puissance Chauffe eau W 
#define capacityOfEnergyBucket 1000 // AU LIEU DE 3600 (900 capacité des panneaux 1800KVC à remplir le bucket en 1/2s)
float safetyMargin_watts = 30;  // <<<------ increase for more export
//+++++++++++++++++++++++++++++++++++++++++++++++++
int voie2 = 0;
#define minpowerbucketforfire  ((8500/halfsin)-1)*-capacityOfEnergyBucket  //calcul du seuil de bucket pour lequel le code pourra déclancher un tire sans générer de parasite seuil établi à 8500microseconds
#define powerforfireimediatly  ((0/halfsin)-1)*-capacityOfEnergyBucket
#define Minpowerroutable CE/cyclesPerSecond *(minpowerbucketforfire/capacityOfEnergyBucket)

float sumenergyInBucket = 0;
float sumrealPower = 0;
unsigned long sumfiringDelayInMicros = 0;


void setup()
{ 
  
   wdt_enable(WDTO_8S);
  
  Serial.begin(9600); // pour tests
  pinMode(CdeCh1, OUTPUT); 
  pinMode(CdeCh2, OUTPUT); 
  digitalWrite(CdeCh1, OFF); 
  digitalWrite(CdeCh2, OFF); 

  Serial.print("La version routeur : ");
  Serial.println(version);
  


  // ++++++++++++++++++++++++++++++++++++++++++++++++++++++
  
    
  POWERCAL = 0.18 ;   //tigous : 0.18 org 0.12 à ajuster pour faire coincider la puissance vraie avec le realPwer.
                    // en utilisant le traceur serie.
                    // NON CRITIQUE car les valeur absolues s'annulent en phase de régulation         // retort and export flows are balanced.  
                    
  VOLTAGECAL = (float)679 / 471; // En volts par pas d'ADC.
                        // Utilisé pour déterminer quand la tension secteur est suffisante pour
                        // exciter le triac. noter les valeurs min et max de la mesure de tension
                        // par exemple 678.8 pic pic 
                        // La dynamique étant de 471 signifie une sous estimation de la tension 
                        // de 471/679.  VOLTAGECAL doit donc être multiplié par l'inverse
                        // de 679/471 soit 1.44
  PHASECAL = 1;      // NON CRITIQUE
     
}  


void loop() // Une paire tension / courant est mesurée à chaque boucle (environ 54 par période)

  {
 
	wdt_reset();
	samplesDuringThisMainsCycle++;  // incrément du nombre d'échantillons par période secteur pour calcul de puissance.

  // Sauvegarde des échantillons précédents
	lastSampleV=sampleV;            // pour le filtre passe haut 
	lastFilteredV = filteredV;      // afin d'identifier le début de chaque période
	lastSampleVminusDC = sampleVminusDC;  // for phasecal calculation 
 
  // Acquisition d'une nouvelle paire d'chantillons bruts. temps total :380µS
	sampleI = analogRead(currentSensorPin);   
	sampleV = analogRead(voltageSensorPin); 
 
  // Soustraction de la composante continue déterminée par le filtre passe bas
	sampleVminusDC = sampleV - DCoffset; 
	sampleIminusDC = sampleI - DCoffset;

  // Un filtre passe haut est utilisé pour déterminer le début de cycle.  
	filteredV = 0.996*(lastFilteredV+sampleV-lastSampleV);  // Sinus tension reconstituée 

  // Détection de la polarité de l'alternance
	byte polarityOfLastReading = polarityNow;
  
	if(filteredV >= 0) 
		polarityNow = POSITIVE; 
	else 
		polarityNow = NEGATIVE;

	if (polarityNow == POSITIVE)
	{
		if (polarityOfLastReading != POSITIVE)
		{
		// C'est le départ d'une nouvelle sinus positive juste après le passage à zéro
		  
			firstLoopOfHalfCycle = true;
		  
		// mise à jour du filtre passe bas pour la soustraction de la composante continue
			prevDCoffset = DCoffset;
			DCoffset = prevDCoffset + (0.015 * cumVdeltasThisCycle); 

		//  Calcul la puissance réelle de toutes les mesures echantillonnées durant 
		//  le cycle précedent, et determination du gain (ou la perte) en energie.
			realPower = POWERCAL * sumP / (float)samplesDuringThisMainsCycle;
			realEnergy = realPower / cyclesPerSecond;
      cycleCount++; // incrément Nb de périodes  


  
      

		  //----------------------------------------------------------------
		  // WARNING - Serial statements can interfere with time-critical code, but
		  //           they can be really useful for calibration trials!
		  // ----------------------------------------------------------------        
		  


			if (beyondStartUpPhase == true)// > 2 secondes
			  {  
		    // Supposant que les filtres ont eu suffisamment de temps de se stabiliser    
		    // ajout de cette energie d'une période à l'énergie du reservoir.
        /*   min conso EC sur une sinus = puissance du CE * 3600 (pour avoir des Joules) / 3600 (pour avoir Nb Joules par seconde) / 100 (par le double de la fréquence du 220V pour avoir J par demie prédiode)
        *    pourcentage mini d'alimentation = 1-100/capacityOfEnergyBucket
        *    energie min évacuée = pourcentage mini d'alimentation x min conso EC sur une sinus
          if (energyInBucket < 100 && realEnergy < CE/100*(100/(float)capacityOfEnergyBucket) && realEnergy >0) { 
          //on est à l'équilibre prod-conso et le sceau n'a que très peu d'énergie. On ne route pas et on remet le sceauà 0
            energyInBucket = 0;
          } else {
            energyInBucket += realEnergy;  
          }
        */
        //if (energyInBucket < minpowerbucketforfire && realEnergy < Minpowerroutable && realEnergy >0) { //si le sceau est sous le seuil de routage et que l'énergie mesurée est inférieure au min soutirable par la charge mais positive on remet le sceau à 0
        //  energyInBucket = 0;
        //} else {
          energyInBucket += realEnergy;  
        //}
		 
		    // Reduction d'énergie dans le reservoir d'une quantité "safety margin"
		    // Ceci permet au système un décalage pour plus d'injection ou + de soutirage 
	      //energyInBucket -= (safetyMargin_watts / cyclesPerSecond)*(0.2-energyInBucket/(float)capacityOfEnergyBucket); // Soustraction de la safety margin en fonction du % de remplissage du sceau Plein on ne retire rien, quasi vide on retire la safetymargin
        if (energyInBucket<(float)capacityOfEnergyBucket*0.35){
          energyInBucket -= ((safetyMargin_watts) / cyclesPerSecond);
        }else {
          energyInBucket += (safetyMargin_watts) / (cyclesPerSecond);
        }
    /*
        if (energyInBucket<(float)capacityOfEnergyBucket/2){
          energyInBucket += (((-2*safetyMargin_watts*energyInBucket)/((float)capacityOfEnergyBucket/2-minpowerbucketforfire)+(safetyMargin_watts-minpowerbucketforfire*(-2*safetyMargin_watts)/((float)capacityOfEnergyBucket/2-minpowerbucketforfire))) / cyclesPerSecond);
        }else {
          energyInBucket += (safetyMargin_watts / cyclesPerSecond);
        }
      */  
        
		    // Limites dans la fourchette 0 ...1000 joules ;ne peut être négatif le 11 / 2 / 18
				if (energyInBucket > capacityOfEnergyBucket)
					energyInBucket = capacityOfEnergyBucket;  
				if (energyInBucket < 0)
					energyInBucket = 0; 

 
             
			} else { // Après un reset attendre 100 périodes (2 secondes) le temps que la composante continue soit éliminée par le filtre   
			  if(cycleCount > 100) // deux secondes
				  beyondStartUpPhase = true; //croisière
			}

		  // ********************************************************
		  // start of section to support phase-angle control of triac 
		  // determines the correct firing delay for a direct-acting trigger

		  // never fire if energy level is below lower threshold (zero power)  
			if (energyInBucket < minpowerbucketforfire) { firingDelayInMicros = halfsin;} //remplacement 99999 par 10000 1S /50Hz /2alternances en microssecondes
			else  
		  // fire immediately if energy level is above upper threshold (full power)
			//if (energyInBucket >= capacityOfEnergyBucket) { firingDelayInMicros = 0;} 
			if (energyInBucket>=powerforfireimediatly){ firingDelayInMicros = Fireimediatly;}// si le calcul du délais est inférieur au délais de démarrage immédiat pour bon armement du triac alors on si positionne sur ce délais
			else
		  // determine the appropriate firing point for the bucket's level
		  // by using either of the following algorithms
			{ 
		    // simple algorithm (with non-linear power response across the energy range)
		    // firingDelayInMicros = 10 * (capacityOfEnergyBucket - energyInBucket); 
//       if (firingDelayInMicros=halfsin) { // si premier tire d'une série on tire au minimum
//        firingDelayInMicros = halfsin;
//       } else {
        firingDelayInMicros = round (halfsin * (1-(energyInBucket/capacityOfEnergyBucket))); //(10000 microsecond durée d'un demie cycle * (1- % de charge du bucket))
//       }
  		  // complex algorithm which reflects the non-linear nature of phase-angle control.  
			  //	firingDelayInMicros = (asin((-1 * (energyInBucket - 1800) / 500)) + (PI/2)) * (10000/PI);
			
			  // Suppress firing at low energy levels to avoid complications with 
			  // logic near the end of each half-cycle of the mains.  
			  // This cut-off affects approximately the bottom 5% of the energy range.
				
				
//				if (firingDelayInMicros > 8500) { firingDelayInMicros = halfsin;} // never fire
//        else if (firingDelayInMicros < Fireimediatly) { firingDelayInMicros = Fireimediatly;} //Déclancment imédiat Fireimediatly pour avoir une meilleure tension d'armement du triac 
			}
      sumenergyInBucket += energyInBucket;
      sumrealPower += realPower;
      sumfiringDelayInMicros += firingDelayInMicros;
      if((cycleCount >= 150)) { // display once per second
        if (sumenergyInBucket/50 == capacityOfEnergyBucket){
          if (voie2 == 0) {
            voie2=20;
          } else if (voie2<100) {
            voie2+=1;
          }
        } else if (voie2>0 && sumenergyInBucket/50 > capacityOfEnergyBucket*0.9 ) {
          voie2-=1;
        }else if (voie2>0 && sumenergyInBucket/50 > capacityOfEnergyBucket*0.5 ) {
          voie2-=5;
        }else if (voie2>0 && voie2>10 ) {
          voie2-=10;
        }else if (voie2>0 ) {
          voie2-=1;
        }
        String message = "EIB>" + (String)(sumenergyInBucket/50); 
        message += ";REE>" + (String)(sumrealPower/50); // Power mesured
        message +=";MPC>" + (String) (Minpowerroutable*cyclesPerSecond); //Min Power Charge en W
        message += ";POR>" + (String)((1-(sumfiringDelayInMicros/50/halfsin))*100); //Energy routed en %
        message += ";FID>" + (String)(sumfiringDelayInMicros/50); //Firedelay
        message += ";SPL>" + (String)(samplesDuringThisMainsCycle);
        message += ";VO2>" + (String)(voie2);
        message += ";";
        message += "$"+ (String)CRC8(message) ;
        Serial.println(message);
        cycleCount = 100;
        sumenergyInBucket=0;
        sumrealPower=0;
        sumfiringDelayInMicros=0;
        //maquette
        //delay (1000);
      } 				
		  // end of section to support phase-angle control of triac
		  //*******************************************************

       
			sumP = 0;
			samplesDuringThisMainsCycle = 0;
			cumVdeltasThisCycle = 0;
		 // ********************************************************
		} 
	}  // end of processing that is specific to positive Vsamples
    else
	{
		if (polarityOfLastReading != NEGATIVE)
		{
				firstLoopOfHalfCycle = true;
		}
	} // end of processing that is specific to positive Vsamples

  
  // Processing for ALL Vsamples, both positive and negative
  //------------------------------------------------------------
   
  // ********************************************************
  // start of section to support phase-angle control of triac 
  // controls the signal for firing the direct-acting trigger. 
  
	timeNowInMicros = micros(); // occurs every loop, for consistent timing
  
	if (firstLoopOfHalfCycle == true)
	{
		timeAtStartOfHalfCycleInMicros = timeNowInMicros;
		firstLoopOfHalfCycle = false;
		phaseAngleTriggerActivated = false;
    // Unless dumping full power, release the trigger on the first loop in each 
    // half cycle.  Ensures that trigger can't get stuck 'on'.  
		if (firingDelayInMicros > Fireimediatly) {     
			digitalWrite(CdeCh1, OFF);
		}
	}
  
	if (phaseAngleTriggerActivated == true)
	{
    // Unless dumping full power, release the trigger on all loops in this 
    // half cycle after the one during which the trigger was set.  
		if (firingDelayInMicros > Fireimediatly) {     
			digitalWrite(CdeCh1, OFF);
		}
	}
	else
	{  
		if (timeNowInMicros >= (timeAtStartOfHalfCycleInMicros + firingDelayInMicros))
		{
			digitalWrite(CdeCh1, ON); 
			phaseAngleTriggerActivated = true;

		}
	}

/*  if (phaseAngleTriggerActivated == false && timeNowInMicros >= (timeAtStartOfHalfCycleInMicros + firingDelayInMicros)){
      digitalWrite(CdeCh1, ON); 
      phaseAngleTriggerActivated = true;
  }*/
   
  // end of section to support phase-angle control of triac
  //*******************************************************



  // Apply phase-shift to the voltage waveform to ensure that the system measures a
  // resistive load with a power factor of unity.
	phaseShiftedVminusDC = lastSampleVminusDC + PHASECAL * (sampleVminusDC - lastSampleVminusDC);  
	instP = phaseShiftedVminusDC * sampleIminusDC; // PUISSANCE derniers échantillons V x I filtrés 
	sumP +=instP;     // accumulation à chaque boucle des puissances instantanées dans une période

	cumVdeltasThisCycle += (sampleV - DCoffset); // pour usage avec filtre passe bas

 


} // end of loop()

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
