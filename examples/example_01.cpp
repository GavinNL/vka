/*
 * Example 1:
 *
 * This example outlines a very basic usage of vka which demonstrates
 * the essentials of graphics rendering.   It is broken down into a number of
 * steps
 *
 * 1. Initialize the Library and create a window
 * 2. Create vertex/index buffers to store geometry
 * 3. Load a texture into the GPU
 * 4. Create a rendering pipeline
 * 5. Start rendering
 *
 *
 *
 *
 *
 *
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
#define APP_TITLE "Example_01 - Hello Textured Rotating Triangle!"




#include <vka/core2/Screen.h>



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

int main(int argc, char ** argv)
{
    //==========================================================================
    // 1. Initlize the library and create a GLFW window
    //==========================================================================
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,  GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, APP_TITLE, nullptr, nullptr);

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);


    // the context is the main class for the vka library. It is keeps track of
    // all the vulkan objects and releases them appropriately when it is destroyed
    // It is also responsible for creating the objects such as buffers, textures
    // command pools, etc.
    vka::context C;

    // Enable the required extensions for being able to draw
    for(uint i=0;i<glfwExtensionCount;i++)  C.enableExtension( glfwExtensions[i] );

    // Enable some extra extensions that we want.
    C.enableExtension( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );

    // Enable the required device extension
    C.enableDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    C.init();

    vk::SurfaceKHR surface;
    if (glfwCreateWindowSurface( C.getInstance(), window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface) ) != VK_SUCCESS)
    {
        ERROR << "Failed to create window surface!" << ENDL;
        throw std::runtime_error("failed to create window surface!");
    }

    C.createDevice(surface); // find the appropriate device


    vka::Screen Screen(&C);
    Screen.create(surface, vk::Extent2D(WIDTH,HEIGHT));


    //==========================================================================



    //==========================================================================
    // Initialize the Command and Descriptor Pools
    //==========================================================================
    vka::DescriptorPool descriptor_pool(&C);
    descriptor_pool.set_pool_size(vk::DescriptorType::eCombinedImageSampler, 2);
    descriptor_pool.set_pool_size(vk::DescriptorType::eUniformBuffer, 1);
    descriptor_pool.create();

    //vka::CommandPool CP(&C);\n CP.create();
    vka::CommandPool CP(&C);
    CP.create();
    //==========================================================================




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
        // Create two buffers, one for vertices and one for indices. THey
        // will each be 1024 bytes long

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

        auto V_buffer = BufferPool.NewSubBuffer(1024);
        auto I_buffer = BufferPool.NewSubBuffer(1024);
        auto U_buffer = BufferPool.NewSubBuffer(1024);



        // This is the vertex structure we are going to use
        // it contains a position and a UV coordates field
        struct Vertex
        {
            glm::vec3 p;
            glm::vec2 u;
        };

        // allocate a staging buffer of 10MB

        auto StagingBuffer  = StagingBufferPool.NewSubBuffer( sizeof(1024*1024*10) );


        // using the map< > method, we can return an array_view into the
        // memory. We are going to place them in their own scope so that
        // the array_view is destroyed after exiting the scope. This is
        // so we do not accidenty access the array_view after the
        // staging_buffer has been unmapped.
        {
            std::array<Vertex,3>    vertex;
            LOG << "Vertex size: " << vertex.size() << ENDL;
            // we can access each vertex as if it was an array. Copy the
            // vertex data we want into the first three indices.
            vertex[0] = {glm::vec3( 0.0,  0.0,  1.0 ) , glm::vec2(0.5 , 0) } ;
            vertex[1] = {glm::vec3( 1.0,  0.0, -1.0 ) , glm::vec2(0   , 1) };
            vertex[2] = {glm::vec3(-1.0,  0.0, -1.0 ) , glm::vec2(1   , 1) };


            vka::MappedMemory m = StagingBuffer->GetMappedMemory();
            m.memcpy( &vertex[0], sizeof(vertex));


        }
        // Do the same for the index buffer. but we want to specific an
        // offset form the start of the buffer so we do not overwrite the
        // vertex data.
        {
            // +--------------------------------------------------------+
            // |  vertex data    |   index data                         |
            // +--------------------------------------------------------+
            std::array<glm::uint16_t,3> index;
            index[0] = 0;
            index[1] = 1;
            index[2] = 2;


            vka::MappedMemory m  = StagingBuffer->GetMappedMemory() + 3*sizeof(Vertex);
            m.memcpy(&index[0], sizeof(index));


            LOG << "Index size: " << index.size() << ENDL;
        }


        // 2. Copy the data from the host-visible buffer to the vertex/index buffers

        // allocate a comand buffer
        vka::CommandBuffer copy_cmd = CP.allocateCommandBuffer();
        copy_cmd.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

        // write the commands to copy each of the buffer data
        const vk::DeviceSize vertex_offset = 0;
        const vk::DeviceSize vertex_size   = 3 * sizeof(Vertex);

        const vk::DeviceSize index_size    = 3 * sizeof(uint16_t);
        const vk::DeviceSize index_offset  = vertex_size;


        copy_cmd.copySubBuffer( StagingBuffer, V_buffer, vk::BufferCopy{vertex_offset, 0 ,vertex_size});
        copy_cmd.copySubBuffer( StagingBuffer, I_buffer, vk::BufferCopy{index_offset,  0 , index_size});


        copy_cmd.end();
        C.submitCommandBuffer(copy_cmd);
        ////===============
        //
        CP.freeCommandBuffer(copy_cmd);

        // Unmap the memory.
        //   WARNING: DO NOT ACCESS the vertex and index array_views as these
        //            now point to unknown memory spaces

//==============================================================================
// Create a texture
//
//==============================================================================


        vka::TextureMemoryPool TP(&C);
        TP.SetSize( 50*1024*1024 );
        TP.SetUsage( vk::ImageUsageFlagBits::eColorAttachment
                     | vk::ImageUsageFlagBits::eSampled
                     | vk::ImageUsageFlagBits::eTransferDst
                     | vk::ImageUsageFlagBits::eTransferSrc);




    // 1. First load host_image into memory, and specifcy we want 4 channels.
        vka::HostImage D("resources/textures/Brick-2852a.jpg",4);





        // 2. Use the context's helper function to create a device local texture
        //    We will be using a texture2d which is a case specific version of the
        //    generic texture

        auto Tex = TP.allocateTexture2D( vk::Format::eR8G8B8A8Unorm,
                                         vk::Extent2D(D.width(), D.height() ),
                                         1,1
                                         );



    // 3. Map the buffer to memory and copy the image to it.

        void * image_buffer_data = StagingBuffer->GetMappedMemory();


        memcpy( image_buffer_data, D.data(), D.size() );




    // 4. Now that the data is on the device. We need to get it from the buffer
    //    to the texture. To do this we will record a command buffer to do the
    //    following:
    //         a. convert the texture2d into a layout which can accept transfer data
    //         b. copy the data from the buffer to the texture2d.
    //         c. convert the texture2d into a layout which is good for shader use

        // allocate the command buffer
        vka::CommandBuffer cb1 = CP.allocateCommandBuffer();
        cb1.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

            // a. convert the texture to eTransferDstOptimal

        cb1.convertTextureLayer( Tex,0,1,vk::ImageLayout::eTransferDstOptimal,
                                 vk::PipelineStageFlagBits::eBottomOfPipe,
                                 vk::PipelineStageFlagBits::eTopOfPipe);



            // b. copy the data from the buffer to the texture
            vk::BufferImageCopy BIC;
            BIC.setBufferImageHeight(  D.height() )
               .setBufferOffset(0) // the image data starts at the start of the buffer
               .setImageExtent( vk::Extent3D(D.width(), D.height(), 1) ) // size of the image
               .setImageOffset( vk::Offset3D(0,0,0)) // where in the texture we want to paste the image
               .imageSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
                                .setBaseArrayLayer(0) // the layer to copy
                                .setLayerCount(1) // only copy one layer
                                .setMipLevel(0);  // only the first mip-map level


            cb1.copySubBufferToTexture( StagingBuffer, Tex, vk::ImageLayout::eTransferDstOptimal, BIC);



            // c. convert the texture into eShaderReadOnlyOptimal

            cb1.convertTextureLayer( Tex,0,1,vk::ImageLayout::eShaderReadOnlyOptimal,
                                     vk::PipelineStageFlagBits::eBottomOfPipe,
                                     vk::PipelineStageFlagBits::eTopOfPipe);


        // end and submit the command buffer
        cb1.end();
        C.submitCommandBuffer(cb1);
        // free the command buffer
        CP.freeCommandBuffer(cb1);
//==============================================================================


//==============================================================================
// Create a Rendering pipeline
//
//==============================================================================

        vka::Pipeline pipeline(&C);
        // Create the graphics Pipeline
          pipeline.setViewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                  ->setScissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )
                  ->setVertexShader( "resources/shaders/uniform_buffer/uniform_buffer.vert", "main" )   // the shaders we want to use
                  ->setFragmentShader( "resources/shaders/uniform_buffer/uniform_buffer.frag", "main" ) // the shaders we want to use

                  // tell the pipeline that attribute 0 contains 3 floats
                  // and the data starts at offset 0
                  ->setVertexAttribute(0,0 ,  0,  vk::Format::eR32G32B32Sfloat,  sizeof(Vertex) )
                  // tell the pipeline that attribute 1 contains 3 floats
                  // and the data starts at offset 12
                  ->setVertexAttribute(0,1 , 12,  vk::Format::eR32G32Sfloat,  sizeof(Vertex) )

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
                  ->setRenderPass( Screen.getRenderPass()  )
                  ->create();



//==============================================================================
// Create a descriptor set:
//   once the pipeline has been created. We need to create a descriptor set
//   which we can use to tell what textures we want to use in the shader.
//   The pipline object can generate a descriptor set for you.
//==============================================================================
    // we want a descriptor set for set #0 in the pipeline.
    //  attach our texture to binding 0 in the set.

    vka::DescriptorSet_p  texture_descriptor = pipeline.createNewDescriptorSet(0, &descriptor_pool);
    texture_descriptor->AttachSampler(0, Tex);

    texture_descriptor->update();


    vka::DescriptorSet_p  ubuffer_descriptor = pipeline.createNewDescriptorSet(1, &descriptor_pool);
    ubuffer_descriptor->AttachUniformBuffer(0,U_buffer, 10);

    ubuffer_descriptor->update();

    // This is the structure of the uniform buffer we want.
    // it needs to match the structure in the shader.
    struct uniform_buffer_t
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };



    vka::MappedMemory StagingBufferMap   = StagingBuffer->GetMappedMemory();

    vka::CommandBuffer cb = CP.allocateCommandBuffer();


    vka::Semaphore_p  image_available_semaphore = C.createSemaphore();
    vka::Semaphore_p  render_complete_semaphore = C.createSemaphore();

    //==========================================================================
    // Perform the Rendering
    //==========================================================================
    // Here we finally perform the main loop for rendering.
    // The steps required are:
    //    a. update the uniform buffer with the MVP matrices
    //
    while (!glfwWindowShouldClose(window) )
    {

      glfwPollEvents();

      //============ Update the uniform buffer=====================
      float t = get_elapsed_time();

      const float AR = WIDTH / ( float )HEIGHT;

      // Create a Reference to the staging buffer map so that we can
      // write data into the buffer
      uniform_buffer_t & UniformBufferRef = *( (uniform_buffer_t*)(StagingBufferMap) );
      UniformBufferRef.model = glm::rotate( glm::mat4(), t * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
      UniformBufferRef.view  = glm::lookAt( glm::vec3(5.0f, 5.0f, 5.0f),
                                              glm::vec3(0.0f, 0.0f, 0.0f),
                                              glm::vec3(0.0f, 1.0f, 0.0f));
      UniformBufferRef.proj  = glm::perspective(glm::radians(45.0f), AR, 0.1f, 10.0f);
      UniformBufferRef.proj[1][1] *= -1;

      //============================================================

      // Get the next available image in the swapchain


      // reset the command buffer so that we can record from scratch again.
      cb.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
      cb.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse) );


      // Copy the uniform buffer data from the stating buffer to the uniform buffer

      cb.copySubBuffer( StagingBuffer,     U_buffer , vk::BufferCopy{ 0,0,sizeof(uniform_buffer_t) } );



      uint32_t frame_index = Screen.getNextFrameIndex(image_available_semaphore);



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
      cb.bindVertexSubBuffer(0, V_buffer );// ( m_VertexBuffer, 0);
      cb.bindIndexSubBuffer( I_buffer, vk::IndexType::eUint16);



    // draw 3 indices, 1 time, starting from index 0, using a vertex offset of 0
            cb.drawIndexed(3, 1, 0 , 0, 0);

            cb.endRenderPass();

      cb.end();

      // Submit the command buffers, but wait until the image_available_semaphore
      // is flagged. Once the commands have been executed, flag the render_complete_semaphore
      C.submitCommandBuffer(cb, image_available_semaphore, render_complete_semaphore);

      // present the image to the surface, but wait for the render_complete_semaphore
      // to be flagged by the submit_command_buffer
      Screen.presentFrame(frame_index, render_complete_semaphore);


      std::this_thread::sleep_for( std::chrono::milliseconds(3) );
    }

    return 0;
}

#define STB_IMAGE_IMPLEMENTATION
#include<stb/stb_image.h>
