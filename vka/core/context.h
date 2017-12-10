#ifndef VKA_CONTEXT_H
#define VKA_CONTEXT_H

#include <vulkan/vulkan.hpp>
#include <vka/core/log.h>

struct GLFWwindow;

namespace vka
{


struct queue_family_index_t
{
    int graphics = -1;
    int present  = -1;

    operator bool() const
    {
        return isComplete();
    }

    bool isComplete() const {
        return graphics >= 0 && present>=0;
    }
};


class context
{
private:
    vk::Instance       m_instance;

    vk::PhysicalDevice           m_physical_device;
    vk::PhysicalDeviceProperties m_physical_device_properties;

    queue_family_index_t         m_queue_family;

    vk::Device         m_device;

    vk::SwapchainKHR                  m_swapchain;
    vk::SurfaceCapabilitiesKHR        m_swapchain_capabilities;
    std::vector<vk::SurfaceFormatKHR> m_swapchain_available_formats;
    vk::SurfaceFormatKHR              m_swapchain_format;
    std::vector<vk::PresentModeKHR>   m_swapchain_available_present_modes;
    vk::PresentModeKHR                m_swapchain_present_mode;

    vk::Queue                  m_graphics_queue;
    vk::Queue                  m_present_queue;

    vk::DebugReportCallbackEXT  m_callback;


    vk::SurfaceKHR     m_surface;

public:
    context() // default constructor
    {

    }

    ~context() // destructor
    {
        clean();
    }

    context(context const & other) // copy constructor
    {
        *this = other;
    }

    context(context && other) // move constructor
    {
        *this = std::move(other);
    }

    context & operator=(context const & other) // copy operator
    {
        if( this != &other)
        {

        }
        return *this;
    }

    context & operator=(context && other) // move operator
    {
        if( this != &other)
        {

        }
        return *this;
    }

    //=========================

    /**
     * @brief init
     *
     * Initialize the library by.
     */
    void init();

    /**
     * @brief clean
     *
     * Cleans up the library
     */
    void clean();


    /**
     * @brief set_window
     * @param window
     *
     * Creates a window surface
     */
    void create_window_surface( GLFWwindow * window );

    void create_device();

    void create_logical_device(vk::PhysicalDevice &p_physical_device, const vka::queue_family_index_t &p_Qfamily);

    void create_swap_chain();
private:


    bool m_enable_validation_layers = true;


    bool check_validation_layer_support();

    queue_family_index_t find_queue_families(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface);

    bool are_device_extensions_supported(const vk::PhysicalDevice &device, const std::vector<const char *> &deviceExtensions);

    bool can_present_to_surface(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface);

    std::vector<const char*> get_required_extensions();


    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);


    std::vector< std::string > m_required_device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    std::vector<const char*> m_required_validation_layers = {
//        "VK_LAYER_LUNARG_standard_validation",
        "VK_LAYER_LUNARG_parameter_validation",
        "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_core_validation",
        "VK_LAYER_GOOGLE_unique_objects"

    };

    void setup_debug_callback();

    /**
     * @brief CreateDebugReportCallbackEXT
     * @param instance
     * @param pCreateInfo
     * @param pAllocator
     * @param pCallback
     * @return
     *
     * Creates a debug callback. This function is needed because it is an extension
     * and requires that the function be called using the getProcAddr
     */
    VkResult CreateDebugReportCallbackEXT(vk::Instance &instance, const vk::DebugReportCallbackCreateInfoEXT &pCreateInfo, const VkAllocationCallbacks *pAllocator, vk::DebugReportCallbackEXT &pCallback);
    void     DestroyDebugReportCallbackEXT(vk::Instance const & instance,vk::DebugReportCallbackEXT & callback,const VkAllocationCallbacks* pAllocator);


};

}

#endif
