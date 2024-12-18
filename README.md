# RySimpleRangeTester

一个简单易用的 C++ 测试框架，专门用于范围测试和批量数据验证。

## 特性

- 🚀 链式调用 API，简洁易用
- 📊 详细的测试报告和性能分析
- 🎯 支持预期失败测试
- 🔍 灵活的过滤和条件判断
- 🌈 彩色控制台输出
- ⚡ 性能追踪和统计
- 🔧 跨平台支持(Windows/Unix)

## 要求

- C++20 或更高版本

## 快速开始

### 基础用法

```cpp
#include "ry_simple_range_tester.hpp"
#include <vector>

void basic_test() {
    ry::RySimpleRangeTester<int> tester;
    
    // 添加测试项并执行
    tester.add(1, "Test_One", "第一个测试")
          .add(2, "Test_Two", "第二个测试")
          .add({3, 4, 5})  // 自动生成名称
          .forEach([](int n) { 
              return n > 0; 
          })
          .run();
}
