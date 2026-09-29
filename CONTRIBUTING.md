# Contribuer au projet

Merci de votre intérêt pour MIATA_WINK.

## Principes

Toute contribution doit préserver les objectifs suivants :

- aucun mouvement bloquant dans `loop()` ;
- tous les mouvements doivent avoir une durée maximale ;
- une nouvelle commande doit pouvoir arrêter la précédente ;
- aucune activation simultanée de la montée et de la descente d'un même phare ;
- tous les relais doivent être inactifs au démarrage ;
- l'arrêt d'urgence doit rester entièrement matériel.

## Proposition de modification

1. créer un fork ;
2. créer une branche ;
3. réaliser la modification ;
4. tester sur banc ;
5. documenter le matériel utilisé ;
6. ouvrir une pull request.

Exemple :

```text
feature/status-api
fix/manual-stop
docs/wiring-diagram
