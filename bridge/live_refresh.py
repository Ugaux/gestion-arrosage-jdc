from os import path
from pathlib import Path

from livereload import Server

if __name__ == "__main__":
    server = Server()
    root = Path(__file__).parent.parent

    watched_path1 = path.join(root, "webui", "src", "assets", "app.min.js")
    print(f"Watching path '{watched_path1}'...")
    server.watch(watched_path1)

    watched_path2 = path.join(root, "webui", "src", "assets", "app.min.css")
    print(f"Watching path '{watched_path2}'...")
    server.watch(watched_path2)

    watched_path3 = path.join(root, "webui", "src", "img", "**")
    print(f"Watching path '{watched_path3}'...")
    server.watch(watched_path3)

    watched_path4 = path.join(root, "webui", "src", "*")
    print(f"Watching path '{watched_path4}'...")
    server.watch(watched_path4)

    server.serve(port=5500, host="0.0.0.0", root=".", liveport=35729)
