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

vka::renderpass *vka::renderpass::set_num_color_attachments(uint32_t n)
{
    uint32_t i=0;
    if( n > m_AttachmentDescription.size() )
    {
        i = m_AttachmentDescription.size();
        m_AttachmentDescription.resize( n );

        vk::AttachmentDescription a;
        a.format         = vk::Format::eUndefined;
        a.samples        = vk::SampleCountFlagBits::e1;      // VK_SAMPLE_COUNT_1_BIT;
        a.loadOp         = vk::AttachmentLoadOp::eClear;     // VK_ATTACHMENT_LOAD_OP_CLEAR;
        a.storeOp        = vk::AttachmentStoreOp::eStore;    // VK_ATTACHMENT_STORE_OP_STORE;
        a.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;  // VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        a.stencilStoreOp = vk::AttachmentStoreOp::eDontCare; // VK_ATTACHMENT_STORE_OP_DONT_CARE;
        a.initialLayout  = vk::ImageLayout::eUndefined;      // VK_IMAGE_LAYOUT_UNDEFINED;
        a.finalLayout    = vk::ImageLayout::ePresentSrcKHR;  // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        for(uint32_t j=i;j<n;j++)
            m_AttachmentDescription[j] = a;

    }

    //m_AttachmentDescription = std::vector<vk::AttachmentDescription>(n, a);

    vk::AttachmentReference AR(-1, vk::ImageLayout::eUndefined);

    m_ColorReferences.resize(n);// = std::vector<vk::AttachmentReference>(n, AR);

    for(i=0;i<n;i++)
        m_ColorReferences[i].attachment = i;

    return this;
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
