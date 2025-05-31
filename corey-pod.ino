// This #include statement was automatically added by the Particle IDE.
#include <elapsedMillis.h>

// This #include statement was automatically added by the Particle IDE.
#include <PublishQueueAsyncRK.h>

// This file is copied from the Particle IDE. You need to add
// the following libraries to your app:
// * elapsedMillis
// * PublishQueueAsyncRK
//


#include <queue>

SYSTEM_THREAD(ENABLED); // This prevents particle cloud connection management from blocking the setup and loop functions

int RECORD_PERIOD=2000; // every n milliseconds, record sensor data
int EST_MSG_SEND_TIME = 1500; // in ms. If there isn't at least this much time left before the next sensor measurement, don't try to send another message
int MAX_DATA_PER_PACKET = 10; // the maximum number of data points to send per particle publish
int SEND_PERIOD = 10000; // every n milliseconds, try to send a message

std::queue<float> q_t; // Temperature Sensor
std::queue<int> q_time; // Timestamp
std::queue<float> q_p; // pH sensor
std::queue<float> q_do; // Dissolved Oxygen Sensor

int lastSendMsgId = 0;
int currMsgId = 0;

retained uint8_t publishQueueRetainedBuffer[3000];
PublishQueueAsync publishQueue(publishQueueRetainedBuffer, sizeof(publishQueueRetainedBuffer));

char tempMsg[255];
int msgIndex = 0;

float ph = -1;
float dissolvedOxygen = -1;

bool sendData(int loopTime){
    elapsedMillis func_timer = loopTime;
    // Particle.publish("left condition", String(func_timer + EST_MSG_SEND_TIME));

    while ((func_timer + EST_MSG_SEND_TIME) < RECORD_PERIOD && Particle.connected() && q_time.size() > 0){
        RGB.control(true);

        RGB.color(0, 0, 255);
        String msg = "{\"robotID\":0,\"tmp\":[";


        int numReadings = min(q_time.size(), MAX_DATA_PER_PACKET);

        for (int i=0; i<numReadings; ++i){
            msg += String(q_t.front(), 2);
            if (i == numReadings - 1){
                msg += "],";
            } else {
                msg += ",";
            }

            q_t.pop();
        }

        msg += "\"ph\":[";

        for (int i=0; i<numReadings; ++i){
            msg += String(q_p.front(), 2);
            if (i == numReadings - 1){
                msg += "],";
            } else {
                msg += ",";
            }

            q_p.pop();
        }
        
        msg += "\"do\":[";

        for (int i=0; i<numReadings; ++i){
            msg += String(q_do.front(), 2);
            if (i == numReadings - 1){
                msg += "],";
            } else {
                msg += ",";
            }

            q_do.pop();
        }

        msg += "\"timestamp\":[";

        for (int i=0; i<numReadings; ++i){
            msg += String(q_time.front());
            if (i == numReadings - 1){
                msg += "]}";
            } else {
                msg += ",";
            }

            q_time.pop();
        }

        publishQueue.publish("waterdata", msg, WITH_ACK);
        delay(1000);
        RGB.control(false);

    }
    if (q_time.size() == 0){
        return false; // there are no messages left to send, go ahead and sleep until next data point should be recorded
    }
    else{
        // publishQueue.publish("queuesize", String(q_time.size()), WITH_ACK);
        return true; // there are still messages that need to be sent
    }
}


void readSensors(){
    // Read temp sensor
    Serial1.print("R\r");
    bool endOfMsg = false;
    for (int i=0; i < msgIndex; ++i){ // reset message buffer before we read data
        tempMsg[i] = 0;
    }
    msgIndex = 0;

    while (true){
        int c = Serial1.read(); // read one char a ta time
        if (c < 0) break; // no chars to read in the buffer
        if (c == 42) break; // * character
        if (msgIndex == 254) break; // break before we overfill the buffer

        tempMsg[msgIndex] = c;
        msgIndex++;

    }
    while (Serial1.read() > 0){} // empty the rest of the buffer

    q_t.push(String(tempMsg).trim().toFloat());

    // Read PH sensor
    // https://files.atlas-scientific.com/Surveyor-pH-datasheet.pdf

    float volts = (analogRead(A1)/4095.0) * 3.3;
    ph = (-5.6548 * volts) + 15.509;

    q_p.push(ph);
    
    float doVolts = (analogRead(A2)/4095.0) * 3.3;
    dissolvedOxygen = doVolts * 3.0;
    q_do.push(dissolvedOxygen);


    q_time.push(millis()/1000.0);

}


void setup() {
    Serial1.begin(9600);
    Serial1.print("c,0\r");
    while (Serial1.available()) Serial1.read(); // clear the buffer
    Particle.keepAlive(5); // send a keepalive every 5 seconds

}

elapsedMillis sensorReadTime;
elapsedMillis sendDataTime;

void loop() {


    if (sensorReadTime > RECORD_PERIOD){
        readSensors();
        sensorReadTime = 0;
    }

    if (sendDataTime > SEND_PERIOD){
        if (!sendData(sensorReadTime)) sendDataTime = 0;
    }


}