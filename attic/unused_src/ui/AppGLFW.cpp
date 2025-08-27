#include "AppGLFW.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace ui {

class AppGLFW::AppGLFWImpl {
public:
    AppGLFWImpl() : window_(nullptr), dashboard_data_(), alerts_(), 
                    discord_webhook_url_(""), obs_url_("ws://localhost:4444"), 
                    obs_password_(""), show_settings_(false) {
        
        // 콜백 초기화
        start_streaming_callback_ = [](){};
        stop_streaming_callback_ = [](){};
        start_recording_callback_ = [](){};
        stop_recording_callback_ = [](){};
        scene_change_callback_ = [](const std::string&){};
        discord_webhook_callback_ = [](const std::string&){};
        
        // GLFW 초기화
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }
        
        // OpenGL 3.3 설정
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        // 윈도우 생성
        window_ = glfwCreateWindow(1200, 800, "LiveOps Sentinel", nullptr, nullptr);
        if (!window_) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }
        
        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1); // VSync 활성화
        
        // ImGui 초기화
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        
        // ImGui 스타일 설정
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
        
        // ImGui 플랫폼/렌더러 바인딩
        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init("#version 330");
    }
    
    ~AppGLFWImpl() {
        if (window_) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            glfwDestroyWindow(window_);
        }
        glfwTerminate();
    }
    
    int run() {
        while (!glfwWindowShouldClose(window_)) {
            glfwPollEvents();
            
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            renderMainWindow();
            
            if (show_settings_) {
                renderSettingsWindow();
            }
            
            ImGui::Render();
            
            int display_w, display_h;
            glfwGetFramebufferSize(window_, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                GLFWwindow* backup_current_context = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(backup_current_context);
            }
            
            glfwSwapBuffers(window_);
        }
        
        return 0;
    }
    
    void updateDashboardData(const DashboardData& data) {
        dashboard_data_ = data;
    }
    
    void updateAlerts(const std::vector<AlertItem>& alerts) {
        alerts_ = alerts;
    }
    
    void setStartStreamingCallback(std::function<void()> callback) {
        start_streaming_callback_ = callback;
    }
    
    void setStopStreamingCallback(std::function<void()> callback) {
        stop_streaming_callback_ = callback;
    }
    
    void setStartRecordingCallback(std::function<void()> callback) {
        start_recording_callback_ = callback;
    }
    
    void setStopRecordingCallback(std::function<void()> callback) {
        stop_recording_callback_ = callback;
    }
    
    void setSceneChangeCallback(std::function<void(const std::string&)> callback) {
        scene_change_callback_ = callback;
    }
    
    void setDiscordWebhookCallback(std::function<void(const std::string&)> callback) {
        discord_webhook_callback_ = callback;
    }
    
private:
    GLFWwindow* window_;
    DashboardData dashboard_data_;
    std::vector<AlertItem> alerts_;
    std::string discord_webhook_url_;
    std::string obs_url_;
    std::string obs_password_;
    bool show_settings_;
    
    // 콜백 함수들
    std::function<void()> start_streaming_callback_;
    std::function<void()> stop_streaming_callback_;
    std::function<void()> start_recording_callback_;
    std::function<void()> stop_recording_callback_;
    std::function<void(const std::string&)> scene_change_callback_;
    std::function<void(const std::string&)> discord_webhook_callback_;
    
    void renderMainWindow() {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("LiveOps Sentinel", nullptr, 
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
        
        // 상단 메뉴바
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Settings", "Ctrl+S")) {
                    show_settings_ = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) {
                    glfwSetWindowShouldClose(window_, true);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Dashboard", nullptr, nullptr, false);
                ImGui::MenuItem("Alerts", nullptr, nullptr, false);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        
        // 메인 컨텐츠 영역
        ImGui::BeginChild("Content", ImVec2(0, 0), true);
        
        // 대시보드 섹션
        renderDashboard();
        
        ImGui::SameLine();
        
        // 알림 섹션
        renderAlerts();
        
        ImGui::EndChild();
        ImGui::End();
    }
    
    void renderDashboard() {
        ImGui::BeginChild("Dashboard", ImVec2(ImGui::GetWindowWidth() * 0.7f, 0), true);
        ImGui::Text("Dashboard");
        ImGui::Separator();
        
        // OBS 컨트롤
        ImGui::BeginGroup();
        ImGui::Text("OBS Control");
        ImGui::Separator();
        
        if (dashboard_data_.streaming) {
            if (ImGui::Button("Stop Streaming", ImVec2(120, 30))) {
                stop_streaming_callback_();
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "● Streaming");
        } else {
            if (ImGui::Button("Start Streaming", ImVec2(120, 30))) {
                start_streaming_callback_();
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "○ Not Streaming");
        }
        
        ImGui::SameLine();
        
        if (dashboard_data_.recording) {
            if (ImGui::Button("Stop Recording", ImVec2(120, 30))) {
                stop_recording_callback_();
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "● Recording");
        } else {
            if (ImGui::Button("Start Recording", ImVec2(120, 30))) {
                start_recording_callback_();
            }
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "○ Not Recording");
        }
        
        ImGui::Text("Current Scene: %s", dashboard_data_.current_scene.c_str());
        ImGui::EndGroup();
        
        ImGui::Spacing();
        
        // 네트워크 메트릭
        ImGui::BeginGroup();
        ImGui::Text("Network Metrics");
        ImGui::Separator();
        
        ImGui::Text("RTT: %.1f ms", dashboard_data_.rtt_ms);
        ImGui::Text("Packet Loss: %.2f%%", dashboard_data_.loss_pct);
        ImGui::Text("Bandwidth: %.1f Mbps", dashboard_data_.bandwidth_mbps);
        ImGui::EndGroup();
        
        ImGui::SameLine();
        
        // 시스템 메트릭
        ImGui::BeginGroup();
        ImGui::Text("System Metrics");
        ImGui::Separator();
        
        ImGui::Text("CPU: %.1f%%", dashboard_data_.cpu_pct);
        ImGui::Text("GPU: %.1f%%", dashboard_data_.gpu_pct);
        ImGui::Text("Memory: %.1f%%", dashboard_data_.memory_pct);
        ImGui::EndGroup();
        
        ImGui::SameLine();
        
        // OBS 메트릭
        ImGui::BeginGroup();
        ImGui::Text("OBS Metrics");
        ImGui::Separator();
        
        ImGui::Text("Dropped Frames: %.0f", dashboard_data_.dropped_frames);
        ImGui::Text("Encoding Lag: %.1f ms", dashboard_data_.encoding_lag_ms);
        ImGui::Text("Render Lag: %.1f ms", dashboard_data_.render_lag_ms);
        ImGui::EndGroup();
        
        ImGui::EndChild();
    }
    
    void renderAlerts() {
        ImGui::BeginChild("Alerts", ImVec2(0, 0), true);
        ImGui::Text("Alerts");
        ImGui::Separator();
        
        // 알림 통계
        ImGui::Text("Warnings: %d", dashboard_data_.alert_count_warning);
        ImGui::SameLine();
        ImGui::Text("Critical: %d", dashboard_data_.alert_count_critical);
        ImGui::Separator();
        
        // 알림 목록
        if (alerts_.empty()) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No alerts");
        } else {
            for (const auto& alert : alerts_) {
                ImVec4 color;
                if (alert.level == "CRITICAL") {
                    color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                } else if (alert.level == "WARNING") {
                    color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                } else {
                    color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                }
                
                ImGui::TextColored(color, "[%s] %s", alert.level.c_str(), alert.title.c_str());
                ImGui::TextWrapped("%s", alert.message.c_str());
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
                                  "%s | %s", alert.timestamp.c_str(), alert.source.c_str());
                ImGui::Separator();
            }
        }
        
        ImGui::EndChild();
    }
    
    void renderSettingsWindow() {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Settings", &show_settings_)) {
            if (ImGui::BeginTabBar("SettingsTabs")) {
                if (ImGui::BeginTabItem("OBS")) {
                    ImGui::Text("OBS WebSocket Settings");
                    ImGui::Separator();
                    
                    ImGui::InputText("WebSocket URL", &obs_url_);
                    ImGui::InputText("Password (optional)", &obs_password_);
                    
                    if (ImGui::Button("Test Connection")) {
                        // 연결 테스트 로직
                    }
                    
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("Discord")) {
                    ImGui::Text("Discord Webhook Settings");
                    ImGui::Separator();
                    
                    ImGui::InputText("Webhook URL", &discord_webhook_url_);
                    
                    if (ImGui::Button("Test Webhook")) {
                        discord_webhook_callback_(discord_webhook_url_);
                    }
                    
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("Thresholds")) {
                    ImGui::Text("Alert Thresholds");
                    ImGui::Separator();
                    
                    static float rtt_warning = 80.0f;
                    static float rtt_critical = 150.0f;
                    static float loss_warning = 2.0f;
                    static float loss_critical = 5.0f;
                    static float cpu_warning = 80.0f;
                    static float cpu_critical = 95.0f;
                    
                    ImGui::SliderFloat("RTT Warning (ms)", &rtt_warning, 10.0f, 200.0f, "%.1f");
                    ImGui::SliderFloat("RTT Critical (ms)", &rtt_critical, 50.0f, 500.0f, "%.1f");
                    ImGui::SliderFloat("Packet Loss Warning (%)", &loss_warning, 0.1f, 10.0f, "%.1f");
                    ImGui::SliderFloat("Packet Loss Critical (%)", &loss_critical, 1.0f, 20.0f, "%.1f");
                    ImGui::SliderFloat("CPU Warning (%)", &cpu_warning, 50.0f, 90.0f, "%.1f");
                    ImGui::SliderFloat("CPU Critical (%)", &cpu_critical, 80.0f, 100.0f, "%.1f");
                    
                    ImGui::EndTabItem();
                }
                
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }
};

// AppGLFW 구현
AppGLFW::AppGLFW() : impl_(std::make_unique<AppGLFWImpl>()) {}

AppGLFW::~AppGLFW() = default;

int AppGLFW::run() {
    return impl_->run();
}

void AppGLFW::updateDashboardData(const DashboardData& data) {
    impl_->updateDashboardData(data);
}

void AppGLFW::updateAlerts(const std::vector<AlertItem>& alerts) {
    impl_->updateAlerts(alerts);
}

void AppGLFW::setStartStreamingCallback(std::function<void()> callback) {
    impl_->setStartStreamingCallback(callback);
}

void AppGLFW::setStopStreamingCallback(std::function<void()> callback) {
    impl_->setStopStreamingCallback(callback);
}

void AppGLFW::setStartRecordingCallback(std::function<void()> callback) {
    impl_->setStartRecordingCallback(callback);
}

void AppGLFW::setStopRecordingCallback(std::function<void()> callback) {
    impl_->setStopRecordingCallback(callback);
}

void AppGLFW::setSceneChangeCallback(std::function<void(const std::string&)> callback) {
    impl_->setSceneChangeCallback(callback);
}

void AppGLFW::setDiscordWebhookCallback(std::function<void(const std::string&)> callback) {
    impl_->setDiscordWebhookCallback(callback);
}

} // namespace ui
