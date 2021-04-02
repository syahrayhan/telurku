#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <DHT.h>
#include <EEPROM.h>
//3 jam 10800000
const unsigned long eventInterval = 60000;
unsigned long previousTime = 0;
const char *ssid = "Redmi9a";
const char *password = "123456789101";

long elapsedMillis;
long startMillis = millis();

float temp, valETemp;
int addrsTemp, valEReset, valEShutdown;
int addrsShutdown, addrsReset, dataShutdown, dataReset;

int addrsETemp = 0;
int addrsEShutdown = 1;
int addrsEReset = 2;

#define relay 5   // Untuk D1     penggerak
#define relay2 4  // Untuk D2     3 lampu
#define dht_pin 2 // Untuk D4      dht

FirebaseData firebaseData;
DHT dht(dht_pin, DHT11);

void setup()
{
    Serial.begin(9600);
    pinMode(relay, OUTPUT);
    pinMode(relay2, OUTPUT);
    digitalWrite(relay, HIGH);
    digitalWrite(relay2, LOW);

    connectWifi();
    Firebase.begin("eggcubator-d446f-default-rtdb.firebaseio.com", "4KYOs3lhfrbQdWKqAu9P5JkMgTtn6nwUV1WqC07i");
    dht.begin();
    EEPROM.begin(512);
    saveeeprom();
}

void saveeeprom()
{
    addrsTemp = Firebase.getInt(firebaseData, "/EggData/maxTemp");
    temp = firebaseData.intData();
    
    EEPROM.write(addrsETemp, temp);
    Serial.println("Berhasil Write ETemp");

    addrsShutdown = Firebase.getInt(firebaseData, "/SettingData/isShutdown");
    dataShutdown = firebaseData.intData();
    addrsReset = Firebase.getInt(firebaseData, "/SettingData/isReset");
    dataReset = firebaseData.intData();

    EEPROM.write(addrsEReset, dataReset);
    Serial.println("Berhasil Write EReset");

    EEPROM.write(addrsEShutdown, dataShutdown);
    Serial.println("Berhasil Write EShutdown");
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
        return;
    }

    addrsTemp = Firebase.getInt(firebaseData, "/EggData/maxTemp");
    temp = firebaseData.intData();

    valETemp = EEPROM.read(addrsETemp);
    if (valETemp != temp)
    {
        EEPROM.write(addrsETemp, temp);
        Serial.println("Berhasil Write ETemp");
    }

    Serial.print("valETemp = ");
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

    if (WiFi.status() != WL_CONNECT_FAILED)
    {
        Serial.println("online");
        addrsShutdown = Firebase.getInt(firebaseData, "/SettingData/isShutdown");
        dataShutdown = firebaseData.intData();
        addrsReset = Firebase.getInt(firebaseData, "/SettingData/isReset");
        dataReset = firebaseData.intData();

        valEReset = EEPROM.read(addrsEReset);
        valEShutdown = EEPROM.read(addrsEShutdown);
        if (valEReset != dataReset)
        {
            EEPROM.write(addrsEReset, dataReset);
            Serial.println("Berhasil Write EReset");
        }

        if (valEShutdown != dataShutdown)
        {
            EEPROM.write(addrsEShutdown, dataShutdown);
            Serial.println("Berhasil Write EShutdown");
        }
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
    delay(250);
}
