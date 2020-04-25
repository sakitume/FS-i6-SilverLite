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

* GEM (Good enough menu) system. See: <https://github.com/Spirik/GEM>
* JeeH by Jean-Claude Wippler. See: <https://platformio.org/lib/show/3082/JeeH and https://git.jeelabs.org/jcw/jeeh>
* DIY Multiprotocol TX Module. See: <https://github.com/pascallanger/DIY-Multiprotocol-TX-Module>
* nRF24L01 multi-protocol RC transmitter. See: <https://github.com/goebish/nrf24_multipro>

## My experiences with these open source projects

I entered the hobby of whoops in the summer of 2019 when I purchased a Furibee F36 on Amazon for $20. It was super awesome! However controlling
it with the toy transmitter was awkward. Adding stick extensions helped but I then discovered you could use hobby grade transmitters, in
conjunction with an adapter module to control these toys. However the Furibee isn't supported by these adapter modules. That's when I
discovered the Eachine E010 and E011 and then I also learned about the BWhoop B03 Pro. These quads can be flashed with [Silverware](https://github.com/silver13/BoldClash-BWHOOP-B-03) as well as [NFE Silverware](https://github.com/NotFastEnuf/NFE_Silverware). And more importantly they use a Bayang protocol
that these protocol adapters support.

I purchased a FlySky FS-i6 because it is the cheapest decent transmitter you can buy. It's actually pretty darn good for the money. About $50 on Amazon or
if you don't mind waiting a little it is as low as $28 dollars on xt-xinte. I ended buying one from Bangood along with an [iRangeX IRX6 multiprotocol module](https://www.banggood.com/IRangeX-iRX6-Multiprotocol-TX-Module-for-Flysky-FS-i6-i6x-Transmitter-p-1161199.html).

This is where Goebish's amazing NRF24L01 module project comes in! I learned that the IRX6 is using his module firmware. This was awesome!
This meant I had a chance at modifying/hacking this IRX6 module. But I soon found that it would be a little more difficult since the tiny module
doesn't have any breakout pads for programming the ATMEGA328 processor.

No worries, I had Goebish's code to work with so I hacked an old VGA adapter cable onto a spare NodeMCU board and was able to adapt Goebish's code to work with the ESP8266.
It worked great! And I learned a whole lot. Again, **this was due to amazing people sharing** their work. Thanks, Goebish!

But one of the features I wanted was to have access to more of the switches on the i6, but the stock firmware only outputs 6 channels. Again, open source software community to the rescue! I discovered there was a custom firmware replacement that could give you 8 channels out of the PPM signal pin on the trainer port. It worked great!

After looking at [that project](https://github.com/qba667/FlySkyI6>) in more detail I began to wonder if I could forego the external microprocessor and just
install the NRF24L01 directly into the i6. This would of course require custom firmeware and enough GPIO pins I could hijack/repurpose for the software SPI
implementation (to interface with the NRF24L01).

After some more Googling I discovered Nathan Tsoi's [project documentation](https://nathan.vertile.com/blog/2016/07/09/flysky-i6-radio-setup-and-hacking-guide/) where he attempted to do something similar. I forked his [github project](https://github.com/nathantsoi/fs-i6-test) and started hacking away.

I cannot say it often enough, "Thank you" to all who open source their projects and share. I hope by releasing [SilverLite FS-i6 firmware](https://github.com/sakitume/SilverLite-FS-i6) and [SilverLite FC firmware](https://github.com/sakitume/SilverLite-FC) I can do my small part at giving back.