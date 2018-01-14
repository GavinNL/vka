#ifndef VKA_DESCRIPTOR_SET_H
#define VKA_DESCRIPTOR_SET_H

#include <vulkan/vulkan.hpp>
#include "context_child.h"
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

    descriptor_set_layout* clear_bindings() { m_DescriptorSetLayoutBindings.clear(); return this;}


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

class descriptor_set : public context_child
{
public:
    CONTEXT_CHILD_DEFAULT_CONSTRUCTOR(descriptor_set)

    ~descriptor_set();



    void create();


    //==========================================

private:
    vk::DescriptorSet m_descriptor_set;

    std::map<vk::DescriptorType, vk::DescriptorPoolSize> m_pools;

    friend class context;
    friend class deleter<descriptor_set>;
};

}

#endif
