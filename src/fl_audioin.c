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

/* TODO(kleindan)
 * Finish the ADC overhaul based on https://github.com/cycfi/infinity_drive/ (MIT License)
 * Also we need a west.yml file for easier building in the future: https://github.com/kpochwala/kubot-software/blob/main/west.yml
 */
internal u32 SeqLength(u32 NumChannels)
internal void AdcDmaConfig(ADC_TypeDef* Adc, uint32_t DmaStream, uint32_t DmaChannel, IRQn_Type DmaChannelIrq, uint16_t values[], uint16_t size)
internal inline void ActivateAdc(ADC_TypeDef* Adc)
internal inline void EnableAdcChannel(ADC_TypeDef* Adc, u32 Channel, u32 Rank)
internal inline void StartAdc(ADC_TypeDef* Adc)
internal inline void StopAdc(ADC_TypeDef* Adc)
internal void AdcConfig(ADC_TypeDef* Adc, uint32_t TimerTriggerId, uint32_t AdcPeriphId, uint32_t NumChannels)
internal void Adc3Init(u16* Data, u32 Size)

#define DMA_NODE		DT_ALIAS(mic_dma)

/* This function is executed in the interrupt context */
internal void DmaCallback(const struct device *dev, void *arg, uint32_t channel, int status)
{
   LL_TIM_DisableCounter(TIM2);
	if (status != 0) {
		LOG_ERR("DMA callback error with channel %d.", channel);
	} else {
      LOG_INF("DMA callback!");
	}
}

struct {
   const struct device *DmaDev;
   u32 Channel;
   struct dma_config DmaCfg;
   struct dma_block_config DmaBlkCfg;
   u8 Priority;
   bool SrcAddrIncrement;
   bool DstAddrIncrement;
	int FifoThreshold;
} DmaStream = {
   .DmaDev = DEVICE_DT_GET(DMA_NODE),
   .Channel = 2,
   .DmaCfg = 
   {
      .dma_slot = 1,
		.channel_direction = 0x2,
		.source_data_size = 0x1,
		.dest_data_size = 0x1,
		.source_burst_length = 1, /* SINGLE transfer */
		.dest_burst_length = 1, /* SINGLE transfer */
		.channel_priority = 0x1,
		.dma_callback = DmaCallback,
		.block_count = 1
   },
   .Priority = 0x01,
   .SrcAddrIncrement = DMA_ADDR_ADJ_NO_CHANGE,
   .DstAddrIncrement = DMA_ADDR_ADJ_INCREMENT,
	.FifoThreshold = 0x3
};

u32 AudioInInit()
{
   u32 Result = 0;
#if 0
   LL_GPIO_InitTypeDef GPIO_InitStruct; //GPIO init structure
   LL_TIM_InitTypeDef TIM2_InitStruct; //TIM2 init structure
   LL_TIM_OC_InitTypeDef TIM2_OCInitStruct; //TIM2 OC init structure
   LL_ADC_InitTypeDef ADC_InitStruct; //ADC init structure
   LL_ADC_CommonInitTypeDef ADC_ComInitStruct; //ADC common init structure
   LL_ADC_REG_InitTypeDef ADC_RegInitStruct; //ADC regular conversion init structure

   /* TBD if this is necessary */
   /*
   IRQ_CONNECT(DT_INST_IRQN(0),
         DT_INST_IRQ(0, priority),
         adc_stm32_shared_irq_handler, NULL, 0);
   irq_enable(DT_INST_IRQN(0));
   */

   __HAL_RCC_GPIOF_CLK_ENABLE(); //enable clock to the GPIOA peripheral
   LL_GPIO_StructInit(&GPIO_InitStruct); //structure initialization to default values
   GPIO_InitStruct.Pin= GPIO_PIN_10; //set pin 10 (ADC_IN8)
   GPIO_InitStruct.Mode= LL_GPIO_MODE_ANALOG; //set GPIO as analog mode
   GPIO_InitStruct.Pull= LL_GPIO_PULL_NO; //no pull up or pull down
   Result = LL_GPIO_Init(GPIOF,&GPIO_InitStruct); //initialize GPIOA, pins 1 and 4
   if (Result != SUCCESS) { LOG_ERR("Failed xxx Init, %d", Result); }
   LL_GPIO_SetPinMode(GPIOF, LL_GPIO_PIN_10, LL_GPIO_MODE_ANALOG);

   LL_ADC_CommonStructInit(&ADC_ComInitStruct);
   ADC_ComInitStruct.CommonClock = LL_ADC_CLOCK_SYNC_PCLK_DIV2;
   Result = LL_ADC_CommonInit(ADC123_COMMON, &ADC_ComInitStruct);
   if (Result != SUCCESS) { LOG_ERR("Failed xxx Init, %d", Result); }

   LL_ADC_StructInit(&ADC_InitStruct);
   ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_12B ;
   ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
   ADC_InitStruct.SequencersScanMode = LL_ADC_SEQ_SCAN_DISABLE;
   Result = LL_ADC_Init(ADC3,&ADC_InitStruct);
   if (Result != SUCCESS) { LOG_ERR("Failed xxx Init, %d", Result); }

   LL_ADC_REG_StructInit(&ADC_RegInitStruct);
   ADC_RegInitStruct.TriggerSource = LL_ADC_REG_TRIG_EXT_TIM2_TRGO;
   ADC_RegInitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
   ADC_RegInitStruct.SequencerLength = LL_ADC_REG_SEQ_SCAN_DISABLE;
   ADC_RegInitStruct.ContinuousMode = LL_ADC_REG_CONV_SINGLE;
   ADC_RegInitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_NONE;
   Result = LL_ADC_REG_Init(ADC3,&ADC_RegInitStruct);
   if (Result != SUCCESS) { LOG_ERR("Failed xxx Init, %d", Result); }

   LL_ADC_REG_SetSequencerRanks(ADC3,LL_ADC_REG_RANK_1,LL_ADC_CHANNEL_8);
   LL_ADC_SetChannelSamplingTime(ADC3, LL_ADC_CHANNEL_8 , LL_ADC_SAMPLINGTIME_56CYCLES);

   /* TIM2 LL configuration */
   __HAL_RCC_TIM2_CLK_ENABLE(); //enable clock
   LL_TIM_StructInit(&TIM2_InitStruct);
   TIM2_InitStruct.Prescaler = 180;
   TIM2_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
   TIM2_InitStruct.Autoreload = 10;
   TIM2_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
   Result = LL_TIM_Init(TIM2,&TIM2_InitStruct);
   if (Result != SUCCESS) { LOG_ERR("Failed xxx Init, %d", Result); }

   LL_TIM_SetTriggerOutput(TIM2, LL_TIM_TRGO_UPDATE);
 
   LL_ADC_Enable(ADC3);
   LL_ADC_EnableIT_EOCS(ADC3);
   LL_TIM_EnableCounter(TIM2);
   LL_ADC_REG_StartConversionExtTrig(ADC3,LL_ADC_REG_TRIG_EXT_RISING);
#endif

   return Result;
}

int AudioInRequestRead(u16* Buffer, u32 BufferSize, u32 NumSamples, fl_event EvSamplesAvailable)
{
   u32 Result = 0;
#if 0
   LL_DMA_InitTypeDef DMA_ADC_InitStruct;
   LL_DMA_StructInit(&DMA_ADC_InitStruct);
   DMA_ADC_InitStruct.Channel = LL_DMA_CHANNEL_2;
   DMA_ADC_InitStruct.PeriphOrM2MSrcAddress = LL_ADC_DMA_GetRegAddr(ADC3,LL_ADC_DMA_REG_REGULAR_DATA);
   DMA_ADC_InitStruct.MemoryOrM2MDstAddress = (uint32_t)Buffer;
   DMA_ADC_InitStruct.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
   DMA_ADC_InitStruct.Mode = LL_DMA_MODE_NORMAL;
   DMA_ADC_InitStruct.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
   DMA_ADC_InitStruct.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
   DMA_ADC_InitStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_HALFWORD;
   DMA_ADC_InitStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_HALFWORD;
   DMA_ADC_InitStruct.NbData = NumSamples;
   DMA_ADC_InitStruct.Priority = LL_DMA_PRIORITY_LOW;
   Result = LL_DMA_Init(DMA2, LL_DMA_STREAM_0, &DMA_ADC_InitStruct);
   if (Result != SUCCESS) { LOG_ERR("Failed xxx Init, %d", Result); }
   LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_0);

   LL_TIM_EnableCounter(TIM2);
#endif

   LOG_INF("Current counter value: %d, ADC %d", LL_TIM_GetCounter(TIM2), LL_ADC_REG_ReadConversionData12(ADC3));



	return 0;
}

struct Adc3Info
{
   const ADC_TypeDef* adc = ADC3;
   const uint32_t PeriphId = LL_APB2_GRP1_PERIPH_ADC3;
   const IRQn_Type DmaIrqId = DMA2_Stream1_IRQn;
   const uint32_t DmaStream = LL_DMA_STREAM_1;
   const uint32_t DmaChannel = LL_DMA_CHANNEL_2;
};

internal u32 SeqLength(u32 NumChannels)
{
   switch (NumChannels)
   {
      case 2: return LL_ADC_REG_SEQ_SCAN_ENABLE_2RANKS;
      case 3: return LL_ADC_REG_SEQ_SCAN_ENABLE_3RANKS;
      case 4: return LL_ADC_REG_SEQ_SCAN_ENABLE_4RANKS;
      case 5: return LL_ADC_REG_SEQ_SCAN_ENABLE_5RANKS;
      case 6: return LL_ADC_REG_SEQ_SCAN_ENABLE_6RANKS;
      case 7: return LL_ADC_REG_SEQ_SCAN_ENABLE_7RANKS;
      case 8: return LL_ADC_REG_SEQ_SCAN_ENABLE_8RANKS;
      case 9: return LL_ADC_REG_SEQ_SCAN_ENABLE_9RANKS;
      case 10: return LL_ADC_REG_SEQ_SCAN_ENABLE_10RANKS;
      case 11: return LL_ADC_REG_SEQ_SCAN_ENABLE_11RANKS;
      case 12: return LL_ADC_REG_SEQ_SCAN_ENABLE_12RANKS;
      case 13: return LL_ADC_REG_SEQ_SCAN_ENABLE_13RANKS;
      case 14: return LL_ADC_REG_SEQ_SCAN_ENABLE_14RANKS;
      case 15: return LL_ADC_REG_SEQ_SCAN_ENABLE_15RANKS;
      default: return LL_ADC_REG_SEQ_SCAN_DISABLE;
   }
}

internal void AdcDmaConfig(
      ADC_TypeDef* Adc,
      uint32_t DmaStream,
      uint32_t DmaChannel,
      IRQn_Type DmaChannelIrq,
      uint16_t values[],
      uint16_t size
   )
{
   const DMA_TypeDef* Dma = DMA2;
   // Configuration of NVIC
   // Configure NVIC to enable DMA interruptions
   NVIC_SetPriority(DmaChannelIrq, 1);  // DMA IRQ lower priority than ADC IRQ
   NVIC_EnableIRQ(DmaChannelIrq);

   // Configuration of DMA
   // Enable the peripheral clock of DMA
   LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);

   LL_DMA_SetChannelSelection(Dma, DmaStream, DmaChannel);

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
         Dma,
         DmaStream,
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
         Dma,
         DmaStream,
         LL_ADC_DMA_GetRegAddr(Adc, LL_ADC_DMA_REG_REGULAR_DATA),
         reinterpret_cast<uint32_t>(values),
         LL_DMA_DIRECTION_PERIPH_TO_MEMORY
         );

   // Set DMA transfer size
   LL_DMA_SetDataLength(Dma, DmaStream, size);

   // Enable DMA transfer interruption: transfer complete
   LL_DMA_EnableIT_TC(Dma, DmaStream);

   // Enable DMA transfer interruption: half transfer
   LL_DMA_EnableIT_HT(Dma, DmaStream);

   // Enable DMA transfer interruption: transfer error
   LL_DMA_EnableIT_TE(Dma, DmaStream);

   // Activation of DMA
   // Enable the DMA transfer
   LL_DMA_EnableStream(Dma, DmaStream);
}

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

internal inline void EnableAdcChannel(
      ADC_TypeDef* Adc, u32 Channel, u32 Rank)
{
   // Set ADC group regular sequence: Channel on the selected sequence Rank.
   LL_ADC_REG_SetSequencerRanks(Adc, Rank, Channel);
   LL_ADC_SetChannelSamplingTime(Adc, Channel, LL_ADC_SAMPLINGTIME_3CYCLES);
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
      uint32_t AdcPeriphId,
      uint32_t NumChannels
   )
{
   // Configuration of NVIC
   // Configure NVIC to enable ADC interruptions
   NVIC_SetPriority(ADC_IRQn, 0); // ADC IRQ greater priority than DMA IRQ
   NVIC_EnableIRQ(ADC_IRQn);

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
   LL_ADC_REG_SetSequencerLength(Adc, SeqLength(NumChannels));

   // Set ADC group regular sequencer discontinuous mode
   // LL_ADC_REG_SetSequencerDiscont(Adc, LL_ADC_REG_SEQ_DISCONT_DISABLE);

   // Enable interruption ADC group regular overrun
   LL_ADC_EnableIT_OVR(Adc);
}

internal void Adc3Init(u16* Data, u32 Size)
{
   // TODO(kleindan) Is this important?! 
   // detail::system_clock_config();

   AdcDmaConfig(
         ADC3,
         LL_DMA_STREAM_1,
         LL_DMA_CHANNEL_2,
         DMA2_Stream1_IRQn;
         Data,
         Size
         );

   AdcConfig(
         ADC3,
         LL_ADC_REG_TRIG_EXT_TIM2_TRGO,
         LL_APB2_GRP1_PERIPH_ADC3,
         channels
         );

   detail::ActivateAdc(ADC3);

   // Set timer the trigger output (TRGO)
   LL_TIM_SetTriggerOutput(TIM2, LL_TIM_TRGO_UPDATE);

   // Clear the ADC buffer
   clear();
}
