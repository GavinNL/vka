/*
 * Example 6: Camera,  Transformation, and Window Inputs
 *
 * This example demonstrates the use of three classes: vka::camera,
 * vka::transform and vka::GLFW_Window_Handler.
 *
 * vka::transform is a class which makes manipulating the postion/orientation
 * of an object easier.
 *
 * The vka::camera class assists in creating the matrices required for
 * the view and projection matrices. In addition it also provides some basic
 * physics to provide motion.
 *
 * call camera.calculate() in the main loop to do the physics calcations
 *
 * vka::GLFW_Window_Handler is a wrapper a around the GLFWWindow, it provides
 * callbacks for input events such as key presses and mouse motion.
 *
 * In this example we are going to use all three of these classes.  In addition
 * we are going to take Example_05 and remove all the verbose methods and replace
 * them with vka specific helper functions.
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



#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#define WIDTH 1024
#define HEIGHT 768
#define APP_TITLE "Example_06 - Camera Control , Transformations, and Sub Buffers"

#include <vka/core/camera.h>
#include <vka/core/transform.h>
#include <vka/core/primatives.h>

#include <vka/utils/buffer_memory_manager.h>
#include <vka/core/managed_buffer.h>
#include <vka/utils/buffer_pool.h>

#include <vka/utils/glfw_window_handler.h>


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
 * @brief The mesh_info_t struct
 * Information about the mesh, number of indices to draw and
 * offset in the index array
 */
struct mesh_info_t
{
    uint32_t offset; // index offset
    uint32_t count; // number of indices
    uint32_t vertex_offset; // vertex offset
};


/**
 * @brief The Object_t struct
 *
 * This class holds all information about an object we want to draw.
 */
struct Object_t
{
    vka::transform           m_transform; // the world space transformation


    vka::pipeline           *m_pipeline; // the graphcis pipeline to use

    dynamic_uniform_buffer_t m_uniform;  // the struct of the uniform buffer

    size_t                   m_uniform_offset; // the offset into the buffer
    vka::descriptor_set     *m_uniform_descriptor_set; // the buffer descriptor set.

    push_constants_t         m_push; // push_constants

    mesh_info_t              m_mesh; // the mesh to use
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


uint32_t tick()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime).count();

    //return time;
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

    vka::screen * screen = C.new_screen("screen");
    screen = C.new_screen("m_win");
    screen->set_extent( vk::Extent2D(WIDTH,HEIGHT) );
    screen->set_surface( surface );
    screen->create();

    //==========================================================================


    //==========================================================================
    #define MAX_OBJECTS 3
    vka::GLFW_Window_Handler m_Window(window);
    std::vector<Object_t>    m_Objects;


    //==========================================================================
    // The camera class is used to help position the camera nd keep track
    // of all the variables associated with it. It also provides methods for
    // moving the camera through space
    //==========================================================================
    vka::camera              m_Camera;
    //==========================================================================



    //==========================================================================
    // Initialize the Command and Descriptor Pools
    //==========================================================================
    vka::descriptor_pool* descriptor_pool = C.new_descriptor_pool("main_desc_pool");
    descriptor_pool->set_pool_size(vk::DescriptorType::eCombinedImageSampler, 2);
    descriptor_pool->set_pool_size(vk::DescriptorType::eUniformBuffer, 1);
    descriptor_pool->set_pool_size(vk::DescriptorType::eUniformBufferDynamic, 1);

    descriptor_pool->create();

    vka::command_pool* cp = C.new_command_pool("main_command_pool");
    //==========================================================================






/*==============================================================================
 Create the Vertex and Index buffers.

 This time we are goign to use a Buffer Pool. Buffer Pools are a VKA specifc
 construct. Simply put, it is esentially a large device-local
 buffer set to be used as vertex|index|uniform.  It is meant to be a large
 buffer big enough to hold many smaller pieces of data.

 Buffer-pools allocate sub_buffers, which hold the same vk::Buffer handle
 as the buffer_pool, but also contains the byte offset into the memory.
 see: https://developer.nvidia.com/vulkan-memory-management

 Sub buffer allocation is handled automatically within the bufferPool. When
 a sub buffer is finished its use, it should free the resource.
==============================================================================*/

        vka::buffer_pool * buf_pool = C.new_buffer_pool("buffer_pool");
        buf_pool->set_size(1024*1024*50); // create 50Mb buffer
        buf_pool->create();
//============================================================================//

        std::vector< mesh_info_t > m_mesh_info;

        std::vector<vka::mesh_t>   meshs;
        meshs.push_back( vka::box_mesh(1,1,1) );
        meshs.push_back( vka::sphere_mesh(0.5,20,20) );

        // Allocate sub buffers from the buffer pool. These buffers can be
        // used for indices/vertices or uniforms.
        vka::sub_buffer* vertex_buffer = buf_pool->new_buffer( 16*1024 );
        vka::sub_buffer* index_buffer  = buf_pool->new_buffer( 16*1024 );
        vka::sub_buffer* u_buffer      = buf_pool->new_buffer( 16*1024 );
        vka::sub_buffer* du_buffer     = buf_pool->new_buffer( 16*1024 );

        // allocate a staging buffer of 10MB since we will need this to copy
        // data to the sub_buffers
        vka::buffer * staging_buffer = C.new_staging_buffer( "sb", 1024*16 );


        for( auto & M : meshs)
        {

            // sub_buffer::insert inserts data into the sub buffer at an available
            // space and returns a sub_buffer object.

            auto m1v = vertex_buffer->insert( M.vertex_data(), M.vertex_data_size() );
            auto m1i = index_buffer->insert(  M.index_data() , M.index_data_size()  );

            // If you wish to free the memory you have allocated, you shoudl call
            // vertex_buffer->free_buffer_object(m1v);

            assert( m1v.m_size != 0);
            assert( m1i.m_size != 0);

            mesh_info_t m_box_mesh;
            m_box_mesh.count         = M.num_indices();

            // the offset returned is the byte offset, so we need to divide it
            // by the index/vertex size to get the actual index/vertex offset
            m_box_mesh.offset        =  m1i.m_offset  / M.index_size();
            m_box_mesh.vertex_offset =  m1v.m_offset  / M.vertex_size();

            m_mesh_info.push_back(m_box_mesh);
        }


//==============================================================================
// Create the Texture2dArray
//
//==============================================================================

    // 1. First load host_image into memory, and specifcy we want 4 channels.
        std::vector<std::string> image_paths = {
            "resources/textures/Brick-2852a.jpg",
            "resources/textures/noise.jpg"
        };



    // 2. Use the context's helper function to create a device local texture
    //    We will be using a texture2d which is a case specific version of the
    //    generic texture
        vka::texture2darray * tex = C.new_texture2darray("test_texture");
        tex->set_size( 512 , 512 );
        tex->set_format(vk::Format::eR8G8B8A8Unorm);
        tex->set_mipmap_levels(8);
        tex->set_layers(10);
        tex->create();
        tex->create_image_view(vk::ImageAspectFlagBits::eColor);


        uint32_t layer=0;
        for(auto & img : image_paths)
        {
            vka::host_image D(img, 4);
            assert( D.width() == 512 && D.height()==512);
            tex->copy_image( D , layer++, vk::Offset2D(0,0) );
        }

//==============================================================================


//==============================================================================
// Create a Rendering pipeline
//
//==============================================================================
        // create the vertex shader from a pre compiled SPIR-V file
        vka::shader* vertex_shader = C.new_shader_module("vs");
        vertex_shader->load_from_file("resources/shaders/mipmaps/mipmaps_v.spv");

        // create the fragment shader from a pre compiled SPIR-V file
        vka::shader* fragment_shader = C.new_shader_module("fs");
        fragment_shader->load_from_file("resources/shaders/mipmaps/mipmaps_f.spv");

        vka::pipeline* pipeline = C.new_pipeline("triangle");

        auto & M = meshs.front();
        // Create the graphics Pipeline
          pipeline->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                  ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                  ->set_vertex_shader(   vertex_shader )   // the shaders we want to use
                  ->set_fragment_shader( fragment_shader ) // the shaders we want to use

                  // tell the pipeline that attribute 0 contains 3 floats
                  // and the data starts at offset 0
                  ->set_vertex_attribute(0,0 ,  M.offset( vka::VertexAttribute::ePosition) , M.format( vka::VertexAttribute::ePosition) , M.vertex_size() )
                  // tell the pipeline that attribute 1 contains 3 floats
                  // and the data starts at offset 12
                  ->set_vertex_attribute(0,1 ,  M.offset( vka::VertexAttribute::eUV)       , M.format( vka::VertexAttribute::eUV) , M.vertex_size() )

                  ->set_vertex_attribute(0,2 ,  M.offset( vka::VertexAttribute::eNormal)   , M.format( vka::VertexAttribute::eNormal) , M.vertex_size() )

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

                  // Tell teh shader that we are going to use a uniform buffer
                  // in Set #0 binding #0
                  ->add_dynamic_uniform_layout_binding(2, 0, vk::ShaderStageFlagBits::eVertex)

                  // Add a push constant to the layout. It is accessable in the vertex shader
                  // stage only.
                  ->add_push_constant( sizeof(push_constants_t), 0, vk::ShaderStageFlagBits::eVertex)
                  //
                  ->set_render_pass( screen->get_renderpass() )
                  ->create();



//==============================================================================
// Create the objects
//==============================================================================


m_Objects.resize(MAX_OBJECTS);
for(uint32_t j=0;j<MAX_OBJECTS;j++)
{
    double a = (j / (double)MAX_OBJECTS) * 2.0 * 3.14159;
    m_Objects[j].m_mesh          = m_mesh_info[0];
    m_Objects[j].m_pipeline      = pipeline;
    m_Objects[j].m_push.index    = j%2;
    m_Objects[j].m_push.miplevel = -1;
    m_Objects[j].m_transform.set_position(  1.5f*glm::vec3( cos(a), 0, sin(a) ));
}

m_Objects[1].m_mesh = m_mesh_info[1];
m_Objects[0].m_transform.set_position( glm::vec3(3,0,0));
m_Objects[0].m_transform.set_scale( glm::vec3(2,2,2));


m_Objects[1].m_transform.set_position( glm::vec3(0,2,0));
m_Objects[2].m_transform.set_position( glm::vec3(0,0,2));


//==============================================================================

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
    ubuffer_descriptor->attach_uniform_buffer(0, u_buffer, sizeof(uniform_buffer_t), u_buffer->offset());
    ubuffer_descriptor->update();

    vka::descriptor_set * dubuffer_descriptor = pipeline->create_new_descriptor_set(2, descriptor_pool);
    dubuffer_descriptor->attach_dynamic_uniform_buffer(0, du_buffer, sizeof(dynamic_uniform_buffer_t), du_buffer->offset());
    dubuffer_descriptor->update();

    vka::array_view<uniform_buffer_t> staging_buffer_map        = staging_buffer->map<uniform_buffer_t>();
    vka::array_view<dynamic_uniform_buffer_t> staging_dbuffer_map = staging_buffer->map<dynamic_uniform_buffer_t>(sizeof(uniform_buffer_t));




    vka::semaphore * image_available_semaphore = C.new_semaphore("image_available_semaphore");
    vka::semaphore * render_complete_semaphore = C.new_semaphore("render_complete_semaphore");





    // They GLFW_Window_Manager has signals which you can attach functions to.
    // We are going to use a lamda function to determine whenever a key was pressed
    // and then change the Camera's acceleration based on what key was pressed
    //
    // We are going to create a general FPS controller.

    // must save the returned slot otherwise, the signal will be
    // unregistered
    auto keyslot = m_Window.onKey << [&] (vka::Key k, bool down)
    {
        float x=0;
        float y=0;

        if( m_Window.is_pressed(vka::Key::A    ) || m_Window.is_pressed(vka::Key::LEFT )) x += -1;
        if( m_Window.is_pressed(vka::Key::D    ) || m_Window.is_pressed(vka::Key::RIGHT)) x +=  1;
        if( m_Window.is_pressed(vka::Key::W    ) || m_Window.is_pressed(vka::Key::UP   ))   y += -1;
        if( m_Window.is_pressed(vka::Key::S    ) || m_Window.is_pressed(vka::Key::DOWN ))  y +=  1;

        m_Camera.set_acceleration( glm::vec3( x, 0, y ) );

    };

    auto mouseslot =  m_Window.onMouseMove << [&] (double dx, double dy)
    {
      dx = m_Window.mouse_x() - dx;
      dy = m_Window.mouse_y() - dy;
      if( m_Window.is_pressed( vka::Button::RIGHT))
      {
          m_Window.show_cursor(false);
          if( fabs(dx) < 10) m_Camera.yaw(   -dx*0.001f);
          if( fabs(dy) < 10) m_Camera.pitch( dy*0.001f);
      }
      else
      {
          m_Window.show_cursor(true);
      }
      //std::cout << "Moving: " << dx << ", " << dy << std::endl;
    };



    // Initialize the camera
    glm::vec3 cam_position(2,2,2);
    glm::vec3 cam_looking_at(0,0,0);
    float field_of_view = glm::radians(60.f);
    float aspect_ratio  = WIDTH / (float)HEIGHT;
    float near_plane    = 0.1f;
    float far_plane     = 100.0f;
    m_Camera.set_fov(  field_of_view );
    m_Camera.set_aspect_ratio( aspect_ratio );
    m_Camera.set_near_plane(near_plane); // default value
    m_Camera.set_far_plane(far_plane);// default value
    m_Camera.set_position( cam_position );
    m_Camera.lookat( cam_looking_at , {0,1,0});


    // Dynamic Uniform buffers have a set alignment, meaning the number of bytes
    // bound to the pipeline must be a multiple of the alignment
    // In most case the alignment is 256 bytes.
    auto alignment = C.get_physical_device_limits().minUniformBufferOffsetAlignment;

    auto cb = cp->AllocateCommandBuffer();
    while (!glfwWindowShouldClose(window) )
    {
       float t = get_elapsed_time();

       // [NEW] We need to call the camera's calculate method periodically
       // this is so that it can perform the proper physics calculations
       // to create smooth motion.
       m_Camera.calculate();

      // Get the next available image in the swapchain

      glfwPollEvents();

      // reset the command buffer so that we can record from scratch again.
      cb.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
      cb.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse) );



      // The staging buffer will hold all data that will be sent to
      // the other two buffers.
      //
      // +--------------+------+--------------------------------+
      // | uniform_data | obj1 | obj2 |                         | Staging Buffer
      // +--------------+------+--------------------------------+


      // Copy the uniform buffer data into the staging buffer
      const float AR = WIDTH / ( float )HEIGHT;
      staging_buffer_map[0].view        = m_Camera.get_view_matrix();
      staging_buffer_map[0].proj        = m_Camera.get_proj_matrix();
      staging_buffer_map[0].proj[1][1] *= -1;

      // +------------------------------------------------------+
      // | uniform_data |                                       | Uniform Buffer
      // +------------------------------------------------------+

      // Copy the uniform buffer data from the stating buffer to the uniform sub_buffer. This normally only needs to be done
      // once per rendering frame because it contains frame constant data.
      cb.copySubBuffer( *staging_buffer ,  u_buffer , vk::BufferCopy{ 0,0,sizeof(uniform_buffer_t) } );


      // Copy the dynamic uniform buffer data from the staging buffer
      // to the appropriate offset in the Dynamic Uniform Buffer.
      // +-------------+---------------------------------------+
      // | obj1        | obj2         | obj3...                | Dynamic Uniform Buffer
      // +-------------+---------------------------------------+
      // |<-alignment->|

      for(uint32_t j=0; j < m_Objects.size(); j++)
      {
          // convert the transform into a matrix which will be stored
          // in the uniform struct
          m_Objects[j].m_uniform.model = m_Objects[j].m_transform.get_matrix();


          // copy uniform struct to staging buffer
          staging_dbuffer_map[j]       = m_Objects[j].m_uniform;

          // byte offset within the staging buffer where the data resides
          auto srcOffset = sizeof(uniform_buffer_t) + j * sizeof(dynamic_uniform_buffer_t);
          // byte offset within the dynamic uniform buffer where to copy the data
          auto dstOffset = j * alignment ;//+ du_buffer->m_offset;
          // number of bytes to copy
          auto size      = sizeof(dynamic_uniform_buffer_t);

          cb.copySubBuffer( *staging_buffer , du_buffer , vk::BufferCopy{ srcOffset,dstOffset, size } );

          m_Objects[j].m_uniform_offset = j * alignment ;
      }



      uint32_t frame_index = screen->prepare_next_frame(image_available_semaphore);
      screen->beginRender(cb, frame_index);

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
        //cb.bindVertexBuffers(0, vertex_buffer->get(), {vertex_buffer->m_offset} );
        //cb.bindIndexBuffer(  index_buffer->get() , index_buffer->m_offset , vk::IndexType::eUint16);

        cb.bindVertexSubBuffer(0, vertex_buffer);
        cb.bindIndexSubBuffer(index_buffer, vk::IndexType::eUint16);
      //========================================================================
      // Draw all the objects while binding the dynamic uniform buffer
      // to
      //========================================================================
      for(uint32_t j=0 ; j < m_Objects.size(); j++)
      {
          auto & obj = m_Objects[j];
          cb.pushConstants( pipeline->get_layout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(push_constants_t), &m_Objects[j].m_push);

          cb.bindDescriptorSets( vk::PipelineBindPoint::eGraphics,
                                                  pipeline->get_layout(),
                                                  2,
                                                  vk::ArrayProxy<const vk::DescriptorSet>( dubuffer_descriptor->get()),

                                                  vk::ArrayProxy<const uint32_t>( m_Objects[j].m_uniform_offset) );



    // draw 3 indices, 1 time, starting from index 0, using a vertex offset of 0

            cb.drawIndexed(m_Objects[j].m_mesh.count,
                           1,
                           m_Objects[j].m_mesh.offset,
                           m_Objects[j].m_mesh.vertex_offset,
                           0);
      }


      screen->endRender(cb);
      cb.end();

      // Submit the command buffers, but wait until the image_available_semaphore
      // is flagged. Once the commands have been executed, flag the render_complete_semaphore
      C.submit_command_buffer(cb, image_available_semaphore, render_complete_semaphore);

      // present the image to the surface, but wait for the render_complete_semaphore
      // to be flagged by the submit_command_buffer
      screen->present_frame(frame_index,render_complete_semaphore);

      std::this_thread::sleep_for( std::chrono::milliseconds(3) );
    }

    return 0;
}

#define STB_IMAGE_IMPLEMENTATION
#include<stb/stb_image.h>

