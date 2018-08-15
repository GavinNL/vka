#ifndef VKA_DESCRIPTOR_SET_LAYOUT_H
#define VKA_DESCRIPTOR_SET_LAYOUT_H

#pragma once

#include <vulkan/vulkan.hpp>
#include <vka/core/context_child.h>
#include "classes.h"
#include <map>


namespace vka
{

class sub_buffer;

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

    descriptor_set_layout*  set_flags(vk::DescriptorSetLayoutCreateFlags flags)
    {
        m_Flags = flags;
        return this;
    }

    void create();


    //==========================================

private:
    vk::DescriptorSetLayout                     m_descriptor_set_layout;
    std::vector<vk::DescriptorSetLayoutBinding> m_DescriptorSetLayoutBindings;
    vk::DescriptorSetLayoutCreateFlags          m_Flags;

    friend class context;
    friend class deleter<descriptor_set_layout>;
};



}

#endif
