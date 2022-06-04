#include "fl_common.h"
#include "fl_lights.h"

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

#define MAX_ORBS (4)

internal fl_orb Orbs[MAX_ORBS];

u32 LightsInit()
{
   Orbs[0].P = 5.0f;
   Orbs[0].dP = 0.0f;
   Orbs[0].R = 5.5f;
   Orbs[0].dR = 0.0f;
   Orbs[0].Color.R = 0.0f;
   Orbs[0].Color.G = 0.7f;
   Orbs[0].Color.B = 0.1f;
   Orbs[0].Controller.Algo = spectrum_window;
   Orbs[0].Controller.Data.SpectrumWindow.PFreq = 9.0;
   Orbs[0].Controller.Data.SpectrumWindow.RFreq = 8.0;
   Orbs[0].Controller.Data.SpectrumWindow.MaxIntensity = 40.0f;

   Orbs[1].P =9.0f;
   Orbs[1].dP = 0.0f;
   Orbs[1].R = 6.5f;
   Orbs[1].dR = 0.0f;
   Orbs[1].Color.R = 1.0f;
   Orbs[1].Color.G = 0.1f;
   Orbs[1].Color.B = 0.2f;
   Orbs[1].Controller.Algo = spectrum_window;
   Orbs[1].Controller.Data.SpectrumWindow.PFreq = 93.0;
   Orbs[1].Controller.Data.SpectrumWindow.RFreq = 9.0;
   Orbs[1].Controller.Data.SpectrumWindow.MaxIntensity = 90.0f;

   Orbs[2].P = 20.0f;
   Orbs[2].dP = 0.0f;
   Orbs[2].R = 8.5f;
   Orbs[2].dR = 0.0f;
   Orbs[2].Color.R = 0.0f;
   Orbs[2].Color.G = 0.4f;
   Orbs[2].Color.B = 0.3f;
   Orbs[2].Controller.Algo = spectrum_window;
   Orbs[2].Controller.Data.SpectrumWindow.PFreq = 173.0;
   Orbs[2].Controller.Data.SpectrumWindow.RFreq = 12.0;
   Orbs[2].Controller.Data.SpectrumWindow.MaxIntensity = 90.0f;

   Orbs[3].P = 26.0f;
   Orbs[3].dP = 0.0f;
   Orbs[3].R = 5.5f;
   Orbs[3].dR = 0.0f;
   Orbs[3].Color.R = 1.0f;
   Orbs[3].Color.G = 0.5f;
   Orbs[3].Color.B = 0.0f;
   Orbs[3].Controller.Algo = spectrum_window;
   Orbs[3].Controller.Data.SpectrumWindow.PFreq = 223.0;
   Orbs[3].Controller.Data.SpectrumWindow.RFreq = 12.0;
   Orbs[3].Controller.Data.SpectrumWindow.MaxIntensity = 90.0f;
   return 0;
}

void LightsUpdateAndRender(pixel *Pixels, u32 NumPixels, f32 *Spectrum, u32 NumSamples)
{
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
                  Intensity += Spectrum[IFreq];
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
      MaxI = MaxI >= NumPixels ? NumPixels : MaxI;
      for ( ; I < MaxI; ++I, P += 1.0f)
      {
         f32 Dist = Abs(P - Orb->P);
         f32 Rate = Clamp(0.0f, 1.0f - (Dist/Orb->R), 1.0f);
         f32 Intensity = Round(Lerp(0, Rate, Orb->Intensity));
         Pixels[I].Color.r += (u8)(Orb->Color.R * Intensity);
         Pixels[I].Color.g += (u8)(Orb->Color.G * Intensity);
         Pixels[I].Color.b += (u8)(Orb->Color.B * Intensity);

      }
   }
}

