// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/CurrentThread.h"

#include <cxxabi.h>
#include <execinfo.h>   // 进程信息； backtrace|backtrace_symbols
#include <stdlib.h>

namespace muduo
{
namespace CurrentThread       // 当前线程命名空间
{
__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char* t_threadName = "unknown";

static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");

/**
 * 栈跟踪
 * @param  demangle [description]
 * @return          [description]
 */
string stackTrace(bool demangle)
{
  string stack;
  const int max_frames = 200;  /// 最大200个堆栈
  void* frame[max_frames];     /// 

  /**
   * linux backtrace 详解
   * 来源： https://www.cnblogs.com/muahao/p/7610645.html
   * 
   */
  int nptrs = ::backtrace(frame, max_frames);
  char** strings = ::backtrace_symbols(frame, nptrs);

  // 如果存在
  if (strings)
  {
    size_t len = 256;

    /// 如果true, 分配256 char*
    char* demangled = demangle ? static_cast<char*>(::malloc(len)) : nullptr;

    for (int i = 1; i < nptrs; ++i)  // skipping the 0-th, which is this function
    {
      if (demangle)
      {
        // https://panthema.net/2008/0901-stacktrace-demangled/
        // bin/exception_test(_ZN3Bar4testEv+0x79) [0x401909]
        char* left_par = nullptr;
        char* plus = nullptr;
        for (char* p = strings[i]; *p; ++p)
        {
          /*
            ./bt(myFunc3+0x4f) [0x400d88]
            将 0x4f 解析出为有意义的字符 append 

           */
          if (*p == '(')
            left_par = p;
          else if (*p == '+')
            plus = p;
        }

        if (left_par && plus)
        {
          *plus = '\0';
          int status = 0;
          /*
          00146    *  @brief Demangling routine.
00147    *  ABI-mandated entry point in the C++ runtime library for demangling.
00148    *
00149    *  @param __mangled_name A NUL-terminated character string
00150    *  containing the name to be demangled.
00151    *
00152    *  @param __output_buffer A region of memory, allocated with
00153    *  malloc, of @a *__length bytes, into which the demangled name is
00154    *  stored.  If @a __output_buffer is not long enough, it is
00155    *  expanded using realloc.  @a __output_buffer may instead be NULL;
00156    *  in that case, the demangled name is placed in a region of memory
00157    *  allocated with malloc.
00158    *
00159    *  @param __length If @a __length is non-NULL, the length of the
00160    *  buffer containing the demangled name is placed in @a *__length.
00161    *
00162    *  @param __status @a *__status is set to one of the following values:
00163    *   0: The demangling operation succeeded.
00164    *  -1: A memory allocation failure occurred.
00165    *  -2: @a mangled_name is not a valid name under the C++ ABI mangling rules.
00166    *  -3: One of the arguments is invalid.
00167    *
00168    *  @return A pointer to the start of the NUL-terminated demangled
00169    *  name, or NULL if the demangling fails.  The caller is
00170    *  responsible for deallocating this memory using @c free.
00171    *
00172    *  The demangling is performed using the C++ ABI mangling rules,
00173    *  with GNU extensions. For example, this function is used in
00174    *  __gnu_cxx::__verbose_terminate_handler.
00175    *
00176    *  See http://gcc.gnu.org/onlinedocs/libstdc++/manual/bk01pt12ch39.html
00177    *  for other examples of use.
00178    *
00179    *  @note The same demangling functionality is available via
00180    *  libiberty (@c <libiberty/demangle.h> and @c libiberty.a) in GCC
00181    *  3.1 and later, but that requires explicit installation (@c
00182    *  --enable-install-libiberty) and uses a different API, although
00183    *  the ABI is unchanged.
00184    */
           */
          /**
           * https://gcc.gnu.org/onlinedocs/libstdc++/libstdc++-api-4.6/a00851_source.html
           */
          char* ret = abi::__cxa_demangle(left_par+1, demangled, &len, &status);
          *plus = '+';
          if (status == 0)
          {
            demangled = ret;  // ret could be realloc()
            stack.append(strings[i], left_par+1);
            stack.append(demangled);
            stack.append(plus);
            stack.push_back('\n');
            continue; /// 不会再走下边的循环体
          }
        }
      }
      // Fallback to mangled names
      // 字符串追加
      stack.append(strings[i]);
      // 放入一个换行符号
      stack.push_back('\n');
    }

    // 是否存在空指针杀死
    free(demangled);
    free(strings);
  }
  return stack;
}

}  // namespace CurrentThread
}  // namespace muduo
