#ifndef VKA_TEXTURE2D_H
#define VKA_TEXTURE2D_H

#include "texture.h"

namespace vka
{

class context;
class buffer;

class texture2d : public texture
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

    /**
     * @brief copy
     * @param t
     *
     * Copies the texture, t, into this texture.  This method will
     * allocate it's own comamnd buffer and execture the commands.
     */
    void copy( texture2d * t);


protected:
    texture2d(vka::context * parent) : texture(parent)
    {
        set_memory_properties(vk::MemoryPropertyFlagBits::eDeviceLocal);
        set_tiling(vk::ImageTiling::eOptimal);
        set_format(vk::Format::eR8G8B8A8Unorm);
        set_view_type(vk::ImageViewType::e2D);
        set_usage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc);
        set_layers(1);
    }

    using texture::set_layers;
    using texture::set_size;
    using texture::set_view_type;

    friend class context;
};

}

#endif
