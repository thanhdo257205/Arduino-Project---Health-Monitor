/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/Classes/Class.java to edit this template
 */
package com.healthmonitor.model;

/**
 *
 * @author dotha
 */
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

public class HealthData {

    private int id;
    private String timestamp;
    private long elapsedMs;
    private float temperature;
    private int heartRate;
    private int spo2;
    private String status;
    private int sessionId;

    private static int counter = 0;
    private static int sessionCounter = 0;

    public HealthData() {
        this.id = ++counter;
        this.timestamp = LocalDateTime.now()
                .format(DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss"));
    }

    public HealthData(long elapsedMs, float temperature, int heartRate, int spo2, int sessionId) {
        this();
        this.elapsedMs = elapsedMs;
        this.temperature = temperature;
        this.heartRate = heartRate;
        this.spo2 = spo2;
        this.sessionId = sessionId;
        this.status = calculateStatus();
    }

    private String calculateStatus() {
        if ((temperature > 0 && (temperature < 35.0f || temperature > 38.5f))
                || (heartRate > 0 && (heartRate < 50 || heartRate > 120))
                || (spo2 > 0 && spo2 < 90)) {
            return "DANGER";
        }
        if ((temperature > 0 && (temperature < 36.0f || temperature > 37.5f))
                || (heartRate > 0 && (heartRate < 60 || heartRate > 100))
                || (spo2 > 0 && spo2 < 95)) {
            return "WARN";
        }
        return "OK";
    }

    public static int nextSession() {
        return ++sessionCounter;
    }

    // === GETTERS ===
    public int getId() {
        return id;
    }

    public String getTimestamp() {
        return timestamp;
    }

    public long getElapsedMs() {
        return elapsedMs;
    }

    public float getTemperature() {
        return temperature;
    }

    public int getHeartRate() {
        return heartRate;
    }

    public int getSpo2() {
        return spo2;
    }

    public String getStatus() {
        return status;
    }

    public int getSessionId() {
        return sessionId;
    }
}
