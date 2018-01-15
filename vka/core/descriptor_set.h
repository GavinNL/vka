#ifndef VKA_DESCRIPTOR_SET_H
#define VKA_DESCRIPTOR_SET_H

#include <vulkan/vulkan.hpp>
#include "context_child.h"
#include "classes.h"
#include <map>


namespace vka
{

class descriptor_set_layout : public context_child
{
public:
    CONTEXT_CHILD_DEFAULT_CONSTRUCTOR(descriptor_set_layout)

    ~descriptor_set_layout();

    operator vk::DescriptorSetLayout()
    {
        return m_descriptor_set_layout;
    }

    vk::DescriptorSetLayout const & get() const {
        return m_descriptor_set_layout;
    }


    descriptor_set_layout* clear_bindings() { m_DescriptorSetLayoutBindings.clear(); return this;}


    descriptor_set_layout* set_bindings(std::vector<vk::DescriptorSetLayoutBinding> const & bindings)
    {
        m_DescriptorSetLayoutBindings = bindings;
        return this;
    }

    descriptor_set_layout*  add_texture_layout_binding(uint32_t binding, vk::ShaderStageFlags stages);

    descriptor_set_layout*  add_uniform_layout_binding(uint32_t binding, vk::ShaderStageFlags stages);

    descriptor_set_layout*  add_dynamic_uniform_layout_binding(uint32_t binding, vk::ShaderStageFlags stages);

    void create();


    //==========================================

private:
    vk::DescriptorSetLayout                     m_descriptor_set_layout;
    std::vector<vk::DescriptorSetLayoutBinding> m_DescriptorSetLayoutBindings;

    friend class context;
    friend class deleter<descriptor_set_layout>;
};

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
    vka::descriptor_set *attach_sampler(uint32_t index, vka::texture *texture);
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
