/**
 * @file
 * @date 2019/4/20
 * @author chu
*/
#pragma once
#include "Stack.hpp"

namespace moe
{
namespace LuaWrapper
{
    /**
     * @brief 引用
     */
    class Reference
    {
        friend class Stack;

    public:
        /**
         * @brief 从栈上捕获值
         * @param st 堆栈
         * @return 引用对象
         *
         * 将会从栈顶Pop掉一个值，并计入引用表中。
         */
        static Reference Capture(Stack& st)
        {
#ifndef NDEBUG
            unsigned topCheck = st.GetTop();
#endif

#ifdef LUA_RIDX_MAINTHREAD
            st.Push<int>(LUA_RIDX_MAINTHREAD);  // ? i
            st.RawGet(LUA_REGISTRYINDEX);  // ? t
#else
            st.Push("__mainthread");  // ? s
            st.RawGet(LUA_REGISTRYINDEX);  // ? t
#endif

            Reference ret;
            ret.m_pContext = (st.TypeOf(-1) != LUA_TTHREAD ? st : Stack(lua_tothread(st, -1)));
            st.Pop(1);

            ret.m_iRef = luaL_ref(st, LUA_REGISTRYINDEX);

#ifndef NDEBUG
            assert(topCheck == st.GetTop() + 1);
#endif
            return ret;
        }

    public:
        Reference()noexcept = default;

        Reference(const Reference& rhs)
            : m_pContext(rhs.m_pContext)
        {
            auto ref = rhs.m_iRef;
            if (ref == LUA_NOREF || ref == LUA_REFNIL)
                m_iRef = ref;
            else
            {
                lua_rawgeti(m_pContext, LUA_REGISTRYINDEX, ref);
                m_iRef = luaL_ref(m_pContext, LUA_REGISTRYINDEX);
            }
        }

        Reference(Reference&& rhs)noexcept
            : m_pContext(std::move(rhs.m_pContext)), m_iRef(rhs.m_iRef)
        {
            rhs.m_iRef = LUA_NOREF;
        }

        ~Reference()noexcept
        {
            auto ref = m_iRef;
            if (ref != LUA_NOREF && ref != LUA_REFNIL)
                luaL_unref(m_pContext, LUA_REGISTRYINDEX, ref);
            m_iRef = LUA_NOREF;
        }

    public:
        operator bool()const noexcept
        {
            return m_iRef != LUA_NOREF;
        }

        Reference& operator=(const Reference& rhs)
        {
            auto ref = m_iRef;
            if (ref != LUA_NOREF && ref != LUA_REFNIL)
                luaL_unref(m_pContext, LUA_REGISTRYINDEX, ref);

            m_pContext = rhs.m_pContext;
            ref = rhs.m_iRef;
            if (ref == LUA_NOREF || ref == LUA_REFNIL)
                m_iRef = ref;
            else
            {
                lua_rawgeti(m_pContext, LUA_REGISTRYINDEX, ref);
                m_iRef = luaL_ref(m_pContext, LUA_REGISTRYINDEX);
            }
            return *this;
        }

        Reference& operator=(Reference&& rhs)noexcept
        {
            auto ref = m_iRef;
            if (ref != LUA_NOREF && ref != LUA_REFNIL)
                luaL_unref(m_pContext, LUA_REGISTRYINDEX, ref);

            m_pContext = std::move(rhs.m_pContext);
            m_iRef = std::move(rhs.m_iRef);
            rhs.m_iRef = LUA_NOREF;
            return *this;
        }

    public:
        bool IsEmpty()const noexcept { return m_iRef == LUA_NOREF; }
        bool IsNil()const noexcept { return m_iRef == LUA_REFNIL; }

    private:
        Stack m_pContext;
        int m_iRef = LUA_NOREF;
    };

    template <typename T>
    typename std::enable_if<details::IsReferenceType<T>::value, int>::type
    Stack::Push(const T& ref)
    {
        if (ref)
            lua_rawgeti(L, LUA_REGISTRYINDEX, ref.m_iRef);
        else
            lua_pushnil(L);
        return 1;
    }

    template <typename T>
    typename std::enable_if<details::IsReferenceType<T>::value, Reference>::type
    Stack::Read(int idx)
    {
        lua_pushvalue(L, idx);
        return std::move(Reference::Capture(*this));
    }
}
}
