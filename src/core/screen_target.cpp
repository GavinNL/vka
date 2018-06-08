#include <vka/core/screen_target.h>
#include <vka/core/context.h>
#include <vka/core/texture.h>
#include <vka/core/renderpass.h>
#include <vka/core/framebuffer.h>
#include <vka/core/command_buffer.h>
#include <vka/core/semaphore.h>

void vka::screen::set_extent( vk::Extent2D e)
{
    m_extent = e;
}


void vka::screen::set_surface(vk::SurfaceKHR surface)
{
    m_surface = surface;

    std::string name = get_parent_context()->get_name<vka::screen>(this);

    if( !m_renderpass)
         m_renderpass = get_parent_context()->new_renderpass(  + "_renderpass");
}

vka::screen::screen(context * parent) : context_child(parent)
{

    m_clear_values[0] = vk::ClearValue( vk::ClearColorValue( std::array<float,4>{0.0f, 0.f, 0.f, 1.f} ) );
    m_clear_values[1] = vk::ClearValue( vk::ClearDepthStencilValue(1.0f,0) ) ;
}

vka::screen::~screen()
{
    if( m_image_views.size() )
    {
        for(auto & v : m_image_views)
            get_device().destroyImageView(v);
    }
    if( m_swapchain)
        get_device().destroySwapchainKHR(m_swapchain);
}


void vka::screen::create()
{
    std::string name = get_parent_context()->get_name<vka::screen>(this);;
    auto m_physical_device = get_parent_context()->get_physical_device();
   // auto m_surface         = get_parent_context()->get_surface();

    if( !m_surface)
        throw std::runtime_error("Surface not set");

    auto swapchain_capabilities            = m_physical_device.getSurfaceCapabilitiesKHR(m_surface);
    auto swapchain_available_formats       = m_physical_device.getSurfaceFormatsKHR(     m_surface);
    auto swapchain_available_present_modes = m_physical_device.getSurfacePresentModesKHR(m_surface);

    //============= Choose the appropriate present mode =============
    auto m_swapchain_present_mode = vk::PresentModeKHR::eFifo;
    for (const auto& availablePresentMode : swapchain_available_present_modes)
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
    if( swapchain_available_formats.size()==1 && swapchain_available_formats.front().format == vk::Format::eUndefined)
    {
        m_swapchain_format.format     = vk::Format::eR8G8B8A8Unorm;
        m_swapchain_format.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    } else {
        m_swapchain_format = swapchain_available_formats.front();
        for (const auto& availableFormat : swapchain_available_formats)
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
    vk::Extent2D   extent  = m_extent;//chooseSwapExtent(swapChainSupport.capabilities);
    if (swapchain_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        extent = swapchain_capabilities.currentExtent;
    }
    else
    {
        extent.width  = std::max(swapchain_capabilities.minImageExtent.width , std::min(swapchain_capabilities.maxImageExtent.width,  extent.width));
        extent.height = std::max(swapchain_capabilities.minImageExtent.height, std::min(swapchain_capabilities.maxImageExtent.height, extent.height));
    }
    //=========================================================

    uint32_t imageCount = swapchain_capabilities.minImageCount+1;
    if ( swapchain_capabilities.maxImageCount > 0 && imageCount > swapchain_capabilities.maxImageCount)
    {
        imageCount = swapchain_capabilities.maxImageCount;
    }
    LOG << "Image count: " << imageCount << ENDL;


    //======================== CREATE THE SWAP CHAIN ===========================
    vk::SwapchainCreateInfoKHR createInfo;

    createInfo.setSurface( m_surface )
              .setMinImageCount(imageCount)
              .setImageFormat      (m_swapchain_format.format           )
              .setImageColorSpace  (m_swapchain_format.colorSpace       )
              .setImageExtent      (extent                              )
              .setImageArrayLayers (1                                   )
              .setImageUsage       (vk::ImageUsageFlagBits::eColorAttachment );

    auto m_queue_family = get_parent_context()->get_queue_family();
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
    createInfo.setPreTransform( swapchain_capabilities.currentTransform)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setPresentMode( m_swapchain_present_mode )
            .setClipped(true);


    //========== Create the actual swap chain =============================
    m_swapchain = get_parent_context()->get_device().createSwapchainKHR(createInfo);
    if( !m_swapchain)
    {
        ERROR << "Failed to create swapchain" << ENDL;
        throw std::runtime_error("Failed to create swapchain");
    }

    m_images       = get_device().getSwapchainImagesKHR(m_swapchain);
    m_image_format = createInfo.imageFormat;
    m_extent       = createInfo.imageExtent;

    m_image_views = create_image_views(m_images, m_image_format);

    //======
    m_depth_texture = get_parent_context()->new_depth_texture(name + "_depth_texture");
    m_depth_texture->set_size( m_extent.width, m_extent.height, 1);
    m_depth_texture->create();
    m_depth_texture->create_image_view( vk::ImageAspectFlagBits::eDepth);
    m_depth_texture->convert(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    //===========
    m_renderpass->set_num_color_attachments(1);
    m_renderpass->set_color_attachment_layout(0, vk::ImageLayout::eColorAttachmentOptimal);
    m_renderpass->set_depth_attachment_layout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
    m_renderpass->get_color_attachment_description(0).format      = vk::Format::eB8G8R8A8Unorm;
    m_renderpass->get_color_attachment_description(0).finalLayout = vk::ImageLayout::ePresentSrcKHR;

    m_renderpass->get_depth_attachment_description().format = m_depth_texture->get_format();
    m_renderpass->get_depth_attachment_description().finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;;

    m_renderpass->create();

    //================

    std::vector<vk::ImageView> & iv = m_image_views;
    int i=0;
    for(auto & view : iv)
    {
        m_framebuffers.push_back(  get_parent_context()->new_framebuffer( name + "_fb_"+ std::to_string(i++) ) );
        m_framebuffers.back()->create( *m_renderpass, m_extent, view,
                                        m_depth_texture->get_image_view()); // [NEW]
    }
}

std::vector<vk::ImageView>  vka::screen::create_image_views( std::vector<vk::Image> const & images, vk::Format image_format)
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


        auto IV = get_device().createImageView( createInfo );
        if(!IV)
        {
            throw std::runtime_error("failed to create image view!");
        }
        m_ImageViews.push_back(IV);

    }

    return m_ImageViews;
}

uint32_t vka::screen::prepare_next_frame(vka::semaphore *signal_semaphore)
{
    m_next_frame_index  = get_device().acquireNextImageKHR( m_swapchain,
                                          std::numeric_limits<uint64_t>::max(),
                                          *signal_semaphore,
                                          vk::Fence()).value;
    return m_next_frame_index;
}
uint32_t vka::screen::get_next_image_index(vka::semaphore *signal_semaphore)
{
    m_next_frame_index  = get_device().acquireNextImageKHR( m_swapchain,
                                          std::numeric_limits<uint64_t>::max(),
                                          *signal_semaphore,
                                          vk::Fence()).value;
    return m_next_frame_index;
}

void vka::screen::present_frame(vka::semaphore * wait_semaphore)
{
    vk::PresentInfoKHR presentInfo;
    if( wait_semaphore)
    {
        presentInfo.waitSemaphoreCount  = 1;
        presentInfo.pWaitSemaphores     = &wait_semaphore->get();
    }

    vk::SwapchainKHR swapChains[] = { m_swapchain };
    presentInfo.swapchainCount    = 1;
    presentInfo.pSwapchains       = swapChains;
    presentInfo.pImageIndices     = &m_next_frame_index;
    presentInfo.pResults = nullptr;

    get_parent_context()->present_image( presentInfo );
}


void vka::screen::beginRender(vka::command_buffer & cb , uint32_t frame_index)
{
    m_renderpass_info.renderPass        = *get_renderpass();
    m_renderpass_info.framebuffer       = *m_framebuffers[frame_index];
    m_renderpass_info.renderArea.offset = vk::Offset2D(0,0);
    m_renderpass_info.renderArea.extent = m_extent;
    m_renderpass_info.clearValueCount   = m_clear_values.size();
    m_renderpass_info.pClearValues      = m_clear_values.data();

    cb.beginRenderPass(m_renderpass_info, vk::SubpassContents::eInline);
}

void vka::screen::endRender(vka::command_buffer & cb)
{
    cb.endRenderPass();
}


