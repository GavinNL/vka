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



    std::vector<vk::AttachmentReference> m_ColorReferences;
    vk::AttachmentReference  m_DepthReference;
    std::vector<vk::AttachmentDescription> m_AttachmentDescription;
    vk::SubpassDescription             m_SubpassDescriptions;

    vk::RenderPassCreateInfo           m_CreateInfo;

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

    vk::RenderPass get() const
    {
        return m_RenderPass;
    }

    void create();

    void attach_color(vk::Format f = vk::Format::eR8G8B8A8Unorm);
    void attach_depth(vk::Format f);

    renderpass* add_attachment_description(vk::AttachmentDescription a)
    {
        m_AttachmentDescription.push_back(a);
        return this;
    }

    renderpass* add_color_attachment_reference(uint32_t i, vk::ImageLayout layout)
    {
        m_ColorReferences.push_back( vk::AttachmentReference(i, layout) );
        return this;
    }

    renderpass* add_depth_attachment_reference(uint32_t i, vk::ImageLayout layout)
    {
        m_DepthReference = vk::AttachmentReference(i, layout);
        return this;
    }

    renderpass* add_subpass_dependency(const vk::SubpassDependency & D)
    {
        m_SubpassDependency.push_back(D);
        return this;
    }
    void create(context & device);

private:

    friend class context;
    friend class deleter<renderpass>;
    friend class pipeline;
};

}

#endif
