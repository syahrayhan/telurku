#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <DHT.h>
#include <EEPROM.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h" 
//3 jam 10800000
const unsigned long eventInterval = 60000;
unsigned long previousTime = 0;

long elapsedMillis;
long startMillis = millis();

float temp, valETemp;
int addrsTemp, valEReset, valEShutdown;
int addrsShutdown, addrsReset, dataShutdown, dataReset;
//int addrsShutdown, addrsReset, dataShutdown, dataReset;
int addrsETemp = 0;
int addrsEShutdown = 1;
int addrsEReset = 2;

#define relay 5   // Untuk D1     penggerak
#define relay2 4  // Untuk D2     3 lampu
#define dht_pin 2 // Untuk D4     dht
#define buzzer 14   // Untuk D5        buzzer

FirebaseData firebaseData;
DHT dht(dht_pin, DHT11);

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void wifidynamic () {
  WiFiManager wifiManager;
  //reset settings - for testing
  wifiManager.resetSettings();
 
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
 
  if(!wifiManager.autoConnect("Telurku WIFI")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  } 
 
  //if you get here you have connected to the WiFi
  Serial.println(F("WIFIManager connected!"));
 
  Serial.print(F("IP --> "));
  Serial.println(WiFi.localIP());
  Serial.print(F("GW --> "));
  Serial.println(WiFi.gatewayIP());
  Serial.print(F("SM --> "));
  Serial.println(WiFi.subnetMask());
 
  Serial.print(F("DNS 1 --> "));
  Serial.println(WiFi.dnsIP(0));
 
  Serial.print(F("DNS 2 --> "));
  Serial.println(WiFi.dnsIP(1));
}

void setup()
{
    Serial.begin(9600);
    pinMode(relay, OUTPUT);
    pinMode(relay2, OUTPUT);
    pinMode(buzzer, OUTPUT);
    digitalWrite(relay, HIGH);
    digitalWrite(relay2, HIGH);
    digitalWrite(buzzer, LOW);

    wifidynamic();
//    connectWifi();
    Firebase.begin("eggcubator-d446f-default-rtdb.firebaseio.com", "4KYOs3lhfrbQdWKqAu9P5JkMgTtn6nwUV1WqC07i");
    dht.begin();
    EEPROM.begin(512);
    addrsTemp = Firebase.getInt(firebaseData, "/EggData/maxTemp");
    temp = firebaseData.intData();
    Serial.println(temp);
    EEPROM.write(addrsETemp, temp);
    Serial.println("Berhasil Write ETemp");
}

void connectWifi()
{
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println(WiFi.status());

    Serial.println("Sukses terkoneksi wifi!");
    Serial.println("IP Address: ");
    Serial.println(WiFi.localIP());
}

void sensorDHT()
{
    float kelembaban = dht.readHumidity();
    float suhu = dht.readTemperature();

    if (isnan(kelembaban) || isnan(suhu))
    {
        Serial.println("Gagal untuk membaca sensor DHT :(");
        digitalWrite(buzzer, HIGH);
        return;
    }
    digitalWrite(buzzer, LOW);

    if(WiFi.status() == 3) {
      addrsTemp = Firebase.getInt(firebaseData, "/EggData/maxTemp");
      temp = firebaseData.intData();
      valETemp = EEPROM.read(addrsETemp);
      Serial.println("dht Online");
      if (valETemp != temp && WiFi.status() == 3)
      {
        Serial.println(valETemp);
        Serial.println(temp);
        EEPROM.write(addrsETemp, temp);
        Serial.println("Berhasil Write ETemp");
      }
    }else {
      Serial.println("DHT OFFLINE");
    }

    Serial.println("ValETemp: ");
    Serial.println(valETemp);

    Serial.print("Kelembaban: ");
    Serial.print(kelembaban);
    Serial.print(" ");
    Serial.print("Suhu: ");
    Serial.print(suhu);
    Serial.println();

    if (Firebase.setFloat(firebaseData, "/FirebaseIOT/temperature", suhu))
    {
        Serial.println("Data suhu terkirim :)");
        // Serial.println();
    }
    else
    {
        Serial.println("Data suhu gagal terkirim :(");
        Serial.println("Error : " + firebaseData.errorReason());
        Serial.println();
    }

    if (Firebase.setFloat(firebaseData, "/FirebaseIOT/humidity", kelembaban))
    {
        Serial.println("Data kelembaban terkirim :)");
        Serial.println();
    }
    else
    {
        Serial.println("Data kelembaban gagal terkirim :(");
        Serial.println("Error : " + firebaseData.errorReason());
        Serial.println();
    }

    if (addrsTemp)
    {
        if (suhu > temp)
        {
            Serial.println("Lampu mati");
            Serial.println(temp);
            digitalWrite(relay2, HIGH);
        }
        else if (suhu < (temp - 1.5))
        {
            Serial.println("Lampu hidup");
            digitalWrite(relay2, LOW);
        }
    }
    else
    {
        Serial.print("Error saat ambil data, Menggunakan data awal ");
        Serial.println(valETemp);
        Serial.println(firebaseData.errorReason());
        if (suhu > valETemp)
        {
            Serial.println("Lampu mati");
            Serial.println(valETemp);
            digitalWrite(relay2, HIGH);
        }
        else if (suhu < (valETemp - 1.5))
        {
            Serial.println("Lampu hidup");
            digitalWrite(relay2, LOW);
        }
    }
}

void loop()
{
    unsigned long currentTime = millis();

    if (WiFi.status() == 3)
    {
       Serial.println("online");
       Serial.println(WiFi.status() == WL_CONNECTED);
       addrsShutdown = Firebase.getInt(firebaseData, "/SettingData/isShutdown");
       dataShutdown = firebaseData.intData();
       addrsReset = Firebase.getInt(firebaseData, "/SettingData/isReset");
       dataReset = firebaseData.intData();

        valEReset = EEPROM.read(addrsEReset);
        valEShutdown = EEPROM.read(addrsEShutdown);
        if (valEReset != dataReset && WiFi.status() == 3)
        {
            Serial.println(valEReset);
            Serial.println(dataReset);
            EEPROM.write(addrsEReset, dataReset);
            Serial.println("Berhasil Write EReset");
        }

        if (valEShutdown != dataShutdown && WiFi.status() == 3)
        {
                      Serial.println(valEShutdown);
            Serial.println(dataShutdown);
            EEPROM.write(addrsEShutdown, dataShutdown);
            Serial.println("Berhasil Write EShutdown");
        }

           Serial.print("addrs Shutdown =  ");
    Serial.println(addrsShutdown);
    Serial.print("data Shutdown = ");
    Serial.println(dataShutdown);

    Serial.print("addrs Reset = ");
    Serial.println(addrsReset);
    Serial.print("data Reset = ");
    Serial.println(dataReset);

    Serial.print("valEReset = ");
    Serial.println(valEReset);
    Serial.print("valEShutdown = ");
    Serial.println(valEShutdown);
    }else {
      Serial.println("offline");

         Serial.print("addrs Shutdown =  ");
    Serial.println(addrsShutdown);
    Serial.print("data Shutdown = ");
    Serial.println(dataShutdown);

    Serial.print("addrs Reset = ");
    Serial.println(addrsReset);
    Serial.print("data Reset = ");
    Serial.println(dataReset);

    Serial.print("valEReset = ");
    Serial.println(valEReset);
    Serial.print("valEShutdown = ");
    Serial.println(valEShutdown);
    }


    if (addrsShutdown)
    {
        if (dataShutdown == 0 && dataReset == 0)
        {
            sensorDHT();
            if (addrsReset)
            {
                if (dataReset == 0)
                {
                    if (currentTime - previousTime >= eventInterval)
                    {
                        /* Event code */
                        Serial.println("Ice Ice Baby");
                        digitalWrite(relay, LOW);
                        delay(7000);
                        Serial.println("nom nom");
                        digitalWrite(relay, HIGH);

                        /* Update the timing for the next time around */
                        previousTime = currentTime;
                    }
                }
                else
                {
                    // Serial.println("alat hidup dan tidak direset");
                    previousTime = currentTime;
                }
            }
        }
        else
        {
            Serial.println("alat mati");
            digitalWrite(relay, HIGH);
            digitalWrite(relay2, HIGH);
            digitalWrite(dht_pin, HIGH);
        }
    }
    else
    {
        if (valEShutdown == 0 && valEReset == 0)
        {
            sensorDHT();
            if (valEReset == 0)
            {
                if (currentTime - previousTime >= eventInterval)
                {
                    /* Event code */
                    Serial.println("Dinamo berputar");
                    digitalWrite(relay, LOW);
                    delay(7000);
                    digitalWrite(relay, HIGH);

                    /* Update the timing for the next time around */
                    previousTime = currentTime;
                }
            }
            else
            {
                previousTime = currentTime;
            }
        }
        else
        {
            Serial.println("alat mati");
            digitalWrite(relay, HIGH);
            digitalWrite(relay2, HIGH);
            digitalWrite(dht_pin, HIGH);
        }
    }
    delay(1000);
}
