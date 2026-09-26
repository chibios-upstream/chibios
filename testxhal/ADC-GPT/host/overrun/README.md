# ADCv3/ADCv6 overrun-state regression

Run from the repository root:

```sh
make -C testxhal/ADC-GPT/host/overrun
make -C testxhal/ADC-GPT/host/overrun clean
```

This focused test extracts each driver's actual `adc_lld_serve_interrupt()`
function at build time, unchanged. State and error constants come from the
production headers; CMSIS supplies G474/H563 register types and status bits.
Only the driver fields used by the service routine and the HLD error-dispatch
hook are mocked. It does not test DMA teardown, callback context or hardware
W1C behavior. The ADCv5 host suite separately exercises the real ADC frontend's
error cleanup and waiter/callback behavior.

Single and dual builds cover linear/circular overruns, master/slave sources,
a missing slave, missing conversion groups, non-active states, and combination
with analog watchdog errors. Both drivers must report circular overruns but
continue ignoring overruns after completion or without a conversion group.
