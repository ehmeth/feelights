# feelights
A dancefloor lighting system using WS2812 LED strips based on ZephyrOS

## Application description
FeeLights is a dancefloor lighting controller that aims to make dancing more pleasurable by ajusting the lighting of a venue to the energy of the music playing.
To capture audio, the FeeLights controller uses a built in microphone, so no additional audio cabling is required to install it in a dance venue.
The system requires WS2812B addressable LED strips as the light source. The controller can power a very short strip for bringup testing, but in general the strips should be powered from external sources.
Multiple strips can be used in parallel to provide more light, but this version of FeeLights software does not support putting different patterns on more than one LED strip

## Hardware description
The system is essentially a straightforward input/output data transformation system with a few auxilary components.

<!--- TODO(kleindan) hardware diagram here!  -->

The core of the system is a STM32F429I DISCOVERY evaluation board, sporting a pretty beefy STM32F429ZI MCU with a Cortex-M4 core and an FPU unit for DSP support.
FeeLights makes use of the MCU internal ADC and one SPI peripheral (MOSI line only) for WS2812B driving.
The STM32F429ZI offers 192 KB of SRAM and 2 MB of flash memory, which is more than enough for the software at this point.
The board offers an additional 8 MB of external SDRAM memory, but it was not necessary for this project.

Additional hardware components include:
- SN74HCT541N Octal Buffer as a level-shifter and line driver for communicating with the LED strip.
- Electret microphone with MAX9814 adjustable-gain amplifier from [Adafruit](https://learn.adafruit.com/adafruit-agc-electret-microphone-amplifier-max9814/).
- Optional UART connection to the PC for a debug shell.
- On-board user switch/button for switching to other operating modes of the device

The controller is powered via the DISCOVERY board USB port. 
Power consumption measurements were not conducted, but given it is a driver for LED strips that consume up to 9 Watts of power per 5 meters (16'4'), the power consumption of the controller can be considered negligable.

## Software description
The controller software is based on Zephyr OS and has been tested with version <!--- TODO(kleindan) VERSION. -->

The code is architected as a simple, event driven state-machine.

<!--- TODO(kleindan) state chart here!  -->

To avoid spending time on moving data around, we're taking full advantage of DMA for transferring audio samples from ADC to memory and out from memory to the LED strips.

## Components description

The code is organized into several modules with well defined responsibilities.

### Main module
Entry point of the software. Initializes other modules at startup and runs the main event loop.

### AudioIn module
Module responsible for reading audio samples and generating an event once a new batch of samples is available.

Due to a lacking implementation of the ADC API in Zephyr OS, the ADC and Timer module configuration had to be done bypassing the OS and using the STM32 LowLevel libraries.

The module generates an event every 512 samples at 40 kSamples/s, so approximately every 12,8ms

### Button module
Aptly named, responsible for handling button presses with simple debouncing. Generates events any time the button is pressed and released.

### Dsp module
Auxilary module used for calculating the frequency magnitude spectrum using the CMSIS Dsp Real FFT transform functions

### Lights module


### Describe code you re-used from other sources, including the licenses for those

## Diagram(s) of the architecture

## Build instructions

### Linux

To build the software:
1. Install `west` and the Zephyr SDK by following the [Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
2. Initialize a Zephyr workspace using the following commands:
```
# initialize my-workspace for the example-application (main branch)
west init -m git@github.com:ehmeth/feelights.git --mr main fl-workspace
# update Zephyr modules
cd fl-workspace
west update
```
3. Build the software using the following commands:
```
cd feelights
west build -b stm32f429i_disc1 -p -s app
```

4. To flash the software onto the board use
```
west flash
```

### Windows
All development can be done using a WSL2 instance of a Linux distro (tested on Ubuntu 20.04 LTS)

To connect the board to the Linux distro via USB additional are required using an open-source project called `usbipd-win`.
You can follow the instruction on the [usbpid-win github page](https://github.com/dorssel/usbipd-win) or follow [this guide from Microsoft](https://docs.microsoft.com/en-us/windows/wsl/connect-usb).

After that you should be able to connect the board to your WSL instance, but there might be an access rights problem to flash/debug the device.
In theory the Zephyr getting started guide includes adding special rules to allow using the device without super-user rights, but those tend not to be loaded on first boot of the WSL instance.
The following command will force a reload of the `udev` rules and you should be able to connect without `sudo`..
```
sudo service udev restart; sudo udevadm control --reload-rules
```

### Debugging
The Zephyr build tool `west` offers a one-command way to connect a gdb debugger instance to the device, just type:
```
west debug
```

I would highly recommend finding a good gdb guide and familarize yourself with at least the basics of running and stopping execution, setting breakpoints and displaying variable contents.. Other than that debugging "just works".

### How you powered it (and how you might in the future)

## Future
### What would be needed to get this project ready for production?
### How would you extend this project to do something more? Are there other features you’d like? How would you go about adding them?

# Grading
# Self assessment of the project: for each criteria, choose a score (1, 2, 3) and explain your reason for the score in 1-2 sentences.  # Have you gone beyond the base requirements? How so?

# Acknowledgements

The AudioIn module implementation was largely based on [infinity_drive](https://github.com/cycfi/infinity_drive), an open-source project by Cycfi Research (MIT License)
