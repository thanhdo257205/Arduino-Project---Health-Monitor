package com.healthmonitor.servlet;
/**
 *
 * @author dotha
 */
import com.healthmonitor.serial.SerialReader;

import jakarta.servlet.ServletException;
import jakarta.servlet.annotation.WebServlet;
import jakarta.servlet.http.HttpServlet;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import java.io.IOException;

@WebServlet(urlPatterns = {"/dashboard", "/connect", "/disconnect"})
public class DashboardServlet extends HttpServlet {

    @Override
    protected void doGet(HttpServletRequest req, HttpServletResponse resp)
            throws ServletException, IOException {

        req.setCharacterEncoding("UTF-8");
        SerialReader reader = SerialReader.getInstance();

        req.setAttribute("ports", SerialReader.getPortDescriptions());
        req.setAttribute("portNames", SerialReader.getAvailablePorts());
        req.setAttribute("isConnected", reader.isConnected());
        req.setAttribute("connectionStatus", reader.getConnectionStatus());
        req.setAttribute("isMeasuring", reader.isMeasuring());
        req.setAttribute("totalSamples", reader.getTotalSamples());
        req.setAttribute("allData", reader.getAllData());
        req.setAttribute("latestResult", reader.getLatestResult());

        req.getRequestDispatcher("/index.jsp").forward(req, resp);
    }

    @Override
    protected void doPost(HttpServletRequest req, HttpServletResponse resp)
            throws ServletException, IOException {

        req.setCharacterEncoding("UTF-8");
        String action = req.getServletPath();
        SerialReader reader = SerialReader.getInstance();

        if ("/connect".equals(action)) {
            String port = req.getParameter("port");
            if (port != null && !port.isEmpty()) {
                reader.connect(port);
            }
        } else if ("/disconnect".equals(action)) {
            reader.disconnect();
        }

        resp.sendRedirect(req.getContextPath() + "/dashboard");
    }
}