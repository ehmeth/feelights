#include "fl_common.h"
#include "fl_button.h"
#include "fl_events.h"

#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>

#define LOG_LEVEL 4
#include <logging/log.h>
LOG_MODULE_REGISTER(button);

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

internal const struct gpio_dt_spec ButtonSpec = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
internal struct gpio_callback ButtonCbData;
internal struct k_timer DebounceTimer;

internal void PressReleaseHandler(struct k_timer *dummy)
{
   EventEmit(ButtonIsPressed() ? EV_BUTTON_PRESSED : EV_BUTTON_RELEASED);
}

internal void ButtonChangeHandler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
   /* wait for level to stabilize, starting a running timer will reset it */
   k_timer_start(&DebounceTimer, K_MSEC(5), K_NO_WAIT);
}

u32 ButtonInit()
{
	int init_error = 0;

	if (!device_is_ready(ButtonSpec.port)) {
		LOG_ERR("button device %s is not ready", ButtonSpec.port->name);
	}

   if (!init_error)
   {
      init_error = gpio_pin_configure_dt(&ButtonSpec, GPIO_INPUT);
      if (init_error) {
         LOG_ERR("%d: failed to configure %s pin %d", init_error, ButtonSpec.port->name, ButtonSpec.pin);
      }
   }

   if (!init_error)
   {
      init_error = gpio_pin_interrupt_configure_dt(&ButtonSpec, GPIO_INT_EDGE_BOTH);
      if (init_error) {
         LOG_ERR("%d: failed to configure interrupt on %s pin %d", init_error, ButtonSpec.port->name, ButtonSpec.pin);
      }
   }

   if (!init_error)
   {
      gpio_init_callback(&ButtonCbData, ButtonChangeHandler, BIT(ButtonSpec.pin));
      gpio_add_callback(ButtonSpec.port, &ButtonCbData);
      LOG_INF("Set up button at %s pin %d", ButtonSpec.port->name, ButtonSpec.pin);
   }

   k_timer_init(&DebounceTimer, PressReleaseHandler, NULL);

   return init_error;
}

bool ButtonIsPressed()
{
   int State = gpio_pin_get_dt(&ButtonSpec);

   if (State < 0) {
      LOG_ERR("Error %d: reading state of %s pin %d", State, ButtonSpec.port->name, ButtonSpec.pin);
      return false;
   } 

   return State == 0;
}

