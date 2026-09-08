#pragma once

#include <cstdint>

namespace Pinout {

namespace ESP {

namespace I2C {  // RTC + IOExpander
constexpr uint8_t SDA = 21;
constexpr uint8_t SCL = 22;
}  // namespace I2C

namespace Sensor {
namespace WaterTank {
// 0-5V digital output scaled to 0-3V (active HIGH)
constexpr uint8_t LowLevel = 16;
// 0-5V digital output scaled to 0-3V (active LOW)
constexpr uint8_t HighLevel = 17;
// 0-4.3V digital output scaled to 0-3V (active HIGH)
constexpr uint8_t Filling = 35;
}  // namespace WaterTank
// Active analog voltage output
constexpr uint8_t SoilMoisture = 34;
// Push-pull digital output
constexpr uint8_t Flow = 39;
// AC analog signal, 1.65V biased, ±1V around bias
constexpr uint8_t PumpCurrent = 36;
}  // namespace Sensor

namespace HandWatering {
// 12V DC, low-side MOSFET switched, active HIGH
constexpr uint8_t Valve = 13;
// 12V DC, low-side MOSFET switched, active HIGH
constexpr uint8_t ButtonLed = 14;
// Digital input
constexpr uint8_t ButtonInput = 23;
// TM1637 display clock
constexpr uint8_t DisplayClk = 18;
// TM1637 display data
constexpr uint8_t DisplayDio = 19;
}  // namespace HandWatering

namespace BoxLed {
// Normally controlled by ESP32; future external watchdog
// will cut power to it in the absence of the heartbeat
constexpr uint8_t Status   = 25;
constexpr uint8_t Fault    = 27;
constexpr uint8_t Watering = 26;
}  // namespace BoxLed

constexpr uint8_t RadioTX = 32;

namespace Unused {
constexpr uint8_t GPIO33 = 33;
}  // namespace Unused

}  // namespace ESP

namespace Expander {

constexpr uint8_t Relay1 = 3;
constexpr uint8_t Relay2 = 2;
constexpr uint8_t Relay3 = 1;
constexpr uint8_t Relay4 = 0;
constexpr uint8_t Relay5 = 7;
constexpr uint8_t Relay6 = 6;
constexpr uint8_t Relay7 = 5;
constexpr uint8_t Relay8 = 4;

}  // namespace Expander

}  // namespace Pinout
