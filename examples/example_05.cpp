/*
 * Example 5: Mip Maps
 *
 * Mipmaps are a downsampled version of a texture. If a square texture has
 * dimensions 512x512, then a level 1 mipmap woudl be 256x256, a level 2 would
 * be 128x128, and so forth all the way down to 1x1.
 *
 * Mipmaps are used when rendering objects far from the camera, if the texture
 * is too detailed, then when an object is far away, two pixels close together
 * might actually be very far apart on the texture. This create large colour
 * variances in objects farther away.
 *
 * When objects are close to the camera, the level 0 mipmap is used at full
 * resoluation. As the object moves farther away, lower levels are used.
 *
 * This example demonstrates how to generate mipmaps for a texture array.
 *
 * We can let the shader choose what mipmap level to use automatically (recomended)
 * or we can manually choose the level we want. We are going to add a
 * variable in the Push Constants to indicate what level we want, or if we choose
 * -1, we will let the shader choose.
 *
 */




#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <vka/core/image.h>
#include <vka/vka.h>

#include <vka/math/linalg.h>

#define WIDTH 1024
#define HEIGHT 768
#define APP_TITLE "Example_05 - Mip Maps"

// This is the vertex structure we are going to use
// it contains a position and a UV coordates field
struct Vertex
{
    glm::vec3 p; // position
    glm::vec2 u; // uv coords
    glm::vec3 n; // normal
};

// This is the structure of the uniform buffer we want.
// it needs to match the structure in the shader.
struct uniform_buffer_t
{
    glm::mat4 view;
    glm::mat4 proj;
};


// Each object just needs the model matrix
struct dynamic_uniform_buffer_t
{
    glm::mat4 model;
};

// This data will be written directly to the command buffer to
// be passed to the shader as a push constant.
struct push_constants_t
{
    int index; // index into the texture array layer
    int miplevel; // mipmap level we want to use, -1 for shader chosen.
};

/**
 * @brief get_elapsed_time
 * @return
 *
 * Gets the number of seconds since the application started.
 */
float get_elapsed_time()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    double time = 1.0 * std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0;

    return time;

}


/**
 * @brief create_box_mesh
 * @param dx - dimension of the box
 * @param dy - dimension of the box
 * @param dz - dimension of the box
 * @param vertices
 * @param indices
 *
 * Create a box mesh and save the vertices and indices in the input vectors.
 * The vertices/indices will then be copied into the graphics's buffers
 */
void create_box_mesh(float dx , float dy , float dz , std::vector<Vertex> & vertices, std::vector<uint16_t> & indices );



int main(int argc, char ** argv)
{
    //==========================================================================
    // 1. Initlize the library and create a GLFW window
    //==========================================================================
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,  GLFW_FALSE); GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, APP_TITLE, nullptr, nullptr);

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);


    // the context is the main class for the vka library. It is keeps track of
    // all the vulkan objects and releases them appropriately when it is destroyed
    // It is also responsible for creating the objects such as buffers, textures
    // command pools, etc.
    vka::context C;

    // Enable the required extensions for being able to draw
    for(uint i=0;i<glfwExtensionCount;i++)  C.enable_extension( glfwExtensions[i] );

    // Enable some extra extensions that we want.
    C.enable_extension( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );

    // Enable the required device extension
    C.enable_device_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    C.init();

    vk::SurfaceKHR surface;
    if (glfwCreateWindowSurface( C.get_instance(), window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface) ) != VK_SUCCESS)
    {
        ERROR << "Failed to create window surface!" << ENDL;
        throw std::runtime_error("failed to create window surface!");
    }

    C.create_device(surface); // find the appropriate device

    // The Screen is essentially a wrapper around the Swapchain, a default Renderpass
    // and framebuffers.
    // in VKA we present images to the screen object.
    // This a simple initialization of creating a screen with depth testing
    vka::Screen  Screen(&C);
    Screen.Create(surface,vk::Extent2D(WIDTH,HEIGHT) );

    //==========================================================================





    //==========================================================================
    // Initialize the Command and Descriptor Pools
    //==========================================================================
    vka::DescriptorPool descriptor_pool(&C);
    descriptor_pool.set_pool_size(vk::DescriptorType::eCombinedImageSampler, 2);
    descriptor_pool.set_pool_size(vk::DescriptorType::eUniformBuffer, 1);
    // [NEW]
    descriptor_pool.set_pool_size(vk::DescriptorType::eUniformBufferDynamic, 1);

    descriptor_pool.create();

    vka::CommandPool CP(&C);
    CP.create();
    //==========================================================================






    //==============================================================================
    // Initalize all the Buffer/Texture Pools
    //==============================================================================
    vka::BufferMemoryPool StagingBufferPool(&C);
    StagingBufferPool.SetMemoryProperties( vk::MemoryPropertyFlagBits::eHostCoherent| vk::MemoryPropertyFlagBits::eHostVisible);
    StagingBufferPool.SetSize(10*1024*1024);
    StagingBufferPool.SetUsage( vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc| vk::BufferUsageFlagBits::eIndexBuffer| vk::BufferUsageFlagBits::eVertexBuffer| vk::BufferUsageFlagBits::eUniformBuffer);
    StagingBufferPool.Create();

    vka::BufferMemoryPool BufferPool(&C);
    BufferPool.SetMemoryProperties( vk::MemoryPropertyFlagBits::eDeviceLocal);
    BufferPool.SetSize(10*1024*1024);
    BufferPool.SetUsage( vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc| vk::BufferUsageFlagBits::eIndexBuffer| vk::BufferUsageFlagBits::eVertexBuffer| vk::BufferUsageFlagBits::eUniformBuffer);
    BufferPool.Create();

    vka::TextureMemoryPool TexturePool(&C);
    TexturePool.SetSize( 50*1024*1024 );
    TexturePool.SetUsage( vk::ImageUsageFlagBits::eColorAttachment
                 | vk::ImageUsageFlagBits::eSampled
                 | vk::ImageUsageFlagBits::eTransferDst
                 | vk::ImageUsageFlagBits::eTransferSrc);

    //==============================================================================

//==============================================================================
// Create the Vertex and Index buffers
//
//  We are going to  create a vertex and index buffer. The vertex buffer will
//  hold the positions and UV coordates of our triangle.
//
//  The steps to create the buffer are.
//    1. Copy the vertex/index data from the host to a memory mappable device buffer
//    2. copy the memory-mapped buffer to the vertex/index buffers
//==============================================================================

        std::vector<Vertex>   vertices;
        std::vector<uint16_t> indices;

        create_box_mesh( 1,1,1, vertices, indices);

        // Create two buffers, one for vertices and one for indices. THey
        // will each be 1024 bytes long
        auto V_buffer  = BufferPool.NewSubBuffer(5*1024);
        auto I_buffer  = BufferPool.NewSubBuffer(5*1024);
        auto U_buffer  = BufferPool.NewSubBuffer(5*1024);
        auto DU_buffer = BufferPool.NewSubBuffer(5*1024);

        //auto StagingBuffer = StagingBufferPool.NewSubBuffer(5*1024*1024);

        // using the map< > method, we can return an array_view into the
        // memory. We are going to place them in their own scope so that
        // the array_view is destroyed after exiting the scope. This is
        // so we do not accidenty access the array_view after the
        // staging_buffer has been unmapped.

        // 1. Allocates 2 staging sub buffers to accept the transfer from the host
        // These Subbuffers will
        auto S_vertex = StagingBufferPool.NewSubBuffer( vertices.size()* sizeof(Vertex));
        auto S_index  = StagingBufferPool.NewSubBuffer( indices.size()* sizeof(uint16_t));

        // 2. Copy the data from the host to the staging buffers
        S_vertex->CopyData( vertices.data(), vertices.size() * sizeof(Vertex)   );
        S_index->CopyData( indices.data()  , indices .size() * sizeof(uint16_t) );

        // 3. Copy the data from the host-visible buffer to the vertex/index buffers
        {
            vka::command_buffer copy_cmd = CP.AllocateCommandBuffer();
            copy_cmd.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

                // write the commands to copy each of the buffer data
                const vk::DeviceSize vertex_size     = vertices.size()*sizeof(Vertex);
                const vk::DeviceSize index_size      = indices.size()*sizeof(uint16_t);

                copy_cmd.copySubBuffer( S_vertex, V_buffer, vk::BufferCopy{ 0 , 0 , vertex_size } );
                copy_cmd.copySubBuffer( S_index , I_buffer, vk::BufferCopy{ 0 , 0 , index_size  } );

            copy_cmd.end();
            C.submit_cmd_buffer(copy_cmd);
            CP.FreeCommandBuffer(copy_cmd);
        }

//==============================================================================
// Create a texture
//
//==============================================================================

    // 1. First load host_image into memory, and specifcy we want 4 channels.
        vka::host_image D("resources/textures/Brick-2852a.jpg",4);
        vka::host_image D2("resources/textures/noise.jpg",4);

    // 2. Use the context's helper function to create a device local texture
    //    We will be using a texture2d which is a case specific version of the
    //    generic texture
        auto Tex = TexturePool.AllocateTexture2D( vk::Format::eR8G8B8A8Unorm,
                                         vk::Extent2D(D.width(), D.height() ),
                                         2,5
                                         );



        // 3. Create a scope so that when we create Staging Buffers, they'll automatically
        //    be deallocated. StagingBuffers allocated from a pool can be allocated and deallocated
        //    without much performance issues.
        {
            auto StagingBuffer = StagingBufferPool.NewSubBuffer( D.size() + D2.size() );
            StagingBuffer->CopyData( D.data(), D.size() );
            StagingBuffer->CopyData(D2.data(), D2.size() , D.size());


        // 4. Now that the data is on the device. We need to get it from the buffer
        //    to the texture. To do this we will record a command buffer to do the
        //    following:
        //         a. convert the texture2d into a layout which can accept transfer data
        //         b. copy the data from the buffer to the texture2d.
        //         c. convert the texture2d into a layout which is good for shader use

            // allocate the command buffer
            vka::command_buffer cb1 = CP.AllocateCommandBuffer();
            cb1.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

            // a. convert the texture to eTransferDstOptimal
            cb1.convertTextureLayerMips( Tex,
                                         0,2, // convert Layer's 0-1
                                         0,1, // convert mip level i
                                         vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                                         vk::PipelineStageFlagBits::eAllCommands,
                                         vk::PipelineStageFlagBits::eAllCommands);

            // b. copy the data from the buffer to the texture
            vk::BufferImageCopy BIC;
            BIC.setBufferImageHeight(  D.height() )
               .setBufferOffset(0) // the image data starts at the start of the buffer
               .setImageExtent( vk::Extent3D(D.width(), D.height(), 1) ) // size of the image
               .setImageOffset( vk::Offset3D(0,0,0)) // where in the texture we want to paste the image
               .imageSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
                                .setBaseArrayLayer(0) // the layer to copy
                                .setLayerCount(2) // only copy one layer
                                .setMipLevel(0);  // only the first mip-map level


            //---------------------------------------------
            cb1.copySubBufferToTexture( StagingBuffer, Tex, vk::ImageLayout::eTransferDstOptimal, BIC);

#define MANUAL_MIP_MAPS

#if defined MANUAL_MIP_MAPS
            // convert the first layer into transferSrc
            cb1.convertTextureLayerMips( Tex,
                                         0,2, // convert Layer's 0-1
                                         0,1, // convert mip level i
                                         vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal,
                                         vk::PipelineStageFlagBits::eAllCommands,
                                         vk::PipelineStageFlagBits::eAllCommands);

            //==================================================================
            // Generate the mipmaps manually for layer 0
            //==================================================================
            for(uint32_t i=1; i < Tex->GetMipLevels() ; i++)
            {
                LOG << "Generating Mipmap Level: " << i+1 << ENDL;

                // Convert Layer i to TransferDst
                cb1.convertTextureLayerMips( Tex,
                                             0,2, // layers 0-1
                                             i,1, // mips level i+1
                                             vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                                             vk::PipelineStageFlagBits::eTransfer,
                                             vk::PipelineStageFlagBits::eHost);

                // Blit from miplevel i-1 to i for layers 0 and 1
                cb1.blitMipMap( Tex,
                                0,2,
                                i-1,i);


                // convert layer i into src so it can be copied from in the next iteraation
                cb1.convertTextureLayerMips( Tex,
                                             0,2, // layers 0-1
                                             i,1, // mips level i+1
                                             vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal,
                                             vk::PipelineStageFlagBits::eHost,
                                             vk::PipelineStageFlagBits::eTransfer);

            }

            //==================================================================

            // c. convert the texture into eShaderReadOnlyOptimal
            cb1.convertTextureLayerMips( Tex,
                                         0,2, // layers 0-1
                                         0,Tex->GetMipLevels(), // mips level i+1
                                         vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                                         vk::PipelineStageFlagBits::eHost,
                                         vk::PipelineStageFlagBits::eTransfer);

            // end and submit the command buffer
#else
            cb1.generateMipMaps( Tex, 0, 2);
#endif

            cb1.end();
            C.submit_cmd_buffer(cb1);
            // free the command buffer
            CP.FreeCommandBuffer(cb1);
        }
//==============================================================================


//==============================================================================
// Create a Rendering pipeline
//
//==============================================================================
        vka::Pipeline pipeline(&C);

        // Create the graphics Pipeline
          pipeline.setViewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                  ->setScissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                  ->setVertexShader(   "resources/shaders/mipmaps/mipmaps.vert", "main") // the shaders we want to use
                  ->setFragmentShader( "resources/shaders/mipmaps/mipmaps.frag", "main") // the shaders we want to use

                  // tell the pipeline that attribute 0 contains 3 floats
                  // and the data starts at offset 0
                  ->setVertexAttribute(0, 0 ,  offsetof(Vertex,p),  vk::Format::eR32G32B32Sfloat,  sizeof(Vertex) )
                  // tell the pipeline that attribute 1 contains 3 floats
                  // and the data starts at offset 12
                  ->setVertexAttribute(0, 1 , offsetof(Vertex,u),  vk::Format::eR32G32Sfloat,  sizeof(Vertex) )

                  ->setVertexAttribute(0, 2 , offsetof(Vertex,n),  vk::Format::eR32G32B32Sfloat,  sizeof(Vertex) )

                  // Triangle vertices are drawn in a counter clockwise manner
                  // using the right hand rule which indicates which face is the
                  // front
                  ->setFrontFace(vk::FrontFace::eCounterClockwise)

                  // Cull all back facing triangles.
                  ->setCullMode(vk::CullModeFlagBits::eBack)

                  // Tell the shader that we are going to use a texture
                  // in Set #0 binding #0
                  ->addTextureLayoutBinding(0, 0, vk::ShaderStageFlagBits::eFragment)

                  // Tell teh shader that we are going to use a uniform buffer
                  // in Set #0 binding #0
                  ->addUniformLayoutBinding(1, 0, vk::ShaderStageFlagBits::eVertex)

                  // Tell teh shader that we are going to use a uniform buffer
                  // in Set #0 binding #0
                  ->addDynamicUniformLayoutBinding(2, 0, vk::ShaderStageFlagBits::eVertex)

                  // Add a push constant to the layout. It is accessable in the vertex shader
                  // stage only.
                  ->addPushConstant( sizeof(push_constants_t), 0, vk::ShaderStageFlagBits::eVertex)
                  //
                  ->setRenderPass( Screen.GetRenderPass() )
                  ->create();




//==============================================================================
// Create a descriptor set:
//   once the pipeline has been created. We need to create a descriptor set
//   which we can use to tell what textures we want to use in the shader.
//   The pipline object can generate a descriptor set for you.
//==============================================================================
    // we want a descriptor set for set #0 in the pipeline.
    vka::DescriptorSet_p  texture_descriptor = pipeline.createNewDescriptorSet(0, &descriptor_pool);
    //  attach our texture to binding 0 in the set.
    texture_descriptor->AttachSampler(0, Tex);
    texture_descriptor->update();

    vka::DescriptorSet_p  ubuffer_descriptor = pipeline.createNewDescriptorSet(1, &descriptor_pool);
    ubuffer_descriptor->AttachUniformBuffer(0,U_buffer, 10);
    ubuffer_descriptor->update();

    vka::DescriptorSet_p  dubuffer_descriptor = pipeline.createNewDescriptorSet(2, &descriptor_pool);
    dubuffer_descriptor->AttachDynamicUniformBuffer(0,DU_buffer, DU_buffer->GetSize() );
    dubuffer_descriptor->update();



    // We will allocate two Staging buffers to copy uniform data as well as dynamic uniform data
    // for each of the objects. Each of the Staging Buffers act like an individual buffer
    // But are simply an offset into the BufferPool it was allocated from.
    //
    //
    // +--------------+
    // | uniform_data |  UniformStagingBuffer
    // +--------------+
    //
    // +------+------+
    // | obj1 | obj2 |    DynamicUniformStagingBuffer
    // +------+------+
    //
    // +--------------+---------+------+------+-------------------------+
    // | uniform_data |         | obj1 | obj2 |                         | StagingBufferPool
    // +--------------+---------+------+------+-------------------------+
    auto UniformStagingBuffer        = StagingBufferPool.NewSubBuffer(   sizeof(uniform_buffer_t ));
    auto DynamicUniformStagingBuffer = StagingBufferPool.NewSubBuffer( 2*sizeof(dynamic_uniform_buffer_t ));

    // Get a MappedMemory object so that we can write data directly into it.
    vka::MappedMemory  UniformStagingBufferMap = UniformStagingBuffer->GetMappedMemory();
    vka::MappedMemory  DynamicStagingBufferMap = DynamicUniformStagingBuffer->GetMappedMemory();

    // Cast the memory to a reference so we can access
    // aliased data.
    uniform_buffer_t & UniformStagingStruct               = *( (uniform_buffer_t*)UniformStagingBufferMap );
    dynamic_uniform_buffer_t * DynamicUniformStagingArray = (dynamic_uniform_buffer_t*)DynamicStagingBufferMap;

    vka::command_buffer cb = CP.AllocateCommandBuffer();


    vka::Semaphore_p  image_available_semaphore = C.createSemaphore();
    vka::Semaphore_p  render_complete_semaphore = C.createSemaphore();



    // Dynamic Uniform buffers have a set alignment, meaning the number of bytes
    // bound to the pipeline must be a multiple of the alignment
    // In most case the alignment is 256 bytes.
    auto alignment = C.get_physical_device_limits().minUniformBufferOffsetAlignment;

    //==========================================================================
    // Perform the Rendering
    //==========================================================================
    // Here we finally perform the main loop for rendering.
    // The steps required are:
    //    a. update the uniform buffer with the MVP matrices
    //
    while (!glfwWindowShouldClose(window) )
    {
       float t = get_elapsed_time();
      // Get the next available image in the swapchain

      glfwPollEvents();

      // reset the command buffer so that we can record from scratch again.
      cb.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
      cb.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse) );


      //--------------------------------------------------------------------------------------
      // Copy the Data from thost to the staging buffers.
      //--------------------------------------------------------------------------------------
      #define MAX_OBJECTS 2
      // Copy the uniform buffer data into the staging buffer
      const float AR = WIDTH / ( float )HEIGHT;
      UniformStagingStruct.view        = glm::lookAt(  ( 5*cosf(0.1*t)*cosf(0.1*t) + 1 )*glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
      UniformStagingStruct.proj        = glm::perspective(glm::radians(45.0f), AR, 0.1f, 30.0f);
      UniformStagingStruct.proj[1][1] *= -1;

      // Copy the dynamic uniform buffer data into the staging buffer
      DynamicUniformStagingArray[0].model   =  glm::rotate(glm::mat4(1.0), t * glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::translate( glm::mat4(), glm::vec3(-1,0,0) ) ;
      DynamicUniformStagingArray[1].model   =  glm::rotate(glm::mat4(1.0), t * glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::translate( glm::mat4(), glm::vec3(1,0,0));
      //--------------------------------------------------------------------------------------

      //--------------------------------------------------------------------------------------
      // Copy the uniform buffer data from the staging buffer to the uniform buffer. This normally only needs to be done
      // once per rendering frame because it contains frame constant data.
      cb.copySubBuffer( UniformStagingBuffer ,  U_buffer , vk::BufferCopy{ 0,0, sizeof(uniform_buffer_t) } );
      //--------------------------------------------------------------------------------------

      // Copy the dynamic uniform buffer data from the staging buffer
      // to the appropriate offset in the Dynamic Uniform Buffer.
      // +-------------+---------------------------------------+
      // | obj1        | obj2         | obj3...                | Dynamic Uniform Buffer
      // +-------------+---------------------------------------+
      // |<-alignment->|
      for(uint32_t j=0; j < MAX_OBJECTS; j++)
      {
          // byte offset within the staging buffer where teh data resides
          auto srcOffset = j * sizeof(dynamic_uniform_buffer_t);
          // byte offset within the dynamic uniform buffer where to copy the data
          auto dstOffset = j * alignment;
          // number of bytes to copy
          auto size      = sizeof(dynamic_uniform_buffer_t);

          cb.copySubBuffer( DynamicUniformStagingBuffer , DU_buffer , vk::BufferCopy{ srcOffset, dstOffset, size } );
      }

      uint32_t frame_index = Screen.GetNextFrameIndex(image_available_semaphore);
      cb.beginRender(Screen, frame_index);


      // bind the pipeline that we want to use next
        cb.bindPipeline( vk::PipelineBindPoint::eGraphics, pipeline );

      // bind the two descriptor sets that we need to that pipeline
       cb.bindDescriptorSets( vk::PipelineBindPoint::eGraphics,
                                                    pipeline.getLayout(),
                                                    0,
                                                    vk::ArrayProxy<const vk::DescriptorSet>( texture_descriptor->get()),
                                                    nullptr );

        cb.bindDescriptorSets( vk::PipelineBindPoint::eGraphics,
                                                pipeline.getLayout(),
                                                1,
                                                vk::ArrayProxy<const vk::DescriptorSet>( ubuffer_descriptor->get()),
                                                nullptr );

    // bind the vertex/index buffers
        cb.bindVertexSubBuffer(0, V_buffer, 0 );
        cb.bindIndexSubBuffer(  I_buffer, vk::IndexType::eUint16, 0);

      //========================================================================
      // Draw all the objects while binding the dynamic uniform buffer
      // to
      //========================================================================
      for(uint32_t j=0 ; j < MAX_OBJECTS; j++)
      {
            // Here we write the data to the command buffer.
            push_constants_t push;
            push.index    = 0;

            if(j==0)
            {
              push.miplevel = -1;
            } else {
              push.miplevel = (int)fmod( t ,Tex->GetMipLevels() );
            }

            cb.pushConstants( pipeline.getLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(push_constants_t), &push);

            cb.bindDescriptorSets( vk::PipelineBindPoint::eGraphics,
                                                    pipeline.getLayout(),
                                                    2,
                                                    vk::ArrayProxy<const vk::DescriptorSet>( dubuffer_descriptor->get()),
                                                    vk::ArrayProxy<const uint32_t>(j*alignment) );



            // draw 3 indices, 1 time, starting from index 0, using a vertex offset of 0
            cb.drawIndexed(36, 1, 0 , 0, 0);
      }



      cb.endRenderPass();
      cb.end();

      // Submit the command buffers, but wait until the image_available_semaphore
      // is flagged. Once the commands have been executed, flag the render_complete_semaphore
      C.submit_command_buffer(cb, image_available_semaphore, render_complete_semaphore);

      // present the image to the surface, but wait for the render_complete_semaphore
      // to be flagged by the submit_command_buffer
      Screen.PresentFrame(frame_index, render_complete_semaphore);

      std::this_thread::sleep_for( std::chrono::milliseconds(3) );
    }

    return 0;
}

#define STB_IMAGE_IMPLEMENTATION
#include<stb/stb_image.h>



void create_box_mesh(float dx , float dy , float dz , std::vector<Vertex> & vertices, std::vector<uint16_t> & indices )
{

    indices .clear();
    vertices.clear();

    using namespace glm;


    std::vector<uint16_t> I;
//       |       Position                           |   UV         |     Normal    |
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {0.0,0.0}  ,  { 0.0,  0.0,  1.0} } );   // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {1.0,0.0}  ,  { 0.0,  0.0,  1.0} } );   // 1
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {1.0,1.0}  ,  { 0.0,  0.0,  1.0} } );   // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {0.0,0.0}  ,  { 0.0,  0.0,  1.0} } );   // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {1.0,1.0}  ,  { 0.0,  0.0,  1.0} } );   // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {0.0,1.0}  ,  { 0.0,  0.0,  1.0} } );   // 3
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {0.0,1.0}  ,  { 0.0,  0.0, -1.0} } ); // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {1.0,1.0}  ,  { 0.0,  0.0, -1.0} } ); // 1
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {1.0,0.0}  ,  { 0.0,  0.0, -1.0} } ); // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {0.0,1.0}  ,  { 0.0,  0.0, -1.0} } ); // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {1.0,0.0}  ,  { 0.0,  0.0, -1.0} } ); // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {0.0,0.0}  ,  { 0.0,  0.0, -1.0} } ); // 3
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {0.0,0.0}  ,  {-1.0f, 0.0,  0.0} } );  // 0
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {1.0,0.0}  ,  {-1.0f, 0.0,  0.0} } );  // 1
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {1.0,1.0}  ,  {-1.0f, 0.0,  0.0} } );  // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {0.0,0.0}  ,  {-1.0f, 0.0,  0.0} } );  // 0
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {1.0,1.0}  ,  {-1.0f, 0.0,  0.0} } );  // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {0.0,1.0}  ,  {-1.0f, 0.0,  0.0} } );  // 3
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {0.0,1.0}  ,  { 1.0f, 0.0,  0.0} } ); // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {1.0,1.0}  ,  { 1.0f, 0.0,  0.0} } ); // 1
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {1.0,0.0}  ,  { 1.0f, 0.0,  0.0} } ); // 2
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {0.0,1.0}  ,  { 1.0f, 0.0,  0.0} } ); // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {1.0,0.0}  ,  { 1.0f, 0.0,  0.0} } ); // 2
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {0.0,0.0}  ,  { 1.0f, 0.0,  0.0} } ); // 3
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {0.0,0.0}  ,  { 0.0f,-1.0,  0.0} } ); // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {1.0,0.0}  ,  { 0.0f,-1.0,  0.0} } ); // 1
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {1.0,1.0}  ,  { 0.0f,-1.0,  0.0} } ); // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,0.0-0.5*dz} , {0.0,0.0}  ,  { 0.0f,-1.0,  0.0} } ); // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {1.0,1.0}  ,  { 0.0f,-1.0,  0.0} } ); // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,0.0 - 0.5*dy  ,dz -0.5*dz} , {0.0,1.0}  ,  { 0.0f,-1.0,  0.0} } ); // 3
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {0.0,1.0}  ,  { 0.0f, 1.0,  0.0} } ); // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {1.0,1.0}  ,  { 0.0f, 1.0,  0.0} } ); // 1
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {1.0,0.0}  ,  { 0.0f, 1.0,  0.0} } ); // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,dz -0.5*dz} , {0.0,1.0}  ,  { 0.0f, 1.0,  0.0} } ); // 0
         vertices.push_back( Vertex{ {dx  - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {1.0,0.0}  ,  { 0.0f, 1.0,  0.0} } ); // 2
         vertices.push_back( Vertex{ {0.0 - 0.5*dx  ,dy  - 0.5*dy  ,0.0-0.5*dz} , {0.0,0.0}  ,  { 0.0f, 1.0,  0.0} } ); // 3

    //=========================
    // Edges of the triangle : postion delta


    //=========================
    for(uint16_t i=0;i<36;i++)
        indices.push_back(i);


}
