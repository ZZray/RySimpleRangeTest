#include <iostream>
#include "RySimpleRangeTester.h"

void testBasicUsage()
{
    RySimpleRangeTester<std::string> tester;

    // 1. 测试单个值添加
    tester.add("value1", "test1", "Basic test 1");
    tester.add("value2"); // 自动生成名称

    // 2. 测试 TestItem 添加
    RySimpleRangeTester<std::string>::TestItem item{"value3", "test3", "Manual test item", false};
    tester.add(item);

    // 3. 设置测试回调
    tester.forEach([](const std::string& value) {
        return !value.empty(); // 简单检查值是否非空
    });

    // 运行测试
    bool allPassed = tester.run();
    std::cout << "Basic tests " << (allPassed ? "PASSED" : "FAILED") << "\n";
}
//
void testContainerAddition()
{
    RySimpleRangeTester<int> tester;

    // 1. 测试vector<T>
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    tester.add(numbers, "num");

    // 2. 测试vector<TestItem>
    std::vector<RySimpleRangeTester<int>::TestItem> items;
    items.push_back({10, "test10", "Special test 10"});
    items.push_back({20, "test20", "Special test 20"});
    tester.add(items);

    // 3. 测试初始化列表
    tester.add({100, 200, 300});

    tester.forEach([](int value) {
        return value > 0; // 检查所有值是否为正
    });

    bool allPassed = tester.run();
    std::cout << "Container tests " << (allPassed ? "PASSED" : "FAILED") << "\n";
}

void testFailureScenarios()
{
    RySimpleRangeTester<double> tester;

    // 1. 添加预期会失败的测试
    tester.add(-1.0, "negative", "Should fail").expectFail("negative");

    // 2. 添加多个测试项，部分预期失败
    std::vector<RySimpleRangeTester<double>::TestItem> items = {{1.0, "positive1", "Should pass"}, {-2.0, "negative2", "Should fail", true}, {0.0, "zero", "Should pass"}};
    tester.add(items);

    // 3. 使用条件标记预期失败
    tester.expectFailIf([](const auto& item) {
        return item.item < 0;
    });

    tester.forEach([](double value) {
        return value >= 0; // 检查值是否非负
    });

    bool allPassed = tester.run();
    std::cout << "Failure scenario tests " << (allPassed ? "PASSED" : "FAILED") << "\n";
}

void testCustomTypes()
{
    struct Point
    {
        int x, y;
        bool isValid() const { return x >= 0 && y >= 0; }
    };

    RySimpleRangeTester<Point> tester;

    // 1. 添加单个Point
    tester.add(Point{1, 1}, "valid_point", "Should pass");
    tester.add(Point{-1, 1}, "invalid_point", "Should fail").expectFail("invalid_point");

    // 2. 添加Point数组
    std::vector<Point> points = {{2, 2}, {3, 3}, {4, 4}};
    tester.add(points, "point");

    // 3. 添加TestItem数组
    std::vector<RySimpleRangeTester<Point>::TestItem> items;
    items.push_back({Point{5, 5}, "point5", "Valid point at (5,5)"});
    items.push_back({Point{-5, 5}, "point_neg", "Invalid point", true});
    tester.add(items);

    tester.forEach([](const Point& p) {
        return p.isValid();
    });

    bool allPassed = tester.run();
    std::cout << "Custom type tests " << (allPassed ? "PASSED" : "FAILED") << "\n";
}

// 字符串特化测试
void testStringSpecialization()
{
    RySimpleRangeTester<std::string> tester;

    // 1. 测试不同的字符串添加方式
    tester.add("hello");              // 直接字符串字面量
    tester.add(std::string("world")); // std::string

    std::string str = "test";
    tester.add(str); // 字符串变量

    // 2. 测试字符串容器
    std::vector<std::string> strings = {"one", "two", "three"};
    tester.add(strings);

    // 3. 测试带有特殊字符的字符串
    tester.add("测试中文", "chinese", "Chinese characters test");
    tester.add("!@#$%^", "special", "Special characters test");

    tester.forEach([](const std::string& s) {
        return !s.empty(); // 检查字符串非空
    });

    bool allPassed = tester.run();
    std::cout << "String specialization tests " << (allPassed ? "PASSED" : "FAILED") << "\n";
}

void testRySimpleRangeTester()
{
    try {
        std::cout << "Running RySimpleRangeTester tests...\n\n";

        testBasicUsage();
        testContainerAddition();
        testFailureScenarios();
        testCustomTypes();
        testStringSpecialization();

        std::cout << "\nAll test suites completed.\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}
