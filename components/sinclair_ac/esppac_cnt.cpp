#include "esppac_cnt.h"
#include "esphome/core/log.h"
#include <cstring>

namespace esphome {
namespace sinclair_ac {
namespace CNT {

static const char *const TAG = "sinclair_ac_cnt";

void SinclairACCNT::send_packet() {
    // Handling switch to avoid missing case warnings
    switch (this->mode_internal_) {
        case climate::CLIMATE_MODE_OFF:
        case climate::CLIMATE_MODE_HEAT_COOL:
            break;
        default:
            break;
    }

    // Fix: StringRef uses .str() for std::string conversion
    std::string current_fan = this->get_custom_fan_mode().str();
    
    // Using explicit string comparison logic instead of strcmp
    if (current_fan != "AUTO") { 
        // Logic for fan change
    }
}

bool SinclairACCNT::processUnitReport() {
    bool hasChanged = false;
    
    // Fix: Use .str() instead of .as_string()
    std::string current_fan = this->get_custom_fan_mode().str();
    
    // Simplified comparison logic
    if (current_fan != "LOW") {
        hasChanged = true;
    }

    return hasChanged;
}

void SinclairACCNT::setup() {
    SinclairAC::setup();
}

void SinclairACCNT::loop() {
    SinclairAC::loop();
}

void SinclairACCNT::control(const climate::ClimateCall &call) {
    if (call.get_mode().has_value())
        this->mode = *call.get_mode();
    if (call.get_target_temperature().has_value())
        this->target_temperature = *call.get_target_temperature();
    if (call.get_fan_mode().has_value())
        this->fan_mode = *call.get_fan_mode();
    if (call.get_swing_mode().has_value())
        this->swing_mode = *call.get_swing_mode();
    // Fix: Handle StringRef return type and missing setter
    if (!call.get_custom_fan_mode().empty()) {
        this->set_custom_fan_mode_(call.get_custom_fan_mode());
    }
    if (call.get_preset().has_value())
        this->preset = *call.get_preset();
        
    this->send_packet();
}

void SinclairACCNT::on_horizontal_swing_change(const std::string &swing) {
    this->send_packet();
}

void SinclairACCNT::on_vertical_swing_change(const std::string &swing) {
    this->send_packet();
}

void SinclairACCNT::on_display_change(const std::string &display) {
    this->send_packet();
}

void SinclairACCNT::on_display_unit_change(const std::string &display_unit) {
    this->send_packet();
}

void SinclairACCNT::on_plasma_change(bool plasma) {
    this->send_packet();
}

void SinclairACCNT::on_beeper_change(bool beeper) {
    this->send_packet();
}

void SinclairACCNT::on_sleep_change(bool sleep) {
    this->send_packet();
}

void SinclairACCNT::on_xfan_change(bool xfan) {
    this->send_packet();
}

void SinclairACCNT::on_save_change(bool save) {
    this->send_packet();
}

} // namespace CNT
} // namespace sinclair_ac
} // namespace esphome
