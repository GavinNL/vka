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
#define APP_TITLE "Multibuffer Vertex Attributes"

#include <vka/core/camera.h>
#include <vka/core/transform.h>
#include <vka/core/primatives.h>

#include <vka/utils/buffer_memory_manager.h>
#include <vka/core/managed_buffer.h>
#include <vka/utils/buffer_pool.h>

#include <vka/eng/mesh_manager.h>

#include <vka/core/offscreen_target.h>

#include <vka/utils/glfw_window_handler.h>

#include "vulkan_app.h"

// This is the structure of the uniform buffer we want.
// it needs to match the structure in the shader.
struct per_frame_uniform_t
{
    glm::mat4 view;
    glm::mat4 proj;
#if defined ENABLE_UNIFORM
    int use_uniform = 1;
#else
    int use_uniform = 0;
#endif
};

// This data will be written directly to the command buffer to
// be passed to the shader as a push constant.
struct push_constants_base_t
{
    glm::mat4 model;
    int index; // index into the texture array layer
    int miplevel; // mipmap level we want to use, -1 for shader chosen.
};

struct push_constants_t : public push_constants_base_t
{
    uint8_t _buffer[ 256 - sizeof(push_constants_base_t)];
};

/**
 * @brief The mesh_info_t struct
 * Information about the mesh, number of indices to draw and
 * offset in the index array
 */
struct mesh_info_t
{
    uint32_t index_offset  = 0; // index offset
    uint32_t count         = 0; // number of indices or vertices
    uint32_t vertex_offset = 0; // vertex offset

    std::vector<vka::sub_buffer*> m_buffers;
    vka::sub_buffer              *m_index_buffer=nullptr;

    void bind(vka::command_buffer & cb)
    {
        uint32_t i = 0;
        if( m_index_buffer)
        {
            cb.bindIndexSubBuffer(m_index_buffer, vk::IndexType::eUint16);
        }

        for(auto & b : m_buffers)
            cb.bindVertexSubBuffer( i++, b);
    }

    void draw( vka::command_buffer & cb)
    {
        if( m_index_buffer)
            cb.drawIndexed(count, 1 , index_offset, vertex_offset,0);
        else
            cb.draw(count,1,vertex_offset,0);
    }
};




/**
 * @brief The RenderComponent_t class
 *
 * The RenderComponent class is used to hold all the infromation
 * required to render an object on the screen.  This includes:
 *
 * 1. The pipeline
 * 2. a map of descriptor sets
 * 3. push constants (transformations, etc)
 * 4. a pointer to the mesh object on the gpu
 */
class RenderComponent_t
{
public:

    vka::pipeline                           *m_pipeline = nullptr;
    std::map<uint32_t, vka::descriptor_set*> m_descriptor_sets;
    push_constants_t                         m_push;
    std::shared_ptr<vka::mesh>               m_mesh_m;

    bool m_draw_axis = true;
};


class AxisRenderer_t
{
public:
    vka::pipeline * m_pipeline;
    vka::command_buffer m_commandbuffer;

    AxisRenderer_t(vka::command_buffer & cb , vka::pipeline * pipeline) : m_pipeline(pipeline) , m_commandbuffer(cb)
    {
        m_commandbuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, *m_pipeline );
    }

    void operator ()( glm::mat4 const & mvp)
    {

        m_commandbuffer.pushConstants( m_pipeline->get_layout(),
                                       vk::ShaderStageFlagBits::eVertex,
                                       0,
                                       sizeof(glm::mat4),
                                       &mvp);
        m_commandbuffer.draw(6,1,0,0);
    }
};


/**
 * @brief The ComponentRenderer_t class
 *
 * The component renderer is used to render a RenderComponent
 * by writing the commands to a command buffer
 */
class ComponentRenderer_t
{
    vka::pipeline  *m_pipeline = nullptr;
    vka::sub_buffer *m_ibuffer = nullptr;
    vka::sub_buffer *m_vbuffer = nullptr;

public:
    // given a render component, draw
    // it into the command buffer.
    void operator ()( vka::command_buffer & cb,
                      RenderComponent_t * obj)
    {
        // ===== bind the pipeline that we want to use next =======
        if( obj->m_pipeline != m_pipeline)
        {
            m_pipeline = obj->m_pipeline;


            cb.bindPipeline( vk::PipelineBindPoint::eGraphics, *m_pipeline );

            for(auto & d : obj->m_descriptor_sets)
            {
                cb.bindDescriptorSet(vk::PipelineBindPoint::eGraphics,
                                     m_pipeline,
                                     d.first, // binding index
                                     d.second);
            }
        }


        cb.pushConstants( m_pipeline->get_layout(),
                          vk::ShaderStageFlagBits::eVertex,
                          0,
                          sizeof(push_constants_t),
                          &obj->m_push);

        obj->m_mesh_m->bind(cb);
        obj->m_mesh_m->draw(cb);

    }
};


struct App : public VulkanApp
{

  /**
   * @brief init_pools
   *
   * Initialize all the pools we need.
   */
  void init_pools()
  {
      //==========================================================================
      // Initialize the Command and Descriptor Pools
      //==========================================================================
      m_descriptor_pool = m_Context.new_descriptor_pool("main_desc_pool");
      m_descriptor_pool->set_pool_size(vk::DescriptorType::eCombinedImageSampler, 25);
      m_descriptor_pool->set_pool_size(vk::DescriptorType::eUniformBuffer, 1);
      m_descriptor_pool->set_pool_size(vk::DescriptorType::eUniformBufferDynamic, 1);
      m_descriptor_pool->create();

      m_command_pool = m_Context.new_command_pool("main_command_pool");
      //==========================================================================


      // Initialize the buffer poool. The buffer pool is
      // used to allocate subbuffers used for different things (vertex/index/uniform)
      // By default, the buffer pool is DeviceLocal and can be used for Vertex, Index and Uniform buffers
      m_buffer_pool = m_Context.new_buffer_pool("main_buffer_pool");
      m_buffer_pool->set_size( 50*1024*1024 ); // create 50Mb buffer
      m_buffer_pool->create();
  }


  void init_memory()
  {
      // Allocate sub buffers from the buffer pool. These buffers can be
      // used for indices/vertices or uniforms.
      m_ubuffer = m_buffer_pool->new_buffer( 10*1024*1024 );

      m_sbuffer = m_Context.new_staging_buffer( "staging_buffer", 10*1024*1024);


  }

  void init_pipelines()
  {
      // Create the graphics Pipeline
      auto M = vka::box_mesh(1,1,1);

        m_pipelines.main = m_Context.new_pipeline("main_pipeline");
        m_pipelines.main->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                ->set_vertex_shader(   "resources/shaders/push_consts_default/push_consts_default_v.spv", "main" )   // the shaders we want to use
                ->set_fragment_shader( "resources/shaders/push_consts_default/push_consts_default_f.spv", "main" ) // the shaders we want to use

                // tell the pipeline that attribute 0 contains 3 floats
                // and the data starts at offset 0
                ->set_vertex_attribute(0, 0 ,  0 , M.format( vka::VertexAttribute::ePosition) , sizeof(glm::vec3) )
                // tell the pipeline that attribute 1 contains 3 floats
                // and the data starts at offset 12
                ->set_vertex_attribute(1, 1 ,  0       , M.format( vka::VertexAttribute::eUV) , sizeof(glm::vec2) )

                ->set_vertex_attribute(2, 2 ,  0   , M.format( vka::VertexAttribute::eNormal) , sizeof(glm::vec3) )

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

                // Add a push constant to the layout. It is accessable in the vertex shader
                // stage only.
                ->add_push_constant( sizeof(push_constants_t), 0, vk::ShaderStageFlagBits::eVertex)
                //
                ->set_render_pass( m_screen->get_renderpass() )
                ->create();


        //======================================================================
        // The axis shader does not require any vertex attributes, it contains
        // 6 vertices, 2 vertices per axis and are drawn as a line list.
        // the vertex positions are generated dynamically in the shader.
        // The shader requires a single push constant which is the
        // cumulative Model-View-Projection matrix.
        //======================================================================
        m_pipelines.axis = m_Context.new_pipeline("axis_pipeline");
        m_pipelines.axis->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                ->set_vertex_shader(   "resources/shaders/axis_shader/axis_shader_v.spv", "main" )   // the shaders we want to use
                ->set_fragment_shader( "resources/shaders/axis_shader/axis_shader_f.spv", "main" ) // the shaders we want to use

                ->set_toplogy(vk::PrimitiveTopology::eLineList)
                ->set_line_width(1.0f)
                // Triangle vertices are drawn in a counter clockwise manner
                // using the right hand rule which indicates which face is the
                // front
                ->set_front_face(vk::FrontFace::eCounterClockwise)

                // Cull all back facing triangles.
                ->set_cull_mode(vk::CullModeFlagBits::eBack)
                // Add a push constant to the layout. It is accessable in the vertex shader
                // stage only.
                ->add_push_constant( sizeof(glm::mat4), 0, vk::ShaderStageFlagBits::eVertex)
                //
                ->set_render_pass( m_screen->get_renderpass() )
                ->create();
        //======================================================================

  }

  /**
   * @brief load_meshs
   *
   * Load the meshs that we are going to use to render. Here we are going to
   * load a Box and a Sphere.
   *
   * We are going to use the vka primitaves to generate a vertex and index
   * values on the Host and then copy this data to the GPU.
   */
  void load_meshs()
  {
      // Initialize the mesh manager.
      // Set the total buffer size to 20MB
      m_mesh_manager.set_context(&m_Context);
      m_mesh_manager.set_buffer_size( 1024*1024*20 );



      //===============
      std::vector<vka::mesh_t>   meshs;

      // create the box and sphere mesh on the host memory
      meshs.push_back( vka::box_mesh(1,1,1) );
      meshs.push_back( vka::sphere_mesh(0.5,20,20) );

      // convert the meshs into gpu meshes so that they can
      // be rendered. convert_to_gpu_mesh returns a shared pointer to
      // the mesh object. But this mesh object is also stored within the
      // mesh manager. We can always get the reference by identifying it
      // with the name provided: "box", "sphere"
      convert_to_gpu_mesh("box",    meshs[0]);
      convert_to_gpu_mesh("sphere", meshs[1]);

  }

  /**
   * @brief convert_to_gpu_mesh
   * @param name
   * @param M
   * @return
   *
   * This function takes a host Mesh and copies it to the GPU and returns
   * a reference to the GPU mesh which can be used to render.
   */
  std::shared_ptr<vka::mesh> convert_to_gpu_mesh( const std::string & name, vka::mesh_t & M)
  {
        // Create a new mesh from the mesh_manager
        auto m = m_mesh_manager.new_mesh(name);

        // Get an attribute view for the Position/UV/Normals
        auto P = M.get_attribute_view<glm::vec3>( vka::VertexAttribute::ePosition);
        auto U = M.get_attribute_view<glm::vec2>( vka::VertexAttribute::eUV    );
        auto N = M.get_attribute_view<glm::vec3>( vka::VertexAttribute::eNormal);
        auto I = M.get_index_view<uint16_t>();

        // Temporary vectors to store the data so they can be easily copied
        std::vector< glm::vec3> p;
        std::vector< glm::vec3> n;
        std::vector< glm::vec2> u;

        // Push the data onto the vectors so that the data is contigiuos
        // in memory for easy copying
        for(uint32_t i=0;i<P.size();i++)
        {
            p.push_back( P[i] );
            n.push_back( N[i] );
            u.push_back( U[i] );
        }

        // Set the total number of vertices/indices
        m->set_num_vertices( M.num_vertices() );
        m->set_num_indices( M.num_indices(), sizeof(uint16_t));

        // Set the type of attributes we want for the gpu_mesh
        m->set_attribute(0, sizeof(glm::vec3) ); // position
        m->set_attribute(1, sizeof(glm::vec2) ); // UV
        m->set_attribute(2, sizeof(glm::vec3) ); // Normals

        // We can now allocate the data for the mesh on the gpu
        m->allocate();

        // and copy the data
        m->copy_attribute_data(0, p.data(), p.size() * sizeof(glm::vec3));
        m->copy_attribute_data(1, u.data(), u.size() * sizeof(glm::vec2));
        m->copy_attribute_data(2, n.data(), n.size() * sizeof(glm::vec3));

        m->copy_index_data( M.index_data(), M.index_data_size() );

        return m;
  }


  /**
   * @brief load_textures
   *
   * Loads textures into the GPU memory so they can be used within the shaders.
   *
   * The most effecient way to handle multiple textures is to store the
   * textures within a texture array instead of single use textures
   */
  void load_textures()
  {
      // 1. First create a texture array.
      m_texture_array = m_Context.new_texture2darray("main_texture_array");
      m_texture_array->set_size( 512 , 512 );
      m_texture_array->set_format(vk::Format::eR8G8B8A8Unorm);
      m_texture_array->set_mipmap_levels(8);
      m_texture_array->set_layers(10); // we can store 10 textures
      m_texture_array->create();
      m_texture_array->create_image_view(vk::ImageAspectFlagBits::eColor);


     // 2. First load host_image into memory, and specifcy we want 4 channels.
      std::vector<std::string> image_paths = {
          "resources/textures/Brick-2852a.jpg",
          "resources/textures/noise.jpg"
      };

      uint32_t layer = 0;

      for(auto & img : image_paths)
      {
          // load the image into a host_image
          vka::host_image D(img, 4);

          // make sure that the image is exactly 512x512
          if( !(D.width() == 512 && D.height()==512) )
          {
              D.resize(512,512);
          }

          // Copy the data into the texture array
          m_texture_array->copy_image( D , layer++, vk::Offset2D(0,0) );
      }
  }



  void init_descriptor_sets()
  {
      // we want a descriptor set for set #0 in the pipeline.
      m_dsets.texture_array = m_pipelines.main->create_new_descriptor_set(0, m_descriptor_pool);
      //  attach our texture to binding 0 in the set.
      m_dsets.texture_array->attach_sampler(0, m_texture_array);
      m_dsets.texture_array->update();

      m_dsets.uniform_buffer = m_pipelines.main->create_new_descriptor_set(1, m_descriptor_pool);
      m_dsets.uniform_buffer->attach_uniform_buffer(0, m_ubuffer, sizeof(per_frame_uniform_t), m_ubuffer->offset());
      m_dsets.uniform_buffer->update();

  }
  virtual void onInit()
  {
      init_pools();


      init_memory();

      load_meshs();


      load_textures();


      init_pipelines();


      init_descriptor_sets();


      m_image_available_semaphore = m_Context.new_semaphore("image_available_semaphore");
      m_render_complete_semaphore = m_Context.new_semaphore("render_complete_semaphore");

      m_commandbuffer = m_command_pool->AllocateCommandBuffer();

      init_scene();


      // create a callback function for the onKey event for the window.
      // we will use this to control the camera
      keyslot = onKey << [&] (vka::Key k, bool down)
      {
          float x=0;
          float y=0;

          if( is_pressed(vka::Key::A    ) || is_pressed(vka::Key::LEFT )) x += -1;
          if( is_pressed(vka::Key::D    ) || is_pressed(vka::Key::RIGHT)) x +=  1;
          if( is_pressed(vka::Key::W    ) || is_pressed(vka::Key::UP   ))   y += -1;
          if( is_pressed(vka::Key::S    ) || is_pressed(vka::Key::DOWN ))  y +=  1;

          m_Camera.set_acceleration( glm::vec3( x, 0, y ) );

      };

      // create a callback function for the onMouseMove event.
      // We will use this to control the camera.
      mouseslot =  onMouseMove << [&] (double dx, double dy)
      {
        dx = mouse_x() - dx;
        dy = mouse_y() - dy;
        if( is_pressed( vka::Button::RIGHT))
        {
            show_cursor(false);
            if( fabs(dx) < 10) m_Camera.yaw(   -dx*0.001f);
            if( fabs(dy) < 10) m_Camera.pitch( dy*0.001f);
        }
        else
        {
            show_cursor(true);
        }

      };

  }



  /**
   * @brief UpdateUniforms
   * @param cb
   *
   * This method will copy the Per-Frame data from the host
   * to the device so that the shaders can use them.
   */
  void UpdateUniforms(vka::command_buffer & cb)
  {
      // Since the uniform buffer is Device Local we must first copy
      // the data from the Host -> Staging Buffer -> Uniform Buffer.

#define SINGLE_COPY
    auto alignment = 256;

    // First memory map the staging buffer
    unsigned char * S = static_cast<unsigned char*>(m_sbuffer->map_memory());

    vk::DeviceSize src_offset  = 0;                             // Index to copy to
    vk::DeviceSize const size  = sizeof( per_frame_uniform_t ); // number of bytes to copy

    // Copy the data from the Host to the Staging buffer
    memcpy( &S[src_offset], &m_frame_uniform, size );

    // Now record the Command Buffer commadn to copy the data from
    // the Staging Buffer to the Uniform Buffer
    cb.copySubBuffer( *m_sbuffer, m_ubuffer, vk::BufferCopy(src_offset, 0 ,size)  );

    src_offset += size;
  }


  /**
   * @brief onFrame
   * @param dt
   * @param T
   *
   * The onFrame command is what is called in the main loop.
   * In this function we need to do the following:
   *
   * 1. Calculate any new positions of objects (and cameras) on the host
   * 2. Reset the command buffer
   * 3. Update the GPU buffers with the new positions of the objects
   * 4. Record teh commands to render the objects to the command buffer
   * 5. Submit the command buffer so that it will render the objects
   * 6. Present the final image to the screen
   */
  virtual void onFrame(double dt, double T)
  {
      //========================================================================
      // 1. Calculate the positions of the new objects/cameras
      m_Camera.calculate();

      m_frame_uniform.view = m_Camera.get_view_matrix();
      m_frame_uniform.proj = m_Camera.get_proj_matrix();
      m_frame_uniform.proj[1][1] *= -1;

      //========================================================================


      // RESET THE COMMAND BUFFER
      m_commandbuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);

      // BEGIN RECORDING TO THE COMMAND BUFFER
      m_commandbuffer.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse) );



      //========================================================================
      // 2. Update the uniforms by copying the data from the CPU to the GPU
      //    Copying buffer data must be done before calling any render commands.
      UpdateUniforms(m_commandbuffer);
      //========================================================================

      //========================================================================
      // 3. Render all the objects.

      // Prepare the next frame for rendering and get the frame_index
      // that we will be drawing to.
      //    - The semaphore we pass into it will be signaled when the frame is
      //      ready
      uint32_t frame_index = m_screen->prepare_next_frame(m_image_available_semaphore);


      // start the actual rendering
      m_screen->beginRender( m_commandbuffer , frame_index );

          // Main component renderer
          ComponentRenderer_t R;

          for(auto * comp : m_Objs)
          {
              R(m_commandbuffer, comp);
          }
      // end the rendering
      m_screen->endRender(m_commandbuffer);
      //========================================================================


      // STOP WRITING TO THE COMMAND BUFFER
      m_commandbuffer.end();


      // Submit the the command buffer to the GPU.
      // A this point all the geometry will be rendered to the screen's image
      m_Context.submit_command_buffer(m_commandbuffer,             // Execute this command buffer
                                      m_image_available_semaphore, // but wait until this semaphore has been signaled
                                      m_render_complete_semaphore);// signal this semaphore when it is done


     //=============== Final Presentation =======================================
     // Present the final image to the screen. But wait on for the semaphore to be flagged first.
     m_screen->present_frame( frame_index, m_render_complete_semaphore );

  }




  /**
   * @brief init_scene
   *
   * Intialize the scene with objects
   */
  void init_scene()
  {
      #define MAX_OBJ 80

      float x = 0;
      float y = 0;
      float z = 0;

      for(int i=0; i<MAX_OBJ ;i++)
      {
        auto * obj = new RenderComponent_t();

        obj->m_mesh_m   = m_mesh_manager.get_mesh("sphere");

        obj->m_pipeline = m_pipelines.main;

        obj->m_descriptor_sets[0] = m_dsets.texture_array;
        obj->m_descriptor_sets[1] = m_dsets.uniform_buffer;


        vka::transform T;
        T.set_position( glm::vec3(0,0,0));
        T.set_scale( glm::vec3(1,1,1));


        obj->m_push.model    = vka::transform( 1.25f*glm::vec3( x , y, z)   ).get_matrix(); // T.get_matrix();
        obj->m_push.index    = i%2;
        obj->m_push.miplevel = -1;


        x += 1;
        if( x > pow(MAX_OBJ,1.0/3.0) )
        {
            x =0;
            y += 1;
        }
        if(y > pow(MAX_OBJ,1.0/3.0) )
        {
            y = 0;
            z += 1;
        }

        m_Objs.push_back(obj);
    }

    //======================

    //========================
    // Initialize the camera
    glm::vec3 cam_position(4,2,4);
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

    m_frame_uniform.view = m_Camera.get_view_matrix();
    m_frame_uniform.proj = m_Camera.get_proj_matrix();
    m_frame_uniform.proj[1][1] *= -1;

  }



  //=====================================
  vka::descriptor_pool *m_descriptor_pool;
  vka::command_pool    *m_command_pool;

  vka::mesh_manager     m_mesh_manager;

  vka::buffer_pool     *m_buffer_pool;
  vka::sub_buffer      *m_ubuffer;
  vka::buffer          *m_sbuffer;

  vka::texture2darray  *m_texture_array;

  vka::semaphore       *m_image_available_semaphore;
  vka::semaphore       *m_render_complete_semaphore;

  //========================================
  vka::camera         m_Camera;
  per_frame_uniform_t m_frame_uniform;

  struct
  {
    vka::descriptor_set * texture_array;
    vka::descriptor_set * uniform_buffer;

  } m_dsets;

  struct
  {    
    vka::pipeline * main;
    vka::pipeline * axis;
  } m_pipelines;


  vka::command_buffer m_commandbuffer;

  //====================================

  std::vector< RenderComponent_t* > m_Objs;

  vka::signal<void(double       , double)>::slot mouseslot;
  vka::signal<void(vka::Key,      int   )>::slot keyslot;

};

int main(int argc, char ** argv)
{
     App A;

     A.init( WIDTH, HEIGHT, APP_TITLE);
     //A.init_default_renderpass(WIDTH,HEIGHT);

     A.start_mainloop();


     return 0;
}

#define STB_IMAGE_IMPLEMENTATION
#include<stb/stb_image.h>

