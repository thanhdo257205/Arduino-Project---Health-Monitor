package com.healthmonitor.servlet;

import com.google.gson.Gson;
import com.healthmonitor.model.HealthData;
import com.healthmonitor.serial.SerialReader;

import jakarta.servlet.annotation.WebServlet;
import jakarta.servlet.http.HttpServlet;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

@WebServlet("/api/data")
public class ApiDataServlet extends HttpServlet {

    private final Gson gson = new Gson();

    @Override
    protected void doGet(HttpServletRequest req, HttpServletResponse resp)
            throws IOException {

        resp.setContentType("application/json");
        resp.setCharacterEncoding("UTF-8");

        // Cho phép cross-origin (nếu cần)
        resp.setHeader("Access-Control-Allow-Origin", "*");
        resp.setHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        resp.setHeader("Pragma", "no-cache");

        SerialReader reader = SerialReader.getInstance();
        Map<String, Object> result = new HashMap<>();

        result.put("connected", reader.isConnected());
        result.put("measuring", reader.isMeasuring());
        result.put("connectionStatus", reader.getConnectionStatus());
        result.put("totalSamples", reader.getTotalSamples());
        result.put("sessionId", reader.getCurrentSessionId());
        result.put("lastRawLine", reader.getLastRawLine());

        HealthData latest = reader.getLatestSample();
        if (latest != null) {
            Map<String, Object> sampleMap = new HashMap<>();
            sampleMap.put("id", latest.getId());
            sampleMap.put("timestamp", latest.getTimestamp());
            sampleMap.put("elapsedMs", latest.getElapsedMs());
            sampleMap.put("temperature", latest.getTemperature());
            sampleMap.put("heartRate", latest.getHeartRate());
            sampleMap.put("spo2", latest.getSpo2());
            sampleMap.put("status", latest.getStatus());
            sampleMap.put("sessionId", latest.getSessionId());
            result.put("latestSample", sampleMap);
        }

        HealthData latestResult = reader.getLatestResult();
        if (latestResult != null) {
            Map<String, Object> resultMap = new HashMap<>();
            resultMap.put("temperature", latestResult.getTemperature());
            resultMap.put("heartRate", latestResult.getHeartRate());
            resultMap.put("spo2", latestResult.getSpo2());
            resultMap.put("status", latestResult.getStatus());
            result.put("latestResult", resultMap);
        }

        // Dữ liệu phiên hiện tại
        List<HealthData> sessionData = reader.getCurrentSessionData();
        result.put("currentSessionSize", sessionData.size());

        // Chuyển session data sang map để đảm bảo JSON đúng
        List<Map<String, Object>> sessionList = new java.util.ArrayList<>();
        for (HealthData d : sessionData) {
            Map<String, Object> m = new HashMap<>();
            m.put("id", d.getId());
            m.put("timestamp", d.getTimestamp());
            m.put("elapsedMs", d.getElapsedMs());
            m.put("temperature", d.getTemperature());
            m.put("heartRate", d.getHeartRate());
            m.put("spo2", d.getSpo2());
            m.put("status", d.getStatus());
            m.put("sessionId", d.getSessionId());
            sessionList.add(m);
        }
        result.put("currentSession", sessionList);

        // Toàn bộ dữ liệu
        List<HealthData> all = reader.getAllData();
        List<Map<String, Object>> allList = new java.util.ArrayList<>();
        for (HealthData d : all) {
            Map<String, Object> m = new HashMap<>();
            m.put("id", d.getId());
            m.put("timestamp", d.getTimestamp());
            m.put("elapsedMs", d.getElapsedMs());
            m.put("temperature", d.getTemperature());
            m.put("heartRate", d.getHeartRate());
            m.put("spo2", d.getSpo2());
            m.put("status", d.getStatus());
            m.put("sessionId", d.getSessionId());
            allList.add(m);
        }
        result.put("allData", allList);

        resp.getWriter().write(gson.toJson(result));
    }
}
