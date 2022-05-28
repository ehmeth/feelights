#include "fl_common.h"
#include "fl_audioin.h"

#include "zephyr.h"
#include <drivers/adc.h>
#include <device.h>

#define LOG_LEVEL 4
#include <logging/log.h>
LOG_MODULE_REGISTER(audioin);

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define ADC_NODE		DT_PHANDLE(DT_PATH(zephyr_user), io_channels)

internal struct adc_channel_cfg ChannelCfg = {
	.gain = ADC_GAIN_1,
	.reference = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQ_TIME_DEFAULT,
	.channel_id = 8,
	.differential = 0
};

internal struct adc_sequence_options Options = {
   .interval_us = 22,
   .extra_samplings = 0,
};

internal struct adc_sequence Sequence = {
	/* individual channels will be added below */
	.channels    = BIT(8),
	.buffer      = 0,
	/* buffer size in bytes, not number of samples */
	.buffer_size = 0,
	.resolution  = 12,
   .options     = &Options,
};


internal const struct device *DevAdc = DEVICE_DT_GET(ADC_NODE);

u32 AudioInInit()
{
   u32 Result = 0;


	if (!device_is_ready(DevAdc)) {
		printk("ADC device not found\n");
		return 4321;
	}
   adc_channel_setup(DevAdc, &ChannelCfg);

   return Result;
}

u32 AudioInRequestRead(u16* Buffer, u32 BufferSize, u32 NumSamples, fl_event EvSamplesAvailable)
{
   int rc = 0;
   Sequence.buffer = Buffer;
   Sequence.buffer_size = BufferSize;
   Options.extra_samplings = NumSamples - 1;

   rc = adc_read_async(DevAdc, &Sequence, &OsSiglnals[EvSamplesAvailable]);
   if (rc != 0) {
      printk("ADC reading failed with error %d.\n", rc);
   }

   return 0;
}
