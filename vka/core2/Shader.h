#ifndef VKA_SHADER_H
#define VKA_SHADER_H

#include <vulkan/vulkan.hpp>

#include <vka/core/context_child.h>

namespace vka
{

class context;
class Pipeline;






class Shader : public context_child
{
    public:

    vk::ShaderModule m_shader;

    Shader(context * c) : context_child(c)
    {
    }

    ~Shader();

    public:

    /**
     * @brief load_from_memory
     * @param source
     *
     * Load a SPIR-V shader from source
     */
    void loadFromMemory(const std::string & spv_source);


    /**
     * @brief load_from_file
     * @param path
     *
     * Load a shader from a path. This can be spirv or glsls.
     *
     * path names must end in one of the following
     * extensions:
     *
     * spv, vert, frag, geom, tesc, tese
     */
    void loadFromFile(const std::string & path);


    operator vk::ShaderModule() const
    {
        return m_shader;
    }

    vk::ShaderModule getShaderModule()
    {
        return m_shader;
    }

    std::string readFile(const std::string &filename);

    friend class deleter<shader>;
    friend class context;
    friend class pipeline;
};

using Shader_p = std::shared_ptr<Shader>;

}

#endif
