/*
 * main.c: entry point for sigutils
 * Creation date: Thu Oct 20 22:56:46 2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include <sigutils/sampling.h>
#include <sigutils/ncqo.h>
#include <sigutils/iir.h>
#include <sigutils/agc.h>
#include <sigutils/pll.h>

#include <sigutils/sigutils.h>

#include "test.h"

#define SU_TEST_AGC_SIGNAL_FREQ 0.025
#define SU_TEST_AGC_WINDOW (1. / SU_TEST_AGC_SIGNAL_FREQ)

#define SU_TEST_PLL_SIGNAL_FREQ 0.025
#define SU_TEST_PLL_BANDWIDTH   (1e-4)

#define SU_TEST_COSTAS_SYMBOL_PERIOD 0x200
#define SU_TEST_COSTAS_SIGNAL_FREQ 1e-4
#define SU_TEST_COSTAS_BANDWIDTH (1. / (2 * SU_TEST_COSTAS_SYMBOL_PERIOD))


SUBOOL
su_test_ncqo(su_test_context_t *ctx)
{
  SUBOOL ok = SU_FALSE;
  SUFLOAT *buffer = NULL;
  unsigned int p = 0;
  su_ncqo_t ncqo = su_ncqo_INITIALIZER;

  SU_TEST_ASSERT(buffer = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));

  SU_TEST_START(ctx);

  su_ncqo_init(&ncqo, 1);

  /* Test in-phase signal */
  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p)
    buffer[p] = su_ncqo_read_i(&ncqo);

  SU_TEST_ASSERT(
      SUFLOAT_EQUAL(
          su_test_buffer_mean(buffer, SU_TEST_SIGNAL_BUFFER_SIZE), 0));

  SU_TEST_ASSERT(
      SUFLOAT_EQUAL(
          su_test_buffer_pp(buffer, SU_TEST_SIGNAL_BUFFER_SIZE), 2));

  /* Test quadrature signal */
  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p)
    buffer[p] = su_ncqo_read_q(&ncqo);

  SU_TEST_ASSERT(
      SUFLOAT_EQUAL(
          su_test_buffer_mean(buffer, SU_TEST_SIGNAL_BUFFER_SIZE), 0));

  SU_TEST_ASSERT(
      SUFLOAT_EQUAL(
          su_test_buffer_pp(buffer, SU_TEST_SIGNAL_BUFFER_SIZE), 0));

  /* Modify phase */
  su_ncqo_set_phase(&ncqo, PI / 2);

  /* Test in-phase signal */
  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p)
    buffer[p] = su_ncqo_read_i(&ncqo);

  SU_TEST_ASSERT(
      SUFLOAT_EQUAL(
          su_test_buffer_mean(buffer, SU_TEST_SIGNAL_BUFFER_SIZE), 0));

  SU_TEST_ASSERT(
      SUFLOAT_EQUAL(
          su_test_buffer_pp(buffer, SU_TEST_SIGNAL_BUFFER_SIZE), 0));

  /* Test quadrature signal */
  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p)
    buffer[p] = su_ncqo_read_q(&ncqo);

  SU_TEST_ASSERT(
      SUFLOAT_EQUAL(
          su_test_buffer_mean(buffer, SU_TEST_SIGNAL_BUFFER_SIZE), 0));

  SU_TEST_ASSERT(
      SUFLOAT_EQUAL(
          su_test_buffer_pp(buffer, SU_TEST_SIGNAL_BUFFER_SIZE), 2));

  /* Test constant signal */
  su_ncqo_set_phase(&ncqo, 0);
  su_ncqo_set_freq(&ncqo, 0);

  /* Test in-phase signal */
  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p) {
    SU_TEST_ASSERT(SUFLOAT_EQUAL(su_ncqo_read_i(&ncqo), 1));
    SU_TEST_ASSERT(SUFLOAT_EQUAL(su_ncqo_read_q(&ncqo), 0));
  }

  ok = SU_TRUE;

done:
  SU_TEST_END(ctx);

  if (buffer != NULL)
    free(buffer);

  return ok;
}

SUBOOL
su_test_butterworth_lpf(su_test_context_t *ctx)
{
  SUBOOL ok = SU_FALSE;
  SUFLOAT *buffer = NULL;
  unsigned int p = 0;
  su_ncqo_t ncqo = su_ncqo_INITIALIZER;
  su_iir_filt_t lpf = su_iir_filt_INITIALIZER;

  SU_TEST_START_TICKLESS(ctx);

  SU_TEST_ASSERT(buffer = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));

  SU_TEST_ASSERT(su_iir_bwlpf_init(&lpf, 5, 0.25));

  SU_TEST_TICK(ctx);

  su_ncqo_init(&ncqo, 1);

  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p)
    buffer[p] = su_iir_filt_feed(
        &lpf,
        su_ncqo_read_i(&ncqo));

  if (ctx->dump_results) {
    SU_TEST_ASSERT(
        su_test_buffer_dump_matlab(
            buffer,
            SU_TEST_SIGNAL_BUFFER_SIZE,
            "hi.m",
            "hi"));

    printf(" hi pp: " SUFLOAT_FMT "\n", su_test_buffer_pp(buffer, SU_TEST_SIGNAL_BUFFER_SIZE));
    printf(" hi mean: " SUFLOAT_FMT "\n", su_test_buffer_mean(buffer, SU_TEST_SIGNAL_BUFFER_SIZE));
    printf(" hi std: " SUFLOAT_FMT "\n", su_test_buffer_std(buffer, SU_TEST_SIGNAL_BUFFER_SIZE));
  }

  su_ncqo_set_freq(&ncqo, .125);

  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p)
    buffer[p] = su_iir_filt_feed(
        &lpf,
        su_ncqo_read_i(&ncqo));

  if (ctx->dump_results) {
    SU_TEST_ASSERT(
        su_test_buffer_dump_matlab(
            buffer,
            SU_TEST_SIGNAL_BUFFER_SIZE,
            "lo.m",
            "lo"));

    printf(" lo pp: " SUFLOAT_FMT "\n", su_test_buffer_pp(buffer, SU_TEST_SIGNAL_BUFFER_SIZE));
    printf(" lo mean: " SUFLOAT_FMT "\n", su_test_buffer_mean(buffer, SU_TEST_SIGNAL_BUFFER_SIZE));
    printf(" lo std: " SUFLOAT_FMT "\n", su_test_buffer_std(buffer, SU_TEST_SIGNAL_BUFFER_SIZE));
  }

  ok = SU_TRUE;

done:
  SU_TEST_END(ctx);

  if (buffer != NULL)
    free(buffer);

  su_iir_filt_finalize(&lpf);

  return ok;
}

SUBOOL
su_test_check_peak(
    const su_test_context_t *ctx,
    const SUFLOAT *buffer,
    unsigned int size,
    unsigned int window,
    SUFLOAT peak_low,
    SUFLOAT peak_high)
{
  unsigned int i, j;
  SUFLOAT peak = 0;
  SUBOOL ok = SU_FALSE;

  /* Skip window. AGC needs some time to start */
  for (i = window; i < size - window; ++i) {
    peak = 0;
    for (j = 0; j < window; ++j) {
      if (SU_ABS(buffer[i + j]) > peak)
        peak = SU_ABS(buffer[i + j]);
    }

    SU_TEST_ASSERT(peak >= peak_low);
    SU_TEST_ASSERT(peak <= peak_high);
  }

  ok = SU_TRUE;

done:
  return ok;
}

SUBOOL
su_test_agc_transient(su_test_context_t *ctx)
{
  SUBOOL ok = SU_FALSE;
  SUFLOAT *buffer = NULL;
  SUFLOAT t;
  su_agc_t agc = su_agc_INITIALIZER;
  struct su_agc_params agc_params = su_agc_params_INITIALIZER;

  unsigned int p = 0;

  SU_TEST_START_TICKLESS(ctx);

  /* Initialize */
  SU_TEST_ASSERT(buffer = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));
  agc_params.delay_line_size  = 10;
  agc_params.mag_history_size = 10;
  agc_params.fast_rise_t      = 2;
  agc_params.fast_fall_t      = 4;

  agc_params.slow_rise_t      = 20;
  agc_params.slow_fall_t      = 40;

  agc_params.threshold        = SU_DB(2e-2);

  agc_params.hang_max         = 30;

  if (!su_agc_init(&agc, &agc_params))
    goto done;

  /* Create a spike */
  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p) {
    t = p - SU_TEST_SIGNAL_BUFFER_SIZE / 2;
    buffer[p] = 1e-2 * rand() / (double) RAND_MAX;

    buffer[p] += SU_EXP(-t * t);
  }

  if (ctx->dump_results) {
    SU_TEST_ASSERT(
        su_test_buffer_dump_matlab(
            buffer,
            SU_TEST_SIGNAL_BUFFER_SIZE,
            "spike.m",
            "spike"));
  }

  SU_TEST_TICK(ctx);

  /* Feed the AGC and put samples back in buffer */
  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p) {
    buffer[p] = SU_C_REAL(su_agc_feed(&agc, buffer[p]));
  }

  ok = SU_TRUE;

done:
  SU_TEST_END(ctx);

  su_agc_finalize(&agc);

  if (buffer != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_buffer_dump_matlab(
              buffer,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "corrected.m",
              "corrected"));
    }

    free(buffer);
  }

  return ok;
}

SUBOOL
su_test_agc_steady_rising(su_test_context_t *ctx)
{
  SUBOOL ok = SU_FALSE;
  SUFLOAT *buffer = NULL;
  SUFLOAT t;
  su_agc_t agc = su_agc_INITIALIZER;
  su_ncqo_t ncqo = su_ncqo_INITIALIZER;
  struct su_agc_params agc_params = su_agc_params_INITIALIZER;
  unsigned int p = 0;

  SU_TEST_START_TICKLESS(ctx);

  /* Initialize */
  SU_TEST_ASSERT(buffer = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));
  agc_params.delay_line_size  = 10;
  agc_params.mag_history_size = 10;
  agc_params.fast_rise_t      = 2;
  agc_params.fast_fall_t      = 4;

  agc_params.slow_rise_t      = 20;
  agc_params.slow_fall_t      = 40;

  agc_params.threshold        = SU_DB(2e-2);

  agc_params.hang_max         = 30;
  agc_params.slope_factor     = 0;

  SU_TEST_ASSERT(su_agc_init(&agc, &agc_params));

  su_ncqo_init(&ncqo, SU_TEST_AGC_SIGNAL_FREQ);

  /* Create a rising sinusoid */
  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p) {
    t = p - SU_TEST_SIGNAL_BUFFER_SIZE / 2;
    buffer[p] = 1e-2 * rand() / (double) RAND_MAX;

    buffer[p] += 0.2 * floor(1 + 5 * p / SU_TEST_SIGNAL_BUFFER_SIZE) * su_ncqo_read_i(&ncqo);
  }

  if (ctx->dump_results) {
    SU_TEST_ASSERT(
        su_test_buffer_dump_matlab(
            buffer,
            SU_TEST_SIGNAL_BUFFER_SIZE,
            "steady.m",
            "steady"));
  }

  SU_TEST_TICK(ctx);

  /* Feed the AGC and put samples back in buffer */
  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p) {
    buffer[p] = SU_C_REAL(su_agc_feed(&agc, buffer[p]));
  }

  /* TODO: Improve levels */
  SU_TEST_ASSERT(
      su_test_check_peak(
          ctx,
          buffer,
          SU_TEST_SIGNAL_BUFFER_SIZE,
          2 * SU_TEST_AGC_WINDOW,
          SU_AGC_RESCALE * 0.9 ,
          SU_AGC_RESCALE * 1.1));

  ok = SU_TRUE;

done:
  SU_TEST_END(ctx);

  su_agc_finalize(&agc);

  if (buffer != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_buffer_dump_matlab(
              buffer,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "corrected.m",
              "corrected"));
    }

    free(buffer);
  }

  return ok;
}

SUBOOL
su_test_agc_steady_falling(su_test_context_t *ctx)
{
  SUBOOL ok = SU_FALSE;
  SUFLOAT *buffer = NULL;
  SUFLOAT t;
  su_agc_t agc = su_agc_INITIALIZER;
  su_ncqo_t ncqo = su_ncqo_INITIALIZER;
  struct su_agc_params agc_params = su_agc_params_INITIALIZER;
  unsigned int p = 0;

  SU_TEST_START_TICKLESS(ctx);

  /* Initialize */
  SU_TEST_ASSERT(buffer = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));
  agc_params.delay_line_size  = 10;
  agc_params.mag_history_size = 10;
  agc_params.fast_rise_t      = 2;
  agc_params.fast_fall_t      = 4;

  agc_params.slow_rise_t      = 20;
  agc_params.slow_fall_t      = 40;

  agc_params.threshold        = SU_DB(2e-2);

  agc_params.hang_max         = 30;
  agc_params.slope_factor     = 0;

  SU_TEST_ASSERT(su_agc_init(&agc, &agc_params));

  su_ncqo_init(&ncqo, SU_TEST_AGC_SIGNAL_FREQ);

  /* Create a falling sinusoid */
  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p) {
    t = p - SU_TEST_SIGNAL_BUFFER_SIZE / 2;
    buffer[p] = 1e-2 * rand() / (double) RAND_MAX;

    buffer[p] += 0.2 * floor(5 - 5 * p / SU_TEST_SIGNAL_BUFFER_SIZE) * su_ncqo_read_i(&ncqo);
  }

  if (ctx->dump_results) {
    SU_TEST_ASSERT(
        su_test_buffer_dump_matlab(
            buffer,
            SU_TEST_SIGNAL_BUFFER_SIZE,
            "steady.m",
            "steady"));
  }

  SU_TEST_TICK(ctx);

  /* Feed the AGC and put samples back in buffer */
  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p) {
    buffer[p] = SU_C_REAL(su_agc_feed(&agc, buffer[p]));
  }

  /* TODO: Improve levels */
  SU_TEST_ASSERT(
      su_test_check_peak(
          ctx,
          buffer,
          SU_TEST_SIGNAL_BUFFER_SIZE,
          2 * SU_TEST_AGC_WINDOW,
          SU_AGC_RESCALE * 0.9 ,
          SU_AGC_RESCALE * 1.1));

  ok = SU_TRUE;

done:
  SU_TEST_END(ctx);

  su_agc_finalize(&agc);

  if (buffer != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_buffer_dump_matlab(
              buffer,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "corrected.m",
              "corrected"));
    }

    free(buffer);
  }

  return ok;
}

SUBOOL
su_test_pll(su_test_context_t *ctx)
{
  SUBOOL ok = SU_FALSE;
  SUFLOAT *input = NULL;
  SUFLOAT *omgerr = NULL;
  SUFLOAT *phierr = NULL;
  SUFLOAT *lock = NULL;

  SUFLOAT t;
  su_ncqo_t ncqo = su_ncqo_INITIALIZER;
  su_pll_t pll = su_pll_INITIALIZER;
  unsigned int p = 0;

  SU_TEST_START_TICKLESS(ctx);

  /* Initialize */
  SU_TEST_ASSERT(input  = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));
  SU_TEST_ASSERT(omgerr = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));
  SU_TEST_ASSERT(phierr = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));
  SU_TEST_ASSERT(lock   = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));

  SU_TEST_ASSERT(su_pll_init(&pll,
      SU_TEST_PLL_SIGNAL_FREQ * 0.5,
      SU_TEST_PLL_BANDWIDTH));

  su_ncqo_init(&ncqo, SU_TEST_PLL_SIGNAL_FREQ);

  /* Create a falling sinusoid */
  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p) {
    input[p] = 0 * (0.5 - rand() / (double) RAND_MAX);
    input[p] += su_ncqo_read_i(&ncqo);
  }

  if (ctx->dump_results) {
    SU_TEST_ASSERT(
        su_test_buffer_dump_matlab(
            input,
            SU_TEST_SIGNAL_BUFFER_SIZE,
            "input.m",
            "input"));
  }

  /* Restart NCQO */
  su_ncqo_init(&ncqo, SU_TEST_PLL_SIGNAL_FREQ);

  SU_TEST_TICK(ctx);

  /* Feed the PLL and save phase value */
  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p) {
    (void) su_ncqo_read_i(&ncqo); /* Used to compute phase errors */
    su_pll_feed(&pll, input[p]);
    input[p]  = su_ncqo_get_i(&pll.ncqo);
    phierr[p] = su_ncqo_get_phase(&pll.ncqo) - su_ncqo_get_phase(&ncqo);
    lock[p]   = pll.lock;

    if (phierr[p] < 0 || phierr[p] > 2 * PI) {
      phierr[p] -= 2 * PI * floor(phierr[p] / (2 * PI));
      if (phierr[p] > PI)
        phierr[p] -= 2 * PI;
    }
    omgerr[p] = pll.ncqo.omega - ncqo.omega;
  }

  ok = SU_TRUE;

done:
  SU_TEST_END(ctx);

  su_pll_finalize(&pll);

  if (input != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_buffer_dump_matlab(
              input,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "output.m",
              "output"));
    }

    free(input);
  }

  if (phierr != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_buffer_dump_matlab(
              phierr,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "phierr.m",
              "phierr"));
    }

    free(phierr);
  }

  if (omgerr != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_buffer_dump_matlab(
              omgerr,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "omgerr.m",
              "omgerr"));
    }

    free(omgerr);
  }

  if (lock != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_buffer_dump_matlab(
              lock,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "lock.m",
              "lock"));
    }

    free(lock);
  }

  return ok;
}

SUBOOL
su_test_block(su_test_context_t *ctx)
{
  SUBOOL ok = SU_FALSE;
  su_block_t *block = NULL;
  su_block_port_t port = su_block_port_INITIALIZER;
  struct su_agc_params agc_params = su_agc_params_INITIALIZER;
  SUCOMPLEX samp = 0;

  SU_TEST_START(ctx);

  agc_params.delay_line_size  = 10;
  agc_params.mag_history_size = 10;
  agc_params.fast_rise_t      = 2;
  agc_params.fast_fall_t      = 4;

  agc_params.slow_rise_t      = 20;
  agc_params.slow_fall_t      = 40;

  agc_params.threshold        = SU_DB(2e-2);

  agc_params.hang_max         = 30;
  agc_params.slope_factor     = 0;

  block = su_block_new("agc", &agc_params);
  SU_TEST_ASSERT(block != NULL);

  /* Plug block to the reading port */
  SU_TEST_ASSERT(su_block_port_plug(&port, block, 0));

  /* Try to read (this must fail) */
  SU_TEST_ASSERT(su_block_port_read(&port, &samp, 1) == SU_BLOCK_PORT_READ_ERROR_ACQUIRE);

  ok = SU_TRUE;
done:
  SU_TEST_END(ctx);

  if (su_block_port_is_plugged(&port))
    su_block_port_unplug(&port);

  if (block != NULL)
    su_block_destroy(block);

  return ok;
}

SUBOOL
su_test_block_plugging(su_test_context_t *ctx)
{
  SUBOOL ok = SU_FALSE;
  su_block_t *agc_block = NULL;
  su_block_t *wav_block = NULL;
  su_block_port_t port = su_block_port_INITIALIZER;
  struct su_agc_params agc_params = su_agc_params_INITIALIZER;
  SUCOMPLEX buffer[17]; /* Prime number on purpose */
  SUFLOAT real = 0;
  int i;
  FILE *fp = NULL;
  ssize_t got;

  SU_TEST_START(ctx);

  if (ctx->dump_results) {
    fp = fopen("su_test_block_plugging.raw", "wb");
    SU_TEST_ASSERT(fp != NULL);
  }

  agc_params.delay_line_size  = 10;
  agc_params.mag_history_size = 10;
  agc_params.fast_rise_t      = 2;
  agc_params.fast_fall_t      = 4;

  agc_params.slow_rise_t      = 20;
  agc_params.slow_fall_t      = 40;

  agc_params.threshold        = SU_DB(2e-2);

  agc_params.hang_max         = 30;
  agc_params.slope_factor     = 0;

  agc_block = su_block_new("agc", &agc_params);
  SU_TEST_ASSERT(agc_block != NULL);

  wav_block = su_block_new("wavfile", "test.wav");
  SU_TEST_ASSERT(wav_block != NULL);

  /* Plug wav file to AGC */
  SU_TEST_ASSERT(su_block_plug(wav_block, 0, 0, agc_block));

  /* Plug AGC to the reading port */
  SU_TEST_ASSERT(su_block_port_plug(&port, agc_block, 0));

  /* Try to read (this must work) */
  do {
    got = su_block_port_read(&port, buffer, 17);
    SU_TEST_ASSERT(got >= 0);

    if (fp != NULL)
      for (i = 0; i < got; ++i) {
        real = SU_C_REAL(buffer[i]);
        fwrite(&real, sizeof(SUFLOAT), 1, fp);
      }
  } while (got > 0);

  ok = SU_TRUE;

done:
  SU_TEST_END(ctx);

  if (su_block_port_is_plugged(&port))
    su_block_port_unplug(&port);

  if (agc_block != NULL)
    su_block_destroy(agc_block);

  if (wav_block != NULL)
    su_block_destroy(wav_block);

  if (fp != NULL)
    fclose(fp);

  return ok;
}

SUBOOL
su_test_tuner(su_test_context_t *ctx)
{
  SUBOOL ok = SU_FALSE;
  su_block_t *agc_block = NULL;
  su_block_t *tuner_block = NULL;
  su_block_t *wav_block = NULL;
  su_block_port_t port = su_block_port_INITIALIZER;
  struct su_agc_params agc_params = su_agc_params_INITIALIZER;
  SUCOMPLEX buffer[17]; /* Prime number on purpose */
  SUFLOAT samp = 0;
  int i;
  FILE *fp = NULL;
  ssize_t got;

  /* Block properties */
  int *samp_rate;
  SUFLOAT *fc;
  SUFLOAT *T;
  SUFLOAT *beta;
  unsigned int *size;
  const unsigned int *d;

  SU_TEST_START(ctx);

  if (ctx->dump_results || 1) {
    fp = fopen("su_test_tuner.raw", "wb");
    SU_TEST_ASSERT(fp != NULL);
  }

  agc_params.delay_line_size  = 10;
  agc_params.mag_history_size = 10;
  agc_params.fast_rise_t      = 2;
  agc_params.fast_fall_t      = 4;

  agc_params.slow_rise_t      = 20;
  agc_params.slow_fall_t      = 40;

  agc_params.threshold        = SU_DB(2e-2);

  agc_params.hang_max         = 30;
  agc_params.slope_factor     = 0;

  wav_block = su_block_new("wavfile", "test.wav");
  SU_TEST_ASSERT(wav_block != NULL);

  samp_rate = su_block_get_property_ref(
      wav_block,
      SU_BLOCK_PROPERTY_TYPE_INTEGER,
      "samp_rate");
  SU_TEST_ASSERT(samp_rate != NULL);
  SU_TEST_ASSERT(*samp_rate == 8000);

  SU_INFO("Wav file opened, sample rate: %d\n", *samp_rate);

  tuner_block = su_block_new(
      "tuner",
      SU_ABS2NORM_FREQ(*samp_rate, 910),  /* Center frequency (910 Hz) */
      SU_T2N_FLOAT(*samp_rate, 1. / 468), /* Signal is 468 baud */
      (SUFLOAT) 1,                        /* Cast is mandatory */
      1000);                              /* 1000 coefficients */
  SU_TEST_ASSERT(tuner_block != NULL);

  d = su_block_get_property_ref(
      tuner_block,
      SU_BLOCK_PROPERTY_TYPE_INTEGER,
      "decimation");
  SU_TEST_ASSERT(d != NULL);
  SU_INFO("Tuner created, decimation: %d\n", *d);

  agc_block = su_block_new("agc", &agc_params);
  SU_TEST_ASSERT(agc_block != NULL);

  /* Plug wav file to tuner */
  SU_TEST_ASSERT(su_block_plug(wav_block, 0, 0, tuner_block));

  /* Plug tuner to AGC */
  SU_TEST_ASSERT(su_block_plug(tuner_block, 0, 0, agc_block));

  /* Plug AGC to the reading port */
  SU_TEST_ASSERT(su_block_port_plug(&port, agc_block, 0));

  /* Try to read (this must work) */
  do {
    got = su_block_port_read(&port, buffer, 17);
    SU_TEST_ASSERT(got >= 0);

    if (fp != NULL)
      for (i = 0; i < got; ++i) {
        samp = SU_C_REAL(buffer[i]);
        fwrite(&samp, sizeof(SUFLOAT), 1, fp);
        samp = SU_C_IMAG(buffer[i]);
        fwrite(&samp, sizeof(SUFLOAT), 1, fp);
      }
  } while (got > 0);

  ok = SU_TRUE;

done:
  SU_TEST_END(ctx);

  if (su_block_port_is_plugged(&port))
    su_block_port_unplug(&port);

  if (agc_block != NULL)
    su_block_destroy(agc_block);

  if (tuner_block != NULL)
    su_block_destroy(tuner_block);

  if (wav_block != NULL)
    su_block_destroy(wav_block);

  if (fp != NULL)
    fclose(fp);

  return ok;
}

SUBOOL
su_test_costas(su_test_context_t *ctx)
{
  SUBOOL ok = SU_FALSE;
  SUFLOAT *input = NULL;
  SUFLOAT *omgerr = NULL;
  SUFLOAT *phierr = NULL;
  SUFLOAT *lock = NULL;

  SUFLOAT t;
  su_ncqo_t ncqo = su_ncqo_INITIALIZER;
  su_pll_t pll = su_pll_INITIALIZER;
  unsigned int p = 0;

  SU_TEST_START_TICKLESS(ctx);

  /* Initialize */
  SU_TEST_ASSERT(input  = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));
  SU_TEST_ASSERT(omgerr = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));
  SU_TEST_ASSERT(phierr = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));
  SU_TEST_ASSERT(lock   = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));

  SU_TEST_ASSERT(su_pll_costas_init(&pll,
      0,
      SU_TEST_COSTAS_BANDWIDTH));

  su_ncqo_init(&ncqo, SU_TEST_COSTAS_SIGNAL_FREQ);

  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p) {
    input[p] = 0 * (0.5 - rand() / (double) RAND_MAX);
    input[p] += su_ncqo_read_i(&ncqo);
  }

  if (ctx->dump_results) {
    SU_TEST_ASSERT(
        su_test_buffer_dump_matlab(
            input,
            SU_TEST_SIGNAL_BUFFER_SIZE,
            "costas_input.m",
            "x"));
  }

  /* Restart NCQO */
  su_ncqo_init(&ncqo, SU_TEST_COSTAS_SIGNAL_FREQ);

  SU_TEST_TICK(ctx);

  /* Feed the PLL and save phase value */
  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p) {
    (void) su_ncqo_read_i(&ncqo); /* Used to compute phase errors */
    su_pll_costas_feed(&pll, (SUCOMPLEX) input[p]);
    input[p]  = su_ncqo_get_i(&pll.ncqo);
    phierr[p] = su_ncqo_get_phase(&pll.ncqo) - su_ncqo_get_phase(&ncqo);
    lock[p]   = pll.lock;

    if (phierr[p] < 0 || phierr[p] > 2 * PI) {
      phierr[p] -= 2 * PI * floor(phierr[p] / (2 * PI));
      if (phierr[p] > PI)
        phierr[p] -= 2 * PI;
    }
    omgerr[p] = pll.ncqo.fnor;
  }

  ok = SU_TRUE;

done:
  SU_TEST_END(ctx);

  su_pll_finalize(&pll);

  if (input != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_buffer_dump_matlab(
              input,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "costas_output.m",
              "y"));
    }

    free(input);
  }

  if (phierr != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_buffer_dump_matlab(
              phierr,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "costas_phierr.m",
              "pe"));
    }

    free(phierr);
  }

  if (omgerr != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_buffer_dump_matlab(
              omgerr,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "costas_omgerr.m",
              "oe"));
    }

    free(omgerr);
  }

  if (lock != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_buffer_dump_matlab(
              lock,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "costas_lock.m",
              "lock"));
    }

    free(lock);
  }

  return ok;
}

SUBOOL
su_test_costas_complex(su_test_context_t *ctx)
{
  SUBOOL ok = SU_FALSE;
  SUCOMPLEX *input = NULL;
  SUFLOAT *omgerr = NULL;
  SUCOMPLEX *phierr = NULL;
  SUFLOAT *lock = NULL;

  SUCOMPLEX x = 0;
  SUCOMPLEX bbs = 1;
  SUCOMPLEX symbols[] = { /* Out of phase BPSK signal */
      SU_SQRT(2) + I * SU_SQRT(2),
      -SU_SQRT(2) - I * SU_SQRT(2)};
  unsigned int filter_period;
  unsigned int symbol_period;
  unsigned int sync_period;
  unsigned int message;
  unsigned int msgbuf;

  unsigned int rx_delay;
  unsigned int rx_buf = 0;

  su_ncqo_t ncqo = su_ncqo_INITIALIZER;
  su_pll_t pll = su_pll_INITIALIZER;
  su_iir_filt_t lpf = su_iir_filt_INITIALIZER;
  unsigned int p = 0;
  unsigned int t = 0;
  unsigned int bit;

  SU_TEST_START_TICKLESS(ctx);

  /* Initialize some parameters */
  symbol_period = SU_TEST_COSTAS_SYMBOL_PERIOD;
  filter_period = 4 * symbol_period;
  sync_period   = 4096; /* Number of samples to allow loop to synchronize */
  message       = 0x414c4f48; /* Some greeting message */
  rx_delay      = filter_period / 2 + sync_period;

  /* Initialize buffers */
  SU_TEST_ASSERT(input  = su_test_complex_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));
  SU_TEST_ASSERT(omgerr = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));
  SU_TEST_ASSERT(phierr = su_test_complex_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));
  SU_TEST_ASSERT(lock   = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));

  SU_TEST_ASSERT(su_pll_costas_init(
      &pll,
      0,
      SU_TEST_COSTAS_BANDWIDTH));

  su_ncqo_init(&ncqo, SU_TEST_COSTAS_SIGNAL_FREQ);

  /* Create Root-Raised-Cosine filter. We will use this to reduce ISI */
  SU_TEST_ASSERT(
      su_iir_rrc_init(
          &lpf,
          filter_period,
          symbol_period,
          0));

  /* Send data */
  msgbuf = message;
  SU_INFO("Modulating 0x%x in BPSK...\n", msgbuf);

  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p) {
    if (p >= sync_period) {
      if (p >= sync_period && p % symbol_period == 0) {
          bit = msgbuf & 1;
          msgbuf >>= 1;
          bbs = symbol_period * symbols[bit];
        } else {
          bbs = 0;
        }
      } else {
        bbs = symbols[1];
      }

    x = su_iir_filt_feed(&lpf, bbs);

    input[p] = x * su_ncqo_read(&ncqo);
  }

  if (ctx->dump_results) {
    SU_TEST_ASSERT(
        su_test_complex_buffer_dump_matlab(
            input,
            SU_TEST_SIGNAL_BUFFER_SIZE,
            "costas_input.m",
            "x"));
  }

  /* Restart NCQO */
  su_ncqo_init(&ncqo, SU_TEST_COSTAS_SIGNAL_FREQ);

  SU_TEST_TICK(ctx);

  /* Feed the loop and perform demodulation */
  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p) {
    (void) su_ncqo_step(&ncqo);
    su_pll_costas_feed(&pll, input[p]);
    input[p]  = su_ncqo_get(&pll.ncqo);
    phierr[p] = pll.a;
    lock[p]   = pll.lock;
    omgerr[p] = pll.ncqo.fnor - ncqo.fnor;

    if (p % symbol_period == 0) {
      if (p >= rx_delay) {
        t = (p - rx_delay) / symbol_period;
        bit = SU_C_ARG(phierr[p]) > 0;

        if (t < 32) {
          rx_buf |= bit << t;
        }
      }
    }
  }

  SU_INFO(
      "RX: 0x%x = ~0x%x in %d samples\n",
      rx_buf,
      ~rx_buf,
      SU_TEST_SIGNAL_BUFFER_SIZE);

  SU_TEST_ASSERT(rx_buf == message || rx_buf == ~message);

  ok = SU_TRUE;

done:
  SU_TEST_END(ctx);

  su_pll_finalize(&pll);

  if (input != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_complex_buffer_dump_matlab(
              input,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "costas_output.m",
              "y"));
    }

    free(input);
  }

  if (phierr != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_complex_buffer_dump_matlab(
              phierr,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "costas_phierr.m",
              "pe"));
    }

    free(phierr);
  }

  if (omgerr != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_buffer_dump_matlab(
              omgerr,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "costas_omgerr.m",
              "oe"));
    }

    free(omgerr);
  }

  if (lock != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_buffer_dump_matlab(
              lock,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "costas_lock.m",
              "lock"));
    }

    free(lock);
  }

  if (lpf.x_size > 0) {
    if (ctx->dump_results) {
      su_test_buffer_dump_matlab(
          lpf.b,
          lpf.x_size,
          "costas_rrc.m",
          "rrc");
    }
  }

  su_iir_filt_finalize(&lpf);

  return ok;
}

SUPRIVATE int
su_test_costas_qpsk_decision(SUCOMPLEX x)
{
  SUFLOAT a = SU_C_ARG(x);

  if (a < -M_PI / 2)
    return 0;
  else if (a < 0)
    return 1;
  else if (a < M_PI / 2)
    return 2;
  else
    return 3;
}

SUINLINE void
su_swap(char *a, char *b)
{
  char tmp;

  tmp = *a;
  *a = *b;
  *b = tmp;
}

SUPRIVATE int
su_test_rotcompare(uint32_t original, uint32_t recv)
{
  unsigned char msgsym[16];
  unsigned char map[4] = {0, 1, 2, 3};
  unsigned char tmp;
  unsigned int n;
  unsigned int i, j, k;
  int count = 0;
  for (n = 0; n < 32; n += 2) {
    msgsym[n >> 1] = (original >> n) & 3;
  }

  /*
   * There are 24 permutations of 4 elements. I'm not making this
   * general, I'll settle for several nested loops
   */

  for (i = 0; i < 4; ++i) {
    su_swap(map + i, map + 3);

    for (j = 0; j < 3; ++j) {
      su_swap(map + j, map + 2);

      for (k = 0; k < 2; ++k) {
        su_swap(map + k, map + 1);

        for (n = 0; n < 32; n += 2) {
          if (msgsym[n >> 1] != map[(recv >> n) & 3])
            break;
        }

        if (n == 32)
          return count;

        ++count;

        su_swap(map + k, map + 1);
      }
      su_swap(map + j, map + 2);
    }
    su_swap(map + i, map + 3);
  }

  return -1;
}

#define SU_QPSK_ROT_1 0x01010101
#define SU_QPSK_ROT_2 0x10101010
#define SU_QPSK_ROT_3 (SU_QPSK_ROT_1 | SU_QPSK_ROT_2)

SUBOOL
su_test_costas_qpsk(su_test_context_t *ctx)
{
  SUBOOL ok = SU_FALSE;
  SUCOMPLEX *input = NULL;
  SUFLOAT *omgerr = NULL;
  SUCOMPLEX *phierr = NULL;
  SUCOMPLEX *rx = NULL;
  SUFLOAT *lock = NULL;
  SUFLOAT N0 = 0;
  SUCOMPLEX x = 0;
  SUCOMPLEX bbs = 1;
  SUCOMPLEX symbols[] = { /* Out of phase BPSK signal */
      SU_SQRT(2) + I * SU_SQRT(2),
      -SU_SQRT(2) - I * SU_SQRT(2),
      SU_SQRT(2) - I * SU_SQRT(2),
      -SU_SQRT(2) + I * SU_SQRT(2),
  };
  unsigned int filter_period;
  unsigned int symbol_period;
  unsigned int sync_period;
  unsigned int message;
  unsigned int msgbuf;

  unsigned int rx_delay;
  unsigned int rx_buf = 0;

  su_ncqo_t ncqo = su_ncqo_INITIALIZER;
  su_costas_t costas = su_costas_INITIALIZER;
  su_iir_filt_t lpf = su_iir_filt_INITIALIZER;
  unsigned int p = 0;
  unsigned int t = 0;
  unsigned int sym;
  unsigned int n = 0;
  unsigned int rx_count = 0;
  unsigned int rx_size;
  int permutations = 0;

  SU_TEST_START_TICKLESS(ctx);

  /* Initialize some parameters */
  symbol_period = SU_TEST_COSTAS_SYMBOL_PERIOD;
  filter_period = 4 * symbol_period;
  sync_period   = 2 * 4096; /* Number of samples to allow loop to synchronize */
  message       = 0x414c4f48; /* Some greeting message */
  rx_delay      = filter_period / 2 + sync_period - symbol_period / 2;
  rx_size       = (SU_TEST_SIGNAL_BUFFER_SIZE - rx_delay) / symbol_period;
  N0            = 5e-1; /* Noise amplitude */

  /* Initialize buffers */
  SU_TEST_ASSERT(input  = su_test_complex_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));
  SU_TEST_ASSERT(omgerr = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));
  SU_TEST_ASSERT(phierr = su_test_complex_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));
  SU_TEST_ASSERT(lock   = su_test_buffer_new(SU_TEST_SIGNAL_BUFFER_SIZE));
  SU_TEST_ASSERT(rx     = su_test_complex_buffer_new(rx_size));

  SU_TEST_ASSERT(su_costas_init(
      &costas,
      SU_COSTAS_KIND_QPSK,
      0,
      SU_TEST_COSTAS_BANDWIDTH,
      10,
      1e-1 * SU_TEST_COSTAS_BANDWIDTH));

  if (ctx->dump_results) {
    SU_TEST_ASSERT(
        su_test_buffer_dump_matlab(
            costas.af.b,
            costas.af.x_size,
            "qpsk_af.m",
            "af"));
  }

  su_ncqo_init(&ncqo, SU_TEST_COSTAS_SIGNAL_FREQ);

  /* Create Root-Raised-Cosine filter. We will use this to reduce ISI */
  SU_TEST_ASSERT(
      su_iir_rrc_init(
          &lpf,
          filter_period,
          symbol_period,
          0));

  if (ctx->dump_results) {
    SU_TEST_ASSERT(
        su_test_buffer_dump_matlab(
            lpf.b,
            lpf.x_size,
            "qpsk_mf.m",
            "mf"));
  }

  /* Send data */
  msgbuf = message;
  SU_INFO("Modulating 0x%x in QPSK...\n", msgbuf);

  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p) {
    if (p >= sync_period) {
      if (p >= sync_period && p % symbol_period == 0) {
          if (n == 32) {
            n = 0;
          }
          msgbuf = message >> n;
          sym = msgbuf & 3;
          n += 2;
          bbs = symbol_period * symbols[sym];
        } else {
          bbs = 0;
        }
      } else {
        /* Send first symbol to synchronize */
        bbs = symbols[1];
      }

    x = su_iir_filt_feed(&lpf, bbs);

    input[p] = x * su_ncqo_read(&ncqo) + N0 * su_c_awgn();
  }

  if (ctx->dump_results) {
    SU_TEST_ASSERT(
        su_test_complex_buffer_dump_matlab(
            input,
            SU_TEST_SIGNAL_BUFFER_SIZE,
            "qpsk_input.m",
            "x"));
  }

  /* Restart NCQO */
  su_ncqo_init(&ncqo, SU_TEST_COSTAS_SIGNAL_FREQ);

  SU_TEST_TICK(ctx);

  /* Feed the loop and perform demodulation */
  for (p = 0; p < SU_TEST_SIGNAL_BUFFER_SIZE; ++p) {
    (void) su_ncqo_step(&ncqo);
    su_costas_feed(&costas, input[p]);
    input[p]  = su_ncqo_get(&costas.ncqo);
    phierr[p] = costas.y;
    lock[p]   = costas.lock;
    omgerr[p] = costas.ncqo.fnor - ncqo.fnor;

    if (p % symbol_period == 0) {
      if (p >= rx_delay) {
        t = (p - rx_delay) / symbol_period;
        sym = su_test_costas_qpsk_decision(phierr[p]);
        if (t < 32) {
          rx_buf |= sym << (2 * t);
        }

        rx[rx_count++] = phierr[p];
      }
    }
  }

  permutations = su_test_rotcompare(rx_buf, message);

  SU_INFO(
      "RX: 0x%x in %d samples\n",
      rx_buf,
      SU_TEST_SIGNAL_BUFFER_SIZE);
  SU_TEST_ASSERT(permutations != -1);
  SU_INFO(
        "RX: message decoded after %d permutations\n",
        permutations);
  ok = SU_TRUE;

done:
  SU_TEST_END(ctx);

  su_costas_finalize(&costas);

  if (input != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_complex_buffer_dump_matlab(
              input,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "qpsk_output.m",
              "y"));
    }

    free(input);
  }

  if (phierr != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_complex_buffer_dump_matlab(
              phierr,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "qpsk_phierr.m",
              "pe"));
    }

    free(phierr);
  }

  if (omgerr != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_buffer_dump_matlab(
              omgerr,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "qpsk_omgerr.m",
              "oe"));
    }

    free(omgerr);
  }

  if (lock != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_buffer_dump_matlab(
              lock,
              SU_TEST_SIGNAL_BUFFER_SIZE,
              "qpsk_lock.m",
              "lock"));
    }

    free(lock);
  }

  if (rx != NULL) {
    if (ctx->dump_results) {
      SU_TEST_ASSERT(
          su_test_complex_buffer_dump_matlab(
              rx,
              rx_size,
              "qpsk_rx.m",
              "rx"));
    }

    free(rx);
  }

  if (lpf.x_size > 0) {
    if (ctx->dump_results) {
      su_test_buffer_dump_matlab(
          lpf.b,
          lpf.x_size,
          "qpsk_rrc.m",
          "rrc");
    }
  }

  su_iir_filt_finalize(&lpf);

  return ok;
}

int
main (int argc, char *argv[], char *envp[])
{
  su_test_cb_t test_list[] = {
      su_test_ncqo,
      su_test_butterworth_lpf,
      su_test_agc_transient,
      su_test_agc_steady_rising,
      su_test_agc_steady_falling,
      su_test_pll,
      su_test_block,
      su_test_block_plugging,
      su_test_tuner,
      su_test_costas,
      su_test_costas_complex,
      su_test_costas_qpsk
  };
  unsigned int test_count = sizeof(test_list) / sizeof(test_list[0]);

  if (!su_lib_init()) {
    SU_ERROR("Failed to initialize sigutils library\n");
    exit (EXIT_FAILURE);
  }

  su_test_run(test_list, test_count, test_count - 1, test_count - 1, SU_TRUE);

  return 0;
}

