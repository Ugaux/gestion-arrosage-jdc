#pragma once

#include <mutex>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <ArduinoJson.h>
#include "Constants.h"
#include "types/FixedString.h"
#include "config/generated/Config.h"

// forward declaration
namespace fs {
class File;
}  // namespace fs

class FaultManager;

// Configuration persistence is designed to be resilient to interrupted writes.
//
// Runtime files are updated atomically through temporary files, so an interrupted
// save cannot leave an active file partially written. During initial setup,
// both runtime files must exist before they are used; if creating them is
// interrupted, the next boot falls back to the defaults and retries.
//
// After initialization, userSettings and schedules are persisted independently,
// A user action modifies and persists only one of the two files at a time,
// so changing one never rewrites the other.
//
// Loading is also performed into a candidate configuration first. The active
// configuration is replaced only after the candidate has been successfully
// loaded and validated.
class ConfigManager {
public:
  static constexpr char kUserSettingsDefaultFilename[] = "/config/userSettings.json";
  static constexpr char kUserSettingsRuntimeFilename[] = "/config/userSettings_r.json";

  static constexpr char kSchedulesDefaultFilename[] = "/config/schedules.json";
  static constexpr char kSchedulesRuntimeFilename[] = "/config/schedules_r.json";

  static constexpr size_t kMaxFileSizeBytes = 2048;

  using UserSettings       = Config::UserSettings;
  using WifiSettings       = UserSettings::Params::Wifi;
  using WateringSettings   = UserSettings::Params::Watering;
  using LineCollection     = UserSettings::WateringModel::LineCollection;
  using ZoneCollection     = UserSettings::WateringModel::ZoneCollection;
  using ScheduleCollection = Config::ScheduleCollection;

  enum class UpdateScheduleAction : u_int8_t {
    Add = 0,
    Remove,
    Update
  };

  enum class UpdateError : u_int8_t {
    None = 0,
    InvalidJson,
    OperationFailed,
    DeserializationFailed,
    ValidationFailed,
    SerializationFailed,
    SaveFailed,
  };

  struct UpdateResult {
    bool success() const {
      return error == UpdateError::None;
    }

    UpdateError error = UpdateError::None;

    FixedString<SchemaLimits::kMaxPathLength>         path;
    FixedString<SchemaLimits::kMaxErrorMessageLength> message;
  };

  ConfigManager(FaultManager& faults) : m_faults(faults) {}

  ConfigManager(const ConfigManager&)            = delete;
  ConfigManager& operator=(const ConfigManager&) = delete;

  std::function<void()> onScheduleUpdate;

  void begin();
  void logDebug();

  const Config& config() const { return m_config; };

  UserSettings     getUserSettings() const;
  WifiSettings     getWifiSettings() const;
  WateringSettings getWateringSettings() const;

  LineCollection     getLines() const;
  ZoneCollection     getZones() const;
  ScheduleCollection getSchedules() const;

  // Updates the user settings from JSON.
  // The input is parsed and converted into a candidate configuration, with
  // validation performed during the conversion. The validated candidate is
  // serialized to the canonical minified JSON representation and persisted
  // before replacing the current configuration, so a failed update leaves
  // the active configuration unchanged.
  // The result reports the type, location, and message of any failure.
  UpdateResult update(std::string_view userSettingsJson);
  // Updates the schedules by applying the requested action to a candidate
  // configuration. The candidate is validated and serialized to the canonical
  // minified JSON representation, then persisted before replacing the current
  // schedules, so a failed update leaves the active configuration unchanged.
  // The result reports the type, location, and message of any failure.
  UpdateResult update(Config::Schedule& schedule, UpdateScheduleAction action);

  // Gets the content of the persisted minified JSON file.
  bool getFileContent(const char* filename, String& content) const;

private:
  bool loadAll(bool defaults);
  bool loadFromFile(const char* filename, JsonDocument& doc);

  bool saveAll();
  // Saves the configuration to a JSON file.
  // The JSON is always serialized in minified form.
  bool saveToFile(JsonDocument& doc, const char* filename);

  bool openValidFile(const char* filename, fs::File& file) const;
  bool fileExists(const char* path);

  bool makeTempFilename(
    const char* filename,
    char*       tempFilename,
    size_t      tempFilenameSize) const;
  bool removeStaleTempFile(const char* filename);

  FaultManager& m_faults;

  Config m_config{};

  mutable std::mutex m_mutex;
};
