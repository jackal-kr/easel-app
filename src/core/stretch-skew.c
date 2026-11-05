/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "stretch-skew.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* The tangent of each whole degree from 0 to 89: a table, so that an S never depends on a machine's tan (P7.4). */
static const double tangent[] = {
  0.0, 0.017455064928217585, 0.03492076949174773, 0.05240777928304121,
  0.069926811943510414, 0.087488663525924007, 0.10510423526567647, 0.1227845609029046,
  0.14054083470239145, 0.15838444032453627, 0.17632698070846498, 0.19438030913771848,
  0.21255656167002213, 0.23086819112556312, 0.24932800284318068, 0.2679491924311227,
  0.28674538575880792, 0.30573068145866039, 0.32491969623290629, 0.34432761328966527,
  0.36397023426620234, 0.38386403503541577, 0.40402622583515679, 0.42447481620960476,
  0.44522868530853621, 0.46630765815499858, 0.48773258856586144, 0.50952544949442879,
  0.53170943166147877, 0.55430905145276899, 0.57735026918962573, 0.60086061902756038,
  0.62486935190932746, 0.64940759319751062, 0.67450851684242674, 0.70020753820970971,
  0.7265425280053609, 0.75355405010279419, 0.7812856265067174, 0.80978403319500702,
  0.83909963117727993, 0.86928673781622667, 0.90040404429783993, 0.93251508613766176,
  0.96568877480707394, 0.99999999999999989, 1.0355303137905696, 1.0723687100246826,
  1.110612514829193, 1.1503684072210094, 1.19175359259421, 1.2348971565350515,
  1.2799416321930788, 1.3270448216204098, 1.3763819204711734, 1.4281480067421144,
  1.4825609685127403, 1.5398649638145829, 1.6003345290410507, 1.6642794823505183,
  1.7320508075688767, 1.8040477552714236, 1.8807264653463318, 1.9626105055051504,
  2.050303841579296, 2.1445069205095586, 2.2460367739042164, 2.3558523658237531,
  2.4750868534162964, 2.6050890646938005, 2.7474774194546216, 2.9042108776758222,
  3.0776835371752531, 3.2708526184841404, 3.4874144438409087, 3.7320508075688776,
  4.0107809335358455, 4.3314758742841573, 4.7046301094784511, 5.1445540159703071,
  5.6712818196177066, 6.3137515146750411, 7.115369722384207, 8.1443464279745932,
  9.5143644542225871, 11.430052302761348, 14.300666256711942, 19.081136687728161,
  28.636253282915515, 57.289961630759144,
};

/* The tangent of d + 1/2 degrees, d from 0 to 88: where the rounding to whole degrees changes. */
static const double tangent_half[] = {
  0.0087268677907587893, 0.026185921569186931, 0.043660942908512058, 0.061162620150484306,
  0.078701706824618439, 0.096289048197538613, 0.11393560830164549, 0.13165249758739583,
  0.14945100134912781, 0.16734260908141957, 0.18533904493153439, 0.20345229942369936,
  0.22169466264293988, 0.24007875908011603, 0.25861758435589027, 0.27732454405983847,
  0.29621349496208027, 0.31529878887898349, 0.33459531950207316, 0.35411857253069801,
  0.37388467948480469, 0.39391047561494241, 0.41421356237309503, 0.43481237496093361,
  0.45572625553258467, 0.47697553269816018, 0.49858160805343149, 0.52056705055174624,
  0.54295569963843693, 0.56577277818777016, 0.58904501642055096, 0.612800788139932,
  0.63707026080749318, 0.66188556119569153, 0.68728095860161309, 0.71329306789700531,
  0.73996107502848762, 0.76732698797896037, 0.79543591666782854, 0.82433638581749569,
  0.8540806854634666, 0.8847252645559438, 0.91633117401742337, 0.94896456671487961,
  0.98269726311568995, 1.0176073929721252, 1.0537801252809622, 1.0913085010692714,
  1.1302943863617527, 1.1708495661125391, 1.2130970040929328, 1.2571722989189547,
  1.3032253728412058, 1.3514224379458082, 1.4019482944763357, 1.4550090286724449,
  1.5108351936149012, 1.5696855771174902, 1.6318516871287894, 1.6976631193260889,
  1.7674940162428909, 1.8417708860334581, 1.9209821269711662, 2.0056897082590202,
  2.0965435990881738, 2.1942997311650374, 2.2998425472362567, 2.4142135623730949,
  2.5386478956643073, 2.674621493926824, 2.8239128856008011, 2.9886849627428935,
  3.1715948023632135, 3.3759434225912446, 3.6058835087608729, 3.8667130948987372,
  4.1652997700904164, 4.5107085036620562, 4.9151570310712049, 5.3955171743191377,
  5.9757643644330667, 6.6911562383174124, 7.5957541127251424, 8.776887356869949,
  10.38539708013815, 12.706204736174696, 16.349855476099663, 22.903765548431192,
  38.188459297025631,
};

/* a / b, rounded to the nearest, halves up; a >= 0, b > 0. */
static int64_t
round_div (int64_t a, int64_t b)
{
  return (2 * a + b) / (2 * b);
}

int
easel_ss_percent (int size, int pct)
{
  int64_t v = round_div ((int64_t) size * pct, 100);

  return v < 1 ? 1 : v > EASEL_SS_MAX_SIZE ? EASEL_SS_MAX_SIZE : (int) v;
}

int
easel_ss_percent_unlimited (int size, int pct)
{
  int64_t v = round_div ((int64_t) size * pct, 100);

  return v < 1 ? 1 : (int) v;
}

int
easel_ss_to_percent (int px, int size)
{
  int64_t v = round_div ((int64_t) px * 100, size);

  return v < 1 ? 1 : v > 500 ? 500 : (int) v;
}

int
easel_ss_ratio_other (int changed, int from, int other)
{
  int64_t v = round_div ((int64_t) other * changed, from);

  return v < 1 ? 1 : v > INT32_MAX ? INT32_MAX : (int) v;
}

int
easel_ss_ratio_fit (int changed, int from, int other)
{
  int low = 1, high = changed;

  if (easel_ss_ratio_other (changed, from, other) <= EASEL_SS_MAX_SIZE)
    return changed;
  while (low + 1 < high)                  /* easel_ss_ratio_other rises with `changed`; `high` is past the limit */
    {
      int mid = low + (high - low) / 2;

      if (easel_ss_ratio_other (mid, from, other) <= EASEL_SS_MAX_SIZE)
        low = mid;
      else
        high = mid;
    }
  return low;
}

int
easel_ss_skew_amount (int n, int degrees)
{
  int d = abs (degrees);
  int s;

  if (n <= 1 || d > 89)
    return 0;
  s = (int) floor ((n - 1) * tangent[d] + 0.5);
  return degrees < 0 ? -s : s;
}

int
easel_ss_skew_degrees (int s, int n)
{
  double ratio;
  int d = 0;

  if (n <= 1)
    return 0;
  ratio = (double) abs (s) / (n - 1);
  while (d < 89 && ratio >= tangent_half[d])
    d++;
  return s < 0 ? -d : d;
}

int
easel_ss_shift (int i, int n, int s)
{
  int magnitude = abs (s);
  int shift;

  if (n <= 1)
    return 0;
  shift = (int) round_div ((int64_t) i * magnitude, n - 1);
  return s < 0 ? magnitude - shift : shift;
}

void
easel_ss_size (int W, int H, int sh, bool sh_degrees, int sv, bool sv_degrees, int *out_w, int *out_h, int *amount_h, int *amount_v)
{
  int ah = H <= 1 ? 0 : sh_degrees ? easel_ss_skew_amount (H, sh) : sh;
  int w = W + abs (ah);
  int av = w <= 1 ? 0 : sv_degrees ? easel_ss_skew_amount (w, sv) : sv;

  *out_w = w;
  *out_h = H + abs (av);
  *amount_h = ah;
  *amount_v = av;
}

bool
easel_ss_too_large (int w, int h)
{
  return w > EASEL_SS_MAX_SIZE || h > EASEL_SS_MAX_SIZE;
}

bool
easel_ss_make (const uint32_t *src, const uint8_t *mask, int w, int h, int W, int H, int amount_h, int amount_v, uint32_t background,
               uint32_t *dst, uint8_t *dst_mask)
{
  int out_w = W + abs (amount_h), out_h = H + abs (amount_v);
  size_t n = (size_t) W + (size_t) H + (size_t) H + (size_t) out_w;
  int *tables = malloc (n * sizeof *tables);
  int *from_x, *from_y, *shift_h, *shift_v;

  if (tables == NULL)
    return false;
  from_x = tables;
  from_y = from_x + W;
  shift_h = from_y + H;
  shift_v = shift_h + H;
  /* The new pixel k takes the old pixel under its center, as a selection's stretch does (REQ-0015-05). */
  for (int k = 0; k < W; k++)
    from_x[k] = (int) (((int64_t) 2 * k + 1) * w / (2 * (int64_t) W));
  for (int k = 0; k < H; k++)
    from_y[k] = (int) (((int64_t) 2 * k + 1) * h / (2 * (int64_t) H));
  for (int y = 0; y < H; y++)
    shift_h[y] = easel_ss_shift (y, H, amount_h);
  for (int x = 0; x < out_w; x++)
    shift_v[x] = easel_ss_shift (x, out_w, amount_v);
  /* Each new pixel asks where it came from: first through the vertical skew, then the horizontal one, then the stretch. */
  for (int Y = 0; Y < out_h; Y++)
    {
      uint32_t *row = dst != NULL ? dst + (size_t) Y * out_w : NULL;
      uint8_t *mrow = dst_mask != NULL ? dst_mask + (size_t) Y * out_w : NULL;

      for (int X = 0; X < out_w; X++)
        {
          int y = Y - shift_v[X];
          int x = y >= 0 && y < H ? X - shift_h[y] : -1;
          bool reached = x >= 0 && x < W;

          if (reached)
            {
              size_t at = (size_t) from_y[y] * w + from_x[x];

              reached = mask == NULL || mask[at];
              if (row != NULL)
                row[X] = reached ? src[at] : background;
            }
          else if (row != NULL)
            row[X] = background;
          if (mrow != NULL)
            mrow[X] = reached;
        }
    }
  free (tables);
  return true;
}

EaselTransformResult
easel_image_stretch_skew (EaselImage *img, EaselHistory *h, int W, int H, int amount_h, int amount_v, uint32_t background, size_t max_bytes)
{
  int out_w = W + abs (amount_h), out_h = H + abs (amount_v);
  size_t total = (size_t) out_w * (size_t) out_h;
  uint32_t *px;

  if (W == img->width && H == img->height && amount_h == 0 && amount_v == 0)
    return EASEL_TRANSFORM_NOTHING;
  if (total > max_bytes / sizeof *px || (px = malloc (total * sizeof *px)) == NULL)
    return EASEL_TRANSFORM_NO_MEMORY;
  if (!easel_ss_make (img->px, NULL, img->width, img->height, W, H, amount_h, amount_v, background, px, NULL)
      || !easel_history_push_buffer (h, img->width, img->height, img->px))
    {
      free (px);
      return EASEL_TRANSFORM_NO_MEMORY;
    }
  img->px = px;
  img->width = out_w;
  img->height = out_h;
  img->dirty = (EaselRect) { 0, 0, out_w, out_h };
  return EASEL_TRANSFORM_DONE;
}
