#include "sensors.h"

static twr_tag_temperature_t temperature_tag;
static twr_tag_barometer_t barometer_tag;
static twr_tag_humidity_t humidity_tag;

void temperature_tag_event_handler(twr_tag_temperature_t *self, twr_tag_temperature_event_t event, void *event_param)
{
    if (event == TWR_TAG_TEMPERATURE_EVENT_UPDATE)
    {
        float celsius;

        twr_tag_temperature_get_temperature_celsius(self, &celsius);
        twr_log_debug("APP: temperature: %.2f °C", celsius);
        twr_radio_pub_temperature(TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT, &celsius);
    }
    else if (event == TWR_TAG_TEMPERATURE_EVENT_ERROR)
    {
        twr_log_error("APP: Thermometer error");
    }
}

void barometer_tag_event_handler(twr_tag_barometer_t *self, twr_tag_barometer_event_t event, void *event_param)
{
    if (event == TWR_TAG_BAROMETER_EVENT_UPDATE)
    {
        float pascal, meter;

        twr_tag_barometer_get_pressure_pascal(self, &pascal);
        twr_tag_barometer_get_altitude_meter(self, &meter);
        twr_log_debug("APP: pressure: %.2f hPa, altitude: %.2f m", pascal / 100, meter);
        twr_radio_pub_barometer(TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT, &pascal, &meter);
    }
    else if (event == TWR_TAG_BAROMETER_EVENT_ERROR)
    {
        twr_log_error("APP: barometer error");
    }
}

void humidity_tag_event_handler(twr_tag_humidity_t *self, twr_tag_humidity_event_t event, void *event_param)
{
    if (event == TWR_TAG_HUMIDITY_EVENT_UPDATE)
    {
        float percentage;

        twr_tag_humidity_get_humidity_percentage(self, &percentage);
        twr_log_debug("APP: humidity: %.1f", percentage);
        twr_radio_pub_humidity(TWR_RADIO_PUB_CHANNEL_R2_I2C0_ADDRESS_DEFAULT, &percentage);
    }
    else if (event == TWR_TAG_HUMIDITY_EVENT_ERROR)
    {
        twr_log_error("APP: hygrometer error");
    }
}

void sensors_init(void)
{
    twr_tag_temperature_init(&temperature_tag, TWR_I2C_I2C0, TWR_TAG_TEMPERATURE_I2C_ADDRESS_DEFAULT);
    twr_tag_temperature_set_event_handler(&temperature_tag, temperature_tag_event_handler, NULL);
    twr_tag_temperature_set_update_interval(&temperature_tag, 10000);

    twr_tag_barometer_init(&barometer_tag, TWR_I2C_I2C0);
    twr_tag_barometer_set_event_handler(&barometer_tag, barometer_tag_event_handler, NULL);
    twr_tag_barometer_set_update_interval(&barometer_tag, 10000);

    twr_tag_humidity_init(&humidity_tag, TWR_TAG_HUMIDITY_REVISION_R2, TWR_I2C_I2C0, TWR_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT);
    twr_tag_humidity_set_event_handler(&humidity_tag, humidity_tag_event_handler, NULL);
    twr_tag_humidity_set_update_interval(&humidity_tag, 10000);
}
