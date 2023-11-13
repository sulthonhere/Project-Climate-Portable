//LoRa Slave 1 [Wemos D1 Mini Pro]

#include <LoRa.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>

#define ss 15
#define rst 2
#define dio0 16

Adafruit_BMP280 bmp; 

String Incoming = "";
String Message = "";

byte LocalAddress = 0x02;   
byte Destination_Master = 0x01; 

float temperature_BMP280, pressure_BMP280, altitude_BMP280;

unsigned long previousMillis_UpdateBMP280 = 0;
const long interval_UpdateBMP280 = 2000;

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
  Message = String(temperature_BMP280) + "," + String(pressure_BMP280) + "," + String(altitude_BMP280);

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
  while (!Serial)
    delay(100); 

  Serial.println(F("SLAVE 1_WEMOS D1 Mini Pro"));
  unsigned status;
  status = bmp.begin(0x76);
  if (!status)
  {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or"));
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  Rst_LORA();
}

void loop()
{
  unsigned long currentMillis_UpdateBMP280 = millis();

  if (currentMillis_UpdateBMP280 - previousMillis_UpdateBMP280 >= interval_UpdateBMP280)
  {
    previousMillis_UpdateBMP280 = currentMillis_UpdateBMP280;

    temperature_BMP280 = bmp.readTemperature();
    pressure_BMP280 = bmp.readPressure();
    altitude_BMP280 = bmp.readAltitude(1013.25);

    Serial.print(F("Temperature = "));
    Serial.print(temperature_BMP280);
    Serial.println(" *C");

    Serial.print(F("Pressure = "));
    Serial.print(pressure_BMP280);
    Serial.println(" Pa");

    Serial.print(F("Approx altitude = "));
    Serial.print(altitude_BMP280); 
    Serial.println(" m");

    if (isnan(temperature_BMP280) || isnan(pressure_BMP280) || isnan(altitude_BMP280))
    {
      Serial.println(F("Failed to read from BMP280 sensor!"));
      return;
    }
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