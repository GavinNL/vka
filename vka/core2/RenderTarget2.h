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
#if 0
        offScreenFrameBuf.width = FB_DIM;
        offScreenFrameBuf.height = FB_DIM;
#else
        vk::Extent2D extent(1024,768);
        m_Extent = extent;

        m_ClearValues.resize(4);
        m_ClearValues[0].color = vk::ClearColorValue{ std::array<float,4>{ 0.0f, 0.0f, 0.0f, 0.0f } };
        m_ClearValues[1].color = vk::ClearColorValue{ std::array<float,4>{ 0.0f, 0.0f, 0.0f, 0.0f } };
        m_ClearValues[2].color = vk::ClearColorValue{ std::array<float,4>{ 0.0f, 0.0f, 0.0f, 0.0f } };
        m_ClearValues[3].depthStencil = vk::ClearDepthStencilValue{ 1.0f, 0 };
#endif
        // Color attachments


        // (World space) Positions
#if 0
        createAttachment(
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            &offScreenFrameBuf.position);

        // (World space) Normals
        ;
        createAttachment(
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            &offScreenFrameBuf.normal);

        // Albedo (color)
        createAttachment(
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            &offScreenFrameBuf.albedo);
#else
        createColorAttachment( vk::Format::eR32G32B32A32Sfloat, extent, nullptr );
        createColorAttachment( vk::Format::eR32G32B32A32Sfloat, extent, nullptr );
        createColorAttachment( vk::Format::eR8G8B8A8Unorm,   extent, nullptr );
#endif
        // Depth attachment
#if 0
        // Find a suitable depth format
        VkFormat attDepthFormat;
        VkBool32 validDepthFormat =  vks::tools::getSupportedDepthFormat(physicalDevice, &attDepthFormat);
        assert(validDepthFormat);

        createAttachment(
            attDepthFormat,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            &offScreenFrameBuf.depth);
#else
        createDepthAttachment( vk::Format::eD32Sfloat, extent );
#endif
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

#if 0
        // Create sampler to sample from the color attachments
        VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
        sampler.magFilter = VK_FILTER_NEAREST;
        sampler.minFilter = VK_FILTER_NEAREST;
        sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.addressModeV = sampler.addressModeU;
        sampler.addressModeW = sampler.addressModeU;
        sampler.mipLodBias = 0.0f;
        sampler.maxAnisotropy = 1.0f;
        sampler.minLod = 0.0f;
        sampler.maxLod = 1.0f;
        sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &colorSampler));
#endif
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
#if 0
        VkImageAspectFlags aspectMask = 0;
        VkImageLayout imageLayout;

        attachment->format = format;

        if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        {
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        assert(aspectMask > 0);
#endif

#if 0
        VkImageCreateInfo image = vks::initializers::imageCreateInfo();
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = format;
        image.extent.width = offScreenFrameBuf.width;
        image.extent.height = offScreenFrameBuf.height;
        image.extent.depth = 1;
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
#else
        m_depth_image = m_DepthPool.AllocateDepthAttachment( extent );
#endif

#if 0
        VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
        VkMemoryRequirements memReqs;

        VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &attachment->image));
        vkGetImageMemoryRequirements(device, attachment->image, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &attachment->mem));
        VK_CHECK_RESULT(vkBindImageMemory(device, attachment->image, attachment->mem, 0));

        VkImageViewCreateInfo imageView = vks::initializers::imageViewCreateInfo();
        imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageView.format = format;
        imageView.subresourceRange = {};
        imageView.subresourceRange.aspectMask = aspectMask;
        imageView.subresourceRange.baseMipLevel = 0;
        imageView.subresourceRange.levelCount = 1;
        imageView.subresourceRange.baseArrayLayer = 0;
        imageView.subresourceRange.layerCount = 1;
        imageView.image = attachment->image;
        VK_CHECK_RESULT(vkCreateImageView(device, &imageView, nullptr, &attachment->view));
#endif

    }


    void createColorAttachment( vk::Format format, vk::Extent2D extent, FrameBufferAttachment *attachment)
    {
#if 0
        VkImageAspectFlags aspectMask = 0;
        VkImageLayout imageLayout;

        attachment->format = format;

        if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        {
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        assert(aspectMask > 0);
#endif

#if 0
        VkImageCreateInfo image = vks::initializers::imageCreateInfo();
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = format;
        image.extent.width = offScreenFrameBuf.width;
        image.extent.height = offScreenFrameBuf.height;
        image.extent.depth = 1;
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
#else
        auto image = m_ColorPool.AllocateColorAttachment( format, extent );
#endif

#if 0
        VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
        VkMemoryRequirements memReqs;

        VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &attachment->image));
        vkGetImageMemoryRequirements(device, attachment->image, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &attachment->mem));
        VK_CHECK_RESULT(vkBindImageMemory(device, attachment->image, attachment->mem, 0));

        VkImageViewCreateInfo imageView = vks::initializers::imageViewCreateInfo();
        imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageView.format = format;
        imageView.subresourceRange = {};
        imageView.subresourceRange.aspectMask = aspectMask;
        imageView.subresourceRange.baseMipLevel = 0;
        imageView.subresourceRange.levelCount = 1;
        imageView.subresourceRange.baseArrayLayer = 0;
        imageView.subresourceRange.layerCount = 1;
        imageView.image = attachment->image;
        VK_CHECK_RESULT(vkCreateImageView(device, &imageView, nullptr, &attachment->view));
#else
    m_images.push_back(image);
#endif

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
