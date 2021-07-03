void tds_init(void);
void config_pins();
void expose_vref();
void enable_tds_sensor();
void disable_tds_sensor();
float read_tds_sensor(int numSamples, float sampleDelay);
float convert_to_ppm(float analogReading);