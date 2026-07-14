# Useful websites

Alpine.js https://alpinejs.dev/start-here

CSS:

- https://www.w3schools.com/w3css/default.asp
- https://www.w3schools.com/css/default.asp

Icons https://fontawesome.com/v7/search

# ⚙️ Tasks

## ✅ OK

- Changement en SPA avec sidebar et overlay/animations (et sidebar présente tout le temps sans close si grande fenêtre)
- Séparation en pages, home (avec status cuve/vannes/capteurs/...), schedule, manual et settings
- Elements does not shift when scroll bar appears (clearly visible on the settings page button and on the clear button of banner) => this was only happening when duplicating the tab on Brave PC when using the F12 dev-tools on mobile mode

## 🔧 To clarify / questions

- Get config button dans settings qui s'enlève quand config reçu et laisse place à une textinput et bouton "save and reboot" + toast "rebooting..." on click. après appuie sur save button, check si config valide sinon toast erreur

### Next updates

Add cleanups (in destroy()) in js

Look upgrade:

- check comportement sur firefox p/r à -webkit (Vendor extensions https://www.w3schools.com/w3css/w3css_validation.asp)
- Transitions on show/close overlay modals
- Use variable Roboto font instead of just regular, italic and bold?
- Keyboard tab focus (:focus, :focus-visible et :focus-within), dont ne pas mettre sur sidebar si masquée
- Responsive layout (adaptive on screen size):
  - for large screens, display the panels in a grid pattern instead of each under the other
  - Sur mobile, collapse toutes les zones sauf la première dans page schedule (et toujours avoir qu'une seule zone de uncollapsed)
    => Single open accordion https://www.penguinui.com/components/accordion
  - avoir toasts centrés en bas sur mobile et en bas à droite sur desktop (besoin de changer le centrage pour l'avoir p/r à main content et pas body, utiliser classe css .is-desktop ?)
- Smooth fast transition for page switching (on nav-link button and section itself)
- FAB (Floating Action Button) pour ajouter nouveau schedule au lieu d'en avoir un par zone ?
- Check que la transition du thème s'applique bien à tous les éléments de toutes les pages (utiliser un délai > 5s pour mieux voir)
- Check que tous les hover et blue highlights sont désactivés sur mobile
- 3 selections toggle for theme mode (light, dark and system) avec juste texte "Theme" https://pinemix.com/components/dark-mode-toggle
- Skeleton sur endroits ou api/ws rempli des infos lors du chargement de page initial
- Niveau cuve (avec widget animé en JS qui bouge doucement quand se vide et beaucoup si en remplissage)

Convenience:

- Add a text input to edit schedule names
- Avoir rappel régulier sur app pour nettoyage filtre (à valider pour l'enlever) -> à aussi avoir dans futur version intégrée dans HA
- Avoir rappel régulier pour verif pression vessie surpresseur tous les mois (0.2 bar en dessous de pression de déclenchement seuil bas du pressostat) + check et nettoyage du filtre mesh
- Edition zones sur page settings/setup dans une card "Link relays to zones" (max 10 characters for way and zone name, à limiter dans ui mais aussi par ws/api) + "Test zones" pour tester chaque zones et faire sequence allumage progressif des vannes dans la zone concernée
- pouvoir faire un arrosage manuel des zones selectionnées qui s'enchaînent (ex: zone 1 10min, zone 2 5min, press un play general et les zones vont s'enchainer sur 15min), utiliser un FAB (Floating Action Button) "Start" en bas à droite pour démarrer arrosage ?
- Pour montrer le détail du planning optimisé, utiliser un graph [Gantt](https://frappe.io/gantt)

Security :

- Password to access settings/setup page par modal avec flou sur page (avec autorisation par ESP32 backend => mdp stocké dans nvs par ex + se souvenir sur l'esp32 du device pour au moins 1 semaine, pour éviter d'avoir à entrer le mdp trop souvent)
- `visibility: hidden` sur tout ce qui doit ne plus être accessible par touche "tab" du clavier par ex
- Prevent XSS vulnerabilities + all other known vulnerabilities
- for production, consider:
  - Authentication (username/password)
  - TLS encryption

### Websocket

#### Fake socket

In browser dev-tools console, use FakeSocket object to test websocket behavior. The following are available :

- `FakeSocket.close()` to simulation a websocket disconnection
- `FakeSocket.loadScenario(scenarioName, delay = 0)` to load a different scenario from the one chosen on load in AppCfg
- `FakeSocket.startSimulation()` to start simulation
- `FakeSocket.haltSimulation()` to pause simulation

Note :

- when developping, don’t let fake mode accidentally “depend” on real-time assumptions elsewhere
- use it only for rendering / logic testing, not connectivity validation

#### Messages

```js
// TYPES: CMD | SNAPSHOT | ACK | EVENT

// Command (web → ESP32)
{
  "type": "CMD",
  "id": "abc123",          // UUID — for acks
  "action": "waterZone",
  "payload": { "zone": 2, "duration_s": 30 }
}

// Acknowledgement (ESP32 → web)
{
  "type": "ACK",
  "id": "abc123",          // echoes the command id
  "ok": true,
  "error": null            // or "zone_busy" / "valve_fault"
}

// Event pushed by ESP32 (no request needed)
{
  "type": "EVENT",
  "event": "sensorUpdate",
  "payload": { "zone": 2, "moisture_pct": 42, "ts": 1718000000 }
}

// Full state snapshot (sent on every new connection)
{
  "type": "SNAPSHOT",
  "payload": [ {topic: "deviceConfig",event: "updateAll",payload: {..}}, {...}]
}
```

Add a `wsAckTracker.js` only when your pending-ack logic grows beyond a simple Map.
Like if you add retry logic, per-command timeouts, or optimistic UI rollback, that logic deserves its own home.
While it fits in 10 lines inside ws.js, leave it there.

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
