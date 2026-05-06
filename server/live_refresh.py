from livereload import Server


def alert(path):
    print("File changed:", path)


if __name__ == "__main__":
    server = Server()
    server.watch("www/assets/css/*.css", alert)
    server.watch("www/assets/js/core/*.js", alert)
    server.watch("www/assets/js/pages/*.js", alert)
    server.watch("www/assets/js/*.js", alert)
    server.watch("www/pages/*.html", alert)
    server.watch("www/*.html", alert)
    server.serve(port=5500, host="0.0.0.0", root=".", liveport=35729)
