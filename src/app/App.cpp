#include "App.h"
#include "../ui/Dashboard.h"
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>
#ifdef _WIN32
#include <Windows.h>
#endif
#include <gl/GL.h>

App::App() {}
App::~App(){
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  if (glctx_) SDL_GL_DeleteContext(glctx_);
  if (window_) SDL_DestroyWindow(window_);
  SDL_Quit();
}

bool App::init(){
  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_AUDIO) != 0) {
    spdlog::error("SDL init failed: {}", SDL_GetError()); return false;
  }
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  window_ = SDL_CreateWindow("LiveOps Sentinel", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 720, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
  glctx_ = SDL_GL_CreateContext(window_);
  SDL_GL_SetSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(window_, glctx_);
  ImGui_ImplOpenGL3_Init("#version 150");

  ui_ = std::make_unique<Dashboard>();
  return true;
}

void App::run(){
  SDL_Event e;
  while (running_){
    while (SDL_PollEvent(&e)){
      ImGui_ImplSDL2_ProcessEvent(&e);
      if (e.type == SDL_QUIT) running_ = false;
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ui_->draw();

    ImGui::Render();
    glViewport(0,0,(int)ImGui::GetIO().DisplaySize.x,(int)ImGui::GetIO().DisplaySize.y);
    glClearColor(0.1f,0.1f,0.12f,1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window_);
  }
}
