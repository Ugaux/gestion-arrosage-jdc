# web ui app arrosage

Page home avec infos du système séparés par bloc dont :

- meteo dt heure, temperature et humidité air/sol (avec traits semi transparents pour valeurs actuelles)
  Graph avec évolution température et humidité air/sol
- niveau cuve faible, intermédiaire, haut (avec icônes de couleurs) et remplissage avec débitmètre (avec icone, et animé si remplissage) et débit en m3/h (0 si pas de remplissage)
- arrosage et même histoire de débit (total ici) avec listes de toutes les zones et icone vanne ouverte ou fermée pour chaque zone
  (Tester variation du débit avec réglage embout arrosage manuel)
  Graph avec évolution conso total eau en m3

Page arrosage avec planificateur et fonction manuel (avec deux bloc pour arrosage manuel et planifié pour indiquer ce qui se passe en haut, prochain arrosage planifié sinon ceux en cours avec 3 points en dessous et défilement auto, Idem arrosage manuel) + indiquer les zones activent visuellement (manuel ou planifié) + valeur ajustée avec pourcentages saisonnier à côté du temps choisi (valeur choisie est la valeur ajustée ?)
Vue récapitulive à la place des deux blocs du haut en fait avec 3 colonnes (1ere pour nom zones et ways, 2e pour info des jours concernés et 3e 24 sous colonnes pour les heures de la journée avec trait horizontal de l'heure actuelle), remplacer les 2 colonnes de droite (ou la plus à droite) par "Arrosage manuel 3min30" par ex (le temps est un décompte)
Séparer planif et arrosage manuel en 2 onglets, et ajouter collapse pour garder que un way ouvert?
Faire en sorte de limiter le nombre de voies pouvant arroser en meme temps en fct de sur quelle vanne principale elles sont mises (max 50L/min sur gros tuyau noir)
Limiter la duree d'arrosage à 1h et espacement minimal de 30 min pour la meme voie

Maintenance

- Éditeur de zones, avec ajout option "arroser même en cas de pluie" (pop up avec confirmation suppression zone et vanne, "êtes vous sûr, tous les créneaux associés seront supprimés") + devoir donner le debit correspondant en L/min à 3-4 bar par voie utilisée et le diametre/longueur du tuyau utilisé (et retenir la valeur limitante)
- console serial
- allumage relais
