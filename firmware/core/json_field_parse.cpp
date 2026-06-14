#include "json_field_parse.h"

String prefilter_json(String F1, String F2, String Json) {
  int p = Json.indexOf(F1);
  Json = Json.substring(p);
  p = Json.indexOf(F2);
  Json = Json.substring(p);
  return Json;
}

float parse_json_float(String nom, String Json) {
  int p = Json.indexOf(nom);
  Json = Json.substring(p);
  p = Json.indexOf(":");
  Json = Json.substring(p + 1);
  int q = Json.indexOf(",");
  p = Json.indexOf("}");
  p = min(p, q);
  float val = 0;
  if (p > 0) {
    Json = Json.substring(0, p);
    val = Json.toFloat();
  }
  return val;
}

long parse_json_long(String nom, String Json) {
  int p = Json.indexOf(nom);
  Json = Json.substring(p);
  p = Json.indexOf(":");
  Json = Json.substring(p + 1);
  int q = Json.indexOf(".");
  p = Json.indexOf("}");
  p = min(p, q);
  long val = 0;
  if (p > 0) {
    Json = Json.substring(0, p);
    val = Json.toInt();
  }
  return val;
}

String parse_json_string(String nom, String Json) {
  int p = Json.indexOf(nom);
  Json = Json.substring(p);
  p = Json.indexOf(":");
  Json = Json.substring(p + 1);
  p = Json.indexOf("\"");
  Json = Json.substring(p + 1);
  p = Json.indexOf("\"");
  Json = Json.substring(0, p);
  return Json;
}
