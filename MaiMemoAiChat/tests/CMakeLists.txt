# 领域层单元测试：每个测试类一个可执行文件，交给 CTest 跑。
find_package(Qt6 REQUIRED COMPONENTS Test)

foreach(test_name IN ITEMS tst_ExpressionUnit tst_ValueTypes tst_Vocabulary tst_LearningStates)
    qt_add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE mai_domain Qt6::Test)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
