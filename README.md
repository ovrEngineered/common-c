# OpenCXA C Libraries

This is a set of C libraries designed to provide all of the nice-to-have features of more modern and full-featured languages, but targeted for an embedded (and usually "bare metal") environment.

To the fullest extent possible these libraries abide by the following rules:  
##### 1. Favor code re-use by being hardware agnostic
If you write enough code, you'll come across design patterns, be them ["well known"](https://en.wikipedia.org/wiki/Software_design_pattern) or just ones that are common to your style or the niche you're currently working in. You should codify these patterns and re-use them whenever possible. It not only reduces the time required for future projects, it reduces the risk. And when you're forced to use a different hardware platform, it's really useful if those patterns are easily adapted to the new platform through the use of abstraction layers.

##### 2. No dynamic memory allocation
In my experience, dynamic memory allocation can cause a lot of insidious problems, especially when dealing with multiple architectures. Did you match all of your `malloc` / `free` calls? Does your implementation of `malloc` / `free` join contiguous free blocks? Is there an easy way to debug memory allocation problems on your target? In a lot of cases the answer to these questions is "I don't know"...and that's fine...just don't use dynamic memory allocation.

##### 3. Make it easy to support other platforms
When you boil it down, there are only a few _basic_ requirements for any given hardware platform:
* General purpose input / output (gpio)
* Delay (delay)
* Serial communication (usart)
* Timing information (timeBase)

If you can provide these basics, you've got yourself a pretty full-featured embedded system.

##### 4. Emulate object-oriented paradigms
Some people may say object-oriented paradigms are slowly dying, but to me, they're easy to understand and do the job well enough. C doesn't support "proper" object-oriented paradigms. C++ is (IMHO) ugly, can cause issues related to #2 above, and isn't supported on all embedded platforms. Other languages like [Rust](https://www.rust-lang.org/) are certainly neat, but let's be honest, I like embedded development because it isn't like web development: I don't want to learn a new language, new language revision, or framework every 3 months. So let's just emulate object-oriented paradigms in C...it can be done, it just requires some boilerplate code and some discipline.

##### 5. No integrated build system
There are a lot of hardware platforms out there, a lot of which have their own build system. We can't possibly support all of these build systems, so it's probably best that we support none of them. Just copy the headers and source code into your project (or better yet, use a git submodule), compile the code you need, forget about the rest, carry on.


## Modules / Features
##### Bluetooth Low Energy (BTLE)
This module provides support for Bluetooth Low Energy in the form of full-featured, hardware-agnostic, [central and peripheral](https://learn.adafruit.com/introduction-to-bluetooth-low-energy/gatt#connected-network-topology-3-2) classes.

##### Collections
This module provides the basic collections classes present in most modern languages including:
 * Array - Array of a single element type utilizing a statically allocated buffer
 * Fixed Byte Buffer - Array of bytes utilizing a statically allocated buffer with convenience functions for byte-oriented operations.
 * Fixed Fifo - First in, first out queue utilizing a statically allocated Buffer
 * Linked Field - Construct for breaking down an array of bytes into "fields" an manipulating them individually

##### Command Line Parser
This module is targeted at POSIX hardware platforms and provides a standardized mechanism for processing command-line arguments (arguments passed to the executable from the linux command line) similar to the [GLib CommandLine Option Parser](https://developer.gnome.org/glib/stable/glib-Commandline-option-parser.html).

##### Console
This module provides a _basic_ real-time console which can be used to execute commands. Common use-cases involve a debugging console bound to the serial port (USART) of an embedded target.

##### Logger
This module provides logging capabilities in a similar manner to [Log4J](https://logging.apache.org/log4j/2.x/manual/usage.html). Common use-cases involve logging to a serial port (USART) of an embedded target.

##### MQTT
This module provided basic [MQTT](http://mqtt.org/) support through the use of a custom MQTT client and connection manager. This module is compatible with [AWS IoT](https://aws.amazon.com/iot/).

##### MQTT-RPC
This module is located within the MQTT module and provides a framework for organizing MQTT topics to support basic [RPC](https://en.wikipedia.org/wiki/Remote_procedure_call) for many connected devices. This module is compatible with [AWS IoT](https://aws.amazon.com/iot/).

##### Net
This module provides abstract implementations of:
 * Ethernet interfaces
 * Wi-Fi interfaces
 * TCP Clients
 * TCP Servers

It provides concrete implementations of the TCP clients and servers for the following stacks:
 * [lwIP](https://savannah.nongnu.org/projects/lwip/)
 * [mbedTLS](https://tls.mbed.org/)
 * [wolfSSL](https://www.wolfssl.com/)

It also provides a stack-agnostic implementation of a HTTP Client.

##### RunLoop
This module, in many ways, is the heart of the whole library. It provides mechanisms to register for execution within a managed runloop, similiar to the [GLib Main Event Loop](https://developer.gnome.org/glib/stable/glib-The-Main-Event-Loop.html). The runloop can be serviced by one or more threads, allowing for implementations to work on "bare metal" hardware platforms OR hardware platforms utilizing a basic OS such as [FreeRTOS](https://www.freertos.org/). It also provides convenience classes for modifying program execution such as a `oneShotTimer` and `softWatchdog`.

##### Serial
This module operates around the concept of an `ioStream`: a class which ultimately provides a single function to write bytes, and a single function to read bytes. There are many types of `ioStreams` to support routing bytes from one destination to another, but the most common use-case is for reading/writing bytes to a USART.

##### State Machine
This module provides an implementation of a [finite state machine](https://en.wikipedia.org/wiki/Finite-state_machine). It is used throughout the rest of the library to maintain the internal state of the various modules.

##### Time Utils
This module provides a number of time-related functions, most important of which is a `timeDiff`. The `timeDiff` allows for the asynchronous implementation of timeouts (executing code after a certain amount of time has passed).


## Structure of this repository
 * `arch-XXX` - provides concrete hardware platform specific implementation of abstract classes
 * `arch-common` - provides a mix of abstract classes (such as `cxa_gpio`) as well as other hardware-agnostic classes
 * `arch-dummy` - provides a base hardware implementation which can be used as a starting point for supporting other hardware platforms

All other folders contain code related to the respective module identified in the name.


## Supported hardware platforms
 * Atmel ATMega (most architectures supported by AVR-GCC)
 * Atmel XMega
 * Silicon Labs BGM (most BGM modules)
 * ESP32
 * PIC32
 * POSIX (including Raspberry Pi)

Hardware support currently fluctuates depending on which platform I'm currently spending most of my time. Generally speaking, all of the architectures listed above are supported, but some may have some compilation issues if APIs have been changed since last I used them. Feel free to [open an issue](https://github.com/OpenCXA/common-c/issues) if you need something resolved.


## Adding this library to your existing project
For most embedded projects, I usually stick to the following file / folder structure:
 * myEmbeddedProject
   * externals
     * openCXA-common
       * include
       * src
       * ...
   * project
     * include
       * cxa_config.h
       * \<project specific includes\>
     * src
       * \<project-specific source files\>

##### Getting source code into your project
The contents of the `openCXA-common` directory can either be copied directly from this repository OR injected using a [git submodule](https://git-scm.com/book/en/v2/Git-Tools-Submodules) (preferred). To use a git submodule, assuming your project source code is already in git, create the `externals` folder, then run the following command:

```bash
git submodule add https://github.com/OpenCXA/common-c.git externals/openCXA-common
```
This can also be done in [SourceTree](https://www.sourcetreeapp.com/) by the `Repository` -> `Add Submodule` menu item.

##### Adding `cxa_config.h`
`cxa_config.h` **MUST** be present in the include path of your project during compilation. Most classes have at least some configuration options that optionally be changed in `cxa_config.h`. Here is a good starting point for this file:
```c
#ifndef CXA_CONFIG_H_
#define CXA_CONFIG_H_

#define CXA_ASSERT_EXIT_FUNC(eStat)                while(1);
#define CXA_ASSERT_LINE_NUM_ENABLE
#define CXA_ASSERT_MSG_ENABLE

#define CXA_CONSOLE_ENABLE

#define CXA_IOSTREAM_FORMATTED_BUFFERLEN_BYTES     80

#define CXA_LINE_ENDING                            "\r\n"

#endif /* CXA_CONFIG_H_ */
```

##### Setting up assertions, logging, and console
These 3 items provide immense benefit to the development process and I highly recommend setting them up as soon as possible. The code below outlines the general procedure for getting these items operational. You'll need to hookup your `rx` and `tx` lines to your favorite serial terminal (perhaps a USB <-> USART converter), and change the `yourArchiture` parts to match your target hardware platform.
```c
#include <cxa_assert.h>
#include <cxa_console.h>
#include <cxa_yourArchitecture_usart.h>

#define CXA_LOG_LEVEL         CXA_LOG_LEVEL_DEBUG
#include <cxa_logger_implementation.h>

void main(void)
{
   // initialize our USART
   cxa_yourArchitecture_usart_t debugUsart;
   // the following function prototype may be different depending on your modifications from above
   cxa_yourArchitecture_usart_init_noHH(&debugUsart, 115200);

   // get a pointer to our USART's ioStream and make sure it's valid
   cxa_ioStream_t* ios = cxa_usart_getIoStream(&debugUsart.super);
   cxa_assert(ios);

   // all failed `cxa_assert` macros will now print to our debug serial port
   cxa_assert_setIoStream(ios);
   // all logging will now show up on our debug serial port
   cxa_logger_setGlobalIoStream(ios);
   // run an interactive console on our debug serial port
   cxa_console_init("My Embedded Project", ios, CXA_RUNLOOP_THREADID_DEFAULT);
}
```


## Adding a new hardware platform
New platform support can usually be implemented in a somewhat piecemeal fashion: not all features need to be supported immediately. Speaking from experience, I've found it useful to implement _at least_ these features in the following order:
 1. General purpose input / output (gpio)
 1. Delay (delay)
 1. Serial communication (usart)
 1. Timing information (timeBase)

Getting `gpio` working allows you to turn on an LED which allows you to verify that your code is compiling, you can program your target platform, and your code is actually running. Adding a `delay` allows you to toggle your `gpio` at differing human-visible rates which can help in debugging. Getting a serial port working immediately gives you the ability to log to the serial port via `logger` and start implementing `console` commands. Finally, `timeBase` allows you to differentiate log commands temporally and add timeouts.

##### Create your include and source directories
 1. First you'll need to come up with a name for your architecture. Perhaps you're implementing on a TI C2000 series processor...you might call your architecture `tiC2000`.
 1. Create a folder in the `include` AND `src` directories named `arch-yourArchitecture`. Continuing with the TI C2000 example, your directory would be named `arch-tiC2000`.

##### Copy the "dummy" implementation to your new directories and rename
 1. Copy the files in the `include/arch-dummy` and `src/arch-dummy` directory to your newly created `include/arch-yourArchitecture` and `include/arch-yourArchitecture` directories respectively.
 1. Rename `cxa_dummy_gpio.h` to `cxa_yourArchitecture_gpio.h` (eg. `cxa_tiC2000_gpio.h`)
 1. Rename `cxa_dummy_gpio.c` to `cxa_yourArchitecture_gpio.c` (eg. `cxa_tiC2000_gpio.c`)
 1. Rename `cxa_dummy_usart.h` to `cxa_yourArchitecture_usart.h` (eg. `cxa_tiC2000_usart.h`)
 1. Rename `cxa_dummy_usart.c` to `cxa_yourArchitecture_usart.c` (eg. `cxa_tiC2000_usart.c`)
 1. Rename `cxa_dummy_delay.c` to `cxa_yourArchitecture_delay.c` (eg. `cxa_tiC2000_delay.c`)
 1. Do a find and replace in all of the above files: replace `_dummy_` with `_yourArchitecture` (eg. `_tiC2000_`)

##### Provide your concrete `gpio` implementation
 1. In `cxa_yourArchitecture_gpio.h` and `cxa_<your architecture_gpio.c` modify `cxa_yourArchitecture_gpio_init_input` and `cxa_yourArchitecture_gpio_init_input` as needed. You'll likely need into include some parameters to identify which ports / pins you'll be using. Don't forget to modify both the header and source files. You may need to add some `#include`s to the header file.
 1. Modify the body of `scm_setDirection` as needed to actually change the direction of the gpio pin. Do not change the prototype.
 1. Modify the body `scm_setValue` as needed to actually set the level of the gpio pin (assuming you're configured as an output). Do not change the prototype.
 1. Modify the body of `scm_getValue` as needed to actually report the level of the gpio pin. Do not change the prototype.

##### Test your `gpio` implementation
Hookup a LED or an oscilloscope to a GPIO pin on your hardware platform. In a file somewhere in your project (usually `main.c`) compile, program, and execute the following code (or similar):
```c
#include <cxa_yourArchitecture_gpio.h>

void main(void)
{
   cxa_yourArchitecture_gpio_t testGpio;
   // the following function prototype may be different depending on your modifications from above
   cxa_yourArchitecture_gpio_init_output(&testGpio, CXA_GPIO_POLARITY_NONINVERTED, 1);
}
```
At this point you should see your LED turn on. Replace the `1` with a `0` then compile, program, and execute again. Your LED should now be off.

##### Provide your concrete `delay` implementation
 1. In `cxa_yourArchitecture_delay.c` provide a simple blocking delay implementation within the body of `cxa_delay_ms`. Take a look at other architecture-specific implementations for hints as to how this may be accomplished.

##### Test your `delay` implementation
Modify your code from above to include a while loop and a delay like so:
```c
#include <cxa_yourArchitecture_gpio.h>

void main(void)
{
   cxa_yourArchitecture_gpio_t testGpio;
   // the following function prototype may be different depending on your modifications from above
   cxa_yourArchitecture_gpio_init_output(&testGpio, CXA_GPIO_POLARITY_NONINVERTED, 0);

   while( 1 )
   {
      cxa_gpio_toggle(&testGpio.super);
      cxa_delay_ms(1000);
   }
}
```
At this point, you should now see your LED blinking at a rate of 1 Hz.

##### Provide your concrete `usart` implementation
1. In `cxa_yourArchitecture_usart.h` and `cxa_<your architecture_usart.c` modify `cxa_yourArchitecture_usart_init_noHH` as needed. You'll likely need into include some parameters to identify which `tx` and `rx` ports / pins you'll be using. Don't forget to modify both the header and source files. You may need to add some `#include`s to the header file.
1. Modify the body of `ioStream_cb_readByte` as needed to read exactly one byte from the serial port. In some cases where a hardware FIFO is not available, I highly suggest implementing an interrupt-driven software fifo to avoid dropped bytes. See the `bgm` implementation for hints as to how this may be accomplished.
1. Modify the body `ioStream_cb_writeBytes` as needed to write the provided bytes to the serial port. This is a simplistic implementation so what "write" means may differ from platform to platform. On devices with a hardware transmit FIFO, simply queuing all of the bytes should be acceptable. On devices without a hardware transmit FIFO, writing the bytes one-at-a-time should be acceptable. Regardless, this function should wait until all bytes are either transmitted or queued for transmission before returning. Only return `false` if there was an error.

##### Test your `usart` implementation
Hookup your `tx` and `rx` lines to your serial terminal of choice (perhaps through a USB <-> UART converter). Modify your code from above and compile, program, and execute the following code:
```c
#include <cxa_yourArchitecture_gpio.h>
#include <cxa_yourArchitecture_usart.h>
#include <cxa_ioStream.h>

void main(void)
{
   // initialize a debug LED
   cxa_yourArchitecture_gpio_t testGpio;
   // the following function prototype may be different depending on your modifications from above
   cxa_yourArchitecture_gpio_init_output(&testGpio, CXA_GPIO_POLARITY_NONINVERTED, 0);

   // initialize our USART
   cxa_yourArchitecture_usart_t debugUsart;
   // the following function prototype may be different depending on your modifications from above
   cxa_yourArchitecture_usart_init_noHH(&debugUsart, 115200);

   // get a pointer to our USART's ioStream and make sure it's valid
   cxa_ioStream_t* ios = cxa_usart_getIoStream(&debugUsart.super);
   cxa_assert(ios);

   // print `hello world`
   cxa_ioStream_writeLine(ios, "hello world");

   // turn on our LED so we know that we've made it here
   cxa_gpio_setValue(&testGpio.super, 1);

   // now read bytes from the serial port and echo them back
   // our LED should toggle every time we receive a byte
   uint8_t rxByte;
   while( 1 )
   {
      if( cxa_ioStream_readByte(ios, &rxByte) == CXA_IOSTREAM_READSTAT_GOTDATA )
      {
         cxa_ioStream_writeByte(ios, rxByte);
         cxa_gpio_toggle(&testGpio.super);
      }
   }
}
```

If everything is working, congratulations! You now have the groundwork for a pretty robust embedded system. I highly suggest hooking up asserts, logging, and a command console to your debug USART as shown in [Setting up assertions, logging, and console](#setting-up-assertions,-logging,-and-console)


## FAQs
##### What does the `super` field in the various structs / classes do?
The `super` field is how we achieve [inheritance](https://en.wikipedia.org/wiki/Inheritance_(object-oriented_programming)) in C (since C isn't object oriented). In any given "subclass" (eg. `cxa_esp32_usart_t` a "subclass" of `cxa_usart_t`), the very first field in the "subclass" struct **MUST** be the "superclass" and **SHOULD** be named `super`. This allows the "subclass" to access the "superclass'" fields. You'll often see "superclass" methods being called on the `super` field of a subclass (eg. `cxa_usart_getIoStream(&debugUsart.super)`)...this allows us to resolve compiler warnings and because `super` is the first field in the "subclass", everything magically works.
