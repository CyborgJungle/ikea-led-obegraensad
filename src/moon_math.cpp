#include "moon_math.h"
#include <math.h>

// the moon's astronomy and its picture, apart from the lamp so they can be
// checked on a computer (see MoonPlugin for the mode around them)

namespace
{
// an LED is this much taller than wide (the lamp is 9:13), so the moon is
// 15 LEDs wide and about 10 tall to look round
constexpr float ASPECT = 13.0f / 9.0f;
constexpr float RADIUS = 7.5f;
constexpr float CENTRE_X = 8.0f, CENTRE_Y = 8.0f;
// points per LED, each way: a smooth edge and shadow line
constexpr int SAMPLES = 4;
// the dark part near new moon, lit by sunlight off the Earth
constexpr float EARTHSHINE = 0.25f;
// half the width of the soft shadow line
constexpr float TERMINATOR = 0.08f;
constexpr uint8_t FLOOR = 10;
// before the clock is set: a dim outline of the moon
constexpr float WAITING = 0.35f;

// the dark seas of the near side, north up as seen from the northern
// hemisphere: x right, y up, in moon radii; radii and how dark they are
struct Mare
{
  float x, y, rx, ry, dark;
};
const Mare MARIA[] = {
    {-0.55f, 0.12f, 0.30f, 0.45f, 0.40f}, // Oceanus Procellarum
    {-0.30f, 0.45f, 0.28f, 0.20f, 0.45f}, // Mare Imbrium
    {0.18f, 0.42f, 0.16f, 0.14f, 0.45f},  // Mare Serenitatis
    {0.33f, 0.12f, 0.18f, 0.18f, 0.45f},  // Mare Tranquillitatis
    {0.68f, 0.30f, 0.11f, 0.12f, 0.50f},  // Mare Crisium
    {0.50f, -0.15f, 0.12f, 0.17f, 0.35f}, // Mare Fecunditatis
    {-0.15f, -0.35f, 0.18f, 0.14f, 0.35f}, // Mare Nubium
    {-0.45f, -0.35f, 0.10f, 0.10f, 0.35f}, // Mare Humorum
    {-0.05f, 0.74f, 0.35f, 0.06f, 0.30f},  // Mare Frigoris
};

float albedo(float x, float y)
{
  float a = 1;
  for (const Mare &m : MARIA)
  {
    const float dx = (x - m.x) / m.rx, dy = (y - m.y) / m.ry;
    const float d = dx * dx + dy * dy;
    if (d < 1)
    {
      const float k = 1 - d;
      a -= m.dark * k * sqrtf(k);
    }
  }
  return fmaxf(0.35f, a);
}

float smoothstep(float low, float high, float value)
{
  float t = (value - low) / (high - low);
  t = t < 0 ? 0 : (t > 1 ? 1 : t);
  return t * t * (3 - 2 * t);
}

double sinDeg(double d) { return sin(d * M_PI / 180); }
double cosDeg(double d) { return cos(d * M_PI / 180); }
double degOf(double r) { return r * 180 / M_PI; }
double wrap360(double d) { return d - 360 * floor(d / 360); }

// the ecliptic to equatorial coordinates (Meeus ch. 13)
void equatorial(double lon, double lat, double eps, double &ra, double &dec)
{
  ra = wrap360(degOf(atan2(sinDeg(lon) * cosDeg(eps) - tan(lat * M_PI / 180) * sinDeg(eps), cosDeg(lon))));
  dec = degOf(asin(sinDeg(lat) * cosDeg(eps) + cosDeg(lat) * sinDeg(eps) * sinDeg(lon)));
}

} // namespace

// Low-precision formulas from Jean Meeus, Astronomical Algorithms (2nd ed.):
// sun ch. 25, moon ch. 47 (largest terms), sidereal time ch. 12, parallactic
// angle ch. 14, illuminated fraction and bright limb ch. 48. Against JPL
// Horizons for Rotterdam over a month: the lit fraction within 0.7 percentage
// points, the direction in the sky within 0.12 degrees.
MoonState moonState(double unixTime, bool hasPlace, double latitude, double longitude)
{
  const double jd = unixTime / 86400.0 + 2440587.5;
  const double t = (jd - 2451545.0) / 36525.0;

  // the sun's apparent longitude and distance
  const double l0 = 280.46646 + 36000.76983 * t;
  const double m = 357.52911 + 35999.05029 * t;
  const double c = (1.914602 - 0.004817 * t) * sinDeg(m) + (0.019993 - 0.000101 * t) * sinDeg(2 * m) +
                   0.000289 * sinDeg(3 * m);
  const double omega = 125.04 - 1934.136 * t;
  const double sunLon = l0 + c - 0.00569 - 0.00478 * sinDeg(omega);
  const double e2 = 0.016708634 * 0.016708634;
  const double sunDist = 149597870.7 * 1.000001018 * (1 - e2) / (1 + 0.016708634 * cosDeg(m + c));

  // the moon's longitude, latitude and distance
  const double lp = 218.3164477 + 481267.88123421 * t;
  const double d = 297.8501921 + 445267.1114034 * t;
  const double mm = 357.5291092 + 35999.0502909 * t;
  const double mp = 134.9633964 + 477198.8675055 * t;
  const double f = 93.2720950 + 483202.0175233 * t;
  const double e = 1 - 0.002516 * t;
  const double moonLon =
      lp + 6.288774 * sinDeg(mp) + 1.274027 * sinDeg(2 * d - mp) + 0.658314 * sinDeg(2 * d) +
      0.213618 * sinDeg(2 * mp) - 0.185116 * e * sinDeg(mm) - 0.114332 * sinDeg(2 * f) +
      0.058793 * sinDeg(2 * d - 2 * mp) + 0.057066 * e * sinDeg(2 * d - mm - mp) + 0.053322 * sinDeg(2 * d + mp) +
      0.045758 * e * sinDeg(2 * d - mm) - 0.040923 * e * sinDeg(mm - mp) - 0.034720 * sinDeg(d) -
      0.030383 * e * sinDeg(mm + mp) + 0.015327 * sinDeg(2 * d - 2 * f) - 0.012528 * sinDeg(mp + 2 * f) +
      0.010980 * sinDeg(mp - 2 * f) + 0.010675 * sinDeg(4 * d - mp) + 0.010034 * sinDeg(3 * mp) +
      0.008548 * sinDeg(4 * d - 2 * mp);
  const double moonLat =
      5.128122 * sinDeg(f) + 0.280602 * sinDeg(mp + f) + 0.277693 * sinDeg(mp - f) +
      0.173237 * sinDeg(2 * d - f) + 0.055413 * sinDeg(2 * d - f + mp) + 0.046271 * sinDeg(2 * d - f - mp) +
      0.032573 * sinDeg(2 * d + f) + 0.017198 * sinDeg(2 * mp + f) + 0.009266 * sinDeg(2 * d + mp - f) +
      0.008822 * sinDeg(2 * mp - f);
  const double moonDist = 385000.56 - 20905.355 * cosDeg(mp) - 3699.111 * cosDeg(2 * d - mp) -
                          2955.968 * cosDeg(2 * d) - 569.925 * cosDeg(2 * mp);

  const double eps = 23.439291 - 0.0130042 * t + 0.00256 * cosDeg(omega);
  double sunRa, sunDec, moonRa, moonDec;
  equatorial(sunLon, 0, eps, sunRa, sunDec);
  equatorial(moonLon, moonLat, eps, moonRa, moonDec);

  MoonState s;
  // how much is lit, and growing or shrinking
  const double cosElong = fmax(-1.0, fmin(1.0, cosDeg(moonLat) * cosDeg(moonLon - sunLon)));
  const double elong = degOf(acos(cosElong));
  s.phaseAngle = degOf(atan2(sunDist * sinDeg(elong), moonDist - sunDist * cosDeg(elong)));
  s.fraction = (1 + cosDeg(s.phaseAngle)) / 2;
  s.waxing = wrap360(moonLon - sunLon) < 180;

  // where the lit edge points, from celestial north towards east
  const double dra = sunRa - moonRa;
  const double chi = wrap360(degOf(atan2(cosDeg(sunDec) * sinDeg(dra),
                                         sinDeg(sunDec) * cosDeg(moonDec) -
                                             cosDeg(sunDec) * sinDeg(moonDec) * cosDeg(dra))));
  s.limbFromUp = chi;
  s.northFromUp = 0;
  s.altitude = 0;
  if (hasPlace)
  {
    // the hour angle from the local sidereal time, then the parallactic angle,
    // which turns celestial north into "up" for this observer
    const double gmst = 280.46061837 + 360.98564736629 * (jd - 2451545.0) + 0.000387933 * t * t;
    const double h = wrap360(gmst + longitude - moonRa);
    const double phi = latitude * M_PI / 180, dlt = moonDec * M_PI / 180, hr = h * M_PI / 180;
    s.altitude = degOf(asin(sin(phi) * sin(dlt) + cos(phi) * cos(dlt) * cos(hr)));
    const double q = degOf(atan2(sin(hr), tan(phi) * cos(dlt) - sin(dlt) * cos(hr)));
    s.limbFromUp = wrap360(chi - q);
    s.northFromUp = wrap360(-q);
  }
  return s;
}

void renderMoon(const MoonState &s, bool clockSet, uint8_t *out)
{
  const float phase = s.phaseAngle * M_PI / 180, limb = s.limbFromUp * M_PI / 180;
  // the sun seen from the moon, in screen space: x right, y up, z towards us
  const float sunX = sinf(phase) * -sinf(limb), sunY = sinf(phase) * cosf(limb), sunZ = cosf(phase);
  const float north = s.northFromUp * M_PI / 180;
  const float cn = cosf(north), sn = sinf(north);
  const float darkSide = 1 - s.fraction;
  const float shine = EARTHSHINE * darkSide * darkSide;

  for (uint8_t row = 0; row < ROWS; row++)
  {
    for (uint8_t col = 0; col < COLS; col++)
    {
      float total = 0;
      for (int sy = 0; sy < SAMPLES; sy++)
      {
        for (int sx = 0; sx < SAMPLES; sx++)
        {
          const float x = (col + (sx + 0.5f) / SAMPLES - CENTRE_X) / RADIUS;
          const float y = -(row + (sy + 0.5f) / SAMPLES - CENTRE_Y) * ASPECT / RADIUS;
          const float r2 = x * x + y * y;
          if (r2 >= 1)
          {
            continue;
          }
          if (!clockSet)
          {
            total += WAITING * (r2 > 0.75f ? 1 : 0);
            continue;
          }
          const float z = sqrtf(1 - r2);
          const float light = smoothstep(-TERMINATOR, TERMINATOR, x * sunX + y * sunY + z * sunZ);
          // the face turned so its north points where the moon's north is in this sky
          const float fx = x * cn + y * sn;
          const float fy = -x * sn + y * cn;
          total += albedo(fx, fy) * light + shine * (1 - light);
        }
      }
      const float value = total / (SAMPLES * SAMPLES);
      // LEDs at half power look much brighter than half: squared keeps it natural
      uint8_t brightness = static_cast<uint8_t>(value * value * 255.0f + 0.5f);
      if (brightness < FLOOR)
      {
        brightness = 0;
      }
      out[row * COLS + col] = brightness;
    }
  }
}
