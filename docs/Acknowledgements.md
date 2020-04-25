# Background

The first two commits of this repo were obtained by forking the [F6-i6 Test](https://github.com/nathantsoi/fs-i6-test) repo by Nathan Tsoi.
This provided me a test bed for building a bare bones `.elf` that I could flash onto my FS-i6 transmitter. While it may not do anything 
except blink the backlight on the LCD display screen, it was a huge help providing me the necessary drivers, header files, etc that one
needs for targetting a microcontroller that I was completely unfamiliar with.

Another huge resource was the [FlySkyI6](https://github.com/qba667/FlySkyI6) repo from Jakub (qba667). 

And of course I must also mention this repo: <https://github.com/benb0jangles/FlySky-i6-Mod-> from Ben (Benbojangles)

I also came across some great information shared by [Thom](https://basejunction.wordpress.com/). Some of the early work investigating
the FS-i6 likely stems from here.

A big brother to the FlySky FS-i6 is the FS-i6s as well as the Turnigy Evolution. Both of these use an stm32 chip instead of the Kinetis
found on the FS-i6. For these transmitters you really must take a look at the amazing breadth of information shared by Simon Schulz (fishpepper)
and particularly his [OpenGround project](https://fishpepper.de/projects/openground/).

These are just some of the resources I came across that I found so helpful. I know that there are *many* more contributors (to understanding
the FS-i6) that I am either unaware of, or unable to identify and provide credit to. For example there are hundreds of pages of discussion
on [rcgroups.com](https://www.rcgroups.com/forums/showthread.php?2486545-FlySky-FS-i6-8-channels-firmware-patch%21) regarding custom firmware
patches for the FS-i6. A careful review of all of those posts would probably help identify other great contributors to this effort.

To all of those that I mentioned, as well as those I'm unaware of: "Thank you very much". Your sharing of knowledge adds so much to
our community of builders, hackers and hobbyists!

## Open Source Software

I was able to reference or even use some really helpful open source code to help with my development. Some of it was previously mentioned but I want to
spend some time to list some other sources I found to be useful.

* GEM (Good enough menu) system. See: https://github.com/Spirik/GEM
* JeeH by Jean-Claude Wippler. See: https://platformio.org/lib/show/3082/JeeH and https://git.jeelabs.org/jcw/jeeh
* DIY Multiprotocol TX Module. See: https://github.com/pascallanger/DIY-Multiprotocol-TX-Module
* nRF24L01 multi-protocol RC transmitter. See: https://github.com/goebish/nrf24_multipro
