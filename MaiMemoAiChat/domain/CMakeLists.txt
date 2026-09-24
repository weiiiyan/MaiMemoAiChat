# 领域层：产品固有事实与不变量。按依赖规则只链 QtCore，
# 不得依赖 QtGui / QtQml / QtNetwork / QtWidgets，也不被任何外层环反向依赖。
qt_add_library(mai_domain STATIC
    ExpressionUnit.h
    ExpressionUnit.cpp
    LearningState.h
    LearningState.cpp
    Rating.h
    Vocabulary.h
    Vocabulary.cpp
    LearningStates.h
    LearningStates.cpp
)

# include 目录设为项目根，使用方写 #include "domain/ExpressionUnit.h"，
# 引用处一眼能看出符号来自哪一层
target_include_directories(mai_domain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(mai_domain
    PUBLIC
        Qt6::Core
)
