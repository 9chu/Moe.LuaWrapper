/**
 * @file
 * @date 2019/4/20
 * @author chu
*/
#pragma once
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <string>
#include <stdexcept>
#include <functional>
#include <type_traits>

#include <lua.hpp>

namespace moe
{
namespace LuaWrapper
{
    class Reference;

    namespace details
    {
        template <typename T>
        using IsStdStringType = typename std::is_same<typename std::decay<T>::type, std::string>;

        template <typename T>
        using IsReferenceType = typename std::is_same<typename std::decay<T>::type, Reference>;

        template <typename T>
        struct TypeRegisterHelper;

        template <typename T>
        struct Object;

        inline int TracebackImpl(lua_State *L)
        {
            int arg = 0;
            lua_State* L1 = nullptr;

            if (lua_isthread(L, 1))
            {
                arg = 1;
                L1 = lua_tothread(L, 1);
            }
            else
            {
                arg = 0;
                L1 = L;
            }

            const char* msg = lua_tostring(L, arg + 1);
            if (msg == nullptr && !lua_isnoneornil(L, arg + 1))
                lua_pushstring(L, "(Non-string error message)");
            else
            {
                int level = (int)luaL_optinteger(L, arg + 2, (L == L1) ? 1 : 0);
                luaL_traceback(L, L1, msg, level);
            }
            return 1;
        }
    }

    /**
     * @brief Lua栈封装
     */
    class Stack
    {
    public:
        Stack()noexcept = default;

        Stack(lua_State* vm)noexcept
            : L(vm)
        {}

        Stack(const Stack& rhs)noexcept
            : L(rhs.L)
        {}

        Stack(Stack&& rhs)noexcept
            : L(rhs.L)
        {
            rhs.L = nullptr;
        }

    public:
        operator lua_State*()const noexcept
        {
            return L;
        }

        Stack& operator=(const Stack& rhs)noexcept
        {
            L = rhs.L;
            return *this;
        }

        Stack& operator=(Stack&& rhs)noexcept
        {
            L = rhs.L;
            rhs.L = nullptr;
            return *this;
        }

    public:
        /**
         * @brief 获取栈高度
         */
        unsigned GetTop()
        {
            return static_cast<unsigned>(lua_gettop(L));
        }

        /**
         * @brief 设置栈高度
         * @param sz 栈大小
         */
        void SetTop(unsigned sz)
        {
            lua_settop(L, sz);
        }

        /**
         * @brief 获取对象类型
         * @param idx 栈索引
         * @return 类型
         *
         * [-0, +0]
         */
        int TypeOf(int idx)
        {
            return lua_type(L, idx);
        }

        /**
         * @brief 从栈上弹出N个对象
         */
        void Pop(unsigned n)
        {
            lua_pop(L, static_cast<int>(n));
        }

        /**
         * @brief 删除指定索引上的元素
         * @param idx 索引
         *
         * [-1, 0]
         */
        void Remove(int idx)
        {
            lua_remove(L, idx);
        }

        /**
         * @brief 将栈顶元素插入到指定位置
         * @param idx 绝对索引
         *
         * [-1, +1]
         */
        void Insert(unsigned idx)
        {
            lua_insert(L, static_cast<int>(idx));
        }

        /**
         * @brief 向栈内推入一个对象
         * @tparam T 类型
         * @param v 值
         *
         * [-0, +1]
         */
        void Push(std::nullptr_t)
        {
            lua_pushnil(L);
        }

        void Push(bool v)
        {
            lua_pushboolean(L, v);
        }

        void Push(char v)
        {
            lua_pushinteger(L, v);
        }

        void Push(uint8_t v)
        {
            lua_pushinteger(L, v);
        }

        void Push(int16_t v)
        {
            lua_pushinteger(L, v);
        }

        void Push(uint16_t v)
        {
            lua_pushinteger(L, v);
        }

        void Push(int32_t v)
        {
            lua_pushinteger(L, v);
        }

        void Push(uint32_t v)
        {
            lua_pushinteger(L, v);
        }

        void Push(lua_Number v)
        {
            lua_pushnumber(L, v);
        }

        void Push(lua_CFunction v)
        {
            if (v == nullptr)
                lua_pushnil(L);
            else
                lua_pushcfunction(L, v);
        }

        void Push(const char* v)
        {
            lua_pushstring(L, v);
        }

        void Push(const std::string& v)
        {
            lua_pushlstring(L, v.c_str(), v.size());
        }

        template <typename TRet, typename... TArgs>
        void Push(TRet(*v)(TArgs...));

        template <typename T, typename TRet, typename... TArgs>
        void Push(TRet(T::*v)(TArgs...));

        template <typename T, typename TRet, typename... TArgs>
        void Push(TRet(T::*v)(TArgs...)const);

        template <typename T>
        typename std::enable_if<details::IsReferenceType<T>::value, void>::type Push(const T& rhs);

        template <typename T>
        typename std::enable_if<!details::IsStdStringType<T>::value && !details::IsReferenceType<T>::value, void>::type Push(T&& rhs);

        template <typename TRet, typename... TArgs>
        void Push(std::function<TRet(TArgs...)>&& v);

        /**
         * @brief 拷贝栈上的值
         * @param idx 索引
         */
        void PushValue(int idx)
        {
            lua_pushvalue(L, idx);
        }

        /**
         * @brief 向栈中推入一个轻量级指针
         * @tparam T 对象类型
         * @param p 指针
         *
         * [-0, +1]
         */
        template <typename T>
        void PushLightUserData(T* p)
        {
            lua_pushlightuserdata(L, p);
        }

        /**
         * @brief 向栈中推入一个C闭包
         * @param fn 函数
         * @param n 捕获值数量
         *
         * [-n, +1]
         */
        void PushNativeClosure(lua_CFunction fn, unsigned n)
        {
            lua_pushcclosure(L, fn, n);
        }

        /**
         * @brief 从栈上读取一个类型到C++
         * @tparam T 类型
         * @param idx 栈索引
         * @return 值
         *
         * [-0, +0]
         */
        template <typename T>
        typename std::enable_if<std::is_fundamental<T>::value || std::is_same<T, const char*>::value ||
            std::is_same<T, lua_CFunction>::value, typename std::remove_reference<T>::type>::type Read(int idx=-1)
        {
            T ret;
            ReadImpl(ret, idx);
            return std::move(ret);
        }

        template <typename T>
        typename std::enable_if<std::is_class<T>::value && !details::IsStdStringType<T>::value &&
            !details::IsReferenceType<T>::value, T>::type
        Read(int idx=-1);

        template <typename T>
        typename std::enable_if<details::IsReferenceType<T>::value, Reference>::type Read(int idx=-1);

        template <typename T>
        typename std::enable_if<std::is_same<T, std::string>::value, std::string>::type Read(int idx=-1);

        template <typename T>
        typename std::enable_if<std::is_same<T, const std::string&>::value, std::string>::type Read(int idx=-1);

        /**
         * @brief 在栈上新建一个用户对象
         * @tparam T 类型
         * @tparam TArgs 构造参数类型
         * @param args 构造参数
         * @return 对象引用
         */
        template <typename T, typename... TArgs>
        typename std::enable_if<details::TypeRegisterHelper<typename details::Object<T>::Type>::CanAutoRegister, T&>::type
        New(TArgs&&... args);

        template <typename T, typename... TArgs>
        typename std::enable_if<!details::TypeRegisterHelper<typename details::Object<T>::Type>::CanAutoRegister, T&>::type
        New(TArgs&&... args);

        /**
         * @brief 对栈顶的N个元素执行拼接操作
         * @param n 元素
         *
         * [-n, +1]
         */
        void Concact(unsigned n)
        {
            lua_concat(L, static_cast<int>(n));
        }

        /**
         * @brief 在栈顶创建一个空表
         *
         * [-0, +1]
         */
        void NewTable()
        {
            lua_newtable(L);
        }

        /**
         * @brief 跳过metatable进行取值
         * @param idx 索引
         *
         * [-1, +1]
         */
        void RawGet(int idx)
        {
            lua_rawget(L, idx);
        }

        /**
         * @brief 跳过metatable进行赋值
         * @param idx 索引
         *
         * [-2, +0]
         */
        void RawSet(int idx)
        {
            lua_rawset(L, idx);
        }

        /**
         * @brief 执行给定栈位置上table对象的访问器方法
         * @param idx table索引
         * @param field key
         *
         * [-0, +1]
         */
        void GetField(int idx, const char* field)
        {
            lua_getfield(L, idx, field);
        }

        /**
         * @brief 设置指定栈位置的table的元素
         * @param idx table索引
         * @param field key
         *
         * [-1, +0]
         */
        void SetField(int idx, const char* field)
        {
            lua_setfield(L, idx, field);
        }

        /**
         * @brief 获取一个全局值
         * @param field key
         *
         * [-0, +1]
         */
        void GetGlobal(const char* field)
        {
            lua_getglobal(L, field);
        }

        /**
         * @brief 设置一个全局值
         * @param field key
         *
         * [-1, +0]
         */
        void SetGlobal(const char* field)
        {
            lua_setglobal(L, field);
        }

        /**
         * @brief 设置函数的执行上下文
         * @param idx 函数或者Thread的索引
         *
         * [-1, +0]
         */
        void SetFunctionEnvironment(int idx)
        {
            lua_setfenv(L, idx);
        }

        /**
         * @brief 抛出一个错误
         * @tparam TArgs 格式化参数类型
         * @param fmt 格式化文本
         * @param args 参数
         */
        template <typename... TArgs>
        [[noreturn]] void Error(const char* fmt, TArgs&&... args)
        {
            luaL_error(L, fmt, std::forward<TArgs>(args)...);
            ::abort();
        }

        /**
         * @brief 调用函数
         * @param nargs 参数个数
         * @param nrets 返回值个数
         *
         * [-(nargs+1), +nrets]
         */
        void Call(unsigned nargs, unsigned nrets)
        {
            lua_call(L, static_cast<int>(nargs), static_cast<int>(nrets));
        }

        /**
         * @brief 安全调用函数
         * @param nargs 参数个数
         * @param nrets 返回值个数
         * @param absIdxOfErrFunc 错误处理函数的绝对索引
         */
        void PCall(unsigned nargs, unsigned nrets, unsigned absIdxOfErrFunc)
        {
            lua_pcall(L, static_cast<int>(nargs), static_cast<int>(nrets), static_cast<int>(absIdxOfErrFunc));
        }

        /**
         * @brief 安全调用函数，并在错误时抛出C++异常
         * @param nargs 参数个数
         * @param nrets 返回值个数
         */
        void CallAndThrow(unsigned nargs, unsigned nrets)
        {
#ifndef NDEBUG
            int topCheck = lua_gettop(L);
#endif

            lua_pushcfunction(L, details::TracebackImpl);  // ... func, arg1, arg2, c

            int where = lua_gettop(L) - nargs - 1;
            assert(where >= 1);
            lua_insert(L, where);  // ... c, func, arg1, arg2

            if (0 != lua_pcall(L, nargs, nrets, where))  // ... c, ret1, ret2
            {
                std::string errmsg = lua_tostring(L, -1);
                lua_pop(L, 2);  // ...

#ifndef NDEBUG
                assert(topCheck - 1 == lua_gettop(L));
#endif

                throw std::runtime_error(std::move(errmsg));
            }

            lua_pop(L, 1);  // ...

#ifndef NDEBUG
            assert(topCheck - 1 == lua_gettop(L));
#endif
        }

        /**
         * @brief 加载缓冲区
         * @param content 内容
         * @param name 名称
         *
         * [-0, +1]
         *
         * 当加载失败时，抛出异常。
         */
        void LoadBuffer(const std::string& content, const char* name="")
        {
            if (0 != luaL_loadbuffer(L, content.c_str(), content.size(), name))
            {
                std::string errmsg = lua_tostring(L, -1);
                lua_pop(L, 1);

                throw std::runtime_error(std::move(errmsg));
            }
        }

        /**
         * @brief 从字符串编译
         * @param content 内容
         *
         * [-0, +1]
         *
         * 当编译失败时抛出异常。
         */
        void LoadString(const char* content)
        {
            if (0 != luaL_loadstring(L, content))
            {
                std::string errmsg = lua_tostring(L, -1);
                lua_pop(L, 1);

                throw std::runtime_error(std::move(errmsg));
            }
        }

    private:
        void ReadImpl(nullptr_t& out, int idx)
        {
            out = nullptr_t {};
            if (!lua_isnil(L, idx))
                luaL_typerror(L, idx, lua_typename(L, LUA_TNIL));
        }

        void ReadImpl(bool& out, int idx)
        {
            if (lua_isboolean(L, idx))
            {
                out = (lua_toboolean(L, idx) != 0);
                return;
            }
            else if (lua_isnumber(L, idx))
            {
                auto x = lua_tointeger(L, idx);
                out = (x != 0);
                return;
            }
            luaL_typerror(L, idx, lua_typename(L, LUA_TBOOLEAN));
        }

        void ReadImpl(char& out, int idx)
        {
            out = static_cast<char>(luaL_checkinteger(L, idx));
        }

        void ReadImpl(uint8_t& out, int idx)
        {
            out = static_cast<uint8_t>(luaL_checkinteger(L, idx));
        }

        void ReadImpl(int16_t& out, int idx)
        {
            out = static_cast<int16_t>(luaL_checkinteger(L, idx));
        }

        void ReadImpl(uint16_t& out, int idx)
        {
            out = static_cast<uint16_t>(luaL_checkinteger(L, idx));
        }

        void ReadImpl(int32_t& out, int idx)
        {
            out = static_cast<int32_t>(luaL_checkinteger(L, idx));
        }

        void ReadImpl(uint32_t& out, int idx)
        {
            out = static_cast<uint32_t>(luaL_checkinteger(L, idx));
        }

        void ReadImpl(lua_Integer& out, int idx)
        {
            out = luaL_checkinteger(L, idx);
        }

        void ReadImpl(lua_Number& out, int idx)
        {
            out = luaL_checknumber(L, idx);
        }

        void ReadImpl(const char*& out, int idx)
        {
            out = luaL_checkstring(L, idx);
        }

        void ReadImpl(lua_CFunction& out, int idx)
        {
            out = lua_tocfunction(L, idx);
        }

    protected:
        lua_State* L = nullptr;
    };

    template <typename T>
    typename std::enable_if<std::is_same<T, std::string>::value, std::string>::type Stack::Read(int idx)
    {
        std::string ret;
        size_t len = 0;

        auto p = luaL_checklstring(L, idx, &len);
        if (p)
            ret.assign(p, len);
        return ret;
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, const std::string&>::value, std::string>::type Stack::Read(int idx)
    {
        std::string ret;
        size_t len = 0;

        auto p = luaL_checklstring(L, idx, &len);
        if (p)
            ret.assign(p, len);
        return ret;
    }

    /**
     * @brief 栈平衡器
     */
    class StackBalancer
    {
    public:
        StackBalancer(Stack& st)
            : m_stStack(st), m_iTop(st.GetTop())
        {}

        StackBalancer(const StackBalancer&) = delete;
        StackBalancer(StackBalancer&&) = delete;

        ~StackBalancer()
        {
            m_stStack.SetTop(m_iTop);
        }

    public:
        unsigned GetTop()const noexcept { return m_iTop; }

    private:
        Stack& m_stStack;
        unsigned m_iTop = 0;
    };
}
}
