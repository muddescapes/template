// This ESP32 takes care of the servos controlling the door to the escape pod. 

#include <Arduino.h>
#include <ESPHelper.h>
#include <mqtt_client.h>
#include <ESP32Servo.h>

constexpr const char *WIFI_NAME = "Claremont-ETC";
constexpr const char *WIFI_PASSWORD = "cityoftrees87";
constexpr const char *MQTT_BROKER = "mqtt://mqtt.eclipseprojects.io";
constexpr const char *MQTT_TOPIC = "muddescapes-esp/varmon";
constexpr const char *MQTT_TOPIC_MANUAL = "muddescapes-esp/manual";

Servo servo1; // "navigation"
Servo servo2; // "pressurize"
Servo servo3; // "power"

bool servo1_status = false;
bool servo2_status = false;
bool servo3_status = false;

// keeps track of whether or not these buttons have ever been pressed
bool button1 = false;
bool button2 = false;
bool button3 = false;

const int locked_angle = 0;
const int unlocked_angle = 90;

void lock_servos() {
    servo1.write(locked_angle);
    servo2.write(locked_angle);
    servo3.write(locked_angle);

    servo1_status = false;
    servo2_status = false;
    servo3_status = false;
}

void unlock_servos() {
    servo1.write(unlocked_angle);
    servo2.write(unlocked_angle);
    servo3.write(unlocked_angle);

    servo1_status = true;
    servo2_status = true;
    servo3_status = true;
}

void lock_servo(int servo_num) {
    if (servo_num == 1) {
        servo1.write(locked_angle);
        servo1_status = false;
    }
    if (servo_num == 2) {
        servo2.write(locked_angle);
        servo2_status = false;
    }
    if (servo_num == 3) {
        servo3.write(locked_angle);
        servo3_status = false;
    }
}

void unlock_servo(int servo_num) {
    if (servo_num == 1) {
        servo1.write(unlocked_angle);
        servo1_status = true;
    }
    if (servo_num == 2) {
        servo2.write(unlocked_angle);
        servo2_status = true;
    }
    if (servo_num == 3) {
        servo3.write(unlocked_angle);
        servo3_status = true;
    }
}

// resets button variables, not servos. use lock_servos to reset those
void reset() {
    button1 = false;
    button2 = false;
    button3 = false;
}

typedef void (*mqtt_callback_t)(esp_mqtt_event_handle_t);

void mqtt_cb_connected(esp_mqtt_event_handle_t event) {
    Serial.println("connected to MQTT broker");
    esp_mqtt_client_subscribe(event->client, MQTT_TOPIC, 0);
    esp_mqtt_client_subscribe(event->client, MQTT_TOPIC_MANUAL, 0);
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

    if (strncmp(event->data, "buttcheek", event->data_len) == 0) {
        Serial.printf("don't you mean asscheek");
        esp_mqtt_client_publish(event->client, MQTT_TOPIC, "ill squeeze that", 0, 0, 0);
        digitalWrite(GPIO_NUM_27,HIGH);
    }

    if (strncmp(event->data, "nut", event->data_len) == 0) {
        Serial.printf("don't you mean cashew");
        esp_mqtt_client_publish(event->client, MQTT_TOPIC, "ill milk that", 0, 0, 0);
        digitalWrite(GPIO_NUM_27,LOW);
    }

    if (strncmp(event->data, "unlock servos override", event->data_len) == 0) {
        esp_mqtt_client_publish(event->client, MQTT_TOPIC_MANUAL, "[Control Panel] Overriding servos - UNLOCK", 0, 0, 0);
        unlock_servos();
    }

    if (strncmp(event->data, "lock servos override", event->data_len) == 0) {
        esp_mqtt_client_publish(event->client, MQTT_TOPIC_MANUAL, "[Control Panel] Overriding servos - LOCK", 0, 0, 0);
        lock_servos();
    }


    if (strncmp(event->data, "unlock servo1 override", event->data_len) == 0) {
        esp_mqtt_client_publish(event->client, MQTT_TOPIC_MANUAL, "[Control Panel] Overriding servo1 - UNLOCK", 0, 0, 0);
        unlock_servo(1);
    }
    if (strncmp(event->data, "unlock servo2 override", event->data_len) == 0) {
        esp_mqtt_client_publish(event->client, MQTT_TOPIC_MANUAL, "[Control Panel] Overriding servo2 - UNLOCK", 0, 0, 0);
        unlock_servo(2);
    }
    if (strncmp(event->data, "unlock servo3 override", event->data_len) == 0) {
        esp_mqtt_client_publish(event->client, MQTT_TOPIC_MANUAL, "[Control Panel] Overriding servo3 - UNLOCK", 0, 0, 0);
        unlock_servo(3);
    }

    if (strncmp(event->data, "lock servo1 override", event->data_len) == 0) {
        esp_mqtt_client_publish(event->client, MQTT_TOPIC_MANUAL, "[Control Panel] Overriding servo1 - LOCK", 0, 0, 0);
        lock_servo(1);
    }
    if (strncmp(event->data, "lock servo2 override", event->data_len) == 0) {
        esp_mqtt_client_publish(event->client, MQTT_TOPIC_MANUAL, "[Control Panel] Overriding servo2 - LOCK", 0, 0, 0);
        lock_servo(2);
    }
    if (strncmp(event->data, "lock servo3 override", event->data_len) == 0) {
        esp_mqtt_client_publish(event->client, MQTT_TOPIC_MANUAL, "[Control Panel] Overriding servo3 - LOCK", 0, 0, 0);
        lock_servo(3);
    }

    if (strncmp(event->data, "reset control panel", event->data_len) == 0) {
        esp_mqtt_client_publish(event->client, MQTT_TOPIC_MANUAL, "[Control Panel] Resetting control panel", 0, 0, 0);
        reset();
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

    pinMode(21,INPUT_PULLUP);
    pinMode(27,INPUT_PULLUP);
    pinMode(15,INPUT_PULLUP);

    servo1.attach(26);
    servo2.attach(25);
    servo3.attach(4); 
    


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
    
    // put your main code here, to run repeatedly:

    if(digitalRead(21)==0) {
        button1 = true;
        servo1.write(unlocked_angle);
        Serial.println("21");
    }
    if(digitalRead(27)==0) {
        button2 = true;
        servo2.write(unlocked_angle);

        Serial.println("27");

    }
    if (digitalRead(15)==0) {
        button3 = true;
        Serial.println("15");
        servo3.write(unlocked_angle);

    }

    if (button1) {
        servo1.write(unlocked_angle);
        servo1_status = true;
    }
    else {
        servo1.write(locked_angle);
    }

    if (button2) {
        servo2.write(unlocked_angle);
        servo2_status = true;
    }
    else {
        servo2.write(locked_angle);
    }

    if (button3) {
        servo3.write(unlocked_angle);
        servo3_status = true;
    }
    else {
        servo3.write(locked_angle);
    }

    if (millis() - msg_time > 1000 && client) {
        msg_time = millis();
        Serial.println("publishing message");

        String message = "button1: ";
        message += button1;
        esp_mqtt_client_publish(client, MQTT_TOPIC, message.c_str(), 0, 0, 0);

        message = "button2: ";
        message += button2;
        esp_mqtt_client_publish(client, MQTT_TOPIC, message.c_str(), 0, 0, 0);

        message = "button3: ";
        message += button3;
        esp_mqtt_client_publish(client, MQTT_TOPIC, message.c_str(), 0, 0, 0);

        message = "servo1_status: ";
        message += servo1_status;
        esp_mqtt_client_publish(client, MQTT_TOPIC, message.c_str(), 0, 0, 0);

        message = "servo2_status: ";
        message += servo2_status;
        esp_mqtt_client_publish(client, MQTT_TOPIC, message.c_str(), 0, 0, 0);

        message = "servo3_status: ";
        message += servo3_status;
        esp_mqtt_client_publish(client, MQTT_TOPIC, message.c_str(), 0, 0, 0);
    }
}