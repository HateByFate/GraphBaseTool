#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>

class TestFramework {
public:
    static TestFramework& getInstance() {
        static TestFramework instance;
        return instance;
    }

    void addTest(const std::string& name, std::function<void()> test) {
        tests.push_back({name, test});
    }

    bool runTests() {
        int passed = 0;
        int failed = 0;

        std::cout << "Running tests...\n\n";

        for (const auto& test : tests) {
            std::cout << "Test: " << test.name << " ... ";
            try {
                test.func();
                std::cout << "PASSED\n";
                passed++;
            } catch (const std::exception& e) {
                std::cout << "FAILED\n";
                std::cout << "Error: " << e.what() << "\n";
                failed++;
            }
        }

        std::cout << "\nTest Summary:\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "Total: " << (passed + failed) << "\n";

        return failed == 0;
    }

private:
    struct Test {
        std::string name;
        std::function<void()> func;
    };

    std::vector<Test> tests;
};

#define TEST(name) \
    void test_##name(); \
    namespace { \
        struct test_##name##_registrar { \
            test_##name##_registrar() { \
                TestFramework::getInstance().addTest(#name, test_##name); \
            } \
        } test_##name##_instance; \
    } \
    void test_##name()

#define ASSERT(condition) \
    if (!(condition)) { \
        throw std::runtime_error("Assertion failed: " #condition); \
    }

#define ASSERT_EQUAL(expected, actual) \
    if ((expected) != (actual)) { \
        throw std::runtime_error("Assertion failed: expected " + std::to_string(expected) + \
                               ", got " + std::to_string(actual)); \
    }

#define ASSERT_THROWS(expression) \
    try { \
        expression; \
        throw std::runtime_error("Expected exception was not thrown"); \
    } catch (...) { \
        /* Exception was thrown as expected */ \
    } 
