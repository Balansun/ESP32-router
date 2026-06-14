#include <balansun/platform/captive_dns.h>

#include <DNSServer.h>
#include <WiFi.h>

static DNSServer s_dns;
static bool s_dnsActive = false;

void balansun_captive_dns_start(const IPAddress &apIp) {
  if (apIp == IPAddress(0, 0, 0, 0)) return;
  if (s_dnsActive) {
    s_dns.stop();
    s_dnsActive = false;
  }
  s_dns.start(53, "*", apIp);
  s_dnsActive = true;
  Serial.println(F("Captive DNS on port 53"));
}

void balansun_captive_dns_stop(void) {
  if (!s_dnsActive) return;
  s_dns.stop();
  s_dnsActive = false;
}

void balansun_captive_dns_process(void) {
  if (s_dnsActive) s_dns.processNextRequest();
}

bool balansun_captive_dns_active(void) { return s_dnsActive; }
