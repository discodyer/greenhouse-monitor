#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP32Ticker.h>
#include <TimeLib.h>
#include <BH1750.h>
#include "sensirion_common.h"
#include "sgp30.h"

String getLocalTime();
void setup_wifi();
void reconnect();
void callback(char *topic, byte *payload, unsigned int length);
void send_temperatureHumidity();
void send_soilTemperatureHumidity();
void send_gas();
void send_lightIntensity();
void send_infrared(int state);
void infrared_callback();
void timerCallback();
int getLEL();
int getCO2();

// MQTT Broker
const char *mqtt_broker = "114.114.114.114"; // 你的MQTT服务器IP
const char *topic_temp_hum = "/temperatureHumidity";
const char *topic_soil = "/soilTemperatureHumidity";
const char *topic_gas = "/gas";
const char *topic_infrared = "/infrared";
const char *topic_ventilator = "/ventilator";
const char *topic_light = "/light";
const char *topic_pump = "/pump";
const char *topic_lightIntensity = "/lightIntensity";
// const char *mqtt_username = "emqx";
// const char *mqtt_password = "public";
// const char *equipmentToken = "10001";
#define equipment_token 10001
const int mqtt_port = 1883;

#define DHTPIN 15
#define INFRAREDPIN 7
#define SOILPIN 8
#define NH3PIN 10
#define LELPIN 9
#define LIGHTPIN 11
#define FANPIN 12
#define PUMPPINF 13
#define PUMPPINB 14
#define I2C_SDA 5
#define I2C_SCL 6

const char *ssid = "WIFI_SSID";      // 你的网络名称
const char *password = "password";   // 你的网络密码
const char *ntpServer = "ntp1.aliyun.com";
const long gmtOffset_sec = 8 * 3600; // 东八区时区偏移量
const int daylightOffset_sec = 0;    // 夏令时偏移量
int infrared_state = 0;
#define DHTTYPE DHT11

DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

BH1750 lightMeter(0x23);

#define INTERRUPT_INTERVAL 3000         // 中断触发间隔（毫秒）
#define INFRARED_INTERRUPT_INTERVAL 200 // 中断触发间隔（毫秒）

Ticker ticker_3s;
Ticker ticker_infrared;

void setup()
{
    s16 err;
    u32 ah = 0;
    u16 scaled_ethanol_signal, scaled_h2_signal;
    // 初始化串口通信
    Serial0.begin(115200, SERIAL_8N1, 44, 43);

    Wire.begin(I2C_SDA, I2C_SCL);

    /*  Init module,Reset all baseline,The initialization takes up to around 15 seconds, during which
        all APIs measuring IAQ(Indoor air quality ) output will not change.Default value is 400(ppm) for co2,0(ppb) for tvoc*/
    while (sgp_probe() != STATUS_OK) {
        Serial0.println("SGP failed");
        while (1);
    }
    /*Read H2 and Ethanol signal in the way of blocking*/
    err = sgp_measure_signals_blocking_read(&scaled_ethanol_signal,
                                            &scaled_h2_signal);
    if (err == STATUS_OK) {
        Serial0.println("get ram signal!");
    } else {
        Serial0.println("error reading signals");
    }

    // Set absolute humidity to 13.000 g/m^3
    //It's just a test value
    sgp_set_absolute_humidity(13000);
    err = sgp_iaq_init();

    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE))
    {
        Serial0.println(F("BH1750 Advanced begin"));
    }
    else
    {
        Serial0.println(F("Error initialising BH1750"));
    }

    pinMode(INFRAREDPIN, INPUT_PULLUP);
    pinMode(SOILPIN, INPUT);
    pinMode(LELPIN, INPUT);
    pinMode(NH3PIN, INPUT);
    // pinMode(LIGHTINTENSITYPIN, INPUT);

    pinMode(LIGHTPIN, OUTPUT);
    digitalWrite(LIGHTPIN, LOW);
    pinMode(PUMPPINF, OUTPUT);
    pinMode(FANPIN, OUTPUT);

    analogWrite(FANPIN, 0);
    analogWrite(PUMPPINF, 0);

    // Clear the buffer
    // display.clearDisplay();
    // Draw a single pixel in white
    // display.drawPixel(10, 10, SSD1306_WHITE);
    // Show the display buffer on the screen. You MUST call display() after
    // drawing commands to make them visible on screen!
    // display.display();
    delay(100);

    setup_wifi();

    // getLocalTime();
    dht.begin();
    Serial0.println(F("DHTxx Unified Sensor Example"));
    // Print temperature sensor details.
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);
    Serial0.println(F("------------------------------------"));
    Serial0.println(F("Temperature Sensor"));
    Serial0.print(F("Sensor Type: "));
    Serial0.println(sensor.name);
    Serial0.print(F("Driver Ver:  "));
    Serial0.println(sensor.version);
    Serial0.print(F("Unique ID:   "));
    Serial0.println(sensor.sensor_id);
    Serial0.print(F("Max Value:   "));
    Serial0.print(sensor.max_value);
    Serial0.println(F("°C"));
    Serial0.print(F("Min Value:   "));
    Serial0.print(sensor.min_value);
    Serial0.println(F("°C"));
    Serial0.print(F("Resolution:  "));
    Serial0.print(sensor.resolution);
    Serial0.println(F("°C"));
    Serial0.println(F("------------------------------------"));
    // Print humidity sensor details.
    dht.humidity().getSensor(&sensor);
    Serial0.println(F("Humidity Sensor"));
    Serial0.print(F("Sensor Type: "));
    Serial0.println(sensor.name);
    Serial0.print(F("Driver Ver:  "));
    Serial0.println(sensor.version);
    Serial0.print(F("Unique ID:   "));
    Serial0.println(sensor.sensor_id);
    Serial0.print(F("Max Value:   "));
    Serial0.print(sensor.max_value);
    Serial0.println(F("%"));
    Serial0.print(F("Min Value:   "));
    Serial0.print(sensor.min_value);
    Serial0.println(F("%"));
    Serial0.print(F("Resolution:  "));
    Serial0.print(sensor.resolution);
    Serial0.println(F("%"));
    Serial0.println(F("------------------------------------"));
    // Set delay between sensor readings based on sensor details.
    delayMS = sensor.min_delay / 1000;

    client.setServer(mqtt_broker, 1883);
    client.setCallback(callback);
    delay(1500);
    ticker_3s.attach_ms(INTERRUPT_INTERVAL, timerCallback);
    ticker_infrared.attach_ms(INFRARED_INTERRUPT_INTERVAL, infrared_callback);

    // attachInterrupt(digitalPinToInterrupt(INFRAREDPIN), infrared_callback, CHANGE);
}

void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();
}

String getLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial0.println("Failed to obtain time");
        return "";
    }
    // Serial0.println(&timeinfo, "%F %T %A"); // 格式化输出
    // display.setTextSize(1);
    // display.clearDisplay();
    // display.setTextColor(WHITE);
    // display.setCursor(0, 0);
    // Display static text
    // display.println(&timeinfo, "%F %T\n%A");
    // display.display();
    char timebuf[30] = {};
    strftime(timebuf, sizeof(timebuf), "%F %T %A", &timeinfo);
    String time = String(timebuf);
    // Serial0.println(time); // 格式化输出
    return time;
}

void setup_wifi()
{

    delay(10);
    // We start by connecting to a WiFi network
    Serial0.println();
    Serial0.print("Connecting to ");
    Serial0.println(ssid);

    WiFi.mode(WIFI_STA);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial0.print(".");
    }

    randomSeed(micros());

    Serial0.println("");
    Serial0.println("WiFi connected");
    Serial0.println("IP address: ");
    Serial0.println(WiFi.localIP());
    // 从网络时间服务器上获取并设置时间
    // 获取成功后芯片会使用RTC时钟保持时间的更新
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial0.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP32Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(clientId.c_str()))
        {
            Serial0.println("connected");
            // Once connected, publish an announcement...
            // client.publish(topic_gas, "hello world");
            // ... and resubscribe
            client.subscribe(topic_ventilator);
            client.subscribe(topic_light);
            client.subscribe(topic_pump);
        }
        else
        {
            Serial0.print("failed, rc=");
            Serial0.print(client.state());
            Serial0.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial0.print("Message arrived [");
    Serial0.print(topic);
    Serial0.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial0.print((char)payload[i]);
    }
    Serial0.println();

    StaticJsonDocument<200> json_payload;
    DeserializationError error = deserializeJson(json_payload, payload);

    if (error)
    {
        Serial0.print(F("deserializeJson() failed: "));
        Serial0.println(error.f_str());
        return;
    }

    int payload_token = json_payload["equipmentToken"];

    if (payload_token != equipment_token)
    {
        return;
    }

    if (strncmp(topic, topic_ventilator, 11) == 0)
    {
        if (json_payload["ventilator"] == 0)
        {
            // 关闭风扇
            Serial0.println("turn off fan");
            analogWrite(FANPIN, 0);
        }
        else if (json_payload["ventilator"] == 1)
        {
            // 打开风扇
            Serial0.println("turn on fan");
            analogWrite(FANPIN, 200);
        }
    }
    else if (strncmp(topic, topic_light, 6) == 0)
    {
        if (json_payload["light"] == 0)
        {
            Serial0.println("turn off light");
            digitalWrite(LIGHTPIN, LOW);
        }
        else if (json_payload["light"] == 1)
        {
            Serial0.println("turn on light");
            digitalWrite(LIGHTPIN, HIGH);
        }
    }
    else if (strncmp(topic, topic_pump, 5) == 0)
    {
        if (json_payload["pump"] == 0)
        {
            // 关闭水泵
            Serial0.println("turn off pump");
            analogWrite(PUMPPINF, 0);
        }
        else if (json_payload["pump"] == 1)
        {
            // 开启水泵
            Serial0.println("turn on pump");
            analogWrite(PUMPPINF, 240);
        }
    }
}

void send_temperatureHumidity()
{
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature))
    {
        Serial0.println(F("Error reading temperature!"));
        return;
    }
    else
    {
        Serial0.print(F("Temperature: "));
        Serial0.print(event.temperature);
        Serial0.println(F("C"));
    }
    String temp = String(event.temperature, 1);

    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity))
    {
        Serial0.println(F("Error reading humidity!"));
        return;
    }
    else
    {
        Serial0.print(F("Humidity: "));
        Serial0.print(event.relative_humidity);
        Serial0.println(F("%"));
    }
    String hum = String(event.relative_humidity, 1);
    StaticJsonDocument<200> json_payload;
    json_payload["equipmentToken"] = equipment_token;
    json_payload["temperature"] = temp;
    json_payload["humidity"] = hum;
    json_payload["creatTime"] = getLocalTime();
    String payload;
    serializeJson(json_payload, payload);

    client.publish(topic_temp_hum, payload.c_str());
}

void send_soilTemperatureHumidity()
{
    String temp = "";

    // Get humidity event and print its value.
    int sensoradc = analogRead(SOILPIN);
    double sensorValue = 0.0;
    if (sensoradc > 2200)
    {
        sensorValue = 0.0;
    }else if (sensoradc < 1600)
    {
        sensorValue = 100.0;
    }else
    {
        sensorValue = - 0.167 * sensoradc + 367.4;
    }
    String hum = String(sensorValue);
    StaticJsonDocument<200> json_payload;
    json_payload["equipmentToken"] = equipment_token;
    json_payload["soilTemperature"] = temp;
    json_payload["soilHumidity"] = hum;
    json_payload["creatTime"] = getLocalTime();
    String payload;
    serializeJson(json_payload, payload);

    client.publish(topic_soil, payload.c_str());
}

void send_gas()
{
    String gasCo2 = "0", gasLEL = "0", gasNH3 = "0";
    // gasCo2 = String(350 * (random(100) * 0.001 + 1));
    gasCo2 = String(getCO2());
    int gasLELValue = analogRead(LELPIN);
    // gasLEL = String(0.1 + (random(100) * 0.01));
    int gasNH3Value = analogRead(NH3PIN);
    gasNH3 = String(gasNH3Value);

    StaticJsonDocument<200> json_payload;
    json_payload["equipmentToken"] = equipment_token;
    json_payload["gasCo2"] = gasCo2; // 二氧化碳浓度
    json_payload["gasLEL"] = gasLEL; // 可燃气体浓度
    json_payload["gasNH3"] = gasNH3; // 氨气浓度
    json_payload["creatTime"] = getLocalTime();
    String payload;
    serializeJson(json_payload, payload);

    client.publish(topic_gas, payload.c_str());
}

void send_infrared(int state)
{
    StaticJsonDocument<200> json_payload;
    json_payload["equipmentToken"] = equipment_token;
    json_payload["infrared"] = state;
    json_payload["creatTime"] = getLocalTime();
    String payload;
    serializeJson(json_payload, payload);

    client.publish(topic_infrared, payload.c_str());
}

void infrared_callback()
{
    int now_state = !digitalRead(INFRAREDPIN);
    if (now_state == infrared_state)
    {
        return;
    }
    send_infrared(now_state);
    Serial0.printf("IR_state: %d\n", now_state);
    infrared_state = now_state;
}

void timerCallback()
{
    send_temperatureHumidity();
    send_soilTemperatureHumidity();
    send_gas();
    send_lightIntensity();
}

void send_lightIntensity()
{
    float lux = 0.0;
    if (lightMeter.measurementReady())
    {
        lux = lightMeter.readLightLevel();
        Serial0.print("Light: ");
        Serial0.print(lux);
        Serial0.println(" lx");
    }
    String intensity;
    intensity = String(lux);

    StaticJsonDocument<200> json_payload;
    json_payload["equipmentToken"] = equipment_token;
    json_payload["intensity"] = intensity;
    json_payload["creatTime"] = getLocalTime();
    String payload;
    serializeJson(json_payload, payload);

    client.publish(topic_lightIntensity, payload.c_str());
}

// int getLEL()
// {
//     int gasConcentration = 0;
//     byte highByte = 0, lowByte = 0;
//     Wire1.beginTransmission(0x54);
//     Wire1.write(0xa1);
//     Wire1.endTransmission();
//     delay(10);
//     Wire1.beginTransmission(0x55);
//     // 读取高字节气体浓度
//     if (Wire1.requestFrom(0x55, 1) == 1)
//     {
//         highByte = Wire1.read();
//     }

//     // 读取低字节气体浓度
//     if (Wire1.requestFrom(0x55, 1) == 1)
//     {
//         lowByte = Wire1.read();

//         // 结束信号
//         Wire1.endTransmission();

//         // 组合高字节和低字节，得到完整的数据
//         gasConcentration = (highByte << 8) | lowByte;

//         Serial0.print("LEL: ");
//         Serial0.println(gasConcentration);
//     }
//     return gasConcentration;
// }

// int getCO2()
// {
//     int concentration = 0;
//     Serial1.write(0xFF);
//     Serial1.write(0x01);
//     Serial1.write(0x86);
//     Serial1.write(0x00);
//     Serial1.write(0x00);
//     Serial1.write(0x00);
//     Serial1.write(0x00);
//     Serial1.write(0x00);
//     Serial1.write(0x79);

//     delay(200); // 等待传感器响应

//     if (Serial1.available() >= 9)
//     { // 检查是否有足够的数据可用
//         byte data[9];
//         Serial1.readBytes(data, 9); // 读取8个字节数据

//         // 解析数据
//         if (data[0] == 0xFF && data[1] == 0x86)
//         {
//             int highByte = data[2];
//             int lowByte = data[3];
//             concentration = (highByte << 8) | lowByte;

//             byte checksum = 0xFF - ((data[1] + data[2] + data[3] + data[4] + data[5] + data[6] + data[7]) & 0xFF) + 1;

//             if (checksum == data[8])
//             {
//                 Serial0.print("CO2: ");
//                 Serial0.println(concentration);
//             }
//             else
//             {
//                 Serial0.println("Checksum error!");
//             }
//         }
//     }
//     return concentration;
// }

int getCO2()
{
    s16 err = 0;
    u16 tvoc_ppb, co2_eq_ppm;
    err = sgp_measure_iaq_blocking_read(&tvoc_ppb, &co2_eq_ppm);
    if (err == STATUS_OK) {
        // Serial.print("tVOC  Concentration:");
        // Serial.print(tvoc_ppb);
        // Serial.println("ppb");

        Serial0.print("CO2eq Concentration:");
        Serial0.print(co2_eq_ppm);
        Serial0.println("ppm");
    } else {
        Serial0.println("error reading IAQ values\n");
    }
    // delay(1000);
    return co2_eq_ppm;
}