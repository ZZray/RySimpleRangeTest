/**
 * @file RySimpleRangeTester.h
 * @author rayzhang
 * @brief 一个简单的范围测试框架，满足简单的测试需求
 * @date 2024年12月18日
 *
 * 特点:
 * - 链式调用
 * - 预期失败测试
 * - 详细的测试报告
 * - 性能分析
 * - 彩色输出
 * - 灵活的测试项管理
 */

#pragma once

#include <chrono>
#include <functional>
#include <iostream>
#include <numeric>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <format>
#include <ranges>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

template <typename T>
class RySimpleRangeTester
{
public:
    /**
     * @struct TestItem
     * @brief 代表一个测试项的结构体
     */
    struct TestItem
    {
        T item;                     ///< 测试项数据
        std::string name;           ///< 测试项名称
        std::string description;    ///< 可选的描述信息
        bool expectedToFail{false}; ///< 是否期望失败

        constexpr TestItem() = default;

        /**
         * @brief 构造函数
         * @param value 测试项的值
         * @param itemName 测试项的名称
         * @param desc 测试项的描述
         * @param expectFail 是否期望测试失败
         */
        constexpr TestItem(T value, std::string itemName = "", std::string desc = "", bool expectFail = false)
            : item(std::move(value))
            , name(std::move(itemName))
            , description(std::move(desc))
            , expectedToFail(expectFail)
        { }
    };

    /**
     * @typedef TestCallback
     * @brief 测试回调函数的类型定义
     */
    using TestCallback = std::function<bool(const T&)>;

    /**
     * @struct TestResult
     * @brief 测试结果的结构体
     */
    struct TestResult
    {
        std::string name;                     ///< 测试名称
        bool success{false};                  ///< 测试是否成功
        std::string error;                    ///< 错误信息
        std::chrono::milliseconds duration{}; ///< 测试持续时间
        bool wasExpectedToFail{false};        ///< 是否期望失败
        std::string description;              ///< 测试描述
    };

private:
    std::unordered_map<std::string, TestItem> m_testItems; ///< 存储测试项的映射
    TestCallback m_testCallback;                           ///< 测试回调函数
    std::vector<TestResult> m_results;                     ///< 存储测试结果的向量

#ifdef _WIN32
    HANDLE m_hConsole{GetStdHandle(STD_OUTPUT_HANDLE)}; ///< Windows控制台句柄
    WORD m_originalAttrs{};                             ///< 原始控制台属性
#endif

    /**
     * @brief 设置控制台输出颜色
     * @param color 颜色名称
     */
    void setColor(std::string_view color) const
    {
#ifdef _WIN32
        if (color == "green") {
            SetConsoleTextAttribute(m_hConsole, FOREGROUND_GREEN);
        }
        else if (color == "red") {
            SetConsoleTextAttribute(m_hConsole, FOREGROUND_RED);
        }
        else if (color == "yellow") {
            SetConsoleTextAttribute(m_hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
        }
        else {
            SetConsoleTextAttribute(m_hConsole, m_originalAttrs);
        }
#else
        if (color == "green") {
            std::cout << "\033[32m";
        }
        else if (color == "red") {
            std::cout << "\033[31m";
        }
        else if (color == "yellow") {
            std::cout << "\033[33m";
        }
        else {
            std::cout << "\033[0m";
        }
#endif
    }

    /**
     * @brief 重置控制台输出颜色
     */
    void resetColor() const
    {
#ifdef _WIN32
        SetConsoleTextAttribute(m_hConsole, m_originalAttrs);
#else
        std::cout << "\033[0m";
#endif
    }

    /**
     * @brief 生成唯一名称
     * @param baseName 基础名称
     * @return 生成的唯一名称
     */
    [[nodiscard]] std::string generateUniqueName(std::string_view baseName = "Test") const
    {
        size_t counter = m_testItems.size() + 1;
        std::string name;
        do {
            name = std::format("{}_{}", baseName, counter++);
        }
        while (m_testItems.contains(name));
        return name;
    }

public:
    /**
     * @brief 构造函数
     */
    RySimpleRangeTester()
    {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
        GetConsoleScreenBufferInfo(m_hConsole, &consoleInfo);
        m_originalAttrs = consoleInfo.wAttributes;
#endif
    }

    /**
     * @brief 添加单个测试项
     * @param item 测试项的值
     * @param name 测试项的名称
     * @param description 测试项的描述
     * @return 当前对象的引用，用于链式调用
     */
    RySimpleRangeTester& add(const T& item, std::string_view name = "", std::string_view description = "")
    {
        auto effectiveName = name.empty() ? generateUniqueName() : std::string{name};
        m_testItems.try_emplace(effectiveName,
                                item,                     // 值
                                std::move(effectiveName), // 名称
                                std::string{description}, // 描述
                                false                     // expectedToFail
        );
        return *this;
    }

    /**
     * @brief 添加完整的测试项
     * @param testItem 测试项对象
     * @return 当前对象的引用，用于链式调用
     */
    RySimpleRangeTester& add(TestItem testItem)
    {
        if (testItem.name.empty()) {
            testItem.name = generateUniqueName();
        }
        m_testItems.try_emplace(testItem.name, std::move(testItem));
        return *this;
    }

    /**
     * @brief 从容器中添加测试项
     * @tparam U 容器中元素的类型
     * @tparam Container 容器类型
     * @param items 包含测试项的容器
     * @param namePrefix 名称前缀
     * @return 当前对象的引用，用于链式调用
     */
    template <typename U, template <typename...> typename Container>
    RySimpleRangeTester& add(const Container<U>& items, std::string_view namePrefix = "Test")
    {
        for (const auto& item : items) {
            if constexpr (std::same_as<std::decay_t<U>, T>) {
                // 如果是 T，使用基础的 add
                add(item, generateUniqueName(namePrefix));
            }
            else {
                // 如果是 TestItem，使用 addItem
                add(item);
            }
        }
        return *this;
    }

    /**
     * @brief 从初始化列表中添加测试项
     * @param items 初始化列表
     * @param namePrefix 名称前缀
     * @return 当前对象的引用，用于链式调用
     */
    RySimpleRangeTester& add(std::initializer_list<T> items, std::string_view namePrefix = "Test")
    {
        for (const auto& item : items) {
            add(item, generateUniqueName(namePrefix));
        }
        return *this;
    }

    /**
     * @brief 移除指定名称的测试项
     * @param name 要移除的测试项名称
     * @return 当前对象的引用，用于链式调用
     */
    RySimpleRangeTester& remove(std::string_view name)
    {
        m_testItems.erase(std::string{name});
        return *this;
    }

    /**
     * @brief 根据条件移除测试项
     * @tparam Pred 谓词类型
     * @param pred 移除条件
     * @return 当前对象的引用，用于链式调用
     */
    template <typename Pred>
    RySimpleRangeTester& removeIf(Pred pred)
    {
        std::erase_if(m_testItems, [&](const auto& pair) {
            return pred(pair.second);
        });
        return *this;
    }

    /**
     * @brief 设置测试回调函数
     * @param callback 测试回调函数
     * @return 当前对象的引用，用于链式调用
     */
    RySimpleRangeTester& forEach(TestCallback callback)
    {
        m_testCallback = std::move(callback);
        return *this;
    }

    /**
     * @brief 设置特定测试项期望失败
     * @param name 测试项名称
     * @return 当前对象的引用，用于链式调用
     */
    RySimpleRangeTester& expect(std::string_view name)
    {
        if (auto it = m_testItems.find(std::string{name}); it != m_testItems.end()) {
            it->second.expectedToFail = true;
        }
        return *this;
    }

    /**
     * @brief 根据条件设置测试项期望失败
     * @tparam Pred 谓词类型
     * @param pred 条件
     * @return 当前对象的引用，用于链式调用
     */
    template <typename Pred>
    RySimpleRangeTester& expectIf(Pred pred)
    {
        for (auto& [_, item] : m_testItems) {
            if (pred(item)) {
                item.expectedToFail = true;
            }
        }
        return *this;
    }

    /**
     * @brief 清除所有测试项和结果
     * @return 当前对象的引用，用于链式调用
     */
    RySimpleRangeTester& clear()
    {
        m_testItems.clear();
        m_results.clear();
        return *this;
    }

    /**
     * @brief 获取指定名称的测试项
     * @param name 测试项名称
     * @return 指向测试项的指针，如果不存在则返回nullptr
     */
    [[nodiscard]] const TestItem* getTestItem(std::string_view name) const
    {
        auto it = m_testItems.find(std::string{name});
        return it != m_testItems.end() ? &it->second : nullptr;
    }

    /**
     * @brief 运行所有测试
     * @return 如果所有测试通过则返回true，否则返回false
     */
    bool run()
    {
        runTests();
        return getPassedCount() == m_testItems.size();
    }

    /**
     * @brief 获取通过的测试数量
     * @return 通过的测试数量
     */
    [[nodiscard]] size_t getPassedCount() const { return std::ranges::count_if(m_results, &TestResult::success); }

    /**
     * @brief 获取失败的测试数量
     * @return 失败的测试数量
     */
    [[nodiscard]] size_t getFailedCount() const { return m_results.size() - getPassedCount(); }

    /**
     * @brief 获取通过率
     * @return 测试通过率（百分比）
     */
    [[nodiscard]] double getPassRate() const { return m_results.empty() ? 0.0 : static_cast<double>(getPassedCount()) / m_results.size() * 100.0; }

private:
    /**
     * @brief 运行所有测试
     */
    void runTests()
    {
        if (!m_testCallback) {
            throw std::runtime_error("Test callback not set");
        }

        const auto totalStartTime = std::chrono::steady_clock::now();
        size_t passedTests        = 0;

        std::cout << std::format("\n[==========] Running {} tests\n", m_testItems.size());

        m_results.clear();
        m_results.reserve(m_testItems.size());

        for (const auto& [name, testItem] : m_testItems) {
            std::cout << std::format("\n[ RUN      ] {}", name);
            if (!testItem.description.empty()) {
                std::cout << std::format(" - {}", testItem.description);
            }
            std::cout << '\n';

            const auto testStart = std::chrono::steady_clock::now();
            TestResult result{name, false, "", {}, testItem.expectedToFail, testItem.description};

            try {
                result.success = m_testCallback(testItem.item);
                if (testItem.expectedToFail) {
                    result.success = !result.success;
                }

                if (!result.success) {
                    result.error = testItem.expectedToFail ? "Test unexpectedly passed" : "Test failed";
                }
            }
            catch (const std::exception& e) {
                result.success = false;
                result.error   = e.what();
            }

            result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - testStart);

            if (result.success) {
                setColor("green");
                std::cout << std::format("[       OK ] {} ({} ms)\n", result.name, result.duration.count());
                resetColor();
                ++passedTests;
            }
            else {
                setColor("red");
                std::cout << std::format("[  FAILED  ] {}\n", result.name);
                if (!result.error.empty()) {
                    std::cout << std::format("Error: {}\n", result.error);
                }
                resetColor();
            }

            m_results.push_back(std::move(result));
        }

        const auto totalEndTime  = std::chrono::steady_clock::now();
        const auto totalTime     = std::chrono::duration_cast<std::chrono::milliseconds>(totalEndTime - totalStartTime);
        const auto totalTestTime = std::accumulate(m_results.begin(), m_results.end(), std::chrono::milliseconds(0), [](auto sum, const auto& result) {
            return sum + result.duration;
        });

        printTestReport(passedTests, totalTime, totalTestTime);
    }

    /**
     * @brief 打印测试报告
     * @param passedTests 通过的测试数量
     * @param totalTime 总测试时间
     * @param totalTestTime 实际测试时间
     */
    void printTestReport(size_t passedTests, std::chrono::milliseconds totalTime, std::chrono::milliseconds totalTestTime) const
    {
        constexpr auto separator    = "==========================================";
        constexpr auto subSeparator = "------------------------------------------";

        std::cout << "\n\n" << separator << "\n";
        std::cout << "                 Test Summary\n";
        std::cout << separator << "\n\n";

        // 总体统计
        std::cout << std::format("Total Tests: {}\n", m_testItems.size());
        std::cout << std::format("Total Time: {} ms\n", totalTime.count());
        std::cout << std::format("Total Test Time: {} ms\n", totalTestTime.count());
        std::cout << std::format("Average Time per Test: {} ms\n", m_testItems.size() > 0 ? totalTestTime.count() / m_testItems.size() : 0);
        std::cout << std::format("Overhead Time: {} ms\n", totalTime.count() - totalTestTime.count());

        // 通过的测试
        if (passedTests > 0) {
            setColor("green");
            std::cout << "\n[PASSED TESTS] " << passedTests << " tests\n";
            std::cout << subSeparator << '\n';
            for (const auto& result : m_results) {
                if (result.success) {
                    std::cout << std::format("+ {} ({} ms)", result.name, result.duration.count());
                    if (!result.description.empty()) {
                        std::cout << std::format(" - {}", result.description);
                    }
                    std::cout << '\n';
                }
            }
            resetColor();
        }

        // 失败的测试
        const size_t failedTests = m_testItems.size() - passedTests;
        if (failedTests > 0) {
            setColor("red");
            std::cout << "\n[FAILED TESTS] " << failedTests << " tests\n";
            std::cout << subSeparator << '\n';
            for (const auto& result : m_results) {
                if (!result.success) {
                    std::cout << std::format("x {} ({} ms)", result.name, result.duration.count());
                    if (!result.description.empty()) {
                        std::cout << std::format(" - {}", result.description);
                    }
                    std::cout << '\n';
                    std::cout << std::format("  Error: {}\n", result.error.empty() ? "Unknown error" : result.error);
                }
            }
            resetColor();
        }

        // 性能分析
        if (!m_results.empty()) {
            std::cout << "\n[PERFORMANCE ANALYSIS]\n";
            std::cout << subSeparator << '\n';

            const auto [minIt, maxIt] = std::minmax_element(m_results.begin(), m_results.end(), [](const TestResult& a, const TestResult& b) {
                return a.duration < b.duration;
            });

            setColor("yellow");
            std::cout << std::format("Slowest Test: {} ({} ms)\n", maxIt->name, maxIt->duration.count());
            std::cout << std::format("Fastest Test: {} ({} ms)\n", minIt->name, minIt->duration.count());

            const auto avgTime = totalTestTime.count() / m_results.size();
            std::cout << std::format("Average Time: {} ms\n", avgTime);

            const auto slowTests = std::ranges::to<std::vector>(m_results | std::views::filter([avgTime](const auto& result) {
                                                                    return result.duration.count() > static_cast<long long>(avgTime * 1.5);
                                                                }));

            if (!slowTests.empty()) {
                std::cout << "\nTests Significantly Above Average (>50%):\n";
                for (const auto& result : slowTests) {
                    std::cout << std::format("- {} ({} ms)", result.name, result.duration.count());
                    if (!result.description.empty()) {
                        std::cout << std::format(" - {}", result.description);
                    }
                    std::cout << '\n';
                }
            }
            resetColor();
        }

        std::cout << "\n" << separator << "\n";
        std::cout << "                    End\n";
        std::cout << separator << "\n\n";
    }
};

// template<typename T>
// using RySimpleRangeTestItem = typename RySimpleRangeTester<T>::TestItem;
