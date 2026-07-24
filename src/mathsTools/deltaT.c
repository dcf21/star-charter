// deltaT.c
//
// -------------------------------------------------
// Copyright 2015-2026 Dominic Ford
//
// This file is part of StarCharter.
//
// StarCharter is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// StarCharter is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with StarCharter.  If not, see <http://www.gnu.org/licenses/>.
// -------------------------------------------------


#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/strConstants.h"

#include "deltaT.h"

typedef struct {
    double mjd;
    double sigma;
    double delta_t;
} IERS_Row;

// ---------- Code for spline interpolation of delta T ----------

//! Constant arrays for cubic spline polynomials,
//! see http://astro.ukho.gov.uk/nao/lvm/Table-S15.2020.txt
//! The coefficients after 2013 have been modified to include data after 2019.

static const double dt_y0[] = {
    -720, -100, 400, 1000, 1150, 1300, 1500, 1600, 1650, 1720, 1800,
    1810, 1820, 1830, 1840, 1850, 1855, 1860, 1865, 1870, 1875, 1880,
    1885, 1890, 1895, 1900, 1905, 1910, 1915, 1920, 1925, 1930, 1935,
    1940, 1945, 1950, 1953, 1956, 1959, 1962, 1965, 1968, 1971, 1974,
    1977, 1980, 1983, 1986, 1989, 1992, 1995, 1998, 2001, 2004, 2007,
    2010, 2013, 2016, 2019, 2022
};

static const double dt_y1[] = {
    -100, 400, 1000, 1150, 1300, 1500, 1600, 1650, 1720, 1800, 1810,
    1820, 1830, 1840, 1850, 1855, 1860, 1865, 1870, 1875, 1880, 1885,
    1890, 1895, 1900, 1905, 1910, 1915, 1920, 1925, 1930, 1935, 1940,
    1945, 1950, 1953, 1956, 1959, 1962, 1965, 1968, 1971, 1974, 1977,
    1980, 1983, 1986, 1989, 1992, 1995, 1998, 2001, 2004, 2007, 2010,
    2013, 2016, 2019, 2022, 2025
};

static const double dt_a0[] = {
    20371.848, 11557.668, 6535.116, 1650.393, 1056.647, 681.149, 292.343,
    109.127, 43.952, 12.068, 18.367, 15.678, 16.516, 10.804, 7.634,
    9.338, 10.357, 9.04, 8.255,
    2.371, -1.126, -3.21, -4.388, -3.884, -5.017, -1.977, 4.923, 11.142,
    17.479, 21.617, 23.789, 24.418, 24.164, 24.426, 27.05, 28.932,
    30.002, 30.76, 32.652, 33.621, 35.093, 37.956, 40.951, 44.244,
    47.291, 50.361, 52.936, 54.984, 56.373, 58.453, 60.678, 62.898,
    64.083, 64.553, 65.197, 66.061, 66.919, 68.130, 69.250, 69.296
};

static const double dt_a1[] = {
    -9999.586, -5822.27, -5671.519, -753.21, -459.628, -421.345,
    -192.841, -78.697, -68.089, 2.507, -3.481, 0.021, -2.157, -6.018,
    -0.416, 1.642, -0.486, -0.591, -3.456, -5.593, -2.314, -1.893, 0.101,
    -0.531, 0.134, 5.715, 6.828, 6.33, 5.518, 3.02, 1.333, 0.052, -0.419,
    1.645, 2.499, 1.127, 0.737, 1.409, 1.577, 0.868, 2.275, 3.035, 3.157,
    3.199, 3.069, 2.878, 2.354, 1.577, 1.648, 2.235, 2.324, 1.804, 0.674,
    0.466, 0.804, 0.839, 1.005, 1.348, 0.594, -0.227
};

static const double dt_a2[] = {
    776.247, 1303.151, -298.291, 184.811, 108.771, 61.953, -6.572,
    10.505, 38.333, 41.731, -1.126, 4.629, -6.806, 2.944, 2.658, 0.261,
    -2.389, 2.284, -5.148, 3.011, 0.269, 0.152, 1.842, -2.474, 3.138,
    2.443, -1.329, 0.831, -1.643, -0.856, -0.831, -0.449, -0.022, 2.086,
    -1.232, 0.22, -0.61, 1.282, -1.115, 0.406, 1.002, -0.242, 0.364,
    -0.323, 0.193, -0.384, -0.14, -0.637, 0.708, -0.121, 0.21, -0.729,
    -0.402, 0.194, 0.144, -0.109, 0.275, 0.068, -0.822, 0.001
};

static const double dt_a3[] = {
    409.16, -503.433, 1085.087, -25.346, -24.641, -29.414, 16.197, 3.018,
    -2.127, -37.939, 1.918, -3.812, 3.25, -0.096, -0.539, -0.883, 1.558,
    -2.477, 2.72, -0.914, -0.039, 0.563, -1.438, 1.871,
    -0.232, -1.257, 0.72, -0.825, 0.262, 0.008, 0.127, 0.142, 0.702,
    -1.106, 0.614, -0.277, 0.631, -0.799, 0.507, 0.199, -0.414, 0.202,
    -0.229, 0.172, -0.192, 0.081, -0.165, 0.448, -0.276, 0.11, -0.313,
    0.109, 0.199, -0.017, -0.084, 0.128, -0.069, -0.297, 0.274, 0.086
};

static const double dt_ys_end = 2025.0;

//! Integration constants for year < -720 and > ys_end
static const double dt_c1 = 1.007739546148514;
static const double dt_c2 = -150.56787057979514;

//! Table for estimating the errors in Delta T for years in [-2000,2500] based on
//! http://astro.ukho.gov.uk/nao/lvm/
static const double dt_y_tab[] = {
    -2000, -1600, -900, -720, -700, -600, -500, -400, -300, -200, -100, 0,
    100, 200, 300, 400, 500, 700, 800, 900, 1000, 1620, 1660, 1670, 1680,
    1730, 1770, 1800, 1802, 1805, 1809, 1831, 1870, 2025, 2025.5, 2026, 2030, 2040,
    2050, 2100, 2200, 2300, 2400, 2500
};

static const double dt_eps_tab[] = {
    1080, 720, 360, 180, 170, 160, 150, 130, 120, 110, 100, 90, 80, 70,
    60, 50, 40, 30, 25, 20, 15, 20, 15, 10, 5, 2, 1, 0.5, 0.4, 0.3, 0.2,
    0.1, 0.05, 0.1, 0.2, 1, 2, 4, 6, 10, 20, 30, 50, 100
};

#define N_SPLINE (sizeof(dt_y0)/sizeof(dt_y0[0]))
#define N_Y_TAB  (sizeof(dt_y_tab)/sizeof(dt_y_tab[0]))


//! search_interval - Replacement for numpy.searchsorted(..., side="right") - 1
//! @param [in] array - The array of doubles to search.
//! @param [in] n - The length of the array to search.
//! @param [in] x - The float value to search for the position of.
//! @return - Position where x would be inserted into the array.
static int search_interval(const double *array, const size_t n, const double x) {
    size_t lo = 0;
    size_t hi = n;

    while (lo < hi) {
        size_t mid = (lo + hi) / 2;

        if (x < array[mid]) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }

    if (lo == 0)
        return 0;

    return (int) (lo - 1);
}

//! spline - Calculate Delta T by cubic spline polynomial.
//! @param [in] year - Year at which to evaluate delta_T.
//! @param [in] d_alpha - A parameter that allows the change in the default tidal acceleration.
//! @return - Interpolated spline.
static double spline(const double year, const double d_alpha) {
    const int i = search_interval(dt_y0, N_SPLINE, year);

    const double t = (year - dt_y0[i]) / (dt_y1[i] - dt_y0[i]);

    double dt = dt_a0[i] + t * (dt_a1[i] + t * (dt_a2[i] + t * dt_a3[i]));

    // Add correction to Delta T as a result of using a different value of tidal acceleration
    if (d_alpha != 0.0) {
        const double t_centuries = 0.01 * year - 19.55;

        if (t_centuries < 0.0) {
            dt -= 0.91072 * d_alpha * t_centuries * t_centuries;
        }
    }

    return dt;
}

//! integrated_lod - Integrated lod (deviation of mean solar day from 86400s) equation from
//!   http://astro.ukho.gov.uk/nao/lvm/:
//!   lod = 1.72 t − 3.5 sin(2*pi*(t+0.75)/14) in ms/day, where t = (y - 1825)/100
//!   Using 1ms = 1e-3s and 1 Julian year = 365.25 days,
//!   lod = 0.62823*t - 1.278375*sin(2*pi/14*(t + 0.75) in s/year
//!   Integrate the equation gives
//!   C + 31.4115*t^2 + 894.8625/pi*cos(2*pi/14*(t + 0.75))
//!   in seconds.
//! @param [in] year - Year at which to evaluate delta_T.
//! @param [in] c - C is the integration constant.
//! @param [in] d_alpha - A parameter that allows the change in the default tidal acceleration.
//! @return - Integrated lod.
static double integrated_lod(const double year, const double c, const double d_alpha) {
    const double t = 0.01 * (year - 1825.0);
    double dt = c + 31.4115 * t * t + 284.8435805251424 * cos(0.4487989505128276 * (t + 0.75));

    // Add correction to Delta T as a result of using a different value of tidal acceleration
    if (d_alpha != 0.0) {
        const double t2 = pow(0.01 * year - 19.55, 2.0);
        const double ts2 = pow(0.01 * dt_ys_end - 19.55, 2.0);
        const double offset = (year > dt_ys_end) ? ts2 : 0.0;

        dt -= 0.91072 * d_alpha * (t2 - offset);
    }

    return dt;
}

//! delta_t_parametric - Compute Delta T using the fitting and extrapolation formulae by
//! Stephenson et al (2016) and Morrison et al (2021). See http://astro.ukho.gov.uk/nao/lvm/
//!
//! The tidal acceleration adopted by Stephenson et al and Morrison et al is:
//! alpha = -25.82"/century^2.
//! d_alpha = new tidal acceleration in "/century^2 - (-25.82)
//!
//! @param [in] year - Year at which to evaluate delta_T.
//! @param [in] d_alpha - A parameter that allows the change in the default tidal acceleration.
//! @return - delta_T, measured in seconds.
double delta_t_parametric(const double year, const double d_alpha) {
    if (year < -720.0) {
        return integrated_lod(year, dt_c1, d_alpha);
    }

    if (year > dt_ys_end) {
        return integrated_lod(year, dt_c2, d_alpha);
    }

    return spline(year, d_alpha);
}

//! delta_t_error_parametric - Estimate the uncertainty in Delta T based on the tables in
//! http://astro.ukho.gov.uk/nao/lvm/
//! The table only gives the error estimate for y in [-2000,2500]. The error outside this
//! range is estimated by quadratic functions, but they are not reliable.
//!
//! @param [in] year - Year at which to evaluate delta_T.
//! @return - Uncertainty in delta_T, measured in seconds.
double delta_t_error_parametric(const double year) {
    const double k1 = 0.74e-4;
    const double k2 = 2.2e-4;

    if (year < dt_y_tab[0]) {
        return k1 * pow(year - 1825.0, 2.0);
    }

    if (year >= dt_y_tab[N_Y_TAB - 1]) {
        return k2 * pow(year - 1825.0, 2.0);
    }

    const int i = search_interval(dt_y_tab, N_Y_TAB, year);
    return dt_eps_tab[i];
}

// ---------- Code for interpolation of IERS tables of delta T ----------

//! load_iers_csv - Load IERS table of delta T values from CSV file.
//! @param [in] step - Step size to use when reading the CSV file (days)
//! @param [out] jd_start - Julian day at which IERS data starts
//! @param [out] jd_end - Julian day at which IERS data ends
//! @param [out] c - Integration constant
//! @param [out] delta_t - Output array of delta_t values read.
//! @param [out] sigma - Output array of sigma values read.
//! @param [out] count - The number of rows we read; the length of the output arrays <delta_t> and <sigma>.
//! @return - Boolean flag: 1 on success; 0 on failure.
static int load_iers_csv(const int step, double *jd_start, double *jd_end,
                         double *c, double **delta_t, double **sigma, size_t *count) {
    // Work out the filename of the CSV table of IERS delta-t values
    char filename[FNAME_LENGTH];

    snprintf(filename, FNAME_LENGTH, "%s/mathsTools/dcf_ast_delta_t_iers.csv", SRCDIR);
    if (DEBUG) {
        char msg[LSTR_LENGTH];
        snprintf(msg, FNAME_LENGTH, "Fetching delta-t data from file <%s>.", filename);
        stch_log(msg);
    }

    // String buffer to read lines of the CSV file into
    char line[1024];

    size_t capacity = 65536; // Size of temporary buffer to read the CSV data into
    size_t n = 0; // Count the number of rows we have read

    // Malloc a buffer to read the CSV data into
    IERS_Row *rows = malloc(capacity * sizeof(IERS_Row));
    if (rows == NULL) return 0;

    // Open CSV file and start reading
    FILE *fp = fopen(filename, "rt");
    if (!fp) {
        free(rows);
        return 0;
    }

    // Skip header line of the CSV file
    if (!fgets(line, sizeof(line), fp)) {
        free(rows);
        return 0;
    }

    // Keep track of line number within the CSV file
    size_t line_number = 0;

    // Read a line of CSV data
    while (fgets(line, sizeof(line), fp)) {
        // Ignore lines if step != 1
        if ((line_number % step) != 0) {
            line_number++;
            continue;
        }

        // Parse columns of data
        double col[7];

        if (sscanf(line, "%lf,%lf,%lf,%lf,%lf,%lf,%lf",
                   &col[0], &col[1], &col[2], &col[3], &col[4], &col[5], &col[6]) != 7) {
            // Skip lines with an insufficient number of elements
            line_number++;
            continue;
        }

        // If temporary buffer is full, enlarge it now
        if (n == capacity) {
            capacity *= 2;

            rows = realloc(rows, capacity * sizeof(IERS_Row));
            if (rows == NULL) {
                fclose(fp);
                return 0;
            }
        }

        // Feed CSV data into temporary buffer
        rows[n].mjd = col[3];
        rows[n].sigma = col[5];
        rows[n].delta_t = col[6];

        // Keep track of the number of lines we've read
        n++;
        line_number++;
    }

    // Close CSV file
    fclose(fp);

    // Allocate the final data structures
    *delta_t = malloc(n * sizeof(double));
    *sigma = malloc(n * sizeof(double));

    if ((*delta_t == NULL) || (*sigma == NULL)) {
        free(rows);
        return 0;
    }

    // Transfer the rows we read into the final data structures
    for (size_t i = 0; i < n; i++) {
        (*delta_t)[i] = rows[i].delta_t;
        (*sigma)[i] = rows[i].sigma;
    }

    // Compute range of the data we read
    *jd_start = 2400000.5 + rows[0].mjd;
    *jd_end = 2400000.5 + rows[n - 1].mjd;

    const double y = 2000.0 + (rows[n - 1].mjd - 51544.0) / 365.2425;
    *c = rows[n - 1].delta_t - integrated_lod(y, 0.0, 0.0);
    *count = n;

    // Free temporary workspace
    free(rows);

    // Success
    return 1;
}

//! delta_t_init - Initialise a DeltaTCalculator struct instance.
//! @param [out] calc - Initialised DeltaTCalculator struct instance.
//! @param [in] csv_filename - Filename of the CSV file from which we should read the IERS tabulated delta_t values
//! @return  - Boolean flag: 1 on success; 0 on failure.
int delta_t_init(DeltaTCalculator *calc) {
    calc->s_iers = 1;

    return load_iers_csv(calc->s_iers, &calc->jd_iers_start, &calc->jd_iers_end, &calc->c_iers,
                         &calc->delta_t_iers, &calc->sigma, &calc->n_iers);
}

//! delta_t_free - Free a DeltaTCalculator struct instance.
//! @param [in] calc - Initialised DeltaTCalculator struct instance.
void delta_t_free(DeltaTCalculator *calc) {
    free(calc->delta_t_iers);
    free(calc->sigma);

    calc->delta_t_iers = NULL;
    calc->sigma = NULL;
    calc->n_iers = 0;
}

//! delta_t_interpolate_iers - Calculate Delta T by linear interpolating the IERS Delta T data.
//! @param [in] calc - Initialised DeltaTCalculator struct instance.
//! @param [in] jd - Julian day number at which to calculate Delta T.
//! @return - Delta T. If jd is out of range of the <delta_t_iers> array, return NaN.
double delta_t_interpolate_iers(const DeltaTCalculator *calc, const double jd) {
    if (jd < calc->jd_iers_start || jd >= calc->jd_iers_end) {
        return NAN;
    }

    double d = (jd - calc->jd_iers_start) / calc->s_iers;
    size_t i = (size_t) d;
    d -= floor(d);

    return calc->delta_t_iers[i] + d * (calc->delta_t_iers[i + 1] - calc->delta_t_iers[i]);
}

//! delta_t_interpolate_iers_error - Estimate the error of Delta T computed from <delta_t_interpolate_iers()>.
//! @param [in] calc - Initialised DeltaTCalculator struct instance.
//! @param [in] jd - Julian day number at which to calculate Delta T.
//! @return - Uncertainty in delta T. If jd is out of range of the <delta_t_iers> array, return NaN.
double delta_t_interpolate_iers_error(const DeltaTCalculator *calc, const double jd) {
    if (jd < calc->jd_iers_start || jd >= calc->jd_iers_end) {
        return NAN;
    }

    size_t i = (size_t) ((jd - calc->jd_iers_start) / calc->s_iers);
    double interp = 0.5 * fabs(calc->delta_t_iers[i + 1] - calc->delta_t_iers[i]);

    return fmax(interp, calc->sigma[i]);
}

//! delta_t_y_from_jd - Convert a Julian day number to a Julian year number.
//! @param [in] jd - Input Julian day number.
//! @return - Julian year number.
double delta_t_y_from_jd(const double jd) {
    if (jd >= 2299160.5) {
        return 2000.0 + (jd - 2451544.5) / 365.2425;
    }

    return (jd + 0.5) / 365.25 - 4712.0;
}

// ---------- Code for hybris use of IERS tables of delta T, and splines for historic epochs ----------

//! delta_t - Compute Delta T at Julian date jd using a hybrid method:
//!   * If jd is in the range of IERS Delta T table, compute Delta T by linear interpolating the IERS Delta T data.
//!   * If jd is outside the range of the IERS data, compute Delta T by cubic spline interpolation or extrapolation
//!     by the integrated lod.
//!
//! @param [in] calc - Initialised DeltaTCalculator struct instance.
//! @param [in] jd - Julian day number at which to calculate Delta T.
//! @return - Delta T (seconds).
double delta_t(const DeltaTCalculator *calc, double jd) {
    // If request is in the future, return current Delta-T value
    if (jd >= calc->jd_iers_end) jd = calc->jd_iers_end - 1.0;

    if (jd >= calc->jd_iers_start) {
        return delta_t_interpolate_iers(calc, jd);
    }

    return delta_t_parametric(delta_t_y_from_jd(jd), 0);
}

//! delta_t_error - Estimate the uncertainty in Delta T.
//!
//! @param [in] calc - Initialised DeltaTCalculator struct instance.
//! @param [in] jd - Julian day number at which to calculate Delta T.
//! @return - Uncertainty in Delta T (seconds).
double delta_t_error(const DeltaTCalculator *calc, const double jd) {
    if (jd >= calc->jd_iers_end ||
        jd < calc->jd_iers_start) {
        return delta_t_error_parametric(delta_t_y_from_jd(jd));
    }

    return delta_t_interpolate_iers_error(calc, jd);
}

//! tt_from_utc - Convert a Julian day number in UTC to TT.
//!
//! @param [in] calc - Initialised DeltaTCalculator struct instance.
//! @param [in] jd_utc - Julian day number in UTC (days).
//! @param [in] has_assumed_delta_t - Use specified value of Delta_T.
//! @param [in] assumed_delta_t - Optionally, specify the value of Delta_T to assume (seconds).
//! @return - Julian day number in TT (days).
double tt_from_utc(const DeltaTCalculator *calc, const double jd_utc,
                   const int has_assumed_delta_t, const double assumed_delta_t) {
    double best_delta_t;

    if (has_assumed_delta_t) {
        best_delta_t = assumed_delta_t;
    } else {
        best_delta_t = delta_t(calc, jd_utc);
    }

    return jd_utc + best_delta_t / SECONDS_PER_DAY;
}

//! utc_from_tt - Convert a Julian day number in TT to UTC.
//!
//! @param [in] calc - Initialised DeltaTCalculator struct instance.
//! @param [in] jd_tt - Julian day number in TT (days).
//! @param [in] has_assumed_delta_t - Use specified value of Delta_T.
//! @param [in] assumed_delta_t - Optionally, specify the value of Delta_T to assume (seconds).
//! @return - Julian day number in UTC (days).
double utc_from_tt(const DeltaTCalculator *calc, const double jd_tt,
                   const int has_assumed_delta_t, const double assumed_delta_t) {
    double best_delta_t;

    if (has_assumed_delta_t) {
        best_delta_t = assumed_delta_t;
    } else {
        best_delta_t = delta_t(calc, jd_tt);
    }

    return jd_tt - best_delta_t / SECONDS_PER_DAY;
}
