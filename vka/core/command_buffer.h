#ifndef VKA_COMMAND_BUFFER_H
#define VKA_COMMAND_BUFFER_H

#include <vulkan/vulkan.hpp>

namespace vka
{

class sub_buffer;

class command_buffer : public vk::CommandBuffer
{
    public:

    command_buffer(const vk::CommandBuffer & C) : vk::CommandBuffer(C)
    {

    }


    /**
     * @brief bindVertexSubBuffer
     * @param firstBinding
     * @param buffers
     *
     * Binds a vka::sub_buffer
     */
    void bindVertexSubBuffer( uint32_t firstBinding,
                              sub_buffer const * buffer, vk::DeviceSize offset=0 ) const;

    void bindIndexSubBuffer( sub_buffer const * buffers, vk::IndexType index_type, vk::DeviceSize offset=0 ) const;


    void copySubBuffer( vk::Buffer srcBuffer, sub_buffer const * dstBuffer, const vk::BufferCopy & region ) const;

    void copySubBuffer( sub_buffer const * srcBuffer, sub_buffer const * dstBuffer, const vk::BufferCopy & region ) const;

};

}

#endif
