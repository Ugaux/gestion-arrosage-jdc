#include "ConfigManager.h"

#include <unistd.h>
#include <LittleFS.h>
#include <StreamUtils.h>
#include "core/FaultManager.h"
#include "config/generated/ConfigReflection.h"
#include "config/runtime/JsonSerializer.h"
#include "config/runtime/JsonDeserializer.h"

namespace {

namespace OkResult {

struct Tag {};

}  // namespace OkResult

class DebugPrinter {
public:
  static constexpr char kTag[] = "[Config]";

  // required by reflection for traversal
  using TraversalResult = Result<OkResult::Tag>;
  using VisitResult     = Reflection::VisitResult<TraversalResult>;

  template<typename Parent, typename Member>
  VisitResult enter(
    const Reflection::Field<Parent, Member>& field, Member& value) {
    m_pathBuilder.enter(field.name);

    return VisitResult::traverse();
  }

  template<typename Parent, typename Member>
  TraversalResult field(const Reflection::Field<Parent, Member>& field, Member& value) {
    m_pathBuilder.enter(field.name);

    if (auto result = logValue(value, field.unit); !result)
      return result;

    m_pathBuilder.leave();
    return {};
  }

  template<typename Parent, typename Member>
  TraversalResult leave(const Reflection::Field<Parent, Member>& field, Member& value) {
    m_pathBuilder.leave();
    return {};
  }

  template<typename Type>
  TraversalResult schema(Type& value) {
    return {};
  }

  TraversalResult finalize(TraversalResult result) {
    return {};
  }

private:
  struct EmptyCollection {};

  void formatValue(char* buffer, size_t bufferSize, const Frequency& value) {
    switch (value) {
      case Frequency::EveryDay:
        snprintf(buffer, bufferSize, "everyday");
        break;
      case Frequency::EvenDays:
        snprintf(buffer, bufferSize, "even days");
        break;
      case Frequency::OddDays:
        snprintf(buffer, bufferSize, "odd days");
        break;
      case Frequency::SpecificDays:
        snprintf(buffer, bufferSize, "specific days");
        break;
    }
  }

  void formatValue(char* buffer, size_t bufferSize, const WeekDays& value) {
    auto bitsetMask = static_cast<std::bitset<7>>(value.mask());

    snprintf(buffer, bufferSize,
             "%s (Sunday -> Monday)",
             bitsetMask.to_string().c_str());
  }

  void formatValue(char* buffer, size_t bufferSize, const UUID& value) {
    auto id = value.unparse();

    snprintf(buffer, bufferSize,
             "%.*s",
             static_cast<int>(id.size()),
             id.data());
  }

  template<size_t N>
  void formatValue(char* buffer, size_t bufferSize, const std::bitset<N>& value) {

    snprintf(buffer, bufferSize,
             "%s (%d -> 1)",
             value.to_string().c_str(), N);
  }

  template<uint16_t N>
  void formatValue(char* buffer, size_t bufferSize, const FixedString<N>& value) {

    snprintf(buffer, bufferSize,
             "\"%s\"",
             value.c_str());
  }

  void formatValue(char* buffer, size_t bufferSize, const EmptyCollection) {

    snprintf(buffer, bufferSize, "{}");
  }

  template<typename T>
  void formatValue(char* buffer, size_t bufferSize, const T& value) {

    if constexpr (std::is_same_v<T, bool>) {
      snprintf(buffer, bufferSize, "%s", value ? "true" : "false");

    } else if constexpr (std::is_integral_v<T>) {
      snprintf(buffer, bufferSize, "%ld", static_cast<long>(value));

    } else if constexpr (std::is_floating_point_v<T>) {
      snprintf(buffer, bufferSize, "%.3f", static_cast<double>(value));

    } else {
      snprintf(buffer, bufferSize, "<unsupported>");
    }
  }

  template<typename T, uint8_t N>
  TraversalResult logValue(Collection<T, N>& value, std::string_view /*unit*/) {
    //  Keep the collection non-const because Reflection::visit() requires
    // mutable elements for traversal. Values are only read when formatting.

    if (value.size() == 0) {
      auto emptyColl = EmptyCollection{};
      return logValue(emptyColl, "");
    }

    for (size_t i = 0; i < value.size(); ++i) {
      m_pathBuilder.index(i);
      if (auto result = Reflection::traverse(value[i], *this); !result)
        return result;
      m_pathBuilder.leave();
    }

    return {};
  }

  template<typename T>
  TraversalResult logValue(T& value, std::string_view unit) {

    char valueBuffer[SchemaLimits::kMaxPathLength + 30];

    formatValue(valueBuffer, sizeof(valueBuffer), value);

    log_d(
      "%s %.*s = %s %.*s",
      kTag,
      static_cast<int>(m_pathBuilder.view().size()),
      m_pathBuilder.view().data(),
      valueBuffer,
      static_cast<int>(unit.size()),
      unit.data());

    return {};
  }

  Reflection::PathBuilder m_pathBuilder;
};

template<typename T, typename Filter>
JsonDeserializer::TraversalResult jsonToConfig(
  const JsonDocument& doc, T& config, const Filter& filter) {

  JsonDeserializer deserializer(doc);
  return Reflection::visit(config, deserializer, filter, false);
}

template<typename T, typename Filter>
JsonSerializer::TraversalResult configToJson(
  const T& config, JsonDocument& doc, const Filter& filter) {

  JsonSerializer serializer(doc);
  return Reflection::visit(config, serializer, filter, false);
}

}  // namespace

void ConfigManager::begin() {
  log_d("ConfigManager is initializing...");

  if (!onScheduleUpdate) {
    log_e("onScheduleUpdate callback is not configured!");
    while (true) delay(1000);
  }

  // Mount LittleFS
  if (!LittleFS.begin()) {
    m_faults.set(
      Fault::Component::ConfigManager,
      Fault::Code::LittleFsMountingFailed);
    return;
  }

  log_d("Loading runtime config...");

  // Try the user's runtime configuration first.
  if (fileExists(kUserSettingsRuntimeFilename)
      && fileExists(kSchedulesRuntimeFilename)) {

    if (!loadAll(false)) {
      m_faults.set(
        Fault::Component::ConfigManager,
        Fault::Code::LoadFailed);
      return;
    }

    // Remove temporary files left behind by an interrupted save.
    // Best-effort cleanup; failure to remove a stale temp
    // file does not affect the active configuration.
    removeStaleTempFile(kUserSettingsRuntimeFilename);
    removeStaleTempFile(kSchedulesRuntimeFilename);

    log_d("Successfully loaded runtime config");
    return;
  }

  log_d("No runtime config found, loading defaults...");

  if (!loadAll(true)) {
    m_faults.set(
      Fault::Component::ConfigManager,
      Fault::Code::LoadFailed);
    return;
  }

  log_d("Successfully loaded default config");

  log_d("Saving default config as minified runtime config...");

  // Persist the default human-readable config as minified JSON so future
  // boots can load the lighter runtime config directly.
  if (!saveAll()) {
    m_faults.set(
      Fault::Component::ConfigManager,
      Fault::Code::SaveFailed);
    return;
  }

  log_d("Default config persisted as runtime");

  log_i("ConfigManager has initialized");
}

ConfigManager::UserSettings ConfigManager::getUserSettings() const {
  std::lock_guard lock(m_mutex);
  return m_config.userSettings;
}

ConfigManager::WifiSettings ConfigManager::getWifiSettings() const {
  std::lock_guard lock(m_mutex);
  return m_config.userSettings.params.wifi;
}

ConfigManager::WateringSettings ConfigManager::getWateringSettings() const {
  std::lock_guard lock(m_mutex);
  return m_config.userSettings.params.watering;
}

ConfigManager::LineCollection ConfigManager::getLines() const {
  std::lock_guard lock(m_mutex);
  return m_config.userSettings.wateringModel.lines;
}

ConfigManager::ZoneCollection ConfigManager::getZones() const {
  std::lock_guard lock(m_mutex);
  return m_config.userSettings.wateringModel.zones;
}

ConfigManager::ScheduleCollection ConfigManager::getSchedules() const {
  std::lock_guard lock(m_mutex);
  return m_config.schedules;
}

void ConfigManager::logDebug() {
  std::lock_guard lock(m_mutex);

  log_d("Printing config...");
  DebugPrinter printer;

  if (!Reflection::visit(m_config, printer))
    log_w("Printing stopped halfway through for no apparent reason!");
}

ConfigManager::UpdateResult ConfigManager::update(
  std::string_view userSettingsJson) {
  Config::UserSettings candidate;

  JsonDocument inputDoc;

  DeserializationError error =
    deserializeJson(inputDoc, userSettingsJson.data(), userSettingsJson.size());

  if (error) {
    FixedString<SchemaLimits::kMaxErrorMessageLength> message;
    message.format("bad JSON: %s", error.c_str());

    return {
      .error   = UpdateError::InvalidJson,
      .path    = "",
      .message = message
    };
  }

  auto deserializationResult = jsonToConfig(inputDoc, candidate,
                                            Reflection::NoFilter{});
  if (!deserializationResult)
    return {
      .error   = UpdateError::DeserializationFailed,
      .path    = deserializationResult.path(),
      .message = deserializationResult.message()
    };

  JsonDocument outputDoc;

  auto serializationResult = configToJson(candidate, outputDoc,
                                          Reflection::NoFilter{});
  if (!serializationResult)
    return {
      .error   = UpdateError::SerializationFailed,
      .path    = serializationResult.path(),
      .message = serializationResult.message()
    };

  std::lock_guard lock(m_mutex);

  // Persist while holding the lock.
  if (!saveToFile(outputDoc, kUserSettingsRuntimeFilename))
    return {
      .error   = UpdateError::SaveFailed,
      .path    = "",
      .message = "failed to save user settings"
    };

  // Commit only after persistence succeeded.
  m_config.userSettings = std::move(candidate);

  return {};
}

ConfigManager::UpdateResult ConfigManager::update(
  Config::Schedule& schedule, UpdateScheduleAction action) {
  {
    std::lock_guard lock(m_mutex);
    Config          candidate = m_config;

    switch (action) {

      case UpdateScheduleAction::Add:
        {
          auto result = candidate.schedules.add(schedule);
          switch (result) {
            case Config::ScheduleCollection::AddResult::Ok:
              {
                uint8_t totalSchedules = 0;
                for (uint8_t i = 0; i < candidate.schedules.size(); i++) {
                  if (candidate.schedules[i].lineId == schedule.lineId)
                    totalSchedules++;
                }
                if (totalSchedules > Config::kMaxSchedulePerLine)
                  return {
                    .error   = UpdateError::OperationFailed,
                    .path    = "",
                    .message = "unable to add schedule: too many schedules"
                               " already exist for that line"
                  };
                break;
              }
            case Config::ScheduleCollection::AddResult::DuplicateId:
              return {
                .error   = UpdateError::OperationFailed,
                .path    = "",
                .message = "unable to add schedule: an element"
                           " with this ID already exists"
              };
            case Config::ScheduleCollection::AddResult::Full:
              return {
                .error   = UpdateError::OperationFailed,
                .path    = "",
                .message = "unable to add schedule:"
                           " storage capacity exceeded"
              };
          }
          break;
        }

      case UpdateScheduleAction::Remove:
        if (!candidate.schedules.remove(schedule.id))
          return {
            .error   = UpdateError::OperationFailed,
            .path    = "",
            .message = "unable to remove schedule:"
                       " no corresponding element with ID"
          };
        break;

      case UpdateScheduleAction::Update:
        if (!candidate.schedules.update(schedule))
          return {
            .error   = UpdateError::OperationFailed,
            .path    = "",
            .message = "unable to update schedule:"
                       " no element with same ID exists"
          };
        break;
    }

    Validation::ValidationVisitor v;

    auto validationResult = Reflection::visit(candidate, v);
    if (!validationResult)
      return {
        .error   = UpdateError::ValidationFailed,
        .path    = validationResult.path(),
        .message = validationResult.message()
      };

    JsonDocument outputDoc;

    auto serializationResult = configToJson(candidate, outputDoc,
                                            SchedulesOnlyFilter{});
    if (!serializationResult)
      return {
        .error   = UpdateError::SerializationFailed,
        .path    = serializationResult.path(),
        .message = serializationResult.message()
      };

    if (!saveToFile(outputDoc, kSchedulesRuntimeFilename))
      return {
        .error   = UpdateError::SaveFailed,
        .path    = "",
        .message = "failed to save schedules"
      };

    m_config.schedules = std::move(candidate.schedules);
  }

  // only happens once the new state is actually committed
  onScheduleUpdate();

  return {};
}

bool ConfigManager::getFileContent(const char* filename, String& content) const {
  std::lock_guard lock(m_mutex);

  fs::File file;

  if (!openValidFile(filename, file))
    return false;

  content = file.readString();
  return true;
}

bool ConfigManager::loadAll(bool defaults) {
  // Build a temporary Config first. This prevents
  // partially modifying m_config if something is invalid.
  // Order matters: userSettings should always be loaded
  // before schedules!

  Config candidate;

  JsonDocument doc;

  if (!loadFromFile(defaults
                      ? kUserSettingsDefaultFilename
                      : kUserSettingsRuntimeFilename,
                    doc))
    return false;
  log_d("UserSettings JSON loaded");
  if (!jsonToConfig(doc, candidate.userSettings,
                    Reflection::NoFilter{}))
    return false;
  log_d("UserSettings JSON parsed");

  if (!loadFromFile(defaults
                      ? kSchedulesDefaultFilename
                      : kSchedulesRuntimeFilename,
                    doc))
    return false;
  log_d("Schedules JSON loaded");
  if (!jsonToConfig(doc, candidate,
                    SchedulesOnlyFilter{}))
    return false;
  log_d("Schedules JSON parsed");

  std::lock_guard lock(m_mutex);
  m_config = std::move(candidate);

  return true;
}

bool ConfigManager::loadFromFile(const char* filename, JsonDocument& doc) {
  fs::File file;

  if (!openValidFile(filename, file))
    return false;

  DeserializationError error = deserializeJson(doc, file);

  if (error) {
    log_d("Failed to load config from file: %s (%s)",
          filename, error.c_str());
    return false;
  }

  return true;
}

bool ConfigManager::saveAll() {
  std::lock_guard lock(m_mutex);

  Validation::ValidationVisitor v;

  if (!Reflection::visit(m_config, v)) {
    log_d("Current config is not valid");
    return false;
  }

  JsonDocument userSettingsDoc;
  JsonDocument schedulesDoc;

  if (!configToJson(m_config.userSettings, userSettingsDoc,
                    Reflection::NoFilter{}))
    return false;
  log_d("UserSettings JSON created");
  if (!configToJson(m_config, schedulesDoc,
                    SchedulesOnlyFilter{}))
    return false;
  log_d("Schedules JSON created");

  if (!saveToFile(userSettingsDoc, kUserSettingsRuntimeFilename))
    return false;
  log_d("UserSettings JSON saved");
  if (!saveToFile(schedulesDoc, kSchedulesRuntimeFilename))
    return false;
  log_d("Schedules JSON saved");

  return true;
}

bool ConfigManager::saveToFile(JsonDocument& doc, const char* filename) {
  // LittleFS is explicitly designed to be fail-safe.
  // Its documentation states that "All POSIX operations, such as
  // remove and rename, are atomic, even in the event of power-loss."
  //
  // Active file:        <filename>
  // Update in progress: <filename>.tmp

  char tempFilename[64];

  if (!makeTempFilename(filename, tempFilename, sizeof(tempFilename)))
    return false;

  // "w" gives a fresh empty file before writing the new content
  fs::File file = LittleFS.open(tempFilename, "w");

  if (!file) {
    log_d("Failed to open temp file: %s", tempFilename);
    return false;
  }

  // Avoid lots of tiny filesystem writes.
  WriteBufferingStream bufferedFile(file, 64);

  if (serializeJson(doc, bufferedFile) == 0) {
    log_d("Failed to write JSON to temp file: %s",
          tempFilename);
    file.close();
    LittleFS.remove(tempFilename);
    return false;
  }

  bufferedFile.flush();

  if (file.getWriteError()) {
    log_d("Failed to write JSON to temp file: %s",
          tempFilename);
    file.close();
    LittleFS.remove(tempFilename);
    return false;
  }

  file.close();

  // Atomically replace the active configuration.
  if (!LittleFS.rename(tempFilename, filename)) {
    log_d("Failed to replace file: %s", filename);
    LittleFS.remove(tempFilename);
    return false;
  }

  return true;
}

bool ConfigManager::openValidFile(const char* filename, fs::File& file) const {
  file = LittleFS.open(filename, "r");

  if (!file) {
    log_d("Failed to open file: %s", filename);
    return false;
  }

  const size_t fileSize = file.size();

  if (fileSize > kMaxFileSizeBytes) {
    log_d("File is too big: %zu bytes > %zu", fileSize, kMaxFileSizeBytes);
    return false;
  }

  return true;
}

bool ConfigManager::fileExists(const char* path) {
  char fullPath[256];
  snprintf(fullPath, sizeof(fullPath), "/littlefs%s", path);

  return access(fullPath, F_OK) == 0;
}

bool ConfigManager::makeTempFilename(
  const char* filename,
  char*       tempFilename,
  size_t      tempFilenameSize) const {

  const int length = snprintf(
    tempFilename,
    tempFilenameSize,
    "%s.tmp",
    filename);

  if (length < 0 || static_cast<size_t>(length) >= tempFilenameSize) {
    log_d("Temp filename is too long: %s", filename);
    return false;
  }

  return true;
}

bool ConfigManager::removeStaleTempFile(const char* filename) {
  char tempFilename[64];

  if (!makeTempFilename(filename, tempFilename, sizeof(tempFilename)))
    return false;

  if (!fileExists(tempFilename))
    return true;

  if (!LittleFS.remove(tempFilename)) {
    log_d("Failed to remove stale temp file: %s", tempFilename);
    return false;
  }

  return true;
}
