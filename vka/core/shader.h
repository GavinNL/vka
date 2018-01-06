#ifndef VKA_SHADER_H
#define VKA_SHADER_H

#include <vulkan/vulkan.hpp>
#include "log.h"
#include "deleter.h"

namespace vka
{

class context;
class pipeline;

class shader
{
    private:

    vk::ShaderModule m_shader;

    shader(){}
    ~shader();

    context * m_parent_context;

    public:

    void load_from_memory(const std::string & source);
    void load_from_file(const std::string & source);

    vk::ShaderModule get_shader_module()
    {
        return m_shader;
    }

    friend class deleter<shader>;
    friend class context;
    friend class pipeline;
};

}

#endif
