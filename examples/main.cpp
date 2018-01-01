#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <vka/vka.h>

#define WIDTH 1024
#define HEIGHT 768
#define APP_TITLE "Test"

int main(int argc, char ** argv)
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,  GLFW_FALSE);

    auto window = glfwCreateWindow(WIDTH, HEIGHT, APP_TITLE, nullptr, nullptr);


    vka::context C;

    C.init();
    C.create_window_surface(window);
    C.create_device();

    C.create_swap_chain( {WIDTH,HEIGHT});

    auto R = C.new_renderpass("main_renderpass");
    R->attach_color(vk::Format::eB8G8R8A8Unorm);
    R->create(C);

    auto B = C.new_buffer("main_buffer",
                          1024,
                          vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible,
                          vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer);



    auto a = B->map<int>();
    a[0] = 10;

    auto cp = C.new_command_pool("main_command_pool");

    while (!glfwWindowShouldClose(window) )
    {
      glfwPollEvents();
      std::this_thread::sleep_for( std::chrono::milliseconds(3) );
    }

    //std::cout << "Test" << std::endl;
    return 0;
}
