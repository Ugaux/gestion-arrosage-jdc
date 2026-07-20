from os import path
from pathlib import Path

from livereload import Server

if __name__ == "__main__":
    server = Server()
    root = Path(__file__).parent.parent

    watched_path = path.join(root, "webui", "dist", "app", "**")
    print(f"Watching path '{watched_path}'...")
    server.watch(watched_path)

    server.serve(port=5500, host="0.0.0.0", root=".", liveport=35729)
