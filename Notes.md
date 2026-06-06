# Watchdog

## Internal vs External

If I use an external watchdog, should I still use the ESP32 internal watchdogs?
Yes. In most cases, use both. Think of them as different layers of protection.

Internal watchdogs protect against:

- task deadlocks
- infinite loops
- blocked code
- FreeRTOS task failures
  They can usually recover the system much faster than an external watchdog.

External watchdog protects against:

- ESP32 completely freezing
- software accidentally disabling internal watchdogs
- clock problems
- severe firmware bugs
- rare hardware lockups
  The external watchdog doesn't care what the software thinks—if it stops receiving heartbeats, it resets the board.

## Heartbeat

Conditions indispensables pour que l'équipement remplisse sa mission :

- CPU fonctionnel
- RAM suffisante
- Tâches critiques vivantes
- Boucles de contrôle opérationnelles
- Wi-Fi indisponible
- perte d'accès à l'interface web
- erreur NTP

LED FAULT + journal d'événements, défauts importants mais non fatals :

- câble Ethernet débranché
- capteur secondaire absent
- etc

Ainsi, le watchdog ne redémarre pas la machine pour des problèmes qu'un redémarrage ne résoudra probablement pas

En résumé. heartbeat seulement si :

- WiFi AP actif
- serveur web actif
- tâche principale vivante
- mémoire disponible suffisante
- aucune erreur critique

```cpp
bool healthy =
    wifi_ok &&
    webserver_ok &&
    task1_ok &&
    task2_ok;

if (healthy) {
    kickWatchdog();
}
```

_Note:_ ne plus gérer les erreurs si système pas healthy pour ne jamais avoir la led fault allumée sans que la led de status ne soit allumée !
-> Je déconseille le cas "STATUS OFF + FAULT ON", car si l'ESP32 est réellement planté, il ne devrait plus être capable de piloter la LED rouge. Cela rend le diagnostic moins clair.

### WebServer health check

```cpp
HTTPClient http;
http.begin("http://127.0.0.1/health");
int code = http.GET();
web_ok = (code == 200);
```

--> pour check que interface web fonctionne, mais seulement arrêter heartbeat si serveur web inaccessible pendant un délai de x secondes ou minutes

## Indicateurs LED

Si ton watchdog externe possède une sortie "OK" ou "HEALTHY", tu peux l'utiliser pour autoriser l'allumage de la LED rouge.

Signification :

- LED Verte STATUS État global du contrôleur ESP32
- LED Rouge FAULT Défaut fonctionnel de l'installation ou d'un périphérique

Led WiFi AP sur boîtier ? Non car la LED de status indique que le hardware propre a l'ESP fonctionne (puce elle-même et wifi). La LED de status allumée indique donc que le système est opérationnel et quand elle est éteinte elle indique un problème (puce cramée, loop frozen, resets en boucle, WiFi AP non opérationnel, ...)

Watchdog resets Suggested action

- 1 reset Log it only
- 2 resets within 24 hours. Warning in web UI/logs
- 3–5 resets within 24 hours. Turn on fault LED
  Continuous reset loop (e.g., >3 within 10 minutes) Immediate fault LED

#

How to detect ap WiFi failure on esp

#

How to turn ESP32 safely off ?

#

Enable brownout detector
