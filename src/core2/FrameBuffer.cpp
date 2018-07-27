#include <vka/core2/FrameBuffer.h>

namespace vka
{


void FrameBuffer::Create()
{

    std::vector<vk::ImageView> attachments;

    for(auto & t : m_attachments)
        attachments.push_back( t->GetImageView() );

    assert( m_CreateInfo.renderPass );
    //m_CreateInfo.renderPass      = m_renderpass->get();

    m_CreateInfo.layers          = 1;
    m_CreateInfo.pAttachments    = attachments.data();
    m_CreateInfo.attachmentCount = attachments.size();

    m_framebuffer = get_device().createFramebuffer( m_CreateInfo );

    if(!m_framebuffer)
    {
        ERROR << "Error creating frame buffer" << ENDL;
        throw std::runtime_error("Error creating frame buffer");
    }

}


}
