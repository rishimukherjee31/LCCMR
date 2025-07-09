<div align="center">
  <img src="./videos/jar jar.gif" alt="ROV Animation" width="800"/>
  <p><em>Jar Jar ROV</em></p>
</div>
<br>

**Build Guides:**
- [Frame Construction](frame-build-guide.md)
- [Sensor Pod Housing](sensor-pod-build-guide.md)
- [Electronics Assembly](electronics-build-guide.md)





## Quick Start

```bash
git clone https://github.com/rishimukherjee31/LCCMR.git
cd water-quality-monitor
```

1. Flash the `./codes/deploy.ino` file to Particle Boron through the Web IDE
2. Insert SD card into Adalogger
4. Power on and monitor LED status, when the LED on the Boron flashes blue it is connected to the web. (You only need to monitor this once)

## Hardware

- **Particle Boron LTE** + Adafruit GPS + Adalogger Featherwings
- **Sensors**: pH (A0), Temperature (A1), Dissolved Oxygen (A2), Turbidity (A3)
- **Power**: 3600mAh LiPo + coin cell battery + TP4056 charging circuit + 6A switch


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
