# Sécurité — MIATA_WINK

## Avertissement général

Ce projet modifie ou complète le circuit électrique d’un véhicule et commande des mécanismes motorisés.

Une erreur de conception, de câblage ou de programmation peut provoquer :

- un court-circuit ;
- une surchauffe ;
- un départ de feu ;
- une décharge de la batterie ;
- une détérioration de l’ESP8266, du PCF8574 ou des relais ;
- une détérioration du faisceau électrique du véhicule ;
- un mouvement involontaire des phares ;
- une perte de la fonction normale des phares ;
- une distraction ou un danger pour les autres usagers.

Ce projet est expérimental et ne constitue pas un équipement automobile homologué.

L’installation et l’utilisation sont réalisées sous la responsabilité de l’utilisateur.

## Règles essentielles

- Débrancher la batterie avant toute intervention sur le faisceau.
- Installer un fusible au plus près de la source +12 V.
- Utiliser une coupure d’urgence matérielle.
- Ne jamais alimenter directement l’ESP8266 en 12 V.
- Utiliser un convertisseur DC-DC adapté à l’environnement automobile.
- Employer des conducteurs correctement dimensionnés.
- Isoler et fixer toutes les connexions.
- Ne jamais activer simultanément la montée et la descente d’un même phare.
- Tester le système sur banc avant son installation.
- Ne pas utiliser les animations sur route ouverte.

## Coupure d’urgence matérielle

La coupure d’urgence est différente du bouton `System Stop` de l’interface web.

### `System Stop`

`System Stop` est une fonction logicielle :

- elle nécessite que l’ESP8266 fonctionne ;
- elle dépend du firmware ;
- elle dépend du PCF8574 et du bus I²C ;
- elle désactive les commandes des relais ;
- elle ne coupe pas physiquement l’alimentation.

### Coupure d’urgence

La coupure d’urgence est une fonction matérielle :

- elle ne dépend pas du firmware ;
- elle ne dépend pas du Wi-Fi ;
- elle ne dépend pas du serveur web ;
- elle coupe le +12 V avant le convertisseur DC-DC ;
- elle doit désalimenter les bobines des relais ;
- elle doit rester accessible pendant les essais.

### Caractéristiques recommandées

Le dispositif doit de préférence être :

- à verrouillage mécanique ;
- à contact normalement fermé, ou `NC` ;
- adapté au courant continu ;
- dimensionné pour la tension et le courant du circuit ;
- installé dans un endroit rapidement accessible ;
- protégé contre les déclenchements accidentels sans devenir difficile à atteindre.

L’utilisation d’un contact normalement fermé permet de désactiver le système si le circuit de commande est interrompu. Le comportement réel dépend néanmoins du schéma complet et doit être testé.

## Fusible

Un fusible doit être placé aussi près que possible du point où le +12 V est prélevé.

Le fusible protège principalement le câblage contre les surintensités et les courts-circuits.

Sa valeur doit être choisie en fonction :

- de la section des conducteurs ;
- du courant maximal du contrôleur ;
- du convertisseur DC-DC ;
- de la carte de relais ;
- du courant d’appel des équipements ;
- des règles de câblage automobile applicables.

Ne pas augmenter la valeur du fusible pour masquer un défaut ou des déclenchements répétés.

Si un fusible fond :

1. couper l’alimentation ;
2. rechercher la cause ;
3. réparer le défaut ;
4. remplacer le fusible par un modèle de même type et de même calibre approprié.

## Alimentation automobile

Le réseau électrique d’un véhicule n’est pas une alimentation 12 V parfaitement stable.

Il peut subir :

- des chutes de tension ;
- des surtensions ;
- des parasites ;
- des transitoires produits par les charges inductives ;
- des perturbations lors du démarrage ;
- une inversion de polarité accidentelle.

Le montage doit intégrer un étage d’alimentation approprié comprenant, selon sa conception :

- une protection contre l’inversion de polarité ;
- une protection contre les surtensions ;
- une protection contre les transitoires ;
- un filtrage ;
- un convertisseur DC-DC automobile ;
- des condensateurs adaptés.

Un convertisseur bon marché destiné à un usage de laboratoire ou domestique peut ne pas être adapté à une installation permanente dans un véhicule.

## Sécurité des relais

Les relais doivent être adaptés :

- à la tension commutée ;
- au courant réel du circuit ;
- au courant continu ;
- aux charges inductives éventuelles ;
- au nombre de cycles attendu ;
- à la température et aux vibrations.

Le courant indiqué sur un module ne garantit pas nécessairement la même capacité pour tous les types de charge.

Vérifier également :

- le comportement des entrées actives à LOW ;
- l’état des relais lorsque le microcontrôleur démarre ;
- l’état des relais lorsque le PCF8574 est absent ;
- l’état des relais lors d’une coupure partielle de l’alimentation ;
- l’alimentation séparée `JD-VCC`, si elle existe ;
- l’absence de retour d’alimentation par les lignes de commande.

## Interverrouillage des directions

Pour chaque phare, les commandes de montée et de descente sont opposées.

Elles ne doivent jamais être actives simultanément.

Le firmware doit appliquer un interverrouillage logiciel. Lorsque cela est possible, ajouter également un interverrouillage matériel ou utiliser une architecture de relais empêchant physiquement l’activation simultanée des deux directions.

Avant chaque essai, vérifier l’état des quatre relais.

## Fins de course internes des moteurs

Les moteurs de phares de la Mazda MX-5 NA disposent de fins de course internes.

Ce mécanisme permet normalement au moteur de s’arrêter lorsqu’il atteint sa position complètement ouverte ou complètement fermée.

Cette caractéristique :

- permet de resynchroniser les phares aux positions extrêmes ;
- évite d’avoir à mesurer continuellement une hauteur réelle pour les commandes complètes ;
- ne remplace pas une temporisation maximale des relais ;
- ne donne aucune position intermédiaire à l’ESP8266 ;
- ne protège pas contre tous les défauts possibles du câblage ou des relais.

Le bon fonctionnement des fins de course doit être contrôlé avant d’utiliser le système.

Si un moteur continue à fonctionner, produit un bruit inhabituel ou chauffe anormalement, actionner immédiatement la coupure d’urgence.

## Hauteur intermédiaire

Les hauteurs comprises entre 0 et 100 % sont estimées à partir de la durée d’activation du moteur.

Cette estimation peut varier en fonction :

- de la tension de batterie ;
- de la température ;
- de l’usure mécanique ;
- des frottements ;
- du temps de réponse des relais ;
- d’une interruption de mouvement ;
- d’un redémarrage de l’ESP8266.

Après une perte d’alimentation ou un comportement incertain, utiliser une commande complète `Up` ou `Down` pour revenir à une position extrême connue.

## Précautions pendant les essais

Les premiers essais doivent être réalisés :

- véhicule à l’arrêt ;
- moteur thermique coupé, sauf nécessité de mesure encadrée ;
- frein de stationnement serré ;
- dans une zone ventilée et dégagée ;
- sans personne près des tringleries et mécanismes ;
- avec la coupure d’urgence immédiatement accessible ;
- avec un extincteur adapté à proximité si les conditions de travail l’exigent ;
- en activant un seul relais à la fois.

Ne pas placer les doigts, des outils ou des câbles dans la trajectoire des phares.

Les mécanismes peuvent bouger brusquement et provoquer un pincement.

## Procédure de premier démarrage

1. Déconnecter les contacts de puissance des relais.
2. Vérifier la polarité de l’alimentation.
3. Vérifier l’absence de court-circuit.
4. Installer le fusible.
5. Vérifier le fonctionnement mécanique de la coupure d’urgence.
6. Alimenter uniquement le contrôleur.
7. Contrôler la tension de sortie du convertisseur DC-DC.
8. Vérifier que les relais restent inactifs au démarrage.
9. Tester chaque sortie séparément.
10. Tester `System Stop`.
11. Tester la coupure d’urgence.
12. Couper l’alimentation.
13. Raccorder les commandes du véhicule.
14. Tester chaque phare séparément avec des impulsions courtes.
15. Tester une ouverture et une fermeture complètes.
16. Vérifier la température des câbles, relais et connecteurs.
17. Fixer définitivement le boîtier et les faisceaux.

## Comportement après une coupure d’urgence

Après activation de la coupure d’urgence :

1. ne pas la réarmer immédiatement ;
2. identifier la cause de l’arrêt ;
3. vérifier les relais ;
4. vérifier le câblage ;
5. vérifier l’absence de surchauffe ;
6. vérifier qu’aucun mécanisme n’est bloqué ;
7. corriger le défaut ;
8. réarmer la coupure ;
9. vérifier que le contrôleur redémarre avec tous les relais inactifs ;
10. envoyer une nouvelle commande uniquement lorsque la situation est sûre.

Le redémarrage ne doit pas déclencher automatiquement une animation ou un mouvement.

## Utilisation sur route

Les animations de phares peuvent :

- distraire les autres usagers ;
- être confondues avec un signal lumineux ;
- réduire temporairement la visibilité ;
- être interdites par la réglementation locale.

Les animations doivent être utilisées uniquement :

- sur terrain privé ;
- véhicule immobilisé ;
- dans un environnement contrôlé ;
- sans gêner ni éblouir d’autres personnes.

Le fonctionnement normal et réglementaire des phares doit rester disponible lorsque le véhicule circule.

## Inspection périodique

Inspecter régulièrement :

- le fusible et son support ;
- le bouton de coupure d’urgence ;
- les cosses et connecteurs ;
- les masses ;
- les câbles exposés aux vibrations ;
- la carte de relais ;
- le convertisseur DC-DC ;
- le boîtier ;
- les traces d’humidité ;
- les traces de corrosion ;
- les signes de surchauffe ;
- le fonctionnement des fins de course ;
- le retour au repos de chaque relais.

Toute connexion desserrée, oxydée ou surchauffée doit être réparée avant une nouvelle utilisation.

## Responsabilité

Ce projet et sa documentation sont fournis sans garantie.

L’auteur et les contributeurs ne garantissent pas :

- la compatibilité avec toutes les versions de Mazda MX-5 NA ;
- la conformité réglementaire ;
- la sécurité d’une installation particulière ;
- la qualité des modules de relais utilisés ;
- la résistance du montage à toutes les perturbations automobiles.

En cas de doute, faire contrôler l’installation par une personne qualifiée en électricité automobile.
