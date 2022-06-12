#include "fl_common.h"
#include "fl_lights.h"

#define LOG_LEVEL 4
#include <logging/log.h>
LOG_MODULE_REGISTER(lights);

#define MIN_ORB_X (5.0f)
#define MAX_ORB_X (123.0f)
#define MIN_ORB_R (5.0f)
#define MAX_ORB_R (10.0f)
#define MIN_ORB_FREQ_IDX (3.0f)
#define MAX_ORB_FREQ_IDX (200.0f - MIN_ORB_FREQ_R)
#define MIN_ORB_FREQ_R (1.0f)
#define MAX_ORB_FREQ_R (5.0f - MIN_ORB_FREQ_R)

typedef struct {
   f32 R;
   f32 G;
   f32 B;
} fl_color;

typedef struct {
   fl_color Base;
   fl_color Accents[3];
} fl_palette;

typedef enum {
   none,
   spectrum_window,
} controller_algo_t;

typedef struct {
   f32 PFreq;
   f32 RFreq;
   f32 IntensityMultiplier;
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
   fl_color Color;
   controller_t Controller;
} fl_orb;

typedef struct
{
   fl_color Color;
   f32 Intensity;
   f32 IntensityMultiplier;
   f32 PFreq;
   f32 RFreq;
} fl_ambient;


#define MAX_ORBS (4)

internal fl_orb Orbs[MAX_ORBS];

internal fl_ambient Ambient;

internal fl_palette Palette[4];

internal void log_orb(fl_orb *Orb)
{
   LOG_INF("P %4f, R %f, RGB: %4f, %4f, %4f, PF %4f, RF, %4f, I %4f",
         Orb->P,
         Orb->R,
         Orb->Color.R,
         Orb->Color.G,
         Orb->Color.B,
         Orb->Controller.Data.SpectrumWindow.PFreq,
         Orb->Controller.Data.SpectrumWindow.RFreq,
         Orb->Controller.Data.SpectrumWindow.IntensityMultiplier
         );
}
internal inline void create_orb(fl_orb *Orb)
{
   Orb->P = MIN_ORB_X + MAX_ORB_X * Random();
   Orb->dP = 0.0f;
   Orb->R = MIN_ORB_R + MAX_ORB_R * Random();
   Orb->dR = 0.0f;
   Orb->Controller.Algo = spectrum_window;
   Orb->Controller.Data.SpectrumWindow.PFreq = MIN_ORB_FREQ_IDX + MAX_ORB_FREQ_IDX * Random();
   Orb->Controller.Data.SpectrumWindow.RFreq = MIN_ORB_FREQ_R + MAX_ORB_FREQ_R * Random();
   Orb->Controller.Data.SpectrumWindow.IntensityMultiplier = 100.0f + 140.0f * Random();
}

internal inline void RandomizeOrbs()
{
   for (i32 I = 0; I < MAX_ORBS; ++I)
   {
      create_orb(&Orbs[I]);
   }
}

internal void ApplyPalette(fl_palette *Palette)
{
   Ambient.Color.R = Palette->Base.R;
   Ambient.Color.G = Palette->Base.G;
   Ambient.Color.B = Palette->Base.B;

   for (i32 I = 0; I < MAX_ORBS; ++I)
   {
      Orbs[I].Color.R = Palette->Accents[I % 3].R;
      Orbs[I].Color.G = Palette->Accents[I % 3].G;
      Orbs[I].Color.B = Palette->Accents[I % 3].B;
   }
}

void MakePalette(fl_palette * Palette, u32 Base, u32 Accent1, u32 Accent2, u32 Accent3)
{
   const f32 OneOver255 = 1.0f / 255.0f;
   const f32 MixingR = 4.1f/5.8f;
   const f32 MixingG = 0.7f/5.8f;
   const f32 MixingB = 1.0f/5.8f;
   const f32 RFactor = MixingR * OneOver255;
   const f32 GFactor = MixingG * OneOver255;
   const f32 BFactor = MixingB * OneOver255;

   Palette->Base.R = (f32)((Base >> 16) & 0xFF) * RFactor;
   Palette->Base.G = (f32)((Base >>  8) & 0xFF) * GFactor;
   Palette->Base.B = (f32)((Base >>  0) & 0xFF) * BFactor;

   Palette->Accents[0].R = (f32)((Accent1 >> 16) & 0xFF) * RFactor;
   Palette->Accents[0].G = (f32)((Accent1 >>  8) & 0xFF) * GFactor;
   Palette->Accents[0].B = (f32)((Accent1 >>  0) & 0xFF) * BFactor;

   Palette->Accents[1].R = (f32)((Accent2 >> 16) & 0xFF) * RFactor;
   Palette->Accents[1].G = (f32)((Accent2 >>  8) & 0xFF) * GFactor;
   Palette->Accents[1].B = (f32)((Accent2 >>  0) & 0xFF) * BFactor;

   Palette->Accents[2].R = (f32)((Accent3 >> 16) & 0xFF) * RFactor;
   Palette->Accents[2].G = (f32)((Accent3 >>  8) & 0xFF) * GFactor;
   Palette->Accents[2].B = (f32)((Accent3 >>  0) & 0xFF) * BFactor;
}

u32 LightsInit()
{
   MakePalette(&Palette[0], 0xFABEC0, 0xF85C70, 0xF37970, 0xE43D40);
   MakePalette(&Palette[1], 0x32CD30, 0x2C5E1A, 0x1A4314, 0xB2D2A4);
   MakePalette(&Palette[2], 0x6AABD2, 0xB7CFDC, 0x385E72, 0xD9E4EC);
   MakePalette(&Palette[3], 0x5D59AF, 0x6AABD2, 0xBE81B6, 0xE390C8);
   ApplyPalette(&Palette[0]);

   RandomizeOrbs();

   Ambient.Intensity = 0.0f;
   Ambient.PFreq = 5.0f;
   Ambient.RFreq = 4.0f;
   Ambient.IntensityMultiplier = 90.0f;

   return 0;
}



void LightsUpdateAndRender(pixel *Pixels, u32 NumPixels, f32 *Spectrum, u32 NumSamples)
{
   static u32 ResetCount = 100;

   for (size_t i = 0; i < NumPixels; ++i)
   {
      Pixels[i].Dword = 0;
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
               MaxIFreq = MaxIFreq >= NumSamples ? NumSamples : MaxIFreq;

               for ( ; IFreq < MaxIFreq; IFreq++)
               {
                  if (Spectrum[IFreq] > 0.002f)
                  {
                     Intensity += Spectrum[IFreq];
                  }
               }
               Intensity /= 2.0f * Window->RFreq;
               Orb->Intensity = Clamp(Orb->Intensity * 0.7f, Intensity * Window->IntensityMultiplier, 255.0f);
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
      MaxI = MaxI >= NumPixels ? NumPixels : MaxI;
      f32 OrbRadiusSq = Square(Orb->R);
      for ( ; I < MaxI; ++I, P += 1.0f)
      {
         f32 DistSq = Square(P - Orb->P);
         f32 Rate = 1.0f - (DistSq - OrbRadiusSq);
         f32 Intensity = Rate * Orb->Intensity;
         Pixels[I].Color.r = ClampU(0, Pixels[I].Color.r + (u32)(Orb->Color.R * Intensity), 250);
         Pixels[I].Color.g = ClampU(0, Pixels[I].Color.g + (u32)(Orb->Color.G * Intensity), 250);
         Pixels[I].Color.b = ClampU(0, Pixels[I].Color.b + (u32)(Orb->Color.B * Intensity), 250);

      }
   }
   
   {
      f32 Intensity = 0.0f;
      i32 IFreq = (i32)ceil(Ambient.PFreq - Ambient.RFreq);
      i32 MaxIFreq = (i32)floor(Ambient.PFreq + Ambient.RFreq);
      IFreq = IFreq < 0 ? 0 : IFreq;
      MaxIFreq = MaxIFreq >= NumSamples ? NumSamples : MaxIFreq;

      for ( ; IFreq < MaxIFreq; IFreq++)
      {
         if (Spectrum[IFreq] > 0.002f)
         {
            Intensity += Spectrum[IFreq];
         }
      }
      Intensity /= 2.0f * Ambient.RFreq;
      Ambient.Intensity = Clamp(Maximum(Ambient.Intensity * 0.9f, 20.0f), Intensity * Ambient.IntensityMultiplier, 255.0f);

      for (i32 I = 0; I < NumPixels; ++I)
      {
         u32 CurrentIntensity = Pixels[I].Color.r + Pixels[I].Color.g + Pixels[I].Color.b;
         if (CurrentIntensity < 50)
         {
            Pixels[I].Color.r = ClampU(0, Pixels[I].Color.r + (u32)(Ambient.Color.R * Ambient.Intensity), 250);
            Pixels[I].Color.g = ClampU(0, Pixels[I].Color.g + (u32)(Ambient.Color.G * Ambient.Intensity), 250);
            Pixels[I].Color.b = ClampU(0, Pixels[I].Color.b + (u32)(Ambient.Color.B * Ambient.Intensity), 250);
         }
      }
   }

   if (--ResetCount == 0)
   {
      static u32 PaletteIndex = 0;
      RandomizeOrbs();
      ApplyPalette(&Palette[++PaletteIndex & 0x3]);
      ResetCount = 50 + (sys_rand32_get() & 0x0FF);
   }

#if 0
   static u32 DebugCount = 0;
   if (DebugCount++ > 100)
   {
      LOG_INF("I1: %7d, I2: %7d, I3 %7d, I4 %7d", 
            (i32)(Orbs[0].Intensity * 1000.0f),
            (i32)(Orbs[1].Intensity * 1000.0f),
            (i32)(Orbs[2].Intensity * 1000.0f),
            (i32)(Orbs[3].Intensity * 1000.0f));
      DebugCount = 0;
   }
#endif

}

