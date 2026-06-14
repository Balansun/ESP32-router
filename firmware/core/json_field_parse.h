#pragma once
#include <Arduino.h>

String prefilter_json(String F1, String F2, String Json);
float parse_json_float(String nom, String Json);
long parse_json_long(String nom, String Json);
String parse_json_string(String nom, String Json);
