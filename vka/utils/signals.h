#ifndef VKA_SIGNALS_H
#define VKA_SIGNALS_H

#include<functional>
#include <memory>
#include <vector>
#include <map>
#include <mutex>

#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_vulkan.h>

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

            id detach()
            {
                auto x = m_id;
                m_id = 0;
                m_Container.reset();
                return x;
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

}

#endif
