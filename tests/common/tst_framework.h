#ifndef TST_FRAMEWORK_H
#define TST_FRAMEWORK_H

#include <functional>
#include <iostream>
#include <string>
#include <vector>

struct TestEntry {
    std::string name;
    void (*fn)();
};

inline std::vector<TestEntry> &testRegistry() {
    static std::vector<TestEntry> tests;
    return tests;
}

inline int g_passed = 0;
inline int g_failed = 0;

#define TEST_CASE(name)                                                     \
    static void test_##name();                                              \
    namespace {                                                             \
        struct Reg_##name {                                                 \
            Reg_##name() { testRegistry().push_back({#name, test_##name}); }\
        } g_reg_##name;                                                     \
    }                                                                       \
    static void test_##name()

#define ASSERT_TRUE(expr)                                                   \
    do {                                                                    \
        if (!(expr)) {                                                      \
            std::cerr << "  FAIL: " << #expr                                \
                      << "\n    at " << __FILE__ << ":" << __LINE__         \
                      << std::endl;                                         \
            g_failed++;                                                     \
            return;                                                         \
        }                                                                   \
    } while (0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))
#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))
#define ASSERT_NE(a, b) ASSERT_TRUE((a) != (b))
#define ASSERT_STREQ(a, b) ASSERT_TRUE(std::string(a) == std::string(b))
#define ASSERT_GT(a, b) ASSERT_TRUE((a) > (b))
#define ASSERT_GE(a, b) ASSERT_TRUE((a) >= (b))
#define ASSERT_LT(a, b) ASSERT_TRUE((a) < (b))
#define ASSERT_LE(a, b) ASSERT_TRUE((a) <= (b))

inline int runAllTests() {
    for (auto &[name, fn] : testRegistry()) {
        std::cout << "Running: " << name << "..." << std::endl;
        fn();
        g_passed++;
    }
    int total = g_passed + g_failed;
    std::cout << "\n========================================" << std::endl;
    std::cout << total << " tests, " << g_passed << " passed, "
              << g_failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;
    return g_failed > 0 ? 1 : 0;
}

#endif // TST_FRAMEWORK_H