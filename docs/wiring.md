# Documentation de câblage

## Vue d'ensemble

```text
+12 V véhicule
      |
    Fusible
      |
Arrêt d'urgence NC
      |
Protection automobile
      |
Convertisseur 12 V -> 5 V
      |
      +-------------------+
      |                   |
   ESP8266             Relais / PCF8574


ESP8266 vers PCF8574

| ESP8266 | PCF8574 |
| --- | --- |
| D2 / SDA | SDA |
| D1 / SCL | SCL |
| GND | GND |
| Alimentation compatible | VCC |


Bouton physique

D7 ---- Résistance (peut-être nécessaire) ---- Bouton momentané ---- GND


Relais

| Sortie PCF8574 | Entrée relais |
| --- | --- |
| P0 | Montée gauche |
| P1 | Descente gauche |
| P2 | Montée droite |
| P3 | Descente droite |


Arrêt d'urgence

+12 V véhicule
      |
    Fusible
      |
 [Contact NC de l'arrêt d'urgence]
      |
 [Entrée du convertisseur DC-DC]
