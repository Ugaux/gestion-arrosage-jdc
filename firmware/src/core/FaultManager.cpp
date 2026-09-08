#include "FaultManager.h"

#include <Arduino.h>
#include "core/DeviceStateStorage.h"

// -----------------------------------------------------------------------------
// Fault policies
// -----------------------------------------------------------------------------

FaultManager::Policy FaultManager::policy(Fault::Code code) const {
  switch (code) {
    case Fault::Code::DeviceNotFound:
    case Fault::Code::LostPower:
    case Fault::Code::RfRemoteInitFailed:
    case Fault::Code::AdcSamplerInitFailed:
    case Fault::Code::FlowSensorInitFailed:
    case Fault::Code::SyncRTCFailed:
    case Fault::Code::SyncFromRTCFailed:
    case Fault::Code::MissingIOExpander:
    case Fault::Code::LoadFailed:
    case Fault::Code::SaveFailed:  // todo: not sure
    case Fault::Code::LittleFsMountingFailed:
    case Fault::Code::RuntimeBuildFailed:
      // startup fault + restart required
      // => condition requires restart/reinitialization
      // (it is not persisted because begin() detects it again)
      return { false, true };

    case Fault::Code::FaultOverflow:
    case Fault::Code::WateringPumpOverCurrent:
    case Fault::Code::InconsistentLevelSensors:
    case Fault::Code::OperationFailed:
    case Fault::Code::NoWaterWhileFilling:
    case Fault::Code::FillingWaterWhilePumpOff:
    case Fault::Code::AdcDmaReadFailed:
    case Fault::Code::PumpCurrentReadingFailed:
    case Fault::Code::FlowReadingFailed:
    case Fault::Code::SoilMoistureReadingFailed:
    case Fault::Code::RadioCommandSendFailed:
    case Fault::Code::RadioCommandRejected:
      // persistent + no restart required
      return { true, false };

    case Fault::Code::PersistenceReadFail:
    case Fault::Code::PersistenceWriteFail:
    case Fault::Code::StationConnectionFailed:
    case Fault::Code::StationSleepModeFailed:
    case Fault::Code::MdnsFailed:
    case Fault::Code::ApCreationFailed:
    case Fault::Code::PowerAdjustFailed:
      // transient + recoverable
      return { false, false };
  }

  return { false, false };
}

// -----------------------------------------------------------------------------
// FaultManager implementation
// -----------------------------------------------------------------------------

FaultManager::FaultManager(DeviceStateStorage &storage)
  : m_storage(storage) {}

void FaultManager::begin() {
  log_d("FaultManager is initializing...");

  if (m_callbackCount == 0) {
    log_e("No onFaultCountChanged callbacks have been configured!");
    while (true) delay(1000);
  }

  restoreFaults();

  log_i("FaultManager has initialized");
}

bool FaultManager::set(Fault::Component component, Fault::Code code) {
  bool    changed    = false;
  uint8_t faultCount = 0;

  Fault::Entry entry = {
    .component = component,
    .code      = code,
  };

  {
    std::lock_guard lock(m_mutex);

    // FaultOverflow is managed internally.
    if (entry.component == Fault::Component::FaultManager
        && entry.code == Fault::Code::FaultOverflow) {
      return false;
    }

    // Don't add the same fault twice
    for (uint8_t i = 0; i < m_entryCount; i++) {
      if (m_entries[i].component == entry.component
          && m_entries[i].code == entry.code) {
        return false;
      }
    }

    entry.id = m_nextFaultId;

    // Store normal fault.
    if (m_entryCount < kMaxFaults - 1) {
      m_entries[m_entryCount++] = entry;
      changed                   = true;
    }
    // Reserve the final slot for FaultOverflow.
    else if (m_entryCount < kMaxFaults) {
      m_entries[m_entryCount++] = {
        .component = Fault::Component::FaultManager,
        .code      = Fault::Code::FaultOverflow,
        .id        = m_nextFaultId,
      };
      changed = true;
    }

    m_nextFaultId++;
    faultCount = m_entryCount;
  }

  // Always log the fault, even if the buffer is already full.
  logFault(entry, faultCount);

  if (!changed)
    return false;

  saveFaults();
  notifyFaultCountChanged();
  return true;
}

bool FaultManager::clear(Fault::Entry entry) {
  bool changed = false;

  {
    std::lock_guard lock(m_mutex);

    for (uint8_t i = 0; i < m_entryCount; i++) {
      if (m_entries[i].component != entry.component
          || m_entries[i].code != entry.code
          || m_entries[i].id != entry.id)
        continue;

      // Remove the entry while preserving the order of the remaining faults.
      for (uint8_t j = i + 1; j < m_entryCount; j++)
        m_entries[j - 1] = m_entries[j];

      --m_entryCount;
      changed = true;
      break;
    }
  }

  if (!changed)
    return false;

  log_d("%s fault of %s has been cleared",
        Fault::toString(entry.code).data(),
        Fault::toString(entry.component).data());

  saveFaults();
  notifyFaultCountChanged();
  return true;
}

bool FaultManager::clear(Fault::Component component, Fault::Code code) {
  Fault::Entry entry;

  {
    std::lock_guard lock(m_mutex);

    for (uint8_t i = 0; i < m_entryCount; i++) {
      if (m_entries[i].component != component
          || m_entries[i].code != code)
        continue;

      entry = m_entries[i];
      break;
    }
  }

  if (entry.id == 0)
    return false;

  return clear(entry);
}

bool FaultManager::dismissOldest() {
  [[maybe_unused]] Fault::Entry entry;  // only for log_d

  {
    std::lock_guard lock(m_mutex);

    if (m_entryCount == 0)
      return false;

    entry = m_entries[0];

    for (uint8_t i = 1; i < m_entryCount; i++)
      m_entries[i - 1] = m_entries[i];
    --m_entryCount;
  }

  log_d("%s fault of %s has been cleared",
        Fault::toString(entry.code).data(),
        Fault::toString(entry.component).data());

  saveFaults();
  notifyFaultCountChanged();
  return true;
}

bool FaultManager::dismissAll() {
  {
    std::lock_guard lock(m_mutex);

    if (m_entryCount == 0)
      return false;

    m_entryCount = 0;
  }

  log_d("All faults have been cleared");

  saveFaults();
  notifyFaultCountChanged();
  return true;
}

bool FaultManager::any() const {
  std::lock_guard lock(m_mutex);

  return m_entryCount > 0;
}

uint8_t FaultManager::count() const {
  std::lock_guard lock(m_mutex);

  return m_entryCount;
}

bool FaultManager::oldest(Fault::Entry &entry) const {
  std::lock_guard lock(m_mutex);

  if (m_entryCount == 0)
    return false;

  entry = m_entries[0];
  return true;
}

bool FaultManager::has(Fault::Component component, Fault::Code code) const {
  std::lock_guard lock(m_mutex);

  for (uint8_t i = 0; i < m_entryCount; i++) {
    if (m_entries[i].component == component && m_entries[i].code == code) {
      return true;
    }
  }

  return false;
}

void FaultManager::subscribeOnFaultCountChanged(FaultCallback callback) {
  if (m_callbackCount >= kMaxCallbacks)
    return;

  m_callbacks[m_callbackCount++] = std::move(callback);
}

void FaultManager::notifyFaultCountChanged() const {
  const auto c = count();
  for (uint8_t i = 0; i < m_callbackCount; i++)
    if (m_callbacks[i]) m_callbacks[i](c);
}

void FaultManager::logFault(Fault::Entry entry, uint8_t faultCount) const {
  log_e("Fault %d! [%s] %s%s",
        entry.id,
        Fault::toString(entry.component).data(),
        Fault::toString(entry.code).data(),
        faultCount == kMaxFaults
          ? " (fault overflow)"
        : faultCount == kMaxFaults - 1
          ? " (maximum active faults reached)"
          : "");
}

void FaultManager::restoreFaults() {
  uint16_t faults[kMaxFaults] = {};
  if (!m_storage.loadFaults(faults)) {
    set(Fault::Component::FaultManager, Fault::Code::PersistenceReadFail);
    return;
  }

  log_d("Restoring stored faults...");

  {
    std::lock_guard lock(m_mutex);

    for (uint8_t i = 0; i < kMaxFaults; ++i) {

      if (faults[i] == 0)
        continue;

      const uint16_t value = faults[i];

      Fault::Entry entry = decode(value);
      entry.id           = m_nextFaultId++;

      m_entries[m_entryCount++] = entry;
    }
  }

  // Log outside the mutex.
  for (uint8_t i = 0; i < m_entryCount; ++i) {
    Fault::Entry entry = m_entries[i];
    log_e("Restored fault: [%s] %s",
          Fault::toString(entry.component).data(),
          Fault::toString(entry.code).data());
  }

  log_d("All stored faults have been restored");
  notifyFaultCountChanged();
}

void FaultManager::saveFaults() {
  uint16_t faults[kMaxFaults] = {};

  {
    std::lock_guard lock(m_mutex);

    uint8_t faultCount = 0;

    for (uint8_t i = 0; i < m_entryCount; ++i) {
      const auto &entry = m_entries[i];

      if (!policy(entry.code).persistent)
        continue;

      faults[faultCount++] =
        encode(entry.component, entry.code);
    }
  }

  if (!m_storage.saveFaults(faults)) {
    set(Fault::Component::FaultManager, Fault::Code::PersistenceWriteFail);
    return;
  }
}

uint16_t FaultManager::encode(Fault::Component component, Fault::Code code) const {
  return 1
         + (static_cast<uint16_t>(component) << 8)
         + static_cast<uint16_t>(code);
}

Fault::Entry FaultManager::decode(uint16_t value) const {
  value--;

  return {
    .component =
      static_cast<Fault::Component>((value >> 8) & 0xFF),

    .code =
      static_cast<Fault::Code>(value & 0xFF),
  };
}
