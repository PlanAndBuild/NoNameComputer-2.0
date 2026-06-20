<div align="center">
  <img src="Images/NoNameRocket.PNG" alt="Render projektu" width="80%">
</div>

---

<h1 align="center">NNCpro</h1>

<h3 align="center">AKA NoNameComputer 2.0</h3>



<p align="center">
 <small><I>Built for sky.<I></small>
</p>

<p align="center">
  <b>NNCpro is a fligth computer designed mainly for rockets but it also can be used for other vehicles.</b>
  <br>
  <b>It is designed to collect data from all of the sensor, sending it IRL back down to the earth via radio link, deploy parachutes, steer rocket in flight using aerodynamic control or TVC and at the end send live location. </b>
  </br>
  <em>I have designed it for my rockets because there is no comercial computers or if there is something it's very expensive. I actually like designig electronics and thats why I'm even here.</em>
</p>

## Features

**8x Servo outputs** ***(PWM channels)***

**6x Pyrotechniacal channels** ***(used for firing parachute charges, motor ignitions etc.)***

**Long-Range radio** ***(based on two LoRa modules for simultaneous transmiting and reciveing)***

**Current and Voltage sensing** ***(based on INA3221 three channel current sensor)***

**Audio signaling** ***(using passive buzzer with possibility of modulation sound wave)***

**Flash memory for storing software and flight data in flight**

**micro-SD card slot for saving fligth data after landing**

**3,3v and 5v power supply**

**ARGB LEDs for signaling computer states**

---
## Sensors
| Sensor Type | Primary Component |
| :--- | :--- |
| **Barometer main** | `MS5611` (High precision) |
| **Barometer secondary** |`BMP390` (Redundant) |
| **Inertail Mesurment Unit main**| `LSM6DSV80XTR` (6-DoF Main) |
| **Inertail Mesurment Unit secondary** |`LSM6DSO32XTR` (6-DoF Secondary) |
| **High-G Acceleration** | `H3LIS200DLTR` (Up to $\pm200\text{g}$) |
| **Magnetometer** | `MMC5983MA` (3-axis Compass) |
| **GNSS (GPS)** | `u-blox MAX-M10M-20b` |
---
## Images

<table align="center" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td>
      <img src="Images/Zine.png" alt="NNCpro Zine" height="450px">
    </td>
    <td>
      <img src="Images/PCBrenderTransparent2.png" alt="NNCpro Render" height="450px">
    </td>
  </tr>
</table>

## ⚖️ License

This project is licensed under the **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)** License.

[![CC BY-NC-SA 4.0][cc-by-nc-sa-shield]][cc-by-nc-sa]

You are free to share and adapt this material under the following terms:
* **Attribution** — You must give appropriate credit to the original author.
* **NonCommercial** — You may not use the material for commercial purposes.
* **ShareAlike** — If you remix, transform, or build upon the material, you must distribute your contributions under the same license.

[cc-by-nc-sa]: http://creativecommons.org/licenses/by-nc-sa/4.0/
[cc-by-nc-sa-shield]: https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg
