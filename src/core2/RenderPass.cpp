#include <vka/core2/RenderPass.h>

namespace vka
{

RenderPass *RenderPass::SetNumColorAttachments(uint32_t n)
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

void RenderPass::Create()
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


}

}
