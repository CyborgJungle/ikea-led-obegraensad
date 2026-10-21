#include "secure_server.h"

#if defined(ESP32) && defined(ENABLE_SERVER)

#include <Preferences.h>
#include <WiFi.h>
#include <esp_https_server.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ecp.h>
#include <mbedtls/entropy.h>
#include <mbedtls/oid.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>
#include <time.h>

#include "PluginManager.h"
#include "camera_page.h"
#include "plugins/CameraPlugin.h"
#include "scheduler.h"
#include "secrets.h"
#include "timing.h"

namespace
{
using Bytes = std::vector<uint8_t>;

Bytes caCertificate;
Bytes caKey;
Bytes serverCertificate;
Bytes serverKey;
httpd_handle_t httpsServer = nullptr;
bool startAttempted = false;

// wait this long for the clock before issuing the certificate with the build date
constexpr unsigned long CLOCK_TIMEOUT = 20000;
// Apple accepts at most 825 days for certificates from a user installed authority
constexpr long SERVER_CERT_DAYS = 800;

struct Random
{
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context drbg;
  bool ready;

  Random()
  {
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&drbg);
    ready = mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy, nullptr, 0) == 0;
  }

  ~Random()
  {
    mbedtls_ctr_drbg_free(&drbg);
    mbedtls_entropy_free(&entropy);
  }
};

struct Key
{
  mbedtls_pk_context pk;
  Key()
  {
    mbedtls_pk_init(&pk);
  }
  ~Key()
  {
    mbedtls_pk_free(&pk);
  }
};

bool generateKey(Key &key, Random &random)
{
  return mbedtls_pk_setup(&key.pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY)) == 0 &&
         mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1,
                             mbedtls_pk_ec(key.pk),
                             mbedtls_ctr_drbg_random,
                             &random.drbg) == 0;
}

// mbedtls writes DER at the end of the buffer and returns its length
Bytes tail(const unsigned char *buffer, size_t size, int length)
{
  return length > 0 ? Bytes(buffer + size - length, buffer + size) : Bytes();
}

Bytes keyToDer(Key &key)
{
  unsigned char buffer[256];
  return tail(buffer, sizeof(buffer), mbedtls_pk_write_key_der(&key.pk, buffer, sizeof(buffer)));
}

// "YYYYMMDDhhmmss" as mbedtls wants validity dates
String validityDate(time_t time)
{
  struct tm parts;
  gmtime_r(&time, &parts);
  char text[16];
  strftime(text, sizeof(text), "%Y%m%d%H%M%S", &parts);
  return text;
}

// the clock if NTP set it, the build date otherwise
time_t certificateNow()
{
  struct tm parts;
  if (getSyncedLocalTime(parts))
  {
    return time(nullptr);
  }
  memset(&parts, 0, sizeof(parts));
  strptime(__DATE__, "%b %d %Y", &parts);
  return mktime(&parts);
}

struct CertificateRequest
{
  Key *subjectKey;
  Key *issuerKey;
  const char *subjectName;
  const char *issuerName;
  String notBefore;
  String notAfter;
  bool authority;
  const mbedtls_x509_san_list *alternativeNames;
};

Bytes writeCertificate(const CertificateRequest &request, Random &random)
{
  mbedtls_x509write_cert writer;
  mbedtls_x509write_crt_init(&writer);

  unsigned char serial[16];
  mbedtls_ctr_drbg_random(&random.drbg, serial, sizeof(serial));
  serial[0] &= 0x7F; // serial numbers must be positive

  mbedtls_x509write_crt_set_version(&writer, MBEDTLS_X509_CRT_VERSION_3);
  mbedtls_x509write_crt_set_md_alg(&writer, MBEDTLS_MD_SHA256);
  mbedtls_x509write_crt_set_subject_key(&writer, &request.subjectKey->pk);
  mbedtls_x509write_crt_set_issuer_key(&writer, &request.issuerKey->pk);

  bool ok = mbedtls_x509write_crt_set_serial_raw(&writer, serial, sizeof(serial)) == 0 &&
            mbedtls_x509write_crt_set_subject_name(&writer, request.subjectName) == 0 &&
            mbedtls_x509write_crt_set_issuer_name(&writer, request.issuerName) == 0 &&
            mbedtls_x509write_crt_set_validity(
                &writer, request.notBefore.c_str(), request.notAfter.c_str()) == 0 &&
            mbedtls_x509write_crt_set_basic_constraints(&writer, request.authority, request.authority ? 0 : -1) == 0 &&
            mbedtls_x509write_crt_set_subject_key_identifier(&writer) == 0 &&
            mbedtls_x509write_crt_set_authority_key_identifier(&writer) == 0;

  if (request.authority)
  {
    ok = ok && mbedtls_x509write_crt_set_key_usage(
                   &writer, MBEDTLS_X509_KU_KEY_CERT_SIGN | MBEDTLS_X509_KU_CRL_SIGN) == 0;
  }
  else
  {
    mbedtls_asn1_sequence serverAuth = {};
    serverAuth.buf.tag = MBEDTLS_ASN1_OID;
    serverAuth.buf.p = (unsigned char *)MBEDTLS_OID_SERVER_AUTH;
    serverAuth.buf.len = MBEDTLS_OID_SIZE(MBEDTLS_OID_SERVER_AUTH);
    ok = ok &&
         mbedtls_x509write_crt_set_key_usage(&writer, MBEDTLS_X509_KU_DIGITAL_SIGNATURE) == 0 &&
         mbedtls_x509write_crt_set_ext_key_usage(&writer, &serverAuth) == 0 &&
         mbedtls_x509write_crt_set_subject_alternative_name(&writer, request.alternativeNames) ==
             0;
  }

  Bytes result;
  if (ok)
  {
    unsigned char buffer[1024];
    result = tail(buffer,
                  sizeof(buffer),
                  mbedtls_x509write_crt_der(
                      &writer, buffer, sizeof(buffer), mbedtls_ctr_drbg_random, &random.drbg));
  }
  mbedtls_x509write_crt_free(&writer);
  return result;
}

const char CA_NAME[] = "CN=" WIFI_HOSTNAME " lamp local authority,O=ikea-led-obegraensad";

bool loadCertificateAuthority()
{
  Preferences storage;
  if (!storage.begin("secure", true))
  {
    return false;
  }
  caCertificate.resize(storage.getBytesLength("caCert"));
  caKey.resize(storage.getBytesLength("caKey"));
  const bool loaded = !caCertificate.empty() && !caKey.empty() &&
                      storage.getBytes("caCert", caCertificate.data(), caCertificate.size()) &&
                      storage.getBytes("caKey", caKey.data(), caKey.size());
  storage.end();
  return loaded;
}

bool createCertificateAuthority()
{
  Random random;
  Key key;
  if (!random.ready || !generateKey(key, random))
  {
    return false;
  }

  CertificateRequest request = {&key, &key, CA_NAME, CA_NAME, "20250101000000", "20450101000000",
                                true, nullptr};
  caCertificate = writeCertificate(request, random);
  caKey = keyToDer(key);
  if (caCertificate.empty() || caKey.empty())
  {
    return false;
  }

  Preferences storage;
  if (storage.begin("secure", false))
  {
    storage.putBytes("caCert", caCertificate.data(), caCertificate.size());
    storage.putBytes("caKey", caKey.data(), caKey.size());
    storage.end();
  }
  return true;
}

bool issueServerCertificate()
{
  Random random;
  Key authorityKey;
  Key key;
  if (!random.ready ||
      mbedtls_pk_parse_key(&authorityKey.pk,
                           caKey.data(),
                           caKey.size(),
                           nullptr,
                           0,
                           mbedtls_ctr_drbg_random,
                           &random.drbg) != 0 ||
      !generateKey(key, random))
  {
    return false;
  }

  static const char hostname[] = WIFI_HOSTNAME ".local";
  const uint32_t ip = WiFi.localIP();
  mbedtls_x509_san_list ipName = {};
  ipName.node.type = MBEDTLS_X509_SAN_IP_ADDRESS;
  ipName.node.san.unstructured_name.p = (unsigned char *)&ip; // network byte order
  ipName.node.san.unstructured_name.len = 4;
  mbedtls_x509_san_list dnsName = {};
  dnsName.node.type = MBEDTLS_X509_SAN_DNS_NAME;
  dnsName.node.san.unstructured_name.p = (unsigned char *)hostname;
  dnsName.node.san.unstructured_name.len = strlen(hostname);
  dnsName.next = &ipName;

  const time_t now = certificateNow();
  const String subject = String("CN=") + hostname;
  CertificateRequest request = {&key,
                                &authorityKey,
                                subject.c_str(),
                                CA_NAME,
                                validityDate(now - 24 * 3600),
                                validityDate(now + SERVER_CERT_DAYS * 24 * 3600),
                                false,
                                &dnsName};
  serverCertificate = writeCertificate(request, random);
  serverKey = keyToDer(key);
  return !serverCertificate.empty() && !serverKey.empty();
}

esp_err_t handleCameraPage(httpd_req_t *request)
{
  httpd_resp_set_type(request, "text/html");
  httpd_resp_set_hdr(request, "Cache-Control", "no-cache");
  return httpd_resp_send(request, CAMERA_PAGE, HTTPD_RESP_USE_STRLEN);
}

esp_err_t handleRoot(httpd_req_t *request)
{
  httpd_resp_set_status(request, "302 Found");
  httpd_resp_set_hdr(request, "Location", "/camera");
  return httpd_resp_send(request, nullptr, 0);
}

// binary: one 16x16 grayscale frame; text "start": switch to the Camera mode
esp_err_t handleCameraSocket(httpd_req_t *request)
{
  if (request->method == HTTP_GET)
  {
    return ESP_OK; // handshake
  }

  httpd_ws_frame_t frame = {};
  esp_err_t result = httpd_ws_recv_frame(request, &frame, 0);
  if (result != ESP_OK || frame.len > TOTAL_PIXELS)
  {
    return result != ESP_OK ? result : ESP_ERR_INVALID_SIZE;
  }

  uint8_t payload[TOTAL_PIXELS + 1] = {};
  frame.payload = payload;
  result = httpd_ws_recv_frame(request, &frame, frame.len);
  if (result != ESP_OK)
  {
    return result;
  }

  if (frame.type == HTTPD_WS_TYPE_BINARY && frame.len == TOTAL_PIXELS)
  {
    CameraPlugin::receiveFrame(payload);
  }
  else if (frame.type == HTTPD_WS_TYPE_TEXT && strcmp((const char *)payload, "start") == 0)
  {
    Scheduler.clearSchedule();
    pluginManager.setActivePlugin("Camera");
  }
  return ESP_OK;
}

void startHttpsServer()
{
  if (!issueServerCertificate())
  {
    Serial.println("[Secure] Could not issue the server certificate");
    return;
  }

  httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
  config.servercert = serverCertificate.data();
  config.servercert_len = serverCertificate.size();
  config.prvtkey_pem = serverKey.data();
  config.prvtkey_len = serverKey.size();
  // every TLS connection holds ~40 KB of heap
  config.httpd.max_open_sockets = 2;
  config.httpd.lru_purge_enable = true;

  if (httpd_ssl_start(&httpsServer, &config) != ESP_OK)
  {
    Serial.println("[Secure] Could not start the HTTPS server");
    httpsServer = nullptr;
    return;
  }

  const httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = handleRoot};
  const httpd_uri_t page = {.uri = "/camera", .method = HTTP_GET, .handler = handleCameraPage};
  httpd_uri_t socket = {.uri = "/ws", .method = HTTP_GET, .handler = handleCameraSocket};
  socket.is_websocket = true;
  httpd_register_uri_handler(httpsServer, &root);
  httpd_register_uri_handler(httpsServer, &page);
  httpd_register_uri_handler(httpsServer, &socket);
  Serial.println("[Secure] HTTPS server started on port 443");
}
} // namespace

void initSecureServer()
{
  if (!loadCertificateAuthority() && !createCertificateAuthority())
  {
    Serial.println("[Secure] Could not create the certificate authority");
  }
}

void secureServerLoop()
{
  if (startAttempted || caKey.empty() || WiFi.status() != WL_CONNECTED)
  {
    return;
  }
  struct tm now;
  if (!getSyncedLocalTime(now) && millis() < CLOCK_TIMEOUT)
  {
    return;
  }
  startAttempted = true;
  startHttpsServer();
}

const std::vector<uint8_t> &getLampCaCertificate()
{
  return caCertificate;
}

#endif
