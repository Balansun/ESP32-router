#pragma once
#include <Arduino.h>

/** Send gzipped SPA (same payload as GET /). */
void WebUi_sendSpa(void);
/** PWA install assets (must be registered before SPA fallback). */
void WebUi_sendManifest(void);
void WebUi_sendServiceWorker(void);
void WebUi_sendPwaIcon192(void);
void WebUi_sendPwaIcon512(void);
/** If true, response already sent (SPA). */
bool WebUi_trySpaFallback(void);
