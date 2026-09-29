# Câblage — MIATA_WINK

Ce document décrit le câblage du contrôleur de phares escamotables pour Mazda MX-5 NA.

> [!WARNING]
> Débrancher la batterie du véhicule avant toute intervention sur le câblage.
>
> Vérifier l’ensemble du montage au multimètre avant de remettre le circuit sous tension.

## Vue d’ensemble

```text
                          CONTRÔLEUR MIATA_WINK

+12 V véhicule
      |
      +--- Fusible
      |
      +--- Coupure d’urgence NC
      |
      +--- Protections automobiles
      |
      +--- Convertisseur DC-DC 12 V vers 5 V
                    |
                    +--- ESP8266
                    |
                    +--- PCF8574
                    |
                    +--- Carte de relais
                              |
                              +--- Montée gauche
                              +--- Descente gauche
                              +--- Montée droite
                              +--- Descente droite
```

La coupure d’urgence doit interrompre physiquement l’alimentation du contrôleur. Elle ne doit pas dépendre de l’ESP8266 ou du firmware.

## Alimentation principale

### Câblage de principe

```text
+12 V véhicule
      |
     [F1]
      |
     [S1 NC]
      |
 [Protection automobile]
      |
 [Convertisseur DC-DC]
      |
     +5 V
```

Avec :

- `F1` : fusible placé au plus près de la source d’alimentation ;
- `S1` : contact normalement fermé de la coupure d’urgence ;
- protection automobile : inversion de polarité, transitoires et filtrage ;
- convertisseur DC-DC : conversion du 12 V automobile vers un 5 V régulé.

### Masse

```text
Masse véhicule
      |
      +--- GND du convertisseur DC-DC
                  |
                  +--- GND ESP8266
                  +--- GND PCF8574
                  +--- GND carte de relais
```

Toutes les masses logiques doivent avoir une référence commune, sauf si le montage utilise une isolation galvanique conçue et câblée à cet effet.

## Coupure d’urgence

### Principe

La coupure d’urgence doit être insérée sur le +12 V avant le convertisseur DC-DC :

```text
+12 V véhicule
      |
   Fusible
      |
   Contact NC
   coupure d’urgence
      |
   Entrée positive
   convertisseur DC-DC
```

Lorsque le bouton est enclenché :

1. le contact normalement fermé s’ouvre ;
2. le convertisseur DC-DC n’est plus alimenté ;
3. l’ESP8266 et le PCF8574 s’éteignent ;
4. les bobines des relais doivent être désalimentées ;
5. les contacts des relais doivent revenir à leur état de repos.

### Alimentation séparée des relais

Certaines cartes de relais possèdent deux alimentations distinctes :

- `VCC` pour la logique ;
- `JD-VCC` pour les bobines.

Si une alimentation `JD-VCC` séparée est utilisée, elle doit elle aussi être coupée par le dispositif d’urgence.

Couper uniquement l’ESP8266 ou le PCF8574 ne garantit pas nécessairement la désactivation de la carte de relais.

### Utilisation d’un relais principal

Si le bouton de coupure d’urgence ne peut pas interrompre directement le courant requis, il peut commander un relais principal.

Circuit de commande :

```text
+12 V après fusible
      |
   Contact NC
   coupure d’urgence
      |
   Bobine du relais principal
      |
     GND
```

Circuit d’alimentation :

```text
+12 V après fusible
      |
   Contact de puissance
   du relais principal
      |
   Convertisseur DC-DC
```

Le relais principal doit retomber lorsque sa bobine n’est plus alimentée. Sa tension, son calibre et son type de contact doivent être adaptés au circuit.

## Connexions ESP8266

| Fonction | Broche ESP8266 | Connexion |
|---|---|---|
| SDA | `D2` | SDA du PCF8574 |
| SCL | `D1` | SCL du PCF8574 |
| Bouton physique | `D7` | Bouton vers GND |
| Alimentation | Selon la carte | Sortie régulée adaptée |
| Masse | `GND` | Masse commune |

Ne jamais appliquer directement le +12 V du véhicule à une broche de l’ESP8266.

## Bouton physique multifonction

Le bouton physique est un bouton momentané câblé entre `D7` et la masse :

```text
ESP8266 D7 -------- Bouton poussoir -------- GND
```

Le firmware configure `D7` en `INPUT_PULLUP`.

| État du bouton | Niveau lu |
|---|---|
| Relâché | `HIGH` |
| Pressé | `LOW` |

Ce bouton exécute des commandes logicielles. Il ne remplace pas la coupure d’urgence matérielle.

## Connexions I²C

```text
ESP8266 D2 / SDA -------- SDA PCF8574
ESP8266 D1 / SCL -------- SCL PCF8574
ESP8266 GND ------------- GND PCF8574
```

### Adresse du PCF8574

L’adresse utilisée par défaut dans le firmware est :

```cpp
#define PCF8574_ADDR 0x20
```

Elle dépend des entrées d’adresse `A0`, `A1` et `A2`.

Si le PCF8574 ne répond pas, vérifier les adresses possibles avec un scanner I²C. Selon le composant et la variante utilisée, consulter également sa fiche technique.

### Niveaux logiques

L’ESP8266 fonctionne en 3,3 V.

Avant de connecter le bus, mesurer ou vérifier la tension de rappel de SDA et SCL. Certains modules alimentés en 5 V tirent ces lignes vers 5 V.

Si nécessaire, utiliser :

- des résistances de rappel vers 3,3 V ;
- une modification du module ;
- un convertisseur de niveau I²C bidirectionnel ;
- une alimentation compatible avec l’ensemble des composants.

## Connexions du PCF8574

| Port du PCF8574 | Entrée de relais | Fonction |
|---|---|---|
| `P0` | Canal 1 | Montée du phare gauche |
| `P1` | Canal 2 | Descente du phare gauche |
| `P2` | Canal 3 | Montée du phare droit |
| `P3` | Canal 4 | Descente du phare droit |
| `P4` à `P7` | Non raccordés | Réservés |

Le mapping doit être vérifié physiquement, car l’ordre des entrées d’une carte de relais peut différer de l’ordre de ses borniers.

## Logique active à LOW

Les relais sont commandés avec une logique active à l’état bas :

```text
PCF8574 à HIGH  -> relais désactivé
PCF8574 à LOW   -> relais activé
```

Le firmware utilise :

```cpp
#define RELAY_ON  LOW
#define RELAY_OFF HIGH
```

Au démarrage, l’état du PCF8574 est initialisé à :

```cpp
uint8_t pcfState = 0xFF;
```

Tous les relais doivent donc être inactifs.

Le comportement de la carte pendant les premières millisecondes de mise sous tension doit malgré tout être vérifié sur banc.

## Commandes des phares

| Relais | Fonction |
|---|---|
| Montée gauche | Commande la montée du phare gauche |
| Descente gauche | Commande la descente du phare gauche |
| Montée droite | Commande la montée du phare droit |
| Descente droite | Commande la descente du phare droit |

La montée et la descente d’un même phare ne doivent jamais être commandées simultanément.

Les contacts des relais doivent être câblés en fonction du circuit réel du véhicule et du type de commande utilisé. Ne pas déduire le brochage automobile à partir du seul mapping logiciel.

Consulter le schéma électrique correspondant à l’année et à la version exacte du véhicule avant de raccorder les contacts des relais.

## Fins de course internes

Les moteurs des phares de la Mazda MX-5 NA possèdent un système interne de fin de course.

Lorsqu’un moteur atteint une position extrême, son mécanisme interne arrête le mouvement. Le firmware limite néanmoins la durée des commandes de relais.

Les fins de course internes permettent une resynchronisation aux positions extrêmes, mais ils ne fournissent pas de valeur de position intermédiaire au contrôleur.

## Ordre recommandé des essais

### 1. Contrôle hors tension

Vérifier :

- l’absence de court-circuit entre le +12 V et la masse ;
- la polarité du convertisseur ;
- la continuité du fusible ;
- le fonctionnement du contact NC de la coupure d’urgence ;
- la présence d’une masse commune ;
- l’isolation des conducteurs.

### 2. Essai de l’alimentation seule

Déconnecter les commandes du véhicule et vérifier :

- la tension d’entrée du convertisseur ;
- la tension de sortie du convertisseur ;
- le fonctionnement de la coupure d’urgence ;
- l’absence de surchauffe.

### 3. Essai de l’ESP8266 et du PCF8574

Vérifier :

- la création du réseau Wi-Fi ;
- la détection du PCF8574 ;
- l’adresse I²C ;
- les niveaux de SDA et SCL ;
- l’absence de redémarrage intempestif.

### 4. Essai des relais sans les phares

Vérifier un canal à la fois :

- montée gauche ;
- descente gauche ;
- montée droite ;
- descente droite ;
- `System Stop` ;
- coupure d’urgence.

### 5. Essai avec les commandes du véhicule

Effectuer les premiers essais avec :

- le véhicule immobilisé ;
- la coupure d’urgence accessible ;
- une seule direction à la fois ;
- des impulsions courtes ;
- aucune personne à proximité des mécanismes.

## Vérifications finales

Avant l’utilisation :

- vérifier que le fusible est installé ;
- vérifier que la coupure d’urgence désalimente les bobines des relais ;
- vérifier que tous les relais reviennent au repos ;
- vérifier qu’aucune direction opposée n’est activée simultanément ;
- fixer et isoler le boîtier ;
- protéger les faisceaux contre les vibrations ;
- vérifier les connexions après plusieurs cycles ;
- s’assurer qu’aucun câble ne gêne un mécanisme mobile.
