#include <EEPROM.h>
#include <RCSwitch.h>
RCSwitch mySwitch = RCSwitch();
int Micro=A0; // capteur micro
int Crep=A1;  // capteur crépusculaire (lumière)
int potg=A2; // potentiometre vantail gauche
int potd=A3;  // potentiometre vantail droit

int RelaiOg=4;// relai ouvre gauche
int RelaiOd=6;// relai ouvre droit
int RelaiFg=3;// relai ferme gauche
int RelaiFd=5;// relai ferme droite
int ampg=A4; //surintesité gauche
int ampd=A5; //surintensité droite// fonction surintensité

int phare=11; //gyrophare sur poteau
int poteau=7; // pin bouton sur poteau
int test=8; // pin bouton de test
bool valAction=1; // valeur de l'action ouverture=1 /fermeture=0
int encours=0; //numero d'un signal en cours (0=rien 1=klaxon 2=lumiere etc...)
int autor=0; // autorisation des permissions de declenchement (dans Sana[8/9/10] suivant permission)

int perm=2; // serait mieux dans l'eeprom. Changé par telecommande, copié généralement dans autor.
int freqgyro=0; // 300 rapide, 1000 lent
bool Signal=0; 

// tableau capteur analogique Sana
int Sana[13][9]{// colonne 0=rien 1=klaxon 2=lumiere 3=poteau 4=test 5-6-7-8=telco 9=portier
// durée de crenaux pour tous les SL
  {0,1,1,1,1,1,1,1,1},//duree mini du créneau Bas *0
  {0,1,15,1,1,1,1,1,1},//duree mini du créneau Haut *1
  {0,1,1,1500,1500,500,500,500,500},//duree maxi du créneau Bas *2
  {0,1,15,1500,1500,500,500,500,500},//duree maxi du créneau Haut *3
// sauf  telcos
  {0,Micro,Crep,poteau,test},// pin micro ou lumiere,etc  *4 
  // pour tous les SL, necessaires pour fonction echantillon
  {0,46,30,11,11,1,1,1,1},//nombre de positifs mini acceptable sur 200 mesures analogiques(echantillon) *5
  {0,200,200,200,200,200,200,200,200},//seuil amplitude pour un positif dans echantillon *6
// pour tous les SL
  {0,3,2,1,1,1,1,1,1},//nombre de coups exact pour que dcl passe à UN  *7
  {0,1,1,1,1,1,1,1,1},// TOUS autorisation 1 (+7=) *8
  {0,0,0,1,1,0,1,1,1},// BOUTONS autorisation 2 (+7=) *9 + teleco B
  {0,1,1,1,0,1,1,1,1},// telco C autorisation 3 (+7=) *10
  {0,0,0,0,1,0,1,1,1}, //TEST SEUL autorisation 4 (+7=) *11
  {0,0,0,1,0,0,1,1,1}// POTEAU SEUL  autorisation 5 (+7=) *12 +telco D
  //{0,1,1,1,0,1,1,1,1}// TELCO A à recopier dans sana [10] autorisation 6 (+7=) *13
// TELCO B à recopier dans sana [10] autorisation 7 (+7=) *14
// TELCO C à recopier dans sana [10] autorisation 8 (+7=) *15
// TELCO D à recopier dans sana [10] autorisation 9 (+7=) *16
  };

// tableau des codes de telco
unsigned long code[4] // ex 1234 pour code[3]
  {1111,2222,3333,4444}; // bouton de telecommande A,B,C,D
// tableau potentionmetre et surintensite
int pot[2]{potg,potd};
int amp[2]{ampg,ampd};
int seuilamp[2]{8,11};
unsigned long autoramp[2]{millis(),millis()};

//tableau relai et fin de course gauche et droit
int VL[3][4]{ // colonne 0=gauche 1=droit
  {RelaiFg,RelaiOg,RelaiFd,RelaiOd},//relai gauche et droit 
  {1,600,1,600},//min G=0, max G=1, min D=2, max D=3
  {0,0,0,0}, // valeur intermediaire du FDC de calibrage pour copie dans EEprom  
};
// calibrage des fins de course
void calibrage(){
VL[1][0]=0;     // modication hors cotes ds FDC
VL[1][1]=1300;
VL[1][2]=0;
VL[1][3]=1300;

for (int a=0; a<2; a++){
          int cpt=0;
          Serial.print("autoramp");
          autoramp[0]=millis(); autoramp[1]=millis();
          action(a);
          for (int i=0; i<4; i++){
  Serial.println(recupFDC(i));}                   
          
    while (cpt<2){ //tester 2 FDC
      for (int i=0; i<2; i++){ // gauche droite
        if (ampere(i)){        // surintensite d'un cote ou l'autre
            VL[2][a+2*i]=(analogRead(pot[i])-5*(2*a-1)); // recopie dans le tableau FDC la valeur du pot -5 ou +5 suivant ouv ou ferm
            VL[1][a+2*i]=!a*1250;
            Serial.print(i);
            cpt++;  // compteur de passge de la boucle while
        }action(a);
      } 
    } 
}
for(int i=0; i<4; i++){
 Serial.println(VL[2][i]);
 stockFDC(i,VL[2][i]);} // memorisation de la valeur du pot dans EEprom
} 

// echantillonnage de la valeur de surintensité
bool ampere(bool gd){
  if (millis()<autoramp[gd]+300){return 0;}
  int b=0;
  for (int i=0; i<50; i++){
    if (analogRead(amp[gd])>seuilamp[gd]){
      b++;}
    if (b>30){
      return 1;    }
  } return 0;
}



// detection d'un signal suivant le capteur, quelle action effectuer
bool detect(int SL){ //appelé dans echantillon()
  
  if (SL==0){return 0;} //impossible en principe
  if (SL==1){return (analogRead(Sana[4][SL])>Sana[6][SL]);} // lecture micro attention > a la valeur
  if (SL==2){return (analogRead(Sana[4][SL])<Sana[6][SL]);} // lecture lumiere attention < a la valeur
  if (SL<5){return !(digitalRead(Sana[4][SL]));}  // lecture poteau et test
  return BoutonTele();  // ex code 1234 pour code[3] de telco 3: SL=3+5
}
// memorisation dans l'EEPROM des FDC et convertion
void stockFDC(int ad,int val){
  EEPROM.write(ad*2,val%256); // stock le modulo sur une adresse paire ex: indice 0 var=40 ad*2=0 stock 40 sur 0 
  EEPROM.write(ad*2+1,val/256); // stock la div sur adresse impaire ex :indice 0 var=40 ad*2+1=1 stock 0 sur 1 
}
//recuperation dans l'EEPROM des FDC et convertion
int recupFDC(int ad){
  return EEPROM.read(ad*2)+(EEPROM.read(ad*2+1)*256);
}
int BoutonTele(){ // passe à UN si code=nombre
int x=0;
  if (mySwitch.available()) {
    Serial.println("je suis là");
      if (mySwitch.getReceivedValue()==code[0]){Serial.println("telco A");x= 1;} // telco A
      if (mySwitch.getReceivedValue()==code[1]){perm=2;x= 0;} //telco B
      if (mySwitch.getReceivedValue()==code[2]){perm=3;x= 0;} //telco C
      if (mySwitch.getReceivedValue()==code[3]){perm=4;x=1;} //telco D
    mySwitch.resetAvailable();
  }return x;
  }

// fonction echantillon du capteur 
int echantillon(int x){ // passe à UN si on a atteint b(dans sana[5]) mesures positives sur 200 mesures
  int b=0;
  for (int i=0; i<200; i++){
    if (detect(x)){b++;}
    if (b>=Sana[5][x]){
      
      return 1;}
  }
return 0;}
// fonction declenchement
int dcl(){ // passe à nsl si le signal est valide (nb de coups en fonction du SL en cours)
  static bool e=0; //dernier etat du signal
  static int tot=0; // nb de crenaux en rapport au nombre de coups demandés
  static unsigned long D=millis(); // date du début du crenau
  static int nsl=0; //marqueur de encours
  bool b=0;
  unsigned long duree=0;
  
  if (encours==0){
    encours=quelSL(nsl);
    nsl=encours;
    if (encours!=0){
      D=millis();
      e=1;
      tot=1; // debut de cycle
      
    }
    return 0;
    
  }
  
  duree=(millis()-D);
  if (duree<Sana[0+e][encours]){
    b=echantillon(encours);
    if(b!=e){
      encours=0;
      e=b;
    }return 0;
  }
  if (duree<Sana[2+e][nsl]){
    b=echantillon(encours);
    if(b!=e){
      tot++;
      e=b;
      D=millis(); // nouveau crenau
    }return 0;
  }
  //nsl=encours;
  b=echantillon(encours);
  encours=0;
  if (b==e && tot==2*Sana[7][nsl]){
    return nsl;
  }return 0;
}
// quel signal cause? recherche parmi ceux autorisés
int quelSL(int x){ //on ne regarde que ceux qui suivent le SL N° x
  if (autor==0){ // aucun signal accepté
    return 0;
  }
  for (int i=x+1; i<10; i++){ // nombre de signaux capteur possible
    if (Sana[7+autor][i]){ // N° autorisation
      if (echantillon(i)){ // echantillon de 200 mesures pour clarifier le signal 
      return i; // return N° SL du signal reçu
      }
    }
  }
  return 0; // aucun signal reçu
}
// fonction surintensité
bool surIntensite(bool onoff){ // parametre passe à 1 à chaque declenchement (dcl)
static bool flag=0; // flag de surintensité
  
  if (onoff){ // on regarde si declenchement =1
    for (int i=0; i<4; i++){  
      VL[1][i]=(recupFDC(i));} // on inscrit FDC de l'EEPROM dans le tableau FDC
    autor=perm; // on remet les autorisation aux permissions de depart
    freqgyro=1000; //gyro lent
  return 1;} // retour au loop() 
  
  if (flag){ // flag est vrai déja eu surintensité 
    for (int i=0; i<2; i++){
      if (ampere(i)){ //regarde si les moteurs sont bloqués
        VL[1][2*i+0]=analogRead(pot[i])+20; // on coupe celui qui est bloqué
        VL[1][2*i+1]=analogRead(pot[i])-20;}}
      if (fini()){ // on regarde si les deux moteurs sont arrêtés
        flag=0; // raz flag fin de surintensité
        autor=2; // seulement autorisation bouton sur poteau
        return 1;} return 0;} // return 1 pour prevenir le loop()
        
  if (ampere(0) || ampere(1)){ // detection de surintensité
    flag=1;         // flag de detection surintensité
    for (int j=0; j<2; j++){ // initialisation FDC pour retour vantaux + gestion de non dépassement de FDC
        VL[1][2*j]=max(analogRead(pot[j])-20,recupFDC(2*j)); // minis à pot-20
        VL[1][2*j+1]=min(analogRead(pot[j])+20,recupFDC(2*j+1)); //maxis à pot+20
        }
      valAction=!valAction; // invesion du sens de marche
      autor=0; // personne ne peut arrêter les moteurs 
      freqgyro=300; //gyro rapide
      autoramp[0]=millis(); autoramp[1]=millis();
  return 0;} return 0;}
// fonction action de valAction   
void action(bool a){
  for (int gd=0; gd<2 ; gd++){       //gauche droite
    digitalWrite(VL[0][!a+2*gd],1); //relai opposé a l'action
    bool dep=(analogRead(pot[gd])<VL[1][a+2*gd])xor a; // generalement a 0
    if (dep){
      VL[1][a+2*gd]=1250*!a;} // Si fdc mini , 0, Si maxi , 1250
    // exemple action(1) =>ouvre RelaiFg=0 et RelaiOg=1 tant que (potg>maxi)xor(action(1))
    digitalWrite(VL[0][a+2*gd],dep);
    Serial.print("dep");
    Serial.println(dep);
  }}
// fonction gyrophare pour gerer la frequence de clignotement
void gyro(){
  if (freqgyro==300){
    cligno();
    return; }
  if (fini()){
    digitalWrite(phare,0);
    return; }
    cligno();
    return;}

void cligno(){
   unsigned long now = millis();
   static unsigned long date = 0; 
   static bool etat=0;
  if(now - date >= freqgyro){
  date = now;
  etat = !etat;}
  digitalWrite(phare,etat);
  }
// fonction qui regarde si le moteur tourne
bool fini(){
  int cpt=0; // raz en debut de boucle
  for (int i=0; i<4; i++){
    cpt=cpt+digitalRead(VL[0][i]);}  // addition de la valeur de tous les moteurs
    return (cpt==4); // return 1 si les 4 moteurs sont arrêtés
}


void setup() {
  Serial.begin(9600);
  pinMode(Micro,INPUT_PULLUP);
  pinMode(Crep,INPUT_PULLUP);
  pinMode(12,INPUT_PULLUP);
  pinMode(potg,INPUT);
  pinMode(potd,INPUT);
  pinMode(RelaiOg,OUTPUT);
  pinMode(RelaiOd,OUTPUT);
  pinMode(RelaiFg,OUTPUT);
  pinMode(RelaiFd,OUTPUT);
  pinMode(ampg,INPUT);
  pinMode(ampd,INPUT);
  pinMode(phare,OUTPUT);
  pinMode(poteau,INPUT_PULLUP);
  pinMode(test,INPUT_PULLUP);
  mySwitch.enableReceive(0);  // Receiver on interrupt 0 => that is pin #2
  
  autor=2; // autorisation seulement les boutons
  int x=0;
  while (x!=3){ // on ne sort que quand poteau actionné
    x=dcl();
    
    if (x==4) {calibrage();}
  }
  Signal=1;

}
void loop() {
Serial.print("autor");
Serial.println(autor);
for (int i=0; i<4; i++){
  Serial.print("fdc");
  Serial.print(i);
  Serial.print("=");
  Serial.println(recupFDC(i));
}
bool x=surIntensite(Signal);
  action(valAction);
  
  if(Signal==1){
    
    Signal=0; }
    int y=dcl();
  if (y){
    valAction=!valAction;
    autoramp[0]=millis();  autoramp[1]=millis();
    Signal=1;}
    Serial.print("                        amp0=");
    Serial.println(ampere(0));
    Serial.print("             amp1=");
    Serial.println(ampere(1));
    
    Serial.println(VL[1][0]);
    Serial.println(VL[1][1]);
    Serial.println(VL[1][2]);
    Serial.println(VL[1][3]);
  gyro();

}
