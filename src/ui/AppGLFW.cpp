#include "AppGLFW.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <deque>
#include <chrono>
#include <random>
#include <cmath>

namespace {
struct Ring {
  std::deque<float> q; size_t cap{600};
  void push(float v){ q.push_back(v); if(q.size()>cap) q.pop_front(); }
  void plot(const char* label, float minv, float maxv){
    ImGui::PlotLines(label, q.data(), (int)q.size(), 0, nullptr, minv, maxv, ImVec2(0,80));
  }
};
}

namespace ui {

int RunApp() {
  if (!glfwInit()) return 1;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow* win = glfwCreateWindow(1280, 720, "LiveOps Sentinel", nullptr, nullptr);
  if (!win) { glfwTerminate(); return 2; }
  glfwMakeContextCurrent(win);
  glfwSwapInterval(1);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { glfwDestroyWindow(win); glfwTerminate(); return 3; }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(win, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  Ring rtt, loss;
  auto t0 = std::chrono::steady_clock::now();
  std::mt19937 rng{std::random_device{}()};
  std::normal_distribution<float> jitter(0.f, 8.f);

  while (!glfwWindowShouldClose(win)) {
    glfwPollEvents();

    // demo metrics (후에 Probe/OBS로 교체)
    auto now = std::chrono::steady_clock::now();
    float sec = std::chrono::duration<float>(now - t0).count();
    float baseRtt  = 40.f + 20.f * std::sin(sec * 1.2f) + jitter(rng);
    float baseLoss = 0.5f + 1.2f * std::abs(std::sin(sec * 0.7f)) + 0.2f * (jitter(rng)*0.1f);
    rtt.push(std::max(0.f, baseRtt));
    loss.push(std::max(0.f, baseLoss));

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("LiveOps Sentinel • Monitor");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Separator();
    rtt.plot("RTT (ms)", 0.f, 120.f);
    loss.plot("Loss (%%)", 0.f, 5.f);
    ImGui::Separator();
    ImGui::TextUnformatted("GUI build OK (ImGui + GLFW + OpenGL3)");
    ImGui::End();

    ImGui::Render();
    int w, h; glfwGetFramebufferSize(win, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(win);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(win);
  glfwTerminate();
  return 0;
}

} // namespace ui
