#ifndef VKA_TEXTURE_H
#define VKA_TEXTURE_H

#include <vulkan/vulkan.hpp>
#include "deleter.h"
#include "context_child.h"
#include "device_memory.h"
#include <map>

namespace vka
{

class context;
class buffer;

class texture : public context_child
{

public:
    uint32_t get_layers() const;
    void set_layers(uint32_t l);
    void set_size(vk::DeviceSize w, vk::DeviceSize h, vk::DeviceSize d);
    vk::Extent3D get_extents() const;
    void set_format(vk::Format F);
    vk::Format get_format() const;
    void set_tiling(vk::ImageTiling T);
    vk::ImageTiling get_tiling() const;
    void set_view_type(vk::ImageViewType type);
    void set_mipmap_levels(uint32_t levels);
    uint32_t get_mipmap_levels() const;
    void set_memory_properties(vk::MemoryPropertyFlags flags);
    vk::ImageUsageFlags get_usage() const;
    void set_usage(vk::ImageUsageFlags flags);

    texture* set_mag_filter(vk::Filter f)
    {
        m_SamplerInfo.magFilter = f;
        return this;
    }
    vk::Filter get_mag_filter() const
    {
        return m_SamplerInfo.magFilter;
    }
    texture* set_min_filter(vk::Filter f)
    {
        m_SamplerInfo.minFilter = f;
        return this;
    }
    vk::Filter get_min_filter() const
    {
        return m_SamplerInfo.minFilter;
    }
    texture* set_address_mode(vk::SamplerAddressMode u,
                              vk::SamplerAddressMode v,
                              vk::SamplerAddressMode w)
    {
        m_SamplerInfo.addressModeU = u;
        m_SamplerInfo.addressModeV = v;
        m_SamplerInfo.addressModeW = w;
        return this;
    }

    vk::Image get_image() const
    {
        return m_Image;
    }

    vk::ImageView get_image_view() const
    {
        return m_View;
    }

    vk::Sampler get_sampler() const
    {
        return m_Sampler;
    }

    void create();
    void create_image_view(const vk::ImageViewCreateInfo &view_info);
    void create_image_view(vk::ImageAspectFlags flags);

    void create_sampler();
    void create_sampler(const vk::SamplerCreateInfo & create_info);

    vk::ImageViewType get_view_type();


    /**
     * @brief copy_buffer
     * @param b
     *
     * Copies data from the buffer to the image.
     */
    void copy_buffer( vk::CommandBuffer, vka::buffer *b , vk::BufferImageCopy);
    void copy_buffer( vka::buffer const * b , vk::BufferImageCopy);


    /**
     * @brief convert
     * @param commandBuffer
     * @param new_layout
     * @param srcStageMask
     * @param dstStageMask
     *
     * Converts the entire texture into a new layout. All layers/mip levels
     * but already be the same layout before it can be changed.
     */
    void convert(vk::CommandBuffer commandBuffer,
                 vk::ImageLayout new_layout,
                 vk::PipelineStageFlags srcStageMask=vk::PipelineStageFlagBits::eTopOfPipe,
                 vk::PipelineStageFlags dstStageMask=vk::PipelineStageFlagBits::eTopOfPipe);


    // converts the image into another layout by writing the approrpiate
    // command into the command buffer
    void convert(vk::CommandBuffer commandBuffer,
                 vk::ImageLayout old_layout,
                 vk::ImageLayout new_layout,
                 const vk::ImageSubresourceRange &range,
                 vk::PipelineStageFlags srcStageMask=vk::PipelineStageFlagBits::eTopOfPipe,
                 vk::PipelineStageFlags dstStageMask=vk::PipelineStageFlagBits::eTopOfPipe);



    void convert(vk::ImageLayout new_layout,
                 vk::PipelineStageFlags srcStageMask=vk::PipelineStageFlagBits::eTopOfPipe,
                 vk::PipelineStageFlags dstStageMask=vk::PipelineStageFlagBits::eTopOfPipe);

    /**
     * @brief convert_layer
     * @param commandBuffer
     * @param layout
     * @param layer
     * @param level
     * @param srcStageMask
     * @param dstStageMask
     *
     * Converts a specific layer to another layout by writing the appropriate
     * command into the command buffer. Note that after this function is called
     * any calls to get_layout( layer, level) will return the newly converted
     * layout even if the command buffer has not been submitted.
     */
    void convert_layer(vk::CommandBuffer commandBuffer,
                       vk::ImageLayout layout,
                       uint32_t layer,
                       uint32_t level,
                       vk::PipelineStageFlags srcStageMask=vk::PipelineStageFlagBits::eTopOfPipe,
                       vk::PipelineStageFlags dstStageMask=vk::PipelineStageFlagBits::eTopOfPipe);


    void blit_mipmap(vk::CommandBuffer &cmdBuff, uint32_t layer, uint32_t src_miplevel, uint32_t dst_miplevel);

    // get the layout of a specific layer and mipmap level
    vk::ImageLayout get_layout(uint32_t layer=0, uint32_t mip_level=0) const;

    void* map_memory();
    void  unmap_memory();



protected:
    bool has_stencil_component(vk::Format format);

     texture(context * parent);
    ~texture();

     uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    vk::Image               m_Image;

    // a map of all the layer's layouts
    std::map< uint32_t, // layer
                       std::map< uint32_t, // mipmap
                                 vk::ImageLayout> >  m_Layout;

    vka::image_memory        m_Memory;


    vk::ImageViewCreateInfo m_ViewInfo;
    vk::ImageView           m_View;

    vk::SamplerCreateInfo   m_SamplerInfo;
    vk::Sampler             m_Sampler;



    vk::ImageCreateInfo     m_CreateInfo;

    //void  * m_Mapped = nullptr;

    friend class context;
    friend class deleter<texture>;
};

}

#endif
