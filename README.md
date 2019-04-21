# Moe.LuaWrapper

Header-Only Lua API封装层。

持续开发中。

## Example

```c++
#include <Moe.LuaWrapper/State.hpp>

#include <iostream>

using namespace std;
using namespace moe;

class GameObject
{
public:
    // 侵入式注册实现，在首次使用该类型时完成注册
    static void Register(LuaWrapper::TypeRegister<GameObject>& reg)
    {
        reg.RegisterProperty("x", &GameObject::GetX, &GameObject::SetX);
        reg.RegisterMethod("meow", &GameObject::Meow);
    }

public:
    int GetX()const noexcept { return m_iX; }
    void SetX(int x) { m_iX = x; }

    void Meow() { cout << "meow!" << endl; }

private:
    int m_iX = 0;
};

GameObject CreateGameObject()
{
    return GameObject {};
}

int main()
{
    LuaWrapper::State L;
    L.OpenStdLibs();

    L.RegisterModule("prefabs")
        .RegisterMethod("create_game_object", CreateGameObject);

    // 非侵入式类型注册
    /*
    L.RegisterType<GameObject>()
        .RegisterProperty("x", &GameObject::GetX, &GameObject::SetX)
        .RegisterMethod("meow", &GameObject::Meow);
     */

    try
    {
        L.LoadString("local obj = prefabs.create_game_object(); obj.x = 100; print(obj.x); obj:meow()");
        L.CallAndThrow(0, 0);
    }
    catch (const std::exception& ex)
    {
        cerr << ex.what() << endl;
    }
    return 0;
}
```
