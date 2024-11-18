#ifndef PZEM_MODBUS_HPP
#define PZEM_MODBUS_HPP

#include <ModbusClientTCP.h>
#include <IPAddress.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Arduino.h>

namespace Node_Utility {

class ModbusManager {
public:
    struct PollingTarget {
        uint32_t token;
        IPAddress target_ip;
        uint8_t slave_id;
        Modbus::FunctionCode function_code;
        uint16_t start_address;
        uint16_t length;
    };
private:
    std::unique_ptr<ModbusClientTCP> MBClient;
    uint32_t timeout;
    uint32_t interval;
    std::vector<PollingTarget> targets;
    uint32_t currentToken;

    // Private constructor for singleton pattern
    ModbusManager(std::unique_ptr<ModbusClientTCP> MClient, uint32_t tm = 2000, uint32_t inter = 200)
        : MBClient(std::move(MClient)), timeout(tm), interval(inter), currentToken(0) {
        MBClient->onDataHandler([this](ModbusMessage response, uint32_t token) { this->handleData(response, token); });
        MBClient->onErrorHandler([this](Error error, uint32_t token) { this->handleError(error, token); });
        MBClient->setTimeout(timeout, interval);
        MBClient->begin(); // Start ModbusTCP background task
    }

    // Generate a unique token
    uint32_t generateUniqueToken() {
        return ++currentToken;
    }

    // Internal method to start the polling task
    void startPollingTask() {
        xTaskCreate(
            [](void* pvParameters) {
                static_cast<ModbusManager*>(pvParameters)->pollingTask();
            },
            "ModbusPollingTask",
            4096,   // Stack size
            this,   // Task parameter
            1,      // Priority
            nullptr // Task handle
        );
    }

    // Polling task implementation
    void pollingTask() {
        while (true) {
            for (const auto& target : targets) {
                MBClient->setTarget(target.target_ip, 502); // Set target IP and port
                
                Modbus::Error modbusError = MBClient->addRequest(
                    target.token,
                    target.slave_id,
                    target.function_code,
                    target.start_address,
                    target.length
                );

                if (modbusError != Modbus::SUCCESS) {
                    ModbusError e(modbusError);
                    Serial.printf("Error creating request for token %08X: %02X - %s\n", target.token, (int)e, (const char*)e);
                }

                vTaskDelay(pdMS_TO_TICKS(500)); // Delay between requests
            }

            vTaskDelay(pdMS_TO_TICKS(1000)); // Delay before the next polling cycle
        }
    }

    // Data and error handlers
    void handleData(ModbusMessage response, uint32_t token) {
        Serial.printf("Response: serverID=%d, FC=%d, Token=%08X, length=%d:\n", response.getServerID(), response.getFunctionCode(), token, response.size());
        for (auto& byte : response) {
            Serial.printf("%02X ", byte);
        }
        Serial.println("");
    }

    void handleError(Error error, uint32_t token) {
        ModbusError me(error);
        Serial.printf("Error response: %02X - %s\n", (int)me, (const char*)me);
    }

public:
    // Singleton access
    static ModbusManager& getInstance(std::unique_ptr<ModbusClientTCP> MClient = nullptr, uint32_t tm = 2000, uint32_t inter = 200) {
        static ModbusManager instance(std::move(MClient), tm, inter);
        return instance;
    }

    // Add autopolling targets
    template <typename... Args>
    void autopoll(bool startPolling, Args... targetsToAdd) {
        (addTarget(targetsToAdd), ...); // Add all targets
        if (startPolling) {
            startPollingTask();
        }
    }

private:
    // Add a target to the list
    void addTarget(PollingTarget target) {
        auto it = std::find_if(targets.begin(), targets.end(), [&](const PollingTarget& t) {
            return t.target_ip == target.target_ip && t.slave_id == target.slave_id &&
                   t.function_code == target.function_code && t.start_address == target.start_address &&
                   t.length == target.length;
        });

        if (it == targets.end()) {
            target.token = generateUniqueToken();
            targets.push_back(target);
        }
    }

    // Prevent copying
    ModbusManager(const ModbusManager&) = delete;
    ModbusManager& operator=(const ModbusManager&) = delete;
};

} // namespace Node_Utility

#endif // PZEM_MODBUS_HPP
