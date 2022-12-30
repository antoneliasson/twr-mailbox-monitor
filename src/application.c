#include "application.h"
#include "sensors.h"

static twr_led_t lcd_leds;

// BOOT button (LCD buttons and encoder wheel button)
static twr_button_t button;

static twr_scheduler_task_id_t display_update_task;
static twr_gfx_t *gfx;

struct display_data
{
    float in_temp;
    float out_temp;
};

struct display_data display_data = {.in_temp = NAN, .out_temp = NAN};

extern void application_error(twr_error_t code);
static void mailbox_notification_update(uint64_t *id, const char *topic, void *value, void *param);
static void radio_update_sensor(uint64_t *id, const char *topic, void *value, void *param);

enum
{
    SUB_INDOOR_TEMPERATURE,
    SUB_OUTDOOR_TEMPERATURE,
};

/* Must apparently have the format "update/-/xyz...".
 * Cannot be too long. Around 32 character subtopic seems ok but not much longer. */
static const twr_radio_sub_t subs[] = {
    {"update/-/notif/state", TWR_RADIO_SUB_PT_BOOL, mailbox_notification_update, NULL},
    {"update/-/indoor/temperature", TWR_RADIO_SUB_PT_FLOAT, radio_update_sensor, (void *)SUB_INDOOR_TEMPERATURE},
    {"update/-/outdoor/temperature", TWR_RADIO_SUB_PT_FLOAT, radio_update_sensor, (void *)SUB_OUTDOOR_TEMPERATURE},
};

static void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param)
{
    twr_log_info("APP: Button event: %i", event);

    if (event == TWR_BUTTON_EVENT_CLICK)
    {
        twr_led_set_mode(&lcd_leds, TWR_LED_MODE_OFF);
    }
}

static void mailbox_notification_update(uint64_t *id, const char *topic, void *value, void *param)
{
    (void)id;
    (void)param;

    bool *notify = value;

    twr_log_info("%s: topic: %s=%s", __func__, topic, *notify ? "true" : "false");

    if (*notify)
    {
        twr_led_set_mode(&lcd_leds, TWR_LED_MODE_ON);
    } else {
        twr_led_set_mode(&lcd_leds, TWR_LED_MODE_OFF);
    }
}

static void radio_update_sensor(uint64_t *id, const char *topic, void *value, void *param)
{
    (void)id;
    int sub = (int)param;

    float *val = value;

    twr_log_info("%s: topic: %s=%.2f", __func__, topic, *val);

    switch (sub)
    {
    case SUB_INDOOR_TEMPERATURE:
        display_data.in_temp = *val;
        break;
    case SUB_OUTDOOR_TEMPERATURE:
        display_data.out_temp = *val;
        break;
    default:
        application_error(TWR_ERROR_INVALID_PARAMETER);
        break;
    }

    twr_scheduler_plan_now(display_update_task);
}

static void draw_lcd_weather_page(void)
{
    twr_gfx_clear(gfx);

    twr_gfx_set_font(gfx, &twr_font_ubuntu_15);
    twr_gfx_printf(gfx, 0, 32, true, "Inne");
    twr_gfx_set_font(gfx, &twr_font_ubuntu_28);
    if (!isnan(display_data.in_temp))
        twr_gfx_printf(gfx, 32, 24, true, "%.1f °C", display_data.in_temp);

    twr_gfx_draw_line(gfx, 8, 64, 120, 64, true);

    twr_gfx_set_font(gfx, &twr_font_ubuntu_15);
    twr_gfx_printf(gfx, 0, 96, true, "Ute");
    twr_gfx_set_font(gfx, &twr_font_ubuntu_28);
    if (!isnan(display_data.out_temp))
        twr_gfx_printf(gfx, 32, 88, true, "%.1f °C", display_data.out_temp);
}

static void display_update(void *param)
{
    (void)param;
    // twr_log_debug("%s enter", __func__);
    twr_system_pll_enable();
    if (!twr_module_lcd_is_ready())
    {
        twr_log_debug("%s not ready", __func__);
        twr_scheduler_plan_current_from_now(10);
    }
    else
    {
        draw_lcd_weather_page();
        twr_gfx_update(gfx);
    }
    twr_system_pll_disable();
    // twr_log_debug("%s leave", __func__);
}

void application_init(void)
{
    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);

    twr_module_lcd_init();
    gfx = twr_module_lcd_get_gfx();

    display_update_task = twr_scheduler_register(display_update, NULL, 0);

    // Initialize LCD module LEDs as default off
    const twr_led_driver_t* driver = twr_module_lcd_get_led_driver();
    twr_led_init_virtual(&lcd_leds, TWR_MODULE_LCD_LED_GREEN, driver, 1);

    twr_button_init(&button, TWR_GPIO_BUTTON, TWR_GPIO_PULL_DOWN, 0);
    twr_button_set_event_handler(&button, button_event_handler, NULL);

    sensors_init();

    twr_radio_init(TWR_RADIO_MODE_NODE_LISTENING);
    twr_radio_set_subs((twr_radio_sub_t *) subs, sizeof(subs)/sizeof(twr_radio_sub_t));
    twr_radio_pairing_request("mailbox-monitor", FW_VERSION);
}
