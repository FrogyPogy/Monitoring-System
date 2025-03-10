# Monitoring-System
Arduino Code to receive air pollution monitoring device signal's (DHT22, LCD, Anemometer, ESP-01, pm2.5) and upload to Thingspeak Cloud in real-time.
> Make sure using arduino IDE with new version at least 22.xx
## Installation Guide
  - Clone this code to your computer
  - Don't forget install dependency library from "https://github.com/miguel5612/MQSensorsLib" or you can find it in Arduino Lib manager.
  - I using anemometer with Digital Output Pulse so if using different anemometer make sure you calculate again.
> [!WARNING]
> Avoid using dynamic allocation array or memmory in Arduino if you don't fully understand it. 
