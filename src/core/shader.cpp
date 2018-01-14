#include <vka/core/context.h>
#include <vka/core/shader.h>
#include <fstream>

vka::shader::~shader()
{
    if(m_shader)
    {
        get_device().destroyShaderModule(m_shader);
    }
}

std::string readFile(const std::string &filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if ( !file.is_open() )
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t fileSize = (size_t) file.tellg();
    std::string buffer;
    buffer.resize(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

void vka::shader::load_from_file(const std::string & path)
{
    std::string source_code = readFile(path);
    load_from_memory(source_code);
}


void vka::shader::load_from_memory(const std::string  &SPIRV_code)
{

    // std::string code( SPIRV_code.begin(), SPIRV_code.end());
    //
    // auto it = ScreenData.m_Shaders.find(code);
    // if( it != ScreenData.m_Shaders.end() )
    // {
    //     LOG << "Shader previously loaded. Using old reference" << ENDL;
    //     reset( it->second);
    //     return;
    // }

    vk::ShaderModuleCreateInfo createInfo;

    createInfo.codeSize = SPIRV_code.size();

    std::vector<uint32_t> codeAligned(SPIRV_code.size() / sizeof(uint32_t)+1 );

    memcpy( codeAligned.data(), SPIRV_code.data(), SPIRV_code.size() );
    createInfo.pCode = codeAligned.data();

    m_shader = get_device().createShaderModule( createInfo );

    if( !m_shader)
    {
        throw std::runtime_error("Failed to create shader modeule");
    }

//    ScreenData.m_Shaders[code] = get_shared();
}
