#ifndef VKA_DESCRIPTOR_SET_LAYOUT_H
#define VKA_DESCRIPTOR_SET_LAYOUT_H

#pragma once

#include <vulkan/vulkan.hpp>
#include <vka/core/context_child.h>
#include <vka/core/classes.h>
#include <map>


namespace vka
{

class sub_buffer;

class DescriptorLayoutSet : public context_child
{
public:
    DescriptorLayoutSet(context * C) : context_child(C)
    {

    }

    ~DescriptorLayoutSet();

    void clear();

    operator vk::DescriptorSetLayout()
    {
        return m_descriptor_set_layout;
    }

    vk::DescriptorSetLayout const & get() const {
        return m_descriptor_set_layout;
    }


    DescriptorLayoutSet* clearBindings() { m_DescriptorSetLayoutBindings.clear(); return this;}


    DescriptorLayoutSet* setBindings(std::vector<vk::DescriptorSetLayoutBinding> const & bindings)
    {
        m_DescriptorSetLayoutBindings = bindings;
        return this;
    }

    DescriptorLayoutSet*  addTextureLayoutBinding(uint32_t binding, vk::ShaderStageFlags stages);

    DescriptorLayoutSet*  addUniformLayoutBinding(uint32_t binding, vk::ShaderStageFlags stages);

    DescriptorLayoutSet*  addDynamicUniformLayoutBinding(uint32_t binding, vk::ShaderStageFlags stages);

    DescriptorLayoutSet*  setFlags(vk::DescriptorSetLayoutCreateFlags flags)
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

using DescriptorSetLayout_p = std::shared_ptr<DescriptorLayoutSet>;


}

#endif
