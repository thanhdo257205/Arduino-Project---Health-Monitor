package com.healthmonitor.servlet;

/**
 *
 * @author dotha
 */
import com.healthmonitor.model.HealthData;
import com.healthmonitor.serial.SerialReader;
import org.apache.poi.ss.usermodel.*;
import org.apache.poi.ss.util.CellRangeAddress;
import org.apache.poi.xssf.usermodel.XSSFWorkbook;

import jakarta.servlet.annotation.WebServlet;
import jakarta.servlet.http.HttpServlet;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

@WebServlet("/export")
public class ExportExcelServlet extends HttpServlet {

    @Override
    protected void doGet(HttpServletRequest req, HttpServletResponse resp)
            throws IOException {

        List<HealthData> allData = SerialReader.getInstance().getAllData();

        // ====== LỌC: Chỉ lấy mẫu hợp lệ (không phải 0) ======
        List<HealthData> validData = new ArrayList<>();
        for (HealthData d : allData) {
            if (d.getTemperature() > 0 || d.getHeartRate() > 0 || d.getSpo2() > 0) {
                validData.add(d);
            }
        }

        try (Workbook workbook = new XSSFWorkbook()) {

            // ========== Sheet 1: Dữ liệu chi tiết ==========
            Sheet sheet = workbook.createSheet("Du Lieu Do");

            // --- Style header ---
            CellStyle headerStyle = workbook.createCellStyle();
            Font headerFont = workbook.createFont();
            headerFont.setBold(true);
            headerFont.setFontHeightInPoints((short) 11);
            headerFont.setColor(IndexedColors.WHITE.getIndex());
            headerStyle.setFont(headerFont);
            headerStyle.setFillForegroundColor(IndexedColors.DARK_BLUE.getIndex());
            headerStyle.setFillPattern(FillPatternType.SOLID_FOREGROUND);
            headerStyle.setAlignment(HorizontalAlignment.CENTER);
            setBorders(headerStyle);

            // --- Style data ---
            CellStyle dataStyle = workbook.createCellStyle();
            dataStyle.setAlignment(HorizontalAlignment.CENTER);
            setBorders(dataStyle);

            // --- Style WARN ---
            CellStyle warnStyle = workbook.createCellStyle();
            warnStyle.cloneStyleFrom(dataStyle);
            warnStyle.setFillForegroundColor(IndexedColors.LIGHT_YELLOW.getIndex());
            warnStyle.setFillPattern(FillPatternType.SOLID_FOREGROUND);

            // --- Style DANGER ---
            CellStyle dangerStyle = workbook.createCellStyle();
            dangerStyle.cloneStyleFrom(dataStyle);
            dangerStyle.setFillForegroundColor(IndexedColors.CORAL.getIndex());
            dangerStyle.setFillPattern(FillPatternType.SOLID_FOREGROUND);
            Font dangerFont = workbook.createFont();
            dangerFont.setBold(true);
            dangerFont.setColor(IndexedColors.DARK_RED.getIndex());
            dangerStyle.setFont(dangerFont);

            // --- Style số thập phân (1 chữ số) ---
            CellStyle tempDataStyle = workbook.createCellStyle();
            tempDataStyle.cloneStyleFrom(dataStyle);
            tempDataStyle.setDataFormat(workbook.createDataFormat().getFormat("0.0"));

            CellStyle tempWarnStyle = workbook.createCellStyle();
            tempWarnStyle.cloneStyleFrom(warnStyle);
            tempWarnStyle.setDataFormat(workbook.createDataFormat().getFormat("0.0"));

            CellStyle tempDangerStyle = workbook.createCellStyle();
            tempDangerStyle.cloneStyleFrom(dangerStyle);
            tempDangerStyle.setDataFormat(workbook.createDataFormat().getFormat("0.0"));

            // --- Tiêu đề ---
            CellStyle titleStyle = workbook.createCellStyle();
            Font titleFont = workbook.createFont();
            titleFont.setBold(true);
            titleFont.setFontHeightInPoints((short) 14);
            titleFont.setColor(IndexedColors.DARK_BLUE.getIndex());
            titleStyle.setFont(titleFont);

            Row titleRow = sheet.createRow(0);
            Cell titleCell = titleRow.createCell(0);
            titleCell.setCellValue("HE THONG DO SUC KHOE IoT - NHOM 5");
            titleCell.setCellStyle(titleStyle);
            sheet.addMergedRegion(new CellRangeAddress(0, 0, 0, 7));

            // --- Thông tin lọc ---
            Row infoRow = sheet.createRow(1);
            Cell infoCell = infoRow.createCell(0);
            infoCell.setCellValue("Chi hien thi mau hop le (loai bo mau co gia tri = 0)");
            CellStyle infoStyle = workbook.createCellStyle();
            Font infoFont = workbook.createFont();
            infoFont.setItalic(true);
            infoFont.setColor(IndexedColors.GREY_50_PERCENT.getIndex());
            infoFont.setFontHeightInPoints((short) 10);
            infoStyle.setFont(infoFont);
            infoCell.setCellStyle(infoStyle);

            // --- Header bảng ---
            String[] headers = {"STT", "Thoi gian", "Phien do",
                "Elapsed (ms)", "Nhiet do (C)", "Nhip tim (BPM)",
                "SpO2 (%)", "Trang thai"};

            Row headerRow = sheet.createRow(3);
            for (int i = 0; i < headers.length; i++) {
                Cell cell = headerRow.createCell(i);
                cell.setCellValue(headers[i]);
                cell.setCellStyle(headerStyle);
            }

            // --- Dữ liệu (CHỈ mẫu hợp lệ) ---
            int rowNum = 4;
            int stt = 1;
            for (HealthData d : validData) {
                Row row = sheet.createRow(rowNum++);

                CellStyle style = dataStyle;
                CellStyle tempStyle = tempDataStyle;
                if ("WARN".equals(d.getStatus())) {
                    style = warnStyle;
                    tempStyle = tempWarnStyle;
                } else if ("DANGER".equals(d.getStatus())) {
                    style = dangerStyle;
                    tempStyle = tempDangerStyle;
                }

                createCell(row, 0, stt++, style);
                createCell(row, 1, d.getTimestamp(), style);
                createCell(row, 2, "#" + d.getSessionId(), style);
                createCell(row, 3, d.getElapsedMs(), style);

                // Nhiệt độ: format 1 chữ số thập phân
                Cell tempCell = row.createCell(4);
                tempCell.setCellValue(d.getTemperature());
                tempCell.setCellStyle(tempStyle);

                createCell(row, 5, d.getHeartRate(), style);
                createCell(row, 6, d.getSpo2(), style);
                createCell(row, 7, d.getStatus(), style);
            }

            for (int i = 0; i < headers.length; i++) {
                sheet.autoSizeColumn(i);
            }

            // ========== Sheet 2: Tổng kết ==========
            Sheet summary = workbook.createSheet("Tong Ket");

            // Style cho sheet tổng kết
            CellStyle sumTitleStyle = workbook.createCellStyle();
            Font sumTitleFont = workbook.createFont();
            sumTitleFont.setBold(true);
            sumTitleFont.setFontHeightInPoints((short) 14);
            sumTitleFont.setColor(IndexedColors.DARK_BLUE.getIndex());
            sumTitleStyle.setFont(sumTitleFont);

            CellStyle sumLabelStyle = workbook.createCellStyle();
            Font sumLabelFont = workbook.createFont();
            sumLabelFont.setBold(true);
            sumLabelFont.setFontHeightInPoints((short) 11);
            sumLabelStyle.setFont(sumLabelFont);
            setBorders(sumLabelStyle);

            CellStyle sumValueStyle = workbook.createCellStyle();
            sumValueStyle.setAlignment(HorizontalAlignment.LEFT);
            setBorders(sumValueStyle);

            CellStyle sumValueNumStyle = workbook.createCellStyle();
            sumValueNumStyle.cloneStyleFrom(sumValueStyle);
            sumValueNumStyle.setDataFormat(workbook.createDataFormat().getFormat("0.0"));

            // Tiêu đề
            Row s0 = summary.createRow(0);
            Cell s0c = s0.createCell(0);
            s0c.setCellValue("TONG KET CAC PHIEN DO");
            s0c.setCellStyle(sumTitleStyle);
            summary.addMergedRegion(new CellRangeAddress(0, 0, 0, 1));

            // ====== TÍNH TOÁN CHỈ LẤY MẪU HỢP LỆ ======
            int totalAll = allData.size();
            int totalValid = validData.size();
            int totalInvalid = totalAll - totalValid;

            // Tính trung bình CHỈ từ mẫu hợp lệ, bỏ qua chỉ số = 0
            float sumTemp = 0;
            int countTemp = 0;
            int sumHR = 0;
            int countHR = 0;
            int sumSpO2 = 0;
            int countSpO2 = 0;

            for (HealthData d : validData) {
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

            // --- Thống kê mẫu ---
            int r = 2;
            Row sr2 = summary.createRow(r++);
            Cell sr2l = sr2.createCell(0);
            sr2l.setCellValue("Tong so mau thu duoc:");
            sr2l.setCellStyle(sumLabelStyle);
            Cell sr2v = sr2.createCell(1);
            sr2v.setCellValue(totalAll);
            sr2v.setCellStyle(sumValueStyle);

            Row sr3 = summary.createRow(r++);
            Cell sr3l = sr3.createCell(0);
            sr3l.setCellValue("Mau hop le (khac 0):");
            sr3l.setCellStyle(sumLabelStyle);
            Cell sr3v = sr3.createCell(1);
            sr3v.setCellValue(totalValid);
            sr3v.setCellStyle(sumValueStyle);

            Row sr4 = summary.createRow(r++);
            Cell sr4l = sr4.createCell(0);
            sr4l.setCellValue("Mau khong hop le (= 0):");
            sr4l.setCellStyle(sumLabelStyle);
            Cell sr4v = sr4.createCell(1);
            sr4v.setCellValue(totalInvalid);
            sr4v.setCellStyle(sumValueStyle);

            r++; // dòng trống

            // --- Kết quả trung bình ---
            Row srTitle = summary.createRow(r++);
            Cell srTitleC = srTitle.createCell(0);
            srTitleC.setCellValue("KET QUA TRUNG BINH (chi tinh mau hop le):");
            CellStyle secStyle = workbook.createCellStyle();
            Font secFont = workbook.createFont();
            secFont.setBold(true);
            secFont.setFontHeightInPoints((short) 12);
            secStyle.setFont(secFont);
            srTitleC.setCellStyle(secStyle);
            summary.addMergedRegion(new CellRangeAddress(r - 1, r - 1, 0, 1));

            r++; // dòng trống

            // Nhiệt độ
            Row rtmp = summary.createRow(r++);
            Cell rtmpL = rtmp.createCell(0);
            rtmpL.setCellValue("Nhiet do trung binh:");
            rtmpL.setCellStyle(sumLabelStyle);
            Cell rtmpV = rtmp.createCell(1);
            if (countTemp > 0) {
                rtmpV.setCellValue(avgTemp);
                rtmpV.setCellStyle(sumValueNumStyle);
            } else {
                rtmpV.setCellValue("Khong co du lieu");
                rtmpV.setCellStyle(sumValueStyle);
            }
            Cell rtmpU = rtmp.createCell(2);
            rtmpU.setCellValue(countTemp > 0 ? "C  (" + countTemp + " mau)" : "");

            // Nhịp tim
            Row rhr = summary.createRow(r++);
            Cell rhrL = rhr.createCell(0);
            rhrL.setCellValue("Nhip tim trung binh:");
            rhrL.setCellStyle(sumLabelStyle);
            Cell rhrV = rhr.createCell(1);
            if (countHR > 0) {
                rhrV.setCellValue(avgHR);
                rhrV.setCellStyle(sumValueStyle);
            } else {
                rhrV.setCellValue("Khong co du lieu");
                rhrV.setCellStyle(sumValueStyle);
            }
            Cell rhrU = rhr.createCell(2);
            rhrU.setCellValue(countHR > 0 ? "BPM  (" + countHR + " mau)" : "");

            // SpO2
            Row rsp = summary.createRow(r++);
            Cell rspL = rsp.createCell(0);
            rspL.setCellValue("SpO2 trung binh:");
            rspL.setCellStyle(sumLabelStyle);
            Cell rspV = rsp.createCell(1);
            if (countSpO2 > 0) {
                rspV.setCellValue(avgSpO2);
                rspV.setCellStyle(sumValueStyle);
            } else {
                rspV.setCellValue("Khong co du lieu");
                rspV.setCellStyle(sumValueStyle);
            }
            Cell rspU = rsp.createCell(2);
            rspU.setCellValue(countSpO2 > 0 ? "%  (" + countSpO2 + " mau)" : "");

            r++; // dòng trống

            // --- Đánh giá sức khỏe ---
            String overallStatus;
            if (avgTemp > 0 && (avgTemp < 35.0f || avgTemp > 38.5f)
                    || avgHR > 0 && (avgHR < 50 || avgHR > 120)
                    || avgSpO2 > 0 && avgSpO2 < 90) {
                overallStatus = "NGUY HIEM - Can kiem tra ngay!";
            } else if (avgTemp > 0 && (avgTemp < 36.0f || avgTemp > 37.5f)
                    || avgHR > 0 && (avgHR < 60 || avgHR > 100)
                    || avgSpO2 > 0 && avgSpO2 < 95) {
                overallStatus = "CANH BAO - Co chi so ngoai nguong binh thuong";
            } else if (countTemp > 0 || countHR > 0 || countSpO2 > 0) {
                overallStatus = "BINH THUONG - Tat ca chi so an toan";
            } else {
                overallStatus = "Khong co du lieu hop le";
            }

            Row rStatus = summary.createRow(r++);
            Cell rStatusL = rStatus.createCell(0);
            rStatusL.setCellValue("Danh gia suc khoe:");
            rStatusL.setCellStyle(sumLabelStyle);
            Cell rStatusV = rStatus.createCell(1);
            rStatusV.setCellValue(overallStatus);

            // Style cho đánh giá
            CellStyle statusCellStyle = workbook.createCellStyle();
            Font statusFont = workbook.createFont();
            statusFont.setBold(true);
            statusFont.setFontHeightInPoints((short) 11);
            if (overallStatus.startsWith("NGUY")) {
                statusFont.setColor(IndexedColors.RED.getIndex());
            } else if (overallStatus.startsWith("CANH")) {
                statusFont.setColor(IndexedColors.ORANGE.getIndex());
            } else {
                statusFont.setColor(IndexedColors.GREEN.getIndex());
            }
            statusCellStyle.setFont(statusFont);
            rStatusV.setCellStyle(statusCellStyle);

            r += 2;

            // --- Ghi chú ---
            Row rNote = summary.createRow(r++);
            rNote.createCell(0).setCellValue("GHI CHU:");
            Row rn1 = summary.createRow(r++);
            rn1.createCell(0).setCellValue("- Mau khong hop le: nhiet do = 0, nhip tim = 0, SpO2 = 0");
            Row rn2 = summary.createRow(r++);
            rn2.createCell(0).setCellValue("- Trung binh duoc tinh rieng cho tung chi so (bo qua gia tri = 0)");
            Row rn3 = summary.createRow(r++);
            rn3.createCell(0).setCellValue("- Nguong binh thuong: Nhiet do 36-37.5C, Nhip tim 60-100 BPM, SpO2 >= 95%");

            summary.autoSizeColumn(0);
            summary.autoSizeColumn(1);
            summary.autoSizeColumn(2);

            // --- Xuất file ---
            resp.setContentType(
                    "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
            resp.setHeader("Content-Disposition",
                    "attachment; filename=HealthMonitor_Data.xlsx");
            workbook.write(resp.getOutputStream());
        }
    }

    private void setBorders(CellStyle style) {
        style.setBorderBottom(BorderStyle.THIN);
        style.setBorderTop(BorderStyle.THIN);
        style.setBorderLeft(BorderStyle.THIN);
        style.setBorderRight(BorderStyle.THIN);
    }

    private void createCell(Row row, int col, Object value, CellStyle style) {
        Cell cell = row.createCell(col);
        if (value instanceof Number) {
            cell.setCellValue(((Number) value).doubleValue());
        } else {
            cell.setCellValue(String.valueOf(value));
        }
        cell.setCellStyle(style);
    }
}
