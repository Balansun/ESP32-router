#pragma once
#include <Arduino.h>

void TriacReadAndResetCounters(int &inDeglitch, int &raw);
int TriacGetOpenPercent();
