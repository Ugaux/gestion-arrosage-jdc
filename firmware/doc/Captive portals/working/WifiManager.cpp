#include <DNSServer.h>

DNSServer m_dnsServer;

void WiFiManager::startAP(bool exclusive)
{

  ...

      // DNS wildcard:
      // Every DNS hostname -> 192.168.4.1
      if (!m_dnsServer.start(53, "*", apIP))
  {
    log_w("DNS FAILED TO START");
    return;
  }

  m_state = State::APActive;
}

void WiFiManager::update(unsigned long now)
{
  if (now - m_lastUpdateTime < UpdateDelayMs)
    return;

  m_lastUpdateTime = now;

  switch (m_state)
  {

  ...

      case State::APActive:
    ...

        // Process DNS requests
        m_dnsServer.processNextRequest();
    break;

    ...
  }
}
