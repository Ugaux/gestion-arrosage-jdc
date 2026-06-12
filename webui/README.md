# Useful websites

Alpine.js https://alpinejs.dev/start-here

CSS:

- https://www.w3schools.com/w3css/default.asp
- https://www.w3schools.com/css/default.asp

Icons https://fontawesome.com/v7/search

# ⚙️ Tasks

## ✅ OK

- WEB UI:
  - changement en SPA avec sidebar et overlay/animations (et sidebar présente tout le temps sans close si grande fenêtre)
  - séparation en pages, home (avec status cuve/vannes/capteurs/...), schedule, manual et settings

## 🔧 To clarify / questions

Bugfix:

- ajout transition sur apparition/masquage du hamburger avec :

  ```css
  .hamburger {
    opacity: 1;
    transform: scale(1);
    transition:
      opacity 200ms ease,
      transform 200ms ease,
      visibility 200ms;
  }

  @media (min-width: 1024px) {
    .hamburger {
      opacity: 0;
      transform: scale(0.8);
      visibility: hidden;
      pointer-events: none;
    }
  }
  ```

- handle overlay pages as modals for large screens (as settings page for example)
- password to access settings/setup page
- add actives valves to home page
- Blur sur overlay sidebar et pour tous les modals
- Add placeholder dans input manual duration
- remove x-data from body ?
- check that there is no double execution of apply because of x-model and x-on:change on toggle (maybe use $watch?)
- PB du hover sur sidebar close button
- sidebar close button hover pas désactivé sur mobile
- clear data of modals when closing them or for stuff that does not need to stay in the dom
- sync de l'heure, 2 options :
  - bouton dans web pour sync heure avec tel, avoir le sync heure été hiver auto plutôt ?
  - maj de l'heure de l'esp automatique quand telephone sy connecte si heure différentes de + de qq secondes (pas besoin de mettre dans settings pr changement manuel) + ajout changements dheures de saison
- check comportement sur firefox p/r à -webkit (Vendor extensions https://www.w3schools.com/w3css/w3css_validation.asp)

  ```CSS
    /* For Chrome, Safari, Edge */
    button, a {
      -webkit-tap-highlight-color: transparent;
    }

    /* For Firefox */
    button:active, a:active,
    button:focus, a:focus {
    outline: none; /* Removes the default focus ring */
    /* Optional: Add a custom visual cue to maintain accessibility */
    /* box-shadow: 0 0 0 2px rgba(0,0,0,0.2); */
    }
  ```

- add cleanups fct in js

### Next updates

Look upgrade:

- Keyboard tab focus (:focus, :focus-visible et :focus-within)
- Responsive layout (adaptive on screen size):
  - for large screens, display the panels in a grid pattern instead of each under the other
  - Sur mobile, collapse toutes les zones sauf la première dans page schedule (et toujours avoir qu'une seule zone de uncollapsed)
    => Single open accordion https://www.penguinui.com/components/accordion
- Smooth fast transition for page switching (on nav-link button and section itself)
- FAB (Floating Action Button) pour ajouter nouveau schedule au lieu d'en avoir un par zone ?
- Check que la transition du thème s'applique bien à tous les éléments de toutes les pages (utiliser un délai > 5s pour mieux voir)
- Check que tous les hover et blue highlights sont désactivés sur mobile
- 3 selections toggle for theme mode (light, dark and system) avec juste texte "Theme" https://pinemix.com/components/dark-mode-toggle
- Skeleton sur endroits ou api/ws rempli des infos lors du chargement de page initial
- Niveau cuve (avec widget animé en JS qui bouge doucement quand se vide et beaucoup si en remplissage)

Convenience:

- Avoir rappel régulier sur app pour nettoyage filtre (à valider pour l'enlever) -> à aussi avoir dans futur version intégrée dans HA
- Avoir rappel régulier pour verif pression vessie surpresseur tous les mois (0.2 bar en dessous de pression de déclenchement seuil bas du pressostat) + check et nettoyage du filtre mesh
- Edition zones sur page settings/setup dans une card "Link relays to zones" (max 10 characters for way and zone name, à limiter dans ui mais aussi par ws/api) + "Test zones" pour tester chaque zones et faire sequence allumage progressif des vannes dans la zone concernée
- pouvoir faire un arrosage manuel des zones selectionnées qui s'enchaînent (ex: zone 1 10min, zone 2 5min, press un play general et les zones vont s'enchainer sur 15min), utiliser un FAB (Floating Action Button) "Start" en bas à droite pour démarrer arrosage ?
- Pour montrer le détail du planning optimisé, utiliser un graph [Gantt](https://frappe.io/gantt)

Security :

- `visibility: hidden` sur tout ce qui doit ne plus être accessible par touche "tab" du clavier par ex
- Prevent XSS vulnerabilities + all other known vulnerabilities
- for production, consider:
  - Authentication (username/password)
  - TLS encryption

### App structure

Inspirations :

- [GARDENA smart system App](https://www.youtube.com/watch?v=d-UKpin0ZoA)
- [Yardian Pro Smart Sprinkler Controller](https://www.youtube.com/watch?v=aXRmxPpq07g&t=676s)

#### Top navbar

Avec boutons NOTIFS/SETTINGS à droite

#### Page "Home" _(ETAT INSTANTANNÉ DU SYSTEME)_

Avec infos du système séparés par bloc, dont météo (par station ou internet)

#### Page "Schedule"

Durée d'arrosage :

- 2 possibilité d'afficher la valeur ajustée par le pourcentage saisonnier, soit :
  - entrer le temps d'arrosage qui correspond au 100% et placer à côté la valeur ajustée du mois actuel
  - entrer le temps d'arrosage pour le mois actuel et placer à côté la valeur ajustée qui correspond au 100%
- Hint qui apparait (sur le côté ou par un toast ?) quand modification de la durée d'arrosage, et qui explique le comportement du seasonal adjustement, en fct du choix de gérer la valeur ajustée ci-dessus dire qu'il faut mettre la durée d'arrosage qui correspond au mois :
  - qui à le seasonal adjustement à 100% (et que ça arrosera moins les autres mois en fonction du graph dans page settings)
  - actuel (et que ça arrosera plus ou mois les autres mois en fonction du graph dans page settings)

#### Page "Manual"

#### Page "History" _(ETAT PASSÉ DU SYSTEME)_

Avec bouton de recherche et deux onglets :

- onglet Event: avec ouverture/fermeture des vannes + erreurs + cuve vide (en tant que warning) + (re-)démarrages de l'esp avec la raison (watchdog, power-on, crash, etc.)
- onglet Charts: all sensors have a graph to see evolution, like water tank level and soil humidity for example (NB: For flow sensor log L/min but also total liters used, same for water tank level log fill state but also number of fillings since first start) + graphique conso eau par zone et general + graph nombre de vannes ouvertes

NB: sur tous les graphs qui le permettent, afficher les valeurs actuelles des capteurs avec trait horizontal semi transparent

#### Page "Settings"

Accès avec mot de passe

La page propose :

- Bloc "Maintenance" avec :
  - Test manuel des vannes (à bloquer si arrosage en cours)
  - Fonction purge qui ouvre chaque vanne une après l'autre (même si déjà possibilité d'ouvrir une vanne manuellement dans settings)
- Bloc "Setup" avec éditeur de zones, dont pop-up avec confirmation suppression zone et vanne, "Êtes vous sûr ? Tous les créneaux associés seront supprimés !"
- Bloc "Parameters" avec :
  - Réglage ajustement saisonnié sur l'année et visu en histogramme (une valeur par mois avec obligatoirement un mois à 100%, mettre juillet par défaut)
  - Réglage ajustement heures de levé du soleil sur l'année et visu en graph
- Bloc "Device Info"
