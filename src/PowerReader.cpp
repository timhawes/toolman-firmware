#include "PowerReader.hpp"

PowerReader::PowerReader() {

}

float PowerReader::ReadRmsCurrent() {
  const int sample_count = 1000;
  static double offsetI = 0;
  double sumI = 0;
  float Irms;

  for (unsigned int n = 0; n < sample_count; n++)
  {
    double sampleI;
    double filteredI;
    double sqI;

    sampleI = analogRead(A0);

    // Digital low pass filter extracts the 2.5 V or 1.65 V dc offset,
    // then subtract this - signal is now centered on 0 counts.
    offsetI = (offsetI + (sampleI-offsetI)/1024);
    filteredI = sampleI - offsetI;

    // Root-mean-square method current
    // 1) square current values
    sqI = filteredI * filteredI;
    // 2) sum
    sumI += sqI;
  }

  Irms = adc_to_amps * sqrt(sumI / sample_count) / 1024;
  return Irms;
}

float PowerReader::ReadSimpleCurrent()
{
  const int valcount = 50;
  const int readings = 4;
  uint16_t values[valcount];
  unsigned int max_val = 0;
  unsigned int min_val = 1024*readings;

  for (int i=0; i<valcount; i++) {
    values[i] = 0;
    for (int j=0; j<readings; j++) {
      values[i] += analogRead(A0);
    }
  }

  for (int i=0; i<valcount; i++) {
    if (values[i] > max_val) {
      max_val = values[i];
    }
    if (values[i] < min_val) {
      min_val = values[i];
    }
  }

  float diff = (float)(max_val - min_val) / 4.0;
  float adc_to_volts = 1.0 / 1024.0;
  float vpp = diff * adc_to_volts;
  float vrms = vpp / 1.414213562 / 2;
  float amps = vrms * 100.0;

  //Serial.print("adc_to_volts="); Serial.print(adc_to_volts, 6);
  //Serial.print(" min="); Serial.print(min_val, DEC);
  //Serial.print(" max="); Serial.print(max_val, DEC);
  //Serial.print(" diff="); Serial.print(diff, DEC);
  //Serial.print(" Vpp="); Serial.print(vpp, 6);
  //Serial.print(" Vrms="); Serial.print(vrms, 6);
  //Serial.print(" amps="); Serial.println(amps, 6);

  return ((float)(max_val - min_val) / (float)readings) * adc_to_amps / 1000;
  //return (unsigned long)((max_val - min_val) * config.adc_multiplier * 1000) / (config.adc_divider * readings);
}

void PowerReader::SetAdcAmpRatio(float ratio) {
    adc_to_amps = ratio;
}

void PowerReader::SetInterval(int _interval) {
    interval = _interval;
}

void PowerReader::SetVoltage(float _voltage) {
    voltage = _voltage;
}
