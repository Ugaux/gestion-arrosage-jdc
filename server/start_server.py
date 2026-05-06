import argparse
import os
import threading
import webbrowser

import mock.esp_client as esp_mock
import uvicorn
from fastapi import FastAPI, Request, status
from fastapi.responses import FileResponse, Response
from fastapi.staticfiles import StaticFiles

DEV_MODE = os.environ.get("DEV_MODE", "0") == "1"
MOCK_ESP_API = os.environ.get("MOCK_ESP_API", "0") == "1"

app = FastAPI()
app.mount("/assets", StaticFiles(directory="www/assets"))


@app.get("/")
def index():
    return FileResponse("www/index.html")


@app.get("/add")
def add():
    return FileResponse("www/add.html")


@app.get("/edit")
def edit():
    return FileResponse("www/edit.html")


@app.get("/api/snapshot")
def get_snapshot():
    print(MOCK_ESP_API)
    if MOCK_ESP_API:
        return esp_mock.mock_snapshot()
    return {
        "wifi": {"ssid": "REALESP32", "ip": "192.168.1.42", "rssi": -10000},
        "valve": {"duration": 15, "openTime": 20},
    }


@app.get("/.well-known/appspecific/com.chrome.devtools.json")
def devtools_json():
    return Response(status_code=status.HTTP_204_NO_CONTENT)


if DEV_MODE:
    # --- Middleware to inject livereload script --- #
    @app.middleware("http")
    async def livereload_injector(request: Request, call_next):
        response = await call_next(request)
        content_type = response.headers.get("content-type", "")
        if "text/html" in content_type.lower():
            body = b""
            async for chunk in response.body_iterator:
                body += chunk

            try:
                html = body.decode("utf-8")
            except UnicodeDecodeError:
                return Response(content=body, status_code=response.status_code, headers=dict(response.headers))

            # Inject script before </body>
            snippet = '<script src="http://localhost:35729/livereload.js"></script>'
            if "</body>" in html:
                html = html.replace("</body>", snippet + "\n</body>")
            else:
                html += snippet
            headers = dict(response.headers)
            headers["Cache-Control"] = "no-store"
            del headers["content-length"]  # remove stale Content-Length

            return Response(
                content=html,
                status_code=response.status_code,
                headers=headers,
                media_type="text/html",
            )

        return response


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--dev_mode", action="store_true")
    parser.add_argument("--mock_esp_api", action="store_true")
    args = parser.parse_args()
    dev_mode_arg = args.dev_mode
    mock_esp_api_arg = args.mock_esp_api

    os.environ["DEV_MODE"] = "1" if dev_mode_arg else "0"
    os.environ["MOCK_ESP_API"] = "1" if mock_esp_api_arg else "0"
    if dev_mode_arg:
        print("-> DEV MODE ENABLED")
    if mock_esp_api_arg:
        print("-> MOCKING ESP API")

    if dev_mode_arg:
        threading.Timer(1.0, lambda: webbrowser.open("http://localhost:8000")).start()
    uvicorn.run("start_server:app", host="0.0.0.0", port=8000, reload=dev_mode_arg)
