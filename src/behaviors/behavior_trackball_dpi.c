// SPDX-License-Identifier: MIT

#define DT_DRV_COMPAT zmk_behavior_trackball_dpi

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>

#include <dt-bindings/zmk/trackball_dpi.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_trackball_dpi_config {
    const struct device *sensor;
    const uint16_t *steps;
    uint8_t steps_len;
    uint8_t default_index;
    bool wrap;
    uint32_t attr;
    bool set_on_init;
};

struct behavior_trackball_dpi_data {
    struct k_mutex lock;
    uint8_t current_index;
    bool current_valid;
};

static int apply_cpi_index(const struct device *dev, const struct behavior_trackball_dpi_config *cfg,
                           struct behavior_trackball_dpi_data *data, uint8_t index) {
    if (index >= cfg->steps_len) {
        return -EINVAL;
    }

    struct sensor_value val = {
        .val1 = cfg->steps[index],
        .val2 = 0,
    };

    int err = sensor_attr_set(cfg->sensor, SENSOR_CHAN_ALL, (enum sensor_attribute)cfg->attr, &val);
    if (err) {
        LOG_ERR("Failed to set CPI %u on %s (err %d)", cfg->steps[index], cfg->sensor->name, err);
        return err;
    }

    data->current_index = index;
    data->current_valid = true;
    LOG_INF("Trackball %s CPI set to %u (index %u)", cfg->sensor->name, cfg->steps[index], index);
    return 0;
}

static inline uint8_t clamp_index(const struct behavior_trackball_dpi_config *cfg, int32_t index) {
    if (index < 0) {
        return 0;
    }
    if (index >= cfg->steps_len) {
        return cfg->steps_len - 1;
    }
    return (uint8_t)index;
}

static inline uint8_t wrap_index(const struct behavior_trackball_dpi_config *cfg, int32_t index) {
    int32_t len = cfg->steps_len;
    if (len == 0) {
        return 0;
    }

    int32_t wrapped = index % len;
    if (wrapped < 0) {
        wrapped += len;
    }
    return (uint8_t)wrapped;
}

static int behavior_trackball_dpi_press(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    if (dev == NULL) {
        return -ENODEV;
    }

    const struct behavior_trackball_dpi_config *cfg = dev->config;
    struct behavior_trackball_dpi_data *data = dev->data;

    if (!device_is_ready(cfg->sensor)) {
        LOG_WRN("Sensor %s not ready", cfg->sensor->name);
        return -ENODEV;
    }

    k_mutex_lock(&data->lock, K_FOREVER);

    uint8_t active_index = data->current_valid ? data->current_index : cfg->default_index;
    int32_t target_index = active_index;
    const uint32_t command = binding->param1;
    const int32_t value = binding->param2;

    switch (command) {
    case ZMK_TRACKBALL_DPI_CMD_STEP_UP: {
        int32_t delta = value > 0 ? value : 1;
        target_index = cfg->wrap ? wrap_index(cfg, active_index + delta)
                                 : clamp_index(cfg, active_index + delta);
        break;
    }
    case ZMK_TRACKBALL_DPI_CMD_STEP_DOWN: {
        int32_t delta = value > 0 ? value : 1;
        target_index = cfg->wrap ? wrap_index(cfg, active_index - delta)
                                 : clamp_index(cfg, active_index - delta);
        break;
    }
    case ZMK_TRACKBALL_DPI_CMD_SET_INDEX:
        target_index = cfg->wrap ? wrap_index(cfg, value) : clamp_index(cfg, value);
        break;
    case ZMK_TRACKBALL_DPI_CMD_RESET:
        target_index = cfg->default_index;
        break;
    default:
        LOG_ERR("Unsupported DPI command %u", command);
        k_mutex_unlock(&data->lock);
        return -ENOTSUP;
    }

    int err = 0;
    if (!data->current_valid || target_index != data->current_index) {
        err = apply_cpi_index(dev, cfg, data, target_index);
    }

    k_mutex_unlock(&data->lock);
    return err;
}

static int behavior_trackball_dpi_init(const struct device *dev) {
    const struct behavior_trackball_dpi_config *cfg = dev->config;
    struct behavior_trackball_dpi_data *data = dev->data;

    k_mutex_init(&data->lock);
    data->current_index = cfg->default_index;
    data->current_valid = false;

    if (!device_is_ready(cfg->sensor)) {
        LOG_WRN("Sensor %s is not ready at boot", cfg->sensor->name);
        return 0;
    }

    if (cfg->set_on_init) {
        int err = apply_cpi_index(dev, cfg, data, cfg->default_index);
        if (err) {
            return err;
        }
    }

    return 0;
}

static const struct behavior_driver_api behavior_trackball_dpi_driver_api = {
    .binding_pressed = behavior_trackball_dpi_press,
};

#define TRACKBALL_DPI_STEPS_LEN(n) DT_INST_PROP_LEN(n, steps)

#define TRACKBALL_DPI_CONFIG_INIT(n)                                                              \
    BUILD_ASSERT(TRACKBALL_DPI_STEPS_LEN(n) > 0, "At least one CPI step is required");            \
    static const uint16_t dpi_steps_##n[] = DT_INST_PROP(n, steps);                               \
    static const struct behavior_trackball_dpi_config behavior_trackball_dpi_config_##n = {       \
        .sensor = DEVICE_DT_GET(DT_INST_PHANDLE(n, sensor)),                                      \
        .steps = dpi_steps_##n,                                                                   \
        .steps_len = TRACKBALL_DPI_STEPS_LEN(n),                                                  \
        .default_index = MIN(DT_INST_PROP_OR(n, default_step_index, 0),                           \
                             TRACKBALL_DPI_STEPS_LEN(n) - 1),                                     \
        .wrap = DT_INST_PROP_OR(n, wrap, false),                                                  \
        .attr = DT_INST_PROP_OR(n, sensor_attr, 0),                                               \
        .set_on_init = DT_INST_PROP_OR(n, apply_default_on_init, true),                           \
    };                                                                                            \
    static struct behavior_trackball_dpi_data behavior_trackball_dpi_data_##n;

#define TRACKBALL_DPI_INST(n)                                                                     \
    TRACKBALL_DPI_CONFIG_INIT(n)                                                                  \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_trackball_dpi_init, NULL,                                 \
                            &behavior_trackball_dpi_data_##n,                                     \
                            &behavior_trackball_dpi_config_##n, POST_KERNEL,                      \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                  \
                            &behavior_trackball_dpi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TRACKBALL_DPI_INST)
