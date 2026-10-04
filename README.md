# FolderTags

Tags colorés pour les dossiers sous Windows, façon Finder macOS.

## Statut
Projet en démarrage (phase 0 : cadrage).

## Objectif
- Clic droit sur un dossier → pastilles de couleur (rouge, orange, jaune, vert, bleu, violet, gris).
- Une pastille colorée s'affiche sur le dossier dans l'Explorateur.
- Retrouver tous les dossiers d'une couleur (vue de recherche / Accès rapide).

## Limites techniques de Windows (à connaître)
- Une extension shell ne peut pas afficher une pastille **à côté du texte** du nom, contrairement à macOS. Windows n'expose que les **icon overlays**, affichés **en bas à gauche de l'icône** du dossier.
- Windows limite les overlays à **15 handlers** au total, partagés avec OneDrive, Dropbox, etc. Il faut 7 slots pour 7 couleurs, et nos noms doivent passer en tête alphabétique.
- Alternative pour se rapprocher du rendu macOS : colorer l'icône du dossier elle-même, ou afficher la couleur dans une colonne de la vue Détails.

## Architecture prévue
1. `FolderTags.Core` : stockage des tags (JSON dans `%APPDATA%\FolderTags`).
2. `FolderTags.Overlay` : 7 icon overlay handlers (COM, un par couleur).
3. `FolderTags.Menu` : commande de menu contextuel (`IExplorerCommand`).
4. `FolderTags.App` : fenêtre listant les dossiers par couleur, installation/désinstallation.

## Feuille de route
- [ ] Phase 1 : Core + prototype d'un overlay (une couleur)
- [ ] Phase 2 : menu contextuel avec les 7 couleurs
- [ ] Phase 3 : application de recherche par couleur + Accès rapide
- [ ] Phase 4 : installeur
