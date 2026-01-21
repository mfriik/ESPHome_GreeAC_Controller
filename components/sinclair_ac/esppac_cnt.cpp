#include "esppac_cnt.h"
#include "esphome/core/log.h"
#include <cstring>

namespace esphome {
namespace sinclair_ac {
namespace CNT {

static const char *const TAG = "sinclair_ac_cnt";

void SinclairACCNT::send_packet() {
    uint8_t packet[47];
    memset(packet, 0, sizeof(packet));

    packet[0] = 0x7E;
    packet[1] = 0x7E;
    packet[2] = 45; // Length
    packet[3] = protocol::CMD_OUT_PARAMS_SET; // 0x01

    // Set Power
    if (this->mode != climate::CLIMATE_MODE_OFF) {
        packet[4 + protocol::REPORT_PWR_BYTE] |= protocol::REPORT_PWR_MASK;
    }

    // Set Mode
    uint8_t mode_byte = protocol::REPORT_MODE_AUTO;
    switch (this->mode) {
        case climate::CLIMATE_MODE_COOL: mode_byte = protocol::REPORT_MODE_COOL; break;
        case climate::CLIMATE_MODE_DRY: mode_byte = protocol::REPORT_MODE_DRY; break;
        case climate::CLIMATE_MODE_HEAT: mode_byte = protocol::REPORT_MODE_HEAT; break;
        case climate::CLIMATE_MODE_FAN_ONLY: mode_byte = protocol::REPORT_MODE_FAN; break;
        case climate::CLIMATE_MODE_AUTO: default: mode_byte = protocol::REPORT_MODE_AUTO; break;
    }
    packet[4 + protocol::REPORT_MODE_BYTE] |= (mode_byte << protocol::REPORT_MODE_POS);

    // Set Temperature
    int temp = (int)this->target_temperature;
    packet[4 + protocol::REPORT_TEMP_SET_BYTE] |= ((temp + protocol::REPORT_TEMP_SET_OFF) << protocol::REPORT_TEMP_SET_POS);

    // Set Fan
    std::string current_fan = this->get_custom_fan_mode().str();
    uint8_t fan_byte = 0;
    if (current_fan == "1 - Low") fan_byte = 1;
    else if (current_fan == "2 - Medium") fan_byte = 2;
    else if (current_fan == "3 - High") fan_byte = 3;
    
    if (current_fan == "0 - Auto" || current_fan.empty()) {
        // Auto fan logic if needed, usually 0
    } else if (current_fan == "4 - Turbo") {
        packet[4 + protocol::REPORT_FAN_TURBO_BYTE] |= protocol::REPORT_FAN_TURBO_MASK;
        fan_byte = 3; // Turbo often implies high fan speed + turbo bit
    }
    packet[4 + protocol::REPORT_FAN_SPD1_BYTE] |= (fan_byte & protocol::REPORT_FAN_SPD1_MASK);

    // Set Swing
    if (this->swing_mode == climate::CLIMATE_SWING_VERTICAL || this->swing_mode == climate::CLIMATE_SWING_BOTH) {
        packet[4 + protocol::REPORT_VSWING_BYTE] |= (protocol::REPORT_VSWING_FULL << protocol::REPORT_VSWING_POS);
    }
    if (this->swing_mode == climate::CLIMATE_SWING_HORIZONTAL || this->swing_mode == climate::CLIMATE_SWING_BOTH) {
        packet[4 + protocol::REPORT_HSWING_BYTE] |= (protocol::REPORT_HSWING_FULL << protocol::REPORT_HSWING_POS);
    }

    // Calculate Checksum
    uint8_t checksum = 0;
    for (int i = 2; i < 46; i++) {
        checksum += packet[i];
    }
    packet[46] = checksum;

    this->write_array(packet, 47);
    this->log_packet(std::vector<uint8_t>(packet, packet + 47), true);
}

bool SinclairACCNT::processUnitReport() {
    if (this->serialProcess_.data.size() < 47) return false;

    // Verify Checksum
    uint8_t checksum = 0;
    for (size_t i = 2; i < this->serialProcess_.data.size() - 1; i++) {
        checksum += this->serialProcess_.data[i];
    }
    if (checksum != this->serialProcess_.data.back()) {
        ESP_LOGW(TAG, "Checksum mismatch");
        return false;
    }

    // Log received packet
    this->log_packet(this->serialProcess_.data, false);

    // Parse Power
    bool power = (this->serialProcess_.data[4 + protocol::REPORT_PWR_BYTE] & protocol::REPORT_PWR_MASK);
    this->mode = power ? climate::CLIMATE_MODE_COOL : climate::CLIMATE_MODE_OFF; // Simplified, needs full mode mapping

    // Parse Mode if powered on
    if (power) {
        uint8_t mode_byte = (this->serialProcess_.data[4 + protocol::REPORT_MODE_BYTE] & protocol::REPORT_MODE_MASK) >> protocol::REPORT_MODE_POS;
        switch (mode_byte) {
            case protocol::REPORT_MODE_COOL: this->mode = climate::CLIMATE_MODE_COOL; break;
            case protocol::REPORT_MODE_HEAT: this->mode = climate::CLIMATE_MODE_HEAT; break;
            case protocol::REPORT_MODE_DRY: this->mode = climate::CLIMATE_MODE_DRY; break;
            case protocol::REPORT_MODE_FAN: this->mode = climate::CLIMATE_MODE_FAN_ONLY; break;
            case protocol::REPORT_MODE_AUTO: this->mode = climate::CLIMATE_MODE_AUTO; break;
        }
    }

    // Parse Temperature
    uint8_t temp_raw = (this->serialProcess_.data[4 + protocol::REPORT_TEMP_SET_BYTE] & protocol::REPORT_TEMP_SET_MASK) >> protocol::REPORT_TEMP_SET_POS;
    this->target_temperature = temp_raw - protocol::REPORT_TEMP_SET_OFF;

    // Parse Current Temperature
    uint8_t curr_temp_raw = this->serialProcess_.data[4 + protocol::REPORT_TEMP_ACT_BYTE];
    this->current_temperature = (curr_temp_raw - protocol::REPORT_TEMP_ACT_OFF) / protocol::REPORT_TEMP_ACT_DIV;

    this->publish_state();
    return true;
}

void SinclairACCNT::setup() {
    SinclairAC::setup();
}

void SinclairACCNT::loop() {
    SinclairAC::loop();
    if (this->serialProcess_.state == STATE_COMPLETE) {
        if (this->serialProcess_.data.size() > 0 && this->serialProcess_.data[3] == protocol::CMD_IN_UNIT_REPORT) {
            this->processUnitReport();
        }
        this->serialProcess_.state = STATE_WAIT_SYNC;
        this->serialProcess_.data.clear();
    }
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
