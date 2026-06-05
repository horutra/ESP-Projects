
ESP_ERROR_CHECK(
    adc_oneshot_config_channel(
        adc_handle,
        ADC_CHANNEL_1,
        &config));

while (1)
{
    int adc_raw = 0;

    ESP_ERROR_CHECK(
        adc_oneshot_read(
            adc_handle,
            ADC_CHANNEL_1,
            &adc_raw));

    printf("ADC: %d\n", adc_raw);

    vTaskDelay(pdMS_TO_TICKS(100));
}

