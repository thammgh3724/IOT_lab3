#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#ifdef ESP32
#include <WiFi.h>
#include <WiFiClientSecure.h>
#endif // ESP32
#endif // ESP8266
#include <random>
#include <Arduino_MQTT_Client.h>
#include <Shared_Attribute_Update.h>
#include <Server_Side_RPC.h>
#include <ThingsBoard.h>
#include "Wire.h"
#include "DHT20.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "time.h"

#include <OTA_Firmware_Update.h>
#include <Espressif_Updater.h>
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_event.h"
#include <esp_err.h>
#include "esp_log.h"
#include <cstdio>
#include "esp_sntp.h"

#include "nvs.h"
#include "nvs_flash.h"



#define ENCRYPTED false
#define THINGSBOARD_ENABLE_PROGMEM 0 
DHT20 dht20;
const char *WIFI_SSID = "QuocPhong";
const char *WIFI_PASSWORD = "quocphong2003";

const char *ntpServer = "pool.ntp.org";
constexpr char TOKEN[] = "VbZ7qInI8ZliEFmkPKYW";

constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";


#if ENCRYPTED
constexpr uint16_t THINGSBOARD_PORT = 8883U;
#else
constexpr uint16_t THINGSBOARD_PORT = 1883U;
#endif

constexpr uint16_t MAX_MESSAGE_SEND_SIZE = 512U;
constexpr uint16_t MAX_MESSAGE_RECEIVE_SIZE = 512U;

constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

#if ENCRYPTED
// See https://comodosslstore.com/resources/what-is-a-root-ca-certificate-and-how-do-i-download-it/
// on how to get the root certificate of the server we want to communicate with,
// this is needed to establish a secure connection and changes depending on the website.
constexpr char ROOT_CERT[] = R"(-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)";
#endif

constexpr const char RPC_JSON_METHOD[] = "example_json";
constexpr const char RPC_TEMPERATURE_METHOD[] = "example_set_temperature";
constexpr const char RPC_SWITCH_METHOD[] = "example_set_switch";
constexpr const char RPC_TEMPERATURE_KEY[] = "temp";
constexpr const char RPC_SWITCH_KEY[] = "switch";
constexpr uint8_t MAX_RPC_SUBSCRIPTIONS = 3U;
constexpr uint8_t MAX_RPC_RESPONSE = 5U;

constexpr size_t MAX_ATTRIBUTES = 6U;

char constexpr SCHEDULER[] = "scheduler";

// UPDATE OTA
constexpr char CURRENT_FIRMWARE_TITLE[] = "Test Update OTA";
constexpr char CURRENT_FIRMWARE_VERSION[] = "v1.0.0";

// Maximum amount of retries we attempt to download each firmware chunck over MQTT
constexpr uint8_t FIRMWARE_FAILURE_RETRIES = 12U;
// Size of each firmware chunck downloaded over MQTT,
// increased packet size, might increase download speed
constexpr uint16_t FIRMWARE_PACKET_SIZE = 10240U;


// Initialize underlying client, used to establish a connection
#if ENCRYPTED
WiFiClientSecure espClient;
#else
WiFiClient espClient;
#endif
// Initalize the Mqtt client instance
Arduino_MQTT_Client mqttClient(espClient);
// Initialize used apis
Shared_Attribute_Update<1U, MAX_ATTRIBUTES> shared_update;
Server_Side_RPC<MAX_RPC_SUBSCRIPTIONS, MAX_RPC_RESPONSE> rpc;
OTA_Firmware_Update<> ota;
const std::array<IAPI_Implementation *, 3U> apis = {
    &rpc,
    &shared_update,
    &ota};
// Initialize ThingsBoard instance with the maximum needed buffer size
ThingsBoard tb(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE, Default_Max_Stack_Size, apis);


Espressif_Updater<> updater;
bool currentFWSent = false;
bool updateRequestSent = false;

// Statuses for subscribing to rpc
bool subscribedRPC = false;
bool subscribedSharedAttributes = false;
/// @brief Initalizes WiFi connection,
// will endlessly delay until a connection has been successfully established
void InitWiFi()
{
  Serial.println("Connecting to AP ...");
  // Attempting to establish a connection to the given WiFi network
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    // Delay 500ms until a connection has been successfully established
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
#if ENCRYPTED
  espClient.setCACert(ROOT_CERT);
#endif
}

/// @brief Reconnects the WiFi uses InitWiFi if the connection has been removed
/// @return Returns true as soon as a connection has been established again
bool reconnect()
{
  // Check to ensure we aren't connected yet
  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED)
  {
    return true;
  }

  // If we aren't establish a new connection to the given WiFi network
  InitWiFi();
  return true;
}

/// @brief Processes function for RPC call "example_json"
/// JsonVariantConst is a JSON variant, that can be queried using operator[]
/// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
/// @param data Data containing the rpc data that was called and its current value
/// @param response Data containgin the response value, any number, string or json, that should be sent to the cloud. Useful for getMethods
void processGetJson(const JsonVariantConst &data, JsonDocument &response)
{
  Serial.println("Received the json RPC method");

  // Size of the response document needs to be configured to the size of the innerDoc + 1.
  StaticJsonDocument<JSON_OBJECT_SIZE(4)> innerDoc;
  innerDoc["string"] = "exampleResponseString";
  innerDoc["int"] = 5;
  innerDoc["float"] = 5.0f;
  innerDoc["bool"] = true;
  response["json_data"] = innerDoc;
}

/// @brief Processes function for RPC call "example_set_temperature"
/// JsonVariantConst is a JSON variant, that can be queried using operator[]
/// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
/// @param data Data containing the rpc data that was called and its current value
/// @param response Data containgin the response value, any number, string or json, that should be sent to the cloud. Useful for getMethods
void processTemperatureChange(const JsonVariantConst &data, JsonDocument &response)
{
  Serial.println("Received the set temperature RPC method");

  // Process data
  const float example_temperature = data[RPC_TEMPERATURE_KEY];

  Serial.print("Example temperature: ");
  Serial.println(example_temperature);

  // Ensure to only pass values do not store by copy, or if they do increase the MaxRPC template parameter accordingly to ensure that the value can be deserialized.RPC_Callback.
  // See https://arduinojson.org/v6/api/jsondocument/add/ for more information on which variables cause a copy to be created
  response["string"] = "exampleResponseString";
  response["int"] = 5;
  response["float"] = 5.0f;
  response["double"] = 10.0;
  response["bool"] = true;
}

/// @brief Processes function for RPC call "example_set_switch"
/// JsonVariantConst is a JSON variant, that can be queried using operator[]
/// See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
/// @param data Data containing the rpc data that was called and its current value
/// @param response Data containgin the response value, any number, string or json, that should be sent to the cloud. Useful for getMethods
void processSwitchChange(const JsonVariantConst &data, JsonDocument &response)
{
  bool state =  strcmp(data.as<String>().c_str(), "true") == 0 ? 1 : 0;
  digitalWrite(GPIO_NUM_48, state);
}

unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    // Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

void ledTask(void *pvParameters)
{
  uint32_t startTime = *((uint32_t *)pvParameters);
  uint32_t endTime = *(((uint32_t *)pvParameters) + 1);
  uint32_t currentTime = getTime(); // Convert to seconds
  // Calculate delay until start time
  if (currentTime < startTime)
  {
    vTaskDelay((startTime - currentTime) * 1000 / portTICK_PERIOD_MS);
  }
  digitalWrite(48, HIGH);
  Serial.println("LED turned ON");
  uint32_t delayTime = (endTime - startTime) * 1000; // Convert to milliseconds
  vTaskDelay(delayTime / portTICK_PERIOD_MS);
  digitalWrite(48, LOW);
  Serial.println("LED turned OFF");
  vTaskDelete(NULL);
}

void processSharedAttributeUpdate(const JsonObjectConst &data)
{
  int index = 0;
  uint32_t startTime = 0;
  uint32_t endTime = 0;
  for (auto it = data.begin(); it != data.end(); ++it)
  {
    Serial.println(it->value().as<String>());

    if (index == 1)
    {
      startTime = it->value().as<String>().toInt();
    }
    if (index == 2)
    {
      endTime = it->value().as<String>().toInt();
    }
    index++;
  }
  uint32_t *timeParams = new uint32_t[2]{startTime, endTime};
  xTaskCreate(ledTask, "LED Task", 2048, timeParams, 1, NULL);
}

void wifiTask(void *pvParameters)
{
  Serial.println("WIFI TAsk");
  while (true)
  {
    if (!reconnect())
    {
      return;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void update_starting_callback() {
  // Nothing to do
}

/// @brief End callback method that will be called as soon as the OTA firmware update, either finished successfully or failed.
/// Is meant to allow to either restart the device if the udpate was successfull or to restart any stopped services before the update started in the subscribed update_starting_callback
/// @param success Either true (update successful) or false (update failed)
void finished_callback(const bool & success) {
  if (success) {
    Serial.println("Done, Reboot now");
    esp_restart();
    return;
  }
  Serial.println("Downloading firmware failed");
}

void progress_callback(const size_t & current, const size_t & total) {
  Serial.printf("Progress %.2f%%\n", static_cast<float>(current * 100U) / total);
}

void thingsboardTask(void *pvParameters)
{
  while (true)
  {
    if (!tb.connected())
    {
      // Reconnect to the ThingsBoard server,
      // if a connection was disrupted or has not yet been established
      Serial.printf("Connecting to: (%s) with token (%s)\n", THINGSBOARD_SERVER, TOKEN);
      if (!tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT))
      {
        Serial.println("Failed to connect");
      }
      else{
        Serial.println("Connected to server");

      }
    }

    if (!subscribedRPC)
    {
      Serial.println("Subscribing for RPC...");
      const std::array<RPC_Callback, MAX_RPC_SUBSCRIPTIONS> callbacks = {
          // Requires additional memory in the JsonDocument for the JsonDocument that will be copied into the response
          RPC_Callback{RPC_JSON_METHOD, processGetJson},
          // Requires additional memory in the JsonDocument for 5 key-value pairs that do not copy their value into the JsonDocument itself
          RPC_Callback{RPC_TEMPERATURE_METHOD, processTemperatureChange},
          // Internal size can be 0, because if we use the JsonDocument as a JsonVariant and then set the value we do not require additional memory
          RPC_Callback{RPC_SWITCH_METHOD, processSwitchChange}};
      if (!rpc.RPC_Subscribe(callbacks.cbegin(), callbacks.cend()))
      {
        Serial.println("Failed to subscribe for RPC");
      }

      Serial.println("Subscribe done");
      subscribedRPC = true;
    }

    if (!subscribedSharedAttributes)
    {
      Serial.println("Subscribing for shared attribute updates...");
      // Shared attributes we want to request from the server
      constexpr std::array<const char *, MAX_ATTRIBUTES> SUBSCRIBED_SHARED_ATTRIBUTES = {SCHEDULER};
      const Shared_Attribute_Callback<MAX_ATTRIBUTES> callback(&processSharedAttributeUpdate, SUBSCRIBED_SHARED_ATTRIBUTES);
      if (!shared_update.Shared_Attributes_Subscribe(callback))
      {
        Serial.println("Failed to subscribe for shared attribute updates");
      }

      Serial.println("Subscribe done");
      subscribedSharedAttributes = true;
    }


    if (!currentFWSent) {
      currentFWSent = ota.Firmware_Send_Info(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION);
    }

    if (!updateRequestSent) {
      Serial.println("Firwmare Update Subscription...");
      const OTA_Update_Callback callback(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, (IUpdater *)&updater, &finished_callback, &progress_callback, &update_starting_callback, FIRMWARE_FAILURE_RETRIES, FIRMWARE_PACKET_SIZE, 30000000UL);
      // See https://thingsboard.io/docs/user-guide/ota-updates/
      // to understand how to create a new OTA pacakge and assign it to a device so it can download it.
      // Sending the request again after a successfull update will automatically send the UPDATED firmware state,
      // because the assigned firmware title and version on the cloud and the firmware version and title we booted into are the same.
      updateRequestSent = ota.Subscribe_Firmware_Update(callback);
    }


    tb.loop();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void dht20Task(void *pvParameters)
{
  Wire.begin(11, 12);
  dht20.begin();
  while (true)
  {
    dht20.read();
    tb.sendTelemetryData("temperature",dht20.getTemperature());
    tb.sendTelemetryData("humidity", dht20.getHumidity());
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

void blinkingLed(void *pvParameters){
  pinMode(48, OUTPUT);
  int ledState = 0;
  while(1){
    digitalWrite(48, 1 - ledState);
    ledState = 1 - ledState;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  // Initalize serial connection for debugging
  Serial.begin(SERIAL_DEBUG_BAUD);
  pinMode(48, OUTPUT);
  delay(1000);
  configTime(7 * 3600, 0, ntpServer);
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
  xTaskCreate(wifiTask, "wifi Task", 4096, NULL, 1, NULL);
  xTaskCreate(thingsboardTask, "Things Board Task", 8192, NULL, 2, NULL);
  xTaskCreate(dht20Task, "DHT Task", 8192, NULL, 3, NULL);
  // xTaskCreate(blinkingLed, "blink LED Task", 4096, NULL, 1, NULL);
}

void loop() {
}