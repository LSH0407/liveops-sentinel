#pragma once
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ui {

enum class ChartType {
    LINE,
    BAR,
    GAUGE,
    PIE,
    SCATTER
};

enum class ChartTheme {
    LIGHT,
    DARK,
    AUTO
};

struct DataPoint {
    double x;
    double y;
    std::string label;
    std::string color;
    
    DataPoint() : x(0.0), y(0.0) {}
    DataPoint(double x_val, double y_val) : x(x_val), y(y_val) {}
    DataPoint(double x_val, double y_val, const std::string& lbl) 
        : x(x_val), y(y_val), label(lbl) {}
};

struct ChartConfig {
    std::string title;
    std::string x_label;
    std::string y_label;
    ChartType type;
    ChartTheme theme;
    int width;
    int height;
    bool show_grid;
    bool show_legend;
    bool auto_scale;
    double min_value;
    double max_value;
    std::vector<std::string> colors;
    
    ChartConfig() : type(ChartType::LINE), theme(ChartTheme::AUTO), 
                   width(400), height(300), show_grid(true), 
                   show_legend(true), auto_scale(true), 
                   min_value(0.0), max_value(100.0) {
        colors = {"#FF6B6B", "#4ECDC4", "#45B7D1", "#96CEB4", "#FFEAA7"};
    }
};

struct TimeSeriesData {
    std::string name;
    std::vector<DataPoint> points;
    std::string color;
    bool visible;
    
    TimeSeriesData() : visible(true) {}
    TimeSeriesData(const std::string& n) : name(n), visible(true) {}
    
    void addPoint(double value, const std::string& timestamp = "");
    void addPoint(double x, double y, const std::string& label = "");
    void clear();
    size_t size() const { return points.size(); }
    bool empty() const { return points.empty(); }
};

class ChartRenderer {
public:
    ChartRenderer();
    ~ChartRenderer();
    
    // 차트 생성 및 관리
    void createChart(const std::string& id, const ChartConfig& config);
    void updateChart(const std::string& id, const std::vector<TimeSeriesData>& data);
    void removeChart(const std::string& id);
    
    // 데이터 관리
    void addDataPoint(const std::string& chart_id, const std::string& series_name, 
                     double value, const std::string& timestamp = "");
    void addDataPoint(const std::string& chart_id, const std::string& series_name, 
                     double x, double y, const std::string& label = "");
    void clearData(const std::string& chart_id);
    void setMaxDataPoints(const std::string& chart_id, size_t max_points);
    
    // 차트 설정
    void setChartConfig(const std::string& id, const ChartConfig& config);
    void setTheme(const std::string& id, ChartTheme theme);
    void setSize(const std::string& id, int width, int height);
    void setRange(const std::string& id, double min_value, double max_value);
    
    // 렌더링
    void renderChart(const std::string& id);
    void renderAllCharts();
    
    // 실시간 업데이트
    void enableRealTimeUpdate(const std::string& id, bool enabled);
    void setUpdateInterval(const std::string& id, std::chrono::milliseconds interval);
    
    // 내보내기
    bool exportChart(const std::string& id, const std::string& filename);
    json exportChartData(const std::string& id);
    
    // 유틸리티
    std::vector<std::string> getChartIds() const;
    bool chartExists(const std::string& id) const;
    ChartConfig getChartConfig(const std::string& id) const;
    
private:
    struct ChartData {
        ChartConfig config;
        std::vector<TimeSeriesData> series;
        size_t max_data_points;
        bool real_time_enabled;
        std::chrono::milliseconds update_interval;
        std::chrono::system_clock::time_point last_update;
    };
    
    std::map<std::string, ChartData> charts_;
    
    // 렌더링 헬퍼 함수들
    void renderLineChart(const std::string& id, const ChartData& chart);
    void renderBarChart(const std::string& id, const ChartData& chart);
    void renderGaugeChart(const std::string& id, const ChartData& chart);
    void renderPieChart(const std::string& id, const ChartData& chart);
    void renderScatterChart(const std::string& id, const ChartData& chart);
    
    // 데이터 처리
    void trimDataPoints(TimeSeriesData& series, size_t max_points);
    void autoScaleChart(ChartData& chart);
    std::pair<double, double> calculateDataRange(const std::vector<TimeSeriesData>& series);
    
    // 색상 관리
    std::string getNextColor(const ChartConfig& config, size_t index);
    std::string getThemeColor(ChartTheme theme, const std::string& element);
    
    // 시간 처리
    std::string formatTimestamp(const std::string& timestamp);
    std::string getCurrentTimestamp();
};

// 특화된 차트 클래스들
class MetricGauge {
public:
    MetricGauge(const std::string& title, double min_value = 0.0, double max_value = 100.0);
    
    void setValue(double value);
    void setThresholds(double warning_threshold, double critical_threshold);
    void setColor(const std::string& color);
    void render(int x, int y, int width, int height);
    
private:
    std::string title_;
    double value_;
    double min_value_;
    double max_value_;
    double warning_threshold_;
    double critical_threshold_;
    std::string color_;
};

class PerformanceGraph {
public:
    PerformanceGraph(const std::string& title, int max_points = 100);
    
    void addMetric(const std::string& name, double value);
    void setTimeRange(std::chrono::minutes range);
    void render(int x, int y, int width, int height);
    
private:
    std::string title_;
    std::map<std::string, TimeSeriesData> metrics_;
    int max_points_;
    std::chrono::minutes time_range_;
};

class AlertHistory {
public:
    AlertHistory(const std::string& title);
    
    void addAlert(const std::string& level, const std::string& message, 
                 const std::chrono::system_clock::time_point& timestamp);
    void render(int x, int y, int width, int height);
    
private:
    std::string title_;
    struct AlertEntry {
        std::string level;
        std::string message;
        std::chrono::system_clock::time_point timestamp;
    };
    std::vector<AlertEntry> alerts_;
};

} // namespace ui
