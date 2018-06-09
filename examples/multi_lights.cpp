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

#include <vka/core/offscreen_target.h>

#include <vka/utils/glfw_window_handler.h>

#include "vulkan_app.h"

#include <vka/eng/mesh_manager.h>

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


struct light_data_t
{
    glm::vec4 position    = glm::vec4(0,2,0,1); // position.xyz, type [0 - omni, 1 - spot, 2 - directional
                                                // if position.a == 2, then position.xyz -> direction.xyz
    glm::vec4 color       = glm::vec4(10,1,1,1);
    glm::vec4 attenuation = glm::vec4(1, 5.8 ,0.0, 10); //[constant, linear, quad, cutoff]
};

struct light_uniform_t
{
    glm::vec2    num_lights = glm::vec2(5,0);
    glm::vec2    num_lights2 = glm::vec2(10,0);
    light_data_t lights[10];
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


class PhysicsComponent_t
{
    vka::transform m_transform;
};

class RenderComponent_t
{
public:
    vka::pipeline   *m_pipeline = nullptr;

    std::map<uint32_t, vka::descriptor_set*> m_descriptor_sets;

    push_constants_t         m_push;

    std::shared_ptr<vka::mesh> m_mesh_m;

    bool m_draw_axis = true;
};


class FullScreenQuadRenderer_t
{
public:
    vka::pipeline     * m_pipeline;
    vka::command_buffer m_commandbuffer;

    FullScreenQuadRenderer_t(vka::command_buffer & cb ,
                             vka::pipeline * pipeline ,
                             vka::descriptor_set * texture_sets,
                             vka::descriptor_set * uniform_sets) : m_pipeline(pipeline) , m_commandbuffer(cb)
    {
        m_commandbuffer.bindPipeline( vk::PipelineBindPoint::eGraphics, *m_pipeline );
        m_commandbuffer.bindDescriptorSet(vk::PipelineBindPoint::eGraphics,
                             m_pipeline,
                             0, // binding index
                             texture_sets);
        m_commandbuffer.bindDescriptorSet(vk::PipelineBindPoint::eGraphics,
                             m_pipeline,
                             1, // binding index
                             uniform_sets);
    }

    void operator()( vka::descriptor_set * dset)
    {
        static uint32_t i=1;
        uint32_t index = -1;//(i/500)%3;

        m_commandbuffer.pushConstants( m_pipeline->get_layout(),
                                       vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                                       0,
                                       sizeof(uint32_t),
                                       &index);

        i++;


        m_commandbuffer.draw(6,1,0,0);

    }
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

class ComponentRenderer_t
{
    vka::pipeline  *m_pipeline = nullptr;

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



struct Entity_t
{
    PhysicsComponent_t *m_physics = nullptr;
    RenderComponent_t  *m_render  = nullptr;
};





struct App : public VulkanApp
{

  void init_pools()
  {
      //==========================================================================
      // Initialize the Command and Descriptor Pools
      //==========================================================================
      m_descriptor_pool = m_Context.new_descriptor_pool("main_desc_pool");
      m_descriptor_pool->set_pool_size(vk::DescriptorType::eCombinedImageSampler, 25);
      m_descriptor_pool->set_pool_size(vk::DescriptorType::eUniformBuffer, 5);
      m_descriptor_pool->set_pool_size(vk::DescriptorType::eUniformBufferDynamic, 1);
      m_descriptor_pool->create();

      m_command_pool = m_Context.new_command_pool("main_command_pool");
      //==========================================================================
  }


  void init_memory()
  {
      m_buffer_pool = m_Context.new_buffer_pool("main_buffer_pool");
      m_buffer_pool->set_size( 50*1024*1024 ); // create 50Mb buffer
      m_buffer_pool->create();

      // Allocate sub buffers from the buffer pool. These buffers can be
      // used for indices/vertices or uniforms.
      m_dbuffer = m_buffer_pool->new_buffer( 10*1024*1024 );

      m_ubuffer_lights = m_buffer_pool->new_buffer( sizeof(light_uniform_t), 256 );
      m_ubuffer        = m_buffer_pool->new_buffer( 10*1024*1024 ,256);

      m_sbuffer = m_Context.new_staging_buffer( "staging_buffer", 10*1024*1024);

      m_texture_array = m_Context.new_texture2darray("main_texture_array");
      m_texture_array->set_size( 512 , 512 );
      m_texture_array->set_format(vk::Format::eR8G8B8A8Unorm);
      m_texture_array->set_mipmap_levels(8);
      m_texture_array->set_layers(10);
      m_texture_array->create();
      m_texture_array->create_image_view(vk::ImageAspectFlagBits::eColor);
  }

  void init_pipelines()
  {
      // Create the graphics Pipeline
      auto M = vka::box_mesh(1,1,1);

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


              ->set_color_attachments( 3 )

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
              ->set_render_pass( m_OffscreenTarget->get_renderpass() );

        m_pipelines.gbuffer->get_color_blend_attachment_state(0).blendEnable=VK_FALSE;
        m_pipelines.gbuffer->get_color_blend_attachment_state(1).blendEnable=VK_FALSE;
        m_pipelines.gbuffer->get_color_blend_attachment_state(2).blendEnable=VK_FALSE;

        m_pipelines.gbuffer->create();

        m_pipelines.main = m_Context.new_pipeline("main_pipeline");
        m_pipelines.main->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                ->set_vertex_shader(   "resources/shaders/deferred_basic/deferred_basic_v.spv", "main" )   // the shaders we want to use
                ->set_fragment_shader( "resources/shaders/deferred_basic/deferred_basic_f.spv", "main" ) // the shaders we want to use

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
                ->set_line_width(5.0f)
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

        //======================================================================
        // The compose pipeline generates a full screen quad (in the shader)
        // and accepts a number of textures. It uses thse textures and combines
        // to produce an image on the screen.
        //======================================================================
        m_pipelines.compose = m_Context.new_pipeline("compose_pipeline");
        m_pipelines.compose->set_viewport( vk::Viewport( 0, 0, WIDTH, HEIGHT, 0, 1) )
                ->set_scissor( vk::Rect2D(vk::Offset2D(0,0), vk::Extent2D( WIDTH, HEIGHT ) ) )

                ->set_vertex_shader(   "resources/shaders/compose/compose_v.spv", "main" )   // the shaders we want to use
                ->set_fragment_shader( "resources/shaders/compose/compose_f.spv", "main" ) // the shaders we want to use

                ->set_toplogy(vk::PrimitiveTopology::eTriangleList)
                ->set_line_width(1.0f)
                // Triangle vertices are drawn in a counter clockwise manner
                // using the right hand rule which indicates which face is the
                // front
                ->set_front_face(vk::FrontFace::eCounterClockwise)

                ->add_texture_layout_binding(0, 0, vk::ShaderStageFlagBits::eFragment)
                ->add_texture_layout_binding(0, 1, vk::ShaderStageFlagBits::eFragment)
                ->add_texture_layout_binding(0, 2, vk::ShaderStageFlagBits::eFragment)
                ->add_texture_layout_binding(0, 3, vk::ShaderStageFlagBits::eFragment)

                // Cull all back facing triangles.
                ->set_cull_mode(vk::CullModeFlagBits::eNone)
                // Add a push constant to the layout. It is accessable in the vertex shader
                // stage only.
                ->add_push_constant( sizeof(int), 0, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)

                ->add_uniform_layout_binding(1, 0, vk::ShaderStageFlagBits::eFragment)
                //
                ->set_render_pass( m_screen->get_renderpass() )
                ->create();
        //======================================================================
  }


  std::pair<glm::vec3, glm::vec3>
  calculate_tangent_bitangent( glm::vec3 const & p0,  glm::vec3 const & p1,  glm::vec3 const & p2,
          glm::vec2 const & uv0, glm::vec2 const & uv1, glm::vec2 const & uv2
          )
  {

      auto deltaUV1 = uv1-uv0;
      auto deltaUV2 = uv2-uv0;

      auto edge1 = p1-p0;
      auto edge2 = p2-p0;

      float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

      std::pair<glm::vec3, glm::vec3> tbt;

      tbt.first.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
      tbt.first.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
      tbt.first.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
      tbt.first = glm::normalize(tbt.first);

      tbt.second.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
      tbt.second.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
      tbt.second.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
      tbt.second = glm::normalize(tbt.second);

      return tbt;
  }

  void load_meshs()
  {
      m_mesh_manager.set_context(&m_Context);
      m_mesh_manager.set_buffer_size( 1024*1024*20 );

      std::shared_ptr<vka::mesh> MM[] = {
                         m_mesh_manager.new_mesh("box"),
                         m_mesh_manager.new_mesh("sphere") ,
                         m_mesh_manager.new_mesh("plane")};


      //box_mesh->allocate_vertex_data()
      std::vector<vka::mesh_t>   meshs;
      meshs.push_back( vka::box_mesh(1,1,1) );
      meshs.push_back( vka::sphere_mesh(0.5,20,20) );
      meshs.push_back( vka::plane_mesh(10,10) );

      uint32_t i=0;
      for( auto & M : meshs)
      {

          auto P = M.get_attribute_view<glm::vec3>( vka::VertexAttribute::ePosition);
          auto U = M.get_attribute_view<glm::vec2>( vka::VertexAttribute::eUV    );
          auto N = M.get_attribute_view<glm::vec3>( vka::VertexAttribute::eNormal);
          auto I = M.get_index_view<uint16_t>();


          std::vector< glm::vec3> p;
          std::vector< glm::vec3> n;
          std::vector< glm::vec2> u;
          std::vector< glm::vec3> t;
          std::vector< glm::vec3> b;

          for(uint32_t i=0; i<I.size(); i+=3)
          {
              auto tbt = calculate_tangent_bitangent( P[ I[i] ] , P[ I[i+1] ], P[ I[i+2] ],
                                                      U[ I[i] ] , U[ I[i+1] ], U[ I[i+2] ]);

              t.push_back(tbt.first);
              b.push_back(tbt.second);
          }

          for(uint32_t i=0;i<P.size();i++)
          {
                p.push_back( P[i] );
                n.push_back( N[i] );
                u.push_back( U[i] );
          }

         MM[i]->set_num_vertices( M.num_vertices() );
         MM[i]->set_num_indices( M.num_indices(), sizeof(uint16_t));
         MM[i]->set_attribute(0, sizeof(glm::vec3) );
         MM[i]->set_attribute(1, sizeof(glm::vec2) );
         MM[i]->set_attribute(2, sizeof(glm::vec3) );
         MM[i]->allocate();
         MM[i]->copy_attribute_data(0, p.data(), p.size() * sizeof(glm::vec3));
         MM[i]->copy_attribute_data(1, u.data(), u.size() * sizeof(glm::vec2));
         MM[i]->copy_attribute_data(2, n.data(), n.size() * sizeof(glm::vec3));
         MM[i]->copy_index_data( M.index_data(), M.index_data_size() );

          i++;
      }
  }

  void load_textures()
  {
      // 1. First load host_image into memory, and specifcy we want 4 channels.
          std::vector<std::string> image_paths = {
              "resources/textures/Brick-2852a.jpg",
              "resources/textures/noise.jpg",
              "resources/textures/normal.jpg"
          };

          uint32_t layer = 0;
          for(auto & img : image_paths)
          {
              vka::host_image D(img, 4);
              assert( D.width() == 512 && D.height()==512);
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


      m_dsets.light_uniform_buffer = m_pipelines.compose->create_new_descriptor_set(1, m_descriptor_pool);
      m_dsets.light_uniform_buffer->attach_uniform_buffer(0, m_ubuffer_lights, sizeof(light_uniform_t), m_ubuffer_lights->offset());
      m_dsets.light_uniform_buffer->update();

      m_dsets.renderTargets = m_pipelines.compose->create_new_descriptor_set(0, m_descriptor_pool);
      m_dsets.renderTargets->attach_sampler(0, m_OffscreenTarget->get_image(0) );
      m_dsets.renderTargets->attach_sampler(1, m_OffscreenTarget->get_image(1) );
      m_dsets.renderTargets->attach_sampler(2, m_OffscreenTarget->get_image(2) );
      m_dsets.renderTargets->attach_sampler(3, m_OffscreenTarget->get_image(3) );
      m_dsets.renderTargets->update();



      // m_dsets.dynamic_uniform_buffer = m_pipelines.main->create_new_descriptor_set(2, m_descriptor_pool);
      // m_dsets.dynamic_uniform_buffer->attach_dynamic_uniform_buffer(0, m_dbuffer, sizeof(dynamic_uniform_buffer_t), m_dbuffer->offset());
      // m_dsets.dynamic_uniform_buffer->update();

  }
  virtual void onInit()
  {
      init_pools();
      init_memory();

      init_render_targets();


      load_meshs();
      load_textures();

      init_pipelines();
      init_descriptor_sets();


      m_image_available_semaphore      = m_Context.new_semaphore("image_available_semaphore");
      m_composition_complete_semaphore = m_Context.new_semaphore("render_complete_semaphore");
      m_offscreen_complete_semaphore   = m_Context.new_semaphore("offscreen_complete_semaphore");

      m_compose_buffer   = m_command_pool->AllocateCommandBuffer();
      m_offscreen_buffer = m_command_pool->AllocateCommandBuffer();

      init_scene();

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
        //std::cout << "Moving: " << dx << ", " << dy << std::endl;
      };

  }




  void UpdateUniforms(vka::command_buffer & cb)
  {
    auto S = static_cast<unsigned char*>(m_sbuffer->map_memory());

    memcpy( &S[0]                            , &m_frame_uniform, sizeof(per_frame_uniform_t) );
    memcpy( &S[0+sizeof(per_frame_uniform_t)], &m_light_uniform, sizeof(light_uniform_t)     );

    cb.copySubBuffer( *m_sbuffer, m_ubuffer       , vk::BufferCopy(0, 0 ,sizeof(per_frame_uniform_t))  );
    cb.copySubBuffer( *m_sbuffer, m_ubuffer_lights, vk::BufferCopy(sizeof(per_frame_uniform_t), 0 ,sizeof(light_uniform_t))  );
  }


  void build_composition_command_buffer(vka::command_buffer & cb )
  {

      m_screen->beginRender(cb);
         FullScreenQuadRenderer_t Q(cb, m_pipelines.compose, m_dsets.renderTargets, m_dsets.light_uniform_buffer);
         Q( m_dsets.renderTargets);
      m_screen->endRender(cb);
  }

  virtual void onFrame(double dt, double T)
  {
      m_Camera.calculate();

      m_frame_uniform.view = m_Camera.get_view_matrix();
      m_frame_uniform.proj = m_Camera.get_proj_matrix();
      m_frame_uniform.proj[1][1] *= -1;

      //============== Render Section     =====================================
      m_offscreen_buffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
      m_offscreen_buffer.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse) );
          // update uniforms
          UpdateUniforms(m_offscreen_buffer);

          // Render
          build_offscreen_commandbuffer(m_offscreen_buffer);

      m_offscreen_buffer.end();


      //=================

      draw();
  }


  void draw()
  {
     m_screen->prepare_next_frame(m_image_available_semaphore);

     //=============== Offscreen Rendering =====================================
     // submit the offscreen buffer, but wait for the "image_available_semaphore"
     // once the submittion is complete, trigger the "offscreen_complete_semaphore"
     m_Context.submit_command_buffer(m_offscreen_buffer, m_image_available_semaphore, m_offscreen_complete_semaphore);


    //=============== Composition Rendering ====================================
     // Draw the composition image. Since this is the final stage, we must wait
    // until the swapchain image is available. Once we submit, trigger the "present_to_screen_complete"

    //============== Composition Section =====================================
    // Get the index of the next available image in the swapchain. Once the
    // and trigger the image_avialble_semaphore


    m_compose_buffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
    m_compose_buffer.begin( vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse) );

    build_composition_command_buffer(m_compose_buffer);

    m_compose_buffer.end();

    m_Context.submit_command_buffer(m_compose_buffer, m_offscreen_complete_semaphore, m_composition_complete_semaphore);

    // Would like to do this eventually:
    // m_Context.submit_command_buffer(m_compose_buffer).waitfor(m_image_available_semaphore)
    //                                                  .signal( m_present_to_screen_complete_semaphore);

    //=============== Final Presentation =======================================
    // Present the final image to the screen. But wait on for the semaphore to be flagged first.
    m_screen->present_frame( m_composition_complete_semaphore );
  }


  void init_scene()
  {

#define S (2.0*3.141592653/10.0)

      ANIMATE(m_light_uniform.lights[0].position, glm::vec4( 4*cos(t+0*S)   , 1.2,  4*sin(t + 0*S),0));
      ANIMATE(m_light_uniform.lights[1].position, glm::vec4( 4*cos(t+1*S)   , 1.2,  4*sin(t + 1*S),0));
      ANIMATE(m_light_uniform.lights[2].position, glm::vec4( 4*cos(t+2*S)   , 1.2,  4*sin(t + 2*S),0));
      ANIMATE(m_light_uniform.lights[3].position, glm::vec4( 4*cos(t+3*S)   , 1.2,  4*sin(t + 3*S),0));
      ANIMATE(m_light_uniform.lights[4].position, glm::vec4( 4*cos(t+4*S)   , 1.2,  4*sin(t + 4*S),0));
      ANIMATE(m_light_uniform.lights[5].position, glm::vec4( 4*cos(t+5*S)   , 1.2,  4*sin(t + 5*S),0));
      ANIMATE(m_light_uniform.lights[6].position, glm::vec4( 4*cos(t+6*S)   , 1.2,  4*sin(t + 6*S),0));
      ANIMATE(m_light_uniform.lights[7].position, glm::vec4( 4*cos(t+7*S)   , 1.2,  4*sin(t + 7*S),0));
      ANIMATE(m_light_uniform.lights[8].position, glm::vec4( 4*cos(t+8*S)   , 1.2,  4*sin(t + 8*S),0));
      ANIMATE(m_light_uniform.lights[9].position, glm::vec4( 4*cos(t+9*S)   , 1.2,  4*sin(t + 9*S),0));

      m_light_uniform.lights[0].position    = glm::vec4(-2,2.2,-2,0);
      m_light_uniform.lights[0].color       = glm::vec4(10.0,0.0,0.1,0);
    //  m_light_uniform.lights[0].attenuation = glm::vec4(1, 0.8 ,0.0, 10);

      m_light_uniform.lights[1].position    = glm::vec4(2,2.2,2,0);
      m_light_uniform.lights[1].color       = glm::vec4(0.0,10.0,0.0,1.0);
   //   m_light_uniform.lights[1].attenuation = glm::vec4(1, 0.0 ,0.8, 10);

      m_light_uniform.lights[2].position    = glm::vec4(2,2.2,2,0);
      m_light_uniform.lights[2].color       = glm::vec4(0.0,0.0,10.0,1.0);
    //  m_light_uniform.lights[2].attenuation = glm::vec4(1, 0.0 ,0.8, 10);
       //m_light_uniform.lights[1].position    = glm::vec3(-5,5.1,-5);
      // m_light_uniform.lights[1].attenuation = glm::vec4(1, 0.001 ,0.001, 10);
      #define MAX_OBJ 1

      float x = 0;
      float y = 0;
      float z = 0;

      std::string mesh[] = {"sphere", "box"};

      for(int i=0; i < MAX_OBJ ; i++)
      {
        auto * obj = new RenderComponent_t();

        obj->m_mesh_m   = m_mesh_manager.get_mesh( "plane");
        obj->m_pipeline = m_pipelines.gbuffer;

        obj->m_descriptor_sets[0] = m_dsets.texture_array;
        obj->m_descriptor_sets[1] = m_dsets.uniform_buffer;

        //obj->m_uniform_buffer = m_dsets.uniform_buffer;
        //obj->m_dynamic_buffer = m_dsets.dynamic_uniform_buffer;
        //obj->m_texture        = m_dsets.texture_array;


        vka::transform T;
        T.set_position( glm::vec3(0,0,0));
        T.set_scale( glm::vec3(1,1,1));


        obj->m_push.model    = vka::transform( 1.25f*glm::vec3( x , y, z)   ).get_matrix(); // T.get_matrix();
        obj->m_push.index    = 0;
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



  /**
   * @brief build_offscreen_commandbuffer
   * @param cb
   *
   * Renders all the objects to the offscreen framebuffer
   */
  void build_offscreen_commandbuffer( vka::command_buffer & cb )
  {

      m_OffscreenTarget->clear_value(0).color = vk::ClearColorValue( std::array<float,4>({0.0f, 0.0f, 0.0f, 0.0f}));
      m_OffscreenTarget->clear_value(1).color = vk::ClearColorValue( std::array<float,4>({0.0f, 0.0f, 0.0f, 0.0f}));
      m_OffscreenTarget->clear_value(2).color = vk::ClearColorValue( std::array<float,4>({0.0f, 0.0f, 0.0f, 0.0f }));

      m_OffscreenTarget->beginRender(cb);

      // Main component renderer
      ComponentRenderer_t R;
      for(auto * comp : m_Objs)
      {
          R(cb, comp);
      }
      m_OffscreenTarget->endRender(cb);
  }


  void init_render_targets()
  {
      m_OffscreenTarget = m_Context.new_offscreen_target("offscreen_target");
      m_OffscreenTarget->add_color_attachment( vk::Extent2D(WIDTH,HEIGHT), vk::Format::eR32G32B32A32Sfloat);
      m_OffscreenTarget->add_color_attachment( vk::Extent2D(WIDTH,HEIGHT), vk::Format::eR32G32B32A32Sfloat);
      m_OffscreenTarget->add_color_attachment( vk::Extent2D(WIDTH,HEIGHT), vk::Format::eR8G8B8A8Unorm);
      m_OffscreenTarget->add_depth_attachment( vk::Extent2D(WIDTH,HEIGHT), vk::Format::eR8G8B8A8Unorm);
      m_OffscreenTarget->set_extents( vk::Extent2D(WIDTH,HEIGHT));
      m_OffscreenTarget->create();
  }

  //=====================================

  vka::descriptor_pool *m_descriptor_pool;
  vka::command_pool    *m_command_pool;

  vka::buffer_pool * m_buffer_pool;

  vka::sub_buffer* m_ubuffer_lights;
  vka::sub_buffer* m_ubuffer;
  vka::sub_buffer* m_dbuffer;
  vka::buffer    * m_sbuffer;

  vka::mesh_manager m_mesh_manager;

  vka::texture2darray * m_texture_array;

  vka::semaphore * m_image_available_semaphore;
  vka::semaphore * m_offscreen_complete_semaphore;
  vka::semaphore * m_composition_complete_semaphore;

  //========================================
          vka::camera m_Camera;
   per_frame_uniform_t m_frame_uniform;
   light_uniform_t     m_light_uniform;

  struct
  {
    vka::descriptor_set * texture_array;
    vka::descriptor_set * uniform_buffer;
    vka::descriptor_set * light_uniform_buffer;
    vka::descriptor_set * dynamic_uniform_buffer;

    vka::descriptor_set * renderTargets;
  } m_dsets;

  struct
  {
    vka::pipeline * gbuffer;
    vka::pipeline * main;
    vka::pipeline * axis;
    vka::pipeline * compose;
  } m_pipelines;


  vka::command_buffer m_offscreen_buffer;
  vka::command_buffer m_compose_buffer;


  vka::offscreen_target * m_OffscreenTarget;
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

