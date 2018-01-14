#ifndef VKA_CONTEXT_H
#define VKA_CONTEXT_H

#include <vulkan/vulkan.hpp>
#include <vka/core/log.h>
#include "deleter.h"
#include "classes.h"
#include <map>

struct GLFWwindow;

namespace vka
{

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

struct blank{};


class context :
                #define X_MACRO(A) public registry_t<A>,
                X_LIST
                public blank
{
#undef X_MACRO
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


    vk::Fence     m_render_fence;
    //vk::Semaphore m_image_available_smaphore;
    //vk::Semaphore m_render_finished_smaphore;

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
    // Get Methods
    //============================================================
    std::vector<vk::ImageView>& get_swapchain_imageviews() {
        return m_image_views;
    }
    //============================================================
    // Object creation
    //   All objects created with teh following funtions are stored
    //   in an internal registry and can be retrived using the
    //   get< > method.
    //   Objects in here are automatically destroyed when the
    //   context is destroyed.
    //============================================================
    vka::renderpass* new_renderpass(const std::string &name);

    vka::framebuffer* new_framebuffer(const std::string & name);

    vka::command_pool* new_command_pool(const std::string & name);

    vka::descriptor_pool* new_descriptor_pool(const std::string & name);

    // this one should be private
    vka::descriptor_set_layout* new_descriptor_set_layout(const std::string & name);


    uint32_t get_next_image_index( vka::semaphore * signal_semaphore);


    void present_image(uint32_t image_index, semaphore *wait_semaphore);
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

    /**
     * @brief new_vertex_buffer
     * @param name
     * @param size
     * @return
     *
     * Create a device local buffer used for vertices
     */
    vka::buffer* new_vertex_buffer(const std::string & name, size_t size)
    {
        return new_buffer(name , size,
                          vk::MemoryPropertyFlagBits::eDeviceLocal,
                          vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer);
    }

    /**
     * @brief new_index_buffer
     * @param name
     * @param size
     * @return
     *
     * Create a device local buffer used for index values
     */
    vka::buffer* new_index_buffer(const std::string & name, size_t size)
    {
        return new_buffer(name , size,
                          vk::MemoryPropertyFlagBits::eDeviceLocal,
                          vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer);
    }

    /**
     * @brief new_uniform_buffer
     * @param name
     * @param size
     * @return
     *
     * Crete a device local buffer used for uniform values
     */
    vka::buffer* new_uniform_buffer(const std::string & name, size_t size)
    {
        return new_buffer(name , size,
                          vk::MemoryPropertyFlagBits::eDeviceLocal,
                          vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer);
    }

    /**
     * @brief new_multi_buffer
     * @param name
     * @param size
     * @return
     *
     * Create a device local buffer used vertices,indices and uniform
     */
    vka::buffer* new_multi_buffer(const std::string & name, size_t size)
    {
        return new_buffer(name , size,
                          vk::MemoryPropertyFlagBits::eDeviceLocal,
                          vk::BufferUsageFlagBits::eTransferDst
                          | vk::BufferUsageFlagBits::eUniformBuffer
                          | vk::BufferUsageFlagBits::eIndexBuffer
                          | vk::BufferUsageFlagBits::eVertexBuffer);
    }


    /**
     * @brief new_staging_buffer
     * @param name
     * @param size
     * @return
     *
     * Create a staging buffer used for staging data and transfering to
     * device local buffers
     */
    vka::buffer* new_staging_buffer(const std::string & name, size_t size, vk::MemoryPropertyFlags extraFlags = vk::MemoryPropertyFlagBits::eHostCoherent)
    {
        return new_buffer(name , size,
                          vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible | extraFlags,
                          vk::BufferUsageFlagBits::eTransferSrc);
    }


    vka::shader* new_shader_module(const std::string &name);


    vka::pipeline* new_pipeline(const std::string & name);


    vka::semaphore* new_semaphore(const std::string & name);

    vka::texture* new_texture(const std::string &name);

    //============================================================



    void submit_cmd_buffer(vk::CommandBuffer b)
    {
        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &b;

        //===========
        if( m_graphics_queue.submit(1, &submitInfo, vk::Fence() ) != vk::Result::eSuccess)
        {
            throw std::runtime_error("Failed to submit Copy Buffer command");
        }

        m_graphics_queue.waitIdle();
    }

    void submit_command_buffer(vk::CommandBuffer const & p_CmdBuffer ,
                                const vka::semaphore *wait_semaphore,
                                const vka::semaphore *signal_semaphore,
                                vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput );


private:

    template<typename T>
    T* _new(const std::string & name)
    {
        if( registry_t<T>::get_object(name) == nullptr)
        {
            std::shared_ptr<T> R( new T(this), vka::deleter<T>() );
            registry_t<T>::insert_object(name, R);

            return R.get();
        }

        return nullptr;
    }

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
