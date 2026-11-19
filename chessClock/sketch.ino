#include <TM1637Display.h>

// ===== Broches =====
const int CLK1 = 2, DIO1 = 3;   // Afficheur joueur 1
const int CLK2 = 4, DIO2 = 5;   // Afficheur joueur 2
const int BTN_J1    = 6;        // Bouton joueur 1
const int BTN_J2    = 7;        // Bouton joueur 2
const int BTN_PLUS  = 8;        // Bouton +
const int BTN_MOINS = 9;        // Bouton -
const int BTN_RESET = 10;       // Bouton reset

TM1637Display afficheur1(CLK1, DIO1);
TM1637Display afficheur2(CLK2, DIO2);
TM1637Display* afficheurs[2] = { &afficheur1, &afficheur2 };

// ===== États de la pendule =====
enum Etat { REGLAGE, EN_COURS, FIN };
Etat etat = REGLAGE;

int secondesReglees = 300;         // Temps par défaut : 5 minutes (300 s)
const int PAS = 5;                 // + et - changent le temps de 5 secondes
const int TEMPS_MIN = 5;           // Minimum : 0:05
const int TEMPS_MAX = 5995;        // Maximum : 99:55

unsigned long tempsRestant[2];     // Temps de chaque joueur (en ms)
int joueurActif = 0;               // 0 = joueur 1, 1 = joueur 2
int perdant = -1;
unsigned long dernierTick = 0;
unsigned long dernierAffichage = 0;

// ===== Gestion des boutons (anti-rebond) =====
const int NB_BOUTONS = 5;
const int pinsBoutons[NB_BOUTONS] = { BTN_J1, BTN_J2, BTN_PLUS, BTN_MOINS, BTN_RESET };
bool etatPrecedent[NB_BOUTONS];
unsigned long dernierChangement[NB_BOUTONS];

// Renvoie true une seule fois, au moment où le bouton est enfoncé
bool vientDEtreAppuye(int i) {
  bool lecture = digitalRead(pinsBoutons[i]);
  if (lecture != etatPrecedent[i] && millis() - dernierChangement[i] > 50) {
    dernierChangement[i] = millis();
    etatPrecedent[i] = lecture;
    return (lecture == LOW);
  }
  return false;
}

// ===== Fonctions utiles =====
void initialiserTemps() {
  tempsRestant[0] = (unsigned long)secondesReglees * 1000UL;
  tempsRestant[1] = tempsRestant[0];
}

void afficherTemps(TM1637Display &aff, unsigned long ms) {
  unsigned long secondes = (ms + 999) / 1000;   // Arrondi à la seconde supérieure
  int mm = secondes / 60;
  int ss = secondes % 60;
  aff.showNumberDecEx(mm * 100 + ss, 0b01000000, true);  // Affiche MM:SS
}

void reinitialiser() {
  etat = REGLAGE;
  perdant = -1;
  initialiserTemps();
}

// ===== Lecture de tous les boutons =====
void lireBoutons() {
  bool appuiJ1    = vientDEtreAppuye(0);
  bool appuiJ2    = vientDEtreAppuye(1);
  bool appuiPlus  = vientDEtreAppuye(2);
  bool appuiMoins = vientDEtreAppuye(3);
  bool appuiReset = vientDEtreAppuye(4);

  // Reset : revient au temps réglé, à tout moment
  if (appuiReset) {
    reinitialiser();
    return;
  }

  if (etat == REGLAGE) {
    // Réglage du temps avec + et - (par pas de 5 secondes)
    if (appuiPlus)  secondesReglees += PAS;
    if (appuiMoins) secondesReglees -= PAS;
    secondesReglees = constrain(secondesReglees, TEMPS_MIN, TEMPS_MAX);
    initialiserTemps();

    // Un joueur appuie : le temps est validé et celui de l'adversaire démarre
    if (appuiJ1) { joueurActif = 1; dernierTick = millis(); etat = EN_COURS; }
    else if (appuiJ2) { joueurActif = 0; dernierTick = millis(); etat = EN_COURS; }
  }
  else if (etat == EN_COURS) {
    // Le joueur dont le temps tourne appuie : on passe à l'autre
    if (joueurActif == 0 && appuiJ1) joueurActif = 1;
    else if (joueurActif == 1 && appuiJ2) joueurActif = 0;
  }
}

// ===== Décompte du temps =====
void mettreAJourTemps() {
  if (etat != EN_COURS) return;
  unsigned long maintenant = millis();
  unsigned long ecoule = maintenant - dernierTick;
  dernierTick = maintenant;

  if (ecoule >= tempsRestant[joueurActif]) {
    tempsRestant[joueurActif] = 0;
    perdant = joueurActif;
    etat = FIN;
  } else {
    tempsRestant[joueurActif] -= ecoule;
  }
}

// ===== Affichage =====
void mettreAJourAffichage() {
  if (millis() - dernierAffichage < 100) return;   // Rafraîchit toutes les 100 ms
  dernierAffichage = millis();

  for (int j = 0; j < 2; j++) {
    if (etat == FIN && j == perdant && (millis() / 500) % 2 == 1) {
      afficheurs[j]->clear();               // Clignotement du joueur qui a perdu
    } else {
      afficherTemps(*afficheurs[j], tempsRestant[j]);
    }
  }
}

// ===== Programme principal =====
void setup() {
  for (int i = 0; i < NB_BOUTONS; i++) {
    pinMode(pinsBoutons[i], INPUT_PULLUP);
    etatPrecedent[i] = HIGH;
    dernierChangement[i] = 0;
  }
  afficheur1.setBrightness(0x0f);
  afficheur2.setBrightness(0x0f);
  initialiserTemps();
}

void loop() {
  lireBoutons();
  mettreAJourTemps();
  mettreAJourAffichage();
}