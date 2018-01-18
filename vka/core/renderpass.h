#ifndef VKA_RENDERPASS_H
#define VKA_RENDERPASS_H

#include <vulkan/vulkan.hpp>
#include "deleter.h"
#include "context_child.h"

namespace vka
{
class context;
class pipeline;

class renderpass : public context_child
{
    private:
    vk::RenderPass                     m_RenderPass;
    vk::AttachmentDescription          m_ColorAttach;
    vk::AttachmentReference            m_ColorRef;
    vk::AttachmentDescription          m_DepthAttach;
    vk::AttachmentReference            m_DepthRef;
    std::vector<vk::SubpassDependency> m_SubpassDependency;


    renderpass(context * parent) : context_child(parent)
    {
        m_DepthRef.attachment =  std::numeric_limits< decltype(m_DepthRef.attachment) >::max();
        m_ColorRef.attachment =  std::numeric_limits< decltype(m_ColorRef.attachment) >::max();
    }
    ~renderpass();
    public:



    operator vk::RenderPass()
    {
        return m_RenderPass;
    }

    void attach_color(vk::Format f = vk::Format::eR8G8B8A8Unorm);
    void attach_depth(vk::Format f);

    void attach(vk::AttachmentDescription a);

    void create(context & device);

private:

    friend class context;
    friend class deleter<renderpass>;
    friend class pipeline;
};

}

#endif
