#ifndef VKA_COMMAND_BUFFER_H
#define VKA_COMMAND_BUFFER_H

#include <vulkan/vulkan.hpp>

namespace vka
{

class buffer;
class sub_buffer;
class pipeline;
class descriptor_set;
class texture;

class PushDescriptorInfo
{
public:
    PushDescriptorInfo()
    {
    }

    PushDescriptorInfo & attach(uint32_t binding, uint32_t count, vka::texture * texArray);
    PushDescriptorInfo & attach(uint32_t binding, uint32_t count, vka::sub_buffer * sub_buffer);

    std::vector<vk::WriteDescriptorSet>   m_writes;
};

class command_buffer : public vk::CommandBuffer
{
    public:

    command_buffer(){}

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


    void pushDescriptorSet( vk::PipelineBindPoint bind_point, vka::pipeline * pipeline, uint32_t set, vka::PushDescriptorInfo const & Info);

    void bindDescriptorSet( vk::PipelineBindPoint pipelineBindPoint,
                            vka::pipeline const * pipeline,
                            uint32_t firstSet,
                            vka::descriptor_set const * set ) const;

    void bindDescriptorSet( vk::PipelineBindPoint pipelineBindPoint,
                            vka::pipeline const * pipeline,
                            uint32_t firstSet,
                            vka::descriptor_set const * set,
                            uint32_t dynamic_offset) const;
};

}

#endif
