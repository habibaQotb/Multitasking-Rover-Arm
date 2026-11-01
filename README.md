# Multitasking Rover Arm: Individual Technical Mission (Project 37)

**Name:** Habiba Ahmed Kotb Metwally
**Sub-team:** Electrical (Powertrain, Embedded Systems)
**Platform:** Arduino Uno, simulated in Wokwi
**Wokwi project link:** [link](https://wokwi.com/projects/479462751148438529)

## What it does

One microcontroller runs three jobs at the same time, with no blocking code:

1. **Operator control:** reads a potentiometer on an analog pin and maps it to a servo target angle (0° to 180°).
2. **Smooth servo motion:** moves the servo toward the target at **1° every 15 ms**, so it never jumps, even if the pot is turned quickly.
3. **System health:** blinks the onboard LED at **2 Hz** (250 ms on, 250 ms off), steady regardless of what the pot or servo is doing.

`delay()`, `HAL_Delay()`, `vTaskDelay()` and spin-wait loops are not used anywhere. Every task is timed with `millis()` timestamp comparisons.

## Hardware and wiring

| Part | Connection |
|---|---|
| Potentiometer | GND to GND, VCC to 5V, SIG to **A0** |
| Servo | GND to GND, V+ to 5V, PWM to **pin 9** |
| Status LED | onboard LED on **pin 13** (`LED_BUILTIN`), no extra wiring |

In simulation the servo runs from the board's 5V. On real hardware it should have its own supply, because its current spikes can brown out the microcontroller.

## How it works

The `loop()` runs fast and each task asks "is it my turn yet?" using its own timestamp:

| Task | Interval | Purpose |
|---|---|---|
| Pot sampling and filtering | 10 ms | gives the filter a fixed sample rate |
| Servo step | 15 ms | 1° per step, about 66.7°/s, so a full 0° to 180° sweep takes about 2.7 s |
| LED toggle | 250 ms | 2 Hz heartbeat, 50% duty |

Key design choices:

- **Target vs. current angle.** The pot only updates `targetAngle`. A timer moves `currentAngle` one degree toward it. This is a slew-rate limit: it protects the gears and limits current spikes, and it keeps motion predictable.
- **Timestamps, not delays.** Blocking would stop the other tasks. With timestamps, each task keeps its own schedule.
- **`now - last >= interval` with unsigned types.** This stays correct when `millis()` overflows (after about 49.7 days).
- **`last += interval` instead of `last = now`.** This stops timing drift, which keeps the LED at an exact 2 Hz on average.
- **`constrain()` on the angle.** The target can never leave 0° to 180°.

## Bonus: EMA filter

The pot reading is smoothed with an exponential moving average:

`filtered = ALPHA * raw + (1 - ALPHA) * filtered`, with `ALPHA = 0.2`.

The pot is sampled on its own 10 ms timer, so 0.2 smooths over roughly 50 ms. Sampling on every pass of `loop()` would make the filter act over less than a millisecond and do almost nothing. The filter is seeded with a real reading at startup so it does not begin at zero. The cost of the filter is a small lag, and a larger `ALPHA` is faster but noisier.

The Wokwi pot is perfectly clean, so jitter does not appear by default. To see the filter working, add fake noise (`raw += random(-8, 9);`) and compare the servo with and without the filter line.

## Bonus status

| Bonus | Status |
|---|---|
| EMA filter on the ADC reading | done |
| Circuit simulation (Wokwi) | done |
| STM32 implementation | not done |

## How to run the simulation

1. Open the Wokwi link above, or create a new Arduino Uno project and copy in `simulation/diagram.json` and `simulation/sketch.ino`.
2. Press the green Start button. If the build queue is busy, wait a minute or two.
3. Leave the pot still: the LED blinks steadily and the servo holds.
4. Turn the pot slowly: the servo follows.
5. Spin the pot quickly back and forth: the servo still moves at a steady rate and does not jump, and the LED does not stutter.

## Known limitations

- **No position feedback.** The Arduino only knows the angle it last commanded, not where the servo horn really is. At startup the first command may move the horn quickly to the starting angle. On a real rover, the arm should be parked at a known home position before shutdown.
- **Simulator behaviour at startup.** Wokwi's simulated servo appeared to keep its position between runs, so it does not show real startup behaviour. I decided not to rely on it for that.
- **Cooperative timing.** All tasks share one loop, so any one task taking too long would delay the others. Each block here is short. A larger system would need interrupts or an RTOS.

## Files

```
multitasking_robot/
  src/arduino/sketch.ino          final commented source code
  simulation/libraries.txt        arduino libraries wokwi needs to build the sketch (Servo)
  simulation/diagram.json         Wokwi circuit
  simulation/sketch.ino           identical copy of the source code
  demo/demo_video.mp4             screen recording of the simulation
  README.md                       this file
```
