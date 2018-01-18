#ifndef VKA_SHADER_H
#define VKA_SHADER_H

#include <vulkan/vulkan.hpp>
#include "log.h"
#include "deleter.h"
#include "context_child.h"

namespace vka
{

class context;
class pipeline;

class shader : public context_child
{
    private:

    vk::ShaderModule m_shader;

    CONTEXT_CHILD_DEFAULT_CONSTRUCTOR(shader)
    ~shader();


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
