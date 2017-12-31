#ifndef VKA_RENDERPASS_H
#define VKA_RENDERPASS_H

#include <vulkan/vulkan.hpp>
#include "deleter.h"

namespace vka
{
class context;

class renderpass
{
    private:
    vk::RenderPass                     m_RenderPass;
    vk::AttachmentDescription          m_ColorAttach;
    vk::AttachmentReference            m_ColorRef;
    vk::AttachmentDescription          m_DepthAttach;
    vk::AttachmentReference            m_DepthRef;
    std::vector<vk::SubpassDependency> m_SubpassDependency;

    context * m_parent_context = nullptr;

    ~renderpass();
    public:

    renderpass()
    {
        m_DepthRef.attachment =  std::numeric_limits< decltype(m_DepthRef.attachment) >::max();
        m_ColorRef.attachment =  std::numeric_limits< decltype(m_ColorRef.attachment) >::max();
    }


    void attach_color(vk::Format f = vk::Format::eR8G8B8A8Unorm);

    void attach(vk::AttachmentDescription a);

    void create(context & device);

    private:

    friend class context;
    friend class deleter<renderpass>;
};

}

#endif
