<%-- 
    Document   : index
    Created on : Feb 17, 2026, 9:34:51 PM
    Author     : dotha
--%>

<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<%@ page import="java.util.List" %>
<%@ page import="com.healthmonitor.model.HealthData" %>
<%@ page import="com.healthmonitor.serial.SerialReader" %>
<%
    boolean isConnected = (Boolean) request.getAttribute("isConnected");
    String connectionStatus = (String) request.getAttribute("connectionStatus");
    boolean isMeasuring = (Boolean) request.getAttribute("isMeasuring");
    int totalSamples = (Integer) request.getAttribute("totalSamples");
    String[] ports = (String[]) request.getAttribute("ports");
    String[] portNames = (String[]) request.getAttribute("portNames");
    List<HealthData> allData = (List<HealthData>) request.getAttribute("allData");
    HealthData latestResult = (HealthData) request.getAttribute("latestResult");
%>
<!DOCTYPE html>
<html lang="vi">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Hệ Thống Theo Dõi Sức Khỏe IoT</title>
        <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.1/dist/chart.umd.min.js"></script>
        <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700;800&display=swap" rel="stylesheet">
        <style>
            :root {
                --bg-primary: #0b0f19;
                --bg-secondary: #111827;
                --bg-card: #1a2236;
                --bg-card-hover: #1e2a45;
                --border: rgba(255,255,255,0.06);
                --text-primary: #f1f5f9;
                --text-secondary: #94a3b8;
                --text-muted: #64748b;
                --accent-blue: #3b82f6;
                --accent-cyan: #06b6d4;
                --accent-green: #10b981;
                --accent-red: #ef4444;
                --accent-yellow: #f59e0b;
                --accent-purple: #8b5cf6;
                --accent-orange: #f97316;
                --glow-blue: rgba(59,130,246,0.25);
                --glow-green: rgba(16,185,129,0.25);
                --glow-red: rgba(239,68,68,0.25);
                --radius: 16px;
                --radius-sm: 10px;
                --shadow: 0 4px 24px rgba(0,0,0,0.25);
            }

            * {
                margin:0;
                padding:0;
                box-sizing:border-box;
            }

            body {
                font-family: 'Inter', -apple-system, BlinkMacSystemFont, sans-serif;
                background: var(--bg-primary);
                color: var(--text-primary);
                min-height: 100vh;
            }

            /* ==================== SIDEBAR ==================== */
            .layout {
                display: flex;
                min-height: 100vh;
            }

            .sidebar {
                width: 264px;
                background: var(--bg-secondary);
                border-right: 1px solid var(--border);
                padding: 24px 16px;
                display: flex;
                flex-direction: column;
                position: fixed;
                height: 100vh;
                z-index: 100;
            }

            .logo {
                display: flex;
                align-items: center;
                gap: 14px;
                padding: 0 8px 24px 8px;
                border-bottom: 1px solid var(--border);
                margin-bottom: 24px;
            }

            /* SVG icon thay cho emoji */
            .logo-icon {
                width: 44px;
                height: 44px;
                background: linear-gradient(135deg, var(--accent-blue), var(--accent-cyan));
                border-radius: 12px;
                display: flex;
                align-items: center;
                justify-content: center;
                flex-shrink: 0;
            }
            .logo-icon svg {
                width: 24px;
                height: 24px;
                fill: #fff;
            }

            .logo-text {
                font-size: 16px;
                font-weight: 700;
                line-height: 1.3;
            }
            .logo-text span {
                color: var(--accent-cyan);
            }
            .logo-sub {
                font-size: 11px;
                color: var(--text-muted);
                font-weight: 400;
            }

            .nav-section {
                margin-bottom: 20px;
            }
            .nav-label {
                font-size: 10px;
                font-weight: 700;
                color: var(--text-muted);
                text-transform: uppercase;
                letter-spacing: 1.5px;
                padding: 0 12px;
                margin-bottom: 8px;
            }
            .nav-item {
                display: flex;
                align-items: center;
                gap: 12px;
                padding: 10px 12px;
                border-radius: var(--radius-sm);
                color: var(--text-secondary);
                font-size: 13px;
                font-weight: 500;
                cursor: pointer;
                transition: all 0.2s;
                text-decoration: none;
            }
            .nav-item:hover {
                background: rgba(255,255,255,0.05);
                color: var(--text-primary);
            }
            .nav-item.active {
                background: rgba(59,130,246,0.1);
                color: var(--accent-blue);
            }
            .nav-item svg {
                width: 18px;
                height: 18px;
                fill: currentColor;
                flex-shrink: 0;
            }

            /* Trạng thái kết nối trong sidebar */
            .status-indicator {
                display: flex;
                align-items: center;
                gap: 8px;
                padding: 10px 12px;
                font-size: 13px;
            }
            .status-dot {
                width: 8px;
                height: 8px;
                border-radius: 50%;
                flex-shrink: 0;
            }
            .dot-green {
                background: var(--accent-green);
                box-shadow: 0 0 8px var(--glow-green);
            }
            .dot-red {
                background: var(--accent-red);
                box-shadow: 0 0 8px var(--glow-red);
            }
            .dot-yellow {
                background: var(--accent-yellow);
                animation: pulse-dot 1.2s infinite;
            }
            @keyframes pulse-dot {
                0%,100%{
                    opacity:1;
                    transform:scale(1)
                }
                50%{
                    opacity:0.4;
                    transform:scale(0.85)
                }
            }

            /* Hộp kết nối COM */
            .connect-box {
                margin-top: auto;
                background: rgba(255,255,255,0.03);
                border: 1px solid var(--border);
                border-radius: var(--radius-sm);
                padding: 16px;
            }
            .connect-box label {
                font-size: 11px;
                color: var(--text-muted);
                font-weight: 700;
                text-transform: uppercase;
                letter-spacing: 0.8px;
                display: block;
                margin-bottom: 8px;
            }
            .connect-box select {
                width: 100%;
                padding: 9px 10px;
                background: var(--bg-primary);
                color: var(--text-primary);
                border: 1px solid var(--border);
                border-radius: 8px;
                font-size: 12px;
                font-family: inherit;
                margin-bottom: 10px;
                outline: none;
                transition: border-color 0.2s;
            }
            .connect-box select:focus {
                border-color: var(--accent-blue);
            }
            .connect-box select option {
                background: var(--bg-secondary);
            }
            .connect-info {
                font-size: 12px;
                color: var(--accent-green);
                margin-bottom: 10px;
                line-height: 1.5;
            }
            .btn-sm {
                width: 100%;
                padding: 10px;
                border: none;
                border-radius: 8px;
                font-size: 12px;
                font-weight: 600;
                font-family: inherit;
                cursor: pointer;
                transition: all 0.2s;
                display: flex;
                align-items: center;
                justify-content: center;
                gap: 6px;
            }
            .btn-sm svg {
                width: 14px;
                height: 14px;
                fill: currentColor;
            }
            .btn-connect-sm {
                background: var(--accent-green);
                color: #fff;
            }
            .btn-connect-sm:hover {
                background: #0ea572;
                box-shadow: 0 2px 12px var(--glow-green);
            }
            .btn-disconnect-sm {
                background: var(--accent-red);
                color: #fff;
            }
            .btn-disconnect-sm:hover {
                background: #dc2626;
                box-shadow: 0 2px 12px var(--glow-red);
            }

            /* ==================== NỘI DUNG CHÍNH ==================== */
            .main {
                flex: 1;
                margin-left: 264px;
                padding: 28px 32px;
            }

            /* Thanh trên cùng */
            .topbar {
                display: flex;
                justify-content: space-between;
                align-items: center;
                margin-bottom: 28px;
            }
            .topbar-left h2 {
                font-size: 22px;
                font-weight: 700;
            }
            .topbar-left p {
                font-size: 13px;
                color: var(--text-muted);
                margin-top: 3px;
            }
            .topbar-right {
                display: flex;
                gap: 10px;
                align-items: center;
            }

            .badge {
                display: inline-flex;
                align-items: center;
                gap: 6px;
                padding: 7px 14px;
                border-radius: 20px;
                font-size: 12px;
                font-weight: 600;
            }
            .badge-measuring {
                background: rgba(245,158,11,0.12);
                border: 1px solid rgba(245,158,11,0.25);
                color: var(--accent-yellow);
            }
            .badge-total {
                background: rgba(59,130,246,0.1);
                border: 1px solid rgba(59,130,246,0.2);
                color: var(--accent-blue);
            }
            .badge svg {
                width: 14px;
                height: 14px;
                fill: currentColor;
            }

            .btn-export {
                display: inline-flex;
                align-items: center;
                gap: 7px;
                padding: 9px 20px;
                border-radius: var(--radius-sm);
                background: linear-gradient(135deg, var(--accent-blue), var(--accent-purple));
                color: #fff;
                font-size: 13px;
                font-weight: 600;
                font-family: inherit;
                text-decoration: none;
                border: none;
                cursor: pointer;
                transition: all 0.2s;
            }
            .btn-export svg {
                width: 16px;
                height: 16px;
                fill: currentColor;
            }
            .btn-export:hover {
                transform: translateY(-1px);
                box-shadow: 0 4px 18px var(--glow-blue);
            }

            /* ==================== THẺ CHỈ SỐ ==================== */
            .stats-grid {
                display: grid;
                grid-template-columns: repeat(4, 1fr);
                gap: 18px;
                margin-bottom: 24px;
            }

            .stat-card {
                background: var(--bg-card);
                border: 1px solid var(--border);
                border-radius: var(--radius);
                padding: 22px 24px;
                position: relative;
                overflow: hidden;
                transition: all 0.3s;
            }
            .stat-card:hover {
                background: var(--bg-card-hover);
                transform: translateY(-2px);
                box-shadow: var(--shadow);
            }
            .stat-card::before {
                content: '';
                position: absolute;
                top: 0;
                left: 0;
                right: 0;
                height: 3px;
            }
            .stat-card.card-temp::before {
                background: linear-gradient(90deg, #f59e0b, #f97316);
            }
            .stat-card.card-hr::before {
                background: linear-gradient(90deg, #ef4444, #ec4899);
            }
            .stat-card.card-spo2::before {
                background: linear-gradient(90deg, #06b6d4, #3b82f6);
            }
            .stat-card.card-status::before {
                background: linear-gradient(90deg, #10b981, #06b6d4);
            }

            .stat-top {
                display: flex;
                justify-content: space-between;
                align-items: flex-start;
                margin-bottom: 16px;
            }
            .stat-label {
                font-size: 12px;
                font-weight: 600;
                color: var(--text-muted);
                text-transform: uppercase;
                letter-spacing: 0.5px;
            }

            /* Icon SVG trong thẻ */
            .stat-icon {
                width: 42px;
                height: 42px;
                border-radius: 12px;
                display: flex;
                align-items: center;
                justify-content: center;
            }
            .stat-icon svg {
                width: 22px;
                height: 22px;
            }
            .icon-temp {
                background: rgba(245,158,11,0.1);
            }
            .icon-temp svg {
                fill: var(--accent-yellow);
            }
            .icon-hr {
                background: rgba(239,68,68,0.1);
            }
            .icon-hr svg {
                fill: var(--accent-red);
            }
            .icon-spo2 {
                background: rgba(6,182,212,0.1);
            }
            .icon-spo2 svg {
                fill: var(--accent-cyan);
            }
            .icon-status {
                background: rgba(16,185,129,0.1);
            }
            .icon-status svg {
                fill: var(--accent-green);
            }

            /* Hiệu ứng nhịp tim đập */
            .heartbeat-icon {
                animation: heartbeat 1.2s ease-in-out infinite;
            }
            @keyframes heartbeat {
                0%,100% {
                    transform: scale(1);
                }
                14% {
                    transform: scale(1.2);
                }
                28% {
                    transform: scale(1);
                }
                42% {
                    transform: scale(1.12);
                }
                56% {
                    transform: scale(1);
                }
            }

            .stat-value {
                font-size: 34px;
                font-weight: 800;
                line-height: 1;
                margin-bottom: 2px;
            }
            .stat-unit {
                font-size: 15px;
                font-weight: 400;
                color: var(--text-muted);
            }
            .stat-range {
                font-size: 11px;
                color: var(--text-muted);
                margin-top: 10px;
                display: flex;
                align-items: center;
                gap: 4px;
            }
            .stat-range svg {
                width: 12px;
                height: 12px;
                fill: var(--accent-green);
                flex-shrink: 0;
            }

            .val-temp {
                color: var(--accent-yellow);
            }
            .val-hr {
                color: var(--accent-red);
            }
            .val-spo2 {
                color: var(--accent-cyan);
            }
            .val-ok {
                color: var(--accent-green);
            }
            .val-warn {
                color: var(--accent-yellow);
            }
            .val-danger {
                color: var(--accent-red);
            }

            /* ==================== BIỂU ĐỒ ==================== */
            .charts-grid {
                display: grid;
                grid-template-columns: 5fr 2fr;
                gap: 18px;
                margin-bottom: 24px;
            }

            .card {
                background: var(--bg-card);
                border: 1px solid var(--border);
                border-radius: var(--radius);
                padding: 22px 24px;
            }
            .card-title {
                font-size: 14px;
                font-weight: 600;
                margin-bottom: 18px;
                display: flex;
                align-items: center;
                gap: 10px;
            }
            .card-title svg {
                width: 16px;
                height: 16px;
                fill: var(--accent-cyan);
            }

            .live-tag {
                display: inline-flex;
                align-items: center;
                gap: 5px;
                padding: 3px 10px;
                border-radius: 12px;
                background: rgba(239,68,68,0.12);
                border: 1px solid rgba(239,68,68,0.25);
                font-size: 10px;
                font-weight: 700;
                color: var(--accent-red);
                text-transform: uppercase;
                letter-spacing: 0.5px;
            }
            .live-tag::before {
                content: '';
                width: 6px;
                height: 6px;
                border-radius: 50%;
                background: var(--accent-red);
                animation: pulse-dot 1s infinite;
            }

            /* Thông tin phiên đo */
            .session-list {
                display: flex;
                flex-direction: column;
                gap: 12px;
            }
            .session-item {
                background: rgba(255,255,255,0.025);
                border: 1px solid var(--border);
                border-radius: var(--radius-sm);
                padding: 14px 16px;
            }
            .si-label {
                font-size: 10px;
                font-weight: 700;
                color: var(--text-muted);
                text-transform: uppercase;
                letter-spacing: 1px;
                margin-bottom: 6px;
            }
            .si-value {
                font-size: 22px;
                font-weight: 700;
            }

            .progress-track {
                height: 6px;
                background: rgba(255,255,255,0.06);
                border-radius: 3px;
                overflow: hidden;
                margin-top: 10px;
            }
            .progress-fill {
                height: 100%;
                border-radius: 3px;
                background: linear-gradient(90deg, var(--accent-blue), var(--accent-cyan));
                transition: width 0.5s ease;
            }

            .result-row {
                display: flex;
                gap: 8px;
                flex-wrap: wrap;
                margin-top: 6px;
            }
            .result-chip {
                display: inline-flex;
                align-items: center;
                gap: 4px;
                padding: 4px 10px;
                border-radius: 8px;
                font-size: 11px;
                font-weight: 600;
                background: rgba(255,255,255,0.04);
                border: 1px solid var(--border);
            }
            .result-chip svg {
                width: 12px;
                height: 12px;
                fill: currentColor;
            }

            /* ==================== BẢNG DỮ LIỆU ==================== */
            .table-card {
                background: var(--bg-card);
                border: 1px solid var(--border);
                border-radius: var(--radius);
                overflow: hidden;
            }
            .table-top {
                display: flex;
                justify-content: space-between;
                align-items: center;
                padding: 18px 24px;
                border-bottom: 1px solid var(--border);
            }
            .table-top h3 {
                font-size: 14px;
                font-weight: 600;
                display: flex;
                align-items: center;
                gap: 8px;
            }
            .table-top h3 svg {
                width: 16px;
                height: 16px;
                fill: var(--accent-blue);
            }
            .table-count {
                font-size: 12px;
                color: var(--text-muted);
            }

            .table-scroll {
                max-height: 400px;
                overflow-y: auto;
            }
            .table-scroll::-webkit-scrollbar {
                width: 5px;
            }
            .table-scroll::-webkit-scrollbar-track {
                background: transparent;
            }
            .table-scroll::-webkit-scrollbar-thumb {
                background: rgba(255,255,255,0.08);
                border-radius: 3px;
            }

            table {
                width: 100%;
                border-collapse: collapse;
            }
            thead th {
                padding: 12px 16px;
                font-size: 11px;
                font-weight: 700;
                color: var(--text-muted);
                text-transform: uppercase;
                letter-spacing: 0.5px;
                text-align: left;
                background: rgba(255,255,255,0.02);
                position: sticky;
                top: 0;
                z-index: 1;
                border-bottom: 1px solid var(--border);
            }
            tbody td {
                padding: 11px 16px;
                font-size: 13px;
                border-bottom: 1px solid var(--border);
                color: var(--text-secondary);
            }
            tbody tr {
                transition: background 0.15s;
            }
            tbody tr:hover td {
                background: rgba(255,255,255,0.02);
            }
            tbody tr:last-child td {
                border-bottom: none;
            }

            .td-mono {
                font-family: 'SF Mono', 'Fira Code', 'Cascadia Code', monospace;
                font-size: 12px;
            }

            .status-pill {
                display: inline-flex;
                align-items: center;
                gap: 5px;
                padding: 4px 12px;
                border-radius: 14px;
                font-size: 10px;
                font-weight: 700;
                text-transform: uppercase;
                letter-spacing: 0.3px;
            }
            .status-pill svg {
                width: 10px;
                height: 10px;
                fill: currentColor;
            }
            .pill-ok {
                background: rgba(16,185,129,0.1);
                color: var(--accent-green);
                border: 1px solid rgba(16,185,129,0.2);
            }
            .pill-warn {
                background: rgba(245,158,11,0.1);
                color: var(--accent-yellow);
                border: 1px solid rgba(245,158,11,0.2);
            }
            .pill-danger {
                background: rgba(239,68,68,0.1);
                color: var(--accent-red);
                border: 1px solid rgba(239,68,68,0.2);
            }

            /* Bảng trống */
            .empty-state {
                text-align: center;
                padding: 48px 20px;
                color: var(--text-muted);
            }
            .empty-state svg {
                width: 48px;
                height: 48px;
                fill: var(--text-muted);
                opacity: 0.3;
                margin-bottom: 12px;
            }
            .empty-state p {
                font-size: 13px;
            }

            /* ==================== SERIAL MONITOR ==================== */
            .serial-bar {
                margin-top: 20px;
                background: var(--bg-card);
                border: 1px solid var(--border);
                border-radius: var(--radius-sm);
                padding: 12px 20px;
                display: flex;
                align-items: center;
                gap: 12px;
            }
            .serial-label {
                font-size: 10px;
                font-weight: 700;
                color: var(--accent-cyan);
                text-transform: uppercase;
                letter-spacing: 1.2px;
                white-space: nowrap;
                display: flex;
                align-items: center;
                gap: 6px;
            }
            .serial-label svg {
                width: 14px;
                height: 14px;
                fill: var(--accent-cyan);
            }
            .serial-data {
                font-family: 'SF Mono', 'Fira Code', 'Cascadia Code', 'Courier New', monospace;
                font-size: 12px;
                color: #22d3ee;
                flex: 1;
                overflow: hidden;
                text-overflow: ellipsis;
                white-space: nowrap;
            }

            /* ==================== RESPONSIVE ==================== */
            @media (max-width: 1200px) {
                .stats-grid {
                    grid-template-columns: repeat(2, 1fr);
                }
                .charts-grid {
                    grid-template-columns: 1fr;
                }
            }
            @media (max-width: 768px) {
                .sidebar {
                    display: none;
                }
                .main {
                    margin-left: 0;
                    padding: 16px;
                }
                .stats-grid {
                    grid-template-columns: 1fr 1fr;
                    gap: 10px;
                }
                .stat-value {
                    font-size: 26px;
                }
                .topbar {
                    flex-direction: column;
                    align-items: flex-start;
                    gap: 12px;
                }
            }
        </style>
    </head>
    <body>
        <div class="layout">

            <!-- ==================== SIDEBAR ==================== -->
            <aside class="sidebar">
                <div class="logo">
                    <div class="logo-icon">
                        <!-- SVG: Trái tim + sóng điện tim -->
                        <svg viewBox="0 0 24 24"><path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z"/></svg>
                    </div>
                    <div>
                        <div class="logo-text">Theo Dõi <span>Sức Khỏe</span></div>
                        <div class="logo-sub">Hệ thống IoT - Nhóm 5</div>
                    </div>
                </div>

                <div class="nav-section">
                    <div class="nav-label">Điều hướng</div>
                    <a class="nav-item active" href="#">
                        <!-- SVG: Dashboard/Grid -->
                        <svg viewBox="0 0 24 24"><path d="M3 3h8v8H3V3zm0 10h8v8H3v-8zm10-10h8v8h-8V3zm0 10h8v8h-8v-8z"/></svg>
                        Bảng điều khiển
                    </a>
                    <a class="nav-item" href="<%= request.getContextPath() %>/export">
                        <!-- SVG: Download/Export -->
                        <svg viewBox="0 0 24 24"><path d="M19 9h-4V3H9v6H5l7 7 7-7zM5 18v2h14v-2H5z"/></svg>
                        Xuất báo cáo Excel
                    </a>
                </div>

                <div class="nav-section">
                    <div class="nav-label">Trạng thái hệ thống</div>
                    <div class="status-indicator">
                        <span class="status-dot <%= isConnected ? (isMeasuring ? "dot-yellow" : "dot-green") : "dot-red" %>"
                              id="sideDot"></span>
                        <span id="sideStatus" style="font-size:13px; color:var(--text-secondary);">
                            <%= isConnected ? (isMeasuring ? "Đang đo lường..." : "Đã kết nối") : "Chưa kết nối" %>
                        </span>
                    </div>
                </div>

                <!-- Hộp kết nối -->
                <div class="connect-box">
                    <% if (!isConnected) { %>
                    <form action="<%= request.getContextPath() %>/connect" method="POST">
                        <label>Cổng kết nối</label>
                        <select name="port">
                            <% if (ports != null) {
                            for (int i = 0; i < ports.length; i++) { %>
                            <option value="<%= portNames[i] %>"><%= ports[i] %></option>
                            <%  } } %>
                            <% if (ports == null || ports.length == 0) { %>
                            <option value="">Không tìm thấy cổng COM</option>
                            <% } %>
                        </select>
                        <button type="submit" class="btn-sm btn-connect-sm">
                            <svg viewBox="0 0 24 24"><path d="M16 13h-3V3h-2v10H8l4 4 4-4zM4 19v2h16v-2H4z"/></svg>
                            Kết nối thiết bị
                        </button>
                    </form>
                    <% } else { %>
                    <form action="<%= request.getContextPath() %>/disconnect" method="POST">
                        <label>Thiết bị</label>
                        <div class="connect-info"><%= connectionStatus %></div>
                        <button type="submit" class="btn-sm btn-disconnect-sm">
                            <svg viewBox="0 0 24 24"><path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"/></svg>
                            Ngắt kết nối
                        </button>
                    </form>
                    <% } %>
                </div>
            </aside>

            <!-- ==================== NỘI DUNG CHÍNH ==================== -->
            <main class="main">

                <!-- Thanh trên -->
                <div class="topbar">
                    <div class="topbar-left">
                        <h2>Bảng Điều Khiển</h2>
                        <p>Giám sát sức khỏe theo thời gian thực qua cảm biến IoT</p>
                    </div>
                    <div class="topbar-right">
                        <div id="measuringBadge" class="badge badge-measuring"
                             style="display:<%= isMeasuring ? "inline-flex" : "none" %>">
                            <span class="status-dot dot-yellow"></span>
                            Đang đo lường...
                        </div>
                        <div class="badge badge-total">
                            <!-- SVG: Database -->
                            <svg viewBox="0 0 24 24"><path d="M12 3C7.58 3 4 4.79 4 7v10c0 2.21 3.58 4 8 4s8-1.79 8-4V7c0-2.21-3.58-4-8-4zm0 2c3.87 0 6 1.5 6 2s-2.13 2-6 2-6-1.5-6-2 2.13-2 6-2zM4 17v-2.34c1.52 1.1 4.28 1.84 8 1.84s6.48-.75 8-1.84V17c0 .5-2.13 2-6 2s-6-1.5-6-2z"/></svg>
                            <span id="totalSamples"><%= totalSamples %></span> mẫu dữ liệu
                        </div>
                        <a href="<%= request.getContextPath() %>/export" class="btn-export">
                            <svg viewBox="0 0 24 24"><path d="M19 9h-4V3H9v6H5l7 7 7-7zM5 18v2h14v-2H5z"/></svg>
                            Xuất Excel
                        </a>
                    </div>
                </div>

                <!-- ==================== THẺ CHỈ SỐ ==================== -->
                <div class="stats-grid">

                    <!-- Nhiệt độ -->
                    <div class="stat-card card-temp">
                        <div class="stat-top">
                            <span class="stat-label">Nhiệt độ cơ thể</span>
                            <div class="stat-icon icon-temp">
                                <svg viewBox="0 0 24 24"><path d="M15 13V5c0-1.66-1.34-3-3-3S9 3.34 9 5v8c-1.21.91-2 2.37-2 4 0 2.76 2.24 5 5 5s5-2.24 5-5c0-1.63-.79-3.09-2-4zm-4-8c0-.55.45-1 1-1s1 .45 1 1h-1v1h1v2h-1v1h1v2h-2V5z"/></svg>
                            </div>
                        </div>
                        <div class="stat-value val-temp">
                            <span id="tempVal">--</span>
                            <span class="stat-unit">&deg;C</span>
                        </div>
                        <div class="stat-range">
                            <svg viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/></svg>
                            Bình thường: 36.0 - 37.5&deg;C
                        </div>
                    </div>

                    <!-- Nhịp tim -->
                    <div class="stat-card card-hr">
                        <div class="stat-top">
                            <span class="stat-label">Nhịp tim</span>
                            <div class="stat-icon icon-hr heartbeat-icon">
                                <svg viewBox="0 0 24 24"><path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z"/></svg>
                            </div>
                        </div>
                        <div class="stat-value val-hr">
                            <span id="hrVal">--</span>
                            <span class="stat-unit">BPM</span>
                        </div>
                        <div class="stat-range">
                            <svg viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/></svg>
                            Bình thường: 60 - 100 nhịp/phút
                        </div>
                    </div>

                    <!-- SpO2 -->
                    <div class="stat-card card-spo2">
                        <div class="stat-top">
                            <span class="stat-label">Nồng độ oxy trong máu</span>
                            <div class="stat-icon icon-spo2">
                                <svg viewBox="0 0 24 24"><path d="M12 2c-5.33 4.55-8 8.48-8 11.8 0 4.98 3.8 8.2 8 8.2s8-3.22 8-8.2c0-3.32-2.67-7.25-8-11.8zm0 18c-3.35 0-6-2.57-6-6.2 0-2.34 1.95-5.44 6-9.14 4.05 3.7 6 6.79 6 9.14 0 3.63-2.65 6.2-6 6.2z"/></svg>
                            </div>
                        </div>
                        <div class="stat-value val-spo2">
                            <span id="spo2Val">--</span>
                            <span class="stat-unit">%</span>
                        </div>
                        <div class="stat-range">
                            <svg viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/></svg>
                            Bình thường: &ge; 95%
                        </div>
                    </div>

                    <!-- Trạng thái -->
                    <div class="stat-card card-status">
                        <div class="stat-top">
                            <span class="stat-label">Đánh giá sức khỏe</span>
                            <div class="stat-icon icon-status">
                                <svg viewBox="0 0 24 24"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-2 15l-5-5 1.41-1.41L10 14.17l7.59-7.59L19 8l-9 9z"/></svg>
                            </div>
                        </div>
                        <div class="stat-value" id="statusVal" style="font-size:20px; line-height:1.4;">
                            Chờ dữ liệu
                        </div>
                        <div class="stat-range" id="statusSub">
                            Đặt ngón tay lên cảm biến để bắt đầu
                        </div>
                    </div>
                </div>

                <!-- ==================== BIỂU ĐỒ ==================== -->
                <div class="charts-grid">

                    <!-- Biểu đồ chính -->
                    <div class="card">
                        <div class="card-title">
                            <!-- SVG: Chart line -->
                            <svg viewBox="0 0 24 24"><path d="M3.5 18.49l6-6.01 4 4L22 6.92l-1.41-1.41-7.09 7.97-4-4L2 16.99z"/></svg>
                            Biểu đồ theo dõi thời gian thực
                            <span class="live-tag">Trực tiếp</span>
                        </div>
                        <canvas id="liveChart" height="95"></canvas>
                    </div>

                    <!-- Thông tin phiên đo -->
                    <div class="card">
                        <div class="card-title">
                            <svg viewBox="0 0 24 24"><path d="M19 3H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm-5 14H7v-2h7v2zm3-4H7v-2h10v2zm0-4H7V7h10v2z"/></svg>
                            Thông tin phiên đo
                        </div>
                        <div class="session-list">
                            <div class="session-item">
                                <div class="si-label">Phiên đo hiện tại</div>
                                <div class="si-value" style="color:var(--accent-blue);" id="sessionId">#0</div>
                            </div>
                            <div class="session-item">
                                <div class="si-label">Số mẫu đã thu thập</div>
                                <div class="si-value" style="color:var(--accent-cyan);" id="sessionSamples">0</div>
                                <div class="progress-track">
                                    <div class="progress-fill" id="sessionProgress" style="width:0%"></div>
                                </div>
                            </div>
                            <div class="session-item">
                                <div class="si-label">Kết quả gần nhất</div>
                                <% if (latestResult != null) { %>
                                <div class="result-row">
                                    <span class="result-chip" style="color:var(--accent-yellow)">
                                        <svg viewBox="0 0 24 24"><path d="M15 13V5c0-1.66-1.34-3-3-3S9 3.34 9 5v8c-1.21.91-2 2.37-2 4 0 2.76 2.24 5 5 5s5-2.24 5-5c0-1.63-.79-3.09-2-4z"/></svg>
                                        <%= latestResult.getTemperature() %>&deg;C
                                    </span>
                                    <span class="result-chip" style="color:var(--accent-red)">
                                        <svg viewBox="0 0 24 24"><path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z"/></svg>
                                        <%= latestResult.getHeartRate() %> BPM
                                    </span>
                                    <span class="result-chip" style="color:var(--accent-cyan)">
                                        <svg viewBox="0 0 24 24"><path d="M12 2c-5.33 4.55-8 8.48-8 11.8 0 4.98 3.8 8.2 8 8.2s8-3.22 8-8.2c0-3.32-2.67-7.25-8-11.8z"/></svg>
                                        <%= latestResult.getSpo2() %>%
                                    </span>
                                </div>
                                <% } else { %>
                                <div style="font-size:12px; color:var(--text-muted); margin-top:6px;">
                                    Chưa có dữ liệu
                                </div>
                                <% } %>
                            </div>
                        </div>
                    </div>
                </div>

                <!-- ==================== BẢNG DỮ LIỆU ==================== -->
                <div class="table-card">
                    <div class="table-top">
                        <h3>
                            <svg viewBox="0 0 24 24"><path d="M3 3v18h18V3H3zm8 16H5v-6h6v6zm0-8H5V5h6v6zm8 8h-6v-6h6v6zm0-8h-6V5h6v6z"/></svg>
                            Lịch sử đo lường
                        </h3>
                        <span class="table-count">
                            Hiển thị <strong id="rowCount"><%= allData != null ? allData.size() : 0 %></strong> bản ghi
                        </span>
                    </div>
                    <div class="table-scroll">
                        <table>
                            <thead>
                                <tr>
                                    <th>STT</th>
                                    <th>Thời gian</th>
                                    <th>Phiên</th>
                                    <th>Thời điểm đo</th>
                                    <th>Nhiệt độ</th>
                                    <th>Nhịp tim</th>
                                    <th>SpO2</th>
                                    <th>Trạng thái</th>
                                </tr>
                            </thead>
                            <tbody id="dataBody">
                                <% if (allData != null && !allData.isEmpty()) {
                                    for (HealthData d : allData) {
                                        String pillCls = "OK".equals(d.getStatus()) ? "pill-ok"
                                            : "WARN".equals(d.getStatus()) ? "pill-warn" : "pill-danger";
                                        String pillText = "OK".equals(d.getStatus()) ? "Bình thường"
                                            : "WARN".equals(d.getStatus()) ? "Cảnh báo" : "Nguy hiểm";
                                        String pillSvg = "OK".equals(d.getStatus())
                                            ? "<svg viewBox='0 0 24 24'><path d='M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z'/></svg>"
                                            : "WARN".equals(d.getStatus())
                                            ? "<svg viewBox='0 0 24 24'><path d='M1 21h22L12 2 1 21zm12-3h-2v-2h2v2zm0-4h-2v-4h2v4z'/></svg>"
                                            : "<svg viewBox='0 0 24 24'><path d='M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-2h2v2zm0-4h-2V7h2v6z'/></svg>";
                                %>
                                <tr>
                                    <td class="td-mono"><%= d.getId() %></td>
                                    <td class="td-mono"><%= d.getTimestamp() %></td>
                                    <td><span style="color:var(--accent-blue)">#<%= d.getSessionId() %></span></td>
                                    <td class="td-mono"><%= d.getElapsedMs() %> ms</td>
                                    <td><%= d.getTemperature() %>&deg;C</td>
                                    <td><%= d.getHeartRate() %> nhịp/phút</td>
                                    <td><%= d.getSpo2() %>%</td>
                                    <td><span class="status-pill <%= pillCls %>"><%= pillSvg %> <%= pillText %></span></td>
                                </tr>
                                <% }
                        } else { %>
                                <tr>
                                    <td colspan="8">
                                        <div class="empty-state">
                                            <svg viewBox="0 0 24 24"><path d="M20 6h-8l-2-2H4c-1.1 0-1.99.9-1.99 2L2 18c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V8c0-1.1-.9-2-2-2zm0 12H4V8h16v10z"/></svg>
                                            <p>Chưa có dữ liệu. Kết nối thiết bị và đặt ngón tay lên cảm biến để bắt đầu đo.</p>
                                        </div>
                                    </td>
                                </tr>
                                <% } %>
                            </tbody>
                        </table>
                    </div>
                </div>

                <!-- ==================== SERIAL MONITOR ==================== -->
                <div class="serial-bar">
                    <span class="serial-label">
                        <svg viewBox="0 0 24 24"><path d="M20 4H4c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V6c0-1.1-.9-2-2-2zm0 14H4V8l8 5 8-5v10zm-8-7L4 6h16l-8 5z"/></svg>
                        Dữ liệu Serial
                    </span>
                    <span class="serial-data" id="rawLine">Đang chờ dữ liệu từ cảm biến...</span>
                </div>

            </main>
        </div>

        <!-- ==================== JAVASCRIPT ==================== -->
        <script>
    var ctx = '<%= request.getContextPath() %>';

    // ===== BIỂU ĐỒ =====
    var chartCtx = document.getElementById('liveChart').getContext('2d');
    var liveChart = new Chart(chartCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: 'Nhiệt độ (\u00B0C)',
                    data: [],
                    borderColor: '#f59e0b',
                    backgroundColor: 'rgba(245,158,11,0.06)',
                    borderWidth: 2, tension: 0.4, fill: true,
                    pointRadius: 3, pointBackgroundColor: '#f59e0b',
                    pointHoverRadius: 6
                },
                {
                    label: 'Nhịp tim (BPM)',
                    data: [],
                    borderColor: '#ef4444',
                    backgroundColor: 'rgba(239,68,68,0.06)',
                    borderWidth: 2, tension: 0.4, fill: true,
                    pointRadius: 3, pointBackgroundColor: '#ef4444',
                    pointHoverRadius: 6
                },
                {
                    label: 'SpO2 (%)',
                    data: [],
                    borderColor: '#06b6d4',
                    backgroundColor: 'rgba(6,182,212,0.06)',
                    borderWidth: 2, tension: 0.4, fill: true,
                    pointRadius: 3, pointBackgroundColor: '#06b6d4',
                    pointHoverRadius: 6
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: true,
            interaction: {mode: 'index', intersect: false},
            plugins: {
                legend: {
                    labels: {
                        color: '#94a3b8',
                        font: {size: 11, family: 'Inter'},
                        usePointStyle: true,
                        pointStyle: 'circle',
                        padding: 20
                    }
                },
                tooltip: {
                    backgroundColor: 'rgba(17,24,39,0.95)',
                    titleColor: '#f1f5f9',
                    bodyColor: '#94a3b8',
                    borderColor: 'rgba(255,255,255,0.1)',
                    borderWidth: 1,
                    cornerRadius: 8,
                    padding: 12
                }
            },
            scales: {
                x: {
                    grid: {color: 'rgba(255,255,255,0.03)'},
                    ticks: {color: '#64748b', font: {size: 10}}
                },
                y: {
                    grid: {color: 'rgba(255,255,255,0.03)'},
                    ticks: {color: '#64748b', font: {size: 10}}
                }
            }
        }
    });

    var chartDataCount = 0;

    // ===== CẬP NHẬT THỜI GIAN THỰC =====
    function updateData() {
        fetch(ctx + '/api/data')
                .then(function (r) {
                    return r.json();
                })
                .then(function (data) {
                    // Serial
                    if (data.lastRawLine) {
                        document.getElementById('rawLine').textContent = data.lastRawLine;
                    }
                    document.getElementById('totalSamples').textContent = data.totalSamples;

                    // Thẻ chỉ số
                    if (data.latestSample) {
                        var s = data.latestSample;
                        document.getElementById('tempVal').textContent = s.temperature.toFixed(1);
                        document.getElementById('hrVal').textContent = s.heartRate;
                        document.getElementById('spo2Val').textContent = s.spo2;

                        var stEl = document.getElementById('statusVal');
                        var stSub = document.getElementById('statusSub');
                        if (s.status === 'OK') {
                            stEl.innerHTML = '<span class="val-ok">Bình thường</span>';
                            stSub.textContent = 'Tất cả chỉ số nằm trong ngưỡng an toàn';
                        } else if (s.status === 'WARN') {
                            stEl.innerHTML = '<span class="val-warn">Cảnh báo</span>';
                            stSub.textContent = 'Có chỉ số nằm ngoài ngưỡng bình thường';
                        } else {
                            stEl.innerHTML = '<span class="val-danger">Nguy hiểm</span>';
                            stSub.textContent = 'Chỉ số bất thường - Cần kiểm tra ngay!';
                        }
                    }

                    // Badge đang đo
                    var mBadge = document.getElementById('measuringBadge');
                    mBadge.style.display = data.measuring ? 'inline-flex' : 'none';

                    // Sidebar
                    var sideStatus = document.getElementById('sideStatus');
                    var sideDot = document.getElementById('sideDot');
                    if (data.measuring) {
                        sideStatus.textContent = 'Đang đo lường...';
                        sideDot.className = 'status-dot dot-yellow';
                    } else if (data.connected) {
                        sideStatus.textContent = 'Đã kết nối';
                        sideDot.className = 'status-dot dot-green';
                    } else {
                        sideStatus.textContent = 'Chưa kết nối';
                        sideDot.className = 'status-dot dot-red';
                    }

                    // Phiên đo
                    document.getElementById('sessionId').textContent = '#' + data.sessionId;
                    var sLen = data.currentSession ? data.currentSession.length : 0;
                    document.getElementById('sessionSamples').textContent = sLen;
                    document.getElementById('sessionProgress').style.width =
                            Math.min(sLen / 20 * 100, 100) + '%';

                    // Biểu đồ
                    if (data.currentSession && data.currentSession.length > chartDataCount) {
                        for (var i = chartDataCount; i < data.currentSession.length; i++) {
                            var d = data.currentSession[i];
                            liveChart.data.labels.push((d.elapsedMs / 1000).toFixed(0) + ' giây');
                            liveChart.data.datasets[0].data.push(d.temperature);
                            liveChart.data.datasets[1].data.push(d.heartRate);
                            liveChart.data.datasets[2].data.push(d.spo2);
                        }
                        chartDataCount = data.currentSession.length;
                        liveChart.update('none');
                    }

                    // Reset biểu đồ khi phiên mới
                    if (data.currentSession && data.currentSession.length === 0 && chartDataCount > 0) {
                        liveChart.data.labels = [];
                        liveChart.data.datasets.forEach(function (ds) {
                            ds.data = [];
                        });
                        chartDataCount = 0;
                        liveChart.update('none');
                    }

                    // Bảng dữ liệu - thêm dòng mới
                    if (data.allData && data.allData.length > 0) {
                        var tbody = document.getElementById('dataBody');
                        // Xóa empty state nếu có
                        var emptyRow = tbody.querySelector('.empty-state');
                        if (emptyRow) {
                            emptyRow.closest('tr').remove();
                        }

                        var current = tbody.rows.length;
                        for (var i = current; i < data.allData.length; i++) {
                            var d = data.allData[i];
                            var tr = document.createElement('tr');
                            var pillCls, pillText, pillSvg;

                            if (d.status === 'OK') {
                                pillCls = 'pill-ok';
                                pillText = 'Bình thường';
                                pillSvg = "<svg viewBox='0 0 24 24'><path d='M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z'/></svg>";
                            } else if (d.status === 'WARN') {
                                pillCls = 'pill-warn';
                                pillText = 'Cảnh báo';
                                pillSvg = "<svg viewBox='0 0 24 24'><path d='M1 21h22L12 2 1 21zm12-3h-2v-2h2v2zm0-4h-2v-4h2v4z'/></svg>";
                            } else {
                                pillCls = 'pill-danger';
                                pillText = 'Nguy hiểm';
                                pillSvg = "<svg viewBox='0 0 24 24'><path d='M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-2h2v2zm0-4h-2V7h2v6z'/></svg>";
                            }

                            tr.innerHTML =
                                    '<td class="td-mono">' + d.id + '</td>' +
                                    '<td class="td-mono">' + d.timestamp + '</td>' +
                                    '<td><span style="color:var(--accent-blue)">#' + d.sessionId + '</span></td>' +
                                    '<td class="td-mono">' + d.elapsedMs + ' ms</td>' +
                                    '<td>' + d.temperature.toFixed(1) + '\u00B0C</td>' +
                                    '<td>' + d.heartRate + ' nhịp/phút</td>' +
                                    '<td>' + d.spo2 + '%</td>' +
                                    '<td><span class="status-pill ' + pillCls + '">' + pillSvg + ' ' + pillText + '</span></td>';
                            tbody.appendChild(tr);
                        }
                        document.getElementById('rowCount').textContent = data.allData.length;

                        // Tự cuộn xuống cuối
                        var wrap = document.querySelector('.table-scroll');
                        wrap.scrollTop = wrap.scrollHeight;
                    }
                })
                .catch(function (err) {
                    console.log('Lỗi kết nối:', err);
                });
    }

    // Cập nhật mỗi 1 giây
    setInterval(updateData, 1000);
        </script>

    </body>
</html>