// This #include statement was automatically added by the Particle IDE.
#include <elapsedMillis.h>

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_GPS.h>

// This #include statement was automatically added by the Particle IDE.
#include <SdFat.h>

// This #include statement was automatically added by the Particle IDE.
#include <PublishQueueAsyncRK.h>

#include <queue>

#define DEFAULT_SAT_VOLTAGE_CONST (40.0*11.0)

const int SD_CS_PIN = D5;

#ifndef F_CPU
#define F_CPU 64000000UL  // Particle Boron runs at 64MHz
#endif

SYSTEM_THREAD(ENABLED); // This prevents particle cloud connection management from blocking the setup and loop functions

SdFat sd;
SdFile dataFile;
bool sdCardInitialized = false;

// Hardware serial port for Featherwing GPS
#define GPSSerial Serial1

// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO false

int RECORD_PERIOD = 2000; // every 2 seconds (2000 ms), record sensor data
int EST_MSG_SEND_TIME = 1500; // in ms. If there isn't at least this much time left before the next sensor measurement, don't try to send another message
int MAX_DATA_PER_PACKET = 10; // the maximum number of data points to send per particle publish
int SEND_PERIOD = 10000; // every n milliseconds, try to send a message

std::queue<float> q_t; // Temperature Sensor
std::queue<int> q_time; // Timestamp
std::queue<float> q_p; // pH sensor
std::queue<float> q_do; // Dissolved Oxygen Sensor
std::queue<float> q_turb; // Turbidity Sensor
std::queue<float> q_lat; // GPS Latitude
std::queue<float> q_lon; // GPS Longitude

int lastSendMsgId = 0;
int currMsgId = 0;

retained uint8_t publishQueueRetainedBuffer[3000];
PublishQueueAsync publishQueue(publishQueueRetainedBuffer, sizeof(publishQueueRetainedBuffer));

float ph = -1;
float temperature = -1;
float dissolvedOxygen = -1;
float turbidity = -1;
float turbidity_ntu = -1;
float Vclear = 2.85; // Calibration constant for turbidity sensor
float latitude = 0.0;  // Default GPS value
float longitude = 0.0; // Default GPS value
float ph_calibration = 15.509 + 9.0; // Calibration constant for pH sensor.

// Update the initializeSDCard function with better error reporting
bool initializeSDCard() {
    Particle.publish("SD", "Initializing SD card...");
    
    // Try using the native SPI speed first
    if (!sd.begin(SD_CS_PIN, SPI_FULL_SPEED)) {
        // Try half speed
        if (!sd.begin(SD_CS_PIN, SPI_HALF_SPEED)) {
            // Try quarter speed as last resort
            if (!sd.begin(SD_CS_PIN, SPI_QUARTER_SPEED)) {
                // Get specific error code
                uint8_t errorCode = sd.cardErrorCode();
                String errorMsg = "SD init failed! Error code: " + String(errorCode);
                Particle.publish("SD", errorMsg);
                return false;
            }
        }
    }
    
    Particle.publish("SD", "SD card initialized successfully.");
    
    // Create or open the data log file and write header as per Excel file format
    if (!dataFile.open("SENSOR_LOG.CSV", O_RDWR | O_CREAT | O_AT_END)) {
        Particle.publish("SD", "Error opening log file!");
        return false;
    }
    
    // Write header if file is new (size 0) - using format from Excel file
    if (dataFile.fileSize() == 0) {
        dataFile.println("Date,Time,Latitude,Longitude,Temperature,pH,DissolvedOxygen,Turbidity");
    }
    dataFile.sync();
    dataFile.close();
    return true;
}

// Function to get formatted date string from GPS
String getFormattedDate() {
    if (GPS.fix) {
        char dateStr[11];
        // Format: YYYY-MM-DD
        sprintf(dateStr, "20%02d-%02d-%02d", GPS.year, GPS.month, GPS.day);
        return String(dateStr);
    } else {
        // Fallback if no GPS fix
        return "0000-00-00";
    }
}

// Function to get formatted time string from GPS
String getFormattedTime() {
    if (GPS.fix) {
        char timeStr[9];
        // Format: HH:MM:SS
        sprintf(timeStr, "%02d:%02d:%02d", GPS.hour, GPS.minute, GPS.seconds);
        return String(timeStr);
    } else {
        // Fallback using device time if no GPS fix
        unsigned long totalSeconds = millis() / 1000;
        int hours = (totalSeconds % 86400) / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int seconds = totalSeconds % 60;
        
        char timeStr[9];
        sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);
        return String(timeStr);
    }
}

// Updated log function to match Excel format
void logDataToSD(float temp, float ph, float dissolvedOxygen, float turbidity, float lat, float lon) {
    if (!sdCardInitialized) return;
    
    if (!dataFile.open("SENSOR_LOG.CSV", O_RDWR | O_CREAT | O_AT_END)) {
        Particle.publish("SD", "Error opening log file for writing!");
        return;
    }
    
    // Format: Date, Time, Latitude, Longitude, Temperature, pH, DO, Turbidity
    dataFile.print(getFormattedDate());
    dataFile.print(",");
    dataFile.print(getFormattedTime());
    dataFile.print(",");
    dataFile.print(lat, 6); // Latitude with 6 decimal places
    dataFile.print(",");
    dataFile.print(lon, 6); // Longitude with 6 decimal places
    dataFile.print(",");
    dataFile.print(temp, 2);
    dataFile.print(",");
    dataFile.print(ph, 2);
    dataFile.print(",");
    dataFile.print(dissolvedOxygen, 2);
    dataFile.print(",");
    dataFile.println(turbidity, 2);
    
    // Make sure data is written to the file
    dataFile.sync();
    dataFile.close();
}

// This function creates the next line written to the csv file. 
bool sendData(int loopTime) {
    elapsedMillis func_timer = loopTime;

    while ((func_timer + EST_MSG_SEND_TIME) < RECORD_PERIOD && Particle.connected() && q_time.size() > 0) {
        RGB.control(true);
        RGB.color(0, 0, 255);
        
        String msg = "{\"robotID\":0,\"tmp\":[";

        int numReadings = min(q_time.size(), MAX_DATA_PER_PACKET);

        for (int i=0; i<numReadings; ++i) {
            msg += String(q_t.front(), 2);
            if (i == numReadings - 1) {
                msg += "],";
            } else {
                msg += ",";
            }
            q_t.pop();
        }

        msg += "\"ph\":[";

        for (int i=0; i<numReadings; ++i) {
            msg += String(q_p.front(), 2);
            if (i == numReadings - 1) {
                msg += "],";
            } else {
                msg += ",";
            }
            q_p.pop();
        }
        
        msg += "\"do\":[";

        for (int i=0; i<numReadings; ++i) {
            msg += String(q_do.front(), 2);
            if (i == numReadings - 1) {
                msg += "],";
            } else {
                msg += ",";
            }
            q_do.pop();
        }
        
        msg += "\"turb\":[";

        for (int i=0; i<numReadings; ++i) {
            msg += String(q_turb.front(), 2);
            if (i == numReadings - 1) {
                msg += "],";
            } else {
                msg += ",";
            }
            q_turb.pop();
        }
        
        msg += "\"lat\":[";

        for (int i=0; i<numReadings; ++i) {
            msg += String(q_lat.front(), 6);
            if (i == numReadings - 1) {
                msg += "],";
            } else {
                msg += ",";
            }
            q_lat.pop();
        }
        
        msg += "\"lon\":[";

        for (int i=0; i<numReadings; ++i) {
            msg += String(q_lon.front(), 6);
            if (i == numReadings - 1) {
                msg += "],";
            } else {
                msg += ",";
            }
            q_lon.pop();
        }

        msg += "\"timestamp\":[";

        for (int i=0; i<numReadings; ++i) {
            msg += String(q_time.front());
            if (i == numReadings - 1) {
                msg += "]}";
            } else {
                msg += ",";
            }
            q_time.pop();
        }

        publishQueue.publish("waterdata", msg, WITH_ACK); // This line publishes the message to the web console
        delay(1000);
        RGB.control(false);
    }
    
    if (q_time.size() == 0) {
        return false; // there are no messages left to send, go ahead and sleep until next data point should be recorded
    } else {
        return true; // there are still messages that need to be sent
    }
}

// Read data from all sensors
void readSensors() {
    // Read pH sensor (now on A0)
    float phVolts = (analogRead(A0)/4095.0) * 3.3;
    ph = (-5.6548 * phVolts) + ph_calibration; // add calibration constant
    q_p.push(ph);
    
    // Read temperature sensor (now analog on A1)
    float tempVolts = (analogRead(A1)/4095.0) * 3.3;
    
    temperature = tempVolts * 100.0 - 107; // add or subtract calibration constant once values have settled
    q_t.push(temperature);
    
    // Read dissolved oxygen sensor on A2
    float doVolts = analogRead(A2);// / 4095.0) * 3300.0 + 130;
    
    Particle.publish("do volts", doVolts);
    
    dissolvedOxygen = doVolts* 100.0 / DEFAULT_SAT_VOLTAGE_CONST;
    //Particle.publish("%d",dissolvedOxygen);

    // Temperature compensation (DO decreases with increasing temperature)
    float tempFactor = 1.0 - (0.02 * (temperature - 20.0));
    
    
    dissolvedOxygen = dissolvedOxygen * tempFactor;
    
    // Ensure reasonable bounds (typical range 0-20 mg/L)
    if (dissolvedOxygen < 0) dissolvedOxygen = 0;
    //if (dissolvedOxygen > 25) dissolvedOxygen = 25;
    
    q_do.push(dissolvedOxygen);
    
    // Read Turbidity sensor on A3
    float actualVolts = 1.0 + (analogRead(A3)) * (3.3 / 4095.0);  // Actual voltage from sensor

    // Scale to 5V equivalent for DFRobot formula
    float scaledVolts = actualVolts * (5.0 / 3.3);
    
    // DFRobot quadratic formula for NTU conversion
    float turbidity_ntu = -1120.4 * scaledVolts * scaledVolts + 5742.3 * scaledVolts - 4352.9;
    
    // Handle edge cases
    if (turbidity_ntu < 0) turbidity_ntu = 0;
    if (turbidity_ntu > 3000) turbidity_ntu = 3000;
    
    // Publish both values for monitoring
    String turbidityData = String::format("{\"voltage\":%.2f,\"ntu\":%.1f}", actualVolts, turbidity_ntu);
    //Particle.publish("turbidity", turbidityData);

    q_turb.push(turbidity_ntu);
    
    // Read GPS data if available
    if (GPS.newNMEAreceived()) {
        // Try to parse the newest NMEA sentence
        if (GPS.parse(GPS.lastNMEA())) {
            // Check if we have a valid fix
            if (GPS.fix) {
                latitude = GPS.latitudeDegrees;
                longitude = GPS.longitudeDegrees;
            }
            // If no fix, maintain last known position or use zeros
        }
    }
    q_lat.push(latitude);
    q_lon.push(longitude);
    
    // Record timestamp
    int timestamp = millis()/1000;
    q_time.push(timestamp);
    
    // Log data to SD card
    if (sdCardInitialized) {
        logDataToSD(temperature, ph, dissolvedOxygen, turbidity, latitude, longitude);
    }
}


void setup() {
    // Initialize the GPS module
    GPS.begin(9600);  // 9600 NMEA is the default baud rate for most GPS modules
    
    // turn on RMC (recommended minimum) and GGA (fix data) including altitude
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    
    // Set the update rate to 1Hz (1 second)
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
    
    // Request updates on antenna status
    GPS.sendCommand(PGCMD_ANTENNA);
    
    delay(1000);  // Give GPS some time to initialize
    
    Particle.keepAlive(5); // send a keepalive every 5 seconds
    
    // Initialize SPI in master mode with appropriate settings for SD card
    SPI.begin();
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
    
    // Set CS pin as output and initialize high (deselected)
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    delay(100); // Give SD card time to stabilize
    
    // Initialize SD card
    sdCardInitialized = initializeSDCard();
    
    // Visual feedback on SD card initialization status
    if (!sdCardInitialized) {
        Particle.publish("SD", "Failed to initialize SD card");
        for (int i = 0; i < 5; i++) {
            RGB.color(255, 0, 0); // Red
            delay(300);
            RGB.color(0, 0, 0);   // Off
            delay(300);
        }
    } else {
        Particle.publish("SD", "SD card ready");
        RGB.color(0, 255, 0);     // Green for success
        delay(1000);
        RGB.color(0, 0, 0);       // Off
    }
}

elapsedMillis sensorReadTime;
elapsedMillis sendDataTime;

void loop() {
    // Read GPS data - important to call this every loop iteration
    // for the GPS library to catch all incoming data
    char c = GPS.read();
    
    // If debug is on, echo the raw GPS data to Serial
    if (GPSECHO && c) Serial.print(c);
    
    if (sensorReadTime > RECORD_PERIOD) {
        readSensors();
        sensorReadTime = 0;
    }

    if (sendDataTime > SEND_PERIOD) {
        if (!sendData(sensorReadTime)) sendDataTime = 0;
    }
}
