enable_testing()

# Unit + Component Tests (single executable)
add_subdirectory(tst_unit)

# Integration Tests (requires runtime plugins and packages)
add_subdirectory(tst_langCore)
