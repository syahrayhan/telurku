#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <DHT.h>  

//3 jam 10800000
const unsigned long eventInterval = 60000;
unsigned long previousTime = 0;
const char* ssid = "Redmi9a";
const char* password = "123456789101";

long elapsedMillis;
long startMillis = millis();

float temp;
int a;
int b;
int c;
int d;

#define relay 5         // Untuk D1     penggerak 
#define relay2 4        // Untuk D2     3 lampu
#define dht_pin 2      // Untuk D4      dht

FirebaseData firebaseData;
DHT dht(dht_pin, DHT11);



void setup() {
    Serial.begin(9600);
    pinMode(relay, OUTPUT);
    pinMode(relay2, OUTPUT);
    digitalWrite(relay, HIGH);
    digitalWrite(relay2, LOW);

    connectWifi();
    Firebase.begin("eggcubator-d446f-default-rtdb.firebaseio.com", "4KYOs3lhfrbQdWKqAu9P5JkMgTtn6nwUV1WqC07i");
    dht.begin();
}

void offline() {
   
}

void connectWifi() {
    WiFi.begin(ssid, password);

    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    } 
    Serial.println(WiFi.status());

      if(WiFi.status() != WL_CONNECT_FAILED){
    Serial.println("online");
    a = Firebase.getInt(firebaseData, "/SettingData/isShutdown");
    b = firebaseData.intData();
    c = Firebase.getInt(firebaseData, "/SettingData/isReset");
    d = firebaseData.intData();
  }
    Serial.println("Sukses terkoneksi wifi!");
    Serial.println("IP Address: ");
    Serial.println(WiFi.localIP());
}


void sensorDHT() {
    float kelembaban = dht.readHumidity();
    float suhu = dht.readTemperature();

    if(isnan(kelembaban) || isnan(suhu)) {
        Serial.println("Gagal untuk membaca sensor DHT :(");
        return;
    }

    Serial.print("Kelembaban: ");
    Serial.print(kelembaban);
    Serial.print(" ");
    Serial.print("Suhu: ");
    Serial.print(suhu);
    Serial.println();

    if (Firebase.setFloat(firebaseData, "/FirebaseIOT/temperature", suhu)) {
        Serial.println("Data suhu terkirim :)");
        // Serial.println();
    } else {
        Serial.println("Data suhu gagal terkirim :(");
        Serial.println("Error : " + firebaseData.errorReason());
        Serial.println();
    }

    if (Firebase.setFloat(firebaseData, "/FirebaseIOT/humidity", kelembaban)) {
        Serial.println("Data kelembaban terkirim :)");
        Serial.println();
    } else {
        Serial.println("Data kelembaban gagal terkirim :(");
        Serial.println("Error : " + firebaseData.errorReason());
        Serial.println();
    }


   if (Firebase.getInt(firebaseData, "/EggData/maxTemp")) {
       temp = firebaseData.intData();
       if (suhu > temp) {
           Serial.println("Lampu mati");
           Serial.println(temp);
           digitalWrite(relay2, HIGH);
       } else if (suhu < (temp - 1.5)) {
           Serial.println("Lampu hidup");
           digitalWrite(relay2, LOW);
       }
   } else {
       Serial.print("Error saat ambil data, Menggunakan data awal ");
       Serial.println(temp);
       Serial.println(firebaseData.errorReason());
        if (suhu > temp) {
           Serial.println("Lampu mati");
           Serial.println(temp);
           digitalWrite(relay2, HIGH);
       } else if (suhu < (temp - 1.5)) {
           Serial.println("Lampu hidup");
           digitalWrite(relay2, LOW);
       }
   }
}


void loop () {
  unsigned long currentTime = millis();

  if(WiFi.status() != WL_CONNECT_FAILED){
    Serial.println("online");
    a = Firebase.getInt(firebaseData, "/SettingData/isShutdown");
    b = firebaseData.intData();
    c = Firebase.getInt(firebaseData, "/SettingData/isReset");
    d = firebaseData.intData();
  }
  Serial.println("not online");
  Serial.print("data dari firebase ");
  Serial.println(a);
  Serial.print("ini yang baru");
  Serial.println(b);

  Serial.print("data dari reset ");
  Serial.println(c);
  Serial.print("ini yang baru reset");
  Serial.println(d);

  if(a) {
    if(b == 0 && d == 0) {
        // Serial.println("alat hidup");
        sensorDHT();
        if(c) {
            if (d == 0) {
                // Serial.println("alat hidup dan tidak direset");
                if (currentTime - previousTime >= eventInterval) {
                    /* Event code */ 
                    Serial.println("Ice Ice Baby");
                    digitalWrite(relay, LOW);
                    delay(8000);
                    Serial.println("nom nom");
                    digitalWrite(relay, HIGH);
            
                /* Update the timing for the next time around */
                    previousTime = currentTime;
                }
            } else {
                // Serial.println("alat hidup dan tidak direset");
                previousTime = currentTime;
            }
        }
    } else {
        Serial.println("alat mati");
        digitalWrite(relay, HIGH);
        digitalWrite(relay2, HIGH);
        digitalWrite(dht_pin, HIGH);
    }
  } else {
  if(b == 0 && d == 0) {
        Serial.println("alat hidup");
        sensorDHT();
            if (d == 0) {
                 Serial.println("alat hidup dan tidak direset");
                if (currentTime - previousTime >= eventInterval) {
                    /* Event code */
                    Serial.println("Ice Ice Baby");
                    digitalWrite(relay, LOW);            
                    delay(8000);
                    Serial.println("nom nom");
                    digitalWrite(relay, HIGH);
            
                /* Update the timing for the next time around */
                    previousTime = currentTime;
                }
            } else {
                // Serial.println("alat hidup dan tidak direset");
                previousTime = currentTime;
            }
    } else {
        Serial.println("alat mati");
        digitalWrite(relay, HIGH);
        digitalWrite(relay2, HIGH);
        digitalWrite(dht_pin, HIGH);
    }
  
  }
    delay(250);
}
