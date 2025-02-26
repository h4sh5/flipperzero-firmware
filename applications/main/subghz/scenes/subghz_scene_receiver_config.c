#include "../subghz_i.h"

#include <lib/subghz/protocols/raw.h>

enum SubGhzSettingIndex {
    SubGhzSettingIndexFrequency,
    SubGhzSettingIndexHopping,
    SubGhzSettingIndexModulation,
    SubGhzSettingIndexDetectRaw,
    SubGhzSettingIndexRSSIThreshold,
    SubGhzSettingIndexLock,
};

#define HOPPING_COUNT 2
const char* const hopping_text[HOPPING_COUNT] = {
    "OFF",
    "ON",
};
const uint32_t hopping_value[HOPPING_COUNT] = {
    SubGhzHopperStateOFF,
    SubGhzHopperStateRunnig,
};

#define DETECT_RAW_COUNT 2
const char* const detect_raw_text[DETECT_RAW_COUNT] = {
    "OFF",
    "ON",
};

const SubGhzProtocolFlag detect_raw_value[DETECT_RAW_COUNT] = {
    SubGhzProtocolFlag_Decodable,
    SubGhzProtocolFlag_Decodable | SubGhzProtocolFlag_RAW,
};

#define RSSI_THRESHOLD_COUNT 7
const char* const rssi_threshold_text[RSSI_THRESHOLD_COUNT] = {
    "-72db",
    "-67db",
    "-62db",
    "-57db",
    "-52db",
    "-47db",
    "-42db",
};

const int rssi_threshold_value[RSSI_THRESHOLD_COUNT] = {
    -72,
    -67,
    -62,
    -57,
    -52,
    -47,
    -42,
};

uint8_t subghz_scene_receiver_config_next_frequency(const uint32_t value, void* context) {
    furi_assert(context);
    SubGhz* subghz = context;
    uint8_t index = 0;
    for(uint8_t i = 0; i < subghz_setting_get_frequency_count(subghz->setting); i++) {
        if(value == subghz_setting_get_frequency(subghz->setting, i)) {
            index = i;
            break;
        } else {
            index = subghz_setting_get_frequency_default_index(subghz->setting);
        }
    }
    return index;
}

uint8_t subghz_scene_receiver_config_next_preset(const char* preset_name, void* context) {
    furi_assert(context);
    SubGhz* subghz = context;
    uint8_t index = 0;
    for(uint8_t i = 0; i < subghz_setting_get_preset_count(subghz->setting); i++) {
        if(!strcmp(subghz_setting_get_preset_name(subghz->setting, i), preset_name)) {
            index = i;
            break;
        } else {
            //  index = subghz_setting_get_frequency_default_index(subghz->setting);
        }
    }
    return index;
}

uint8_t subghz_scene_receiver_config_hopper_value_index(
    const uint32_t value,
    const uint32_t values[],
    uint8_t values_count,
    void* context) {
    furi_assert(context);
    UNUSED(values_count);
    SubGhz* subghz = context;

    if(value == values[0]) {
        return 0;
    } else {
        variable_item_set_current_value_text(
            (VariableItem*)scene_manager_get_scene_state(
                subghz->scene_manager, SubGhzSceneReceiverConfig),
            " -----");
        return 1;
    }
}

uint8_t subghz_scene_receiver_config_detect_raw_value_index(
    const SubGhzProtocolFlag value,
    const SubGhzProtocolFlag values[],
    uint8_t values_count) {
    uint8_t index = 0;
    for(uint8_t i = 0; i < values_count; i++) {
        if(value == values[i]) {
            index = i;
            break;
        }
    }
    return index;
}

uint8_t subghz_scene_receiver_config_rssi_threshold_value_index(
    const int value,
    const int values[],
    uint8_t values_count) {
    uint8_t index = 0;
    for(uint8_t i = 0; i < values_count; i++) {
        if(value == values[i]) {
            index = i;
            break;
        }
    }
    return index;
}

static void subghz_scene_receiver_config_set_frequency(VariableItem* item) {
    SubGhz* subghz = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);

    if(subghz->txrx->hopper_state == SubGhzHopperStateOFF) {
        char text_buf[10] = {0};
        snprintf(
            text_buf,
            sizeof(text_buf),
            "%lu.%02lu",
            subghz_setting_get_frequency(subghz->setting, index) / 1000000,
            (subghz_setting_get_frequency(subghz->setting, index) % 1000000) / 10000);
        variable_item_set_current_value_text(item, text_buf);
        subghz->txrx->preset->frequency = subghz_setting_get_frequency(subghz->setting, index);
        subghz->last_setting->frequency = subghz->txrx->preset->frequency;
        subghz_setting_set_default_frequency(subghz->setting, subghz->txrx->preset->frequency);
    } else {
        variable_item_set_current_value_index(
            item, subghz_setting_get_frequency_default_index(subghz->setting));
    }
}

static void subghz_scene_receiver_config_set_preset(VariableItem* item) {
    SubGhz* subghz = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    const char* preset_name = subghz_setting_get_preset_name(subghz->setting, index);
    variable_item_set_current_value_text(item, preset_name);
    string_set_str(subghz->last_setting->preset_name, preset_name);

    subghz_preset_init(
        subghz,
        preset_name,
        subghz->txrx->preset->frequency,
        subghz_setting_get_preset_data(subghz->setting, index),
        subghz_setting_get_preset_data_size(subghz->setting, index));
}

static void subghz_scene_receiver_config_set_rssi_threshold(VariableItem* item) {
    SubGhz* subghz = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, rssi_threshold_text[index]);
    subghz_protocol_decoder_raw_set_rssi_threshold(
        subghz_receiver_search_decoder_base_by_name(
            subghz->txrx->receiver, SUBGHZ_PROTOCOL_RAW_NAME),
        rssi_threshold_value[index]);
    subghz->last_setting->rssi_threshold = rssi_threshold_value[index];
}

static void subghz_scene_receiver_config_set_detect_raw(VariableItem* item) {
    SubGhz* subghz = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, detect_raw_text[index]);
    subghz_receiver_set_filter(subghz->txrx->receiver, detect_raw_value[index]);

    //subghz->last_setting->detect_raw = detect_raw_value[index];

    subghz_protocol_decoder_raw_set_auto_mode(
        subghz_receiver_search_decoder_base_by_name(
            subghz->txrx->receiver, SUBGHZ_PROTOCOL_RAW_NAME),
        (index == 1));
}

static void subghz_scene_receiver_config_set_hopping_running(VariableItem* item) {
    SubGhz* subghz = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, hopping_text[index]);
    if(hopping_value[index] == SubGhzHopperStateOFF) {
        char text_buf[10] = {0};
        snprintf(
            text_buf,
            sizeof(text_buf),
            "%lu.%02lu",
            subghz_setting_get_default_frequency(subghz->setting) / 1000000,
            (subghz_setting_get_default_frequency(subghz->setting) % 1000000) / 10000);
        variable_item_set_current_value_text(
            (VariableItem*)scene_manager_get_scene_state(
                subghz->scene_manager, SubGhzSceneReceiverConfig),
            text_buf);
        subghz->txrx->preset->frequency = subghz_setting_get_default_frequency(subghz->setting);
        variable_item_set_current_value_index(
            (VariableItem*)scene_manager_get_scene_state(
                subghz->scene_manager, SubGhzSceneReceiverConfig),
            subghz_setting_get_frequency_default_index(subghz->setting));
    } else {
        variable_item_set_current_value_text(
            (VariableItem*)scene_manager_get_scene_state(
                subghz->scene_manager, SubGhzSceneReceiverConfig),
            " -----");
        variable_item_set_current_value_index(
            (VariableItem*)scene_manager_get_scene_state(
                subghz->scene_manager, SubGhzSceneReceiverConfig),
            subghz_setting_get_frequency_default_index(subghz->setting));
    }

    subghz->txrx->hopper_state = subghz->last_setting->hopping = hopping_value[index];
}

static void subghz_scene_receiver_config_var_list_enter_callback(void* context, uint32_t index) {
    furi_assert(context);
    SubGhz* subghz = context;
    if(index == SubGhzSettingIndexLock) {
        view_dispatcher_send_custom_event(
            subghz->view_dispatcher, SubGhzCustomEventSceneSettingLock);
    }
}

void subghz_scene_receiver_config_on_enter(void* context) {
    SubGhz* subghz = context;
    VariableItem* item;
    uint8_t value_index;

    item = variable_item_list_add(
        subghz->variable_item_list,
        "Frequency:",
        subghz_setting_get_frequency_count(subghz->setting),
        subghz_scene_receiver_config_set_frequency,
        subghz);
    value_index =
        subghz_scene_receiver_config_next_frequency(subghz->txrx->preset->frequency, subghz);
    scene_manager_set_scene_state(
        subghz->scene_manager, SubGhzSceneReceiverConfig, (uint32_t)item);
    variable_item_set_current_value_index(item, value_index);
    char text_buf[10] = {0};
    snprintf(
        text_buf,
        sizeof(text_buf),
        "%lu.%02lu",
        subghz_setting_get_frequency(subghz->setting, value_index) / 1000000,
        (subghz_setting_get_frequency(subghz->setting, value_index) % 1000000) / 10000);
    variable_item_set_current_value_text(item, text_buf);

    if(scene_manager_get_scene_state(subghz->scene_manager, SubGhzSceneReadRAW) !=
       SubGhzCustomEventManagerSet) {
        item = variable_item_list_add(
            subghz->variable_item_list,
            "Hopping:",
            HOPPING_COUNT,
            subghz_scene_receiver_config_set_hopping_running,
            subghz);
        value_index = subghz_scene_receiver_config_hopper_value_index(
            subghz->txrx->hopper_state, hopping_value, HOPPING_COUNT, subghz);
        variable_item_set_current_value_index(item, value_index);
        variable_item_set_current_value_text(item, hopping_text[value_index]);
    }

    item = variable_item_list_add(
        subghz->variable_item_list,
        "Modulation:",
        subghz_setting_get_preset_count(subghz->setting),
        subghz_scene_receiver_config_set_preset,
        subghz);
    value_index = subghz_scene_receiver_config_next_preset(
        string_get_cstr(subghz->txrx->preset->name), subghz);
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(
        item, subghz_setting_get_preset_name(subghz->setting, value_index));

    if(scene_manager_get_scene_state(subghz->scene_manager, SubGhzSceneReadRAW) !=
       SubGhzCustomEventManagerSet) {
        item = variable_item_list_add(
            subghz->variable_item_list,
            "Detect Raw:",
            DETECT_RAW_COUNT,
            subghz_scene_receiver_config_set_detect_raw,
            subghz);
        value_index = subghz_scene_receiver_config_detect_raw_value_index(
            subghz_receiver_get_filter(subghz->txrx->receiver),
            detect_raw_value,
            DETECT_RAW_COUNT);
        variable_item_set_current_value_index(item, value_index);
        variable_item_set_current_value_text(item, detect_raw_text[value_index]);
    }

    if(scene_manager_get_scene_state(subghz->scene_manager, SubGhzSceneReadRAW) !=
       SubGhzCustomEventManagerSet) {
        item = variable_item_list_add(
            subghz->variable_item_list,
            "RSSI for Raw:",
            RSSI_THRESHOLD_COUNT,
            subghz_scene_receiver_config_set_rssi_threshold,
            subghz);
        value_index = subghz_scene_receiver_config_rssi_threshold_value_index(
            subghz_protocol_encoder_get_rssi_threshold(subghz_receiver_search_decoder_base_by_name(
                subghz->txrx->receiver, SUBGHZ_PROTOCOL_RAW_NAME)),
            rssi_threshold_value,
            RSSI_THRESHOLD_COUNT);
        variable_item_set_current_value_index(item, value_index);
        variable_item_set_current_value_text(item, rssi_threshold_text[value_index]);
    }

    if(scene_manager_get_scene_state(subghz->scene_manager, SubGhzSceneReadRAW) !=
       SubGhzCustomEventManagerSet) {
        variable_item_list_add(subghz->variable_item_list, "Lock Keyboard", 1, NULL, NULL);
        variable_item_list_set_enter_callback(
            subghz->variable_item_list,
            subghz_scene_receiver_config_var_list_enter_callback,
            subghz);
    }

    view_dispatcher_switch_to_view(subghz->view_dispatcher, SubGhzViewIdVariableItemList);
}

bool subghz_scene_receiver_config_on_event(void* context, SceneManagerEvent event) {
    SubGhz* subghz = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SubGhzCustomEventSceneSettingLock) {
            subghz->lock = SubGhzLockOn;
            scene_manager_previous_scene(subghz->scene_manager);
            consumed = true;
        }
    }
    return consumed;
}

void subghz_scene_receiver_config_on_exit(void* context) {
    SubGhz* subghz = context;
    variable_item_list_reset(subghz->variable_item_list);
    scene_manager_set_scene_state(
        subghz->scene_manager, SubGhzSceneReadRAW, SubGhzCustomEventManagerNoSet);
}
