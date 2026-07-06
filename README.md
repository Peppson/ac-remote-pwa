
# AC Remote Pwa [![PlatformIO](https://img.shields.io/badge/PlatformIO-FF6600?logo=platformio&logoColor=white)](https://platformio.org/)

A ESP32 project to control my Whirlpool AC over IR from a web interface.  
Keeps all clients (usually just my phone and desktop) in sync over Websockets.   

> **Note:** This is a personal hobby project.  


## PWA / App
<div align="center">
  <video
    src="https://github.com/user-attachments/assets/26912659-2e06-457c-9699-3ccbce801512"
    width="800"
    autoplay
    loop
    muted
    playsinline>
  </video>
</div>


## Hardware
Non-invasive install, everything is mounted externally. Let's call it *"good enough"*...   
IR diode is in front of the AC, while the device is to the right.

<div align="center">
  <img src="images/hardware/ACOpen.jpg" width="600" alt="hardware">
</div>


## Features
- IR transmit support for the Whirlpool AC protocol (reverse engineered).
- Time-of-Flight sensor pointed upwards to detect power state and oscillation lid movements.
- Embedded, minified web UI for faster load times (instead of serving raw assets from flash).
- OTA firmware updates, triggered from a physical button on the device.
- Websocket sync for real-time control updates across all client.


## Notes
- IR timing and protocol details are tuned for this specific AC unit.
- Includes a small C# checksum cracker in [CheckSumCracker/](CheckSumCracker/) used during reverse engineering.


<br>

<details open>
    <summary>PWA Images</summary>

<br>

<p>Cool</p>
<div align="center">
  <img src="images/app/cool.png" width="300">
</div>

<p>Dry</p>
<div align="center">
  <img src="images/app/dry.png" width="300">
</div>

<p>Fan</p>
<div align="center">
  <img src="images/app/fan.png" width="300">
</div>

<p>Auto</p>
<div align="center">
  <img src="images/app/auto.png" width="300">
</div>

<p>Off</p>
<div align="center">
  <img src="images/app/off.png" width="300">
</div>

</details>


--- 

<details open>
    <summary>Hardware Images</summary>

<br>

<p>ESP32</p>
<div align="center">
  <img src="images/hardware/ESP32.jpg" width="500">
</div>

<p>AC Closed</p>
<div align="center">
  <img src="images/hardware/ACClosed.jpg" width="500">
</div>

<p>AC Open</p>
<div align="center">
  <img src="images/hardware/ACOpen.jpg" width="500">
</div>

</details>