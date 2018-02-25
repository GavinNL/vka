#ifndef VKA_GLFW_WINDOW
#define VKA_GLFW_WINDOW

#include<functional>
#include <memory>
#include <vector>
#include <map>
#include <mutex>

#include <GLFW/glfw3.h>

namespace vka
{

class base_slot
{
    public:
        virtual ~base_slot() {}
};

/**
 * @brief The Signal class
 *
 * Signal class:  Attach multiple function objects
 * and call all function objects with a single call
 * to the () operator
 */
template<typename func_t>
class signal
{
    public:
        using id                = unsigned long;  //id type
        using function_t        = std::function<func_t>; // function object type

        using value_t           = std::pair<id,function_t>;

        class container_t
        {
        public:
            void remove(id ID)
            {
                if( ID == 0 ) return;

                std::lock_guard<std::mutex> L( m_mutex);


                m_container.erase(std::remove_if(m_container.begin(),
                                                 m_container.end(),
                                                 [ID](const value_t & x){ return x.first==ID;}),
                                  m_container.end());

            }

            public:
                std::vector<value_t>  m_container;
                std::mutex            m_mutex;
        };

        using container_p       = std::shared_ptr< container_t >;

        class slot : public base_slot
        {
        private:
            id           m_id;
            container_p  m_Container;

        public:
            slot() : m_id(0){}

            slot(id ID, container_p p ) : m_id(ID) , m_Container(p)
            {
            }

            slot(const slot & other) = delete;

            slot(slot && other)
            {
                m_id        = std::move( other.m_id );
                other.m_id  = 0;
                m_Container = std::move( other.m_Container);
            }


            virtual ~slot()
            {
                disconnect();
            }

            slot & operator=(const slot & other) = delete;

            slot & operator=(slot && other)
            {
                if( this != &other)
                {
                    m_id        = std::move( other.m_id );
                    m_Container = std::move( other.m_Container);
                    other.m_id = 0;
                }
                return *this;
            }

            /**
             * @brief Disconnect
             *
             * Disconnects the slot from the signal caller. This is automatically
             * called when the slot is destroyed.
             */
            virtual bool disconnect()
            {
                if( m_id == 0 ) return false;
                auto i = m_id;
                m_id = 0;
                m_Container->remove(i);
                return true;

            }

            void detach()
            {
                m_id = 0;
                m_Container.reset();
            }
        };


        signal() : m_signals( new container_t() )
        {

        }

        /**
         * @brief Connect
         * @param f - function object to connect
         * @param position - position in the list of signals, default is -1 (back of the list)
         * @return The slot ID used to disconnect the slot.
         *
         * Connects a function object to the signal
         */
        slot connect( function_t f , int position = -1)
        {
            static id ID = 1;
            std::lock_guard<std::mutex> L( get_container().m_mutex );

            auto & container = get_container().m_container;
            if(position<0)
                container.emplace_back( ID, f );
            else
                container.insert( container.begin()+position, value_t(ID,f) );

            return  slot(ID++, m_signals);

        }

        void disconnect(id SlotID)
        {
            std::lock_guard<std::mutex> L( get_container().m_mutex );
            auto it = get_container().begin();
            while(it != get_container().end() )
            {
                if( (*it).first == SlotID )
                {
                    std::cout << "Disconnecting slot: " << SlotID << std::endl;
                    it = get_container().erase(it);
                    return;
                }
                ++it;
            }
        }

        /**
         * @brief operator <<
         * @param f - function object to connect
         *
         * Same as the Connect( ) method.
         */
        slot operator << ( function_t f )
        {
            return connect(f);
        }

        template<typename ..._Funct>
        void operator()( _Funct &&... _Args )
        {
            std::lock_guard<std::mutex> L( m_signals->m_mutex );
            for(auto & f : get_container().m_container)
            {
                if( f.first != 0 )
                {
                    f.second( std::forward<_Funct>(_Args)...);
                }
            }
        }

        std::size_t NumSlots() const
        {
            return m_signals->m_container.size();
        }

        void clear()
        {
            get_container().m_container.clear();
        }

    protected:
        container_t&                 get_container() const { return *m_signals; }

        container_p                  m_signals;
      //  std::shared_ptr<std::mutex>  m_mutex;
};


enum class Button
{
      _1         =  GLFW_MOUSE_BUTTON_1     ,
      _2         =  GLFW_MOUSE_BUTTON_2     ,
      _3         =  GLFW_MOUSE_BUTTON_3     ,
      _4         =  GLFW_MOUSE_BUTTON_4     ,
      _5         =  GLFW_MOUSE_BUTTON_5     ,
      _6         =  GLFW_MOUSE_BUTTON_6     ,
      _7         =  GLFW_MOUSE_BUTTON_7     ,
      _8         =  GLFW_MOUSE_BUTTON_8     ,
      LAST      =  GLFW_MOUSE_BUTTON_LAST  ,
      LEFT      =  GLFW_MOUSE_BUTTON_LEFT  ,
      RIGHT     =  GLFW_MOUSE_BUTTON_RIGHT ,
      MIDDLE    =  GLFW_MOUSE_BUTTON_MIDDLE
};

enum class Key
{
    /* The unknown key */
    UNKNOWN            =GLFW_KEY_UNKNOWN        ,
    SPACE              =GLFW_KEY_SPACE          ,
    APOSTROPHE         =GLFW_KEY_APOSTROPHE     ,
    COMMA              =GLFW_KEY_COMMA          ,
    MINUS              =GLFW_KEY_MINUS          ,
    PERIOD             =GLFW_KEY_PERIOD         ,
    SLASH              =GLFW_KEY_SLASH          ,
    _0                  =GLFW_KEY_0             ,
    _1                  =GLFW_KEY_1             ,
    _2                  =GLFW_KEY_2             ,
    _3                  =GLFW_KEY_3             ,
    _4                  =GLFW_KEY_4             ,
    _5                  =GLFW_KEY_5             ,
    _6                  =GLFW_KEY_6             ,
    _7                  =GLFW_KEY_7             ,
    _8                  =GLFW_KEY_8             ,
    _9                  =GLFW_KEY_9             ,
    SEMICOLON          =GLFW_KEY_SEMICOLON      ,
    EQUAL              =GLFW_KEY_EQUAL          ,
    A                  =GLFW_KEY_A              ,
    B                  =GLFW_KEY_B              ,
    C                  =GLFW_KEY_C              ,
    D                  =GLFW_KEY_D              ,
    E                  =GLFW_KEY_E              ,
    F                  =GLFW_KEY_F              ,
    G                  =GLFW_KEY_G              ,
    H                  =GLFW_KEY_H              ,
    I                  =GLFW_KEY_I              ,
    J                  =GLFW_KEY_J              ,
    K                  =GLFW_KEY_K              ,
    L                  =GLFW_KEY_L              ,
    M                  =GLFW_KEY_M              ,
    N                  =GLFW_KEY_N              ,
    O                  =GLFW_KEY_O              ,
    P                  =GLFW_KEY_P              ,
    Q                  =GLFW_KEY_Q              ,
    R                  =GLFW_KEY_R              ,
    S                  =GLFW_KEY_S              ,
    T                  =GLFW_KEY_T              ,
    U                  =GLFW_KEY_U              ,
    V                  =GLFW_KEY_V              ,
    W                  =GLFW_KEY_W              ,
    X                  =GLFW_KEY_X              ,
    Y                  =GLFW_KEY_Y              ,
    Z                  =GLFW_KEY_Z              ,
    LEFT_BRACKET       =GLFW_KEY_LEFT_BRACKET   ,
    BACKSLASH          =GLFW_KEY_BACKSLASH      ,
    RIGHT_BRACKET      =GLFW_KEY_RIGHT_BRACKET  ,
    GRAVE_ACCENT       =GLFW_KEY_GRAVE_ACCENT   ,
    WORLD_1            =GLFW_KEY_WORLD_1        ,
    WORLD_2            =GLFW_KEY_WORLD_2        ,
    ESCAPE             =GLFW_KEY_ESCAPE         ,
    ENTER              =GLFW_KEY_ENTER          ,
    TAB                =GLFW_KEY_TAB            ,
    BACKSPACE          =GLFW_KEY_BACKSPACE      ,
    INSERT             =GLFW_KEY_INSERT         ,
    DEL                =GLFW_KEY_DELETE         ,
    RIGHT              =GLFW_KEY_RIGHT          ,
    LEFT               =GLFW_KEY_LEFT           ,
    DOWN               =GLFW_KEY_DOWN           ,
    UP                 =GLFW_KEY_UP             ,
    PAGE_UP            =GLFW_KEY_PAGE_UP        ,
    PAGE_DOWN          =GLFW_KEY_PAGE_DOWN      ,
    HOME               =GLFW_KEY_HOME           ,
    END                =GLFW_KEY_END            ,
    CAPS_LOCK          =GLFW_KEY_CAPS_LOCK      ,
    SCROLL_LOCK        =GLFW_KEY_SCROLL_LOCK    ,
    NUM_LOCK           =GLFW_KEY_NUM_LOCK       ,
    PRINT_SCREEN       =GLFW_KEY_PRINT_SCREEN   ,
    PAUSE              =GLFW_KEY_PAUSE          ,
    F1                 =GLFW_KEY_F1             ,
    F2                 =GLFW_KEY_F2             ,
    F3                 =GLFW_KEY_F3             ,
    F4                 =GLFW_KEY_F4             ,
    F5                 =GLFW_KEY_F5             ,
    F6                 =GLFW_KEY_F6             ,
    F7                 =GLFW_KEY_F7             ,
    F8                 =GLFW_KEY_F8             ,
    F9                 =GLFW_KEY_F9             ,
    F10                =GLFW_KEY_F10            ,
    F11                =GLFW_KEY_F11            ,
    F12                =GLFW_KEY_F12            ,
    F13                =GLFW_KEY_F13            ,
    F14                =GLFW_KEY_F14            ,
    F15                =GLFW_KEY_F15            ,
    F16                =GLFW_KEY_F16            ,
    F17                =GLFW_KEY_F17            ,
    F18                =GLFW_KEY_F18            ,
    F19                =GLFW_KEY_F19            ,
    F20                =GLFW_KEY_F20            ,
    F21                =GLFW_KEY_F21            ,
    F22                =GLFW_KEY_F22            ,
    F23                =GLFW_KEY_F23            ,
    F24                =GLFW_KEY_F24            ,
    F25                =GLFW_KEY_F25            ,
    KP_0               =GLFW_KEY_KP_0           ,
    KP_1               =GLFW_KEY_KP_1           ,
    KP_2               =GLFW_KEY_KP_2           ,
    KP_3               =GLFW_KEY_KP_3           ,
    KP_4               =GLFW_KEY_KP_4           ,
    KP_5               =GLFW_KEY_KP_5           ,
    KP_6               =GLFW_KEY_KP_6           ,
    KP_7               =GLFW_KEY_KP_7           ,
    KP_8               =GLFW_KEY_KP_8           ,
    KP_9               =GLFW_KEY_KP_9           ,
    KP_DECIMAL         =GLFW_KEY_KP_DECIMAL     ,
    KP_DIVIDE          =GLFW_KEY_KP_DIVIDE      ,
    KP_MULTIPLY        =GLFW_KEY_KP_MULTIPLY    ,
    KP_SUBTRACT        =GLFW_KEY_KP_SUBTRACT    ,
    KP_ADD             =GLFW_KEY_KP_ADD         ,
    KP_ENTER           =GLFW_KEY_KP_ENTER       ,
    KP_EQUAL           =GLFW_KEY_KP_EQUAL       ,
    LEFT_SHIFT         =GLFW_KEY_LEFT_SHIFT     ,
    LEFT_CONTROL       =GLFW_KEY_LEFT_CONTROL   ,
    LEFT_ALT           =GLFW_KEY_LEFT_ALT       ,
    LEFT_SUPER         =GLFW_KEY_LEFT_SUPER     ,
    RIGHT_SHIFT        =GLFW_KEY_RIGHT_SHIFT    ,
    RIGHT_CONTROL      =GLFW_KEY_RIGHT_CONTROL  ,
    RIGHT_ALT          =GLFW_KEY_RIGHT_ALT      ,
    RIGHT_SUPER        =GLFW_KEY_RIGHT_SUPER    ,
    MENU               =GLFW_KEY_MENU           ,
    LAST               =GLFW_KEY_LAST
};

class GLFW_Window_Handler
{

public:
    //static void __OnWindowPosition(GLFWwindow* window,int x, int y)                    { static_cast<GLFW_Window*>(glfwGetWindowUserPointer(window))->OnWindowPosition(x,y); }
    //static void __OnWindowSize(GLFWwindow* window,int width, int height)               { static_cast<GLFW_Window*>(glfwGetWindowUserPointer(window))->OnWindowSize(width, height); }
    //static void __OnFramebufferSize(GLFWwindow* window,int width, int height)          { static_cast<GLFW_Window*>(glfwGetWindowUserPointer(window))->OnFramebufferSize(width,height); }
    //static void __OnClose(GLFWwindow* window)                                          { static_cast<GLFW_Window*>(glfwGetWindowUserPointer(window))->OnClose(); }
    //static void __OnRefresh(GLFWwindow* window)                                        { static_cast<GLFW_Window*>(glfwGetWindowUserPointer(window))->OnRefresh(); }
    //static void __OnFocus(GLFWwindow* window,int focused)                              { static_cast<GLFW_Window*>(glfwGetWindowUserPointer(window))->OnFocus(focused); }
    //static void __OnIconify(GLFWwindow* window,int iconified)                          { static_cast<GLFW_Window*>(glfwGetWindowUserPointer(window))->OnIconify(iconified); }
    //static void __OnMouseEnter(GLFWwindow* window,int entered)                         { static_cast<GLFW_Window*>(glfwGetWindowUserPointer(window))->OnMouseEnter(entered); }
    //static void __OnScroll(GLFWwindow* window,double x, double y)                      { static_cast<GLFW_Window*>(glfwGetWindowUserPointer(window))->OnScroll(x,y); }
    //static void __OnCharacter(GLFWwindow* window,unsigned int codepoint)               { static_cast<GLFW_Window*>(glfwGetWindowUserPointer(window))->OnCharacter(codepoint); }
    //static void __OnCharacterMods(GLFWwindow* window,unsigned int codepoint, int mods) { static_cast<GLFW_Window*>(glfwGetWindowUserPointer(window))->OnCharacterMods(codepoint, mods); }
    //static void __OnDrop(GLFWwindow* window,int count, const char** paths)             { static_cast<GLFW_Window*>(glfwGetWindowUserPointer(window))->OnDrop(count, paths); }

    static void __OnMouseButton(GLFWwindow* window,int button, int action, int mods)
    {
        auto * THIS = static_cast<GLFW_Window_Handler*>( glfwGetWindowUserPointer(window) );
        THIS->m_button[ static_cast<Button>(button) ] = action;
        static_cast<GLFW_Window_Handler*>(glfwGetWindowUserPointer(window))->onMouseButton( static_cast<Button>(button) ,action);
    }

    static void __OnMousePosition(GLFWwindow* window,double x, double y)
    {
        auto * THIS = static_cast<GLFW_Window_Handler*>( glfwGetWindowUserPointer(window) );
        static_cast<GLFW_Window_Handler*>(glfwGetWindowUserPointer(window))->onMouseMove(x,y);
        THIS->m_mouse_x = x;
        THIS->m_mouse_y = y;
    }

    static void __OnKey(GLFWwindow* window,int key, int scancode, int action, int mods)
    {
        auto * THIS = static_cast<GLFW_Window_Handler*>( glfwGetWindowUserPointer(window) );
        THIS->m_key[ static_cast<Key>(key) ] = action;
        static_cast<GLFW_Window_Handler*>(glfwGetWindowUserPointer(window))->onKey( static_cast<Key>(key) , action);
    }

    GLFW_Window_Handler(GLFWwindow * w = nullptr) : m_Window(w)
    {
        if(m_Window)
        {
            m_start_time = std::chrono::system_clock::now();
            SetupCallbacks(m_Window);
        }
    }

    void attach_window(GLFWwindow *w)
    {
        m_Window = w;
        SetupCallbacks(w);
    }

    ~GLFW_Window_Handler()
    {
        if( m_Window)
        {
            glfwDestroyWindow(m_Window);
        }

        if(count()-- == 0)
        {
            glfwTerminate();
        }
    }

    double mouse_y()
    {
        return m_mouse_y;
    }

    double mouse_x()
    {
        return m_mouse_x;
    }

    void set_window_title( const char * title)
    {
         glfwSetWindowTitle( m_Window, title);
    }

    void Poll()
    {
        double t = std::chrono::duration<double>(std::chrono::system_clock::now() - m_start_time).count();
        glfwPollEvents();
        onPoll(t);
    }

    operator bool()
    {
        return !glfwWindowShouldClose(m_Window);
    }


    inline void show_cursor(bool b)
    {
        glfwSetInputMode(m_Window, GLFW_CURSOR, b ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }

    bool is_pressed(Button b) const
    {
        auto f = m_button.find(b);
        if( f == m_button.end() ) return false;
        return f->second;
    }

    bool is_pressed(Key k) const
    {
        auto f = m_key.find(k);
        if( f == m_key.end() ) return false;
        return f->second;
    }

    signal<void(double  , double)>    onMouseMove;
    signal<void(Button  , int   )>    onMouseButton;
    signal<void(Key,      int   )>    onKey;
    signal<void(double)>              onPoll;



    private:
        std::map< Key, bool>    m_key;
        std::map< Button, bool> m_button;

        double m_mouse_x;
        double m_mouse_y;

        std::chrono::system_clock::time_point m_start_time;
        static int & count()
        {
            static std::shared_ptr<int> c = std::make_shared<int>(0);
            return *c;
        }

        GLFWwindow * m_Window;

        void SetupCallbacks( GLFWwindow * w)
        {
            glfwSetWindowUserPointer( w , this);

            //glfwSetWindowPosCallback(      w,  GLFW_Window::__OnWindowPosition);
            //glfwSetWindowSizeCallback(     w,  GLFW_Window::__OnWindowSize);
            //glfwSetFramebufferSizeCallback(w,  GLFW_Window::__OnFramebufferSize);
            //glfwSetWindowCloseCallback(    w,  GLFW_Window::__OnClose);
            //glfwSetWindowRefreshCallback(  w,  GLFW_Window::__OnRefresh);
            //glfwSetWindowFocusCallback(    w,  GLFW_Window::__OnFocus);
            //glfwSetWindowIconifyCallback(  w,  GLFW_Window::__OnIconify);
            //glfwSetCursorEnterCallback(    w,  GLFW_Window::__OnMouseEnter);
            //glfwSetScrollCallback(         w,  GLFW_Window::__OnScroll);
            //glfwSetCharCallback(           w,  GLFW_Window::__OnCharacter);
            //glfwSetCharModsCallback(       w,  GLFW_Window::__OnCharacterMods);
            //glfwSetDropCallback(           w,  GLFW_Window::__OnDrop);

            glfwSetMouseButtonCallback(    w,  GLFW_Window_Handler::__OnMouseButton);
            glfwSetCursorPosCallback(      w,  GLFW_Window_Handler::__OnMousePosition);
            glfwSetKeyCallback(            w,  GLFW_Window_Handler::__OnKey);
        }

};


}

#endif
