# Matériel — MIATA_WINK

Ce dossier contient la documentation matérielle du contrôleur de phares escamotables pour Mazda MX-5 NA.

Le système repose sur un ESP8266, un expandeur I²C PCF8574, quatre relais actifs à l’état bas et un bouton physique multifonction.

> [!WARNING]
> Ce projet intervient sur le circuit électrique d’un véhicule. Une erreur de câblage peut endommager le véhicule, provoquer un court-circuit, un départ de feu ou un mouvement involontaire des phares.
>
> Le montage doit comporter un fusible, une coupure d’urgence matérielle et un étage d’alimentation adapté à l’environnement automobile.

## Matériel principal

| Quantité | Composant | Fonction |
|---:|---|---|
| 1 | ESP8266 NodeMCU ou Wemos D1 mini | Microcontrôleur, Wi-Fi et serveur web |
| 1 | PCF8574 | Expandeur d’entrées/sorties I²C |
| 4 | Relais actifs à LOW | Commandes montée/descente gauche/droite |
| 1 | Bouton poussoir momentané | Commande physique multifonction |
| 1 | Bouton de coupure d’urgence à verrouillage | Coupure matérielle du contrôleur |
| 1 | Convertisseur DC-DC automobile 12 V vers 5 V | Alimentation de l’électronique |
| 1 | Fusible et porte-fusible | Protection du câblage d’alimentation |
| 1 | Protection contre l’inversion de polarité | Protection de l’électronique |
| 1 | Protection contre les surtensions transitoires | Protection contre les perturbations automobiles |
| — | Câbles automobiles et connecteurs isolés | Raccordement |
| — | Boîtier isolant | Protection du montage |

## Matériel retiré

Les premières versions du projet comportaient deux bandeaux LED WS2812B ainsi que des relais dédiés à leur alimentation.

Ces composants ont été retirés physiquement et ne sont plus pris en charge par le firmware.

## Alimentation

Le contrôleur ne doit jamais être directement alimenté par le réseau 12 V du véhicule.

La chaîne d’alimentation recommandée est la suivante :

```text
+12 V véhicule
      |
      +--- Fusible placé près de la source
      |
      +--- Coupure d’urgence à contact normalement fermé
      |
      +--- Protection contre l’inversion de polarité
      |
      +--- Protection contre les surtensions et transitoires
      |
      +--- Convertisseur DC-DC automobile 12 V vers 5 V
      |
      +--- ESP8266 + PCF8574 + carte de relais
```

Le convertisseur doit être conçu pour supporter les variations et perturbations d’un réseau électrique automobile.

## Coupure d’urgence

La coupure d’urgence doit être matérielle et indépendante du firmware.

Elle doit couper le +12 V alimentant le contrôleur avant le convertisseur DC-DC. Elle ne doit pas être reliée uniquement à une entrée de l’ESP8266.

Le dispositif recommandé est :

- à verrouillage mécanique ;
- avec déverrouillage volontaire, par exemple par rotation ;
- équipé d’un contact normalement fermé, ou `NC` ;
- adapté au courant continu ;
- dimensionné pour le courant effectivement interrompu.

Si le contact ne supporte pas directement le courant du contrôleur, il doit commander un relais ou un contacteur principal correctement dimensionné.

La coupure doit désalimenter au minimum :

- l’ESP8266 ;
- le PCF8574 ;
- la carte de relais ;
- les bobines des relais ;
- toute alimentation séparée de type `JD-VCC`, si elle est utilisée.

Le bouton `System Stop` de l’interface web est un arrêt logiciel. Il ne remplace pas la coupure d’urgence matérielle.

## Mapping de l’ESP8266

| Fonction | Broche |
|---|---|
| SDA I²C | `D2` |
| SCL I²C | `D1` |
| Bouton physique | `D7` |
| Masse | `GND` |

Le bouton physique est raccordé entre `D7` et `GND`.

Le firmware utilise la résistance de rappel interne :

```cpp
pinMode(buttonPin, INPUT_PULLUP);
```

L’état électrique est donc :

- `HIGH` lorsque le bouton est relâché ;
- `LOW` lorsque le bouton est pressé.

## Mapping du PCF8574

| Port | Fonction |
|---|---|
| `P0` | Relais de montée du phare gauche |
| `P1` | Relais de descente du phare gauche |
| `P2` | Relais de montée du phare droit |
| `P3` | Relais de descente du phare droit |
| `P4` | Libre |
| `P5` | Libre |
| `P6` | Libre |
| `P7` | Libre |

Adresse I²C utilisée par défaut :

```cpp
#define PCF8574_ADDR 0x20
```

L’adresse réelle dépend de la configuration des broches ou cavaliers `A0`, `A1` et `A2`. Selon cette configuration, l’adresse peut généralement être comprise entre `0x20` et `0x27`.

## Logique des relais

Les entrées de la carte de relais sont actives à l’état bas :

```cpp
#define RELAY_ON  LOW
#define RELAY_OFF HIGH
```

| Sortie du PCF8574 | Relais |
|---|---|
| `LOW` | Activé |
| `HIGH` | Désactivé |

Au démarrage, le firmware écrit `0xFF` dans le PCF8574 afin de placer les huit sorties à l’état haut et de désactiver les relais.

Le comportement réel de la carte de relais lors de la mise sous tension et de la coupure d’alimentation doit être vérifié sur banc avant son installation dans le véhicule.

## Compatibilité électrique I²C

L’ESP8266 utilise une logique de 3,3 V.

Certaines cartes PCF8574 alimentées en 5 V comportent des résistances de rappel reliant SDA et SCL au 5 V. Cette configuration peut exposer les entrées de l’ESP8266 à une tension inadaptée.

Avant la mise sous tension, vérifier :

- la tension d’alimentation du PCF8574 ;
- la tension des résistances de rappel I²C ;
- le schéma du module ;
- la compatibilité de la carte avec une logique de 3,3 V.

Les solutions possibles sont notamment :

- alimenter le PCF8574 en 3,3 V si le module et la carte de relais le permettent ;
- relier les résistances de rappel I²C au 3,3 V ;
- retirer ou modifier les résistances de rappel intégrées au module ;
- utiliser un convertisseur de niveau I²C bidirectionnel.

## Moteurs de phares de la MX-5 NA

Les moteurs de phares de la Mazda MX-5 NA possèdent un mécanisme interne de fin de course.

Lorsqu’un phare atteint sa position complètement ouverte ou complètement fermée, son moteur arrête automatiquement son mouvement. Le firmware conserve malgré tout une temporisation maximale afin de ne pas laisser inutilement les relais de commande actifs.

Les fins de course internes permettent de retrouver les positions extrêmes avec les commandes complètes `Up` et `Down`.

Ils ne donnent cependant aucune information de position intermédiaire à l’ESP8266. Les positions comprises entre 0 et 100 % restent donc estimées à partir de la durée d’activation des moteurs.

## Relais et interverrouillage

La montée et la descente d’un même phare ne doivent jamais être commandées simultanément.

Un interverrouillage logiciel est recommandé. Un interverrouillage électrique ou mécanique est préférable lorsque la carte et le câblage le permettent.

Avant l’installation, vérifier :

- le calibre des contacts des relais ;
- la compatibilité avec le courant continu ;
- le courant réellement commuté ;
- le comportement des relais en cas de redémarrage ;
- le comportement des relais lorsque les entrées sont flottantes ;
- l’absence d’activation simultanée des deux directions ;
- le retour des contacts au repos lorsque les bobines sont désalimentées.

## Recommandations de montage

- Placer le fusible au plus près de la prise +12 V.
- Utiliser du câble automobile multibrin correctement dimensionné.
- Utiliser des cosses serties et des connecteurs verrouillables.
- Protéger les câbles contre les vibrations, l’abrasion et les arêtes métalliques.
- Installer l’électronique dans un boîtier isolant.
- Éloigner les lignes SDA et SCL des câbles de puissance.
- Maintenir les connexions I²C aussi courtes que possible.
- Prévoir une masse fiable et correctement dimensionnée.
- Vérifier l’absence de retour d’alimentation par les lignes de signal.
- Tester chaque relais séparément avant de raccorder les commandes du véhicule.
- Garder la coupure d’urgence immédiatement accessible.

## Contenu associé

La documentation détaillée est disponible dans :

- [`../docs/wiring.md`](../docs/wiring.md) pour le câblage ;
- [`../docs/safety.md`](../docs/safety.md) pour les consignes de sécurité ;
- `../docs/diagrams/` pour les schémas ;
- `../docs/screenshots/` pour les captures de l’interface.
