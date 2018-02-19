#include <vka/core/context.h>
#include <vka/core/renderpass.h>
#include <vka/core/log.h>

vka::renderpass::~renderpass()
{
  //  if( m_parent_context )
    {
        if( m_RenderPass)
        {
            get_device().destroyRenderPass( m_RenderPass );
            LOG << "Render pass destroyed" << ENDL;
        }
    }
}
void vka::renderpass::attach_color(vk::Format f)
{
    m_ColorAttach.format         = f;//vk::Format::eB8G8R8A8Unorm;// m_Swapchain->image_format();// m_SwapChainImageFormat;// swapChainImageFormat;
    m_ColorAttach.samples        = vk::SampleCountFlagBits::e1;      // VK_SAMPLE_COUNT_1_BIT;
    m_ColorAttach.loadOp         = vk::AttachmentLoadOp::eClear;     // VK_ATTACHMENT_LOAD_OP_CLEAR;
    m_ColorAttach.storeOp        = vk::AttachmentStoreOp::eStore;    // VK_ATTACHMENT_STORE_OP_STORE;
    m_ColorAttach.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;  // VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_ColorAttach.stencilStoreOp = vk::AttachmentStoreOp::eDontCare; // VK_ATTACHMENT_STORE_OP_DONT_CARE;
    m_ColorAttach.initialLayout  = vk::ImageLayout::eUndefined;      // VK_IMAGE_LAYOUT_UNDEFINED;
    m_ColorAttach.finalLayout    = vk::ImageLayout::ePresentSrcKHR;  // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    m_ColorRef.attachment = 0;
    m_ColorRef.layout     = vk::ImageLayout::eColorAttachmentOptimal;// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

void vka::renderpass::attach_depth(vk::Format f)
{
    m_DepthAttach.format         = f;// m_Swapchain->image_format();// m_SwapChainImageFormat;// swapChainImageFormat;
    m_DepthAttach.samples        = vk::SampleCountFlagBits::e1;//VK_SAMPLE_COUNT_1_BIT;
    m_DepthAttach.loadOp         = vk::AttachmentLoadOp::eClear;//VK_ATTACHMENT_LOAD_OP_CLEAR;
    m_DepthAttach.storeOp        = vk::AttachmentStoreOp::eDontCare;// VK_ATTACHMENT_STORE_OP_DONT_CARE;
    m_DepthAttach.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;//VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    m_DepthAttach.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;//VK_ATTACHMENT_STORE_OP_DONT_CARE;
    m_DepthAttach.initialLayout  = vk::ImageLayout::eUndefined;//VK_IMAGE_LAYOUT_UNDEFINED;
    m_DepthAttach.finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal;//VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    m_DepthRef.attachment        = 1;
    m_DepthRef.layout            = vk::ImageLayout::eDepthStencilAttachmentOptimal;// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}


void vka::renderpass::create(vka::context & Context)
{
    vk::SubpassDescription                 SubpassDescriptions;
    std::vector<vk::AttachmentDescription> AttachmentDescriptions;

    uint32_t count=0;

    SubpassDescriptions.pipelineBindPoint    = vk::PipelineBindPoint::eGraphics;

    if( m_ColorRef.attachment != std::numeric_limits< decltype(m_ColorRef.attachment) >::max() )
    {
        AttachmentDescriptions.push_back( m_ColorAttach );

        m_ColorRef.attachment = count++;

        SubpassDescriptions.colorAttachmentCount = 1;
        SubpassDescriptions.pColorAttachments    = &m_ColorRef;
    }

    if( m_DepthRef.attachment != std::numeric_limits< decltype(m_DepthRef.attachment) >::max() )
    {
        AttachmentDescriptions.push_back( m_DepthAttach );

        m_DepthRef.attachment = count++;

        SubpassDescriptions.pDepthStencilAttachment = &m_DepthRef;
    }

    if( m_SubpassDependency.size() ==0 )
    {
        vk::SubpassDependency             SubpassDependencies;
        SubpassDependencies.srcSubpass    = VK_SUBPASS_EXTERNAL;
        SubpassDependencies.dstSubpass    = 0;
        SubpassDependencies.srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        SubpassDependencies.srcAccessMask = vk::AccessFlags();
        SubpassDependencies.dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        SubpassDependencies.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        m_SubpassDependency.push_back(SubpassDependencies);
    }

    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = AttachmentDescriptions.size();
    renderPassInfo.pAttachments    = AttachmentDescriptions.data();// &m_AttachmentDescription;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &SubpassDescriptions;
    renderPassInfo.dependencyCount = m_SubpassDependency.size();
    renderPassInfo.pDependencies   = m_SubpassDependency.data();

    m_RenderPass = Context.get_device().createRenderPass( renderPassInfo );

    if( !m_RenderPass )
    {
        throw std::runtime_error("failed to create render pass!");
    }
    LOG << "Render Pass created with " << count << " attachments" << ENDL;
}


void vka::renderpass::create()
{
    std::vector<vk::AttachmentDescription> AttachmentDescriptions;
    AttachmentDescriptions = m_AttachmentDescription;

    if(  m_DepthAttachmentDescription.format != vk::Format::eUndefined )
    {
        AttachmentDescriptions.push_back( m_DepthAttachmentDescription );
        m_DepthReference.attachment = AttachmentDescriptions.size() - 1;
    }


    m_SubpassDescriptions.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
    m_SubpassDescriptions.colorAttachmentCount    = m_ColorReferences.size();
    m_SubpassDescriptions.pColorAttachments       = m_ColorReferences.data();
    m_SubpassDescriptions.pDepthStencilAttachment = &m_DepthReference;


    m_CreateInfo.attachmentCount = AttachmentDescriptions.size();
    m_CreateInfo.pAttachments    = AttachmentDescriptions.data();
    m_CreateInfo.subpassCount = 1;
    m_CreateInfo.pSubpasses   = &m_SubpassDescriptions;


    if( m_SubpassDependency.size() ==0 )
    {
        vk::SubpassDependency             SubpassDependencies;
        SubpassDependencies.srcSubpass    = VK_SUBPASS_EXTERNAL;
        SubpassDependencies.dstSubpass    = 0;
        SubpassDependencies.srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        SubpassDependencies.srcAccessMask = vk::AccessFlags();
        SubpassDependencies.dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        SubpassDependencies.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        m_SubpassDependency.push_back(SubpassDependencies);
    }

    m_CreateInfo.dependencyCount = m_SubpassDependency.size();
    m_CreateInfo.pDependencies   = m_SubpassDependency.data();




    m_RenderPass = get_device().createRenderPass( m_CreateInfo );

    if( !m_RenderPass )
    {
        throw std::runtime_error("failed to create render pass!");
    }
    LOG << "Render Pass created with " << ENDL;
}
