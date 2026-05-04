from flask import Flask, send_from_directory

app = Flask(__name__, static_folder="www/assets")


@app.route("/")
def index():
    return send_from_directory("www", "index.html")


@app.route("/add")
def add():
    return send_from_directory("www", "add.html")


@app.route("/edit")
def edit():
    return send_from_directory("www", "edit.html")


app.run(debug=True)
