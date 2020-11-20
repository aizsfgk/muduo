#ifndef MUDUO_BASE_NONCOPYABLE_H
#define MUDUO_BASE_NONCOPYABLE_H

namespace muduo
{

class noncopyable
{
 public:
  noncopyable(const noncopyable&) = delete;    // 1. 禁止拷贝构造
  void operator=(const noncopyable&) = delete; // 2. 禁止拷贝赋值

 protected:
  noncopyable() = default;     // 使用默认的构造函数
  ~noncopyable() = default;    // 使用默认的析构函数
};

}  // namespace muduo

#endif  // MUDUO_BASE_NONCOPYABLE_H
