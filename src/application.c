// Tower Kit documentation https://tower.hardwario.com/
// SDK API description https://sdk.hardwario.com/
// Forum https://forum.hardwario.com/

#include "application.h"
#include "sensors.h"

static twr_led_t lcd_leds;

// BOOT button (LCD buttons and encoder wheel button)
static twr_button_t button;

extern void application_error(twr_error_t code);
static void mailbox_notification_update(uint64_t *id, const char *topic, void *value, void *param);

/* Must apparently have the format "update/-/xyz...".
 * Cannot be too long. Around 32 character subtopic seems ok but not much longer. */
static const twr_radio_sub_t subs[] = {
    {"update/-/notif/state", TWR_RADIO_SUB_PT_BOOL, mailbox_notification_update, NULL},
};

static void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param)
{
    // Log button event
    twr_log_info("APP: Button event: %i", event);

    // Check event source
    if (event == TWR_BUTTON_EVENT_CLICK)
    {
        twr_led_set_mode(&lcd_leds, TWR_LED_MODE_OFF);
    }
}

static void mailbox_notification_update(uint64_t *id, const char *topic, void *value, void *param)
{
    (void)id;
    (void)param;

    twr_log_info("%s: topic: %s", __func__, topic);

    bool *notify = value;

    if (*notify)
    {
        twr_led_set_mode(&lcd_leds, TWR_LED_MODE_ON);
    } else {
        twr_led_set_mode(&lcd_leds, TWR_LED_MODE_OFF);
    }
}

// Application initialization function which is called once after boot
void application_init(void)
{
    // Initialize logging
    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);

    // Initialize LCD module LEDs as default off
    const twr_led_driver_t* driver = twr_module_lcd_get_led_driver();
    twr_led_init_virtual(&lcd_leds, TWR_MODULE_LCD_LED_GREEN, driver, 1);

    // Initialize button
    twr_button_init(&button, TWR_GPIO_BUTTON, TWR_GPIO_PULL_DOWN, 0);
    twr_button_set_event_handler(&button, button_event_handler, NULL);

    sensors_init();

    // Initialize radio
    twr_radio_init(TWR_RADIO_MODE_NODE_LISTENING);
    twr_radio_set_subs((twr_radio_sub_t *) subs, sizeof(subs)/sizeof(twr_radio_sub_t));
    // Send radio pairing request
    twr_radio_pairing_request("mailbox-monitor", FW_VERSION);
}

// Application task function (optional) which is called peridically if scheduled
void application_task(void)
{
    static int counter = 0;

    // Log task run and increment counter
    twr_log_debug("APP: Task run (count: %d)", ++counter);

    // Plan next run of this task in 1000 ms
    twr_scheduler_plan_current_from_now(1000);
}
