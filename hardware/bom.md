# Nomenclature — Bill of Materials

## Électronique principale

| Quantité | Désignation | Spécifications recommandées |
|---:|---|---|
| 1 | ESP8266 | NodeMCU ou Wemos D1 mini |
| 1 | PCF8574 | Module I²C, adresse configurable |
| 1 | Carte de relais | 4 canaux, entrées compatibles avec le PCF8574 |
| 1 | Convertisseur DC-DC | Entrée automobile, sortie 5 V régulée |
| 1 | Bouton poussoir | Contact momentané normalement ouvert |
| 1 | Arrêt d'urgence | Contact NC, verrouillage mécanique |
| 1 | Fusible | À déterminer selon le circuit réel |
| 1 | Porte-fusible | Modèle automobile |
| 1 | Protection inversion | Diode ou MOSFET adapté |
| 1 | Protection surtension | TVS automobile correctement dimensionnée |
| 1 | Convertisseur de niveau I²C | Si le PCF8574 tire SDA/SCL vers 5 V |

## Câblage

- câble automobile multibrin ;
- gaines thermorétractables ;
- cosses serties ;
- connecteurs verrouillables ;
- porte-fusible ;
- boîtier isolant ;
- presse-étoupes ;
- gaine annelée automobile ;
- repères de câbles.

## Vérifications avant achat

Vérifier :

- la tension des bobines de relais ;
- le courant consommé par la carte de relais ;
- la logique active LOW ;
- le comportement des relais lorsque les entrées sont flottantes ;
- la présence d'un cavalier `JD-VCC` ;
- la tension des résistances de rappel I²C ;
- la capacité DC de l'arrêt d'urgence ;
- les caractéristiques du convertisseur DC-DC.
