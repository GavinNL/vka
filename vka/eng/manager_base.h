#include <vka/core/classes.h>


namespace vka
{

class manager_base
{
    public:
        manager_base()
        {
        }

        manager_base& set_context(vka::context * context)
        {
            m_context = context;
            return *this;
        }

        vka::context* get_parent_context()
        {
            return m_context;
        }

    private:
        vka::context * m_context = nullptr;
};

}
