#ifndef VKA_TEXTURE2D_ARRAY_H
#define VKA_TEXTURE2D_ARRAY_H

#include "texture.h"

namespace vka
{

class context;
class buffer;

class texture2darray : public texture
{

public:

    void set_size(vk::DeviceSize w, vk::DeviceSize h)
    {
        texture::set_size(w,h,1);
    }


    /**
     * @brief copy
     * @param cm
     * @param t
     *
     * Copies texture t into this texture by writing the appropriate
     * commands into the command buffer.
     */
    void copy(vk::CommandBuffer cm, texture2d* t, vk::Offset2D dstOffset, vk::Offset2D srcOffset, vk::Extent2D extent);


protected:
    texture2darray(vka::context * parent) : texture(parent)
    {
        set_memory_properties(vk::MemoryPropertyFlagBits::eDeviceLocal);
        set_tiling(vk::ImageTiling::eOptimal);
        set_format(vk::Format::eR8G8B8A8Unorm);
        set_view_type(vk::ImageViewType::e2DArray);
        set_usage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc);
        set_layers(1u);
    }

    using texture::set_size;
    using texture::set_view_type;

    friend class context;
};

}

#endif
