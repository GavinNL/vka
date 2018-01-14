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
    vk::DeviceSize get_layers() const;
    void set_layers(vk::DeviceSize l);
    void set_size(vk::DeviceSize w, vk::DeviceSize h, vk::DeviceSize d);
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

    vk::Image get_image() const
    {
        return m_Image;
    }

    void create();
    void create_image_view(const vk::ImageViewCreateInfo &view_info);
    void create_image_view(vk::ImageAspectFlags flags);


    vk::ImageViewType get_view_type();


    /**
     * @brief copy_buffer
     * @param b
     *
     * Copies data from the buffer to the image.
     */
    void copy_buffer( vk::CommandBuffer, vka::buffer *b , vk::BufferImageCopy);

    // converts the image into another layout by writing the approrpiate
    // command into the command buffer
    void convert(vk::CommandBuffer commandBuffer, vk::ImageLayout old_layout, vk::ImageLayout new_layout, const vk::ImageSubresourceRange &range, vk::PipelineStageFlags srcStageMask=vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlags dstStageMask=vk::PipelineStageFlagBits::eTopOfPipe);

    // converts a specific layer and mipmap level to another layout.
    void convert_layer(vk::CommandBuffer commandBuffer, vk::ImageLayout layout, uint32_t layer, uint32_t level, vk::PipelineStageFlags srcStageMask=vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlags dstStageMask=vk::PipelineStageFlagBits::eTopOfPipe);

    // get the layout of a specific layer and mipmap level
    vk::ImageLayout get_layout(uint32_t layer=0, uint32_t mip_level=0) const;

    void* map_memory();
    void  unmap_memory();

protected:
    bool has_stencil_component(vk::Format format);
private:
     texture(context * parent);
    ~texture();

     uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    vk::Image               m_Image;

    // a map of all the layer's layouts
    std::map< uint32_t, // layer
                       std::map< uint32_t, // mipmap
                                 vk::ImageLayout> >  m_Layout;

    vka::image_memory        m_Memory;
    //vk::DeviceMemory        m_Memory;
    //vk::MemoryRequirements  m_MemoryRequirements;
    //vk::MemoryPropertyFlags m_MemoryProperties;

    vk::ImageViewCreateInfo m_ViewInfo;
    vk::ImageView           m_View;

    vk::ImageCreateInfo     m_CreateInfo;

    //void  * m_Mapped = nullptr;

    friend class context;
    friend class deleter<texture>;
};

}

#endif
