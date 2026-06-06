from flask import Flask, send_file

app = Flask(__name__, static_url_path="/assets", static_folder="www/assets")


@app.route("/")
def index():
    return send_file("www/index.html")


@app.route("/add")
def add():
    return send_file("www/add.html")


@app.route("/edit")
def edit():
    return send_file("www/edit.html")


app.run(debug=True)
