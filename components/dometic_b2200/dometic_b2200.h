#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/switch/switch.h"

namespace esphome {
namespace dometic_b2200 {

static const char *const TAG = "dometic_b2200";

class DometicB2200Climate;

enum DometicSwitchType : uint8_t { LIGHT = 0, SLEEP = 1, I_FEEL = 2 };

class DometicB2200Switch : public switch_::Switch, public Component {
 public:
  void set_parent(DometicB2200Climate *parent) { this->parent_ = parent; }
  void set_type(DometicSwitchType type) { this->type_ = type; }
  DometicSwitchType get_type() const { return this->type_; }
  void dump_config() override { LOG_SWITCH("  ", "Dometic B2200 option", this); }

 protected:
  void write_state(bool state) override;
  DometicB2200Climate *parent_{nullptr};
  DometicSwitchType type_{LIGHT};
};

class DometicB2200Climate : public climate::Climate, public Component {
 public:
  void set_transmitter(remote_transmitter::RemoteTransmitterComponent *transmitter) {
    this->transmitter_ = transmitter;
  }
  void set_temperature_sensor(sensor::Sensor *sensor) { this->temperature_sensor_ = sensor; }
  void set_tail_byte_1(uint8_t value) { this->tail_byte_1_ = value; }
  void set_tail_byte_2(uint8_t value) { this->tail_byte_2_ = value; }
  void register_switch(DometicB2200Switch *sw) { this->switches_.push_back(sw); }

  void setup() override {
    this->set_supported_custom_fan_modes({"Stufe 1", "Stufe 2", "Stufe 3", "Stufe 4"});
    this->mode = climate::CLIMATE_MODE_OFF;
    this->target_temperature = 22.0f;
    this->set_custom_fan_mode_("Stufe 1");
    this->publish_option_states_();
    this->publish_state();
  }

  void dump_config() override {
    ESP_LOGCONFIG(TAG, "Dometic B2200 Climate:");
    LOG_CLIMATE("  ", "Dometic B2200", this);
    ESP_LOGCONFIG(TAG, "  Tail bytes: 0x%02X 0x%02X", this->tail_byte_1_, this->tail_byte_2_);
    ESP_LOGCONFIG(TAG, "  Light: %s", ONOFF(this->light_));
    ESP_LOGCONFIG(TAG, "  Sleep: %s", ONOFF(this->sleep_));
    ESP_LOGCONFIG(TAG, "  I Feel: %s", ONOFF(this->i_feel_));
  }

  climate::ClimateTraits traits() override {
    auto traits = climate::ClimateTraits();
    traits.set_supported_modes({
        climate::CLIMATE_MODE_OFF,
        climate::CLIMATE_MODE_AUTO,
        climate::CLIMATE_MODE_COOL,
        climate::CLIMATE_MODE_HEAT,
        climate::CLIMATE_MODE_DRY,
        climate::CLIMATE_MODE_FAN_ONLY,
    });
    traits.set_visual_min_temperature(16.0f);
    traits.set_visual_max_temperature(30.0f);
    traits.set_visual_temperature_step(1.0f);
    return traits;
  }

  void loop() override {
    if (this->temperature_sensor_ == nullptr || !this->temperature_sensor_->has_state())
      return;

    const float value = this->temperature_sensor_->state;
    if (std::isnan(value))
      return;

    const bool changed = std::isnan(this->current_temperature) || std::fabs(this->current_temperature - value) >= 0.1f;
    this->current_temperature = value;

    const int rounded = static_cast<int>(std::lround(value));
    if (this->i_feel_ && rounded != this->last_i_feel_temperature_) {
      this->last_i_feel_temperature_ = rounded;
      this->transmit_state_();
    }
    if (changed)
      this->publish_state();
  }

  void set_option(DometicSwitchType type, bool enabled) {
    switch (type) {
      case LIGHT:
        this->light_ = enabled;
        break;
      case SLEEP:
        this->sleep_ = enabled;
        break;
      case I_FEEL:
        this->i_feel_ = enabled;
        break;
    }
    this->transmit_state_();
    this->publish_option_states_();
  }

  bool get_option(DometicSwitchType type) const {
    switch (type) {
      case LIGHT:
        return this->light_;
      case SLEEP:
        return this->sleep_;
      case I_FEEL:
        return this->i_feel_;
    }
    return false;
  }

 protected:
  void control(const climate::ClimateCall &call) override {
    if (call.get_mode().has_value())
      this->mode = *call.get_mode();

    if (call.get_target_temperature().has_value()) {
      float temperature = *call.get_target_temperature();
      temperature = std::max(16.0f, std::min(30.0f, temperature));
      this->target_temperature = std::round(temperature);
    }

    if (call.has_custom_fan_mode()) {
      const auto custom = call.get_custom_fan_mode();
      this->set_custom_fan_mode_(custom);
    }

    this->transmit_state_();
    this->publish_state();
  }

  uint8_t fan_step_() const {
    if (!this->has_custom_fan_mode())
      return 1;
    const auto mode = this->get_custom_fan_mode();
    if (mode == "Stufe 2") return 2;
    if (mode == "Stufe 3") return 3;
    if (mode == "Stufe 4") return 4;
    return 1;
  }

  uint8_t encode_command_() const {
    const int target = static_cast<int>(std::lround(this->target_temperature));
    const int clamped = std::max(16, std::min(30, target));

    switch (this->mode) {
      case climate::CLIMATE_MODE_HEAT:
        return static_cast<uint8_t>(((clamped - 16) << 4) | 0x02);
      case climate::CLIMATE_MODE_FAN_ONLY:
        return static_cast<uint8_t>(0x63 + (this->fan_step_() - 1) * 4);
      case climate::CLIMATE_MODE_AUTO:
        return 0x40;
      case climate::CLIMATE_MODE_DRY:
        return 0x30;
      case climate::CLIMATE_MODE_COOL:
      case climate::CLIMATE_MODE_OFF:
      default:
        return static_cast<uint8_t>(((clamped - 16) << 4) | 0x09);
    }
  }

  uint8_t encode_flags_() const {
    uint8_t flags = 0x87;
    if (this->mode == climate::CLIMATE_MODE_OFF)
      flags &= static_cast<uint8_t>(~0x80);
    if (this->light_)
      flags |= 0x40;
    if (this->sleep_)
      flags |= 0x08;
    if (this->i_feel_)
      flags &= static_cast<uint8_t>(~0x01);
    if (this->mode == climate::CLIMATE_MODE_DRY)
      flags &= static_cast<uint8_t>(~0x02);
    return flags;
  }

  uint8_t encode_i_feel_temperature_() const {
    float value = this->target_temperature;
    if (this->temperature_sensor_ != nullptr && this->temperature_sensor_->has_state() &&
        !std::isnan(this->temperature_sensor_->state)) {
      value = this->temperature_sensor_->state;
    }
    const int temperature = std::max(16, std::min(30, static_cast<int>(std::lround(value))));
    return static_cast<uint8_t>(((temperature - 16) << 4) | 0x09);
  }

  void transmit_state_() {
    if (this->transmitter_ == nullptr) {
      ESP_LOGW(TAG, "No remote transmitter configured");
      return;
    }

    uint8_t bytes[8];
    bytes[0] = this->encode_flags_();
    bytes[1] = static_cast<uint8_t>(~bytes[0]);
    bytes[2] = this->i_feel_ ? this->encode_i_feel_temperature_() : this->encode_command_();
    bytes[3] = static_cast<uint8_t>(~bytes[2]);
    bytes[4] = this->tail_byte_1_;
    bytes[5] = static_cast<uint8_t>(~bytes[4]);
    bytes[6] = this->tail_byte_2_;
    bytes[7] = static_cast<uint8_t>(~bytes[6]);

    ESP_LOGD(TAG, "Sending: %02X %02X %02X %02X %02X %02X %02X %02X", bytes[0], bytes[1], bytes[2],
             bytes[3], bytes[4], bytes[5], bytes[6], bytes[7]);

    auto transmit = this->transmitter_->transmit();
    auto *raw = transmit.get_data();
    raw->set_carrier_frequency(38000);
    raw->mark(6250);
    raw->space(7200);

    for (uint8_t byte : bytes) {
      for (int bit = 7; bit >= 0; bit--) {
        raw->mark(630);
        raw->space((byte & (1U << bit)) ? 3130 : 1250);
      }
    }

    raw->mark(630);
    raw->space(7250);
    raw->mark(630);
    transmit.perform();
  }

  void publish_option_states_() {
    for (auto *sw : this->switches_)
      sw->publish_state(this->get_option(sw->get_type()));
  }

  friend class DometicB2200Switch;
  remote_transmitter::RemoteTransmitterComponent *transmitter_{nullptr};
  sensor::Sensor *temperature_sensor_{nullptr};
  std::vector<DometicB2200Switch *> switches_;
  bool light_{false};
  bool sleep_{false};
  bool i_feel_{false};
  int last_i_feel_temperature_{-1000};
  uint8_t tail_byte_1_{0x3B};
  uint8_t tail_byte_2_{0x53};
};

inline void DometicB2200Switch::write_state(bool state) {
  if (this->parent_ == nullptr) {
    ESP_LOGW(TAG, "Option switch has no parent climate component");
    return;
  }
  this->parent_->set_option(this->type_, state);
}

}  // namespace dometic_b2200
}  // namespace esphome
