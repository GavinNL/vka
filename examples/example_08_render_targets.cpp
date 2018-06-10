/*
 * Example 8: Render Targets
 *
 * This example demonstrates how to use the vka::mesh_manager.
 *
 * The Mesh Manager is a wrapper around a single large buffer, which can
 * then allocate sub-buffers. The sub buffers are used for storing the
 * attributes of a mesh such as position/normals/UV coords/colors, etc.
 *
 * To use the Mesh Manager, you first initalize it with a certain size
 * of memory. Then you create new meshs from it and set them up with the
 * appropriate attributes. Then allocate the memory for the mesh.
 *
 * To draw a mesh. Use the vka::command_buffer::bind( mesh ) function
 * follows by the vka::command_bufer::draw( mesh ) command. Under the hood
 * these two fucntions
 *
 *
 *                      +----------------+
 *                      |                |
 *                      |  +----------------+                       +--------------------+
 *                      |  |                |                       |                    |
 *          Draw        |  |  +----------------+                    |                    |
 *OBJECT  +---------->  |  |  |                |     Compose        |   Screen           |
 *                      |  |  |                |  +-------------->  |                    |
 *                      +--|  |                |                    |                    |
 *                         |  |                |                    |                    |
 *                         +--|                |                    +--------------------+
 *                            |                |
 *                            +----------------+
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

/**
 * @brief The per_frame_uniform_t struct
 *
 * This structure will hold the information the scene will require per frame.
 * This includes the camera's position/projection informaiton
 * and anything else that only changes once per frame.
 */
struct per_frame_uniform_t
{
    glm::mat4 view;
    glm::mat4 proj;
};

/**
 * @brief The push_constants_base_t struct
 *
 * We are goign to use this structure to hold any information that
 * must be passed to the shader. We can use this block of
 * data to pass in the model matrix for each object
 * plus any additional information we might need such as
 * the mipmap level or the index into the texture layer
 */
struct push_constants_base_t
{
    glm::mat4 model;
    int index; // index into the texture array layer
    int miplevel; // mipmap level we want to use, -1 for shader chosen.
};

/**
 * @brief The push_constants_t struct
 *
 * Push constants can only be a certain size. It must be a minimum of 128bytes
 * according to the Vulkan spec, but some video cards can go up to 256
 */
struct push_constants_t : public push_constants_base_t
{
    static_assert( sizeof(push_constants_base_t) <= 128, "Push constants size is too large");
    uint8_t _buffer[ 128 - sizeof(push_constants_base_t)];
};

struct compose_pipeline_push_consts
{
    glm::vec2 position;
    glm::vec2 size;
    int layer;
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

    // We are going to use these variables to make the object
    // orbit around the origin at various speeds/distances
    double m_orbital_speed;
    double m_orbital_radius;
    double m_orbital_phase;
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

class FullScreenQuadRenderer_t
{
public:
    vka::pipeline * m_pipeline;
    vka::command_buffer m_commandbuffer;

    FullScreenQuadRenderer_t(vka::command_buffer & cb , vka::pipeline * pipeline) : m_pipeline(pipeline) , m_commandbuffer(cb)
    {
        m_commandbuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, *m_pipeline );
    }

    void operator()( vka::descriptor_set * dset, compose_pipeline_push_consts const & pc )
    {
        static uint32_t i=1;
        //uint32_t index = (i/500)%3;

        m_commandbuffer.bindDescriptorSet(vk::PipelineBindPoint::eGraphics,
                             m_pipeline,
                             0, // binding index
                             dset);
        m_commandbuffer.pushConstants( m_pipeline->get_layout(),
                                       vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                       0,
                                       sizeof(compose_pipeline_push_consts),
                                       &pc);

        i++;


        m_commandbuffer.draw(6,1,0,0);

    }
};


/**
 * @brief The App struct
 *
 * This is the main structure of our app. We will inherit from VulkanApp which
 * performs some low level stuff we do not need to worry to much about. It
 * performs things like creating the window, providing callback functions
 * for mouse/keyboard/window events.
 *
 * It also intializes the Screen object to draw to
 */
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

      // convert the meshs into gpu meshes so that they can
      // be rendered. convert_to_gpu_mesh returns a shared pointer to
      // the mesh object. But this mesh object is also stored within the
      // mesh manager. We can always get the reference by identifying it
      // with the name provided: "box", "sphere"
      auto box = vka::box_mesh_host(1,1,1);
      //auto sphere = vka::sphere_mesh_host(0.5,20,20);

      auto sphere = vka::plane_mesh_host(2,2);
      host_to_gpu_mesh("box",    box);
      host_to_gpu_mesh("sphere", sphere);
  }

  /**
   * @brief convert_to_gpu_mesh
   * @param name
   * @param M
   * @return
   *
   * This function takes a host Mesh and copies it to the GPU via the Mesh Manager.
   *
   * It returns a reference to the mesh which can be used to draw.
   */
  std::shared_ptr<vka::mesh> host_to_gpu_mesh( const std::string & name, vka::host_mesh & M)
  {
        // Create a new mesh from the mesh_manager
        auto m = m_mesh_manager.new_mesh(name);

        // Get an attribute view for the Position/UV/Normals
        auto & P = M.get_attribute( vka::VertexAttribute::ePosition);
        auto & U = M.get_attribute( vka::VertexAttribute::eUV    );
        auto & N = M.get_attribute( vka::VertexAttribute::eNormal);
        auto & I = M.get_attribute( vka::VertexAttribute::eIndex);


        assert( P.count() == U.count() );
        assert( P.count() == N.count() );

        // Set the total number of vertices/indices
        m->set_num_vertices( P.count() );
        m->set_num_indices( I.count(), I.attribute_size()==2 ?sizeof(uint16_t) : sizeof(uint32_t) );

        // Set the type of attributes we want for the gpu_mesh
        m->set_attribute(0, P.attribute_size() ); // position
        m->set_attribute(1, U.attribute_size() ); // UV
        m->set_attribute(2, P.attribute_size() ); // Normals

        // We can now allocate the data for the mesh on the gpu
        m->allocate();

        // and copy the data
        m->copy_attribute_data(0, P.data(), P.data_size() );
        m->copy_attribute_data(1, U.data(), U.data_size() );
        m->copy_attribute_data(2, N.data(), N.data_size() );

        m->copy_index_data( I.data(), I.data_size() );

        return m;
  }

  /**
   * @brief convert_to_gpu_mesh
   * @param name
   * @param M
   * @return
   *
   * This function takes a host Mesh and copies it to the GPU via the Mesh Manager.
   *
   * It returns a reference to the mesh which can be used to draw.
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
      m_dsets.texture_array = m_pipelines.gbuffer->create_new_descriptor_set(0, m_descriptor_pool);
      //  attach our texture to binding 0 in the set.
      m_dsets.texture_array->attach_sampler(0, m_texture_array);
      m_dsets.texture_array->update();

      m_dsets.uniform_buffer = m_pipelines.gbuffer->create_new_descriptor_set(1, m_descriptor_pool);
      m_dsets.uniform_buffer->attach_uniform_buffer(0, m_ubuffer, sizeof(per_frame_uniform_t), m_ubuffer->offset());
      m_dsets.uniform_buffer->update();

  }

  /**
   * @brief init_render_targets
   *
   * In this function we will initialize the offscreen render target.
   *
   * The render target will consist of multiple images which can be drawn to
   *
   */
  void init_render_targets()
  {
      m_OffscreenTarget = m_Context.new_offscreen_target("offscreen_target");

      // we're going to watn to have 3 colour attachments and 1 depth
      // attachment. This will give us 4 images we can draw to.
      // we are going to use the first one for position values
      //                        second one for normal values
      //                         third one for albedo (color)
      //                        fourth one for depth information
      m_OffscreenTarget->add_color_attachment( vk::Extent2D(WIDTH,HEIGHT), vk::Format::eR32G32B32A32Sfloat);
      m_OffscreenTarget->add_color_attachment( vk::Extent2D(WIDTH,HEIGHT), vk::Format::eR32G32B32A32Sfloat);
      m_OffscreenTarget->add_color_attachment( vk::Extent2D(WIDTH,HEIGHT), vk::Format::eR8G8B8A8Unorm);
      m_OffscreenTarget->add_depth_attachment( vk::Extent2D(WIDTH,HEIGHT), vk::Format::eR8G8B8A8Unorm); // NOTE the format isn't used here.  need to fix this.

      // Set the dimensions of the offscreen target.
      m_OffscreenTarget->set_extents( vk::Extent2D(WIDTH,HEIGHT));

      // Finally create the target in GPU memory.
      m_OffscreenTarget->create();

      //======================================================================
      // Now we will generate a new pipeline which will draw a fullscreen
      // quad on the screen. It will take in the 4 texture images we
      // created in the rendertarget and compose them into a single image
      // which can be displayed to the screen
      //======================================================================
      m_pipelines.compose = m_Context.new_pipeline("compose_pipeline");
      m_pipelines.compose->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
              ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

              ->set_vertex_shader(   "resources/shaders/compose_rendertargets/compose_rendertargets_v.spv", "main" )   // the shaders we want to use
              ->set_fragment_shader( "resources/shaders/compose_rendertargets/compose_rendertargets_f.spv", "main" ) // the shaders we want to use

              ->set_toplogy(vk::PrimitiveTopology::eTriangleList)
              ->set_line_width(1.0f)
              // Triangle vertices are drawn in a counter clockwise manner
              // using the right hand rule which indicates which face is the
              // front
              ->set_front_face(vk::FrontFace::eCounterClockwise)

              // Here are the 4 textures we are going to need
              // See the fragment shader code for more information.
              ->add_texture_layout_binding(0, 0, vk::ShaderStageFlagBits::eFragment)
              ->add_texture_layout_binding(0, 1, vk::ShaderStageFlagBits::eFragment)
              ->add_texture_layout_binding(0, 2, vk::ShaderStageFlagBits::eFragment)
              ->add_texture_layout_binding(0, 3, vk::ShaderStageFlagBits::eFragment)

              // Cull all back facing triangles.
              ->set_cull_mode(vk::CullModeFlagBits::eNone)
              // Add a push constant to the layout. It is accessable in the vertex shader
              // stage only.
              ->add_push_constant( sizeof(compose_pipeline_push_consts), 0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
              // Since we are drawing this to the screen, we need the screen's
              // renderpass.
              ->set_render_pass( m_screen->get_renderpass() )
              ->create();
      //======================================================================


      //======================================================================
      // Each attachment in the render target is an image. Once we draw to the
      // image, we can essentially read from it like it was any other texture.
      // Now we are goign to take those images and create a descriptor set for
      // it.

      // Create a new descriptor set based on the descriptor information we
      // gave to the Compose pipeline
      m_dsets.renderTargets = m_pipelines.compose->create_new_descriptor_set(0, m_descriptor_pool);

      m_dsets.renderTargets->attach_sampler(0, m_OffscreenTarget->get_image(0) );
      m_dsets.renderTargets->attach_sampler(1, m_OffscreenTarget->get_image(1) );
      m_dsets.renderTargets->attach_sampler(2, m_OffscreenTarget->get_image(2) );
      m_dsets.renderTargets->attach_sampler(3, m_OffscreenTarget->get_image(3) );
      m_dsets.renderTargets->update();
      //======================================================================



      // We are now going to create the graphics pipeline to be able to
      // draw our geometry to the render target instead of the screen.
      // This pipeline is very similar to the main pipeline in example 07.
      auto M = vka::box_mesh_OLD(1,1,1);

      m_pipelines.gbuffer = m_Context.new_pipeline("gbuffer_pipeline");
      m_pipelines.gbuffer->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
              ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

              ->set_vertex_shader(   "resources/shaders/gbuffer/gbuffer_v.spv", "main" )   // the shaders we want to use
              ->set_fragment_shader( "resources/shaders/gbuffer/gbuffer_f.spv", "main" ) // the shaders we want to use

              // tell the pipeline that attribute 0 contains 3 floats
              // and the data starts at offset 0
              ->set_vertex_attribute(0, 0 ,  0 , M.format( vka::VertexAttribute::ePosition) , sizeof(glm::vec3) )
              // tell the pipeline that attribute 1 contains 3 floats
              // and the data starts at offset 12
              ->set_vertex_attribute(1, 1 ,  0       , M.format( vka::VertexAttribute::eUV) , sizeof(glm::vec2) )

              ->set_vertex_attribute(2, 2 ,  0   , M.format( vka::VertexAttribute::eNormal) , sizeof(glm::vec3) )

              //===============================================================
              // Here we are going to set the number of color attachments.
              // We created 3 color attachments for our render target.
              //===============================================================
              ->set_color_attachments( 3 )
              //===============================================================

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
              //===============================================================
              // Since we are no longer drawing to the main screen. we need
              // to set the render pass to the OffscreenTarget
              ->set_render_pass( m_OffscreenTarget->get_renderpass() );
              //===============================================================

        //======================================================================
        // Tell the gbuffer pipeline to disable blending for the colours
        m_pipelines.gbuffer->get_color_blend_attachment_state(0).blendEnable=VK_FALSE;
        m_pipelines.gbuffer->get_color_blend_attachment_state(1).blendEnable=VK_FALSE;
        m_pipelines.gbuffer->get_color_blend_attachment_state(2).blendEnable=VK_FALSE;

        m_pipelines.gbuffer->create();

  }


  /**
   * @brief onInit
   *
   * This is called as soon as the VulkanApp starts. We will
   * use this to initalize all the data we need
   */
  virtual void onInit()
  {
      // create all the pools required
      init_pools();

      // initalize all the memory we need
      init_memory();

      // load 3d meshs
      load_meshs();

      // load textures
      load_textures();



      init_render_targets();

      // create the descriptor sets
      init_descriptor_sets();


      m_image_available_semaphore      = m_Context.new_semaphore("image_available_semaphore");
      m_offscreen_complete_semaphore   = m_Context.new_semaphore("offscreen_complete_semaphore");
      m_composition_complete_semaphore = m_Context.new_semaphore("composition_complete_semaphore");

      // Create the command buffer we are going to use to draw to the
      // offscreen render target
      m_offscreen_cmd_buffer = m_command_pool->AllocateCommandBuffer();

      // Create the command buffer we are goign to use to draw to the
      // Screen
      m_compose_cmd_buffer       = m_command_pool->AllocateCommandBuffer();

      // Initalize the scene we are going to render
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



  void calculate(double T)
  {
      m_Camera.calculate();

      m_frame_uniform.view = m_Camera.get_view_matrix();
      m_frame_uniform.proj = m_Camera.get_proj_matrix();
      m_frame_uniform.proj[1][1] *= -1;

     for(RenderComponent_t * m : m_Objs)
     {
        auto th = T * m->m_orbital_speed * 2.0*3.14159 + m->m_orbital_phase;

        auto p = glm::vec3( m->m_orbital_radius * cos( th ), 0.0, m->m_orbital_radius * sin( th ) );

        m->m_push.model = vka::transform( p ).get_matrix();
     }
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
      //m_Camera.calculate();
      calculate(T);
      //========================================================================




      // RESET THE COMMAND BUFFER
      m_offscreen_cmd_buffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);

      // BEGIN RECORDING TO THE COMMAND BUFFER
      m_offscreen_cmd_buffer.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse) );
      {
          //========================================================================
          // 2. Update the uniforms by copying the data from the CPU to the GPU
          //    Copying buffer data must be done before calling any render commands.
          UpdateUniforms(m_offscreen_cmd_buffer);
          //========================================================================

          //========================================================================
          // 3. Render all the objects to the Offscreen Render Target

          m_OffscreenTarget->clear_value(0).color = vk::ClearColorValue( std::array<float,4>({0.0f, 0.0f, 0.0f, 0.0f}));
          m_OffscreenTarget->clear_value(1).color = vk::ClearColorValue( std::array<float,4>({0.0f, 0.0f, 0.0f, 0.0f}));
          m_OffscreenTarget->clear_value(2).color = vk::ClearColorValue( std::array<float,4>({0.0f, 0.0f, 0.0f, 0.0f }));

          m_OffscreenTarget->beginRender( m_offscreen_cmd_buffer );
          {
              // Main component renderer
              ComponentRenderer_t R;
              for(auto * comp : m_Objs)
              {
                  R(m_offscreen_cmd_buffer, comp);
              }
          }
          m_OffscreenTarget->endRender( m_offscreen_cmd_buffer );
      }
      m_offscreen_cmd_buffer.end();


      //========================================================================
      //========================================================================
      //========================================================================
      //========================================================================

      // Now we are going to build the command buffer to render to the screen
      // We are going to draw 4 quads onto the screen, each quad will
      // use a different color component from the previous rendering stage
      // as a texture so we can see what image is drawn to each of the components.


      // Prepare the next frame for rendering and get the frame_index
      // that we will be drawing to.
      //    - The semaphore we pass into it will be signaled when the frame is
      //      ready
      uint32_t frame_index = m_screen->prepare_next_frame(m_image_available_semaphore);

      m_compose_cmd_buffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
      m_compose_cmd_buffer.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse) );
      {
          // start the actual rendering
          m_screen->beginRender(m_compose_cmd_buffer, frame_index);
          {
              FullScreenQuadRenderer_t Q(m_compose_cmd_buffer, m_pipelines.compose);

              compose_pipeline_push_consts pc;
              pc.layer = 0;

              pc.size     = glm::vec2(1,1);

              pc.layer = 0; pc.position = glm::vec2(-1,-1);
              Q( m_dsets.renderTargets, pc);

              pc.layer = 1; pc.position = glm::vec2( 0,0);
              Q( m_dsets.renderTargets, pc);

              pc.layer = 2; pc.position = glm::vec2( 0,-1);
              Q( m_dsets.renderTargets, pc);

              pc.layer = 3; pc.position = glm::vec2( -1,0);
              Q( m_dsets.renderTargets, pc);
          }
          m_screen->endRender(m_compose_cmd_buffer);
      }
      m_compose_cmd_buffer.end();

      //========================================================================





      //========================================================================
      // Now submit the two command buffers. Because we ahve two targets
      // to render to (the offscreen and the main screen), we need to make sure
      // that our resources are synced properly
      //========================================================================


      //=============== Offscreen Rendering =====================================
      // submit the offscreen buffer, but wait for the "image_available_semaphore"
      // once the submittion is complete, trigger the "offscreen_complete_semaphore"
      m_Context.submit_command_buffer(m_offscreen_cmd_buffer,          // submit this command buffer
                                      m_image_available_semaphore,     // // Wait for this semaphore before we start writing
                                      m_offscreen_complete_semaphore); // Signal this semaphore when the commands have been executed




      // Submit the the command buffer to the GPU.
      // A this point all the geometry will be rendered to the screen's image
      m_Context.submit_command_buffer(m_compose_cmd_buffer,        // Execute this command buffer
                                      m_offscreen_complete_semaphore, // but wait until this semaphore has been signaled
                                      m_composition_complete_semaphore);// signal this semaphore when it is done




     //=============== Final Presentation =======================================
     // Present the final image to the screen. But wait for the
     // compose command buffer to finish executing all the commands.
     m_screen->present_frame( frame_index, m_composition_complete_semaphore );

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

      std::string meshs[] = {"box", "sphere"};
      for(int i=0; i<MAX_OBJ ;i++)
      {
        auto * obj = new RenderComponent_t();


        obj->m_mesh_m   = m_mesh_manager.get_mesh( meshs[i%2] );

        obj->m_pipeline = m_pipelines.gbuffer;

        obj->m_descriptor_sets[0] = m_dsets.texture_array;
        obj->m_descriptor_sets[1] = m_dsets.uniform_buffer;


        obj->m_orbital_radius = std::rand() % 20+1;
        obj->m_orbital_speed  = 1.0 / ( obj->m_orbital_radius );
        obj->m_orbital_phase = ((std::rand()%10000) / 10000.0) * 2.0*3.141592653589;


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

  // This is the offscreen target. It can be used to
  // render to just like the screen.
  vka::offscreen_target * m_OffscreenTarget;


  //=====================================
  vka::descriptor_pool *m_descriptor_pool;
  vka::command_pool    *m_command_pool;

  vka::mesh_manager     m_mesh_manager;

  vka::buffer_pool     *m_buffer_pool;
  vka::sub_buffer      *m_ubuffer;
  vka::buffer          *m_sbuffer;

  vka::texture2darray  *m_texture_array;

  //vka::semaphore       *m_render_complete_semaphore;

  vka::semaphore * m_image_available_semaphore;
  vka::semaphore * m_offscreen_complete_semaphore;
  vka::semaphore * m_composition_complete_semaphore;
  //========================================
  vka::camera         m_Camera;
  per_frame_uniform_t m_frame_uniform;

  struct
  {
    vka::descriptor_set * texture_array;
    vka::descriptor_set * uniform_buffer;

    vka::descriptor_set * renderTargets;
  } m_dsets;

  struct
  {    
    vka::pipeline * gbuffer;
    vka::pipeline * compose;
  } m_pipelines;


  vka::command_buffer m_offscreen_cmd_buffer;
  vka::command_buffer m_compose_cmd_buffer;

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

