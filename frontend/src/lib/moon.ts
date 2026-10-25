// Today's moon phase for the Moon page: the same low-precision formulas as the
// lamp (src/moon_math.cpp, Meeus ch. 25, 47 and 48), only the part the phase needs

const rad = Math.PI / 180;
const sin = (deg: number) => Math.sin(deg * rad);
const cos = (deg: number) => Math.cos(deg * rad);

export interface MoonPhase {
  fraction: number; // 0 new .. 1 full
  waxing: boolean;
  name: string;
}

export const moonPhase = (date: Date): MoonPhase => {
  const jd = date.getTime() / 86400000 + 2440587.5;
  const t = (jd - 2451545) / 36525;

  const m = 357.52911 + 35999.05029 * t;
  const c =
    (1.914602 - 0.004817 * t) * sin(m) +
    (0.019993 - 0.000101 * t) * sin(2 * m) +
    0.000289 * sin(3 * m);
  const sunLon = 280.46646 + 36000.76983 * t + c - 0.00569 - 0.00478 * sin(125.04 - 1934.136 * t);
  const sunDist =
    (149597870.7 * 1.000001018 * (1 - 0.016708634 ** 2)) / (1 + 0.016708634 * cos(m + c));

  const lp = 218.3164477 + 481267.88123421 * t;
  const d = 297.8501921 + 445267.1114034 * t;
  const mm = 357.5291092 + 35999.0502909 * t;
  const mp = 134.9633964 + 477198.8675055 * t;
  const f = 93.272095 + 483202.0175233 * t;
  const e = 1 - 0.002516 * t;
  const moonLon =
    lp +
    6.288774 * sin(mp) +
    1.274027 * sin(2 * d - mp) +
    0.658314 * sin(2 * d) +
    0.213618 * sin(2 * mp) -
    0.185116 * e * sin(mm) -
    0.114332 * sin(2 * f) +
    0.058793 * sin(2 * d - 2 * mp) +
    0.057066 * e * sin(2 * d - mm - mp) +
    0.053322 * sin(2 * d + mp) +
    0.045758 * e * sin(2 * d - mm) -
    0.040923 * e * sin(mm - mp) -
    0.03472 * sin(d) -
    0.030383 * e * sin(mm + mp);
  const moonLat =
    5.128122 * sin(f) + 0.280602 * sin(mp + f) + 0.277693 * sin(mp - f) + 0.173237 * sin(2 * d - f);
  const moonDist =
    385000.56 -
    20905.355 * cos(mp) -
    3699.111 * cos(2 * d - mp) -
    2955.968 * cos(2 * d) -
    569.925 * cos(2 * mp);

  const elong = Math.acos(Math.max(-1, Math.min(1, cos(moonLat) * cos(moonLon - sunLon)))) / rad;
  const phase = Math.atan2(sunDist * sin(elong), moonDist - sunDist * cos(elong)) / rad;
  const fraction = (1 + cos(phase)) / 2;
  const waxing = (((moonLon - sunLon) % 360) + 360) % 360 < 180;

  let name: string;
  if (fraction < 0.02) name = "New moon";
  else if (fraction > 0.98) name = "Full moon";
  else if (Math.abs(fraction - 0.5) < 0.04) name = waxing ? "First quarter" : "Last quarter";
  else name = `${waxing ? "Waxing" : "Waning"} ${fraction < 0.5 ? "crescent" : "gibbous"}`;
  return { fraction, waxing, name };
};
