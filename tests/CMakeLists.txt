enable_testing()

# Unit + Component Tests (single executable)
add_subdirectory(tst_unit)

# Context Isolation Tests (Voice Bank Scoped Package)
add_subdirectory(tst_context)

# Integration Tests (requires runtime plugins and packages)
add_subdirectory(tst_langCore)
