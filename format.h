
#ifndef FORMAT_SHORT_H  // ��ֹͷ�ļ��ظ���������Ҫ��
#define FORMAT_SHORT_H

#include <string>
#include <fmt/format.h>  // �������� fmt ��Ļ������ͣ��� string_view��memory_buffer��

namespace fmt {
  /**
   * ���ٸ�ʽ�����ַ���������ʹ��ջ�ϻ��������ٶѷ���
   * @param format_str ��ʽ���ַ���
   * @param args �ɱ�����б�
   * @return ��ʽ������ַ���
   */
  template <typename... Args>
  std::string format_short(string_view format_str, const Args&... args) {
    constexpr size_t STACK_BUFFER_SIZE = 256;  // ջ��������С
    char stack_buffer[STACK_BUFFER_SIZE];      // ջ�Ϸ��仺����
    memory_buffer buf(stack_buffer, stack_buffer + STACK_BUFFER_SIZE);

    try {
      // ���� fmt ��� vformat_to ���и�ʽ��
      vformat_to(buf, format_str, make_format_args(args...));
      return std::string(buf.data(), buf.size());
    } catch (const buffer_overflow&) {
      // ����������ʱ������Ϊ��̬����
      return format(format_str, args...);
    }
  }
}

#endif  // FORMAT_SHORT_H
