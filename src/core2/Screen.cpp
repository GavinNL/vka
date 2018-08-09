#include <vka/core2/Screen.h>

#include <vka/core/semaphore.h>
#include <vka/core/context.h>
#include <vka/core/texture.h>

#include <vka/core/types.h>

namespace vka
{



Screen::Screen(context * parent) : context_child(parent),
                                   m_DepthPool(parent)
{
    m_clear_values[0] = vk::ClearValue( vk::ClearColorValue( std::array<float,4>{0.0f, 0.f, 0.f, 1.f} ) );
    m_clear_values[1] = vk::ClearValue( vk::ClearDepthStencilValue(1.0f,0) ) ;
}

Screen::~Screen()
{
    auto device = get_device();

    // destroy the framebuffers and the image views
    for(auto & fb : m_Swapchain.framebuffer) device.destroyFramebuffer(fb);
    for(auto & fb : m_Swapchain.view) device.destroyImageView(fb);


    // destroy the render pass
    if(m_renderpass) device.destroyRenderPass(m_renderpass);

    // destroy the swapchain
    if( m_Swapchain.swapchain) device.destroySwapchainKHR(m_Swapchain.swapchain);
}


vk::RenderPass Screen::CreateRenderPass(vk::Device device, vk::Format swapchain_format, vk::Format depth_format)
{
    // This example will use a single render pass with one subpass

    // Descriptors for the attachments used by this renderpass
    std::array<vk::AttachmentDescription, 2> attachments = {};

    // Color attachment
    attachments[0].format         = swapchain_format;									// Use the color format selected by the swapchain
    attachments[0].samples        = vk::SampleCountFlagBits::e1;//VK_SAMPLE_COUNT_1_BIT;									// We don't use multi sampling in this example
    attachments[0].loadOp         = vk::AttachmentLoadOp::eClear;//VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear this attachment at the start of the render pass
    attachments[0].storeOp        = vk::AttachmentStoreOp::eStore;//VK_ATTACHMENT_STORE_OP_STORE;							// Keep it's contents after the render pass is finished (for displaying it)
    attachments[0].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;//VK_ATTACHMENT_LOAD_OP_DONT_CARE;					// We don't use stencil, so don't care for load
    attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;// VK_ATTACHMENT_STORE_OP_DONT_CARE;				// Same for store
    attachments[0].initialLayout  = vk::ImageLayout::eUndefined;//VK_IMAGE_LAYOUT_UNDEFINED;						// Layout at render pass start. Initial doesn't matter, so we use undefined
    attachments[0].finalLayout    = vk::ImageLayout::ePresentSrcKHR;//VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;					// Layout to which the attachment is transitioned when the render pass is finished
                                                                             // As we want to present the color buffer to the swapchain, we transition to PRESENT_KHR


    attachments[1].format         = depth_format;									// Use the color format selected by the swapchain
    attachments[1].samples        = vk::SampleCountFlagBits::e1;//VK_SAMPLE_COUNT_1_BIT;									// We don't use multi sampling in this example
    attachments[1].loadOp         = vk::AttachmentLoadOp::eClear;//VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear this attachment at the start of the render pass
    attachments[1].storeOp        = vk::AttachmentStoreOp::eDontCare;//VK_ATTACHMENT_STORE_OP_STORE;							// Keep it's contents after the render pass is finished (for displaying it)
    attachments[1].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;//VK_ATTACHMENT_LOAD_OP_DONT_CARE;					// We don't use stencil, so don't care for load
    attachments[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;// VK_ATTACHMENT_STORE_OP_DONT_CARE;				// Same for store
    attachments[1].initialLayout  = vk::ImageLayout::eUndefined;//VK_IMAGE_LAYOUT_UNDEFINED;						// Layout at render pass start. Initial doesn't matter, so we use undefined
    attachments[1].finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal;//VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;					// Layout to which the attachment is transitioned when the render pass is finished
                                                                             // As we want to present the color buffer to the swapchain, we transition to PRESENT_KHR

    // Depth attachment
//    attachments[1].format = depthFormat;											// A proper depth format is selected in the example base
//    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
//    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at start of first subpass
//    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;						// We don't need depth after render pass has finished (DONT_CARE may result in better performance)
//    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;					// No stencil
//    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;				// No Stencil
//    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;						// Layout at render pass start. Initial doesn't matter, so we use undefined
//    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;	// Transition to depth/stencil attachment

    // Setup attachment references
    vk::AttachmentReference colorReference;
    colorReference.attachment = 0;													// Attachment 0 is color
    colorReference.layout = vk::ImageLayout::eColorAttachmentOptimal;// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;				// Attachment layout used as color during the subpass

    vk::AttachmentReference depthReference;
    depthReference.attachment = 1;													// Attachment 1 is color
    depthReference.layout =  vk::ImageLayout::eDepthStencilAttachmentOptimal;//VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;		// Attachment used as depth/stemcil used during the subpass

    // Setup a single subpass reference
    vk::SubpassDescription subpassDescription;
    subpassDescription.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;// VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount    = 1;									// Subpass uses one color attachment
    subpassDescription.pColorAttachments       = &colorReference;							// Reference to the color attachment in slot 0
    subpassDescription.pDepthStencilAttachment = &depthReference;					// Reference to the depth attachment in slot 1
    subpassDescription.inputAttachmentCount    = 0;									// Input attachments can be used to sample from contents of a previous subpass
    subpassDescription.pInputAttachments       = nullptr;									// (Input attachments not used by this example)
    subpassDescription.preserveAttachmentCount = 0;									// Preserved attachments can be used to loop (and preserve) attachments through subpasses
    subpassDescription.pPreserveAttachments    = nullptr;								// (Preserve attachments not used by this example)
    subpassDescription.pResolveAttachments     = nullptr;								// Resolve attachments are resolved at the end of a sub pass and can be used for e.g. multi sampling

    // Setup subpass dependencies
    // These will add the implicit ttachment layout transitionss specified by the attachment descriptions
    // The actual usage layout is preserved through the layout specified in the attachment reference
    // Each subpass dependency will introduce a memory and execution dependency between the source and dest subpass described by
    // srcStageMask, dstStageMask, srcAccessMask, dstAccessMask (and dependencyFlags is set)
    // Note: VK_SUBPASS_EXTERNAL is a special constant that refers to all commands executed outside of the actual renderpass)
    std::array<vk::SubpassDependency, 2> dependencies;

    // First dependency at the start of the renderpass
    // Does the transition from final to initial layout
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;								// Producer of the dependency
    dependencies[0].dstSubpass = 0;													// Consumer is our single subpass that will wait for the execution depdendency
    dependencies[0].srcStageMask  = vk::PipelineStageFlagBits::eBottomOfPipe;// VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;// VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;// VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;// VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;// VK_DEPENDENCY_BY_REGION_BIT;

    // Second dependency at the end the renderpass
    // Does the transition from the initial to the final layout
    dependencies[1].srcSubpass      = 0;													// Producer of the dependency is our single subpass
    dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;								// Consumer are all commands outside of the renderpass
    dependencies[1].srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;//VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask    = vk::PipelineStageFlagBits::eBottomOfPipe;//VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask   = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    dependencies[1].dstAccessMask   = vk::AccessFlagBits::eMemoryRead;
    dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    // Create the actual renderpass
    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());		// Number of attachments used by this render pass
    renderPassInfo.pAttachments = attachments.data();								// Descriptions of the attachments used by the render pass
    renderPassInfo.subpassCount = 1;												// We only use one subpass in this example
    renderPassInfo.pSubpasses = &subpassDescription;								// Description of that subpass
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());	// Number of subpass dependencies
    renderPassInfo.pDependencies = dependencies.data();								// Subpass dependencies used by the render pass


    auto Render_Pass = device.createRenderPass(renderPassInfo);
    assert( Render_Pass );

    return Render_Pass;
}

void Screen::CreateSwapchain(vk::PhysicalDevice physicalDevice, vk::Device device, SwapChainData & SC, vk::SurfaceKHR surface, const vk::Extent2D &extent, bool vsync)
{
    vk::SwapchainKHR oldSwapchain = SC.swapchain;

    SC.format = GetSurfaceFormats(physicalDevice, device, surface);

    // Get physical device surface properties and formats
    //VkSurfaceCapabilitiesKHR surfCaps;
    //vk::SurfaceCapabilitiesKHR surfCaps;
    //VK_CHECK_RESULT(fpGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfCaps));
    vk::SurfaceCapabilitiesKHR surfCaps = physicalDevice.getSurfaceCapabilitiesKHR(surface);

    // Get available present modes
    //uint32_t presentModeCount;
    //VK_CHECK_RESULT(fpGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL));
    //std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    //VK_CHECK_RESULT(fpGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));
    auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
    assert(presentModes.size() > 0);


    vk::Extent2D swapchainExtent;// = {};
    // If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
    if (surfCaps.currentExtent.width == (uint32_t)-1)
    {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent = extent;
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfCaps.currentExtent;
      //  *width  = surfCaps.currentExtent.width;
      //  *height = surfCaps.currentExtent.height;
    }


    // Select a present mode for the swapchain

    // The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
    // This mode waits for the vertical blank ("v-sync")
    vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;// VK_PRESENT_MODE_FIFO_KHR;

    // If v-sync is not requested, try to find a mailbox mode
    // It's the lowest latency non-tearing present mode available
    if (!vsync)
    {
        //for (size_t i = 0; i < presentModes.size(); i++)
        for(auto & presentMode : presentModes)
        {
            if (presentMode == vk::PresentModeKHR::eMailbox /*VK_PRESENT_MODE_MAILBOX_KHR*/)
            {
                swapchainPresentMode = vk::PresentModeKHR::eMailbox;// VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if ((swapchainPresentMode != vk::PresentModeKHR::eMailbox ) && (presentMode == vk::PresentModeKHR::eImmediate))
            {
                swapchainPresentMode = vk::PresentModeKHR::eImmediate;
            }
        }
    }

    // Determine the number of images
    uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
    if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
    {
        desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
    }

    // Find the transformation of the surface
    //VkSurfaceTransformFlagsKHR preTransform;
    vk::SurfaceTransformFlagBitsKHR preTransform;

    if (surfCaps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity /*VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR*/)
    {
        // We prefer a non-rotated transform
        preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    }
    else
    {
        preTransform = surfCaps.currentTransform;
    }

    // Find a supported composite alpha format (not all devices support alpha opaque)
    //VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

    // Simply select the first composite alpha format available
    std::vector<vk::CompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
        vk::CompositeAlphaFlagBitsKHR::eOpaque /*VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR*/,
        vk::CompositeAlphaFlagBitsKHR::ePreMultiplied /*VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR*/,
        vk::CompositeAlphaFlagBitsKHR::ePostMultiplied /*VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR*/,
        vk::CompositeAlphaFlagBitsKHR::eInherit /*VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR*/,
    };
    for (auto& compositeAlphaFlag : compositeAlphaFlags) {
        if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
            compositeAlpha = compositeAlphaFlag;
            break;
        };
    }


    //VkSwapchainCreateInfoKHR swapchainCI = {};
    vk::SwapchainCreateInfoKHR swapchainCI;
    swapchainCI.surface         = surface;
    swapchainCI.minImageCount   = desiredNumberOfSwapchainImages;
    swapchainCI.imageFormat     = SC.format.format;
    swapchainCI.imageColorSpace = SC.format.colorSpace;
    swapchainCI.imageExtent     =  swapchainExtent;//{ swapchainExtent.width, swapchainExtent.height };
    swapchainCI.imageUsage      = vk::ImageUsageFlagBits::eColorAttachment;// VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCI.preTransform = (vk::SurfaceTransformFlagBitsKHR)preTransform;
    swapchainCI.imageArrayLayers = 1;
    swapchainCI.imageSharingMode = vk::SharingMode::eExclusive;
    swapchainCI.queueFamilyIndexCount = 0;
    swapchainCI.pQueueFamilyIndices = NULL;
    swapchainCI.presentMode = swapchainPresentMode;
    swapchainCI.oldSwapchain = oldSwapchain;
    // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
    swapchainCI.clipped = VK_TRUE;
    swapchainCI.compositeAlpha = compositeAlpha;

    // Enable transfer source on swap chain images if supported
    if (surfCaps.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferSrc/* VK_IMAGE_USAGE_TRANSFER_SRC_BIT*/) {
        swapchainCI.imageUsage |= vk::ImageUsageFlagBits::eTransferSrc/* VK_IMAGE_USAGE_TRANSFER_SRC_BIT*/;
    }

    // Enable transfer destination on swap chain images if supported
    if (surfCaps.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst/* VK_IMAGE_USAGE_TRANSFER_DST_BIT*/) {
        swapchainCI.imageUsage |= vk::ImageUsageFlagBits::eTransferDst;
    }

    SC.swapchain = device.createSwapchainKHR(swapchainCI);
    if( !SC.swapchain )
    {
        throw std::runtime_error("Error creating swapchain");
    }
    //VK_CHECK_RESULT(fpCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapChain));

    // If an existing swap chain is re-created, destroy the old swap chain
    // This also cleans up all the presentable images
    if (oldSwapchain)
    {
        for (uint32_t i = 0; i < SC.view.size(); i++)
        {
            device.destroyImageView( SC.view[i]);
            //vkDestroyImageView(device, buffers[i].view, nullptr);
        }
        device.destroySwapchainKHR(oldSwapchain);
    }

    auto images = device.getSwapchainImagesKHR(SC.swapchain);
    assert(images.size());
    //VK_CHECK_RESULT(fpGetSwapchainImagesKHR(device, swapChain, &imageCount, NULL));

    // Get the swap chain images
    //images.resize(imageCount);
    //VK_CHECK_RESULT(fpGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data()));

    // Get the swap chain buffers containing the image and imageview
    SC.image.clear();
    SC.view.clear();
    //m_buffers.resize( images.size() );
    for (uint32_t i = 0; i < images.size() ; i++)
    {
        vk::ImageViewCreateInfo colorAttachmentView;

        colorAttachmentView.format = SC.format.format;
        colorAttachmentView.components = {
            vk::ComponentSwizzle::eR,
            vk::ComponentSwizzle::eG,
            vk::ComponentSwizzle::eB,
            vk::ComponentSwizzle::eA
        };
        colorAttachmentView.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;// VK_IMAGE_ASPECT_COLOR_BIT;
        colorAttachmentView.subresourceRange.baseMipLevel   = 0;
        colorAttachmentView.subresourceRange.levelCount     = 1;
        colorAttachmentView.subresourceRange.baseArrayLayer = 0;
        colorAttachmentView.subresourceRange.layerCount     = 1;
        colorAttachmentView.viewType = vk::ImageViewType::e2D;// VK_IMAGE_VIEW_TYPE_2D;

        SC.image.push_back(images[i]);

        colorAttachmentView.image = SC.image.back();//m_buffers[i].image;


        auto view = device.createImageView(colorAttachmentView);
        assert(view);
        SC.view.push_back(view);
        //VK_CHECK_RESULT(vkCreateImageView(device, &colorAttachmentView, nullptr, &buffers[i].view));
    }
}

vk::SurfaceFormatKHR Screen::GetSurfaceFormats(vk::PhysicalDevice physical_device, vk::Device device, vk::SurfaceKHR surface)
{
    vk::SurfaceFormatKHR srfFormat;

    vk::Format        & colorFormat = srfFormat.format;
    vk::ColorSpaceKHR & colorSpace  = srfFormat.colorSpace;

    //auto device = get_device();
    //auto physical_device = get_physical_device();

    auto surfaceFormats = physical_device.getSurfaceFormatsKHR(surface);
    //std::vector<vk::SurfaceFormatKHR> surfaceFormats(formatCount);
    //VK_CHECK_RESULT(fpGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data()));

    // If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
    // there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
    if ((surfaceFormats.size() == 1) && (surfaceFormats[0].format == vk::Format::eUndefined/*VK_FORMAT_UNDEFINED*/))
    {
        colorFormat = vk::Format::eB8G8R8A8Unorm;// VK_FORMAT_B8G8R8A8_UNORM;
        colorSpace  = surfaceFormats[0].colorSpace;
    }
    else
    {
        // iterate over the list of available surface format and
        // check for the presence of VK_FORMAT_B8G8R8A8_UNORM
        bool found_B8G8R8A8_UNORM = false;
        for (auto&& surfaceFormat : surfaceFormats)
        {
            if (surfaceFormat.format == vk::Format::eB8G8R8A8Unorm)
            {
                colorFormat = surfaceFormat.format;
                colorSpace = surfaceFormat.colorSpace;
                found_B8G8R8A8_UNORM = true;
                break;
            }
        }

        // in case VK_FORMAT_B8G8R8A8_UNORM is not available
        // select the first available color format
        if (!found_B8G8R8A8_UNORM)
        {
            colorFormat = surfaceFormats[0].format;
            colorSpace = surfaceFormats[0].colorSpace;
        }
    }

    return srfFormat;
}


// Create a frame buffer for each swap chain image
// Note: Override of virtual function in the base class and called from within VulkanExampleBase::prepare
std::vector<vk::Framebuffer> Screen::CreateFrameBuffers( vk::Device device,
                                                      vk::Extent2D const & extent,
                                                      vk::RenderPass renderpass,
                                                      std::vector<vk::ImageView> const & swapchain_views,
                                                      vk::ImageView depth_view)
{
    std::vector<vk::Framebuffer> framebuffers;
    // Create a frame buffer for every image in the swapchain
    //frameBuffers.resize(swapChain.imageCount);

    for (size_t i = 0; i < swapchain_views.size(); i++)
    {
        std::array<vk::ImageView, 2> attachments;
        attachments[0] = swapchain_views[i];									// Color attachment is the view of the swapchain image
        attachments[1] = depth_view;


        vk::FramebufferCreateInfo frameBufferCreateInfo;
        // All frame buffers use the same renderpass setup
        frameBufferCreateInfo.renderPass      = renderpass;
        frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        frameBufferCreateInfo.pAttachments    = attachments.data();
        frameBufferCreateInfo.width           = extent.width;
        frameBufferCreateInfo.height          = extent.height;
        frameBufferCreateInfo.layers          = 1;
        // Create the framebuffer

        auto fb  = device.createFramebuffer(frameBufferCreateInfo);
        assert(fb);
        framebuffers.push_back(fb);
    }

    return framebuffers;
}





uint32_t Screen::GetNextFrameIndex(vka::semaphore *signal_semaphore)
{
    auto
    m_next_frame_index  = get_device().acquireNextImageKHR( m_Swapchain.swapchain,
                                          std::numeric_limits<uint64_t>::max(),
                                          *signal_semaphore,
                                          vk::Fence()).value;
    return m_next_frame_index;
}


void Screen::PresentFrame(uint32_t frame_index, vka::semaphore * wait_semaphore)
{
    vk::PresentInfoKHR presentInfo;
    if( wait_semaphore)
    {
        presentInfo.waitSemaphoreCount  = 1;
        presentInfo.pWaitSemaphores     = &wait_semaphore->get();
    }

    vk::SwapchainKHR swapChains[] = { m_Swapchain.swapchain };
    presentInfo.swapchainCount    = 1;
    presentInfo.pSwapchains       = swapChains;
    presentInfo.pImageIndices     = &frame_index;
    presentInfo.pResults = nullptr;

    get_parent_context()->present_image( presentInfo );
}


}
