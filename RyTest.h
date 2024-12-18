/**
 * @author rayzhang
 * @brief 一个简单的测试框架，满足简单的测试需求，方便自定义
 * @date 2024年12月17日
 */
// ry_test.h
#pragma once

#include <source_location>
#include <chrono>
#include <format>
#include <vector>
#include <functional>
#include <iostream>
#include <string_view>
#include <unordered_set>
#include <optional>
#include <sstream>
#include <regex>
#include <cstdlib>

#if defined(_WIN32)
    #include <windows.h>
#endif

namespace RyTest {

// 控制台颜色处理
class ConsoleColor {
public:
    enum class Color {
        Red,
        Green,
        Yellow,
        Blue,
        Default
    };

    static void setColor(Color color) {
#if defined(_WIN32)
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        WORD attribute;
        switch (color) {
            case Color::Red:     attribute = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
            case Color::Green:   attribute = FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
            case Color::Yellow:  attribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
            case Color::Blue:    attribute = FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
            default:            attribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        }
        SetConsoleTextAttribute(hConsole, attribute);
#else
        const char* colorCode;
        switch (color) {
            case Color::Red:     colorCode = "\033[1;31m"; break;
            case Color::Green:   colorCode = "\033[1;32m"; break;
            case Color::Yellow:  colorCode = "\033[1;33m"; break;
            case Color::Blue:    colorCode = "\033[1;34m"; break;
            default:            colorCode = "\033[0m"; break;
        }
        std::cout << colorCode;
#endif
    }

    static void resetColor() {
#if defined(_WIN32)
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
        std::cout << "\033[0m";
#endif
    }
};

// 测试用例状态
enum class TestStatus {
    Enabled,
    Disabled,
    Skip
};

// 测试结果
struct TestResult {
    bool success{false};
    std::string message;
    std::chrono::milliseconds duration{0};
};

// 测试用例信息
struct TestCase {
    std::string_view suiteName;
    std::string_view caseName;
    std::function<void()> testFunc;
    std::function<bool()> validateFunc;
    TestStatus status{TestStatus::Enabled};
    std::string skipReason;
};

// 测试统计
struct TestStatistics {
    size_t total{0};
    size_t passed{0};
    size_t failed{0};
    size_t skipped{0};
    std::chrono::milliseconds totalTime{0};
};

// 测试过滤器
class TestFilter {
public:
    static TestFilter& instance() {
        static TestFilter filter;
        return filter;
    }

    void initializeFromEnv() {
        if (const char* disabled = std::getenv("RYTEST_DISABLED_TESTS")) {
            std::string disabledTests(disabled);
            std::istringstream ss(disabledTests);
            std::string test;
            while (std::getline(ss, test, ',')) {
                m_disabledTests.insert(test);
            }
        }

        if (const char* pattern = std::getenv("RYTEST_FILTER")) {
            m_filterPattern = pattern;
        }
    }

    [[nodiscard]] bool shouldRun(const std::string& fullTestName) const {
        if (m_disabledTests.contains(fullTestName)) {
            return false;
        }

        if (m_filterPattern && !matchesPattern(fullTestName, *m_filterPattern)) {
            return false;
        }

        return true;
    }

private:
    std::unordered_set<std::string> m_disabledTests;
    std::optional<std::string> m_filterPattern;

    static bool matchesPattern(const std::string& testName, const std::string& pattern) {
        try {
            std::string regexPattern = pattern;
            // 将通配符转换为正则表达式
            std::replace(regexPattern.begin(), regexPattern.end(), '*', '.');
            std::regex re(regexPattern);
            return std::regex_match(testName, re);
        }
        catch (...) {
            return false;
        }
    }
};

// 测试注册表
class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry registry;
        return registry;
    }

    void addTest(TestCase test) {
        m_tests.push_back(std::move(test));
    }

    int runAllTests() {
        TestFilter::instance().initializeFromEnv();
        
        TestStatistics stats;
        auto startTime = std::chrono::steady_clock::now();

        // 计算要运行的测试数量
        for (const auto& test : m_tests) {
            std::string fullName = std::format("{}.{}", test.suiteName, test.caseName);
            if (test.status != TestStatus::Disabled && 
                TestFilter::instance().shouldRun(fullName)) {
                ++stats.total;
            }
        }

        ConsoleColor::setColor(ConsoleColor::Color::Blue);
        std::cout << std::format("\n[==========] Running {} tests\n", stats.total);
        ConsoleColor::resetColor();

        for (const auto& test : m_tests) {
            std::string fullName = std::format("{}.{}", test.suiteName, test.caseName);
            
            if (test.status == TestStatus::Disabled || 
                !TestFilter::instance().shouldRun(fullName)) {
                if (test.status == TestStatus::Skip) {
                    ConsoleColor::setColor(ConsoleColor::Color::Yellow);
                    std::cout << std::format("[  SKIPPED ] {}: {}\n", 
                        fullName, test.skipReason);
                    ConsoleColor::resetColor();
                    ++stats.skipped;
                }
                continue;
            }

            ConsoleColor::setColor(ConsoleColor::Color::Blue);
            std::cout << std::format("\n[ RUN      ] {}\n", fullName);
            ConsoleColor::resetColor();

            auto testStart = std::chrono::steady_clock::now();
            bool success = true;
            std::string errorMsg;

            try {
                if (test.validateFunc && !test.validateFunc()) {
                    throw std::runtime_error("Validation failed");
                }
                test.testFunc();
            }
            catch (const std::exception& e) {
                success = false;
                errorMsg = e.what();
            }
            catch (...) {
                success = false;
                errorMsg = "Unknown error";
            }

            auto testEnd = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                testEnd - testStart);

            if (success) {
                ConsoleColor::setColor(ConsoleColor::Color::Green);
                std::cout << std::format("[       OK ] {} ({} ms)\n",
                    fullName, duration.count());
                ConsoleColor::resetColor();
                ++stats.passed;
            }
            else {
                ConsoleColor::setColor(ConsoleColor::Color::Red);
                std::cout << std::format("[  FAILED  ] {}\n", fullName);
                if (!errorMsg.empty()) {
                    std::cout << "Error: " << errorMsg << "\n";
                }
                ConsoleColor::resetColor();
                ++stats.failed;
            }
        }

        auto endTime = std::chrono::steady_clock::now();
        stats.totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime);

        printSummary(stats);
        
        return stats.failed > 0 ? 1 : 0;
    }

private:
    std::vector<TestCase> m_tests;

    static void printSummary(const TestStatistics& stats) {
        ConsoleColor::setColor(ConsoleColor::Color::Blue);
        std::cout << "\n[==========] "
                  << stats.total << " tests ran. ("
                  << stats.totalTime.count() << " ms total)\n";
        
        if (stats.passed > 0) {
            ConsoleColor::setColor(ConsoleColor::Color::Green);
            std::cout << std::format("[  PASSED  ] {} tests.\n", stats.passed);
        }

        if (stats.failed > 0) {
            ConsoleColor::setColor(ConsoleColor::Color::Red);
            std::cout << std::format("[  FAILED  ] {} tests.\n", stats.failed);
        }

        if (stats.skipped > 0) {
            ConsoleColor::setColor(ConsoleColor::Color::Yellow);
            std::cout << std::format("[  SKIPPED ] {} tests.\n", stats.skipped);
        }
        
        ConsoleColor::resetColor();
    }
};

} // namespace RyTest

// 测试断言宏
#define RY_EXPECT_TRUE(condition) \
    if (!(condition)) { \
        throw std::runtime_error(std::format("Expected true: {}", #condition)); \
    }

#define RY_EXPECT_FALSE(condition) \
    if (condition) { \
        throw std::runtime_error(std::format("Expected false: {}", #condition)); \
    }

#define RY_EXPECT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        throw std::runtime_error(std::format("Expected {} == {}", #expected, #actual)); \
    }

#define RY_EXPECT_NE(expected, actual) \
    if ((expected) == (actual)) { \
        throw std::runtime_error(std::format("Expected {} != {}", #expected, #actual)); \
    }

#define RY_EXPECT_GT(a, b) \
    if (!((a) > (b))) { \
        throw std::runtime_error(std::format("Expected {} > {}", #a, #b)); \
    }

#define RY_EXPECT_GE(a, b) \
    if (!((a) >= (b))) { \
        throw std::runtime_error(std::format("Expected {} >= {}", #a, #b)); \
    }

#define RY_EXPECT_LT(a, b) \
    if (!((a) < (b))) { \
        throw std::runtime_error(std::format("Expected {} < {}", #a, #b)); \
    }

#define RY_EXPECT_LE(a, b) \
    if (!((a) <= (b))) { \
        throw std::runtime_error(std::format("Expected {} <= {}", #a, #b)); \
    }

#define RY_EXPECT_THROW(statement, exception) \
    try { \
        statement; \
        throw std::runtime_error(std::format("Expected {} to throw {}", #statement, #exception)); \
    } catch (const exception&) {}

// 测试套件和用例定义宏
#define RY_TEST_SUITE(name) constexpr std::string_view currentTestSuite = #name

#define RY_TEST(name) RY_TEST_STATUS(name, RyTest::TestStatus::Enabled, "")

#define RY_DISABLED_TEST(name) RY_TEST_STATUS(name, RyTest::TestStatus::Disabled, "")

#define RY_SKIP_TEST(name, reason) RY_TEST_STATUS(name, RyTest::TestStatus::Skip, reason)

#define RY_TEST_STATUS(name, status, reason) \
    static void TestFunc_##name(); \
    static struct TestRegister_##name { \
        TestRegister_##name() { \
            RyTest::TestRegistry::instance().addTest({ \
                currentTestSuite, \
                #name, \
                TestFunc_##name, \
                nullptr, \
                status, \
                reason \
            }); \
        } \
    } TestRegister_##name##_instance; \
    static void TestFunc_##name()

#define RY_TEST_WITH_VALIDATE(name, validateFunc) \
    static void TestFunc_##name(); \
    static struct TestRegister_##name { \
        TestRegister_##name() { \
            RyTest::TestRegistry::instance().addTest({ \
                currentTestSuite, \
                #name, \
                TestFunc_##name, \
                validateFunc, \
                RyTest::TestStatus::Enabled, \
                "" \
            }); \
        } \
    } TestRegister_##name##_instance; \
    static void TestFunc_##name()

// 运行所有测试的宏
#define RY_RUN_ALL_TESTS() \
    return RyTest::TestRegistry::instance().runAllTests()

