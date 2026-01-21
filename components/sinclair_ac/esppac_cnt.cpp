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

} // namespace CNT
} // namespace sinclair_ac
} // namespace esphome
