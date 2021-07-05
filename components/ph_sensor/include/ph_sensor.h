void ph_init(void);
void config_ph_pins();
void ph_calibration(int numSamples, float samplePeriod);
float read_ph_sensor(int numSamples, float samplePeriod);
float convert_to_ph(float analogReading);
void expose_ph_vref();
void enable_ph_sensor();
void disable_ph_sensor();