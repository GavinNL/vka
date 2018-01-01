#include <vka/core/context.h>
#include <vka/core/renderpass.h>
#include <vka/core/command_pool.h>
#include <vka/core/buffer.h>
#include <vka/core/framebuffer.h>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <set>

void vka::context::init()
{
    LOG << "Initializing vka context" << ENDL;

    if(m_enable_validation_layers && !check_validation_layer_support() )
    {
        throw std::runtime_error("Validation layers required but not available");
    }


    auto required_extensions = get_required_extensions();

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

    setup_debug_callback();

}


void vka::context::create_window_surface( GLFWwindow * window )
{
    if (glfwCreateWindowSurface( m_instance, window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&m_surface) ) != VK_SUCCESS)
    {
        ERROR << "Failed to create window surface!" << ENDL;
        throw std::runtime_error("failed to create window surface!");
    }
}

void vka::context::create_device()
{
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
    for(auto & d : m_required_device_extensions)
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
    for(auto & s : m_required_device_extensions)
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


void vka::context::create_swap_chain(vk::Extent2D extents)
{

    //

    if( m_swapchain)
    {
        throw std::runtime_error("swap chain already created");
    }
    if( !m_device)
    {
        throw std::runtime_error("device not created");
    }
    if( !m_surface)
    {
        throw std::runtime_error("Surface not created\n");
    }

    LOG << "Creating swaphain" << ENDL;
    m_swapchain_capabilities = m_physical_device.getSurfaceCapabilitiesKHR(m_surface);
    m_swapchain_available_formats = m_physical_device.getSurfaceFormatsKHR(     m_surface);
    m_swapchain_available_present_modes = m_physical_device.getSurfacePresentModesKHR(m_surface);

    //============= Choose teh appropriate present mode =============
    m_swapchain_present_mode = vk::PresentModeKHR::eFifo;
    for (const auto& availablePresentMode : m_swapchain_available_present_modes)
    {
        if (availablePresentMode ==vk::PresentModeKHR::eMailbox)
        {
            m_swapchain_present_mode=  availablePresentMode;
            break;
        }
        else if (availablePresentMode ==vk::PresentModeKHR::eImmediate)
        {
            m_swapchain_present_mode = availablePresentMode;
            break;
        }
    }
    //================= Choose the appropriate format==================
    if( m_swapchain_available_formats.size()==1 && m_swapchain_available_formats.front().format == vk::Format::eUndefined)
    {
        m_swapchain_format.format     = vk::Format::eR8G8B8A8Unorm;
        m_swapchain_format.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    } else {
        m_swapchain_format = m_swapchain_available_formats.front();
        for (const auto& availableFormat : m_swapchain_available_formats)
        {
            if (availableFormat.format == vk::Format::eR8G8B8A8Unorm && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                m_swapchain_format = availableFormat;
            }
        }
    }
    LOG << "Format     Selected: " << vk::to_string(m_swapchain_format.format) << ENDL;
    LOG << "Color SpaceSelected: " << vk::to_string(m_swapchain_format.colorSpace) << ENDL;

    //================== Choose the appropriate extent ====================
    vk::Extent2D   extent  = extents;//chooseSwapExtent(swapChainSupport.capabilities);
    if (m_swapchain_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        extent = m_swapchain_capabilities.currentExtent;
    }
    else
    {
        extent.width  = std::max(m_swapchain_capabilities.minImageExtent.width , std::min(m_swapchain_capabilities.maxImageExtent.width,  extent.width));
        extent.height = std::max(m_swapchain_capabilities.minImageExtent.height, std::min(m_swapchain_capabilities.maxImageExtent.height, extent.height));
    }
    //=========================================================
    uint32_t imageCount = m_swapchain_capabilities.minImageCount+1;
    if ( m_swapchain_capabilities.maxImageCount > 0 && imageCount > m_swapchain_capabilities.maxImageCount)
    {
        imageCount = m_swapchain_capabilities.maxImageCount;
    }
    LOG << "Image count: " << imageCount << ENDL;
    vk::SwapchainCreateInfoKHR createInfo;

    createInfo.setSurface( m_surface )
              .setMinImageCount(imageCount)
              .setImageFormat      (m_swapchain_format.format           )
              .setImageColorSpace  (m_swapchain_format.colorSpace       )
              .setImageExtent      (extent                              )
              .setImageArrayLayers (1                                   )
              .setImageUsage       (vk::ImageUsageFlagBits::eColorAttachment );

    uint32_t QFamilyIndices[] = {(uint32_t)m_queue_family.graphics, (uint32_t)m_queue_family.present };

    if (m_queue_family.graphics != m_queue_family.present)
    {
        createInfo.setImageSharingMode      (vk::SharingMode::eConcurrent)
                .setQueueFamilyIndexCount (2)
                .setPQueueFamilyIndices   (QFamilyIndices);
    } else {
        createInfo.setImageSharingMode      ( vk::SharingMode::eExclusive)
                .setQueueFamilyIndexCount ( 0 ) // Optional
                .setPQueueFamilyIndices   ( nullptr) ; // Optional
    }
    createInfo.setPreTransform( m_swapchain_capabilities.currentTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode( m_swapchain_present_mode )
            .setClipped(true);


    //========== Create the actual swap chain =============================
    m_swapchain = m_device.createSwapchainKHR(createInfo);
    if( !m_swapchain)
    {
        ERROR << "Failed to create swapchain" << ENDL;
        throw std::runtime_error("Failed to create swapchain");
    }


    m_images       = m_device.getSwapchainImagesKHR(m_swapchain);
    m_image_format = createInfo.imageFormat;
    m_extent       = createInfo.imageExtent;

    m_image_views = create_image_views(m_images, m_image_format);
    LOG << "Image Views created" << ENDL;
}


vka::framebuffer* vka::context::new_framebuffer(const std::string & name)
{
    if( registry_t<framebuffer>::get_object(name) == nullptr)
    {
        std::shared_ptr<vka::framebuffer> R(new vka::framebuffer, vka::deleter<vka::framebuffer>() );

        R->m_parent_context = this;
        registry_t<framebuffer>::insert_object(name, R);

        return R.get();
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
    if( registry_t<buffer>::get_object(name) == nullptr)
    {
        std::shared_ptr<vka::buffer> R(new vka::buffer, vka::deleter<vka::buffer>() );

        R->m_parent_context = this;
        registry_t<buffer>::insert_object(name, R);

        return R.get();
    }

    return nullptr;
}

vka::renderpass* vka::context::new_renderpass(const std::string & name)
{
    if( registry_t<renderpass>::get_object(name) == nullptr)
    {
        std::shared_ptr<vka::renderpass> R( new vka::renderpass, vka::deleter<vka::renderpass>() );
        R->m_parent_context = this;
        registry_t<renderpass>::insert_object(name, R);

        return R.get();
    }

    return nullptr;
}

vka::command_pool* vka::context::new_command_pool(const std::string & name)
{
    //vka::command_pool* new_command_pool(const std::string & name);
    if( registry_t<command_pool>::get_object(name) == nullptr)
    {
        std::shared_ptr<vka::command_pool> R( new vka::command_pool, vka::deleter<vka::command_pool>() );
        R->m_parent_context = this;

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
    registry_t<vka::buffer>::clear();
    registry_t<vka::command_pool>::clear();
    registry_t<vka::renderpass>::clear();
    registry_t<vka::framebuffer>::clear();

    for(auto & image_view : m_image_views)
    {
        m_device.destroyImageView(image_view);
    }
    m_image_views.clear();


    if( m_swapchain)
        m_device.destroySwapchainKHR(m_swapchain);
    m_images.clear();

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
            DestroyDebugReportCallbackEXT(m_instance, m_callback, nullptr);
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

    std::vector<vk::LayerProperties> m_LayerProperties = vk::enumerateInstanceLayerProperties();

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

std::vector<const char*> vka::context::get_required_extensions()
{
    std::vector<const char *> extensions;

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (unsigned int i = 0; i < glfwExtensionCount; i++)
    {
        extensions.push_back(glfwExtensions[i]);
    }

    if ( m_enable_validation_layers )
    {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    return extensions;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
{
    std::cerr << "validation layer: " << msg << std::endl;

    return VK_FALSE;
}

VkResult vka::context::CreateDebugReportCallbackEXT( vk::Instance & instance,
                                       const vk::DebugReportCallbackCreateInfoEXT & pCreateInfo,
                                       const VkAllocationCallbacks* pAllocator,
                                       vk::DebugReportCallbackEXT & pCallback)
{
    auto func = (PFN_vkCreateDebugReportCallbackEXT) instance.getProcAddr("vkCreateDebugReportCallbackEXT");
    if (func != nullptr) {
        return func(instance,
                    reinterpret_cast<VkDebugReportCallbackCreateInfoEXT const*>(&pCreateInfo),
                    pAllocator,
                    reinterpret_cast<VkDebugReportCallbackEXT*>(&pCallback) );
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void vka::context::DestroyDebugReportCallbackEXT(vk::Instance const & instance,
                                   vk::DebugReportCallbackEXT & callback,
                                   const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT) instance.getProcAddr("vkDestroyDebugReportCallbackEXT");
    if (func != nullptr)
    {
        func(instance,
             callback,
             pAllocator);
    }
}

void vka::context::setup_debug_callback()
{
    if (!m_enable_validation_layers) return;


    vk::DebugReportCallbackCreateInfoEXT createInfo;

    createInfo.setFlags( vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning)
              .setPfnCallback( debugCallback );


    if (CreateDebugReportCallbackEXT(m_instance, createInfo, nullptr, m_callback) != VK_SUCCESS)
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
