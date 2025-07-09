<div align="center">
  <img src="./videos/jar jar.gif" alt="ROV Animation" width="800"/>
  <p><em>Jar Jar ROV</em></p>
</div>
<br>
<br>

**Build Guides:**
- [Frame Construction](frame-build-guide.md)
- [Sensor Pod Housing](sensor-pod-build-guide.md)
- [Electronics Assembly](electronics-build-guide.md)

<br>


## Quick Start

<br>

The Sensor Pod usees a [Particle Boron Microcontroller](https://store.particle.io/products/boron-lte-cat-m1-noram-with-ethersim-4th-gen?srsltid=AfmBOorGyjtaKMBeFM4IDkFqGIA-umYWDAvHu_w6I5nC4h2ciwpgvu81). To program this device, we use the [Particle Web IDE](https://build.particle.io/build/new). The code in this github is written to run on Boron microcontrolers. Our main use case is to log water wuality data and GPS coordinates to an SD card using the Boron. 

Like most microntrollers, the Boron needs a firmware to run. We have prepared this firmware in ```deploy.ino``` that can be found in the ```/code``` directory. This file contains code written in C++. You will need to download or copy this code into the Web IDE. Type the following commands in your terminal:

<br>

> This will 'clone' or download this repository to your machine. You can then navigate to the codes folder to access the firmware.

<br>

```bash
git clone https://github.com/rishimukherjee31/LCCMR.git
cd codes
ls
```

<br>

1. Flash the `./codes/deploy.ino` file to Particle Boron through the Web IDE
2. Insert SD card into Adalogger
4. Power on and monitor LED status, when the LED on the Boron flashes blue it is connected to the web. (You only need to monitor this once)


## Configuration

```cpp
int RECORD_PERIOD = 2000;        // Sensor reading interval (ms)
int SEND_PERIOD = 10000;         // Data transmission interval (ms)
int MAX_DATA_PER_PACKET = 10;    // Max readings per transmission

// Calibration constants
float ph_calibration = 15.509;
float temp_calibration = -107;
float do_calibration = 20.0;
```

## Data Output

**SD Card (CSV):**
```
Time,Latitude,Longitude,Temperature,pH,DissolvedOxygen,Turbidity
00:30:25,40.123456,-74.654321,22.5,7.2,8.4,12.3
```

**Cloud (JSON):**
```json
{
  "robotID": 0,
  "tmp": [22.5],
  "ph": [7.2],
  "do": [8.4],
  "turb": [12.3],
  "lat": [40.123456],
  "lon": [-74.654321],
  "timestamp": [112]
}
```

## LED Status

- **Blue**: Transmitting data/ Connected
- **Green**: SD card initialized successfully
- **Red**: SD card initialization failed

## Libraries

Auto-included by Particle IDE:
- `Adafruit_GPS`
- `SdFat` 
- `PublishQueueAsyncRK`
- `elapsedMillis`

## Troubleshooting

| Issue | Solution |
|-------|----------|
| SD card error | Format as FAT32, check D5 connection |
| No GPS fix | Wait 2-5 minutes, ensure clear sky view |
| Sensor readings off | Adjust calibration constants |
| Housing leaks | Check PVC cement joints, ROV penetrators |

## License

MIT License
