#pragma once

namespace mai::domain {

/// 评分：三档、固定，表达回忆质量（R7）。
/// 不落本地盘（R22/R24），故不定义序列化词汇；档位到引擎按钮的对应属适配映射（R33）。
enum class Rating { Forgot, Hard, Good };

} // namespace mai::domain
