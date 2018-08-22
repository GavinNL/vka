#ifndef VKA_DESCRIPTOR_SET_H
#define VKA_DESCRIPTOR_SET_H

#include <vulkan/vulkan.hpp>
#include <vka/core/context_child.h>
#include <vka/core/classes.h>
#include <map>


namespace vka
{

class SubBuffer;
class Texture;
class DescriptorPool;

struct DescriptorInfo
{
    enum {None, DynamicBuffer, Buffer, Image} type;
    vk::DescriptorBufferInfo  buffer;
    vk::DescriptorImageInfo   image;
};


class DescriptorSet : public context_child
{
public:

    DescriptorSet(context * parent) : context_child(parent)
    {
    }

    ~DescriptorSet();


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

    vka::DescriptorSet* AttachUniformBuffer(uint32_t index,
                                             std::shared_ptr<SubBuffer> & sub_buffer ,
                                             vk::DeviceSize size,
                                             vk::DeviceSize offset=0);

    vka::DescriptorSet *AttachDynamicUniformBuffer(uint32_t index,
                                                    std::shared_ptr<SubBuffer> &sub_buffer,
                                                    vk::DeviceSize size,
                                                    vk::DeviceSize offset=0);

    vka::DescriptorSet *AttachSampler(uint32_t index,
                                        std::shared_ptr<Texture> texture,
                                       vk::ImageView view,
                                       vk::Sampler sampler);

    vka::DescriptorSet *AttachSampler(uint32_t index,
                                       std::shared_ptr<vka::Texture> texture,
                                       std::string const & view_name = "default",
                                       std::string const & sampler_name = "default");


private:
    vk::DescriptorSet     m_descriptor_set;
    vka::DescriptorPool *m_parent_pool = nullptr;
    std::vector< vk::DescriptorSetLayoutBinding > m_bindings;
    std::map<uint32_t, DescriptorInfo>    m_DescriptorInfos;


    friend class context;
    friend class DescriptorPool;
};


using DescriptorSet_p = std::shared_ptr<DescriptorSet>;
using DescriptorSet_w = std::shared_ptr<DescriptorSet>;

}

#endif
