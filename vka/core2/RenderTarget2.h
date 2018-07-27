#pragma once
#ifndef VKA_RENDER_TARGET_3_H
#define VKA_RENDER_TARGET_3_H

#include "TextureMemoryPool.h"
#include <vulkan/vulkan.hpp>

namespace vka
{

class RenderTarget2 : public context_child
{
public:
    RenderTarget2(context * parent) :
        context_child(parent),
        m_ColorPool(parent),
        m_DepthPool(parent)
    {

    }
    ~RenderTarget2() {}


    RenderTarget2 & operator = ( RenderTarget2 && other)
    {
        if( this != & other)
        {

        }
        return *this;
    }

    void Create()
    {

        vk::Extent2D extent(1024,768);
        m_Extent = extent;

        m_ClearValues.resize(4);
        m_ClearValues[0].color = vk::ClearColorValue{ std::array<float,4>{ 0.0f, 0.0f, 0.0f, 0.0f } };
        m_ClearValues[1].color = vk::ClearColorValue{ std::array<float,4>{ 0.0f, 0.0f, 0.0f, 0.0f } };
        m_ClearValues[2].color = vk::ClearColorValue{ std::array<float,4>{ 0.0f, 0.0f, 0.0f, 0.0f } };
        m_ClearValues[3].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };

        // Color attachments


        // (World space) Positions

        createColorAttachment( vk::Format::eR32G32B32A32Sfloat, extent, nullptr );
        createColorAttachment( vk::Format::eR32G32B32A32Sfloat, extent, nullptr );
        createColorAttachment( vk::Format::eR8G8B8A8Unorm,   extent, nullptr );

        // Depth attachment

        createDepthAttachment( vk::Format::eD32Sfloat, extent );

        // Set up separate renderpass with references to the color and depth attachments
        std::array<vk::AttachmentDescription, 4> attachmentDescs = {};

        // Init attachment properties
        for (uint32_t i = 0; i < 4; ++i)
        {
            attachmentDescs[i].samples        = vk::SampleCountFlagBits::e1;      // VK_SAMPLE_COUNT_1_BIT;
            attachmentDescs[i].loadOp         = vk::AttachmentLoadOp::eClear;     // VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachmentDescs[i].storeOp        = vk::AttachmentStoreOp::eStore;    // VK_ATTACHMENT_STORE_OP_STORE;
            attachmentDescs[i].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;  // VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachmentDescs[i].stencilStoreOp = vk::AttachmentStoreOp::eDontCare; // VK_ATTACHMENT_STORE_OP_DONT_CARE;
            if (i == 3)
            {
                attachmentDescs[i].initialLayout = vk::ImageLayout::eUndefined;// VK_IMAGE_LAYOUT_UNDEFINED;
                attachmentDescs[i].finalLayout   = vk::ImageLayout::eDepthStencilAttachmentOptimal;//  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
            else
            {
                attachmentDescs[i].initialLayout = vk::ImageLayout::eUndefined;             //VK_IMAGE_LAYOUT_UNDEFINED;
                attachmentDescs[i].finalLayout   = vk::ImageLayout::eShaderReadOnlyOptimal; //VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
        }

        // Formats
        attachmentDescs[0].format = m_images[0]->GetFormat();  // offScreenFrameBuf.position.format;
        attachmentDescs[1].format = m_images[1]->GetFormat();  //offScreenFrameBuf.normal.format;
        attachmentDescs[2].format = m_images[2]->GetFormat();  //offScreenFrameBuf.albedo.format;
        attachmentDescs[3].format = m_depth_image->GetFormat();// offScreenFrameBuf.depth.format;

        std::vector<vk::AttachmentReference> colorReferences;
        colorReferences.push_back({ 0, vk::ImageLayout::eColorAttachmentOptimal });
        colorReferences.push_back({ 1, vk::ImageLayout::eColorAttachmentOptimal });
        colorReferences.push_back({ 2, vk::ImageLayout::eColorAttachmentOptimal });

        vk::AttachmentReference depthReference = {};
        depthReference.attachment = 3;
        depthReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;// VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        vk::SubpassDescription subpass;// = {};
        subpass.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;// VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.pColorAttachments       = colorReferences.data();
        subpass.colorAttachmentCount    = static_cast<uint32_t>(colorReferences.size());
        subpass.pDepthStencilAttachment = &depthReference;

        // Use subpass dependencies for attachment layput transitions
        std::array<vk::SubpassDependency, 2> dependencies;

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;//  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;// VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;//VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;// VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion; //VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
        dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        dependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
        dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

        vk::RenderPassCreateInfo renderPassInfo;// = {};
        renderPassInfo.pAttachments = attachmentDescs.data();
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 2;
        renderPassInfo.pDependencies = dependencies.data();

        m_RenderPass = get_device().createRenderPass( renderPassInfo );
        assert(m_RenderPass);
        //VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &offScreenFrameBuf.renderPass));

        std::array<vk::ImageView,4> attachments;
        attachments[0] = m_images[0]->GetImageView();  //offScreenFrameBuf.position.view;
        attachments[1] = m_images[1]->GetImageView();  //offScreenFrameBuf.normal.view;
        attachments[2] = m_images[2]->GetImageView();  //offScreenFrameBuf.albedo.view;
        attachments[3] = m_depth_image->GetImageView();  //offScreenFrameBuf.depth.view;

        vk::FramebufferCreateInfo fbufCreateInfo;// = {};
        fbufCreateInfo.pNext = NULL;
        fbufCreateInfo.renderPass = m_RenderPass;
        fbufCreateInfo.pAttachments = attachments.data();
        fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        fbufCreateInfo.width  = extent.width;// offScreenFrameBuf.width;
        fbufCreateInfo.height = extent.height;//offScreenFrameBuf.height;
        fbufCreateInfo.layers = 1;

        m_Framebuffer = get_device().createFramebuffer(fbufCreateInfo);
        assert(m_Framebuffer);
        //VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &offScreenFrameBuf.frameBuffer));

    }

    struct FrameBufferAttachment
    {
        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
        vk::Format format;
    };

    void InitMemory()
    {
        m_ColorPool.SetUsage( vk::ImageUsageFlagBits::eColorAttachment  |
                         vk::ImageUsageFlagBits::eSampled);
        m_ColorPool.SetSize( 50*1024*1024 );

        m_DepthPool.SetUsage( vk::ImageUsageFlagBits::eDepthStencilAttachment  |
                         vk::ImageUsageFlagBits::eSampled);
        m_DepthPool.SetSize( 20*1024*1024 );
    }

    void createDepthAttachment( vk::Format format, vk::Extent2D extent)
    {
        m_depth_image = m_DepthPool.AllocateDepthAttachment( extent );
    }


    void createColorAttachment( vk::Format format, vk::Extent2D extent, FrameBufferAttachment *attachment)
    {
        auto image = m_ColorPool.AllocateColorAttachment( format, extent );

        m_images.push_back(image);
    }


    vk::RenderPass GetRenderPass() const
    {
        return m_RenderPass;
    }

    vk::Framebuffer GetFramebuffer() const
    {
        return m_Framebuffer;
    }

    Texture_p GetColorImage(uint32_t i) const
    {
        return m_images.at(i);
    }

    Texture_p GetDepthImage() const
    {
        return m_depth_image;
    }

    std::vector<vk::ClearValue> const & GetClearValues() const
    {
        return m_ClearValues;
    }

    vk::Extent2D GetExtent() const
    {
        return m_Extent;
    }
protected:
    TextureMemoryPool m_ColorPool;
    TextureMemoryPool m_DepthPool;

    vk::RenderPass m_RenderPass;
    vk::Framebuffer m_Framebuffer;

    std::vector<Texture_p> m_images;
    Texture_p              m_depth_image;

    std::vector<vk::ClearValue> m_ClearValues;

    vk::Extent2D  m_Extent;
};

}

#endif
