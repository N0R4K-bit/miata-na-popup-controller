
---

# 5. `docs/safety.md`

```markdown
# Sécurité

## Avertissement

Ce projet commande des éléments mécaniques et électriques d'un véhicule.

Toute installation est réalisée sous la responsabilité de l'utilisateur.

## Arrêt d'urgence

Le système doit posséder un arrêt d'urgence matériel à contact normalement fermé.

L'arrêt d'urgence doit :

- couper le +12 V avant le convertisseur DC-DC ;
- désalimenter la carte de relais ;
- ne pas dépendre de l'ESP8266 ;
- rester verrouillé après activation ;
- nécessiter une action volontaire pour être réarmé.

## Fusible

Un fusible doit être placé au plus près de la prise d'alimentation +12 V.

Le fusible protège principalement le câblage. Sa valeur doit être déterminée en fonction :

- de la section des conducteurs ;
- du courant maximal du contrôleur ;
- du courant d'appel des relais ;
- du convertisseur DC-DC ;
- des règles applicables à l'installation.

## Relais

Les contacts des relais doivent être adaptés :

- à la tension du circuit ;
- au courant commandé ;
- aux charges inductives éventuelles ;
- à l'environnement automobile.

Ne pas utiliser une carte de relais de faible qualité pour commuter directement un courant moteur important sans vérifier son calibre réel.

## Interverrouillage

La montée et la descente d'un même phare ne doivent jamais être commandées simultanément.

Un interverrouillage logiciel est recommandé, mais une protection matérielle est préférable.

## Essais

Effectuer les premiers essais :

- véhicule immobilisé ;
- moteur thermique arrêté ;
- phares accessibles ;
- sans personne à proximité des mécanismes ;
- avec l'arrêt d'urgence immédiatement accessible ;
- avec un fusible installé ;
- en testant un seul canal à la fois.

## Utilisation routière

Les animations peuvent distraire les autres usagers et être incompatibles avec la réglementation locale.

Ne pas utiliser les animations en circulation.
