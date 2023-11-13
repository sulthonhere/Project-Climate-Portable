// Slave 2 (ESP32 v1)

#include <LoRa.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_AHT10.h>
#include <SoftwareSerial.h>

#define ss 5
#define rst 4
#define dio0 2

#define RE 14
#define DE 14

// From ModbusPoll (pH value AFTER INVERTED)
const byte npk_moisture[] = {0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0xD5, 0xCA};    // 0-100%       (2 digit, harus dibagi 10) Float
const byte npk_temperature[] = {0x01, 0x03, 0x00, 0x02, 0x00, 0x01, 0x25, 0xCA}; // -40-80y      (2 digit, harus dibagi 10) Float
const byte npk_ec[] = {0x01, 0x03, 0x00, 0x03, 0x00, 0x01, 0x74, 0x0A};          // 0-2000 us/cm (2 digit) Int
const byte npk_ph[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A};          // 3-9pH        (2 digit, harus dibagi 10) Float

byte values[8];
SoftwareSerial mod(16, 17);

Adafruit_AHT10 aht;

String Incoming = "";
String Message = "";

byte LocalAddress = 0x03;       
byte Destination_Master = 0x01; 

float temperature_AHT10, humidity_AHT10, soil_moisture, soil_temperature, soil_ph;
int soil_ec;

unsigned long previousMillis_UpdateAHT10 = 0;
const long interval_UpdateAHT10 = 2000;

unsigned long previousMillis_RestartLORA = 0;
const long interval_RestartLORA = 1000;

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
  
  if (sender != Destination_Master)
  {
    Serial.println();
    Serial.println("i"); 

    Count_to_Rst_LORA = 0;
    return; 
  }
  
  Incoming = "";

  while (LoRa.available())
  {
    Incoming += (char)LoRa.read();
  }

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

  else
  {
    Serial.println();
    Serial.println("Rc from: 0x" + String(sender, HEX));
    Serial.println("Message: " + Incoming);

    Processing_incoming_data();
  }
}

void Processing_incoming_data()
{
  Message = String(temperature_AHT10) + "," + String(humidity_AHT10) + "," + String(soil_moisture) + "," + String(soil_temperature) + "," + String(soil_ec) + "," + String(soil_ph);

  Serial.println();
  Serial.println("Tr to  : 0x" + String(Destination_Master, HEX));
  Serial.println("Message: " + Message);

  sendMessage(Message, Destination_Master);
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
  Serial.println(F("Restart LoRa..."));
  Serial.println(F("Start LoRa init..."));
  if (!LoRa.begin(915E6))
  { 
    Serial.println(F("LoRa init failed. Check your connections."));
    while (true)
      ; 
  }
  Serial.println(F("LoRa init succeeded."));

  Count_to_Rst_LORA = 0;
}

void setup()
{
  Serial.begin(115200);
  mod.begin(4800);

  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);

  while (!Serial)
    delay(100); 

  Serial.println(F("SLAVE 2_ESP32"));

  if (!aht.begin())
  {
    Serial.println("Could not find AHT10? Check wiring");
    while (1)
      delay(10);
  }

  Rst_LORA();
}

void loop()
{
  unsigned long currentMillis_UpdateAHT10 = millis();

  if (currentMillis_UpdateAHT10 - previousMillis_UpdateAHT10 >= interval_UpdateAHT10)
  {
    previousMillis_UpdateAHT10 = currentMillis_UpdateAHT10;

    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp); 

    temperature_AHT10 = temp.temperature;
    humidity_AHT10 = humidity.relative_humidity;

    Serial.print(F("Temperature = "));
    Serial.print(temperature_AHT10);
    Serial.println(" *C");

    Serial.print(F("Humidity = "));
    Serial.print(humidity_AHT10);
    Serial.println(" %");

    if (isnan(temperature_AHT10) || isnan(humidity_AHT10))
    {
      Serial.println(F("Failed to read from AHT10 sensor!"));
      return;
    }

    //======Soil Moisture Reading======
    digitalWrite(DE, HIGH);
    digitalWrite(RE, HIGH);
    delay(10);

    if (mod.write(npk_moisture, sizeof(npk_moisture)) == 8)
    {
      digitalWrite(DE, LOW);
      digitalWrite(RE, LOW);
      for (byte i = 0; i < 7; i++)
      {
        values[i] = mod.read();
        //Serial.print(values[i], HEX);
      }
      //Serial.println();
    }
    soil_moisture = float(values[3] << 8 | values[4]);
    soil_moisture /= 10;
    delay(50);

    //======Soil Temperature Reading======
    digitalWrite(DE, HIGH);
    digitalWrite(RE, HIGH);
    delay(10);

    if (mod.write(npk_temperature, sizeof(npk_temperature)) == 8)
    {
      digitalWrite(DE, LOW);
      digitalWrite(RE, LOW);
      for (byte i = 0; i < 7; i++)
      {
        values[i] = mod.read();
        //Serial.print(values[i], HEX);
      }
      //Serial.println();
    }
    soil_temperature = float(values[3] << 8 | values[4]);
    soil_temperature /= 10;
    delay(50);

    //======Soil EC Reading======
    digitalWrite(DE, HIGH);
    digitalWrite(RE, HIGH);
    delay(10);

    if (mod.write(npk_ec, sizeof(npk_ec)) == 8)
    {
      digitalWrite(DE, LOW);
      digitalWrite(RE, LOW);
      for (byte i = 0; i < 7; i++)
      {
        values[i] = mod.read();
        //Serial.print(values[i], HEX);
      }
      //Serial.println();
    }
    soil_ec = int(values[3] << 8 | values[4]);
    delay(50);

    //======Soil pH Reading======
    digitalWrite(DE, HIGH);
    digitalWrite(RE, HIGH);
    delay(10);

    if (mod.write(npk_ph, sizeof(npk_ph)) == 8)
    {
      digitalWrite(DE, LOW);
      digitalWrite(RE, LOW);
      for (byte i = 0; i < 7; i++)
      {
        values[i] = mod.read();
        //Serial.print(values[i], HEX);
      }
      //Serial.println();
    }
    soil_ph = float(values[3] << 8 | values[4]);
    soil_ph /= 10;
    delay(50);

    //SerialPrintAll();
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
}

void SerialPrintAll()
{
  Serial.print("Ambient Humidity : ");
  Serial.println(humidity_AHT10);

  Serial.print("Ambient Temperature : ");
  Serial.println(temperature_AHT10);

  Serial.print("Soil Moisture: ");
  Serial.println(soil_temperature);

  Serial.print("Soil Temperature : ");
  Serial.println(soil_temperature);

  Serial.print("Soil EC : ");
  Serial.println(soil_ec);

  Serial.print("Soil pH : ");
  Serial.println(soil_ph);
}
