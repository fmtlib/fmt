
#ifndef FORMAT_SHORT_H  // 防止头文件重复包含（重要）
#define FORMAT_SHORT_H

#include <string>
#include <fmt/format.h>  // 假设依赖 fmt 库的基础类型（如 string_view、memory_buffer）

namespace fmt {
  /**
   * 快速格式化短字符串，优先使用栈上缓冲区减少堆分配
   * @param format_str 格式化字符串
   * @param args 可变参数列表
   * @return 格式化后的字符串
   */
  template <typename... Args>
  std::string format_short(string_view format_str, const Args&... args) {
    constexpr size_t STACK_BUFFER_SIZE = 256;  // 栈缓冲区大小
    char stack_buffer[STACK_BUFFER_SIZE];      // 栈上分配缓冲区
    memory_buffer buf(stack_buffer, stack_buffer + STACK_BUFFER_SIZE);

    try {
      // 复用 fmt 库的 vformat_to 进行格式化
      vformat_to(buf, format_str, make_format_args(args...));
      return std::string(buf.data(), buf.size());
    } catch (const buffer_overflow&) {
      // 缓冲区不足时，降级为动态分配
      return format(format_str, args...);
    }
  }
}

#endif  // FORMAT_SHORT_H
