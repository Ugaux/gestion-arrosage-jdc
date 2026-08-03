/* Hypothetical future WayManager, owned wherever Way instances are created.
The key change enabling this is Way no longer being static Way m_way[MAX_WAY] 
with static getFirst() / getNext() — instead something external owns a std::vector<Way>(or std::array), 
and WebsocketServer::begin() just iterates a reference to it, same as it already does with sensors.
That mirrors your SensorManager pattern exactly : one owned object, passed by reference, begin() reaches 
into it.class WayManager { */
class WayManager {
public:
  std::vector<Way> &getAll() {
    return m_ways;
  }

private:
  std::vector<Way> m_ways;
};

// WebsocketServer.h
void begin(AsyncWebServer &server, SensorManager &sensors, WayManager &ways);

// WebsocketServer.cpp
void WebsocketServer::begin(AsyncWebServer &server, SensorManager &sensors, WayManager &ways) {
  ... for (Way &w : ways.getAll()) {
    w.onManualChanged.push_back([this, &w](bool manual) {
      ...
    });
  }
}
