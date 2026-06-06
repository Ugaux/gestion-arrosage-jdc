# ⚙️ Tâches

## ✅ OK

## 🔧 To clarify / questions

- transition du darkmode qui s'applique aussi après un clique sur bouton un nav-link (avec durée de transition elevée, on voit bien le phénomène)
  --> devrait pas être comme ça, il faudrait pouvoir séparer la transition du dark mode du reste + ensuite ajouter transition smooth sur bouton nav link et page lors d'un changement de page pour le rendre plus agréable

- ajout transition sur apparition/masquage du hamburger avec :

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

- check que rien du responsive layout n'ait de display none + visibility: hidden sur tout ce qui doit ne plus être accessible par tab par ex

- avoir duration de la transition opacity sur overlay de alpine js avec var --sidebar-duration
- check tous les hover et blue highlights sur tel apres darkmode
- darkmode transition à tester avec délai de 5s pour voir les éléments qui switch instant (check sur toutes les pages)
- darkmode garder logo jdc sans fond et ajouter un fond en css qui utilise une var du theme
- darkmode avec Alpine store, cf chatgpt

- PB du hover sur btn close sidebar
- PB ? 2e call de handleHash qd url "/" entré
- Bouton sidebar highlighté quand page affichée

- petit indicateur que système est connecté :
  - Connected: controls enabled, immediate command send, wait for ACK/state update
  - Reconnecting: controls disabled, overlay/banner visible, automatic reconnect attempts
  - Disconnected: blocking modal/full-screen, no interaction possible

- appuie sur texte "dark mode" pour toggle aussi
- close button hover pas désactivé sur mobile
- AJOUT PAGE 404 NOT FOUND SI URL INVALIDE

- skeleton sur endroits ou api va remplir de data

# Fast web dev

Quick setup for developing and testing the web interface locally.

## Prerequisites

Create the virtual environment and install all python dependencies:

```bash
cd bridge
uv sync
```

## 1. Start the server

```bash
cd bridge/scripts
uv run start_bridge.py [--dev_mode] [--mock_esp_api]
```

### Options

- `--mock_esp_api`  
  Use a fake API for frontend-only development. requests are sent to the ESP32 API using the URL defined in `/server/mock/config.py`.
- `--dev_mode`  
  Enables development features (see below).

---

## 2. Development mode

When using `--dev_mode`, start the file watcher beforehand in a separate terminal with:

```bash
cd bridge/scripts
uv run live_refresh.py
```

This enables:

- Automatic browser opening
- Automatic browser refresh on file changes
- FastAPI auto-reload (backend)
- LiveReload injection (frontend)

---

## 3. Development workflow

Edit the following files:

```
/webui/www/*
/bridge/scripts/start_bridge.py
```

### Tips

- Keep the Python web architecture consistent with the ESP32 implementation (`WebServer.h/cpp`)
- Use the API docs for debugging at http://127.0.0.1:8000/docs

---

## 4. Export for production

Once everything works:

```bash
cd webui/scripts
uv run minify
```

---

## 5. Deploy to ESP32

- Build the filesystem image
- Upload it to ESP32 using PlatformIO
