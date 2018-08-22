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
    descriptor_set_layout(context * C) : context_child(C)
    {

    }

    ~descriptor_set_layout();

    void clear();

    operator vk::DescriptorSetLayout()
    {
        return m_descriptor_set_layout;
    }

    vk::DescriptorSetLayout const & get() const {
        return m_descriptor_set_layout;
    }


    descriptor_set_layout* clearBindings() { m_DescriptorSetLayoutBindings.clear(); return this;}


    descriptor_set_layout* setBindings(std::vector<vk::DescriptorSetLayoutBinding> const & bindings)
    {
        m_DescriptorSetLayoutBindings = bindings;
        return this;
    }

    descriptor_set_layout*  addTextureLayoutBinding(uint32_t binding, vk::ShaderStageFlags stages);

    descriptor_set_layout*  addUniformLayoutBinding(uint32_t binding, vk::ShaderStageFlags stages);

    descriptor_set_layout*  addDynamicUniformLayoutBinding(uint32_t binding, vk::ShaderStageFlags stages);

    descriptor_set_layout*  setFlags(vk::DescriptorSetLayoutCreateFlags flags)
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

};

using DescriptorSetLayout_p = std::shared_ptr<descriptor_set_layout>;


}

#endif
