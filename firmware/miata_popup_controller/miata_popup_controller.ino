#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <Wire.h>

// --- Configuration du Point d'Accès ---
const char* mySSID = "MIATA_WINK";
const char* mySecKey = "123456789";

// --- Bus I2C (carte d'extension PCF8574) ---
#define I2C_SDA D2
#define I2C_SCL D1
#define PCF8574_ADDR 0x20   // Adresse par défaut si A0/A1/A2 du module sont à la masse.
                            // Si le bus ne répond pas, essaie 0x21 à 0x27 (selon les cavaliers A0-A2 du module).

// --- Mapping des relais sur le PCF8574 (P0 à P7), confirmé sur le câblage réel ---
#define PCF_LEFTUP     0
#define PCF_LEFTDOWN   1
#define PCF_RIGHTUP    2
#define PCF_RIGHTDOWN  3
// P4 à P7 libres.

uint8_t pcfState = 0xFF; // Tous les relais OFF au démarrage (actif LOW -> bit à 1 = OFF)

// --- Définition des broches directes ESP8266 ---
#define buttonPin D7

// --- MACROS POUR LOGIQUE ACTIVE LOW ---
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

ESP8266WebServer myWeb(80);
DNSServer dnsServer;

/* --------------------------------------------------------------------------------------------------------
 * Fonctions PCF8574 (écrit un bit sur l'expandeur I2C en gardant l'état des 7 autres)
 * -------------------------------------------------------------------------------------------------------- */
void pcfWrite(uint8_t pin, uint8_t value) {
  if (value) pcfState |= (1 << pin);
  else       pcfState &= ~(1 << pin);
  Wire.beginTransmission(PCF8574_ADDR);
  Wire.write(pcfState);
  Wire.endTransmission();
}

// --- Machine à États Phares ---
enum AnimType {  
  ANIM_NONE, ANIM_WAVE, ANIM_UP, ANIM_DOWN,  
  ANIM_WINK_G, ANIM_WINK_D,  
  ANIM_SLEEPY, ANIM_PINGPONG, ANIM_DWINK_G,
  ANIM_OPPOSITE, ANIM_CURIOUS
};
AnimType currentAnim = ANIM_NONE;

int seqStep = -1;  
unsigned long seqTimer = 0;
bool seqContinuous = false;

// --- Réglage de hauteur individuelle (contrôle manuel, boucle ouverte) ---
#define FULL_TRAVEL_MS 750UL // Durée pour une course complète 0% <-> 100% (identique aux animations Up/Down)

int leftHeight = 0;   // 0 = complètement baissé, 100 = complètement levé (estimation)
int rightHeight = 0;
bool leftMoving = false, rightMoving = false;
unsigned long leftMoveEnd = 0, rightMoveEnd = 0;

// --- VARIABLES POUR LE BOUTON PHYSIQUE ---
bool isUp = false;
// holdTime = déclenchement Ping-Pong (2s) / longHoldTime = déclenchement Reset Sécurité (4s)
unsigned long debounce = 50, DCgap = 250, holdTime = 2000, longHoldTime = 4000;
bool buttonVal = HIGH, buttonLast = HIGH, DCwaiting = false, DConUp = false, singleOK = true;
unsigned long downTime = 0, upTime = 0, pressStartTime = 0;
bool ignoreUp = false, holdEventPast = false, longHoldEventPast = false;

// --- Déclaration des fonctions ---
void stopAllMotors();
void stopSequence();
void startAnim(AnimType anim, bool continuous);
void updateSequence();
void setHeadlightHeight(bool isLeft, int target);
void updateManualHeadlights();
int checkButton();

/* --------------------------------------------------------------------------------------------------------
 * Page Web
 * -------------------------------------------------------------------------------------------------------- */
String webPage01() {
  String p = R"====(
<html lang="fr-FR">
<head>
    <title>Miata Popups - Synthwave</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <meta name="color-scheme" content="dark">  
    <meta charset="UTF-8">
    <style>
        @import url('https://fonts.googleapis.com/css2?family=Orbitron:wght@500;700;900&display=swap');

        * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }

        body {  
            background-color: #0b0c10;  
            background-image:  
                linear-gradient(rgba(255, 0, 150, 0.05) 1px, transparent 1px),
                linear-gradient(90deg, rgba(255, 0, 150, 0.05) 1px, transparent 1px);
            background-size: 20px 20px;  
            color: #fff;  
            font-family: 'Orbitron', sans-serif;  
            display: flex; flex-direction: column; justify-content: flex-start; align-items: center;  
            min-height: 100vh; margin: 0; padding: 20px;  
        }

        .app-card {
            background-color: rgba(11, 12, 16, 0.85);
            border: 2px solid #ea00d9;
            box-shadow: 0 0 15px rgba(234, 0, 217, 0.4), inset 0 0 10px rgba(234, 0, 217, 0.2);
            border-radius: 12px;
            padding: 30px 20px;
            width: 100%; max-width: 440px;
            margin-bottom: 20px;
        }

        h1 {  
            font-size: 2em; color: #0abdc6; text-align: center; margin-top: 0; margin-bottom: 20px;  
            text-transform: uppercase; letter-spacing: 3px; font-weight: 900;
            text-shadow: 0 0 8px #0abdc6, 0 0 15px rgba(10, 189, 198, 0.6);
        }
       
        .section-title {
            color: #ea00d9; font-size: 1.1em; text-align: center; margin-bottom: 15px; border-bottom: 1px solid #711c91; padding-bottom: 5px;
            margin-top: 15px; letter-spacing: 1px;
        }
       
        .section-title:first-of-type { margin-top: 0; }

        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-bottom: 20px;}
       
        .btn {  
            appearance: none; -webkit-appearance: none;
            background-color: transparent;
            padding: 15px 10px; font-size: 0.9em; font-weight: 700; color: #fff;  
            border-radius: 6px; cursor: pointer;  
            display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 10px;
            text-transform: uppercase; transition: all 0.15s;
            border: 2px solid #711c91;
            box-shadow: 0 0 10px rgba(113, 28, 145, 0.7);
        }

        .btn span.icon { font-size: 2em; }

        .btn:active:not(:disabled) {  
            background-color: #ea00d9; border-color: #ea00d9;
            box-shadow: 0 0 20px #ea00d9; color: #0b0c10;
        }
        .btn:active:not(:disabled) span.icon { filter: brightness(0); }
       
        .btn:disabled { border-color: #222; color: #444; box-shadow: none; cursor: not-allowed; }
        .btn:disabled span.icon { filter: grayscale(100%) opacity(0.2); }
       
        .btn-stop {  
            grid-column: span 2; flex-direction: row; justify-content: center; padding: 20px; font-size: 1.2em;
            border-color: #ff003c; color: #ff003c; text-shadow: 0 0 5px #ff003c; box-shadow: 0 0 12px rgba(255, 0, 60, 0.5);
        }
        .btn-stop:active:not(:disabled) { background-color: #ff003c; color: #fff; text-shadow: none; box-shadow: 0 0 25px #ff003c; }

        /* --- STYLES RÉGLAGE HAUTEUR (Sliders Verticaux) --- */
        .height-wrapper {
            background: rgba(0,0,0,0.5); border: 2px solid #0abdc6; border-radius: 6px; padding: 15px 10px;
            box-shadow: 0 0 10px rgba(10, 189, 198, 0.4); grid-column: span 2;
            display: flex; justify-content: space-around; align-items: flex-start;
        }
        .vslider-col { display: flex; flex-direction: column; align-items: center; gap: 12px; }
        .vslider-col label { color: #0abdc6; font-size: 0.85em; text-transform: uppercase; font-weight: bold; }
        .vslider-wrap { height: 160px; width: 60px; display: flex; align-items: center; justify-content: center; overflow: hidden; }
        input[type=range].vslider {
            -webkit-appearance: none; appearance: none;
            width: 160px; height: 30px; background: transparent;
            transform: rotate(-90deg);
        }
        input[type=range].vslider::-webkit-slider-thumb {
            -webkit-appearance: none; height: 22px; width: 22px; border-radius: 50%;
            background: #ea00d9; cursor: pointer; box-shadow: 0 0 10px #ea00d9; margin-top: -9px;
        }
        input[type=range].vslider::-webkit-slider-runnable-track {
            width: 100%; height: 4px; cursor: pointer; background: #711c91; border-radius: 2px;
        }
        .val-display { color: #ea00d9; font-weight: bold; font-size: 0.9em; }
        .btn-apply { padding: 8px 22px; font-size: 0.8em; }

        /* --- STYLES DE LA CARTE D'INFORMATIONS --- */
        .info-card {
            background-color: rgba(11, 12, 16, 0.85); border: 1px solid #711c91; box-shadow: 0 0 10px rgba(113, 28, 145, 0.3);
            border-radius: 12px; padding: 20px; width: 100%; max-width: 440px; font-size: 0.85em; line-height: 1.6; color: #d3d3d3;
        }
        .info-card h2 { color: #ea00d9; font-size: 1.2em; margin-top: 0; text-align: center; text-transform: uppercase; text-shadow: 0 0 5px rgba(234, 0, 217, 0.4); letter-spacing: 1px; margin-bottom: 15px; }
        .info-card h3 { color: #0abdc6; font-size: 1em; margin-top: 15px; margin-bottom: 5px; text-transform: uppercase; border-bottom: 1px solid #0abdc6; padding-bottom: 5px; }
        .info-card ul { list-style-type: none; padding: 0; margin: 0; }
        .info-card li { margin-bottom: 10px; border-bottom: 1px solid rgba(113, 28, 145, 0.2); padding-bottom: 6px; }
        .info-card li:last-child { border-bottom: none; margin-bottom: 0; padding-bottom: 0; }
        .info-card strong { color: #fff; font-size: 1.05em; }

    </style>
    <script>
        // Commandes Phares (Bloquent uniquement les .btn-motor)
        function sendCommand(cmd, duration) {
            let motorButtons = document.querySelectorAll('.btn-motor');
            if (cmd === 'Stop') { motorButtons.forEach(b => b.disabled = false);
            } else {
                motorButtons.forEach(b => { if (!b.classList.contains('btn-stop')) b.disabled = true; });
                if (duration > 0) setTimeout(() => { motorButtons.forEach(b => b.disabled = false); }, duration);
            }
            fetch('/?cmd=' + cmd).catch(e => console.log(e));
        }

        // Réglage de hauteur individuelle : le slider ne fait qu'afficher la valeur pendant le glissement.
        // Le relais n'est activé qu'au clic sur le bouton "OK", pour limiter le nombre de mouvements et l'usure des moteurs.
        function previewHeight(val, displayId) {
            document.getElementById(displayId).innerText = val + '%';
        }
        function applyHeight(side, sliderId, displayId) {
            const val = document.getElementById(sliderId).value;
            document.getElementById(displayId).innerText = val + '%';
            fetch('/?height' + side + '=' + val).catch(e => console.log(e));
        }
    </script>
</head>
<body>
    <div class="app-card">
        <h1>MIATA_OS</h1>
       
        <div class="section-title">PHARES POP-UPS</div>
        <div class="grid">
            <button class="btn btn-motor" onclick="sendCommand('Up', 750)"><span class="icon">⬆️</span>Up</button>
            <button class="btn btn-motor" onclick="sendCommand('Down', 750)"><span class="icon">⬇️</span>Down</button>
             
            <button class="btn btn-motor" onclick="sendCommand('WinkG', 1500)"><span class="icon">😉</span>Wink L.</button>
            <button class="btn btn-motor" onclick="sendCommand('WinkD', 1500)"><span class="icon">😜</span>Wink R.</button>

            <button class="btn btn-motor" onclick="sendCommand('Sleepy', 500)"><span class="icon">😑</span>Sleepy</button>
            <button class="btn btn-motor" onclick="sendCommand('DWink', 1800)"><span class="icon">😘</span>D-Wink</button>

            <button class="btn btn-motor" onclick="sendCommand('PingPong', 2450)"><span class="icon">🏓</span>Ping-Pong</button>
            <button class="btn btn-motor" onclick="sendCommand('Curious', 7500)"><span class="icon">🧐</span>Curieux</button>
             
            <button class="btn btn-motor" style="border-color: #0abdc6; box-shadow: 0 0 10px #0abdc6;" onclick="sendCommand('Wave', 3400)"><span class="icon">🌊</span>Vague</button>
            <button class="btn btn-motor" style="border-color: #0abdc6; box-shadow: 0 0 10px #0abdc6;" onclick="sendCommand('WaveCont', 0)"><span class="icon">♾️</span>Continue</button>
             
            <button class="btn btn-motor btn-stop" onclick="sendCommand('Stop', 0)"><span class="icon">🛑</span>SYSTEM STOP</button>
        </div>

        <div class="section-title">HAUTEUR INDIVIDUELLE</div>
        <div class="grid">
            <div class="height-wrapper">
                <div class="vslider-col">
                    <label>Gauche</label>
                    <div class="vslider-wrap">
                        <input type="range" class="vslider" id="sliderL" min="0" max="100" value="0"
                               oninput="previewHeight(this.value, 'heightValL')">
                    </div>
                    <span class="val-display" id="heightValL">0%</span>
                    <button class="btn btn-apply" onclick="applyHeight('L', 'sliderL', 'heightValL')">OK</button>
                </div>
                <div class="vslider-col">
                    <label>Droite</label>
                    <div class="vslider-wrap">
                        <input type="range" class="vslider" id="sliderR" min="0" max="100" value="0"
                               oninput="previewHeight(this.value, 'heightValR')">
                    </div>
                    <span class="val-display" id="heightValR">0%</span>
                    <button class="btn btn-apply" onclick="applyHeight('R', 'sliderR', 'heightValR')">OK</button>
                </div>
            </div>
        </div>
    </div>

    <div class="info-card">
        <h2>Documentation</h2>
         
        <h3>Bouton Physique (D7)</h3>
        <ul>
            <li><strong>1 Clic :</strong> Monte ou descend les deux phares selon l'état précédent.</li>
            <li><strong>2 Clics :</strong> Clin d'œil gauche.</li>
            <li><strong>Maintien 2s :</strong> Active l'effet Ping-Pong.</li>
            <li><strong>Maintien 4s :</strong> Reset sécurité (baisse uniquement les phares).</li>
        </ul>

        <h3>Commandes Phares</h3>
        <ul>
            <li><strong>⬆️ Up / ⬇️ Down :</strong> Ouverture ou fermeture complète standard.</li>
            <li><strong>😉 Wink L / 😜 R :</strong> Clin d'œil simple intelligent (1.5s).</li>
            <li><strong>😑 Sleepy :</strong> Ouvre les phares à moitié (Regard endormi/agressif).</li>
            <li><strong>😘 D-Wink :</strong> Double clin d'œil rapide du phare gauche.</li>
            <li><strong>🏓 Ping-Pong :</strong> Essuie-glace (Croisement des phares).</li>
            <li><strong>🧐 Curieux :</strong> Séquence de curiosité (Gauche -> Droite -> Ouverts -> Fermés).</li>
            <li><strong>🌊 Vague / ♾️ Cont. :</strong> Ola de gauche à droite.</li>
            <li><strong>🛑 System Stop :</strong> Coupe instantanément les relais des moteurs.</li>
        </ul>

        <h3>Hauteur Individuelle</h3>
        <ul>
            <li><strong>🎚️ Slider Gauche / Droite :</strong> Choisis la hauteur souhaitée (0% baissé à 100% levé), le phare ne bouge pas encore. Appuie sur <strong>OK</strong> pour envoyer la commande — ça évite de solliciter le moteur à chaque petit mouvement du doigt. Estimation en boucle ouverte (pas de capteur de position) : passe de temps en temps par 0% ou 100% pour resynchroniser.</li>
        </ul>
    </div>
</body>
</html>
  )====";
  return p;
}

/* --------------------------------------------------------------------------------------------------------
 * Gestion des requêtes Web
 * -------------------------------------------------------------------------------------------------------- */
void runPage01() {
  // --- GESTION PHARES ---
  if (myWeb.hasArg("cmd")) {
    String cmd = myWeb.arg("cmd");

    if (cmd == "Stop") {
      stopSequence();  
      stopAllMotors();
    } else if (cmd == "WinkG") startAnim(ANIM_WINK_G, false);
      else if (cmd == "WinkD") startAnim(ANIM_WINK_D, false);
      else if (cmd == "Up") { isUp = true; startAnim(ANIM_UP, false); }
      else if (cmd == "Down") { isUp = false; startAnim(ANIM_DOWN, false); }
      else if (cmd == "Wave") startAnim(ANIM_WAVE, false);
      else if (cmd == "WaveCont") startAnim(ANIM_WAVE, true);
      else if (cmd == "Sleepy") startAnim(ANIM_SLEEPY, false);
      else if (cmd == "PingPong") startAnim(ANIM_PINGPONG, false);
      else if (cmd == "DWink") startAnim(ANIM_DWINK_G, false);
      else if (cmd == "Curious") startAnim(ANIM_CURIOUS, false);
  }

  // --- GESTION HAUTEUR INDIVIDUELLE ---
  if (myWeb.hasArg("heightL")) {
    setHeadlightHeight(true, myWeb.arg("heightL").toInt());
  }
  if (myWeb.hasArg("heightR")) {
    setHeadlightHeight(false, myWeb.arg("heightR").toInt());
  }

  myWeb.send(200, "text/html", webPage01());
}

/* --------------------------------------------------------------------------------------------------------
 * GESTION DU BOUTON PHYSIQUE
 * -------------------------------------------------------------------------------------------------------- */
int checkButton() {    
  int event = 0;
  buttonVal = digitalRead(buttonPin);
  unsigned long m = millis();

  if (buttonVal == LOW && buttonLast == HIGH && (m - upTime) > debounce) {
    downTime = m; pressStartTime = m; ignoreUp = false; singleOK = true; holdEventPast = false; longHoldEventPast = false;
    if ((m - upTime) < DCgap && !DConUp && DCwaiting) DConUp = true; else DConUp = false;
    DCwaiting = false;
  } else if (buttonVal == HIGH && buttonLast == LOW && (m - downTime) > debounce) {        
    if (!ignoreUp) { upTime = m; if (!DConUp) DCwaiting = true; else { event = 2; DConUp = false; DCwaiting = false; singleOK = false; } }
  }

  if (buttonVal == HIGH && (m - upTime) >= DCgap && DCwaiting && !DConUp && singleOK && event != 2) { event = 1; DCwaiting = false; }

  // Maintien : pressStartTime n'est JAMAIS réinitialisé pendant l'appui, donc les deux seuils
  // (Ping-Pong à holdTime, Reset sécurité à longHoldTime) sont bien mesurés depuis l'appui initial.
  if (buttonVal == LOW && (m - pressStartTime) >= holdTime) {
    if (!holdEventPast) { event = 3; ignoreUp = true; DConUp = false; DCwaiting = false; holdEventPast = true; }
    if ((m - pressStartTime) >= longHoldTime) { if (!longHoldEventPast) { event = 4; longHoldEventPast = true; } }
  }
  buttonLast = buttonVal;
  return event;
}

/* --------------------------------------------------------------------------------------------------------
 * SETUP
 * -------------------------------------------------------------------------------------------------------- */
void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(buttonPin, INPUT_PULLUP);

  // Init Bus I2C + PCF8574 (tous les relais sur OFF, actif LOW donc on écrit 0xFF)
  Wire.begin(I2C_SDA, I2C_SCL);
  pcfState = 0xFF;
  Wire.beginTransmission(PCF8574_ADDR);
  Wire.write(pcfState);
  Wire.endTransmission();

  WiFi.softAP(mySSID, mySecKey);
  dnsServer.start(53, "*", WiFi.softAPIP());

  myWeb.on("/", runPage01);
  myWeb.onNotFound([]() {
    myWeb.sendHeader("Location", "/", true);
    myWeb.send(302, "text/plain", "");
  });

  myWeb.begin();
}

/* --------------------------------------------------------------------------------------------------------
 * LOOP
 * -------------------------------------------------------------------------------------------------------- */
void loop() {
  dnsServer.processNextRequest();
  myWeb.handleClient();
 
  updateSequence();
  updateManualHeadlights();

  int b = checkButton();
  if (b == 1) {
    // 1 clic : monte/descend les deux phares selon l'état précédent
    isUp = !isUp;
    startAnim(isUp ? ANIM_UP : ANIM_DOWN, false);
  }
  else if (b == 2) {
    // 2 clics : clin d'œil gauche
    startAnim(ANIM_WINK_G, false);
  }
  else if (b == 3) {
    // Maintien 2s : effet Ping-Pong
    startAnim(ANIM_PINGPONG, false);
  }
  else if (b == 4) {
    // Maintien 4s : reset sécurité, baisse uniquement les phares
    isUp = false;
    startAnim(ANIM_DOWN, false);
  }
}

/* --------------------------------------------------------------------------------------------------------
 * LOGIQUE DES MOTEURS PHARES (via PCF8574)
 * -------------------------------------------------------------------------------------------------------- */
void stopAllMotors() {
  pcfWrite(PCF_LEFTUP, RELAY_OFF);
  pcfWrite(PCF_RIGHTUP, RELAY_OFF);
  pcfWrite(PCF_LEFTDOWN, RELAY_OFF);
  pcfWrite(PCF_RIGHTDOWN, RELAY_OFF);
}

void stopSequence() { currentAnim = ANIM_NONE; seqContinuous = false; }

void startAnim(AnimType anim, bool continuous) {
  // Annule tout réglage manuel de hauteur en cours pour éviter les conflits de relais
  leftMoving = false; rightMoving = false;
  stopAllMotors(); currentAnim = anim; seqContinuous = continuous; seqStep = 0; seqTimer = millis();
}

/* --------------------------------------------------------------------------------------------------------
 * RÉGLAGE DE HAUTEUR INDIVIDUELLE (NON-BLOQUANT, BOUCLE OUVERTE)
 * -------------------------------------------------------------------------------------------------------- */
void setHeadlightHeight(bool isLeft, int target) {
  target = constrain(target, 0, 100);

  // Sécurité : on annule toute animation préréglée et on coupe tous les relais avant de bouger
  // manuellement, pour ne jamais laisser un relais bloqué en position ON.
  currentAnim = ANIM_NONE;
  seqContinuous = false;
  stopAllMotors();
  leftMoving = false; rightMoving = false;

  if (isLeft) {
    int diff = target - leftHeight;
    if (diff != 0) {
      unsigned long duration = (unsigned long)abs(diff) * FULL_TRAVEL_MS / 100;
      pcfWrite(diff > 0 ? PCF_LEFTUP : PCF_LEFTDOWN, RELAY_ON);
      leftMoveEnd = millis() + duration;
      leftMoving = true;
    }
    leftHeight = target;
  } else {
    int diff = target - rightHeight;
    if (diff != 0) {
      unsigned long duration = (unsigned long)abs(diff) * FULL_TRAVEL_MS / 100;
      pcfWrite(diff > 0 ? PCF_RIGHTUP : PCF_RIGHTDOWN, RELAY_ON);
      rightMoveEnd = millis() + duration;
      rightMoving = true;
    }
    rightHeight = target;
  }

  // Met à jour l'état global "isUp" par estimation, pour rester cohérent avec les autres animations
  isUp = (leftHeight >= 50 && rightHeight >= 50);
}

void updateManualHeadlights() {
  unsigned long now = millis();
  if (leftMoving && now >= leftMoveEnd) {
    pcfWrite(PCF_LEFTUP, RELAY_OFF);
    pcfWrite(PCF_LEFTDOWN, RELAY_OFF);
    leftMoving = false;
  }
  if (rightMoving && now >= rightMoveEnd) {
    pcfWrite(PCF_RIGHTUP, RELAY_OFF);
    pcfWrite(PCF_RIGHTDOWN, RELAY_OFF);
    rightMoving = false;
  }
}

void updateSequence() {
  if (currentAnim == ANIM_NONE) return;  
  unsigned long now = millis();
  unsigned long elapsed = now - seqTimer;

  switch (currentAnim) {
    case ANIM_UP:
      pcfWrite(PCF_LEFTUP, RELAY_ON); pcfWrite(PCF_RIGHTUP, RELAY_ON);
      if (elapsed >= 750) { stopAllMotors(); currentAnim = ANIM_NONE; leftHeight = 100; rightHeight = 100; } break;
    case ANIM_DOWN:
      pcfWrite(PCF_LEFTDOWN, RELAY_ON); pcfWrite(PCF_RIGHTDOWN, RELAY_ON);
      if (elapsed >= 750) { stopAllMotors(); currentAnim = ANIM_NONE; leftHeight = 0; rightHeight = 0; } break;
    case ANIM_SLEEPY:
      pcfWrite(PCF_LEFTUP, RELAY_ON); pcfWrite(PCF_RIGHTUP, RELAY_ON);
      if (elapsed >= 500) { stopAllMotors(); currentAnim = ANIM_NONE; } break;
    case ANIM_WINK_G:
      if (seqStep == 0) { if (isUp) pcfWrite(PCF_LEFTDOWN, RELAY_ON); else pcfWrite(PCF_LEFTUP, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); seqTimer = now; seqStep++; } }
      else if (seqStep == 1) { if (isUp) pcfWrite(PCF_LEFTUP, RELAY_ON); else pcfWrite(PCF_LEFTDOWN, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); currentAnim = ANIM_NONE; } } break;
    case ANIM_WINK_D:
      if (seqStep == 0) { if (isUp) pcfWrite(PCF_RIGHTDOWN, RELAY_ON); else pcfWrite(PCF_RIGHTUP, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); seqTimer = now; seqStep++; } }
      else if (seqStep == 1) { if (isUp) pcfWrite(PCF_RIGHTUP, RELAY_ON); else pcfWrite(PCF_RIGHTDOWN, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); currentAnim = ANIM_NONE; } } break;
    case ANIM_OPPOSITE:
      if (isUp) { pcfWrite(PCF_LEFTUP, RELAY_ON); pcfWrite(PCF_RIGHTDOWN, RELAY_ON); } else { pcfWrite(PCF_LEFTDOWN, RELAY_ON); pcfWrite(PCF_RIGHTUP, RELAY_ON); }
      if (elapsed >= 750) { stopAllMotors(); currentAnim = ANIM_NONE; } break;
    case ANIM_DWINK_G:
      if (seqStep == 0) { pcfWrite(PCF_LEFTUP, RELAY_ON); if (elapsed >= 300) { stopAllMotors(); seqTimer = now; seqStep++; } }
      else if (seqStep == 1) { pcfWrite(PCF_LEFTDOWN, RELAY_ON); if (elapsed >= 300) { stopAllMotors(); seqTimer = now; seqStep++; } }
      else if (seqStep == 2) { if (elapsed >= 150) { seqTimer = now; seqStep++; }  }
      else if (seqStep == 3) { pcfWrite(PCF_LEFTUP, RELAY_ON); if (elapsed >= 300) { stopAllMotors(); seqTimer = now; seqStep++; } }
      else if (seqStep == 4) { pcfWrite(PCF_LEFTDOWN, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); currentAnim = ANIM_NONE; } } break;
    case ANIM_PINGPONG:
      if (seqStep == 0) { pcfWrite(PCF_LEFTUP, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); seqTimer = now; seqStep++; } }
      else if (seqStep == 1) { if (elapsed >= 100) { seqTimer = now; seqStep++; }  }
      else if (seqStep == 2) { pcfWrite(PCF_LEFTDOWN, RELAY_ON); pcfWrite(PCF_RIGHTUP, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); seqTimer = now; seqStep++; } }
      else if (seqStep == 3) { if (elapsed >= 100) { seqTimer = now; seqStep++; }  }
      else if (seqStep == 4) { pcfWrite(PCF_RIGHTDOWN, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); currentAnim = ANIM_NONE; } } break;
    case ANIM_CURIOUS:
      if (seqStep == 0) { pcfWrite(PCF_LEFTUP, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); seqTimer = now; seqStep++; } }
      else if (seqStep == 1) { pcfWrite(PCF_LEFTDOWN, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); seqTimer = now; seqStep++; } }
      else if (seqStep == 2) { if (elapsed >= 500) { seqTimer = now; seqStep++; } }
      else if (seqStep == 3) { pcfWrite(PCF_RIGHTUP, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); seqTimer = now; seqStep++; } }
      else if (seqStep == 4) { pcfWrite(PCF_RIGHTDOWN, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); seqTimer = now; seqStep++; } }
      else if (seqStep == 5) { if (elapsed >= 500) { seqTimer = now; seqStep++; } }
      else if (seqStep == 6) { pcfWrite(PCF_LEFTUP, RELAY_ON); pcfWrite(PCF_RIGHTUP, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); seqTimer = now; seqStep++; } }
      else if (seqStep == 7) { if (elapsed >= 2000) { seqTimer = now; seqStep++; } }
      else if (seqStep == 8) { pcfWrite(PCF_LEFTDOWN, RELAY_ON); pcfWrite(PCF_RIGHTDOWN, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); currentAnim = ANIM_NONE; } } break;
    case ANIM_WAVE:
      if (seqStep == 0) { pcfWrite(PCF_RIGHTUP, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); seqTimer = now; seqStep++; }  }
      else if (seqStep == 1) { if (elapsed >= 100) { seqTimer = now; seqStep++; }  }
      else if (seqStep == 2) { pcfWrite(PCF_RIGHTDOWN, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); seqTimer = now; seqStep++; }  }
      else if (seqStep == 3) { if (elapsed >= 100) { seqTimer = now; seqStep++; }  }
      else if (seqStep == 4) { pcfWrite(PCF_LEFTUP, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); seqTimer = now; seqStep++; }  }
      else if (seqStep == 5) { if (elapsed >= 100) { seqTimer = now; seqStep++; }  }
      else if (seqStep == 6) { pcfWrite(PCF_LEFTDOWN, RELAY_ON); if (elapsed >= 750) { stopAllMotors(); seqTimer = now; seqStep++; }  }
      else if (seqStep == 7) { if (elapsed >= 100) { if (seqContinuous) { seqStep = 0; } else { currentAnim = ANIM_NONE; } seqTimer = now; } } break;
  }
}
