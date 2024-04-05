# LED panel

Initially based on the project of the [LHC](www.lhc.net.br)'s [LED pannel](https://lhc.net.br/w/index.php?title=Painel_de_LED).
The original page was based on [platformio](https://platformio.org/). More details on its [page on GitHub](https://github.com/lhc/nodeLHC_WS2811_LED_Controller).

## Description

The goal of this project was to take practice with the [ESP8266 SDK](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/)
and using Unit Test with [CppUTest](https://cpputest.github.io/).

This project is intended to control a LED panel build with addressable LED (like WS2811, WS2812).
It implements 2 UDP sockets to control the panel using the protocol [ArtNet](https://en.wikipedia.org/wiki/Art-Net) and [MXP](https://github.com/Jeija/WS2811LEDMatrix).

## How to use

using the `make.sh`
```bash
bash src/make.sh "cd build/ && cmake .. && make -j8"    # Using cmake will generate the compile_commands.json
bash src/make.sh "make simple_monitor"                  # Uses a target of the make file 
```

The `make.sh` script uses the docker to run all commands. Below is described some docker commands.

```bash
docker build -t esp-docker:1.0 .
# Configure the SSID, Password and panel's number of LEDs on menuconfig
docker run --rm -it -v (git rev-parse --show-toplevel):(git rev-parse --show-toplevel) --device=/dev/ttyUSB0 --workdir (git rev-parse --show-toplevel)/src esp-docker:1.0 bash -c "make menuconfig"
# build, flash and open the serial monitor
docker run --rm -it -v (git rev-parse --show-toplevel):(git rev-parse --show-toplevel) --device=/dev/ttyUSB0 --workdir (git rev-parse --show-toplevel)/src esp-docker:1.0 bash -c "make flash monitor"
```

## MXP UDP protocol for WS2811/WS2812 controller

Check ```mxp.h```.

## Art-Net UDP protocol for WS2811/WS2812 controller

[Art-Net](https://en.wikipedia.org/wiki/Art-Net) is a UDP-based protocol used mainly for lighting controllers. Current implementation read data from ```Net: 0```, ```Subnet: 0``` and ```Universe: 0```. 

Check [Glediator](http://www.solderlab.de/index.php/software/glediator) and [Jinx!](http://www.live-leds.de/) applications to generate awesome effects!

