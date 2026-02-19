/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/Classes/Class.java to edit this template
 */
package com.healthmonitor.serial;

import com.fazecast.jSerialComm.SerialPort;
import com.healthmonitor.model.HealthData;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

public class SerialReader {

    private static SerialReader instance;
    private SerialPort serialPort;
    private Thread readerThread;
    private volatile boolean running = false;

    private final List<HealthData> allData = new CopyOnWriteArrayList<>();
    private final List<HealthData> currentSessionData = new CopyOnWriteArrayList<>();
    private volatile HealthData latestResult = null;

    private int currentSession = 0;
    private volatile boolean isMeasuring = false;
    private volatile String lastRawLine = "";
    private volatile String connectionStatus = "Chua ket noi";

    // ====== LOG để debug ======
    private final List<String> debugLog = new CopyOnWriteArrayList<>();
    private static final int MAX_LOG = 200;

    private SerialReader() {
    }

    public static synchronized SerialReader getInstance() {
        if (instance == null) {
            instance = new SerialReader();
        }
        return instance;
    }

    /**
     * Thêm log debug
     */
    private void log(String msg) {
        String entry = System.currentTimeMillis() + " | " + msg;
        System.out.println("[SerialReader] " + msg);
        debugLog.add(entry);
        if (debugLog.size() > MAX_LOG) {
            debugLog.remove(0);
        }
    }

    public List<String> getDebugLog() {
        return Collections.unmodifiableList(new ArrayList<>(debugLog));
    }

    /**
     * Liệt kê cổng COM
     */
    public static String[] getAvailablePorts() {
        SerialPort[] ports = SerialPort.getCommPorts();
        String[] names = new String[ports.length];
        for (int i = 0; i < ports.length; i++) {
            names[i] = ports[i].getSystemPortName();
        }
        return names;
    }

    public static String[] getPortDescriptions() {
        SerialPort[] ports = SerialPort.getCommPorts();
        String[] descs = new String[ports.length];
        for (int i = 0; i < ports.length; i++) {
            descs[i] = ports[i].getSystemPortName() + " - " + ports[i].getDescriptivePortName();
        }
        return descs;
    }

    /**
     * Kết nối đến cổng COM
     */
    public boolean connect(String portName) {
        try {
            disconnect();

            serialPort = SerialPort.getCommPort(portName);
            serialPort.setBaudRate(9600);
            serialPort.setComPortTimeouts(SerialPort.TIMEOUT_READ_SEMI_BLOCKING, 0, 0);

            if (serialPort.openPort()) {
                connectionStatus = "Da ket noi: " + portName;
                log("KET NOI THANH CONG: " + portName);
                startReading();
                return true;
            } else {
                connectionStatus = "Loi: Khong the mo " + portName;
                log("LOI KET NOI: Khong the mo " + portName);
                return false;
            }
        } catch (Exception e) {
            connectionStatus = "Loi: " + e.getMessage();
            log("LOI: " + e.getMessage());
            return false;
        }
    }

    /**
     * Ngắt kết nối
     */
    public void disconnect() {
        running = false;
        if (readerThread != null) {
            readerThread.interrupt();
        }
        if (serialPort != null && serialPort.isOpen()) {
            serialPort.closePort();
        }
        connectionStatus = "Da ngat ket noi";
        log("DA NGAT KET NOI");
    }

    /**
     * Đọc serial trên thread riêng
     */
    private void startReading() {
        running = true;
        readerThread = new Thread(() -> {
            log("Bat dau doc serial...");
            try (BufferedReader reader = new BufferedReader(
                    new InputStreamReader(serialPort.getInputStream()))) {
                String line;
                while (running && (line = reader.readLine()) != null) {
                    // Loại bỏ ký tự đặc biệt, khoảng trắng thừa
                    line = line.trim()
                            .replaceAll("[\\r\\n]", "")
                            .replaceAll("[^\\x20-\\x7E,\\.\\-]", ""); // Chỉ giữ ASCII

                    if (!line.isEmpty()) {
                        lastRawLine = line;
                        log("RAW: [" + line + "]");
                        parseLine(line);
                    }
                }
            } catch (Exception e) {
                if (running) {
                    connectionStatus = "Loi doc: " + e.getMessage();
                    log("LOI DOC SERIAL: " + e.getMessage());
                }
            }
            log("Thread doc serial da dung.");
        }, "SerialReaderThread");
        readerThread.setDaemon(true);
        readerThread.start();
    }

    /**
     * Parse dữ liệu từ Arduino.
     *
     * Arduino gửi: ">>> BAT DAU DO <<<"
     *   "Time,Temp,HR,SpO2"
     *   "1023,36.5,75,97"
     *   ">> Giai doan 1/3" "=== KET QUA ===" "Temp: 36.5C" "HR: 75 BPM" "SpO2:
     * 97%" "San sang..." "! !! DA HUY !!!"
     */
    private void parseLine(String line) {
        try {
            // ===== 1. Bắt đầu phiên đo =====
            if (line.contains("BAT DAU DO")) {
                currentSession = HealthData.nextSession();
                currentSessionData.clear();
                isMeasuring = true;
                log(">>> PHIEN DO #" + currentSession + " BAT DAU <<<");
                return;
            }

            // ===== 2. Kết thúc phiên đo =====
            if (line.contains("KET QUA")) {
                isMeasuring = false;
                log(">>> PHIEN DO #" + currentSession + " KET THUC, " + currentSessionData.size() + " mau <<<");
                // Tính kết quả trung bình
                calculateResult();
                return;
            }

            // ===== 3. Hủy đo =====
            if (line.contains("DA HUY")) {
                isMeasuring = false;
                currentSessionData.clear();
                log(">>> DA HUY DO <<<");
                return;
            }

            // ===== 4. Bỏ qua các dòng không phải dữ liệu =====
            if (line.startsWith("Time,Temp") || line.startsWith("Time,")
                    || line.startsWith("===") || line.startsWith("---")
                    || line.startsWith(">>") || line.startsWith("San sang")
                    || line.startsWith("LOI") || line.startsWith("Health")
                    || line.startsWith("Khoi dong")
                    || line.startsWith("Temp:") || line.startsWith("HR:")
                    || line.startsWith("SpO2:") || line.startsWith("Giai doan")
                    || line.startsWith("HE THONG") || line.startsWith("Dat ngon")
                    || line.startsWith("SAN SANG") || line.startsWith("Bat dau")
                    || line.startsWith("Giu yen") || line.startsWith("DA HUY")) {
                log("BO QUA: " + line);
                return;
            }

            // ===== 5. Parse dữ liệu mẫu: "elapsed,temp,hr,spo2" =====
            if (line.contains(",")) {
                String[] parts = line.split(",");
                if (parts.length >= 4) {
                    try {
                        long elapsed = Long.parseLong(parts[0].trim());
                        float temp = Float.parseFloat(parts[1].trim());
                        int hr = Integer.parseInt(parts[2].trim());
                        int spo2 = Integer.parseInt(parts[3].trim());

                        HealthData data = new HealthData(elapsed, temp, hr, spo2, currentSession);
                        allData.add(data);
                        currentSessionData.add(data);

                        // Đánh dấu đang đo nếu chưa (phòng trường hợp miss "BAT DAU DO")
                        if (!isMeasuring) {
                            isMeasuring = true;
                            if (currentSession == 0) {
                                currentSession = HealthData.nextSession();
                            }
                            log("TU DONG BAT DAU PHIEN DO #" + currentSession);
                        }

                        log("DU LIEU #" + currentSessionData.size()
                                + ": T=" + temp + ", HR=" + hr + ", SpO2=" + spo2
                                + " [elapsed=" + elapsed + "ms]");
                    } catch (NumberFormatException e) {
                        log("PARSE LOI (so): " + line + " -> " + e.getMessage());
                    }
                } else {
                    log("PARSE LOI (thieu cot): " + line + " [" + parts.length + " cot]");
                }
            } else {
                log("KHONG XU LY: " + line);
            }

        } catch (Exception e) {
            log("LOI PARSE: " + line + " -> " + e.getMessage());
        }
    }

    /**
     * Tính kết quả trung bình từ phiên đo hiện tại
     */
    private void calculateResult() {
        if (currentSessionData.isEmpty()) {
            log("Khong co du lieu de tinh ket qua");
            return;
        }

        float sumTemp = 0;
        int countTemp = 0;
        int sumHR = 0;
        int countHR = 0;
        int sumSpO2 = 0;
        int countSpO2 = 0;

        for (HealthData d : currentSessionData) {
            if (d.getTemperature() > 0) {
                sumTemp += d.getTemperature();
                countTemp++;
            }
            if (d.getHeartRate() > 0) {
                sumHR += d.getHeartRate();
                countHR++;
            }
            if (d.getSpo2() > 0) {
                sumSpO2 += d.getSpo2();
                countSpO2++;
            }
        }

        float avgTemp = countTemp > 0 ? sumTemp / countTemp : 0;
        int avgHR = countHR > 0 ? sumHR / countHR : 0;
        int avgSpO2 = countSpO2 > 0 ? sumSpO2 / countSpO2 : 0;

        if (countTemp > 0 || countHR > 0 || countSpO2 > 0) {
            latestResult = new HealthData(0, avgTemp, avgHR, avgSpO2, currentSession);
            log("KET QUA: T=" + avgTemp + "(" + countTemp + " mau), HR=" + avgHR
                    + "(" + countHR + " mau), SpO2=" + avgSpO2 + "(" + countSpO2 + " mau)");
        } else {
            log("Tat ca mau deu = 0, khong co ket qua");
        }
    }

    // === GETTERS ===
    public List<HealthData> getAllData() {
        return Collections.unmodifiableList(new ArrayList<>(allData));
    }

    public List<HealthData> getCurrentSessionData() {
        return Collections.unmodifiableList(new ArrayList<>(currentSessionData));
    }

    public HealthData getLatestResult() {
        return latestResult;
    }

    public HealthData getLatestSample() {
        return allData.isEmpty() ? null : allData.get(allData.size() - 1);
    }

    public boolean isMeasuring() {
        return isMeasuring;
    }

    public boolean isConnected() {
        return serialPort != null && serialPort.isOpen();
    }

    public String getConnectionStatus() {
        return connectionStatus;
    }

    public String getLastRawLine() {
        return lastRawLine;
    }

    public int getTotalSamples() {
        return allData.size();
    }

    public int getCurrentSessionId() {
        return currentSession;
    }

    public void clearAllData() {
        allData.clear();
        currentSessionData.clear();
        latestResult = null;
    }
}
