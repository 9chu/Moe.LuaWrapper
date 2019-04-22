/**
 * @file
 * @date 2019/4/20
 * @author chu
*/
#pragma once
#include "Stack.hpp"
#include "Details.hpp"
#include "Reference.hpp"

namespace moe
{
namespace LuaWrapper
{
    class State;

    template <typename T>
    class RegisterTypeWrapper :
        public TypeRegister<T>
    {
        friend class State;

    public:
        RegisterTypeWrapper(Stack& st)
            : TypeRegister<T>(st, 0)
        {
#ifndef NDEBUG
            m_iCheckTop = TypeRegister<T>::m_stStack.GetTop();
#endif

            const char* name = details::TypeHelper<T>::TypeName();

            if (luaL_newmetatable(TypeRegister<T>::m_stStack, name))
            {
                // 注册基本函数
                details::GenericRegisterFuncs<T>::Register(st);
            }
            TypeRegister<T>::m_iIndex = TypeRegister<T>::m_stStack.GetTop();
        }

        ~RegisterTypeWrapper()
        {
            TypeRegister<T>::m_stStack.Remove(TypeRegister<T>::m_iIndex);

#ifndef NDEBUG
            if (static_cast<lua_State*>(TypeRegister<T>::m_stStack))
                assert(TypeRegister<T>::m_stStack.GetTop() == m_iCheckTop);
#endif
        }

    protected:
        RegisterTypeWrapper(RegisterTypeWrapper&& rhs)noexcept
            : TypeRegister<T>(std::move(rhs))
        {
#ifndef NDEBUG
            m_iCheckTop = rhs.m_iCheckTop;
            rhs.m_iCheckTop = 0;
#endif
        }

        RegisterTypeWrapper& operator=(RegisterTypeWrapper&& rhs)noexcept
        {
#ifndef NDEBUG
            m_iCheckTop = rhs.m_iCheckTop;
            rhs.m_iCheckTop = 0;
#endif
            return *this;
        }

    private:
#ifndef NDEBUG
        unsigned m_iCheckTop;
#endif
    };

    class RegisterModuleWrapper
    {
        friend class State;

    public:
        RegisterModuleWrapper(Stack& st, const char* name)
            : m_stStack(st)
        {
#ifndef NDEBUG
            m_iCheckTop = m_stStack.GetTop();
#endif

            m_stStack.NewTable();
            m_iIndex = m_stStack.GetTop();

            m_stStack.PushValue(-1);
            m_stStack.SetGlobal(name);
        }

        RegisterModuleWrapper(const RegisterModuleWrapper&) = delete;

        ~RegisterModuleWrapper()
        {
            m_stStack.Remove(m_iIndex);

#ifndef NDEBUG
            if (static_cast<lua_State*>(m_stStack))
                assert(m_stStack.GetTop() == m_iCheckTop);
#endif
        }

        RegisterModuleWrapper& operator=(const RegisterModuleWrapper&) = delete;

    public:
        template <typename T>
        RegisterModuleWrapper& RegisterValue(const char* name, T&& val)
        {
            m_stStack.Push(std::forward<T>(val));
            m_stStack.SetField(m_iIndex, name);
            return *this;
        }

        template <typename TRet, typename... TArgs>
        RegisterModuleWrapper& RegisterMethod(const char* name, TRet(*f)(TArgs...))
        {
            m_stStack.Push(f);
            m_stStack.SetField(m_iIndex, name);
            return *this;
        }

        RegisterModuleWrapper& RegisterMethod(const char* name, lua_CFunction func)
        {
            m_stStack.Push(func);
            m_stStack.SetField(m_iIndex, name);
            return *this;
        }

        template <typename TRet, typename... TArgs>
        RegisterModuleWrapper& RegisterMethod(const char* name, std::function<TRet(TArgs...)>&& func)
        {
            m_stStack.Push(std::move(func));
            m_stStack.SetField(m_iIndex, name);
            return *this;
        }

    protected:
        RegisterModuleWrapper(RegisterModuleWrapper&& rhs)noexcept
            : m_stStack(std::move(rhs.m_stStack)), m_iIndex(rhs.m_iIndex)
        {
            rhs.m_iIndex = 0;

#ifndef NDEBUG
            m_iCheckTop = rhs.m_iCheckTop;
            rhs.m_iCheckTop = 0;
#endif
        }

        RegisterModuleWrapper& operator=(RegisterModuleWrapper&& rhs)noexcept
        {
            m_stStack = std::move(rhs.m_stStack);
            m_iIndex = rhs.m_iIndex;
            rhs.m_iIndex = 0;

#ifndef NDEBUG
            m_iCheckTop = rhs.m_iCheckTop;
            rhs.m_iCheckTop = 0;
#endif
            return *this;
        }

    private:
#ifndef NDEBUG
        unsigned m_iCheckTop;
#endif
        Stack m_stStack;
        int m_iIndex = 0;
    };

    /**
     * @brief Lua状态封装
     */
    class State :
        public Stack
    {
    public:
        State()
            : Stack(luaL_newstate())
        {
            if (!L)
                throw std::runtime_error("luaL_newstate failed");
        }
        State(const State&) = delete;
        State(State&& rhs)noexcept
            : Stack(std::move(rhs))
        {}

        ~State()noexcept
        {
            if (L)
            {
                lua_close(L);
                L = nullptr;
            }
        }

    public:
        State& operator=(const State&) = delete;
        State& operator=(State&& rhs)noexcept
        {
            Stack::operator=(std::move(rhs));
            return *this;
        }

    public:
        /**
         * @brief 加载标准库
         */
        void OpenStdLibs()
        {
            luaL_openlibs(L);
        }

        /**
         * @brief 注册一个类型
         * @tparam T 类型
         * @return 类型注册器
         *
         * 若类型已被注册，则返回的类型注册器亦可用于修改元方法。
         */
        template <typename T>
        RegisterTypeWrapper<typename details::Object<T>::Type> RegisterType()
        {
            return RegisterTypeWrapper<typename details::Object<T>::Type>(*this);
        }

        /**
         * @brief 注册模块
         * @param name 模块名
         * @return 模块注册器
         */
        RegisterModuleWrapper RegisterModule(const char* name)
        {
            return RegisterModuleWrapper(*this, name);
        }
    };
}
}
