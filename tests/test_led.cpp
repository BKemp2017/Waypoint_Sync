#include <wiringPi.h>
#include <iostream>


// Pin definiitions (WiringPi pin numbers)
#define POWER_LED_PIN 7            // GPIO 4 (Blue LED for Power)
#define LISTENING_LED_PIN 3        // GPIO 22 (Green LED for Listening)
#define TRANSMITTING_LED_PIN 0     // GPIO 17 (Yellow LED for Transmitting)
#define ERROR_LED_PIN 2            // GPIO 27 (Red LED for Error)

int main() {
    // Initialize WiringPi
    if (wiringPiSetup() == -1) {
        std::cerr << "WiringPi initialization failed" << std::endl;
        return 1;
    }

    // Set all the pins to output
    pinMode(POWER_LED_PIN, OUTPUT);
    pinMode(LISTENING_LED_PIN, OUTPUT);
    pinMode(TRANSMITTING_LED_PIN, OUTPUT);
    pinMode(ERROR_LED_PIN, OUTPUT);

    // Set all the LEDs to LOW (off)
    digitalWrite(POWER_LED_PIN, LOW);
    digitalWrite(LISTENING_LED_PIN, LOW);
    digitalWrite(TRANSMITTING_LED_PIN, LOW);
    digitalWrite(ERROR_LED_PIN, LOW);

    // Display status to console
    std::cout << "All LEDs initialized and set to OFF state." << std::endl;

    // Blink each LED in sequence
    for (int i=0; i < 5; i++) {

        // Turn on Error LED
        std::cout << "Error LED ON" << std::endl;
        digitalWrite(ERROR_LED_PIN, HIGH);
        delay(500);
        digitalWrite(ERROR_LED_PIN, LOW);
        delay(500);


        // Turn on Power LED
        std::cout << "Power LED ON" << std::endl;
        digitalWrite(POWER_LED_PIN, HIGH);
        delay(500);
        digitalWrite(POWER_LED_PIN, LOW);
        delay(500);


        // Turn on Listening LED
        std::cout << "Listening LED ON" << std::endl;
        digitalWrite(LISTENING_LED_PIN, HIGH);
        delay(500);
        digitalWrite(LISTENING_LED_PIN, LOW);
        delay(500);

        // Turn on Transmitting LED
        std::cout << "Transmitting LED ON" << std::endl;
        digitalWrite(TRANSMITTING_LED_PIN, HIGH);
        delay(500);
        digitalWrite(TRANSMITTING_LED_PIN, LOW);
        delay(500);



    }

    return 0;

}