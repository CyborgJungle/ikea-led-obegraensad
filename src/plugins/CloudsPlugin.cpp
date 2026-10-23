#include "plugins/CloudsPlugin.h"

// the fetch needs WiFi and a second core for its task
#ifdef ESP32

#include "constants.h"
#include "mode_clock.h"
#include "screen.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <algorithm>
#include <math.h>
#include <memory>

namespace
{
// EUMETSAT's public map server: the newest Meteosat (MTG) infrared image, 10.5 um,
// every 10 minutes, day and night. Brighter is colder, so higher cloud. The area is
// about 120 km wide and 175 km tall around Amsterdam, as tall as the lamp's shape;
// the server shrinks it to 16x16 and sends it as an uncompressed TIFF.
const char *const FEED_URL =
    "https://view.eumetsat.int/geoserver/ows?service=WMS&version=1.3.0&request=GetMap"
    "&crs=EPSG:4326&bbox=51.58,4.02,53.16,5.78&styles=&layers=mtg_fd:ir105_hrfi"
    "&width=16&height=16&format=image/tiff";
constexpr unsigned long FETCH_EVERY_MS = 10UL * 60 * 1000;
constexpr unsigned long RETRY_AFTER_MS = 60UL * 1000;
// the TIFF is about 2 KB: 16x16 pixels and a 256-colour palette
constexpr size_t MAX_TIFF_BYTES = 4096;

// collects a download in a fixed buffer; more than fits marks it overflowed
struct BufferStream : public Stream
{
  uint8_t data[MAX_TIFF_BYTES];
  size_t size = 0;
  bool overflowed = false;

  size_t write(uint8_t byte) override
  {
    return write(&byte, 1);
  }
  size_t write(const uint8_t *buffer, size_t length) override
  {
    if (size + length > MAX_TIFF_BYTES)
    {
      overflowed = true;
      return 0;
    }
    memcpy(data + size, buffer, length);
    size += length;
    return length;
  }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
};

// an LED is about 1.5 times taller than wide, as in Blobs
constexpr float ASPECT = 1.5f;
// a new image morphs in over 10 minutes of the mode's time, the feed's own pace
constexpr float BLEND_S = 600;
// texture drift in LEDs per second, plus some for the wind measured between images
constexpr float DRIFT = 0.35f;
constexpr float WIND_GAIN = 0.25f;
// an image with less contrast than this is not stretched into noise
constexpr float MIN_RANGE = 40;
// before any image: an even, gentle sky
constexpr float NO_DATA_COVERAGE = 0.45f;
// field values of the soft edge, and the dimmest LED that is not left off
constexpr float EDGE_LO = 0.12f;
constexpr float EDGE_HI = 1.05f;
constexpr uint8_t FLOOR = 10;
constexpr unsigned long FRAME_MS = 40;
constexpr float MAX_STEP_S = 0.4f;

float smoothstep(float low, float high, float value)
{
  float t = (value - low) / (high - low);
  t = t < 0 ? 0 : (t > 1 ? 1 : t);
  return t * t * (3 - 2 * t);
}

// value noise on a lattice that repeats every 4096 cells, so the texture's
// position can wrap by that much without a jump
float hashCell(int32_t ix, int32_t iy, uint32_t seed)
{
  uint32_t h = static_cast<uint32_t>(ix & 4095) * 374761393u + static_cast<uint32_t>(iy & 4095) * 668265263u +
               seed * 2246822519u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return ((h ^ (h >> 16)) & 0xFFFF) / 65535.0f;
}

float valueNoise(float x, float y, uint32_t seed)
{
  const float fx0 = floorf(x), fy0 = floorf(y);
  const int32_t ix = static_cast<int32_t>(fx0), iy = static_cast<int32_t>(fy0);
  float fx = x - fx0, fy = y - fy0;
  fx = fx * fx * (3 - 2 * fx);
  fy = fy * fy * (3 - 2 * fy);
  const float a = hashCell(ix, iy, seed), b = hashCell(ix + 1, iy, seed);
  const float c = hashCell(ix, iy + 1, seed), d = hashCell(ix + 1, iy + 1, seed);
  return (a + (b - a) * fx) * (1 - fy) + (c + (d - c) * fx) * fy;
}

// keeps a noise coordinate offset small, a whole number of lattice periods at a time
float wrapLattice(double value)
{
  return static_cast<float>(value - 4096.0 * floor(value / 4096.0));
}

// Reads an uncompressed 8-bit TIFF of 16x16 (palette or grey) into grey levels.
// Every offset is checked against the data: a broken or unexpected file is refused.
bool readTiff(const uint8_t *data, size_t size, uint8_t *grey)
{
  if (size < 8)
    return false;
  const bool big = data[0] == 'M' && data[1] == 'M';
  if (!big && !(data[0] == 'I' && data[1] == 'I'))
    return false;
  auto u16 = [&](size_t at) -> uint32_t {
    return big ? (data[at] << 8) | data[at + 1] : data[at] | (data[at + 1] << 8);
  };
  auto u32 = [&](size_t at) -> uint32_t {
    return big ? (u16(at) << 16) | u16(at + 2) : u16(at) | (u16(at + 2) << 16);
  };
  // "does [at, at + length) fit": written so a huge offset cannot wrap around
  auto fits = [&](uint32_t at, uint32_t length) { return at <= size && length <= size - at; };
  if (u16(2) != 42)
    return false;
  const uint32_t ifd = u32(4);
  if (!fits(ifd, 2))
    return false;
  const uint32_t entries = u16(ifd);
  if (!fits(ifd + 2, entries * 12))
    return false;

  uint32_t width = 0, height = 0, bits = 0, compression = 0, photometric = 1;
  uint32_t stripOffset = 0, stripCount = 0, stripBytes = 0, colormap = 0;
  for (uint32_t i = 0; i < entries; i++)
  {
    const size_t e = ifd + 2 + i * 12;
    const uint32_t tag = u16(e), type = u16(e + 2), count = u32(e + 4);
    // values of up to 4 bytes sit in the entry itself; larger ones (the palette)
    // are elsewhere, and the entry holds their 4-byte offset
    const uint32_t bytes = (type == 3 ? 2 : 4) * (count > 4 ? 4 : count);
    const uint32_t value = bytes > 4 || type != 3 ? u32(e + 8) : u16(e + 8);
    switch (tag)
    {
    case 256: width = value; break;
    case 257: height = value; break;
    case 258: bits = value; break;
    case 259: compression = value; break;
    case 262: photometric = value; break;
    case 273: stripOffset = value; stripCount = count; break;
    case 279: stripBytes = value; break;
    case 320: colormap = type == 3 && count == 768 ? value : 0; break;
    }
  }
  // the server sends one strip; more would need a list of offsets
  if (width != COLS || height != ROWS || bits != 8 || compression != 1 || stripCount != 1 ||
      stripBytes < ROWS * COLS || !fits(stripOffset, ROWS * COLS))
    return false;
  const uint8_t *pixels = data + stripOffset;
  if (photometric == 3)
  {
    // a palette: 256 reds, then greens, then blues, 16 bits each
    if (colormap == 0 || !fits(colormap, 768 * 2))
      return false;
    for (int i = 0; i < ROWS * COLS; i++)
    {
      const uint32_t p = pixels[i];
      const uint32_t r = u16(colormap + 2 * p), g = u16(colormap + 2 * (256 + p)),
                     b = u16(colormap + 2 * (512 + p));
      grey[i] = ((r * 299 + g * 587 + b * 114) / 1000) >> 8;
    }
    return true;
  }
  if (photometric == 1)
  {
    memcpy(grey, pixels, ROWS * COLS);
    return true;
  }
  return false;
}

// How cloudy each LED is, 0..1: the image stretched to its own contrast, then
// each LED averaged with its neighbours so single-pixel noise does not speckle
void coverage(const uint8_t *grey, float *out)
{
  uint8_t sorted[ROWS * COLS];
  memcpy(sorted, grey, sizeof(sorted));
  std::sort(sorted, sorted + ROWS * COLS);
  float low = sorted[ROWS * COLS / 10], high = sorted[ROWS * COLS * 19 / 20];
  const float span = fmaxf(high - low, MIN_RANGE);
  low = (low + high) / 2 - span / 2;

  float raw[ROWS * COLS];
  for (int i = 0; i < ROWS * COLS; i++)
  {
    raw[i] = fminf(1, fmaxf(0, (grey[i] - low) / span));
  }
  for (int y = 0; y < ROWS; y++)
  {
    for (int x = 0; x < COLS; x++)
    {
      float total = 0, weight = 0;
      for (int dy = -1; dy <= 1; dy++)
      {
        for (int dx = -1; dx <= 1; dx++)
        {
          if (x + dx >= 0 && x + dx < COLS && y + dy >= 0 && y + dy < ROWS)
          {
            const float w = dx == 0 && dy == 0 ? 2 : 1;
            total += raw[(y + dy) * COLS + x + dx] * w;
            weight += w;
          }
        }
      }
      out[y * COLS + x] = total / weight;
    }
  }
}

// How far the clouds moved between two maps, in LEDs, below a whole LED too:
// one shift for the whole image, least squares on the brightness gradients
void wind(const float *a, const float *b, float &dx, float &dy)
{
  float sxx = 0, sxy = 0, syy = 0, sxt = 0, syt = 0;
  for (int y = 1; y < ROWS - 1; y++)
  {
    for (int x = 1; x < COLS - 1; x++)
    {
      const int i = y * COLS + x;
      const float gx = (a[i + 1] - a[i - 1] + b[i + 1] - b[i - 1]) / 4;
      const float gy = (a[i + COLS] - a[i - COLS] + b[i + COLS] - b[i - COLS]) / 4;
      const float gt = b[i] - a[i];
      sxx += gx * gx;
      sxy += gx * gy;
      syy += gy * gy;
      sxt += gx * gt;
      syt += gy * gt;
    }
  }
  const float det = sxx * syy - sxy * sxy;
  if (det < 1e-6f)
  {
    dx = dy = 0;
    return;
  }
  dx = fmaxf(-3, fminf(3, -(syy * sxt - sxy * syt) / det));
  dy = fmaxf(-3, fminf(3, -(sxx * syt - sxy * sxt) / det));
}
} // namespace

void CloudsPlugin::setup()
{
  Screen.clear();
  memset(previousBrightness, 0, sizeof(previousBrightness));
  // the first time: an even sky until the first image is in; afterwards the
  // last images are kept, so coming back to Clouds carries on where it was
  if (!initialized)
  {
    for (int i = 0; i < ROWS * COLS; i++)
    {
      previous[i] = next[i] = NO_DATA_COVERAGE;
    }
    blend = 1;
    vx = targetVx = DRIFT;
    vy = targetVy = 0.08f;
    initialized = true;
  }
  lastFrame = millis();
  lastModeTime = modeMillis();
  // fetch now if the last image is old
  if (!fetchedOnce || millis() - lastFetch >= FETCH_EVERY_MS)
  {
    startFetch();
  }
}

void CloudsPlugin::teardown()
{
  // a fetch still running finishes on its own; its image waits for next time
}

void CloudsPlugin::startFetch()
{
  if (fetchRunning.exchange(true))
  {
    return;
  }
  lastFetch = millis();
  fetchFailed = false;
  // core 1, away from the screen, like the weather fetch; TLS needs the stack
  if (xTaskCreatePinnedToCore(fetchTask, "cloudsFetch", 16384, this, 1, NULL, 1) != pdPASS)
  {
    Serial.println("[Clouds] Could not start the fetch task");
    fetchFailed = true;
    fetchRunning = false;
  }
}

void CloudsPlugin::fetchTask(void *parameter)
{
  CloudsPlugin *plugin = static_cast<CloudsPlugin *>(parameter);
  const bool ok = plugin->fetch();
  plugin->fetchFailed = !ok;
  plugin->fetchRunning = false;
  vTaskDelete(NULL);
}

// Runs in the fetch task: downloads and reads the newest image into fetched[]
bool CloudsPlugin::fetch()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    return false;
  }
  // a public image, not secret; certificates are not checked, as for the weather
  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure());
  client->setInsecure();
  HTTPClient http;
  http.setTimeout(15000);
  if (!http.begin(*client, FEED_URL))
  {
    return false;
  }
  const int code = http.GET();
  if (code != HTTP_CODE_OK)
  {
    Serial.printf("[Clouds] Feed answered %d\n", code);
    http.end();
    return false;
  }

  // the server sends the image in chunks; writeToStream takes those apart
  std::unique_ptr<BufferStream> buffer(new BufferStream());
  BufferStream &body = *buffer;
  const int written = http.writeToStream(&body);
  http.end();
  if (written <= 0 || body.overflowed)
  {
    Serial.printf("[Clouds] Could not download the image (%d)\n", written);
    return false;
  }

  // loop() has not taken the previous image yet: keep that one
  if (frameReady)
  {
    return true;
  }
  if (!readTiff(body.data, body.size, fetched))
  {
    Serial.printf("[Clouds] Could not read the image (%u bytes)\n", static_cast<unsigned>(body.size));
    return false;
  }
  fetchedOnce = true;
  frameReady = true;
  return true;
}

void CloudsPlugin::takeFrame(const uint8_t *grey)
{
  float cover[ROWS * COLS];
  coverage(grey, cover);
  float current[ROWS * COLS];
  currentField(current);
  if (hasLastCoverage)
  {
    float dx, dy;
    wind(lastCoverage, cover, dx, dy);
    const float distance = sqrtf(dx * dx + dy * dy);
    // the real wind's direction, at the texture's own pace
    if (distance > 0.05f)
    {
      const float speed = DRIFT + WIND_GAIN * distance;
      targetVx = speed * dx / distance;
      targetVy = speed * dy / distance;
    }
  }
  memcpy(lastCoverage, cover, sizeof(cover));
  hasLastCoverage = true;
  memcpy(previous, current, sizeof(current));
  memcpy(next, cover, sizeof(cover));
  blend = 0;
}

void CloudsPlugin::currentField(float *field) const
{
  const float k = smoothstep(0, 1, blend);
  for (int i = 0; i < ROWS * COLS; i++)
  {
    field[i] = previous[i] + (next[i] - previous[i]) * k;
  }
}

void CloudsPlugin::loop()
{
  // a new image is in: it starts morphing in now
  if (frameReady)
  {
    takeFrame(fetched);
    frameReady = false;
  }
  if (!fetchRunning)
  {
    const unsigned long wait = fetchFailed ? RETRY_AFTER_MS : FETCH_EVERY_MS;
    if (millis() - lastFetch >= wait)
    {
      startFetch();
    }
  }

  // frames on real time, so it stays smooth also when slowed down
  const unsigned long now = millis();
  if (now - lastFrame < FRAME_MS)
  {
    return;
  }
  lastFrame = now;
  // everything moves by the mode's clock, so the speed setting applies
  const uint32_t modeNow = modeMillis();
  const float dt = fminf((modeNow - lastModeTime) / 1000.0f, MAX_STEP_S);
  lastModeTime = modeNow;

  blend = fminf(1, blend + dt / BLEND_S);
  // a change of wind turns the drift over about half a minute
  const float ease = fminf(1, dt / 30);
  vx += (targetVx - vx) * ease;
  vy += (targetVy - vy) * ease;
  offsetX += vx * dt;
  offsetY += vy * dt;
  time += dt;
  render();
}

void CloudsPlugin::render()
{
  float cover[ROWS * COLS];
  currentField(cover);

  // the texture's offsets for this frame, wrapped by whole lattice periods
  const float base1x = wrapLattice(offsetX * 0.32);
  const float base1y = wrapLattice(offsetY * ASPECT * 0.22);
  const float base2x = wrapLattice(offsetX * 0.65 - time * 0.011);
  const float base2y = wrapLattice(offsetY * ASPECT * 0.45 + time * 0.007);

  for (uint8_t row = 0; row < ROWS; row++)
  {
    for (uint8_t col = 0; col < COLS; col++)
    {
      // two layers drifting at different speeds: the texture keeps changing shape
      const float n1 = valueNoise(col * 0.32f - base1x, row * ASPECT * 0.22f - base1y, 1);
      const float n2 = valueNoise(col * 0.65f - base2x, row * ASPECT * 0.45f - base2y, 2);
      const float texture = 0.65f * n1 + 0.35f * n2;
      // the real sky sets how much cloud there is here, the texture its shape
      const float value = cover[row * COLS + col] * 0.75f + (texture - 0.5f) * 1.3f - 0.05f;

      // LEDs at half power look much brighter than half: squared keeps the edge soft
      const float level = smoothstep(EDGE_LO, EDGE_HI, value);
      uint8_t brightness = static_cast<uint8_t>(level * level * 255.0f + 0.5f);
      if (brightness < FLOOR)
      {
        brightness = 0;
      }
      const uint16_t index = row * COLS + col;
      if (previousBrightness[index] != brightness)
      {
        Screen.setPixel(col, row, brightness > 0, brightness);
        previousBrightness[index] = brightness;
      }
    }
  }
}

const char *CloudsPlugin::getName() const
{
  return "Clouds";
}

#endif
