# Custom firmware for the FlySky FS-i6

[SilverLite FS-i6 firmware](https://github.com/sakitume/SilverLite-FS-i6) is a firmware
**replacement** for the **FlySky-i6** transmitter (and not the FlySky FS-i6S or the FlySky FS-i6X).
This custom firmware was designed for my particular use case: controlling 65mm/75mm whoops and micro (100mm) sized quadcopters.
The features that are available along with the user interface are therefore rather limited. It does just
what I need and not much more.

# Silverware
This firmware is primarily designed to interface with flight controllers running [Silverware](http://sirdomsen.diskstation.me/dokuwiki/doku.php) or variations of that
software: [BWhoop Silverware](https://github.com/silver13/BoldClash-BWHOOP-B-03), [SilF4ware](https://www.rcgroups.com/forums/showthread.php?3294959-SilF4ware-an-STM32F4-port-of-SilverWare), [NFE Silverware](https://github.com/NotFastEnuf/NFE_Silverware), NFE Silverware with SilverLite extensions (a fork I made but haven't released), and [SilverLite](https://github.com/sakitume/SilverLite-FC). 

All of these Silveware based flight controllers provide support for a Bayang protocol. This is the protocol that
SilverLite FS-i6 is primarily based upon. However this requires a hardware modification to the FlySky-i6 so that an NRF24L01 transceiver
module be installed. The internal A7105 transceiver module is left alone and intact, but the firmware simply ignores it.

# Multiprotocol module
In addition to the Bayang protocol, SilverFlite FS-i6 can also work with a [multiprotocol module](https://sakitume.github.io/SilverLite-FS-i6/Multiprotocol/)
connected via serial (trainer port on back of the i6). This restores the FlySky protocol as well as introduces the huge variety
of additional protocols provided by [Pascal's module](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module).

# Documentation

Documentation for [SilverLite FS-i6 firmware](https://github.com/sakitume/SilverLite-FS-i6) can be found [here](https://sakitume.github.io/SilverLite-FS-i6/)