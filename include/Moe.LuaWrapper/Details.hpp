/**
 * @file
 * @date 2019/4/20
 * @author chu
 */
#pragma once
#include "Stack.hpp"

#if defined(__clang__)
#if __has_feature(cxx_rtti)
#define MOE_LUAWRP_RTTI
#endif
#elif defined(__GNUG__)
#if defined(__GXX_RTTI)
#define MOE_LUAWRP_RTTI
#endif
#elif defined(_MSC_VER)
#if defined(_CPPRTTI)
#define MOE_LUAWRP_RTTI
#endif
#endif

#ifdef MOE_LUAWRP_RTTI
#include <typeinfo>
#endif

namespace moe
{
namespace LuaWrapper
{
    class Stack;

    /**
     * @brief 类型注册器
     * @tparam T 类型
     */
    template <typename T>
    class TypeRegister
    {
    public:
        TypeRegister(Stack& st, int mtAbsIdx)
            : m_stStack(st), m_iIndex(mtAbsIdx) {}

        TypeRegister(const TypeRegister&) = delete;

        TypeRegister& operator=(const TypeRegister&) = delete;

    protected:
        TypeRegister(TypeRegister&& rhs)noexcept
            : m_stStack(std::move(rhs.m_stStack)), m_iIndex(rhs.m_iIndex)
        {
            rhs.m_iIndex = 0;
        }

        TypeRegister& operator=(TypeRegister&& rhs)noexcept
        {
            m_stStack = std::move(rhs.m_stStack);
            m_iIndex = rhs.m_iIndex;
            rhs.m_iIndex = 0;
            return *this;
        }

    public:
        /**
         * @brief 注册方法
         * @tparam TRet 返回值
         * @tparam TArgs 参数类型
         * @param name 方法名称
         * @param f 方法
         */
        template <typename TRet, typename... TArgs>
        TypeRegister& RegisterMethod(const char* name, TRet(T::*f)(TArgs...))
        {
            m_stStack.Push(name);
            m_stStack.Push(f);
            m_stStack.RawSet(m_iIndex);
            return *this;
        }

        template <typename TRet, typename... TArgs>
        TypeRegister& RegisterMethod(const char* name, TRet(T::*f)(TArgs...)const)
        {
            m_stStack.Push(name);
            m_stStack.Push(f);
            m_stStack.RawSet(m_iIndex);
            return *this;
        }

        /**
         * @brief 注册一个原生方法
         * @param name 名称
         * @param func 回调函数
         */
        TypeRegister& RegisterMethod(const char* name, lua_CFunction func)
        {
            m_stStack.Push(name);
            m_stStack.Push(func);
            m_stStack.RawSet(m_iIndex);
            return *this;
        }

        /**
         * @brief 注册只读属性
         * @tparam TValue 值类型
         * @param name 属性名称
         * @param reader 读取方法
         */
        template <typename TValue>
        TypeRegister& RegisterProperty(const char* name, TValue(T::*reader)())
        {
            m_stStack.Push("__get_");
            m_stStack.Push(name);
            m_stStack.Concact(2);
            m_stStack.Push(reader);
            m_stStack.RawSet(m_iIndex);
            return *this;
        }

        template <typename TValue>
        TypeRegister& RegisterProperty(const char* name, TValue(T::*reader)()const)
        {
            m_stStack.Push("__get_");
            m_stStack.Push(name);
            m_stStack.Concact(2);
            m_stStack.Push(reader);
            m_stStack.RawSet(m_iIndex);
            return *this;
        }

        /**
         * @brief 注册读写属性
         * @tparam TValue 值类型
         * @param name 属性名称
         * @param reader 读取方法
         * @param writer 赋值方法
         */
        template <typename TValue, typename TValue2 = TValue>
        TypeRegister& RegisterProperty(const char* name, TValue(T::*reader)(), void(T::*writer)(TValue2))
        {
            m_stStack.Push("__get_");
            m_stStack.Push(name);
            m_stStack.Concact(2);
            m_stStack.Push(reader);
            m_stStack.RawSet(m_iIndex);

            m_stStack.Push("__set_");
            m_stStack.Push(name);
            m_stStack.Concact(2);
            m_stStack.Push(writer);
            m_stStack.RawSet(m_iIndex);
            return *this;
        }

        template <typename TValue, typename TValue2 = TValue>
        TypeRegister& RegisterProperty(const char* name, TValue(T::*reader)()const, void(T::*writer)(TValue2))
        {
            m_stStack.Push("__get_");
            m_stStack.Push(name);
            m_stStack.Concact(2);
            m_stStack.Push(reader);
            m_stStack.RawSet(m_iIndex);

            m_stStack.Push("__set_");
            m_stStack.Push(name);
            m_stStack.Concact(2);
            m_stStack.Push(writer);
            m_stStack.RawSet(m_iIndex);
            return *this;
        }

    protected:
        Stack m_stStack;
        int m_iIndex = 0;
    };

    namespace details
    {
        template <class, template <class> class>
        struct IsInstance : public std::false_type {};

        template <class T, template <class> class U>
        struct IsInstance<U<T>, U> : public std::true_type {};

        // --- StackIndexSeq ---

        template <int... Ints>
        struct StackIndexSeq
        {
            using Type = StackIndexSeq;
            using ValueType = int;
            static constexpr std::size_t Size()noexcept { return sizeof...(Ints); }
        };

        template <class Seq1, class Seq2>
        struct StackIndexSeqMerger;

        template <int... I1, int... I2>
        struct StackIndexSeqMerger<StackIndexSeq<I1...>, StackIndexSeq<I2...>> :
            StackIndexSeq<I1..., (sizeof...(I1) + I2)...>
        {};

        template <size_t N>
        struct MakeStackIndexSeq :
            StackIndexSeqMerger<typename MakeStackIndexSeq<N / 2>::Type, typename MakeStackIndexSeq<N - N / 2>::Type>
        {};

        template <>
        struct MakeStackIndexSeq<0> : StackIndexSeq<>
        {};

        template <>
        struct MakeStackIndexSeq<1> : StackIndexSeq<1>
        {};

        template <typename... TArgs>
        struct MatchFuncArgsIndexSeq :
            MakeStackIndexSeq<sizeof...(TArgs)>
        {};

        template <typename... TArgs>
        struct MatchFuncArgsIndexSeq<Stack&, TArgs...> :
            MakeStackIndexSeq<sizeof...(TArgs)>
        {};

        // --- TypeHelper ---

        template <typename T>
        using RemoveCVType = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

#ifdef MOE_LUAWRP_RTTI
        template <typename T>
        struct TypeNameConstructor
        {
            std::string Buffer;

            TypeNameConstructor(uintptr_t id)
            {
                static_cast<void>(id);

                const auto& info = typeid(T);
                Buffer.append("__type_");
                Buffer.append(info.name());
                Buffer.append("_");
                Buffer.append(std::to_string(id));
            }

            const char* GetBuffer()const noexcept
            {
                return Buffer.c_str();
            }
        };
#else
        template <typename T>
        struct TypeNameConstructor
        {
            char Buffer[32];

            TypeNameConstructor(uintptr_t id)
            {
                sprintf(Buffer, "__type_%p", reinterpret_cast<void*>(id));
            }

            const char* GetBuffer()const noexcept
            {
                return Buffer;
            }
        };
#endif

        template <typename T>
        struct TypeHelperImpl
        {
            static uintptr_t TypeId()noexcept
            {
                static int s_iStub;
                return reinterpret_cast<uintptr_t>(&s_iStub);
            }

            static const char* TypeName()noexcept
            {
                static const TypeNameConstructor<T> s_stName(TypeId());
                return s_stName.GetBuffer();
            }
        };

        template <typename T>
        struct TypeHelper :
            TypeHelperImpl<RemoveCVType<T>>
        {};

        // --- Object ---

        struct ObjectHeader
        {
            //uintptr_t TypeId;
        };

        template <typename T>
        struct ObjectImpl
        {
            using Type = T;

            ObjectHeader Header;
            typename std::aligned_storage<sizeof(T), alignof(T)>::type Value;
        };

        template <typename T>
        struct Object :
            ObjectImpl<RemoveCVType<T>>
        {};

        // --- TypeRegisterHelper ---

        struct HasStaticMethodRegisterValidator
        {
            template <class T,
                typename = typename std::decay<decltype(T::Register)>::type>
            static std::true_type Test(int);

            template <typename>
            static std::false_type Test(...);
        };

        template <typename T>
        struct HasStaticMethodRegister :
            public decltype(HasStaticMethodRegisterValidator::template Test<T>(0))
        {};

        struct GenericRegisterFuncsBase
        {
            static int IndexWrapper(lua_State* L)  // obj, k
            {
                lua_pushvalue(L, 2);  // obj, key, key
                lua_rawget(L, lua_upvalueindex(1));  // obj, key, value

                if (!lua_isnil(L, -1))
                    return 1;

                // try getter
                lua_pop(L, 1);  // obj, key

                lua_pushstring(L, "__get_");  // obj, key, s
                lua_pushvalue(L, 2);  // obj, key, s, key
                lua_concat(L, 2);  // obj, key, s
                lua_rawget(L, lua_upvalueindex(1));  // obj, key, getter

                if (lua_isnil(L, -1))
                    return 1;

                // call getter
                lua_insert(L, 1);  // getter, obj, key
                lua_call(L, 2, 1);
                return 1;
            }

            static int NewIndexWrapper(lua_State* L)
            {
                lua_pushvalue(L, 2);  // obj, key, value, key
                lua_insert(L, 1);  // key, obj, key, value
                lua_remove(L, 3);  // key, obj, value
                lua_pushvalue(L, 1);  // key, obj, value, key

                lua_pushstring(L, "__set_");  // key, obj, value, key, s
                lua_pushvalue(L, 1);  // key, obj, value, key, s, key
                lua_concat(L, 2);  // key, obj, value, key, s
                lua_rawget(L, lua_upvalueindex(1));  // key, obj, value, key, setter

                if (lua_isnil(L, -1))
                {
                    luaL_error(L, "Property '%s' cannot be set", lua_tostring(L, 1));
                    return 0;
                }

                // call setter
                lua_insert(L, 2);  // key, setter, obj, value, key
                lua_call(L, 3, 0);
                return 0;
            }

            static void Register(Stack& st)
            {
                // 注册__index方法
                st.Push("__index");
                st.PushValue(-2);
                st.PushNativeClosure(IndexWrapper, 1);
                st.RawSet(-3);

                // 注册__newindex方法
                st.Push("__newindex");
                st.PushValue(-2);
                st.PushNativeClosure(NewIndexWrapper, 1);
                st.RawSet(-3);
            }
        };

        template <typename T>
        struct GenericRegisterFuncs :
            GenericRegisterFuncsBase
        {
            static int GCWrapper(lua_State* L)
            {
                Stack st(L);

                // 对象
                auto p = static_cast<Object<T>*>(luaL_checkudata(L, 1, TypeHelper<T>::TypeName()));
                if (!p)
                {
                    assert(false);
                    return 0;
                }

                reinterpret_cast<T*>(reinterpret_cast<char*>(&p->Value))->~T();
                return 0;
            }

            static void Register(Stack& st)
            {
                GenericRegisterFuncsBase::Register(st);

                // 注册GC方法
                st.Push("__gc");
                st.Push(GCWrapper);
                st.RawSet(-3);
            }
        };

        template <typename T, int Tag>
        struct TypeRegisterHelperImpl;

        template <typename T>
        struct TypeRegisterHelperImpl<T, 0>
        {
            static const bool CanAutoRegister = false;
        };

        template <typename T>
        struct TypeRegisterHelperImpl<T, 1>
        {
            static const bool CanAutoRegister = true;

            static void Register(Stack& st)
            {
                // 注册基本函数
                GenericRegisterFuncs<T>::Register(st);

                // 调用注册函数
                TypeRegister<T> reg(st, st.GetTop());
                T::Register(reg);
            }
        };

        template <typename T>
        struct TypeRegisterHelperImpl<T, 2>
        {
            static const bool CanAutoRegister = true;

            static void Register(Stack& st)
            {
                // 注册基本函数
                GenericRegisterFuncs<T>::Register(st);
            }
        };

        template <typename T>
        struct TypeRegisterHelper :
            std::conditional<
                HasStaticMethodRegister<T>::value,
                TypeRegisterHelperImpl<T, 1>,
                typename std::conditional<
                    IsInstance<T, std::function>::value,
                    TypeRegisterHelperImpl<T, 2>,
                    TypeRegisterHelperImpl<T, 0>>::type>
                ::type
        {};

        // --- FunctionWrapper ---

        template <class TSeq, typename TRet, typename... TArgs>
        struct FunctionWrapper;

        template <int... Ints, typename... TArgs>
        struct FunctionWrapper<StackIndexSeq<Ints...>, void, TArgs...>
        {
            static int Wrapper(lua_State* L)
            {
                Stack st(L);

                auto ptr = reinterpret_cast<void(*)(TArgs...)>(lua_touserdata(L, lua_upvalueindex(1)));
                assert(ptr);

                try
                {
                    ptr(st.Read<TArgs>(Ints)...);
                }
                catch (const std::exception& ex)
                {
                    st.Error("%s", ex.what());
                }
                return 0;
            }

            static void Push(Stack& st, void(*f)(TArgs...))
            {
                st.PushLightUserData(reinterpret_cast<void*>(f));  // upvalue 1
                st.PushNativeClosure(Wrapper, 1);
            }
        };

        template <int... Ints, typename TRet, typename... TArgs>
        struct FunctionWrapper<StackIndexSeq<Ints...>, TRet, TArgs...>
        {
            static int Wrapper(lua_State* L)
            {
                Stack st(L);

                auto ptr = reinterpret_cast<TRet(*)(TArgs...)>(lua_touserdata(L, lua_upvalueindex(1)));
                assert(ptr);

                try
                {
                    return st.Push(ptr(st.Read<TArgs>(Ints)...));
                }
                catch (const std::exception& ex)
                {
                    st.Error("%s", ex.what());
                    return 0;
                }
            }

            static void Push(Stack& st, TRet(*f)(TArgs...))
            {
                st.PushLightUserData(reinterpret_cast<void*>(f));  // upvalue 1
                st.PushNativeClosure(Wrapper, 1);
            }
        };

        template <int... Ints, typename... TArgs>
        struct FunctionWrapper<StackIndexSeq<Ints...>, void, Stack&, TArgs...>
        {
            static int Wrapper(lua_State* L)
            {
                Stack st(L);

                auto ptr = reinterpret_cast<void(*)(Stack&, TArgs...)>(lua_touserdata(L, lua_upvalueindex(1)));
                assert(ptr);

                try
                {
                    ptr(st, st.Read<TArgs>(Ints)...);
                }
                catch (const std::exception& ex)
                {
                    st.Error("%s", ex.what());
                }
                return 0;
            }

            static void Push(Stack& st, void(*f)(Stack&, TArgs...))
            {
                st.PushLightUserData(reinterpret_cast<void*>(f));  // upvalue 1
                st.PushNativeClosure(Wrapper, 1);
            }
        };

        template <int... Ints, typename TRet, typename... TArgs>
        struct FunctionWrapper<StackIndexSeq<Ints...>, TRet, Stack&, TArgs...>
        {
            static int Wrapper(lua_State* L)
            {
                Stack st(L);

                auto ptr = reinterpret_cast<TRet(*)(Stack&, TArgs...)>(lua_touserdata(L, lua_upvalueindex(1)));
                assert(ptr);

                try
                {
                    return st.Push(ptr(st, st.Read<TArgs>(Ints)...));
                }
                catch (const std::exception& ex)
                {
                    st.Error("%s", ex.what());
                    return 0;
                }
            }

            static void Push(Stack& st, TRet(*f)(Stack&, TArgs...))
            {
                st.PushLightUserData(reinterpret_cast<void*>(f));  // upvalue 1
                st.PushNativeClosure(Wrapper, 1);
            }
        };

        // --- MemberFunctionWrapper ---

        template <class TSeq, typename T, typename TRet, typename... TArgs>
        struct MemberFunctionWrapper;

        template <int... Ints, typename T, typename... TArgs>
        struct MemberFunctionWrapper<StackIndexSeq<Ints...>, T, void, TArgs...>
        {
            struct MemberPtrWrapper
            {
                void(T::*Ptr)(TArgs...);
            };

            static int Wrapper(lua_State* L)
            {
                Stack st(L);

                // 获得成员函数指针
                auto w = static_cast<MemberPtrWrapper*>(lua_touserdata(L, lua_upvalueindex(1)));
                assert(w);

                // 对象
                auto p = static_cast<Object<T>*>(luaL_checkudata(L, 1, TypeHelper<T>::TypeName()));
                if (!p)
                {
                    assert(false);
                    return 0;
                }

                try
                {
                    (reinterpret_cast<T*>(reinterpret_cast<char*>(&p->Value))->*(w->Ptr))(st.Read<TArgs>(Ints + 1)...);
                }
                catch (const std::exception& ex)
                {
                    st.Error("%s", ex.what());
                }
                return 0;
            }

            static void Push(Stack& st, void(T::*f)(TArgs...))
            {
                auto p = static_cast<MemberPtrWrapper*>(lua_newuserdata(st, sizeof(MemberPtrWrapper)));  // upvalue 1
                if (!p)
                    throw std::bad_alloc();

                p->Ptr = f;
                st.PushNativeClosure(Wrapper, 1);
            }
        };

        template <int... Ints, typename T, typename TRet, typename... TArgs>
        struct MemberFunctionWrapper<StackIndexSeq<Ints...>, T, TRet, TArgs...>
        {
            struct MemberPtrWrapper
            {
                TRet(T::*Ptr)(TArgs...);
            };

            static int Wrapper(lua_State* L)
            {
                Stack st(L);

                // 获得成员函数指针
                auto w = static_cast<MemberPtrWrapper*>(lua_touserdata(L, lua_upvalueindex(1)));
                assert(w);

                // 对象
                auto p = static_cast<Object<T>*>(luaL_checkudata(L, 1, TypeHelper<T>::TypeName()));
                if (!p)
                {
                    assert(false);
                    return 0;
                }

                try
                {
                    return st.Push((reinterpret_cast<T*>(reinterpret_cast<char*>(&p->Value))->*(w->Ptr))(
                        st.Read<TArgs>(Ints + 1)...));
                }
                catch (const std::exception& ex)
                {
                    st.Error("%s", ex.what());
                    return 0;
                }
            }

            static void Push(Stack& st, TRet(T::*f)(TArgs...))
            {
                auto p = static_cast<MemberPtrWrapper*>(lua_newuserdata(st, sizeof(MemberPtrWrapper)));  // upvalue 1
                if (!p)
                    throw std::bad_alloc();

                p->Ptr = f;
                st.PushNativeClosure(Wrapper, 1);
            }
        };

        template <int... Ints, typename T, typename... TArgs>
        struct MemberFunctionWrapper<StackIndexSeq<Ints...>, T, void, Stack&, TArgs...>
        {
            struct MemberPtrWrapper
            {
                void(T::*Ptr)(Stack&, TArgs...);
            };

            static int Wrapper(lua_State* L)
            {
                Stack st(L);

                // 获得成员函数指针
                auto w = static_cast<MemberPtrWrapper*>(lua_touserdata(L, lua_upvalueindex(1)));
                assert(w);

                // 对象
                auto p = static_cast<Object<T>*>(luaL_checkudata(L, 1, TypeHelper<T>::TypeName()));
                if (!p)
                {
                    assert(false);
                    return 0;
                }

                try
                {
                    (reinterpret_cast<T*>(reinterpret_cast<char*>(&p->Value))->*(w->Ptr))(st,
                        st.Read<TArgs>(Ints + 1)...);
                }
                catch (const std::exception& ex)
                {
                    st.Error("%s", ex.what());
                }
                return 0;
            }

            static void Push(Stack& st, void(T::*f)(Stack&, TArgs...))
            {
                auto p = static_cast<MemberPtrWrapper*>(lua_newuserdata(st, sizeof(MemberPtrWrapper)));  // upvalue 1
                if (!p)
                    throw std::bad_alloc();

                p->Ptr = f;
                st.PushNativeClosure(Wrapper, 1);
            }
        };

        template <int... Ints, typename T, typename TRet, typename... TArgs>
        struct MemberFunctionWrapper<StackIndexSeq<Ints...>, T, TRet, Stack&, TArgs...>
        {
            struct MemberPtrWrapper
            {
                TRet(T::*Ptr)(Stack&, TArgs...);
            };

            static int Wrapper(lua_State* L)
            {
                Stack st(L);

                // 获得成员函数指针
                auto w = static_cast<MemberPtrWrapper*>(lua_touserdata(L, lua_upvalueindex(1)));
                assert(w);

                // 对象
                auto p = static_cast<Object<T>*>(luaL_checkudata(L, 1, TypeHelper<T>::TypeName()));
                if (!p)
                {
                    assert(false);
                    return 0;
                }

                try
                {
                    return st.Push((reinterpret_cast<T*>(reinterpret_cast<char*>(&p->Value))->*(w->Ptr))(st,
                        st.Read<TArgs>(Ints + 1)...));
                }
                catch (const std::exception& ex)
                {
                    st.Error("%s", ex.what());
                    return 0;
                }
            }

            static void Push(Stack& st, TRet(T::*f)(Stack&, TArgs...))
            {
                auto p = static_cast<MemberPtrWrapper*>(lua_newuserdata(st, sizeof(MemberPtrWrapper)));  // upvalue 1
                if (!p)
                    throw std::bad_alloc();

                p->Ptr = f;
                st.PushNativeClosure(Wrapper, 1);
            }
        };

        // --- ConstMemberFunctionWrapper ---

        template <class TSeq, typename T, typename TRet, typename... TArgs>
        struct ConstMemberFunctionWrapper;

        template <int... Ints, typename T, typename... TArgs>
        struct ConstMemberFunctionWrapper<StackIndexSeq<Ints...>, T, void, TArgs...>
        {
            struct MemberPtrWrapper
            {
                void(T::*Ptr)(TArgs...)const;
            };

            static int Wrapper(lua_State* L)
            {
                Stack st(L);

                // 获得成员函数指针
                auto w = static_cast<MemberPtrWrapper*>(lua_touserdata(L, lua_upvalueindex(1)));
                assert(w);

                // 对象
                auto p = static_cast<Object<T>*>(luaL_checkudata(L, 1, TypeHelper<T>::TypeName()));
                if (!p)
                {
                    assert(false);
                    return 0;
                }

                try
                {
                    (reinterpret_cast<T*>(reinterpret_cast<char*>(&p->Value))->*(w->Ptr))(st.Read<TArgs>(Ints + 1)...);
                }
                catch (const std::exception& ex)
                {
                    st.Error("%s", ex.what());
                }
                return 0;
            }

            static void Push(Stack& st, void(T::*f)(TArgs...)const)
            {
                auto p = static_cast<MemberPtrWrapper*>(lua_newuserdata(st, sizeof(MemberPtrWrapper)));  // upvalue 1
                if (!p)
                    throw std::bad_alloc();

                p->Ptr = f;
                st.PushNativeClosure(Wrapper, 1);
            }
        };

        template <int... Ints, typename T, typename TRet, typename... TArgs>
        struct ConstMemberFunctionWrapper<StackIndexSeq<Ints...>, T, TRet, TArgs...>
        {
            struct MemberPtrWrapper
            {
                TRet(T::*Ptr)(TArgs...)const;
            };

            static int Wrapper(lua_State* L)
            {
                Stack st(L);

                // 获得成员函数指针
                auto w = static_cast<MemberPtrWrapper*>(lua_touserdata(L, lua_upvalueindex(1)));
                assert(w);

                // 对象
                auto p = static_cast<Object<T>*>(luaL_checkudata(L, 1, TypeHelper<T>::TypeName()));
                if (!p)
                {
                    assert(false);
                    return 0;
                }

                try
                {
                    return st.Push((reinterpret_cast<T*>(reinterpret_cast<char*>(&p->Value))->*(w->Ptr))(
                        st.Read<TArgs>(Ints + 1)...));
                }
                catch (const std::exception& ex)
                {
                    st.Error("%s", ex.what());
                    return 0;
                }
            }

            static void Push(Stack& st, TRet(T::*f)(TArgs...)const)
            {
                auto p = static_cast<MemberPtrWrapper*>(lua_newuserdata(st, sizeof(MemberPtrWrapper)));  // upvalue 1
                if (!p)
                    throw std::bad_alloc();

                p->Ptr = f;
                st.PushNativeClosure(Wrapper, 1);
            }
        };

        template <int... Ints, typename T, typename... TArgs>
        struct ConstMemberFunctionWrapper<StackIndexSeq<Ints...>, T, void, Stack&, TArgs...>
        {
            struct MemberPtrWrapper
            {
                void(T::*Ptr)(Stack&, TArgs...)const;
            };

            static int Wrapper(lua_State* L)
            {
                Stack st(L);

                // 获得成员函数指针
                auto w = static_cast<MemberPtrWrapper*>(lua_touserdata(L, lua_upvalueindex(1)));
                assert(w);

                // 对象
                auto p = static_cast<Object<T>*>(luaL_checkudata(L, 1, TypeHelper<T>::TypeName()));
                if (!p)
                {
                    assert(false);
                    return 0;
                }

                try
                {
                    (reinterpret_cast<T*>(reinterpret_cast<char*>(&p->Value))->*(w->Ptr))(st,
                        st.Read<TArgs>(Ints + 1)...);
                }
                catch (const std::exception& ex)
                {
                    st.Error("%s", ex.what());
                }
                return 0;
            }

            static void Push(Stack& st, void(T::*f)(Stack&, TArgs...)const)
            {
                auto p = static_cast<MemberPtrWrapper*>(lua_newuserdata(st, sizeof(MemberPtrWrapper)));  // upvalue 1
                if (!p)
                    throw std::bad_alloc();

                p->Ptr = f;
                st.PushNativeClosure(Wrapper, 1);
            }
        };

        template <int... Ints, typename T, typename TRet, typename... TArgs>
        struct ConstMemberFunctionWrapper<StackIndexSeq<Ints...>, T, TRet, Stack&, TArgs...>
        {
            struct MemberPtrWrapper
            {
                TRet(T::*Ptr)(Stack&, TArgs...)const;
            };

            static int Wrapper(lua_State* L)
            {
                Stack st(L);

                // 获得成员函数指针
                auto w = static_cast<MemberPtrWrapper*>(lua_touserdata(L, lua_upvalueindex(1)));
                assert(w);

                // 对象
                auto p = static_cast<Object<T>*>(luaL_checkudata(L, 1, TypeHelper<T>::TypeName()));
                if (!p)
                {
                    assert(false);
                    return 0;
                }

                try
                {
                    return st.Push((reinterpret_cast<T*>(reinterpret_cast<char*>(&p->Value))->*(w->Ptr))(st,
                        st.Read<TArgs>(Ints + 1)...));
                }
                catch (const std::exception& ex)
                {
                    st.Error("%s", ex.what());
                    return 0;
                }
            }

            static void Push(Stack& st, TRet(T::*f)(Stack&, TArgs...)const)
            {
                auto p = static_cast<MemberPtrWrapper*>(lua_newuserdata(st, sizeof(MemberPtrWrapper)));  // upvalue 1
                if (!p)
                    throw std::bad_alloc();

                p->Ptr = f;
                st.PushNativeClosure(Wrapper, 1);
            }
        };

        // --- StdFunctionWrapper ---

        template <class TSeq, typename TRet, typename... TArgs>
        struct StdFunctionWrapper;

        template <int... Ints, typename... TArgs>
        struct StdFunctionWrapper<StackIndexSeq<Ints...>, void, TArgs...>
        {
            using FuncType = std::function<void(TArgs...)>;

            static int Wrapper(lua_State* L)
            {
                Stack st(L);
#ifndef NDEBUG
                auto p = static_cast<Object<FuncType>*>(luaL_checkudata(L, lua_upvalueindex(1), TypeHelper<FuncType>::TypeName()));
                assert(p);
#else
                auto p = static_cast<Object<FuncType>*>(lua_touserdata(L, lua_upvalueindex(1)));
#endif

                auto& obj = *reinterpret_cast<FuncType*>(reinterpret_cast<char*>(&p->Value));
                try
                {
                    if (obj)
                        obj(st.Read<TArgs>(Ints)...);
                }
                catch (const std::exception& ex)
                {
                    st.Error("%s", ex.what());
                }
                return 0;
            }

            static void Push(Stack& st, FuncType&& func)
            {
                st.New<FuncType>(std::move(func));  // upvalue 1
                st.PushNativeClosure(Wrapper, 1);
            }
        };

        template <int... Ints, typename TRet, typename... TArgs>
        struct StdFunctionWrapper<StackIndexSeq<Ints...>, TRet, TArgs...>
        {
            using FuncType = std::function<TRet(TArgs...)>;

            static int Wrapper(lua_State* L)
            {
                Stack st(L);
#ifndef NDEBUG
                auto p = static_cast<Object<FuncType>*>(luaL_checkudata(L, lua_upvalueindex(1), TypeHelper<FuncType>::TypeName()));
                assert(p);
#else
                auto p = static_cast<Object<FuncType>*>(lua_touserdata(L, lua_upvalueindex(1)));
#endif

                auto& obj = *reinterpret_cast<FuncType*>(reinterpret_cast<char*>(&p->Value));
                try
                {
                    if (obj)
                        return st.Push(obj(st.Read<TArgs>(Ints)...));
                }
                catch (const std::exception& ex)
                {
                    st.Error("%s", ex.what());
                }
                return 0;
            }

            static void Push(Stack& st, FuncType&& func)
            {
                st.New<FuncType>(std::move(func));  // upvalue 1
                st.PushNativeClosure(Wrapper, 1);
            }
        };

        template <int... Ints, typename... TArgs>
        struct StdFunctionWrapper<StackIndexSeq<Ints...>, void, Stack&, TArgs...>
        {
            using FuncType = std::function<void(Stack&, TArgs...)>;

            static int Wrapper(lua_State* L)
            {
                Stack st(L);
#ifndef NDEBUG
                auto p = static_cast<Object<FuncType>*>(luaL_checkudata(L, lua_upvalueindex(1), TypeHelper<FuncType>::TypeName()));
                assert(p);
#else
                auto p = static_cast<Object<FuncType>*>(lua_touserdata(L, lua_upvalueindex(1)));
#endif

                auto& obj = *reinterpret_cast<FuncType*>(reinterpret_cast<char*>(&p->Value));
                try
                {
                    if (obj)
                        obj(st, st.Read<TArgs>(Ints)...);
                }
                catch (const std::exception& ex)
                {
                    st.Error("%s", ex.what());
                }
                return 0;
            }

            static void Push(Stack& st, FuncType&& func)
            {
                st.New<FuncType>(std::move(func));  // upvalue 1
                st.PushNativeClosure(Wrapper, 1);
            }
        };

        template <int... Ints, typename TRet, typename... TArgs>
        struct StdFunctionWrapper<StackIndexSeq<Ints...>, TRet, Stack&, TArgs...>
        {
            using FuncType = std::function<TRet(Stack&, TArgs...)>;

            static int Wrapper(lua_State* L)
            {
                Stack st(L);
#ifndef NDEBUG
                auto p = static_cast<Object<FuncType>*>(luaL_checkudata(L, lua_upvalueindex(1), TypeHelper<FuncType>::TypeName()));
                assert(p);
#else
                auto p = static_cast<Object<FuncType>*>(lua_touserdata(L, lua_upvalueindex(1)));
#endif

                auto& obj = *reinterpret_cast<FuncType*>(reinterpret_cast<char*>(&p->Value));
                try
                {
                    if (obj)
                        return st.Push(obj(st, st.Read<TArgs>(Ints)...));
                }
                catch (const std::exception& ex)
                {
                    st.Error("%s", ex.what());
                }
                return 0;
            }

            static void Push(Stack& st, FuncType&& func)
            {
                st.New<FuncType>(std::move(func));  // upvalue 1
                st.PushNativeClosure(Wrapper, 1);
            }
        };
    }

    template <typename TRet, typename... TArgs>
    int Stack::Push(TRet(*v)(TArgs...))
    {
        details::FunctionWrapper<typename details::MatchFuncArgsIndexSeq<TArgs...>::Type, TRet, TArgs...>::
            Push(*this, v);
        return 1;
    }

    template <typename T, typename TRet, typename... TArgs>
    int Stack::Push(TRet(T::*v)(TArgs...))
    {
        details::MemberFunctionWrapper<typename details::MatchFuncArgsIndexSeq<TArgs...>::Type, T, TRet, TArgs...>::
            Push(*this, std::move(v));
        return 1;
    }

    template <typename T, typename TRet, typename... TArgs>
    int Stack::Push(TRet(T::*v)(TArgs...)const)
    {
        details::ConstMemberFunctionWrapper<
            typename details::MatchFuncArgsIndexSeq<TArgs...>::Type, T, TRet, TArgs...>::Push(*this, std::move(v));
        return 1;
    }

    template <typename T>
    typename std::enable_if<details::IsOtherType<T>::value, int>::type Stack::Push(T&& rhs)
    {
        New<T>(std::forward<T>(rhs));
        return 1;
    }

    template <typename TRet, typename... TArgs>
    int Stack::Push(std::function<TRet(TArgs...)>&& v)
    {
        details::StdFunctionWrapper<typename details::MatchFuncArgsIndexSeq<TArgs...>::Type, TRet, TArgs...>::Push(*this, std::move(v));
        return 1;
    }

    template <typename T>
    typename std::enable_if<std::is_class<typename std::decay<T>::type>::value && details::IsOtherType<T>::value, T>::type
    Stack::Read(int idx)
    {
        auto p = static_cast<details::Object<T>*>(luaL_checkudata(L, idx, details::TypeHelper<T>::TypeName()));
        assert(p);

        return *reinterpret_cast<typename details::Object<T>::Type*>(reinterpret_cast<char*>(&p->Value));
    }

    template <typename T, typename... TArgs>
    typename std::enable_if<details::TypeRegisterHelper<typename details::Object<T>::Type>::CanAutoRegister, T&>::type
    Stack::New(TArgs&&... args)
    {
#ifndef NDEBUG
        int topCheck = lua_gettop(L);
#endif

        using RealType = typename details::Object<T>::Type;

        // 构造对象
        auto p = static_cast<details::Object<T>*>(lua_newuserdata(L, sizeof(details::Object<T>)));
        if (!p)
            throw std::bad_alloc();

        try
        {
            //p->Header.TypeId = details::TypeHelper<T>::TypeId();
            new(&p->Value) RealType(std::forward<TArgs>(args)...);
        }
        catch (...)
        {
            lua_pop(L, 1);

#ifndef NDEBUG
            assert(topCheck == lua_gettop(L));
#endif
            throw;
        }

        auto ret = reinterpret_cast<RealType*>(reinterpret_cast<char*>(&p->Value));

        // 如果对象未被注册，则调用方法进行注册
        if (luaL_newmetatable(L, details::TypeHelper<T>::TypeName()))
        {
            try
            {
                details::TypeRegisterHelper<RealType>::Register(*this);
            }
            catch (...)
            {
                ret->~RealType();
                lua_pop(L, 2);

#ifndef NDEBUG
                assert(topCheck == lua_gettop(L));
#endif
                throw;
            }
        }

        // 设置元表
        lua_setmetatable(L, -2);

#ifndef NDEBUG
        assert(topCheck == lua_gettop(L) - 1);
#endif
        return *ret;
    }

    template <typename T, typename... TArgs>
    typename std::enable_if<!details::TypeRegisterHelper<typename details::Object<T>::Type>::CanAutoRegister, T&>::type
    Stack::New(TArgs&&... args)
    {
#ifndef NDEBUG
        int topCheck = lua_gettop(L);
#endif

        using RealType = typename details::Object<T>::Type;

        // 构造对象
        auto p = static_cast<details::Object<T>*>(lua_newuserdata(L, sizeof(details::Object<T>)));
        if (!p)
            throw std::bad_alloc();

        try
        {
            //p->Header.TypeId = details::TypeHelper<T>::TypeId();
            new(&p->Value) RealType(std::forward<TArgs>(args)...);
        }
        catch (...)
        {
            lua_pop(L, 1);

#ifndef NDEBUG
            assert(topCheck == lua_gettop(L));
#endif
            throw;
        }

        auto ret = reinterpret_cast<RealType*>(reinterpret_cast<char*>(&p->Value));

        // 获取元表
        luaL_getmetatable(L, details::TypeHelper<T>::TypeName());
        if (lua_isnil(L, -1))
        {
            ret->~RealType();
            lua_pop(L, 2);

#ifndef NDEBUG
            assert(topCheck == lua_gettop(L));
#endif
            throw std::runtime_error(std::string("User type is not registered: ") + details::TypeHelper<T>::TypeName());
        }

        // 设置元表
        lua_setmetatable(L, -2);

#ifndef NDEBUG
        assert(topCheck == lua_gettop(L) - 1);
#endif
        return *ret;
    }

    template <typename T>
    typename std::enable_if<std::is_class<typename std::decay<T>::type>::value && details::IsOtherType<T>::value, bool>::type
    Stack::CheckType(int idx)
    {
        return static_cast<details::Object<T>*>(luaL_checkudata(L, idx, details::TypeHelper<T>::TypeName())) != nullptr;
    }
}
}
