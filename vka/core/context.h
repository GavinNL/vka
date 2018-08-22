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

class DescriptorLayoutSet;
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
              std::shared_ptr<vka::DescriptorLayoutSet>,
              DescriptorSetLayoutBindingCmp> m_DescriptorSetLayouts;


    std::vector< std::string > m_instance_extensions;
    std::vector< std::string > m_device_extensions;
    std::vector< std::string > m_validation_layers;

public:
    vk::Instance getInstance() { return m_instance; }
    vk::Device getDevice() { return m_device; }
    vk::PhysicalDevice getPhysicalDevice() { return m_physical_device; }
    vk::SurfaceKHR  getSurface() { return m_surface; }
    queue_family_index_t getQueueFamily() { return m_queue_family; }

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


    void setWindowSurface( vk::SurfaceKHR & surface)
    {
        m_surface = surface;
    }


    void createDevice(vk::SurfaceKHR surface_to_use);

    void createLogicalDevice(vk::PhysicalDevice &p_physical_device, const vka::queue_family_index_t &p_Qfamily);


    std::vector<vk::ImageView> createImageViews(const std::vector<vk::Image> &images, vk::Format image_format);

    uint32_t getNextImageIndex( vka::Semaphore * signal_semaphore);


    void presentImage(uint32_t image_index, Semaphore *wait_semaphore);
    void presentImage(const vk::PresentInfoKHR & info);


    //============================================================
    /**
     * @brief new_descriptor_set_layout
     * @param bindings
     * @return
     *
     * Creates a new descriptor set layout based on the binding information given.
     * or returns one that already exists which matches the binding
     */
    std::shared_ptr<vka::DescriptorLayoutSet> createDescriptorSetLayout( std::vector< vk::DescriptorSetLayoutBinding > const & bindings,
                                                           vk::DescriptorSetLayoutCreateFlags flags=vk::DescriptorSetLayoutCreateFlags());


    //============================================================

    void submitCommandBuffer(vk::CommandBuffer b);

    void submitCommandBuffer(vk::CommandBuffer const & p_CmdBuffer ,
                             const std::shared_ptr<Semaphore> &wait_semaphore,
                             const std::shared_ptr<Semaphore> &signal_semaphore,
                             vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput );





    vk::Format findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    vk::Format findDepthFormat();

    vk::PhysicalDeviceLimits const & get_physical_device_limits() const
    {
        return m_physical_device_properties.limits;
    }


    std::shared_ptr<Semaphore>   createSemaphore();

private:
    std::vector<std::weak_ptr<Semaphore> > m_semaphores;

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
    void enableExtension( const std::string & extension);
    void enableValidationLayer( const std::string & layer_name);
    void enableDeviceExtension(const std::string & extension);

private:


};

}

#endif
