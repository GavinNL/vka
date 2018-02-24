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
    vk::AttachmentReference              m_DepthReference;


    std::vector<vk::AttachmentDescription> m_AttachmentDescription;
    vk::AttachmentDescription              m_DepthAttachmentDescription;

    vk::SubpassDescription             m_SubpassDescriptions;

    vk::RenderPassCreateInfo           m_CreateInfo;

    renderpass(context * parent) : context_child(parent)
    {
        m_DepthAttachmentDescription.format         = vk::Format::eUndefined;
        m_DepthAttachmentDescription.samples        = vk::SampleCountFlagBits::e1;      // VK_SAMPLE_COUNT_1_BIT;
        m_DepthAttachmentDescription.loadOp         = vk::AttachmentLoadOp::eClear;     // VK_ATTACHMENT_LOAD_OP_CLEAR;
        m_DepthAttachmentDescription.storeOp        = vk::AttachmentStoreOp::eStore;    // VK_ATTACHMENT_STORE_OP_STORE;
        m_DepthAttachmentDescription.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;  // VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        m_DepthAttachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare; // VK_ATTACHMENT_STORE_OP_DONT_CARE;
        m_DepthAttachmentDescription.initialLayout  = vk::ImageLayout::eUndefined;      // VK_IMAGE_LAYOUT_UNDEFINED;
        m_DepthAttachmentDescription.finalLayout    = vk::ImageLayout::ePresentSrcKHR;  // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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

    //===============================

    renderpass* set_num_color_attachments(uint32_t n);

    vk::AttachmentDescription & get_color_attachment_description(uint32_t i)
    {
        return m_AttachmentDescription.at(i);
    }
    vk::AttachmentDescription & get_depth_attachment_description()
    {
        return m_DepthAttachmentDescription;
    }

    renderpass* set_color_attachment_layout( uint32_t i, vk::ImageLayout L)
    {
        m_ColorReferences.at(i).layout = L;
        return this;
    }
    renderpass* set_depth_attachment_layout( vk::ImageLayout L)
    {
        m_DepthReference.layout = L;
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
