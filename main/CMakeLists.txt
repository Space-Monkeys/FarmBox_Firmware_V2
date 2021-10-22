

set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )
set(COMPONENT_SRCS "main.c ../components/esp_cron/cron.c ../components/esp_cron/library/jobs/jobs.c ../components/esp_cron/library/ccronexpr/ccronexpr.c ../components/esp_cron/include/timegm.c")
set(COMPONENT_ADD_INCLUDEDIRS "../components/esp_cron ../components/esp_cron/include ../components/esp_cron/library/ccronexpr ../components/esp_cron/library/jobs")
register_component()