#pragma once

// 引擎工具链兼容头。
//
// cocos2d-x 4.0（2019 年）依赖旧版 libc++ 的传递包含，而新版 Xcode/libc++ 已移除
// 这些传递包含，导致引擎源码出现 “no template named 'function' in namespace 'std'”
// 一类编译错误。
//
// 该头文件只通过编译选项 `-include` 强制预包含进 **引擎** 目标，不作用于本项目的
// `Classes/` 代码；本项目代码仍必须显式包含自己用到的头文件。
//
// 这里只补充标准库头，不修改引擎行为，也不引入项目类型。
//
// 引擎目标同时包含 C、C++ 和 Objective-C++ 源文件，而该头会被预包含进全部源文件，
// 因此必须用 __cplusplus 守卫，避免把 C++ 标准库头塞进 C 编译单元。

#ifdef __cplusplus

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#endif  // __cplusplus
