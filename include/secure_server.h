#pragma once

#include "constants.h"

#if defined(ESP32) && defined(ENABLE_SERVER)

#include <Arduino.h>
#include <vector>

// Phones only allow the camera on HTTPS pages, so the camera page runs on a
// second, HTTPS server (port 443). The lamp creates its own certificate
// authority once (kept in NVS); a phone trusts it after installing
// http://<lamp>/lamp-ca.cer. Each boot the lamp issues a fresh server
// certificate for <hostname>.local and its IP from that authority.

// loads or creates the certificate authority, call before the web server starts
void initSecureServer();
// starts the HTTPS server once the clock is set (or after a timeout)
void secureServerLoop();
// DER encoded, for the download on the plain HTTP server
const std::vector<uint8_t> &getLampCaCertificate();

#endif
