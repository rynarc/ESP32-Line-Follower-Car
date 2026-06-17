# Tuning Guide

Notes on how to pick the right variant and tune the PD controller for this
line follower. Written so it's usable even by someone without a coding
background.

## Step 1 — Check your motors

Run a plain "go forward" test first to see whether motor 1 or motor 2 spins
weaker than the other. Small manufacturing/wiring differences mean the two
motors are rarely perfectly matched.

## Step 2 — Pick the right file

| Result of Step 1 | Use |
|---|---|
| Both motors equal | `line_follower_pd_base.ino` |
| Motor 1 weaker | `line_follower_pd_m1_weaker.ino` |
| Motor 2 weaker | `line_follower_pd_m2_weaker.ino` |

> Back up your current working code before editing — these versions are
> already tuned and stable.

## Step 3 — Understand the PD logic

See [`docs/images/pid-manual-calculation.jpg`](../docs/images/pid-manual-calculation.jpg)
for a worked-through example of how the weighted sensor error and the PD
correction term are actually calculated by hand.

## Step 4 — Parameters you can edit

| Parameter | Meaning |
|---|---|
| `base_speed` | The motor's base/cruising speed |
| `Kp` | Proportional gain — how hard it corrects based on the current error |
| `Kd` | Derivative gain — dampens the correction based on how fast the error is changing, so the robot doesn't overshoot/oscillate |
| `motor1_trim` / `motor2_trim` | Compensation factor for whichever motor is weaker. Adjust only the decimals — e.g. `1.04` for a ~16-unit speed difference, `1.00` means no difference to correct |

## Why PD and not full PID?

This only needs **P**roportional + **D**erivative control, no **I**ntegral
term. With this sensor setup, an integral term tends to accumulate
(over-correct) and gets the robot stuck oscillating instead of settling.
The motor trim effectively does the job an integral term would otherwise do
(correcting a persistent, constant offset), so a full PID isn't necessary
here.
