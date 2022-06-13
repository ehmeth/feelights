#include "fl_common.h"
#include "fl_audioin.h"

#include "zephyr.h"
#include <drivers/adc.h>
#include <drivers/dma.h>
#include <drivers/dma/dma_stm32.h>
#include <device.h>

#include "stm32f4xx_ll_adc.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_tim.h"

#define LOG_LEVEL 4
#include <logging/log.h>
LOG_MODULE_REGISTER(audioin);

internal void AdcDmaConfig(u16* Data, u32 size);
internal inline void ActivateAdc(ADC_TypeDef* Adc);
internal inline void EnableAdcChannel(ADC_TypeDef* Adc, u32 Channel, u32 Rank);
internal inline void StartAdc(ADC_TypeDef* Adc);
internal inline void StopAdc(ADC_TypeDef* Adc);
internal void AdcConfig(ADC_TypeDef* Adc, uint32_t TimerTriggerId, uint32_t AdcPeriphId);
internal void Adc3Init(u16* Data, u32 BufferSize);

#define DMA_NODE		DT_ALIAS(mic_dma)
internal const struct device *DmaDevice = DEVICE_DT_GET(DMA_NODE);

internal void TIM2IrqHandler()
{
   if (LL_TIM_IsActiveFlag_UPDATE(TIM2) == 1)
   {
      // Clear the update interrupt flag
      LL_TIM_ClearFlag_UPDATE(TIM2);
   }
}

internal void ADCIrqHandler()
{
   if (LL_ADC_IsActiveFlag_OVR(ADC1) != 0)
   {
      LL_ADC_ClearFlag_OVR(ADC1);
      LOG_ERR("ADC1 overdflow");
   }

   if (LL_ADC_IsActiveFlag_OVR(ADC2) != 0)
   {
      LL_ADC_ClearFlag_OVR(ADC2);
      LOG_ERR("ADC2 overdflow");
   }

   if (LL_ADC_IsActiveFlag_OVR(ADC3) != 0)
   {
      LL_ADC_ClearFlag_OVR(ADC3);
      LOG_ERR("ADC3 overdflow");
   }
}

u32 AudioInInit(u16* Buffer, u32 BufferSize)
{
   // Timer config 
   u32 Tim2ClockFrequency = 200000;
   u32 SamplingFrequency = 40000;
   u32 TimerClock = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 4;
   u32 Result = 0;

   LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
   LL_TIM_SetPrescaler(TIM2, __LL_TIM_CALC_PSC(TimerClock, Tim2ClockFrequency));
   LL_TIM_SetAutoReload(TIM2, __LL_TIM_CALC_ARR(TimerClock, LL_TIM_GetPrescaler(TIM2), SamplingFrequency));

   IRQ_CONNECT(TIM2_IRQn, 0, TIM2IrqHandler, NULL, 0);
   irq_enable(TIM2_IRQn);

   Adc3Init(Buffer, BufferSize);

   LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOF);
   LL_GPIO_SetPinMode(GPIOF, LL_GPIO_PIN_10, LL_GPIO_MODE_ANALOG);

   EnableAdcChannel(ADC3, LL_ADC_CHANNEL_8, LL_ADC_REG_RANK_1);


   return Result;
}

u32 AudioInStart()
{
   int ReturnCode = dma_start(DmaDevice, 1);
   if (ReturnCode != 0)
   {
      LOG_ERR("Dma start failed %d", ReturnCode);
   }

   StartAdc(ADC3);
   LL_TIM_EnableCounter(TIM2);
   LL_TIM_GenerateEvent_UPDATE(TIM2);

   return ReturnCode;
}

u32 AudioInStop()
{
   LL_TIM_DisableCounter(TIM2);
   StopAdc(ADC3);

   int ReturnCode = dma_stop(DmaDevice, 1);
   if (ReturnCode != 0)
   {
      LOG_ERR("Dma stop failed %d", ReturnCode);
   }

   return ReturnCode;
}

internal void DmaCallback(const struct device *Dev, void *UserData, uint32_t Channel, int Status)
{
   EventEmit(EV_AUDIO_SAMPLES_AVAILABLE);
}

internal void AdcDmaConfig(u16* Data, u32 BufferSize)
{
   int ReturnCode = 0;

   struct dma_block_config BlockConfig = {
      .source_address = LL_ADC_DMA_GetRegAddr(ADC3, LL_ADC_DMA_REG_REGULAR_DATA),
      .dest_address = (u32)Data,
      .source_gather_interval = 0,
      .dest_scatter_interval = 0,
      .dest_scatter_count = 0,
      .source_gather_count = 0,
      .block_size = BufferSize,
      .next_block = NULL,
      .source_gather_en = 0,
      .dest_scatter_en = 0,
      .source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
      .dest_addr_adj = DMA_ADDR_ADJ_INCREMENT,
      .source_reload_en = 1,
      .dest_reload_en = 1,
      .fifo_mode_control = 0,
      .flow_control_mode = 0,
   };
   struct dma_config DevConfig = {
      .dma_slot = 2,
      .channel_direction = PERIPHERAL_TO_MEMORY,
      .complete_callback_en = 1,
      .error_callback_en = 1,
      .source_handshake = 0, /* HW, probably ignored param */
      .dest_handshake = 1, /* SW, probably ignored param */
      .channel_priority = 0x0, /* Low prio, tbd if this is ok */
      .source_chaining_en = 0, /* ignored? */
      .dest_chaining_en = 0, /* ignored? */
      .linked_channel = 0, /* ignored? */
      .source_data_size = 2,
      .dest_data_size = 2,
      .source_burst_length = 1,
      .dest_burst_length = 1,
      .block_count = 1,
      .user_data = NULL,
      .dma_callback = DmaCallback,
      .head_block = &BlockConfig,
   };

   ReturnCode = dma_config(DmaDevice, 1, &DevConfig);
   if (ReturnCode != 0)
   {
      LOG_ERR("Dma config failed %d", ReturnCode);
   }

}

#if 0
internal void AdcDmaConfig(u16* Data, u32 Size)
{
   // Configuration of NVIC
   // Configure NVIC to enable DMA interruptions
   IRQ_CONNECT(DMA2_Stream1_IRQn, 0, DMA2Stream1IrqHandler, NULL, 0);
   irq_enable(DMA2_Stream1_IRQn);
   /*
   NVIC_SetPriority(DMA2_Stream1_IRQn, 1);  // DMA IRQ lower priority than ADC IRQ
   NVIC_EnableIRQ(DMA2_Stream1_IRQn);
   */

   // Configuration of DMA
   // Enable the peripheral clock of DMA
   LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);

   LL_DMA_SetChannelSelection(DMA2, LL_DMA_STREAM_1, LL_DMA_CHANNEL_2);

   // Configure the DMA transfer
   //    - DMA transfer in circular mode to match with ADC configuration:
   //      DMA unlimited requests.
   //    - DMA transfer from ADC without address increment.
   //    - DMA transfer to memory with address increment.
   //    - DMA transfer from ADC by half-word to match with ADC configuration:
   //      ADC resolution 12 bits.
   //    - DMA transfer to memory by half-word to match with ADC conversion data
   //      buffer variable type: half-word.

   LL_DMA_ConfigTransfer(
         DMA2,
         LL_DMA_STREAM_1,
         LL_DMA_DIRECTION_PERIPH_TO_MEMORY |
         LL_DMA_MODE_CIRCULAR              |
         LL_DMA_PERIPH_NOINCREMENT         |
         LL_DMA_MEMORY_INCREMENT           |
         LL_DMA_PDATAALIGN_HALFWORD        |
         LL_DMA_MDATAALIGN_HALFWORD        |
         LL_DMA_PRIORITY_HIGH
         );

   // Set DMA transfer addresses of source and destination
   LL_DMA_ConfigAddresses(
         DMA2,
         LL_DMA_STREAM_1,
         LL_ADC_DMA_GetRegAddr(ADC3, LL_ADC_DMA_REG_REGULAR_DATA),
         (uint32_t)Data,
         LL_DMA_DIRECTION_PERIPH_TO_MEMORY
         );

   // Set DMA transfer size
   LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_1, Size);

   // Enable DMA transfer interruption: transfer complete
   LL_DMA_EnableIT_TC(DMA2, LL_DMA_STREAM_1);

   // Enable DMA transfer interruption: half transfer
   LL_DMA_EnableIT_HT(DMA2, LL_DMA_STREAM_1);

   // Enable DMA transfer interruption: transfer error
   LL_DMA_EnableIT_TE(DMA2, LL_DMA_STREAM_1);

   // Activation of DMA
   // Enable the DMA transfer
   LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_1);
}
#endif

internal inline void ActivateAdc(ADC_TypeDef* Adc)
{
   // ADC must be disabled at this point
   if (LL_ADC_IsEnabled(Adc) != 0)
   {
      LOG_ERR("ADC should not be already eanbled");
   }

   // Enable ADC
   LL_ADC_Enable(Adc);
}

internal inline void EnableAdcChannel(ADC_TypeDef* Adc, u32 Channel, u32 Rank)
{
   // Set ADC group regular sequence: Channel on the selected sequence Rank.
   LL_ADC_REG_SetSequencerRanks(Adc, Rank, Channel);
   LL_ADC_SetChannelSamplingTime(Adc, Channel, LL_ADC_SAMPLINGTIME_144CYCLES);
}

internal inline void StartAdc(ADC_TypeDef* Adc)
{
   if (LL_ADC_IsEnabled(Adc))
   {
      LL_ADC_REG_StartConversionExtTrig(Adc, LL_ADC_REG_TRIG_EXT_RISING);
   }
}

internal inline void StopAdc(ADC_TypeDef* Adc)
{
   if (LL_ADC_IsEnabled(Adc))
   {
      LL_ADC_REG_StopConversionExtTrig(Adc);
   }
}

internal void AdcConfig(
      ADC_TypeDef* Adc,
      uint32_t TimerTriggerId,
      uint32_t AdcPeriphId
   )
{
   // Configuration of NVIC
   // Configure NVIC to enable ADC interruptions
   IRQ_CONNECT(ADC_IRQn, 0, ADCIrqHandler, NULL, 0);
   irq_enable(ADC_IRQn);

   // Enable ADC clock (core clock)
   LL_APB2_GRP1_EnableClock(AdcPeriphId);

   static bool AdcCommonInitialized = false;

   if (!AdcCommonInitialized)
   {
      // ADC common instance must not be enabled at this point
      if (__LL_ADC_IS_ENABLED_ALL_COMMON_INSTANCE() != 0)
      {
         LOG_ERR("ADC Common instance already initialized");
      }

      // Set ADC clock (conversion clock) common to several ADC instances
      LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(Adc), LL_ADC_CLOCK_SYNC_PCLK_DIV2);

      AdcCommonInitialized = true;
   }

   // ADC must not be enabled at this point
   if (LL_ADC_IsEnabled(Adc) != 0)
   {
      LOG_ERR("ADC should not be enabled yet");
   }

   // Set ADC data resolution
   LL_ADC_SetResolution(Adc, LL_ADC_RESOLUTION_12B);

   // Set ADC conversion data alignment
   LL_ADC_SetResolution(Adc, LL_ADC_DATA_ALIGN_RIGHT);

   // Set Set ADC sequencers scan mode, for all ADC groups
   // (group regular, group injected).
   LL_ADC_SetSequencersScanMode(Adc, LL_ADC_SEQ_SCAN_DISABLE);

   // Set ADC group regular trigger source
   LL_ADC_REG_SetTriggerSource(Adc, TimerTriggerId);

   // Set ADC group regular trigger polarity
   // LL_ADC_REG_SetTriggerEdge(Adc, LL_ADC_REG_TRIG_EXT_RISING);

   // Set ADC group regular continuous mode
   LL_ADC_REG_SetContinuousMode(Adc, LL_ADC_REG_CONV_SINGLE);

   // Set ADC group regular conversion data transfer
   LL_ADC_REG_SetDMATransfer(Adc, LL_ADC_REG_DMA_TRANSFER_UNLIMITED);

   // Set ADC group regular sequencer length and scan direction
   LL_ADC_REG_SetSequencerLength(Adc, LL_ADC_REG_SEQ_SCAN_DISABLE);

   // Set ADC group regular sequencer discontinuous mode
   // LL_ADC_REG_SetSequencerDiscont(Adc, LL_ADC_REG_SEQ_DISCONT_DISABLE);

   // Enable interruption ADC group regular overrun
   LL_ADC_EnableIT_OVR(Adc);
}

internal void Adc3Init(u16* Data, u32 BufferSize)
{
   AdcDmaConfig(Data, BufferSize);

   AdcConfig(ADC3, LL_ADC_REG_TRIG_EXT_TIM2_TRGO, LL_APB2_GRP1_PERIPH_ADC3);

   ActivateAdc(ADC3);

   // Set timer the trigger output (TRGO)
   LL_TIM_SetTriggerOutput(TIM2, LL_TIM_TRGO_UPDATE);
}
