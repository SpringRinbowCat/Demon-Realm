// Windows 平台入口。
//
// 只负责创建应用对象并把控制权交给引擎主循环；窗口、分辨率和首个场景的装配都在
// GameLauncher 内完成，平台入口不承载业务逻辑。
#include "main.h"

#include "cocos2d.h"

#include "App/GameLauncher.hpp"

int WINAPI _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR lpCmdLine,
                     int nCmdShow)
{
    static_cast<void>(hInstance);
    static_cast<void>(hPrevInstance);
    static_cast<void>(lpCmdLine);
    static_cast<void>(nCmdShow);

    DemonRealm::GameLauncher app;
    return cocos2d::Application::getInstance()->run();
}
