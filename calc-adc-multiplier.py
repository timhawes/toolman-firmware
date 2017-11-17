#!/usr/bin/env python

import math

sensor_multiplier = 100.0 # 100A->1V
fudge_multiplier = 1.093

adc_volts = 1.0
adc_max = 1024
adc_diff_to_vpp = adc_volts / adc_max
adc_diff_to_vrms = adc_diff_to_vpp / math.sqrt(2) / 2
adc_diff_to_amps = adc_diff_to_vrms * sensor_multiplier * fudge_multiplier

print(adc_diff_to_amps)
