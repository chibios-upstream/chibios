# RP2350 RTC-VALIDATION

This test application validates the RP2350 RTC alarm contract, implemented
by the RTCv2 LLD on top of the POWMAN Always-On timer (the RP2350 has no
dedicated RTC peripheral).

## Purpose

- A programmed alarm fires and invokes the registered callback.
- `rtcGetAlarm()` reflects the armed alarm.
- `rtcSetAlarm(..., NULL)` disables the alarm instead of faulting.
- An alarm specification with an all-zero mask disables the alarm.
- Both disable forms update the saved state read by `rtcGetAlarm()`.
- A disabled alarm no longer fires.

## Build

From repository root:

```sh
cd testhal/RP/RP2350/RTC-VALIDATION
make clean
make -j$(nproc)
```

Produced artifacts are in the local `build/` directory (`ch.elf`, `ch.hex`, `ch.bin`).

## Configuration Notes

- HAL RTC and HAL SIO are enabled in [cfg/halconf.h](cfg/halconf.h).
- The RTC interrupt priority is set in [cfg/mcuconf.h](cfg/mcuconf.h).

## Runtime Notes

- Results are printed as `PASS`/`FAIL` lines on SIO0 (GPIO0 TX, GPIO1 RX)
  at the default bit rate, followed by a summary.
- The board LED blinks slowly when every check passed and rapidly otherwise.
