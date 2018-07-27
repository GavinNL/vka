#pragma once
#ifndef VKA_CORE2_RENDERPASS_H
#define VKA_CORE2_RENDERPASS_H

#include <vka/core/context_child.h>

namespace vka
{

class RenderPass : public context_child
{
public:
    RenderPass( context * C) : context_child(C)
    {

    }

    ~RenderPass() {}


    RenderPass & operator = ( RenderPass const & other)
    {
        if( this != & other)
        {

        }
        return *this;
    }

    RenderPass & operator = ( RenderPass && other)
    {
        if( this != & other)
        {

        }
        return *this;
    }


    //=====================================================

    void AttachColor(vk::Format f = vk::Format::eR8G8B8A8Unorm);
    void AttachDepth(vk::Format f);

    vk::AttachmentDescription & GetColorAttachmentDescription(uint32_t i)
    {
        return m_AttachmentDescription.at(i);
    }
    vk::AttachmentDescription & GetDepthAttachmentDescription()
    {
        return m_DepthAttachmentDescription;
    }



    RenderPass* SetNumColorAttachments(uint32_t n);

    RenderPass* SetColorAttachmentLayout( uint32_t i, vk::ImageLayout L)
    {
        m_ColorReferences.at(i).layout = L;
        return this;
    }
    RenderPass* SetDepthAttachmentLayout( vk::ImageLayout L)
    {
        m_DepthReference.layout = L;
        return this;
    }
    RenderPass* AddSubpassdependency(const vk::SubpassDependency & D)
    {
        m_SubpassDependency.push_back(D);
        return this;
    }

    void Create();

    vk::RenderPass get() const
    {
        return m_RenderPass;
    }
protected:
    vk::RenderPass                         m_RenderPass;

    std::vector<vk::SubpassDependency>     m_SubpassDependency;

    std::vector<vk::AttachmentReference>   m_ColorReferences;
    vk::AttachmentReference                m_DepthReference;

    std::vector<vk::AttachmentDescription> m_AttachmentDescription;
    vk::AttachmentDescription              m_DepthAttachmentDescription;

    vk::SubpassDescription                 m_SubpassDescriptions;
    vk::RenderPassCreateInfo               m_CreateInfo;

};

}

#endif
