// LoRa Master [ESP32 Wroom32u]

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char *ssid = "02";
const char *password = "1290zxc1290";
WiFiClient ESP32_ClimateHabibi;

const char *mqtt_server = "test.mosquitto.org";
PubSubClient mqtt(ESP32_ClimateHabibi);

// MQTT Topic
//=========Slave 1 (Wemos)=========
#define slave1_temperature_topic "porogapit/temp1"
#define slave1_pressure_topic "porogapit/pres1"
#define slave1_altitude_topic "porogapit/alti1"
#define slave1_rssi_topic "porogapit/rssi1"
#define slave1_snr_topic "porogapit/snr1"
//=========Slave 2 (ESP32)=========
#define slave2_temperature_topic "porogapit/temp2"
#define slave2_humidity_topic "porogapit/humi2"
#define slave2_soil_moisture_topic "porogapit/soil_mois2"
#define slave2_soil_temperature_topic "porogapit/soil_temp2"
#define slave2_soil_ec_topic "porogapit/soil_ec2"
#define slave2_soil_ph_topic "porogapit/soil_ph2"
#define slave2_rssi_topic "porogapit/rssi2"
#define slave2_snr_topic "porogapit/snr2"

long lastMsg = 0;

#define ss 5   // GPIO 15
#define rst 4  // GPIO 16
#define dio0 2 // GPIO 4

#define i2c_Address 0x3C
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String Incoming = "";
String Message = "";

//======LoRa Destination Address
byte LocalAddress = 0x01;              // Master Address
byte Destination_ESP32_Slave_1 = 0x02; // Slave 1 Address (Wemos D1 Mini Pro)
byte Destination_ESP32_Slave_2 = 0x03; // Slave 2 Address (ESP32 v1)

unsigned long previousMillis_SendMSG = 0;
const long interval_SendMSG = 3000;

unsigned long previousMillis_RestartLORA = 0;
const long interval_RestartLORA = 1000;

//==========Variable For Master==========
float Humd[1], Temp[2], pres, alt;
float soil_moisture, soil_temperature, soil_ph;
int soil_ec;
int rssi_slave1, snr_slave1, rssi_slave2, snr_slave2;

//======Variable Count Slave
byte Slv = 0;
byte SL_Address;

byte Count_OLED_refresh_when_no_data_comes_in = 0;
byte Count_to_Rst_LORA = 0;

void sendMessage(String Outgoing, byte Destination)
{
    LoRa.beginPacket();
    LoRa.write(Destination);
    LoRa.write(LocalAddress);
    LoRa.write(Outgoing.length());
    LoRa.print(Outgoing);
    LoRa.endPacket();
}

void onReceive(int packetSize)
{
    if (packetSize == 0)
        return;

    int recipient = LoRa.read();
    byte sender = LoRa.read();
    byte incomingLength = LoRa.read();

    SL_Address = sender;
    Incoming = "";

    while (LoRa.available())
    {
        Incoming += (char)LoRa.read();
    }

    Count_OLED_refresh_when_no_data_comes_in = 0;

    if (incomingLength != Incoming.length())
    {
        Serial.println();
        Serial.println("er");
        return;
    }

    if (recipient != LocalAddress)
    {
        Serial.println();
        Serial.println("This message is not for me.");
        return;
    }

    Serial.println();
    Serial.println("Rc from: 0x" + String(sender, HEX));
    Serial.println("Message: " + Incoming);

    Processing_incoming_data();
}

void Processing_incoming_data()
{
    //======Get Data Slave 1 [Wemos D1 Mini Pro]
    if (SL_Address == Destination_ESP32_Slave_1)
    {
        Temp[0] = GetValue(Incoming, ',', 0).toFloat();
        pres = GetValue(Incoming, ',', 1).toFloat();
        alt = GetValue(Incoming, ',', 2).toFloat();
        rssi_slave1 = LoRa.packetRssi();
        snr_slave1 = LoRa.packetSnr();

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SH110X_WHITE);

        display.setCursor(20, 0);
        display.print("Slave 1 (Wemos)");

        display.setCursor(0, 15);
        display.print("Temp   : ");
        display.setCursor(60, 15);
        display.print(Temp[0]);
        display.print(" ");
        display.print((char)247);
        display.print("C");

        display.setCursor(0, 25);
        display.print("Pres   : ");
        display.setCursor(60, 25);
        display.print(pres);
        display.print(" pa");

        display.setCursor(0, 35);
        display.print("Alti   : ");
        display.setCursor(60, 35);
        display.print(alt);
        display.print(" m");

        display.setCursor(0, 45);
        display.print("RSSI   : ");
        display.setCursor(60, 45);
        display.print(rssi_slave1);

        display.setCursor(0, 55);
        display.print("SNR    : ");
        display.setCursor(60, 55);
        display.print(snr_slave1);

        display.display();
        delay(2000);
    }

    //======Get Data Slave 2 [ESP32 v1]
    if (SL_Address == Destination_ESP32_Slave_2)
    {
        Humd[0] = GetValue(Incoming, ',', 0).toFloat();
        Temp[1] = GetValue(Incoming, ',', 1).toFloat();
        soil_moisture = GetValue(Incoming, ',', 2).toFloat();
        soil_temperature = GetValue(Incoming, ',', 3).toFloat();
        soil_ec = GetValue(Incoming, ',', 4).toInt();
        soil_ph = GetValue(Incoming, ',', 5).toFloat();

        rssi_slave2 = LoRa.packetRssi();
        snr_slave2 = LoRa.packetSnr();

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SH110X_WHITE);

        display.setCursor(20, 0);
        display.print("Slave 2 (ESP32)");

        display.setCursor(0, 15);
        display.print("Temp A : ");
        display.setCursor(60, 15);
        display.print(Temp[1]);
        display.print(" ");
        display.print((char)247);
        display.print("C");

        display.setCursor(0, 25);
        display.print("Humi A : ");
        display.setCursor(60, 25);
        display.print(Humd[0]);
        display.print("%");

        display.setCursor(0, 45);
        display.print("RSSI   : ");
        display.setCursor(60, 45);
        display.print(rssi_slave2);

        display.setCursor(0, 55);
        display.print("SNR    : ");
        display.setCursor(60, 55);
        display.print(snr_slave2);

        display.display(); //============
        delay(2000);

        display.clearDisplay();
        display.setTextSize(1);

        display.setCursor(20, 0);
        display.print("Slave 2 -Soil-");

        display.setCursor(0, 15);
        display.print("S Mois : ");
        display.setCursor(60, 15);
        display.print(soil_moisture);
        display.print("%");

        display.setCursor(0, 25);
        display.print("S Temp : ");
        display.setCursor(60, 25);
        display.print(soil_temperature);
        display.print(" ");
        display.print((char)247);
        display.print("C");

        display.setCursor(0, 35);
        display.print("S EC   : ");
        display.setCursor(60, 35);
        display.print(soil_ec);

        display.setCursor(0, 45);
        display.print("S pH   : ");
        display.setCursor(60, 45);
        display.print(soil_ph);

        display.display(); //============
        delay(2000);
    }
    //==============================SEND SLAVE 1 (Wemos) to MQTT=========================================
    // BMP Sensor
    mqtt.publish(slave1_temperature_topic, String(Temp[0]).c_str());
    mqtt.publish(slave1_altitude_topic, String(alt).c_str());
    mqtt.publish(slave1_pressure_topic, String(pres).c_str());
    // LoRa Signal Strength
    mqtt.publish(slave1_rssi_topic, String(rssi_slave1).c_str());
    mqtt.publish(slave1_snr_topic, String(snr_slave1).c_str());

    //==============================SEND SLAVE 2 (ESP32) to MQTT=========================================
    // AHT10 Sensor
    mqtt.publish(slave2_temperature_topic, String(Temp[1]).c_str());
    mqtt.publish(slave2_humidity_topic, String(Humd[0]).c_str());
    // NPK 5 Probe Sensor
    mqtt.publish(slave2_soil_moisture_topic, String(soil_moisture).c_str());
    mqtt.publish(slave2_soil_temperature_topic, String(soil_temperature).c_str());
    mqtt.publish(slave2_soil_ec_topic, String(soil_ec).c_str());
    mqtt.publish(slave2_soil_ph_topic, String(soil_ph).c_str());
    // LoRa Signal Strength
    mqtt.publish(slave2_rssi_topic, String(rssi_slave2).c_str());
    mqtt.publish(slave2_snr_topic, String(snr_slave2).c_str());

    Incoming = "";
}

void Getting_data_for_the_first_time()
{
    Serial.println();
    Serial.println("Getting data for the first time...");

    while (true)
    {
        unsigned long currentMillis_SendMSG = millis();

        if (currentMillis_SendMSG - previousMillis_SendMSG >= interval_SendMSG)
        {
            previousMillis_SendMSG = currentMillis_SendMSG;

            Slv++;
            if (Slv > 2)
            {
                Slv = 0;
                Serial.println();
                Serial.println("Getting data for the first time has been completed.");
                break;
            }

            Message = "N,N";

            //=======Condition for sending message Slave 1 (Wemos)
            if (Slv == 1)
            {
                Serial.println();
                Serial.print("Send message to ESP32 Slave " + String(Slv));
                Serial.println(" for first time : " + Message);
                sendMessage(Message, Destination_ESP32_Slave_1);
            }

            //=======Condition for sending message Slave 2 (ESP32)
            if (Slv == 2)
            {
                Serial.println();
                Serial.print("Send message to ESP32 Slave " + String(Slv));
                Serial.println(" for first time : " + Message);
                sendMessage(Message, Destination_ESP32_Slave_2);
            }
        }

        onReceive(LoRa.parsePacket());
    }
}

String GetValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++)
    {
        if (data.charAt(i) == separator || i == maxIndex)
        {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void Rst_LORA()
{
    LoRa.setPins(ss, rst, dio0);

    Serial.println();
    Serial.println("Restart LoRa...");
    Serial.println("Start LoRa init...");
    if (!LoRa.begin(915E6))
    {
        Serial.println("LoRa init failed. Check your connections.");
        while (true)
            ;
    }
    Serial.println("LoRa init succeeded.");

    Count_to_Rst_LORA = 0;
}

void setup_wifi()
{
    delay(10);

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    mqtt.setServer(mqtt_server, 1883);
    mqtt.setClient(ESP32_ClimateHabibi);
}

void reconnect()
{
    while (!mqtt.connected())
    {
        Serial.print("Attempting MQTT connection...");
        if (mqtt.connect("ESP32_ClimateHabibi"))
        {
            Serial.println("connected");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(mqtt.state());
            Serial.println(" try again in 5 seconds");

            delay(5000);
        }
    }
}

//======VOID SETUP======
void setup()
{
    Serial.begin(115200);

    setup_wifi();

    if (!display.begin(0x3C, true))
    {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ;
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(35, 15);
    display.println("LoRa Master");
    display.setCursor(45, 35);
    display.println("Wroom32u");
    display.display();

    delay(2000);

    for (byte i = 0; i < 2; i++)
    {
        Humd[i] = 0;
        Temp[i] = 0.00;
        pres = 0.00;
        alt = 0.00;
        soil_moisture = 0.00;
        soil_temperature = 0.00;
        soil_ec = 0.00;
        soil_ph = 0.00;
    }

    delay(1000);

    Rst_LORA();
    Getting_data_for_the_first_time();
}

//======VOID LOOP=====
void loop()
{
    if (!mqtt.connected())
    {
        reconnect();
    }

    unsigned long currentMillis_SendMSG = millis();

    if (currentMillis_SendMSG - previousMillis_SendMSG >= interval_SendMSG)
    {
        previousMillis_SendMSG = currentMillis_SendMSG;

        Count_OLED_refresh_when_no_data_comes_in++;
        if (Count_OLED_refresh_when_no_data_comes_in > 5)
        {
            Count_OLED_refresh_when_no_data_comes_in = 0;
            Processing_incoming_data();
        }

        Slv++;
        if (Slv > 2)
            Slv = 1;

        if (Slv == 1)
        {
            Serial.println();
            Serial.println("Tr to  : 0x" + String(Destination_ESP32_Slave_1, HEX));
            Serial.println("Message: " + Message);
            Temp[0] = 0.00;
            pres = 0.00;
            alt = 0.00;
            sendMessage(Message, Destination_ESP32_Slave_1);
        }

        if (Slv == 2)
        {
            Serial.println();
            Serial.println("Tr to  : 0x" + String(Destination_ESP32_Slave_2, HEX));
            Serial.println("Message: " + Message);
            Humd[0] = 0;
            Temp[1] = 0.00;
            soil_moisture = 0.00;
            soil_temperature = 0.00;
            soil_ec = 0.00;
            soil_ph = 0.00;
            sendMessage(Message, Destination_ESP32_Slave_2);
        }

    unsigned long currentMillis_RestartLORA = millis();

    if (currentMillis_RestartLORA - previousMillis_RestartLORA >= interval_RestartLORA)
    {
        previousMillis_RestartLORA = currentMillis_RestartLORA;

        Count_to_Rst_LORA++;
        if (Count_to_Rst_LORA > 30)
        {
            LoRa.end();
            Rst_LORA();
        }
    }

    onReceive(LoRa.parsePacket());
    mqtt.loop();

}