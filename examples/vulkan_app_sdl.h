#ifndef VKA_VULKAN_APP_SDL_H
#define VKA_VULKAN_APP_SDL_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_vulkan.h>

#include <vulkan/vulkan.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <vka/core/image.h>
#include <vka/vka.h>

#include <vka/utils/buffer_memory_manager.h>
#include <vka/core/managed_buffer.h>
#include <vka/utils/buffer_pool.h>

#include <vka/utils/sdl_window_handler.h>
#include <vka/core/screen_target.h>


struct VulkanApp :   public vka::SDL_Window_Handler
{
#define ANIMATE(variable, change)\
(onPoll << [&](double t)\
{                       \
    variable = change;  \
}).detach();

  VulkanApp()
  {
      static int init__=false;
      if(!init__)
      {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS );
        init__ = true;
      }
  }

  void init(uint32_t w, uint32_t h, const char* title)
  {

      SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS );

      if(SDL_Vulkan_LoadLibrary(NULL) == -1)
      {
          std::cout << "Error loading vulkan" << std::endl;
          exit(1);
      }
      atexit(SDL_Quit);

      auto window = SDL_CreateWindow("APPLICATION_NAME",
          SDL_WINDOWPOS_UNDEFINED,
          SDL_WINDOWPOS_UNDEFINED,
          WIDTH,
          HEIGHT,
          SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

      if(window == NULL)
      {
          std::cout << "Couldn\'t set video mode: " << SDL_GetError() << std::endl;
          exit(1);
      }
      m_win = window;

      attach_window(m_win);


      unsigned int count = 0;
      SDL_Vulkan_GetInstanceExtensions(window, &count, NULL);
      const char **names = new const char *[count];
      SDL_Vulkan_GetInstanceExtensions(window, &count, names);

      std::vector<char const *> extensions(names, names + count );
      extensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );

      m_Context.init(extensions);

      vk::SurfaceKHR surface;
      if( !SDL_Vulkan_CreateSurface( window, m_Context.get_instance(), reinterpret_cast<VkSurfaceKHR*>(&surface)  ) )
      {
          ERROR << "Failed to create surface" << ENDL;
      }
      m_Context.create_device(surface); // find the appropriate device



      m_screen = m_Context.new_screen("m_win");
      m_screen->set_extent( vk::Extent2D(w,h) );
      m_screen->set_surface( surface );
      m_screen->create();

  }


  virtual void onInit() = 0;
  virtual void onFrame(double dt, double T) = 0;

  virtual void start_mainloop()
  {
      onInit();

      double T  = 0;
      double dt = 0;

      while ( !m_quit )
      {
          dt  = get_elapsed_time()-T;
          T  += dt;

          Poll();

          onFrame(dt, T);

          std::this_thread::sleep_for( std::chrono::milliseconds(3) );
      }
  }

  double get_elapsed_time() const
  {
      static auto startTime = std::chrono::high_resolution_clock::now();

      auto currentTime = std::chrono::high_resolution_clock::now();
      double time = 1.0 * std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0;

      return time;

  }

  uint64_t microseconds() const
  {
      static auto startTime = std::chrono::high_resolution_clock::now();
      auto currentTime = std::chrono::high_resolution_clock::now();
      return std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime).count();
  }
  //=====================================

  SDL_Window            * m_win;
  vka::context            m_Context;
  vka::screen           * m_screen;

  vka::texture * m_depth ;
  vka::renderpass               *m_default_renderpass;
  std::vector<vka::framebuffer*> m_framebuffers;

};

#endif
