#include <vka/core/context.h>
#include <vka/core/renderpass.h>
#include <vka/core/command_pool.h>
#include <vka/core/buffer.h>
#include <vka/core/framebuffer.h>
#include <vka/core/shader.h>
#include <vka/core/pipeline.h>
#include <vka/core/semaphore.h>
#include <vka/core/texture.h>
#include <vka/core/descriptor_pool.h>
#include <vka/core/descriptor_set.h>
#include <vulkan/vulkan.hpp>
#include <vka/core/context_child.h>
#include <GLFW/glfw3.h>
#include <set>


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
    return _new<vka::framebuffer>(name);
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



void vka::context::submit_command_buffer(const vk::CommandBuffer &p_CmdBuffer,
                                         const vka::semaphore * wait_semaphore,
                                         const vka::semaphore * signal_semaphore,
                                         vk::PipelineStageFlags wait_stage)
{
    vk::SubmitInfo submitInfo;

    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &wait_semaphore->m_semaphore;
    submitInfo.pWaitDstStageMask    = &wait_stage;


    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &signal_semaphore->m_semaphore;


    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &p_CmdBuffer;


    if( m_graphics_queue.submit( 1, &submitInfo,  m_render_fence ) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to submit draw command buffer");
    }


    vk::Result r;
    do
    {
        r = m_device.waitForFences( m_render_fence, true, 100000000);
    } while(  r == vk::Result::eTimeout);

    m_device.resetFences( m_render_fence );
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

uint32_t vka::context::get_next_image_index(vka::semaphore *signal_semaphore)
{
    return  m_device.acquireNextImageKHR( m_swapchain,
                                          std::numeric_limits<uint64_t>::max(),
                                          *signal_semaphore,
                                          vk::Fence()).value;
}

void vka::context::present_image(uint32_t image_index,  vka::semaphore * wait_semaphore)
{
    ///vk::Semaphore signalSemaphores[] = { m_render_finished_smaphore};

    // uint32_t nextIndex = GetNextImageIndex();
    //std::cout << "Next Image index: " << image_index << std::endl;
    //=== finally present the image ====
    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount  = 1;
    presentInfo.pWaitSemaphores     = &wait_semaphore->get();

    vk::SwapchainKHR swapChains[] = { m_swapchain };
    presentInfo.swapchainCount    = 1;
    presentInfo.pSwapchains       = swapChains;
    presentInfo.pImageIndices     = &image_index;
    presentInfo.pResults = nullptr;

    m_present_queue.presentKHR( presentInfo );
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

    for(auto & image_view : m_image_views)
    {
        m_device.destroyImageView(image_view);
    }
    m_image_views.clear();


    if( m_swapchain)
        m_device.destroySwapchainKHR(m_swapchain);
    m_images.clear();

    //if( m_image_available_smaphore)
    //    m_device.destroySemaphore( m_image_available_smaphore );
    //
    //if( m_render_finished_smaphore)
    //    m_device.destroySemaphore( m_render_finished_smaphore );

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


