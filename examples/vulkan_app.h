#ifndef VKA_VULKAN_APP_H
#define VKA_VULKAN_APP_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <vka/core/HostImage.h>
#include <vka/vka.h>

#include <vka/utils/buffer_memory_manager.h>
#include <vka/core/managed_buffer.h>
#include <vka/utils/buffer_pool.h>

#include <vka/utils/glfw_window_handler.h>
#include <vka/core/screen_target.h>


struct VulkanApp :   public vka::GLFW_Window_Handler
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
        glfwInit();
        init__ = true;
      }
  }

  void init(uint32_t w, uint32_t h, const char* title,
            std::vector<std::string> const & extra_instance_extensions = std::vector<std::string>(),
            std::vector<std::string> const & extra_device_extensions = std::vector<std::string>())
  {

      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE,  GLFW_FALSE);

      m_win = glfwCreateWindow(w,h,title,nullptr,nullptr);

      unsigned int glfwExtensionCount = 0;
      const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
      for(uint i=0;i<glfwExtensionCount;i++)  m_Context.enable_extension( glfwExtensions[i] );
      m_Context.enable_extension( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );

      for(auto & v : extra_instance_extensions)
          m_Context.enable_extension( &v[0] );

      m_Context.enable_device_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
      for(auto & v : extra_device_extensions)
          m_Context.enable_device_extension( &v[0] );

      attach_window(m_win);

      m_Context.init();

      // need a surface --> device --> swapchaim (screen)
      vk::SurfaceKHR surface;
      if (glfwCreateWindowSurface( m_Context.get_instance(), m_win, nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface) ) != VK_SUCCESS)
      {
          ERROR << "Failed to create window surface!" << ENDL;
          throw std::runtime_error("failed to create window surface!");
      }

      m_Context.create_device(surface); // find the appropriate device


#if 0
      m_Context.create_swap_chain( {w,h}); // create the swap chain
#else
      m_screen = m_Context.new_screen("m_win");
      m_screen->set_extent( vk::Extent2D(w,h) );
      m_screen->set_surface( surface );
      m_screen->create();
#endif

  }

#if 0

  void init_default_renderpass(uint32_t w, uint32_t h)
  {
      //==========================================================================
      // Create the Render pass and the frame buffers.
      //
      // The Context holds the images that will be used for the swap chain. We need
      // to create the framebuffers with those images.
      //
      // All objects created by the context requires a unique name
      //==========================================================================
      // Create a depth texture which we will use to be store the depths
      // of each pixel.
      m_depth = m_Context.new_depth_texture("depth_texture");
      m_depth->set_size( w, h, 1);
      m_depth->create();
      m_depth->create_image_view( vk::ImageAspectFlagBits::eDepth);

      // Convert the depth texture into the proper image layout.
      // This method will automatically allocate a a command buffer and
      // and then submit it.  Alternatively, passing in a command buffer
      // as the first argument simply writes the command to the buffer
      // but does not submit it.
      m_depth->convert(vk::ImageLayout::eDepthStencilAttachmentOptimal);

      m_default_renderpass = m_Context.new_renderpass("default_renderpass");

#if 1
      // The default render pass only needs one colour attachment (The main scren)
      // and one depth attachment which will be a depth texture
      m_default_renderpass->set_num_color_attachments(1);

      // set the layout of the colour and depth attachments
      m_default_renderpass->set_color_attachment_layout(0, vk::ImageLayout::eColorAttachmentOptimal);
      m_default_renderpass->set_depth_attachment_layout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

      // Set the format and final layout
      m_default_renderpass->get_color_attachment_description(0).format      = vk::Format::eB8G8R8A8Unorm;
      m_default_renderpass->get_color_attachment_description(0).finalLayout = vk::ImageLayout::ePresentSrcKHR;

      m_default_renderpass->get_depth_attachment_description().format = m_depth->get_format();
      m_default_renderpass->get_depth_attachment_description().finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;;

      m_default_renderpass->create();
#else
      // old way
      m_default_renderpass->attach_color(vk::Format::eB8G8R8A8Unorm);
      m_default_renderpass->attach_depth( m_depth->get_format() );  // [NEW] we will now be using a depth attachment
      m_default_renderpass->create(m_Context);
#endif



      std::vector<vk::ImageView> & iv = m_Context.get_swapchain_imageviews();
      int i=0;
      for(auto & view : iv)
      {
          m_framebuffers.push_back(  m_Context.new_framebuffer( std::string("default_fb_") + std::to_string(i++) ) );
          m_framebuffers.back()->create( *m_default_renderpass, vk::Extent2D{WIDTH,HEIGHT}, view,
                                       m_depth->get_image_view()); // [NEW]
      }
  }

#endif

  virtual void onInit() = 0;
  virtual void onFrame(double dt, double T) = 0;

  virtual void start_mainloop()
  {
      onInit();

      double T  = 0;
      double dt = 0;

      while ( !glfwWindowShouldClose( m_win ) )
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

  GLFWwindow            * m_win;
  vka::context            m_Context;
  vka::screen           * m_screen;

  vka::texture * m_depth ;
  vka::renderpass               *m_default_renderpass;
  std::vector<vka::framebuffer*> m_framebuffers;

};

#endif
