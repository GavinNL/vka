#ifndef VKA_SDL_WINDOW
#define VKA_SDL_WINDOW

#include<functional>
#include <memory>
#include <vector>
#include <map>
#include <mutex>

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_vulkan.h>

#include "signals.h"

namespace vka
{

enum class Button
{
      _1         =  0     ,
      _2         =  1     ,
      _3         =  2     ,
      _4         =  3     ,
      _5         =  4     ,
      _6         =  5     ,
      _7         =  6     ,
      _8         =  7     ,
      LAST      =  7  ,
      LEFT      =  SDL_BUTTON_LEFT  ,
      RIGHT     =  SDL_BUTTON_RIGHT ,
      MIDDLE    =  SDL_BUTTON_MIDDLE
};

enum class Key
{
    /* The unknown key */
    UNKNOWN            =SDL_SCANCODE_UNKNOWN        ,
    SPACE              =SDL_SCANCODE_SPACE          ,
    APOSTROPHE         =SDL_SCANCODE_APOSTROPHE     ,
    COMMA              =SDL_SCANCODE_COMMA          ,
    MINUS              =SDL_SCANCODE_MINUS          ,
    PERIOD             =SDL_SCANCODE_PERIOD         ,
    SLASH              =SDL_SCANCODE_SLASH          ,
    _0                  =SDL_SCANCODE_0             ,
    _1                  =SDL_SCANCODE_1             ,
    _2                  =SDL_SCANCODE_2             ,
    _3                  =SDL_SCANCODE_3             ,
    _4                  =SDL_SCANCODE_4             ,
    _5                  =SDL_SCANCODE_5             ,
    _6                  =SDL_SCANCODE_6             ,
    _7                  =SDL_SCANCODE_7             ,
    _8                  =SDL_SCANCODE_8             ,
    _9                  =SDL_SCANCODE_9             ,
    SEMICOLON          =SDL_SCANCODE_SEMICOLON      ,
    EQUAL              =SDL_SCANCODE_EQUALS          ,
    A                  =SDL_SCANCODE_A              ,
    B                  =SDL_SCANCODE_B              ,
    C                  =SDL_SCANCODE_C              ,
    D                  =SDL_SCANCODE_D              ,
    E                  =SDL_SCANCODE_E              ,
    F                  =SDL_SCANCODE_F              ,
    G                  =SDL_SCANCODE_G              ,
    H                  =SDL_SCANCODE_H              ,
    I                  =SDL_SCANCODE_I              ,
    J                  =SDL_SCANCODE_J              ,
    K                  =SDL_SCANCODE_K              ,
    L                  =SDL_SCANCODE_L              ,
    M                  =SDL_SCANCODE_M              ,
    N                  =SDL_SCANCODE_N              ,
    O                  =SDL_SCANCODE_O              ,
    P                  =SDL_SCANCODE_P              ,
    Q                  =SDL_SCANCODE_Q              ,
    R                  =SDL_SCANCODE_R              ,
    S                  =SDL_SCANCODE_S              ,
    T                  =SDL_SCANCODE_T              ,
    U                  =SDL_SCANCODE_U              ,
    V                  =SDL_SCANCODE_V              ,
    W                  =SDL_SCANCODE_W              ,
    X                  =SDL_SCANCODE_X              ,
    Y                  =SDL_SCANCODE_Y              ,
    Z                  =SDL_SCANCODE_Z              ,
    LEFT_BRACKET       =SDL_SCANCODE_LEFTBRACKET   ,
    BACKSLASH          =SDL_SCANCODE_BACKSLASH      ,
    RIGHT_BRACKET      =SDL_SCANCODE_RIGHTBRACKET  ,
    GRAVE_ACCENT       =SDL_SCANCODE_GRAVE   ,
    //WORLD_1            =SDL_SCANCODE_WORLD_1        ,
    //WORLD_2            =SDL_SCANCODE_WORLD_2        ,
    ESCAPE             =SDL_SCANCODE_ESCAPE         ,
    ENTER              =SDL_SCANCODE_RETURN          ,
    TAB                =SDL_SCANCODE_TAB            ,
    BACKSPACE          =SDL_SCANCODE_BACKSPACE      ,
    INSERT             =SDL_SCANCODE_INSERT         ,
    DEL                =SDL_SCANCODE_DELETE         ,
    RIGHT              =SDL_SCANCODE_RIGHT          ,
    LEFT               =SDL_SCANCODE_LEFT           ,
    DOWN               =SDL_SCANCODE_DOWN           ,
    UP                 =SDL_SCANCODE_UP             ,
    PAGE_UP            =SDL_SCANCODE_PAGEUP        ,
    PAGE_DOWN          =SDL_SCANCODE_PAGEDOWN      ,
    HOME               =SDL_SCANCODE_HOME           ,
    END                =SDL_SCANCODE_END            ,
    CAPS_LOCK          =SDL_SCANCODE_CAPSLOCK      ,
    SCROLL_LOCK        =SDL_SCANCODE_SCROLLLOCK    ,
    NUM_LOCK           =SDL_SCANCODE_NUMLOCKCLEAR       ,
    PRINT_SCREEN       =SDL_SCANCODE_PRINTSCREEN   ,
    PAUSE              =SDL_SCANCODE_PAUSE          ,
    F1                 =SDL_SCANCODE_F1             ,
    F2                 =SDL_SCANCODE_F2             ,
    F3                 =SDL_SCANCODE_F3             ,
    F4                 =SDL_SCANCODE_F4             ,
    F5                 =SDL_SCANCODE_F5             ,
    F6                 =SDL_SCANCODE_F6             ,
    F7                 =SDL_SCANCODE_F7             ,
    F8                 =SDL_SCANCODE_F8             ,
    F9                 =SDL_SCANCODE_F9             ,
    F10                =SDL_SCANCODE_F10            ,
    F11                =SDL_SCANCODE_F11            ,
    F12                =SDL_SCANCODE_F12            ,
    F13                =SDL_SCANCODE_F13            ,
    F14                =SDL_SCANCODE_F14            ,
    F15                =SDL_SCANCODE_F15            ,
    F16                =SDL_SCANCODE_F16            ,
    F17                =SDL_SCANCODE_F17            ,
    F18                =SDL_SCANCODE_F18            ,
    F19                =SDL_SCANCODE_F19            ,
    F20                =SDL_SCANCODE_F20            ,
    F21                =SDL_SCANCODE_F21            ,
    F22                =SDL_SCANCODE_F22            ,
    F23                =SDL_SCANCODE_F23            ,
    F24                =SDL_SCANCODE_F24            ,
    KP_0               =SDL_SCANCODE_KP_0           ,
    KP_1               =SDL_SCANCODE_KP_1           ,
    KP_2               =SDL_SCANCODE_KP_2           ,
    KP_3               =SDL_SCANCODE_KP_3           ,
    KP_4               =SDL_SCANCODE_KP_4           ,
    KP_5               =SDL_SCANCODE_KP_5           ,
    KP_6               =SDL_SCANCODE_KP_6           ,
    KP_7               =SDL_SCANCODE_KP_7           ,
    KP_8               =SDL_SCANCODE_KP_8           ,
    KP_9               =SDL_SCANCODE_KP_9           ,
    KP_DECIMAL         =SDL_SCANCODE_KP_DECIMAL     ,
    KP_DIVIDE          =SDL_SCANCODE_KP_DIVIDE      ,
    KP_MULTIPLY        =SDL_SCANCODE_KP_MULTIPLY    ,
    KP_SUBTRACT        =SDL_SCANCODE_KP_MINUS    ,
    KP_ADD             =SDL_SCANCODE_KP_PLUS         ,
    KP_ENTER           =SDL_SCANCODE_KP_ENTER       ,
    KP_EQUAL           =SDL_SCANCODE_KP_EQUALS       ,
    LEFT_SHIFT         =SDL_SCANCODE_LSHIFT     ,
    LEFT_CONTROL       =SDL_SCANCODE_LCTRL   ,
    LEFT_ALT           =SDL_SCANCODE_LALT       ,
    LEFT_SUPER         =SDL_SCANCODE_APPLICATION     ,
    RIGHT_SHIFT        =SDL_SCANCODE_RSHIFT    ,
    RIGHT_CONTROL      =SDL_SCANCODE_RCTRL  ,
    RIGHT_ALT          =SDL_SCANCODE_RALT      ,
    RIGHT_SUPER        =SDL_SCANCODE_APPLICATION,
    MENU               =SDL_SCANCODE_MENU           ,
};

class SDL_Window_Handler
{

public:

    SDL_Window_Handler(SDL_Window * w = nullptr) : m_Window(w)
    {
        m_start_time = std::chrono::system_clock::now();
    }

    void attach_window(SDL_Window *w)
    {
        m_Window = w;
    }

    ~SDL_Window_Handler()
    {
        if( m_Window)
        {
            SDL_DestroyWindow(m_Window);
        }

        if(count()-- == 0)
        {
            SDL_Quit();
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
        SDL_SetWindowTitle(m_Window , title);
    }

    void Poll()
    {
        double t = std::chrono::duration<double>(std::chrono::system_clock::now() - m_start_time).count();
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch(event.type)
            {
            case SDL_QUIT:
                m_quit = true;
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                m_button[static_cast<Button>(event.button.button)] = event.type==SDL_MOUSEBUTTONDOWN?1:0;
                onMouseButton( static_cast<Button>(event.button.button), event.type==SDL_MOUSEBUTTONDOWN?1:0);
                break;
            case SDL_MOUSEMOTION:
                onMouseMove( (double)event.motion.x, (double)event.motion.y);
                m_mouse_x = (double)event.motion.x;
                m_mouse_y = (double)event.motion.y;
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                m_key[static_cast<Key>(event.key.keysym.scancode)] = event.type==SDL_KEYDOWN?1:0;
                onKey( static_cast<Key>(event.key.keysym.scancode), event.type==SDL_KEYDOWN?1:0);
                break;

            }
        }
        onPoll(t);
    }

    bool m_quit = false;
    operator bool()
    {
        return m_quit;
    }


    inline void show_cursor(bool b)
    {
        SDL_ShowCursor(b);
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

        SDL_Window * m_Window;



};


}

#endif
