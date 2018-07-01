#include <vka/core/context.h>
#include <vka/core/renderpass.h>
#include <vka/core/command_pool.h>
#include <vka/core/buffer.h>
#include <vka/core/managed_buffer.h>
#include <vka/utils/buffer_pool.h>
#include <vka/core/framebuffer.h>
#include <vka/core/shader.h>
#include <vka/core/pipeline.h>
#include <vka/core/semaphore.h>
#include <vka/core/texture.h>
#include <vka/core/texture2d.h>
#include <vka/core/texture2darray.h>
#include <vka/core/descriptor_pool.h>
#include <vka/core/descriptor_set.h>
#include <vka/core/offscreen_target.h>
#include <vka/core/screen_target.h>
#include <vulkan/vulkan.hpp>
#include <vka/core/context_child.h>
#include <vka/core/extensions.h>
#include <set>

namespace vka
{
    vka::ExtensionDispatcher ExtDispatcher;
}

vk::Device          vka::context_child::get_device()
{
    return m_parent->get_device();
}

vk::PhysicalDevice  vka::context_child::get_physical_device()
{
    return m_parent->get_physical_device();
}
vka::context*            vka::context_child::get_parent_context()
{
    return m_parent;
}


void vka::context::init( )
{
    LOG << "Initializing vka context" << ENDL;

    if(m_enable_validation_layers && !check_validation_layer_support() )
    {
        throw std::runtime_error("Validation layers required but not available");
    }



    std::vector<char const*>  required_extensions;
    for(auto & v : m_instance_extensions)
        required_extensions.push_back( &v[0] );

    INFO<< "Required Extensions: " <<  required_extensions.size() << ENDL;
    for(auto & e : required_extensions) INFO << e << ENDL;




    vk::ApplicationInfo appInfo;
    appInfo.setPApplicationName( "Hello Triangle" )
           .setApplicationVersion( VK_MAKE_VERSION(1, 0, 0) )
           .setPEngineName("No Engine")
           .setEngineVersion(VK_MAKE_VERSION(1, 0, 0) )
           .setApiVersion( VK_API_VERSION_1_0);

    vk::InstanceCreateInfo createInfo;

    createInfo.setPApplicationInfo(&appInfo)
              .setEnabledExtensionCount(required_extensions.size() )
              .setPpEnabledExtensionNames(required_extensions.data())
              .setEnabledLayerCount(0);
    if( m_enable_validation_layers )
    {
        createInfo.setEnabledLayerCount(   m_required_validation_layers.size() )
                  .setPpEnabledLayerNames( m_required_validation_layers.data() );
    }

    m_instance = vk::createInstance(createInfo);
    if(!m_instance)
    {
        ERROR << "Failed to create Instance!" << ENDL;
        throw std::runtime_error("Failed to create instance");
    }
    LOG << "Instance Created" << ENDL;

    //==========================================================================
    // Load Extensions
    //==========================================================================

    ExtDispatcher.load_extensions(m_instance, m_instance_extensions);
    ExtDispatcher.load_extensions(m_instance, m_device_extensions);
//
// #define XX(A)
//     ExtDispatcher.A = (PFN_ ## A) m_instance.getProcAddr( #A );\
//     if( ExtDispatcher.A == nullptr)\
//         throw std::runtime_error("Could not load extension: " + std::string(#A) );\
//
//     EXT_LIST
//     #undef XX
    //==========================================================================

    setup_debug_callback();


}


void vka::context::create_device( vk::SurfaceKHR surface_to_use)
{
    m_surface = surface_to_use;
    if( !m_surface)
    {
        ERROR << "Surface not created. Must create a window surface using create_window_surface( ) first." << ENDL;
        throw std::runtime_error("Surface not created. Must create a window surface using create_window_surface( ) first.");
    }

    auto devices = m_instance.enumeratePhysicalDevices();
    if( devices.size() == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    LOG << "Physical Devices found: " << devices.size() << ENDL;

    std::vector<const char *> DeviceExtensions;
    for(auto & d : m_device_extensions)
        DeviceExtensions.push_back( d.data() );

    int i=0;
    for ( vk::PhysicalDevice & device : devices)
    {
        // a device is suitable if it supports all the extensions
        if( are_device_extensions_supported( device, DeviceExtensions ) )
        {
            LOG << "Device " << i << " supports all the required extensions" << ENDL;

            // If it has appropriate graphics and present queues
            auto Qindex = find_queue_families(device, m_surface);

            if( Qindex )
            {
                LOG << "Device " << i << " supports Graphics and Present Queues" << ENDL;
                // it can present to the surface
                if( can_present_to_surface(device,  m_surface ) )
                {
                    LOG << "Device " << i << " Can present to surface" << ENDL;
                    LOG << "Suitable graphics device found!" << ENDL;

                    m_physical_device_properties = device.getProperties();
                    m_physical_device            = device;
                    m_queue_family               = Qindex;

                    create_logical_device(m_physical_device, m_queue_family);


                    //m_image_available_smaphore  = m_device.createSemaphore( vk::SemaphoreCreateInfo() );
                    //m_render_finished_smaphore  = m_device.createSemaphore( vk::SemaphoreCreateInfo() );
                    //
                    //if( !m_image_available_smaphore)
                    //    throw std::runtime_error("Error creating image available semaphore");
                    //if( !m_render_finished_smaphore)
                    //    throw std::runtime_error("Error creating render finished semaphore");
                    m_render_fence = m_device.createFence(vk::FenceCreateInfo());
                    if(!m_render_fence)
                    {
                        throw std::runtime_error("Error creating fence");
                    }
                    m_command_pool   = new_command_pool("context_command_pool");
                    m_staging_buffer = new_staging_buffer("context_staging_buffer",1024*1024*10);
                    return;
                }
            }
        }
    }

}

void vka::context::create_logical_device(vk::PhysicalDevice & p_physical_device, const vka::queue_family_index_t & p_Qfamily)
{
    if(!p_physical_device)
    {
        ERROR << "Physical device not created yet"<<ENDL;
        throw std::runtime_error("Physical device not created yet");
    }


    if( !p_Qfamily )
    {
        ERROR << "Cannot create logical device. The physical device does not support the proper queues" << ENDL;
        throw std::runtime_error("Cannot create logical device. The physical device does not support the proper queues");
    }

    std::set<int> uniqueQueueFamilies = {p_Qfamily.graphics, p_Qfamily.present};

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    float queuePriority = 1.0f;
    for(int queueFamily : uniqueQueueFamilies)
    {
        vk::DeviceQueueCreateInfo queueCreateInfo;

        queueCreateInfo.setQueueFamilyIndex(queueFamily)
                .setQueueCount( 1 )
                .setPQueuePriorities(&queuePriority);

        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures;
    deviceFeatures.fillModeNonSolid = 1;

    vk::DeviceCreateInfo createInfo;

    createInfo.setPQueueCreateInfos(    queueCreateInfos.data() )
            .setQueueCreateInfoCount( queueCreateInfos.size() )
            .setPEnabledFeatures(     &deviceFeatures )
            .setEnabledExtensionCount(0)
            .setEnabledLayerCount(    0); // set default to zero

    bool enableValidationLayers = m_required_validation_layers.size() > 0;

    std::vector<const char*> validationLayers;

    for(auto & s : m_required_validation_layers)
        validationLayers.push_back( &s[0] );

    if( enableValidationLayers )
    {
        INFO << "Enabling validation layers" << ENDL;
        for(auto & c : validationLayers)
            INFO <<  "   layer: " << c << ENDL;

        createInfo.setEnabledLayerCount(   validationLayers.size() )
                .setPpEnabledLayerNames( validationLayers.data() );
    }

    std::vector<const char*> deviceExtensions;
    for(auto & s : m_device_extensions)
        deviceExtensions.push_back( &s[0] );

    for(auto & c : deviceExtensions)
        INFO <<  "device extensions: " << c << ENDL;

    createInfo.setEnabledExtensionCount(   deviceExtensions.size() )
            .setPpEnabledExtensionNames( deviceExtensions.data() );



    m_device = p_physical_device.createDevice(createInfo);

    if( !m_device )
    {
        ERROR << "failed to create logical device!" << ENDL;
        throw std::runtime_error("failed to create logical device!");
    }
    LOG << "Logical Device created" << ENDL;

    m_graphics_queue = m_device.getQueue( p_Qfamily.graphics, 0u);
    m_present_queue  = m_device.getQueue( p_Qfamily.present,  0u);

    if(!m_graphics_queue)
    {
        ERROR << "Error getting graphics queue" << ENDL;;
        throw std::runtime_error("Error getting graphics queue");
    }
    if( !m_present_queue )
    {
        ERROR << "Error getting present queue" << ENDL;;
        throw std::runtime_error("Error getting present queue");
    }

    LOG << "Graphics Queue created" << ENDL;
    LOG << "Present Queue created" << ENDL;

    LOG << "Logical Device created successfully" << ENDL;
    //vkGetDeviceQueue(device, indices.graphics, 0, &graphicsQueue);
}


vka::buffer_pool* vka::context::new_buffer_pool(const std::string & name)
{
    return _new<vka::buffer_pool>(name);
}


vka::framebuffer* vka::context::new_framebuffer(const std::string & name)
{
    return _new<vka::framebuffer>(name);
}


vka::managed_buffer*   vka::context::new_managed_buffer(const std::string & name,
                          size_t size,
                          vk::MemoryPropertyFlags memory_properties,
                          vk::BufferUsageFlags usage)
{

    auto * b = new_managed_buffer(name);
    if( b )
    {
        b->set_memory_properties(memory_properties);
        b->set_size(size);
        b->set_usage(usage);
        b->create();
        return b;
    }
    return nullptr;
}

vka::buffer*   vka::context::new_buffer(const std::string & name,
                          size_t size,
                          vk::MemoryPropertyFlags memory_properties,
                          vk::BufferUsageFlags usage)
{

    auto * b = new_buffer(name);
    if( b )
    {
        b->set_memory_properties(memory_properties);
        b->set_size(size);
        b->set_usage(usage);
        b->create();
        return b;
    }
    return nullptr;
}

vka::buffer* vka::context::new_buffer(const std::string & name)
{
        return _new<vka::buffer>(name);
}

vka::managed_buffer* vka::context::new_managed_buffer(const std::string & name)
{
        return _new<vka::managed_buffer>(name);
}

vka::shader* vka::context::new_shader_module(const std::string &name)
{
    return _new<vka::shader>(name);
}


vka::pipeline* vka::context::new_pipeline(const std::string &name)
{
    auto * R =  _new<vka::pipeline>(name);
    if( R )
    {
        R->set_scissor( vk::Rect2D({0,0}, m_extent) );
        R->set_viewport( vk::Viewport(0,0, m_extent.width,m_extent.height,0,1));
    }

    return R;
}

vka::semaphore *vka::context::new_semaphore(const std::string &name)
{
    return _new<vka::semaphore>(name);
}

vka::texture *vka::context::new_texture(const std::string &name)
{
    return _new<vka::texture>(name);
}

vka::texture2d *vka::context::new_texture2d(const std::string &name)
{
    return _new<vka::texture2d>(name);
}


vka::texture2darray *vka::context::new_texture2darray(const std::string &name)
{
    return _new<vka::texture2darray>(name);
}

vka::texture * vka::context::new_depth_texture(const std::string & name, vk::ImageUsageFlags flags)
{
    auto * t = new_texture(name);

    auto format = find_depth_format();

    t->set_format( format );
    t->set_usage( flags | vk::ImageUsageFlagBits::eDepthStencilAttachment );
    t->set_memory_properties( vk::MemoryPropertyFlagBits::eDeviceLocal);
    t->set_tiling( vk::ImageTiling::eOptimal);
    t->set_view_type(vk::ImageViewType::e2D);
    return t;
}

vk::Format vka::context::find_depth_format()
{
    return find_supported_format(
    {vk::Format::eD32Sfloat      ,// VK_FORMAT_D32_SFLOAT,
     vk::Format::eD32SfloatS8Uint,// VK_FORMAT_D32_SFLOAT_S8_UINT,
     vk::Format::eD24UnormS8Uint},  //VK_FORMAT_D24_UNORM_S8_UINT},

                vk::ImageTiling::eOptimal,// VK_IMAGE_TILING_OPTIMAL,
                vk::FormatFeatureFlagBits::eDepthStencilAttachment// VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
                );
}

vk::Format vka::context::find_supported_format(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{

    for (vk::Format format : candidates)
    {

        vk::FormatProperties props = get_physical_device().getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear
                && (props.linearTilingFeatures & features) == features)
        {
            LOG << "Depth Format selected: " << vk::to_string(format) << ENDL;
            return format;
        }
        else if (tiling == vk::ImageTiling::eOptimal
                 && (props.optimalTilingFeatures & features) == features)
        {
            LOG << "Depth Format selected: " << vk::to_string(format) << ENDL;
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}



vka::texture2d *vka::context::new_texture2d_host_visible(const std::string &name)
{
    auto * staging_texture = new_texture2d(name);
    //staging_texture->set_size(512,512);
    staging_texture->set_tiling(vk::ImageTiling::eLinear);
    staging_texture->set_usage(  vk::ImageUsageFlagBits::eSampled  | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc );
    staging_texture->set_memory_properties( vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    staging_texture->set_format(vk::Format::eR8G8B8A8Unorm);
    staging_texture->set_view_type(vk::ImageViewType::e2D);
    staging_texture->set_mipmap_levels(1);
    //staging_texture->create();
    //staging_texture->create_image_view(vk::ImageAspectFlagBits::eColor);
    return staging_texture;
}
vka::descriptor_set_layout *vka::context::new_descriptor_set_layout(const std::vector<vk::DescriptorSetLayoutBinding> &bindings)
{
    auto it = m_DescriptorSetLayouts.find( bindings );


    if( it == m_DescriptorSetLayouts.end() )
    {
        static int i=0;
        std::string name = "descriptor_set_" + std::to_string(i++);
        auto * ds = new_descriptor_set_layout(name);

        ds->set_bindings( bindings );
        ds->create();
        m_DescriptorSetLayouts[bindings] = ds;
        LOG << "New descriptor set created" << ENDL;
        return ds;
    }
    LOG << "New descriptor already created, returning previously created set" << ENDL;
    return it->second;
}

vka::command_pool* vka::context::get_command_pool()
{
    return m_command_pool;
}

void vka::context::submit_command_buffer(const vk::CommandBuffer &p_CmdBuffer,
                                         const vka::semaphore * wait_semaphore,
                                         const vka::semaphore * signal_semaphore,
                                         vk::PipelineStageFlags wait_stage)
{
    vk::SubmitInfo submitInfo;

    if( wait_semaphore)
    {
        submitInfo.waitSemaphoreCount   = 1;
        submitInfo.pWaitSemaphores      = &wait_semaphore->m_semaphore;
    }
    submitInfo.pWaitDstStageMask    = &wait_stage;


    if( signal_semaphore)
    {
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = &signal_semaphore->m_semaphore;
    }


    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &p_CmdBuffer;


    if( m_graphics_queue.submit( 1, &submitInfo,  m_render_fence ) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to submit draw command buffer");
    }


    auto s = std::chrono::system_clock::now();
    vk::Result r;
    do
    {
        r = m_device.waitForFences( m_render_fence, true, 100000000);
    } while(  r == vk::Result::eTimeout);

    m_device.resetFences( m_render_fence );
}

vka::screen* vka::context::new_screen(const std::string & name)
{
    return _new<vka::screen>(name);
}

vka::offscreen_target* vka::context::new_offscreen_target(const std::string & name)
{
    return _new<vka::offscreen_target>(name);
}

vka::renderpass* vka::context::new_renderpass(const std::string & name)
{
    return _new<vka::renderpass>(name);
}

vka::descriptor_pool* vka::context::new_descriptor_pool(const std::string & name)
{
    return _new<vka::descriptor_pool>(name);
}

vka::descriptor_set_layout *vka::context::new_descriptor_set_layout(const std::string &name)
{
    return _new<vka::descriptor_set_layout>(name);
}

vka::command_pool* vka::context::new_command_pool(const std::string & name)
{
    //vka::command_pool* new_command_pool(const std::string & name);
    if( registry_t<command_pool>::get_object(name) == nullptr)
    {
        std::shared_ptr<vka::command_pool> R( new vka::command_pool(this), vka::deleter<vka::command_pool>() );

        //====
        vk::CommandPoolCreateInfo poolInfo;

        poolInfo.queueFamilyIndex = m_queue_family.graphics;
        poolInfo.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer; // Optional

        R->m_command_pool = get_device().createCommandPool(poolInfo);

        if( !R->m_command_pool )
        {
            throw std::runtime_error("Failed to create command pool!");
        }
        LOG << "Command Pool created" << ENDL;
                //=====
        registry_t<command_pool>::insert_object(name, R);

        return R.get();
    }

    return nullptr;
}

//uint32_t vka::context::get_next_image_index(vka::semaphore *signal_semaphore)
//{
//    return  m_device.acquireNextImageKHR( m_swapchain,
//                                          std::numeric_limits<uint64_t>::max(),
//                                          *signal_semaphore,
//                                          vk::Fence()).value;
//}

// void vka::context::present_image(uint32_t image_index,  vka::semaphore * wait_semaphore)
// {
//     ///vk::Semaphore signalSemaphores[] = { m_render_finished_smaphore};
//
//     // uint32_t nextIndex = GetNextImageIndex();
//     //std::cout << "Next Image index: " << image_index << std::endl;
//     //=== finally present the image ====
//     vk::PresentInfoKHR presentInfo;
//     if( wait_semaphore)
//     {
//         presentInfo.waitSemaphoreCount  = 1;
//         presentInfo.pWaitSemaphores     = &wait_semaphore->get();
//     }
//
//     vk::SwapchainKHR swapChains[] = { m_swapchain };
//     presentInfo.swapchainCount    = 1;
//     presentInfo.pSwapchains       = swapChains;
//     presentInfo.pImageIndices     = &image_index;
//     presentInfo.pResults = nullptr;
//
//     m_present_queue.presentKHR( presentInfo );
// }

void vka::context::present_image(const vk::PresentInfoKHR & info)
{
    m_present_queue.presentKHR( info );
}

std::vector<vk::ImageView>  vka::context::create_image_views( std::vector<vk::Image> const & images, vk::Format image_format)
{
    std::vector<vk::ImageView>   m_ImageViews;

    m_ImageViews.reserve( images.size() );
    auto size = images.size();

    for (uint32_t i = 0; i < size; i++)
    {
        vk::ImageViewCreateInfo createInfo;

        createInfo.setImage      ( images[i] )
                .setViewType     ( vk::ImageViewType::e2D )
                .setFormat       ( image_format )
                .setComponents(
                    vk::ComponentMapping(
                        vk::ComponentSwizzle::eIdentity,
                        vk::ComponentSwizzle::eIdentity,
                        vk::ComponentSwizzle::eIdentity,
                        vk::ComponentSwizzle::eIdentity)
                    );

        createInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel   = 0;
        createInfo.subresourceRange.levelCount     = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount     = 1;


        auto IV = m_device.createImageView( createInfo );
        if(!IV)
        {
            throw std::runtime_error("failed to create image view!");
        }
        m_ImageViews.push_back(IV);

    }

    return m_ImageViews;
}

void vka::context::clean()
{
#define X_MACRO(A) registry_t<A>::clear();
X_LIST
#undef X_MACRO

    if( m_render_fence )
    {
        m_device.destroyFence(m_render_fence);
    }

    if( m_device)
    {
        LOG << "Destroying logical device: " << m_device << ENDL;
        m_device.destroy();
    }

    if( m_instance )
    {
        LOG << "Destroying Instance" << ENDL;
        if( m_callback)
        {
            ExtDispatcher.vkDestroyDebugReportCallbackEXT(m_instance, m_callback, nullptr);
            LOG << "Debug Report callback destroyed" << ENDL;
        }
        m_instance.destroy();
        m_instance = vk::Instance();
    }
}



bool vka::context::can_present_to_surface(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface)
{
    auto capabilities = device.getSurfaceCapabilitiesKHR( surface );
    auto formats      = device.getSurfaceFormatsKHR(      surface );
    auto presentModes = device.getSurfacePresentModesKHR( surface );

    return !formats.empty() && !presentModes.empty();
}


vka::queue_family_index_t vka::context::find_queue_families(const vk::PhysicalDevice &device, const vk::SurfaceKHR &surface)
{
    queue_family_index_t indices;

    auto queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (const vk::QueueFamilyProperties & queueFamily : queueFamilies)
    {
        if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) ) {
            indices.graphics = i;
        }


        VkBool32 presentSupport = false;
        device.getSurfaceSupportKHR(i, surface, &presentSupport);

        if( queueFamily.queueCount > 0 && presentSupport)
        {
            indices.present = i;
        }

        if (indices.isComplete())
        {
            break;
        }

        i++;
    }

    return indices;
}


/**
 * @brief vka::context::are_device_extensions_supported
 * @param device
 * @param deviceExtensions
 * @return
 *
 * Checks if all the device extensions in the input paramters are supported by
 * the device
 */
bool vka::context::are_device_extensions_supported(const vk::PhysicalDevice &device, const std::vector<const char *> &deviceExtensions)
{
    std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();


    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    if(requiredExtensions.size() )
    {
        WARN << "The following extensions are not supported: ";
        for(auto & r : requiredExtensions)
        {
            std::cout << r << "  ,";
        }
        std::cout <<ENDL;
    }
    return requiredExtensions.empty();
}

/**
 * @brief vka::context::check_validation_layer_support
 * @return
 *
 * Checks if all the validation layers are available in in the vulkan
 * implementation provided by the driver
 */
bool vka::context::check_validation_layer_support()
{
    auto available_layers = vk::enumerateInstanceLayerProperties();

    for(auto & layers : available_layers)
    {
        INFO << "Validation Layer: " << layers.layerName << ENDL;
    }

    for (const char* layerName : m_required_validation_layers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : available_layers)
        {
            if( strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            ERROR <<  "Validation layers requested but not available: " << layerName << ENDL;
            return false;
        }
    }

    return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
{
    std::cerr << "validation layer: " << msg << std::endl;

    return VK_FALSE;
}


void vka::context::setup_debug_callback()
{
    if (!m_enable_validation_layers) return;


    vk::DebugReportCallbackCreateInfoEXT createInfo;

    createInfo.setFlags( vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning)
              .setPfnCallback( debugCallback );


    if(
    ExtDispatcher.vkCreateDebugReportCallbackEXT(m_instance,
                                                 reinterpret_cast<VkDebugReportCallbackCreateInfoEXT const*>(&createInfo),
                                                 nullptr,
                                                 reinterpret_cast<VkDebugReportCallbackEXT*>(&m_callback) )
            != VK_SUCCESS)
    {
        ERROR <<"failed to set up debug callback!" << ENDL;
        throw std::runtime_error("failed to set up debug callback!");
    }

    if (!m_callback)
    {
        ERROR << "Failed to set up the debug callback function" << ENDL;
        throw std::runtime_error("failed to set up debug callback!");
    }
    LOG << "Debug Callback created" << ENDL;
}


void vka::context::enable_extension( const std::string & extension)
{
    m_instance_extensions.push_back(extension);
}
void vka::context::enable_validation_layer( const std::string & layer_name)
{
    m_validation_layers.push_back(layer_name);
}
void vka::context::enable_device_extension(const std::string & extension)
{
    m_device_extensions.push_back(extension);
}


