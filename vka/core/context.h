#ifndef VKA_CONTEXT_H
#define VKA_CONTEXT_H

#include <vulkan/vulkan.hpp>
#include <vka/core/log.h>
#include <map>

struct GLFWwindow;

namespace vka
{


class renderpass;
class command_pool;
class buffer;

template<typename T>
class registry_t
{
public:
    virtual ~registry_t()
    {
        LOG << "Destroying registr"    <<ENDL;
        clear();
    }

    void clear()
    {
        LOG << "Registry cleared" << ENDL;
        m_registry.clear();
    }
protected:
    bool insert_object(std::string const & name, std::shared_ptr<T> obj)
    {
        auto i = m_registry.find( name );

        if(i == m_registry.end() )
        {
            m_registry[name] = obj;
            LOG << "Object[" << name << "] registered" << ENDL;
            return true;
        }

        return false;
    }

    T* get_object( std::string const & name)
    {
        auto i = m_registry.find( name );

        if(i != m_registry.end() )
        {
            return i->second.get();
        }

        return nullptr;
    }
    std::map<std::string, std::shared_ptr<T> > m_registry;
};


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


class context : public registry_t<vka::renderpass>,
                public registry_t<vka::command_pool>,
                public registry_t<vka::buffer>
{
private:
    vk::Instance       m_instance;

    vk::PhysicalDevice           m_physical_device;
    vk::PhysicalDeviceProperties m_physical_device_properties;

    queue_family_index_t         m_queue_family;

    vk::Device         m_device;


    //=========== Swap Chain stuff=============
    vk::SwapchainKHR                  m_swapchain;
    std::vector<vk::SurfaceFormatKHR> m_swapchain_available_formats;
    std::vector<vk::PresentModeKHR>   m_swapchain_available_present_modes;

    vk::SurfaceCapabilitiesKHR        m_swapchain_capabilities;
    vk::SurfaceFormatKHR              m_swapchain_format;
    vk::PresentModeKHR                m_swapchain_present_mode;

    vk::Extent2D                      m_extent;
    vk::Format                        m_image_format;
    std::vector<vk::Image>            m_images;
    std::vector<vk::ImageView>        m_image_views;
    std::vector<vk::Framebuffer>      m_framebuffers;
    //==========================================


    vk::Queue                  m_graphics_queue;
    vk::Queue                  m_present_queue;

    vk::DebugReportCallbackEXT  m_callback;


    vk::SurfaceKHR     m_surface;

public:
    vk::Device get_device() { return m_device; }
    vk::PhysicalDevice get_physical_device() { return m_physical_device; }


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

    void create_swap_chain(vk::Extent2D extents);

    std::vector<vk::ImageView> create_image_views(const std::vector<vk::Image> &images, vk::Format image_format);

    //============================================================
    // Object creation
    //   All objects created with teh following funtions are stored
    //   in an internal registry and can be retrived using the
    //   get< > method.
    //   Objects in here are automatically destroyed when the
    //   context is destroyed.
    //============================================================
    vka::renderpass* new_renderpass(const std::string &name);

    vka::command_pool* new_command_pool(const std::string & name);

    /**
     * @brief new_buffer
     * @param name
     * @return
     *
     * Create an unconfigured,unallocated buffer
     */
    vka::buffer*   new_buffer(const std::string & name);

    /**
     * @brief new_buffer
     * @param name - name of the buffer
     * @param size - size of the buffer in bytes
     * @param memory_properties - memory property
     * @param usage - buffer usage
     * @return
     *
     * Create a configured and allocated buffer.
     */
    vka::buffer*   new_buffer(const std::string & name,
                              size_t size,
                              vk::MemoryPropertyFlags memory_properties,
                              vk::BufferUsageFlags usage);
    //============================================================

private:


    std::map< std::string, std::shared_ptr<vka::renderpass> > m_renderpasses;


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
