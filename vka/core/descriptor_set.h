#ifndef VKA_DESCRIPTOR_SET_H
#define VKA_DESCRIPTOR_SET_H

#include <vulkan/vulkan.hpp>
#include "context_child.h"
#include "classes.h"
#include <map>


namespace vka
{

class sub_buffer;
class SubBuffer;

struct DescriptorInfo
{
    enum {None, DynamicBuffer, Buffer, Image} type;
    vk::DescriptorBufferInfo  buffer;
    vk::DescriptorImageInfo   image;
};


class descriptor_set : public context_child
{
public:
    CONTEXT_CHILD_DEFAULT_CONSTRUCTOR(descriptor_set)

    ~descriptor_set();


    operator vk::DescriptorSet()
    {
        return m_descriptor_set;
    }

    vk::DescriptorSet const & get() const
    {
        return m_descriptor_set;
    }
    vk::DescriptorSet  & get()
    {
        return m_descriptor_set;
    }

    void create(std::vector< vk::DescriptorSetLayoutBinding > const & bindings);



    //==========================================
    void update();
    vka::descriptor_set * attach_sampler(uint32_t index, vka::texture *texture);
    vka::descriptor_set * attach_uniform_buffer(uint32_t index, const buffer *buff, vk::DeviceSize size, vk::DeviceSize offset);
    vka::descriptor_set * attach_uniform_buffer(uint32_t index, const sub_buffer *buff, vk::DeviceSize size, vk::DeviceSize offset);

    vka::descriptor_set * attach_dynamic_uniform_buffer(uint32_t index, const buffer *buff, vk::DeviceSize size, vk::DeviceSize offset);
    vka::descriptor_set * attach_dynamic_uniform_buffer(uint32_t index, const sub_buffer *buff, vk::DeviceSize size, vk::DeviceSize offset);




    vka::descriptor_set* AttachUniformBuffer(uint32_t index,
                                             std::shared_ptr<SubBuffer> & sub_buffer ,
                                             vk::DeviceSize size,
                                             vk::DeviceSize offset=0);
private:
    vk::DescriptorSet     m_descriptor_set;
    vka::descriptor_pool *m_parent_pool = nullptr;
    std::vector< vk::DescriptorSetLayoutBinding > m_bindings;
    std::map<uint32_t, DescriptorInfo>    m_DescriptorInfos;


    friend class context;
    friend class deleter<descriptor_set>;
    friend class descriptor_pool;
};

}

#endif
