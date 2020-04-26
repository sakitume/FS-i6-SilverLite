# TODO: Placeholder

This is where I'll describe channel configuration. There are two types/modes to consider.

* Silverware/Bayang
* Multiprotocol

## Silveware/Bayang

Silverware firmware with Bayang protocol defines 16 auxliary channels. 
These channels are defined in the `defines.h` header file of Silverware.

```
#define AUXNUMBER 16

// defines for bayang protocol radio
#define CH_ON (AUXNUMBER - 2)
#define CH_OFF (AUXNUMBER - 1)
#define CH_FLIP 0
#define CH_EXPERT 1
#define CH_HEADFREE 2
#define CH_RTH 3
#define CH_AUX1 4
#define CH_AUX2 5
#define CH_EMG 10
#define CH_TO 11
#define CH_ANA_AUX1 12
#define CH_ANA_AUX2 13
// trims numbers have to be sequential, start at CH_PIT_TRIM
#define CH_PIT_TRIM 6
#define CH_RLL_TRIM 7
#define CH_THR_TRIM 8
#define CH_YAW_TRIM 9
// next 3 channels only when *not* using USE_STOCK_TX
#define CH_INV 6
#define CH_VID 7
#define CH_PIC 8
```

Two of these (`CH_ANA_AUX1` and `CH_ANA_AUX2`) can be treated as analog values (in the range of 0..255) 
but the rest are considered to be on/off (either 0 or 255 and nothing in between).

> Note: The 2 analog channels are only available if you build Silverware with `USE_ANALOG_AUX` defined.

Examining NFS Silverware rx code verified that these channels will be available for use:

* `CH_INV`
* `CH_VID`
* `CH_PIC`
* `CH_TO`
* `CH_EMG`
* `CH_FLIP`
* `CH_HEADFREE`
* `CH_RTH`

And if `USE_ANALOG_AUX` was defined when Silverware was built, these analog channels (range of 0..255) become availble:

* `CH_ANA_AUX1`
* `CH_ANA_AUX2`

## Hardcoded!

Currently the mapping of FlySky FS-i6 switches to Silverware aux channels (`CH_INV`, `CH_VID`, etc) is hardcoded
in the file `Bayang/bayang_tx_main.cpp` inside function `Bayang_tx_update()`. In addition to the mapping of
i6 switch to aux channel, the logic of the switch (position 1 yields high or low) is also hardcoded.

**The mapping of i6 channels to Silveware channels, as well as the logic is going to depend on how you built Silverware**

**I will be refactoring this so that it can be user customized via a new menu screen**

In the meantime feel free to edit that function to meet your needs.
