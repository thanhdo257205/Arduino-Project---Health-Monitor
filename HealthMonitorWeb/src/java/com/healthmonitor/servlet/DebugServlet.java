package com.healthmonitor.servlet;

import com.healthmonitor.serial.SerialReader;

import jakarta.servlet.annotation.WebServlet;
import jakarta.servlet.http.HttpServlet;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.List;

@WebServlet("/debug")
public class DebugServlet extends HttpServlet {

    @Override
    protected void doGet(HttpServletRequest req, HttpServletResponse resp)
            throws IOException {

        resp.setContentType("text/html; charset=UTF-8");
        PrintWriter out = resp.getWriter();

        SerialReader reader = SerialReader.getInstance();
        List<String> logs = reader.getDebugLog();

        out.println("<!DOCTYPE html><html><head>");
        out.println("<meta charset='UTF-8'>");
        out.println("<title>Debug Serial</title>");
        out.println("<meta http-equiv='refresh' content='2'>"); // Tự reload mỗi 2 giây
        out.println("<style>");
        out.println("body{background:#111;color:#0f0;font-family:'Courier New',monospace;font-size:13px;padding:20px;}");
        out.println("h2{color:#0ff;} .info{color:#ff0;} .err{color:#f00;} .data{color:#0f0;font-weight:bold;}");
        out.println(".status{background:#222;padding:15px;border-radius:8px;margin-bottom:15px;border:1px solid #333;}");
        out.println(".log-box{background:#0a0a0a;padding:15px;border-radius:8px;border:1px solid #222;max-height:600px;overflow-y:auto;}");
        out.println(".log-line{padding:2px 0;border-bottom:1px solid #1a1a1a;}");
        out.println("</style></head><body>");

        out.println("<h2>DEBUG SERIAL MONITOR</h2>");

        // Trạng thái
        out.println("<div class='status'>");
        out.println("<div class='info'>Ket noi: " + reader.getConnectionStatus() + "</div>");
        out.println("<div class='info'>isConnected: " + reader.isConnected() + "</div>");
        out.println("<div class='info'>isMeasuring: " + reader.isMeasuring() + "</div>");
        out.println("<div class='info'>Tong mau: " + reader.getTotalSamples() + "</div>");
        out.println("<div class='info'>Phien hien tai: #" + reader.getCurrentSessionId() + "</div>");
        out.println("<div class='info'>Mau trong phien: " + reader.getCurrentSessionData().size() + "</div>");
        out.println("<div class='data'>Last raw: " + escapeHtml(reader.getLastRawLine()) + "</div>");

        if (reader.getLatestSample() != null) {
            var s = reader.getLatestSample();
            out.println("<div class='data'>Latest: T=" + s.getTemperature()
                    + " HR=" + s.getHeartRate() + " SpO2=" + s.getSpo2()
                    + " [" + s.getStatus() + "]</div>");
        }
        out.println("</div>");

        // Log
        out.println("<h2>LOG (" + logs.size() + " dong)</h2>");
        out.println("<div class='log-box'>");

        // Hiện log mới nhất ở trên
        for (int i = logs.size() - 1; i >= 0; i--) {
            String log = escapeHtml(logs.get(i));
            String cls = "log-line";
            if (log.contains("LOI") || log.contains("ERR")) {
                cls += " err";
            } else if (log.contains("DU LIEU") || log.contains("KET QUA")) {
                cls += " data";
            } else if (log.contains("RAW")) {
                cls += " info";
            }
            out.println("<div class='" + cls + "'>" + log + "</div>");
        }

        if (logs.isEmpty()) {
            out.println("<div class='info'>Chua co log. Ket noi Arduino va thu do.</div>");
        }

        out.println("</div>");
        out.println("<br><div style='color:#666'>Trang tu dong reload moi 2 giay</div>");
        out.println("</body></html>");
    }

    private String escapeHtml(String s) {
        if (s == null) {
            return "";
        }
        return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;");
    }
}
