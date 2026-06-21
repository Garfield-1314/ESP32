#include "device/inc/sntp_time.h"

#include <string.h>
#include <sys/time.h>
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "sntp_time";

/* NTP 服务器地址（国内较快的服务器）*/
#define NTP_SERVER1 "ntp.aliyun.com"
#define NTP_SERVER2 "ntp1.aliyun.com"
#define NTP_SERVER3 "pool.ntp.org"

/* 时区设置为北京时间 (UTC+8) */
#define TIMEZONE "CST-8"

static bool s_time_synced = false;

/* 时间同步完成回调 */
static void time_sync_cb(struct timeval *tv)
{
    s_time_synced = true;
    ESP_LOGI(TAG, "Time synchronized successfully");

    /* 设置时区为北京时间 */
    setenv("TZ", TIMEZONE, 1);
    tzset();

    /* 打印当前时间 */
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "Current Beijing time: %s", strftime_buf);
}

void sntp_time_init(void)
{
    /* 检查时间是否已经设置 */
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year >= (2024 - 1900)) {
        /* 时间已经设置过（可能是从 RTC 恢复） */
        s_time_synced = true;
        ESP_LOGI(TAG, "Time already set");
        return;
    }

    ESP_LOGI(TAG, "Initializing SNTP...");

    /* 配置 SNTP - 使用经典 API */
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVER1);
    esp_sntp_setservername(1, NTP_SERVER2);
    esp_sntp_setservername(2, NTP_SERVER3);
    esp_sntp_set_time_sync_notification_cb(time_sync_cb);
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);

    esp_sntp_init();

    ESP_LOGI(TAG, "SNTP initialized, waiting for time sync...");
}

bool sntp_time_get_local(struct tm *tm)
{
    if (!s_time_synced) {
        return false;
    }

    time_t now;
    time(&now);
    localtime_r(&now, tm);
    return true;
}

void sntp_time_get_str(char *buf, size_t len)
{
    struct tm timeinfo;
    if (!sntp_time_get_local(&timeinfo)) {
        snprintf(buf, len, "--:--:--");
        return;
    }
    strftime(buf, len, "%H:%M:%S", &timeinfo);
}

void sntp_time_get_date_str(char *buf, size_t len)
{
    struct tm timeinfo;
    if (!sntp_time_get_local(&timeinfo)) {
        snprintf(buf, len, "----/--/--");
        return;
    }
    strftime(buf, len, "%Y/%m/%d", &timeinfo);
}

bool sntp_time_is_synced(void)
{
    return s_time_synced;
}

bool sntp_time_wait_for_sync(uint32_t timeout_ms)
{
    if (s_time_synced) {
        return true;
    }

    int retry = 0;
    const int retry_count = timeout_ms / 1000;
    while (retry < retry_count && !s_time_synced) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry++;
    }

    return s_time_synced;
}