#pragma once

#include <WebServer.h>

/** OS captive-portal probe URLs (Apple / Android / Windows). */
void balansun_platform_register_captive_portal_routes(WebServer &server);
