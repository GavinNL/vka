#ifndef VKA_BUFFER_MEMORY_MANAGER_H
#define VKA_BUFFER_MEMORY_MANAGER_H

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <iostream>
#include <cassert>
#include <exception>
#include <list>

namespace vka
{

/**
 * @brief The buffer_memory_manager class
 *
 * A buffer memory manager is a class which keeps
 * track of where allocated and free memory are
 * in a block of contigiuos memory.
 *
 */
class buffer_memory_manager
{

    struct info
    {
      size_t m_size      = 0;
      size_t m_offset    = 0;
      bool   m_allocated = false;


      bool empty()
      {
        return !m_allocated;
      }
    };
public:

    static constexpr size_t error = std::numeric_limits<size_t>::max();

    buffer_memory_manager(size_t i)
    {
        reset(i);
    }

    void reset(size_t total_bytes)
    {
        m_list.clear();
        info k;
        k.m_size      = total_bytes;
        k.m_offset    = 0;
        k.m_allocated = false;

        m_list.push_front(k);
    }

    size_t num_blocks() const
    {
        return m_list.size();
    }

    /**
     * @brief free
     * @param offset
     *
     * Frees the the block which is at the given offset.
     * If no block exists, it will throw an error
     */
    void free(size_t offset)
    {
        auto i = m_list.begin();
        while( i != m_list.end() )
        {
            if( i->m_offset == offset )
            {
                i->m_allocated = false;
                forward_merge(i);
                return;
            }
            ++i;
        }
        throw std::runtime_error("Bad free");
    }

    void print(size_t block_size=8)
    {
        static int i=0;
        char s[] = {'#','&','@'};

        for(auto & f : m_list)
        {
            if( f.empty() )
            {
                std::cout << std::string(f.m_size/block_size, '.');
            }
            else
            {
                std::cout << std::string(f.m_size/block_size, s[i%sizeof(s)]);
                i++;
            }
        }
        std::cout << std::endl;
    }




    size_t allocate(size_t n)
    {
        auto i = m_list.begin();
        while( i != m_list.end() )
        {
            if( i->empty()  )
            {
                if( n <= i->m_size )
                {
                    info free_block;

                    free_block.m_size      = i->m_size - n;
                    free_block.m_offset    = i->m_offset + n;
                    free_block.m_allocated = false;

                    i->m_size = n;
                    i->m_allocated = true;
                    auto offset = i->m_offset;

                    i++;
                    if( free_block.m_size != 0)
                        m_list.insert(i,free_block);
                    return offset;
                }
            }
            ++i;
        }
        return error;
    }

private:
    size_t m_size;
    size_t m_used=0;
    size_t m_offset=0;

    std::list<info> m_list;

    void forward_merge(  std::list<info>::iterator iterator)
    {
        auto next = iterator;
        next++;

        if( next != m_list.end())
        {
            if( iterator->empty() && next->empty() )
            {
                iterator->m_size += next->m_size;
                m_list.erase(next);
                forward_merge(iterator);
                iterator--;
                forward_merge( iterator );
            }
        }
    }
};


}


#endif
