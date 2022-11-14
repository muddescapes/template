#include <Arduino.h>
#include <ESPHelper.h>
#include <mqtt_client.h>

// Connect to Claremont-ETC wifi
constexpr const char *WIFI_NAME = "Claremont-ETC";
constexpr const char *WIFI_PASSWORD = "Cl@remontI0T";

// Connect to MQTT
constexpr const char *MQTT_BROKER = "mqtt://broker.hivemq.com";
constexpr const char *MQTT_TOPIC = "muddescapes-esp/test";

// Declare variables: 
// These will be set by input voltage to ESP:
bool led_status = false;
bool button_status = false;

// This is an internal variable for me to keep track of what should be happening
bool led_is_on = false;

// This is an internal variable allowing me to create a "latch" from a button
bool pressed = false;

// BEGIN TEMPLATE CODE (DO NOT CHANGE)

typedef void (*mqtt_callback_t)(esp_mqtt_event_handle_t);

void mqtt_cb_connected(esp_mqtt_event_handle_t event) {
    Serial.println("connected to MQTT broker");
    esp_mqtt_client_subscribe(event->client, MQTT_TOPIC, 0);
}

static void mqtt_cb_subscribed(esp_mqtt_event_handle_t event) {
    Serial.printf("subscribed to topic %.*s\n", event->topic_len, event->topic);
}

static void mqtt_cb_published(esp_mqtt_event_handle_t event) {
    ESP_LOGI(TAG, "published message with id %d", event->msg_id);
}

static void mqtt_cb_message(esp_mqtt_event_handle_t event) {
    if (event->data_len != event->total_data_len) {
        Serial.println("ignoring incomplete message (too long)");
        return;
    }
    Serial.printf("message arrived on topic %.*s: %.*s\n", event->topic_len, event->topic, event->data_len, event->data);
    
    // END TEMPLATE CODE

    // Here, we "listen" for certain messages and can act upon them. 
    // Example below: We listen for the string "test". If we hear it, we print to serial monitor "test message received" (internal, local)
    // and we publish the same message to MQTT (external, public)

    if (strncmp(event->data, "test", event->data_len) == 0) {
        const String resp("test message received");
        Serial.printf(resp.c_str());
        esp_mqtt_client_enqueue(event->client, MQTT_TOPIC, resp.c_str(), resp.length(), 0, 0, true);
    }

    // Here, if we receive this specific string that will be outputted from control-center, it overrides my local variables turn_on_led

    if (strncmp(event->data, "set led-status {on, off} status=on", event->data_len) == 0) {
        Serial.printf("Received command: led-status on");
        // esp_mqtt_client_publish(event->client, MQTT_TOPIC, "Received override led-status on", 0, 0, 0);
        led_is_on = true;
    }

    if (strncmp(event->data, "set led-status {on, off} status=off", event->data_len) == 0) {
        Serial.printf("Received command: led-status off");
        // esp_mqtt_client_publish(event->client, MQTT_TOPIC, "Received override led-status off", 0, 0, 0);
        led_is_on = false;
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    // https://github.com/jkhsjdhjs/esp32-mqtt-door-opener/blob/eee9e60e4f3a623913d470b4c7cbbc844300561d/main/src/mqtt.c
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
    reinterpret_cast<mqtt_callback_t>(handler_args)(event);
}

unsigned int msg_time = 0;
esp_mqtt_client_handle_t client = NULL;

void setup() {
    Serial.begin(115200);

    // CUSTOM CODE FOR YOUR PUZZLE BEGIN: 

    // For Arduinos, we have to specify which pins are inputs and outputs. Be sure to check the ESP-32 Huzzah Pinout to see which pins can be I/O and which are digital/analog

    pinMode(27,INPUT); // button input monitor
    pinMode(14,INPUT); // led status monitor
    pinMode(33, OUTPUT); // led lighter upper

    // CUSTOM CODE END

    setup_wifi(WIFI_NAME, WIFI_PASSWORD);

    // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html
    esp_mqtt_client_config_t mqtt_cfg = {0};
    mqtt_cfg.uri = MQTT_BROKER;

    client = esp_mqtt_client_init(&mqtt_cfg);
    assert(client);

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, MQTT_EVENT_CONNECTED, mqtt_event_handler, reinterpret_cast<void *>(mqtt_cb_connected)));
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, MQTT_EVENT_SUBSCRIBED, mqtt_event_handler, reinterpret_cast<void *>(mqtt_cb_subscribed)));
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, MQTT_EVENT_PUBLISHED, mqtt_event_handler, reinterpret_cast<void *>(mqtt_cb_published)));
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, MQTT_EVENT_DATA, mqtt_event_handler, reinterpret_cast<void *>(mqtt_cb_message)));

    ESP_ERROR_CHECK(esp_mqtt_client_start(client));

    msg_time = millis();
}

void loop() {
    
    // CUSTOM CODE FOR YOUR PUZZLE BEGIN: 
    // This is simply Arduino code. You can look up lots of tutorials for this stuff! 

    led_status = digitalRead(14);
    button_status = digitalRead(27);

    if (led_is_on) {
        digitalWrite(33, HIGH);
    }
    else {
        digitalWrite(33, LOW);
    }

    // Serial print for debugging

    // String s = "digitalRead(14) ";
    // s = s + digitalRead(14);
    // s = s + "      27 ";
    // s = s + digitalRead(27);
    // Serial.println(s);

    // Creation of message for MQTT. The format of an MQTT message must be: 
    // "variable-name {option1, option2} status=currentstatus"
    // in order for it to show up correctly on control center

    String message1 = "led-status {on, off} status=";
    if(led_status) {
        message1 = message1 + "on";
    }
    else {
        message1 = message1 + "off";
    }

    String message2 = "button_status {pressed, unpressed} status=";
    if(button_status) {
        message2 = message2 + "pressed";

        if (!pressed){
            led_is_on = !led_is_on; // toggle LED if button was pressed again (after being unpressed)
        }
        
        pressed = true;
    }
    else {
        message2 = message2 + "unpressed";

        pressed = false;
    }
    
    // put your main code here, to run repeatedly:
    if (millis() - msg_time > 500 && client) { //change the value here to change how often it publishes
        msg_time = millis();
        Serial.println("publishing message");

        // Sending our created MQTT strings to the internet. Copy the line below for as many messages as you have. 
        esp_mqtt_client_enqueue(client, MQTT_TOPIC, message1.c_str(), message1.length(), 0, 0, true);
        esp_mqtt_client_enqueue(client, MQTT_TOPIC, message2.c_str(), message2.length(), 0, 0, true);
    }

    // CUSTOM CODE END
}