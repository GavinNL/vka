#ifndef VKA_CONTEXT_H
#define VKA_CONTEXT_H

#include <vulkan/vulkan.hpp>
#include <vka/core/log.h>
#include "extensions.h"
#include "deleter.h"
#include "classes.h"
#include <map>

struct GLFWwindow;


namespace vka
{

class descriptor_set_layout;
class Semaphore;

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


struct DescriptorSetLayoutBindingCmp
{
bool operator()(const std::vector<vk::DescriptorSetLayoutBinding> & lhs, const std::vector<vk::DescriptorSetLayoutBinding> & rhs)
{
    if( lhs.size() < rhs.size() ) return true;
    if( lhs.size() > rhs.size() ) return false;

    if( memcmp( lhs.data(), rhs.data(), sizeof(vk::DescriptorSetLayoutBinding)* rhs.size() ) < 0)
    {
        return true;
    }

    return false;
}

};

class context
{

private:
    vk::Instance                 m_instance;
    vk::PhysicalDevice           m_physical_device;
    vk::PhysicalDeviceProperties m_physical_device_properties;
    queue_family_index_t         m_queue_family;
    vk::Device                   m_device;
    vk::SurfaceKHR               m_surface;


    vk::Extent2D                 m_extent;
    vk::Queue                    m_graphics_queue;
    vk::Queue                    m_present_queue;
    vk::Fence                    m_render_fence;

    vk::DebugReportCallbackEXT   m_callback;


    std::map< std::vector<vk::DescriptorSetLayoutBinding>,
              std::shared_ptr<vka::descriptor_set_layout>,
              DescriptorSetLayoutBindingCmp> m_DescriptorSetLayouts;


    std::vector< std::string > m_instance_extensions;
    std::vector< std::string > m_device_extensions;
    std::vector< std::string > m_validation_layers;

public:
    vk::Instance get_instance() { return m_instance; }
    vk::Device get_device() { return m_device; }
    vk::PhysicalDevice get_physical_device() { return m_physical_device; }
    vk::SurfaceKHR  get_surface() { return m_surface; }
    queue_family_index_t get_queue_family() { return m_queue_family; }

    context() // default constructor
    {

    }

    ~context() // destructor
    {
        clean();
    }

    context(context const & other)  = delete;// copy constructor

    context(context && other) // move constructor
    {
        *this = std::move(other);
    }

    context & operator=(context const & other) = delete; // copy operator

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
    vk::SurfaceKHR create_window_surface( GLFWwindow * window );

    void set_window_surface( vk::SurfaceKHR & surface)
    {
        m_surface = surface;
    }


    void create_device(vk::SurfaceKHR surface_to_use);

    void create_logical_device(vk::PhysicalDevice &p_physical_device, const vka::queue_family_index_t &p_Qfamily);

    void create_swap_chain(vk::Extent2D extents);

    std::vector<vk::ImageView> create_image_views(const std::vector<vk::Image> &images, vk::Format image_format);

    uint32_t get_next_image_index( vka::Semaphore * signal_semaphore);


    void present_image(uint32_t image_index, Semaphore *wait_semaphore);
    void present_image(const vk::PresentInfoKHR & info);


    //============================================================
    /**
     * @brief new_descriptor_set_layout
     * @param bindings
     * @return
     *
     * Creates a new descriptor set layout based on the binding information given.
     * or returns one that already exists which matches the binding
     */
    std::shared_ptr<vka::descriptor_set_layout> create_descriptor_set_layout( std::vector< vk::DescriptorSetLayoutBinding > const & bindings,
                                                           vk::DescriptorSetLayoutCreateFlags flags=vk::DescriptorSetLayoutCreateFlags());


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
                                const std::shared_ptr<Semaphore> &wait_semaphore,
                                const std::shared_ptr<Semaphore> &signal_semaphore,
                                vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput );





    vk::Format find_supported_format(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    vk::Format find_depth_format();

    vk::PhysicalDeviceLimits const & get_physical_device_limits() const
    {
        return m_physical_device_properties.limits;
    }


    std::shared_ptr<Semaphore>   createSemaphore();
    std::vector<std::weak_ptr<Semaphore> > m_semaphores;

private:

    bool m_enable_validation_layers = true;


    bool check_validation_layer_support();

    queue_family_index_t find_queue_families(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface);

    bool are_device_extensions_supported(const vk::PhysicalDevice &device, const std::vector<const char *> &deviceExtensions);

    bool can_present_to_surface(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface);

    std::vector<const char*> get_required_extensions();


    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);


    std::vector<const char*> m_required_validation_layers = {
//        "VK_LAYER_LUNARG_standard_validation",
        "VK_LAYER_LUNARG_parameter_validation",
        "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_core_validation",
        "VK_LAYER_GOOGLE_unique_objects"

    };

    void setup_debug_callback();

public:
    void enable_extension( const std::string & extension);
    void enable_validation_layer( const std::string & layer_name);
    void enable_device_extension(const std::string & extension);

private:


};

}

#endif
