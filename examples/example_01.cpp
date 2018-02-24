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


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define WIDTH 1024
#define HEIGHT 768
#define APP_TITLE "Example_01 - Hello Textured Rotating Triangle!"

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
    // 1. Initlize the library and create a window
    //==========================================================================
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE,  GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, APP_TITLE, nullptr, nullptr);


    // the context is the main class for the vka library. It is keeps track of
    // all the vulkan objects and releases them appropriately when it is destroyed
    // It is also responsible for creating the objects such as buffers, textures
    // command pools, etc.
    vka::context C;

    C.init();
    auto surface = C.create_window_surface(window); // create the vulkan surface using the window provided
    C.create_device(); // find the appropriate device


    // The Screen is essentially a wrapper around the Swapchain, a default Renderpass
    // and framebuffers.
    // in VKA we present images to the screen object.
    // This a simple initialization of creating a screen with depth testing
    auto * screen = C.new_screen("screen");
    screen = C.new_screen("m_win");
    screen->set_extent( vk::Extent2D(WIDTH,HEIGHT) );
    screen->set_surface( surface );
    screen->create();

    //==========================================================================



    //==========================================================================
    // Initialize the Command and Descriptor Pools
    //==========================================================================
    vka::descriptor_pool* descriptor_pool = C.new_descriptor_pool("main_desc_pool");
    descriptor_pool->set_pool_size(vk::DescriptorType::eCombinedImageSampler, 2);
    descriptor_pool->set_pool_size(vk::DescriptorType::eUniformBuffer, 1);
    descriptor_pool->create();

    vka::command_pool* cp = C.new_command_pool("main_command_pool");
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
        vka::buffer* vertex_buffer = C.new_vertex_buffer("vb", 1024 );
        vka::buffer* index_buffer  = C.new_index_buffer( "ib", 1024 );
        vka::buffer* u_buffer      = C.new_uniform_buffer( "ub", 1024);

        // This is the vertex structure we are going to use
        // it contains a position and a UV coordates field
        struct Vertex
        {
            glm::vec3 p;
            glm::vec2 u;
        };

        // allocate a staging buffer of 10MB
        vka::buffer * staging_buffer = C.new_staging_buffer( "sb", 1024*1024*10 );

        // using the map< > method, we can return an array_view into the
        // memory. We are going to place them in their own scope so that
        // the array_view is destroyed after exiting the scope. This is
        // so we do not accidenty access the array_view after the
        // staging_buffer has been unmapped.
        {
            vka::array_view<Vertex> vertex =  staging_buffer->map<Vertex>();

            LOG << "Vertex size: " << vertex.size() << ENDL;
            // we can access each vertex as if it was an array. Copy the
            // vertex data we want into the first three indices.
            vertex[0] = {glm::vec3( 0.0,  0.0,  1.0 ) , glm::vec2(0.5 , 0) } ;
            vertex[1] = {glm::vec3( 1.0,  0.0, -1.0 ) , glm::vec2(0   , 1) };
            vertex[2] = {glm::vec3(-1.0,  0.0, -1.0 ) , glm::vec2(1   , 1) };
        }
        // Do the same for the index buffer. but we want to specific an
        // offset form the start of the buffer so we do not overwrite the
        // vertex data.
        {
            // +--------------------------------------------------------+
            // |  vertex data    |   index data                         |
            // +--------------------------------------------------------+
            vka::array_view<glm::uint16_t> index =  staging_buffer->map<glm::uint16>( 3*sizeof(Vertex) );
            index[0] = 0;
            index[1] = 1;
            index[2] = 2;
            LOG << "Index size: " << index.size() << ENDL;
        }


        // 2. Copy the data from the host-visible buffer to the vertex/index buffers

        // allocate a comand buffer
        vk::CommandBuffer copy_cmd = cp->AllocateCommandBuffer();
        copy_cmd.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

        // write the commands to copy each of the buffer data
        const vk::DeviceSize vertex_offset = 0;
        const vk::DeviceSize vertex_size   = 3 * sizeof(Vertex);

        const vk::DeviceSize index_size    = 3 * sizeof(uint16_t);
        const vk::DeviceSize index_offset  = vertex_size;


        copy_cmd.copyBuffer( *staging_buffer , *vertex_buffer, vk::BufferCopy{ vertex_offset    , 0 , vertex_size } );
        copy_cmd.copyBuffer( *staging_buffer , *index_buffer , vk::BufferCopy{ index_offset     , 0 , index_size  } );

        copy_cmd.end();
        C.submit_cmd_buffer(copy_cmd);
        ////===============
        //
        cp->FreeCommandBuffer(copy_cmd);

        // Unmap the memory.
        //   WARNING: DO NOT ACCESS the vertex and index array_views as these
        //            now point to unknown memory spaces
        staging_buffer->unmap_memory();


//==============================================================================
// Create a texture
//
//==============================================================================

    // 1. First load host_image into memory, and specifcy we want 4 channels.
        vka::host_image D("resources/textures/Brick-2852a.jpg",4);


    // 2. Use the context's helper function to create a device local texture
    //    We will be using a texture2d which is a case specific version of the
    //    generic texture
        vka::texture2d * tex = C.new_texture2d("test_texture");
        tex->set_size( D.width() , D.height() );
        tex->set_format(vk::Format::eR8G8B8A8Unorm);
        tex->set_mipmap_levels(1);
        tex->create();
        tex->create_image_view(vk::ImageAspectFlagBits::eColor);


    // 3. Map the buffer to memory and copy the image to it.
        void * image_buffer_data = staging_buffer->map_memory();
        memcpy( image_buffer_data, D.data(), D.size() );
        staging_buffer->unmap_memory();


    // 4. Now that the data is on the device. We need to get it from the buffer
    //    to the texture. To do this we will record a command buffer to do the
    //    following:
    //         a. convert the texture2d into a layout which can accept transfer data
    //         b. copy the data from the buffer to the texture2d.
    //         c. convert the texture2d into a layout which is good for shader use

        // allocate the command buffer
        vk::CommandBuffer cb1 = cp->AllocateCommandBuffer();
        cb1.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) );

            // a. convert the texture to eTransferDstOptimal
            tex->convert_layer(cb1, vk::ImageLayout::eTransferDstOptimal,0,0);

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

            tex->copy_buffer( cb1, staging_buffer, BIC);

            // c. convert the texture into eShaderReadOnlyOptimal
            tex->convert(cb1, vk::ImageLayout::eShaderReadOnlyOptimal);

        // end and submit the command buffer
        cb1.end();
        C.submit_cmd_buffer(cb1);
        // free the command buffer
        cp->FreeCommandBuffer(cb1);
//==============================================================================


//==============================================================================
// Create a Rendering pipeline
//
//==============================================================================
        // create the vertex shader from a pre compiled SPIR-V file
        vka::shader* vertex_shader = C.new_shader_module("vs");
        vertex_shader->load_from_file("resources/shaders/uniform_buffer/uniform_buffer_v.spv");

        // create the fragment shader from a pre compiled SPIR-V file
        vka::shader* fragment_shader = C.new_shader_module("fs");
        fragment_shader->load_from_file("resources/shaders/uniform_buffer/uniform_buffer_f.spv");

        vka::pipeline* pipeline = C.new_pipeline("triangle");

        // Create the graphics Pipeline
          pipeline->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                  ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                  ->set_vertex_shader(   vertex_shader )   // the shaders we want to use
                  ->set_fragment_shader( fragment_shader ) // the shaders we want to use

                  // tell the pipeline that attribute 0 contains 3 floats
                  // and the data starts at offset 0
                  ->set_vertex_attribute(0,0 ,  0,  vk::Format::eR32G32B32Sfloat,  sizeof(Vertex) )
                  // tell the pipeline that attribute 1 contains 3 floats
                  // and the data starts at offset 12
                  ->set_vertex_attribute(0,1 , 12,  vk::Format::eR32G32Sfloat,  sizeof(Vertex) )

                  // Triangle vertices are drawn in a counter clockwise manner
                  // using the right hand rule which indicates which face is the
                  // front
                  ->set_front_face(vk::FrontFace::eCounterClockwise)

                  // Cull all back facing triangles.
                  ->set_cull_mode(vk::CullModeFlagBits::eBack)

                  // Tell the shader that we are going to use a texture
                  // in Set #0 binding #0
                  ->add_texture_layout_binding(0, 0, vk::ShaderStageFlagBits::eFragment)

                  // Tell teh shader that we are going to use a uniform buffer
                  // in Set #0 binding #0
                  ->add_uniform_layout_binding(1, 0, vk::ShaderStageFlagBits::eVertex)
                  //
                  ->set_render_pass( screen->get_renderpass() )
                  ->create();



//==============================================================================
// Create a descriptor set:
//   once the pipeline has been created. We need to create a descriptor set
//   which we can use to tell what textures we want to use in the shader.
//   The pipline object can generate a descriptor set for you.
//==============================================================================
    // we want a descriptor set for set #0 in the pipeline.
    vka::descriptor_set * texture_descriptor = pipeline->create_new_descriptor_set(0, descriptor_pool);
    //  attach our texture to binding 0 in the set.
    texture_descriptor->attach_sampler(0, tex);
    texture_descriptor->update();

    vka::descriptor_set * ubuffer_descriptor = pipeline->create_new_descriptor_set(1, descriptor_pool);
    ubuffer_descriptor->attach_uniform_buffer(0, u_buffer, 10, 0);
    ubuffer_descriptor->update();

    // This is the structure of the uniform buffer we want.
    // it needs to match the structure in the shader.
    struct uniform_buffer_t
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    vka::array_view<uniform_buffer_t> staging_buffer_map = staging_buffer->map<uniform_buffer_t>();

    vka::command_buffer cb = cp->AllocateCommandBuffer();


    vka::semaphore * image_available_semaphore = C.new_semaphore("image_available_semaphore");
    vka::semaphore * render_complete_semaphore = C.new_semaphore("render_complete_semaphore");

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

      staging_buffer_map[0].model = glm::rotate(glm::mat4(), t * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
      staging_buffer_map[0].view  = glm::lookAt( glm::vec3(5.0f, 5.0f, 5.0f),
                                                 glm::vec3(0.0f, 0.0f, 0.0f),
                                                 glm::vec3(0.0f, 1.0f, 0.0f));
      staging_buffer_map[0].proj  = glm::perspective(glm::radians(45.0f), AR, 0.1f, 10.0f);
      staging_buffer_map[0].proj[1][1] *= -1;

      //============================================================

      // Get the next available image in the swapchain


      // reset the command buffer so that we can record from scratch again.
      cb.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
      cb.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse) );


      // Copy the uniform buffer data from the stating buffer to the uniform buffer
      cb.copyBuffer( *staging_buffer , *u_buffer , vk::BufferCopy{ 0,0,sizeof(uniform_buffer_t) } );


      screen->prepare_next_frame(image_available_semaphore);
      screen->beginRender(cb);

      // bind the pipeline that we want to use next
            cb.bindPipeline( vk::PipelineBindPoint::eGraphics, *pipeline );

      // bind the two descriptor sets that we need to that pipeline
            cb.bindDescriptorSets( vk::PipelineBindPoint::eGraphics,
                                                    pipeline->get_layout(),
                                                    0,
                                                    vk::ArrayProxy<const vk::DescriptorSet>( texture_descriptor->get()),
                                                    nullptr );
            cb.bindDescriptorSets( vk::PipelineBindPoint::eGraphics,
                                                    pipeline->get_layout(),
                                                    1,
                                                    vk::ArrayProxy<const vk::DescriptorSet>( ubuffer_descriptor->get()),
                                                    nullptr );

     // bind the vertex/index buffers
            cb.bindVertexBuffers(0, vertex_buffer->get(), {0} );// ( m_VertexBuffer, 0);
            cb.bindIndexBuffer(  index_buffer->get() , 0 , vk::IndexType::eUint16);

    // draw 3 indices, 1 time, starting from index 0, using a vertex offset of 0
            cb.drawIndexed(3, 1, 0 , 0, 0);

      screen->endRender(cb);
      cb.end();

      // Submit the command buffers, but wait until the image_available_semaphore
      // is flagged. Once the commands have been executed, flag the render_complete_semaphore
      C.submit_command_buffer(cb, image_available_semaphore, render_complete_semaphore);

      // present the image to the surface, but wait for the render_complete_semaphore
      // to be flagged by the submit_command_buffer
      screen->present_frame(render_complete_semaphore);


      std::this_thread::sleep_for( std::chrono::milliseconds(3) );
    }

    return 0;
}

#define STB_IMAGE_IMPLEMENTATION
#include<stb/stb_image.h>
