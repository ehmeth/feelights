/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#define LOG_LEVEL 4
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr.h>
#include <drivers/led_strip.h>
#include <drivers/adc.h>
#include <device.h>
#include <drivers/spi.h>
#include <sys/util.h>

#include <arm_math.h>

typedef unsigned char      u8;
typedef unsigned int       u32;
typedef          char      i8;
typedef          int       i32;
typedef          float32_t f32;

#define ArrayCount(Array) (sizeof(Array)/sizeof(Array[0]))

#define Maximum(A, B) ((A) < (B) ? (B) : (A))
#define Minimum(A, B) ((A) < (B) ? (A) : (B))

#define internal static

internal inline f32 Square(f32 A) {
   return A*A;
}

internal inline f32 Abs(f32 A) {
   return A >= 0 ? A : -A;
}

internal inline f32 Clamp(f32 Min, f32 A, f32 Max) {
   return Maximum(Minimum(A, Max), Min);
}

internal inline i32 Round(f32 A) {
   return (i32)(A + 0.5f);
}

internal inline f32 Lerp(f32 A, f32 t, f32 B) {
   return (1 - t) * A + t * B;
}

internal inline f32 Sine(f32 V) {
   return arm_sin_f32(V);
}


/* ADC */
#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define ADC_NODE		DT_PHANDLE(DT_PATH(zephyr_user), io_channels)

#define NUM_SAMPLES (128)
#define SAMPLE_INDEX_MASK  (0xFF)

internal int16_t sample_buffer[2*NUM_SAMPLES];

struct adc_channel_cfg channel_cfg = {
	.gain = ADC_GAIN_1,
	.reference = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQ_TIME_DEFAULT,
	.channel_id = 8,
	.differential = 0
};

struct adc_sequence_options options = {
   .interval_us = 22,
   .extra_samplings = NUM_SAMPLES - 1,
};

struct adc_sequence sequence = {
	/* individual channels will be added below */
	.channels    = BIT(8),
	.buffer      = sample_buffer,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(sample_buffer)/2,
	.resolution  = 12,
   .options     = &options,
};

internal struct k_poll_signal adc_signal;

/* LED STRIP */
#define STRIP_NODE		DT_ALIAS(led_strip)
#define STRIP_NUM_PIXELS	DT_PROP(DT_ALIAS(led_strip), chain_length)

#define DELAY_TIME K_MSEC(10)

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

typedef union {
   u32 Dword;
   struct led_rgb Color;
} Pixel;

Pixel pixel_buffer[STRIP_NUM_PIXELS * 2];
Pixel *pixels = pixel_buffer;

internal const struct device *strip = DEVICE_DT_GET(STRIP_NODE);

internal f32 fft_in[NUM_SAMPLES];
internal f32 fft_complex[2*NUM_SAMPLES];
internal f32 fft_out[NUM_SAMPLES];

typedef enum {
   none,
   spectrum_window,
} controller_algo_t;

typedef struct {
   f32 PFreq;
   f32 RFreq;
   f32 MaxIntensity;
} algo_spectrum_window_t;

typedef struct {
   controller_algo_t Algo;
   union {
      algo_spectrum_window_t SpectrumWindow;
   } Data;
} controller_t ;


typedef struct 
{
   f32 P;
   f32 dP;
   f32 R;
   f32 dR;
   f32 Intensity;
   struct {
      f32 R;
      f32 G;
      f32 B;
   } Color;
   controller_t Controller;
} fl_orb;

#define MAX_ORBS (2)

internal fl_orb Orbs[MAX_ORBS];

internal void UpdateAndRender()
{
   for (size_t i = 0; i < STRIP_NUM_PIXELS; ++i)
   {
      pixels[i].Dword = 0;
   }

   for (u32 IOrb = 0; IOrb < MAX_ORBS; ++IOrb)
   {
      fl_orb * Orb = &Orbs[IOrb];

      switch (Orb->Controller.Algo)
      {
         case spectrum_window:
            {
               algo_spectrum_window_t * Window = &Orb->Controller.Data.SpectrumWindow;
               f32 Intensity = 0.0f;
               i32 IFreq = (i32)ceil(Window->PFreq - Window->RFreq);
               i32 MaxIFreq = (i32)floor(Window->PFreq + Window->RFreq);
               IFreq = IFreq < 0 ? 0 : IFreq;
               MaxIFreq = MaxIFreq >= NUM_SAMPLES ? NUM_SAMPLES : MaxIFreq;

               for ( ; IFreq < MaxIFreq; IFreq++)
               {
                  Intensity += fft_out[IFreq];
               }
               Intensity /= 2.0f * Window->RFreq;
               Orb->Intensity = Lerp(0, Intensity, Window->MaxIntensity);
            }
            break;
         case none:
         default:
            break;

      }
      f32 P = ceil(Orb->P - Orb->R);
      i32 MaxI = (i32)floor(Orb->P + Orb->R);
      i32 I = (i32)P;
      I = I < 0 ? 0 : I;
      MaxI = MaxI >= STRIP_NUM_PIXELS ? STRIP_NUM_PIXELS : MaxI;
      for ( ; I < MaxI; ++I, P += 1.0f)
      {
         f32 Dist = Abs(P - Orb->P);
         f32 Rate = Clamp(0.0f, 1.0f - (Dist/Orb->R), 1.0f);
         f32 Intensity = Round(Lerp(0, Rate, Orb->Intensity));
         pixels[I].Color.r += (u8)(Orb->Color.R * Intensity);
         pixels[I].Color.g += (u8)(Orb->Color.G * Intensity);
         pixels[I].Color.b += (u8)(Orb->Color.B * Intensity);

      }
   }
}


void main(void)
{
   arm_rfft_fast_instance_f32 fft;
   size_t start_index = 0;
   float div_norm = 1.0/2000.0;
	const struct device *dev_adc = DEVICE_DT_GET(ADC_NODE);
	int rc = 0;
   struct k_poll_event events[1] = {
        K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
                                 K_POLL_MODE_NOTIFY_ONLY,
                                 &adc_signal),
   };

   k_poll_signal_init(&adc_signal);

	if (!device_is_ready(dev_adc)) {
		printk("ADC device not found\n");
		return;
	}
   adc_channel_setup(dev_adc, &channel_cfg);
   rc = adc_read_async(dev_adc, &sequence, &adc_signal);
   if (rc != 0) {
      printk("ADC reading failed with error %d.\n", rc);
      return;
   }

	if (device_is_ready(strip)) {
		LOG_INF("Found LED strip device %s", strip->name);
	} else {
		LOG_ERR("LED strip device %s is not ready", strip->name);
		return;
	}
	LOG_INF("Displaying pattern on strip");

   Orbs[0].P = 7.0f;
   Orbs[0].dP = 0.0f;
   Orbs[0].R = 5.5f;
   Orbs[0].dR = 0.0f;
   Orbs[0].Color.R = 0.0f;
   Orbs[0].Color.G = 0.7f;
   Orbs[0].Color.B = 0.1f;
   Orbs[0].Controller.Algo = spectrum_window;
   Orbs[0].Controller.Data.SpectrumWindow.PFreq = 5.0;
   Orbs[0].Controller.Data.SpectrumWindow.RFreq = 4.0;
   Orbs[0].Controller.Data.SpectrumWindow.MaxIntensity = 40.0f;

   Orbs[1].P = 20.0f;
   Orbs[1].dP = 0.0f;
   Orbs[1].R = 8.5f;
   Orbs[1].dR = 0.0f;
   Orbs[1].Color.R = 0.8f;
   Orbs[1].Color.G = 0.1f;
   Orbs[1].Color.B = 0.2f;
   Orbs[1].Controller.Algo = spectrum_window;
   Orbs[1].Controller.Data.SpectrumWindow.PFreq = 23.0;
   Orbs[1].Controller.Data.SpectrumWindow.RFreq = 6.0;
   Orbs[1].Controller.Data.SpectrumWindow.MaxIntensity = 60.0f;

	while (1) {
      if (0 == k_poll(events, 1, K_MSEC(30)))
      {
         u32 mean = 0;
         f32 mean_f = 0;

         events[0].signal->signaled = 0;
         events[0].state = K_POLL_STATE_NOT_READY;

         rc = led_strip_update_rgb(strip, &pixels->Color, STRIP_NUM_PIXELS);
         if (rc) {
            LOG_ERR("couldn't update strip: %d", rc);
         }
         pixels = pixels + STRIP_NUM_PIXELS;
         if (pixels >= pixel_buffer + 2*STRIP_NUM_PIXELS)
         {
            pixels = pixel_buffer;
         }

         sequence.buffer = &sample_buffer[(start_index + NUM_SAMPLES) & SAMPLE_INDEX_MASK];
         rc = adc_read_async(dev_adc, &sequence, &adc_signal);
         if (rc != 0) {
            printk("ADC reading failed with error %d.\n", rc);
            return;
         }

         for (int i = 0; i < NUM_SAMPLES; ++i)
         {
            mean += sample_buffer[start_index + i];
         }
         mean /= NUM_SAMPLES;
         mean_f = (f32) mean;

         for (int i = 0; i < NUM_SAMPLES; ++i)
         {
            fft_in[i] = div_norm * (((f32) sample_buffer[start_index+i]) - mean_f);
         }
         arm_rfft_fast_init_f32(&fft, NUM_SAMPLES);
         arm_rfft_fast_f32(&fft, fft_in, fft_complex, 0);
         arm_cmplx_mag_f32(fft_complex, fft_out, NUM_SAMPLES);

         start_index = (start_index + NUM_SAMPLES) & SAMPLE_INDEX_MASK;

         UpdateAndRender();
      }
      k_sleep(DELAY_TIME);
	}
}
