#include <vka/core/context.h>
#include <vka/core/semaphore.h>
#include <vka/core2/DescriptorPool.h>
#include <vka/core/descriptor_set_layout.h>
#include <vka/core2/DescriptorSet.h>
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
    return m_parent->getDevice();
}

vk::PhysicalDevice  vka::context_child::get_physical_device()
{
    return m_parent->getPhysicalDevice();
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

    //==========================================================================

    setup_debug_callback();


}


void vka::context::createDevice( vk::SurfaceKHR surface_to_use)
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

                    createLogicalDevice(m_physical_device, m_queue_family);


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

                    return;
                }
            }
        }
    }

}

void vka::context::createLogicalDevice(vk::PhysicalDevice & p_physical_device, const vka::queue_family_index_t & p_Qfamily)
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
    deviceFeatures.tessellationShader = 1;
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


vka::Semaphore_p vka::context::createSemaphore()
{
    std::shared_ptr<vka::Semaphore> s(new vka::Semaphore(this));

    return s;
}


vk::Format vka::context::findDepthFormat()
{
    return findSupportedFormat(
    {vk::Format::eD32Sfloat      ,// VK_FORMAT_D32_SFLOAT,
     vk::Format::eD32SfloatS8Uint,// VK_FORMAT_D32_SFLOAT_S8_UINT,
     vk::Format::eD24UnormS8Uint},  //VK_FORMAT_D24_UNORM_S8_UINT},

                vk::ImageTiling::eOptimal,// VK_IMAGE_TILING_OPTIMAL,
                vk::FormatFeatureFlagBits::eDepthStencilAttachment// VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
                );
}

vk::Format vka::context::findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{

    for (vk::Format format : candidates)
    {

        vk::FormatProperties props = getPhysicalDevice().getFormatProperties(format);

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


std::shared_ptr<vka::descriptor_set_layout> vka::context::createDescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding> &bindings, vk::DescriptorSetLayoutCreateFlags flags)
{
    auto it = m_DescriptorSetLayouts.find( bindings );


    if( it == m_DescriptorSetLayouts.end() )
    {
        static int i=0;
        std::string name = "descriptor_set_" + std::to_string(i++);

        auto ds = std::make_shared<vka::descriptor_set_layout>(this);
        //auto * ds = new_descriptor_set_layout(name);

        ds->setFlags(flags);
        ds->setBindings( bindings );
        ds->create();

        m_DescriptorSetLayouts[bindings] = ds;
        LOG << "New descriptor set created" << ENDL;
        return ds;
    }
    LOG << "New descriptor already created, returning previously created set" << ENDL;
    return it->second;
}

void vka::context::submitCommandBuffer(vk::CommandBuffer b)
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

void vka::context::submitCommandBuffer(const vk::CommandBuffer &p_CmdBuffer,
                                       const std::shared_ptr<vka::Semaphore> & wait_semaphore,
                                       const std::shared_ptr<vka::Semaphore> & signal_semaphore,
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


void vka::context::presentImage(const vk::PresentInfoKHR & info)
{
    m_present_queue.presentKHR( info );
}

std::vector<vk::ImageView>  vka::context::createImageViews( std::vector<vk::Image> const & images, vk::Format image_format)
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

    for(auto & l : m_DescriptorSetLayouts)
    {
        l.second->clear();
    }

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


void vka::context::enableExtension( const std::string & extension)
{
    m_instance_extensions.push_back(extension);
}
void vka::context::enableValidationLayer( const std::string & layer_name)
{
    m_validation_layers.push_back(layer_name);
}
void vka::context::enableDeviceExtension(const std::string & extension)
{
    m_device_extensions.push_back(extension);
}


