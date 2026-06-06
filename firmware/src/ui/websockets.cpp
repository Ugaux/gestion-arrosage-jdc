unsigned long lastSend = 0;

void handleSensors() {
  // change detection + throttling:
  // only send value if changed and never more often than every 500ms

  readSensors();

  bool          changed = abs(moisture - lastMoisture) > 1;  // threshold example
  unsigned long now     = millis();

  if (changed && (now - lastSend > 500)) {
    lastSend = now;
    sendSensors();
  }
}

void sensors() {
  if (millis() - lastSend >= 500) {

    if (!moistureDirty && !flowDirty && !tempDirty) return;

    StaticJsonDocument<256> doc;
    doc["update"] = "sensors";

    if (moistureDirty) {
      moistureDirty   = false;
      doc["moisture"] = moisture;
    }
    if (flowDirty) {
      flowDirty   = false;
      doc["flow"] = flow;
    }
    if (tempDirty) {
      tempDirty   = false;
      doc["temp"] = temp;
    }

    notifyClients(json);
    lastSend = millis();
  }
}

void sendSnapshot() {
  StaticJsonDocument<400> doc;

  doc["update"] = "snapshot";

  JsonObject sensors  = doc.createNestedObject("sensors");
  sensors["moisture"] = state.moisture;
  sensors["flow"]     = state.flow;

  doc["watering_status"] = state.wateringStatus;
  doc["manual"]          = state.manual;
  doc["next_watering"]   = state.nextWatering;

  String data;
  serializeJson(doc, data);
  notifyClients(data);
}
