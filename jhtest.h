// Tiny unit testing macros using GNU extension constructors reserving
// priority numbers 500, 501, and 502 for internal usage.
// Single-header lib with stb-style JHTEST_IMPLEMENTATION macro
// ...JHTEST     : function decorator for testing (501 priority)
// ...jhtest()   : macro with different behavior in JHTEST and runtime
// ...NDEBUG     : preprocessor flag to disable tests (-D)
#ifndef JH_TESTING_MACROS_H
#define JH_TESTING_MACROS_H

#ifndef __GNUC__
#error jhtest: Compile on GNU compatible compiler for [[gnu]] extensions
#endif

#if __STDC_VERSION__ < 202311L
#error jhtest: Compile on at least gnu23
#endif

#ifndef NDEBUG
#define JHTEST [[gnu::constructor(501)]]
#define jhtest(expr)\
  ({\
    if (impl_jhtest_is_prio_501_)\
      {\
        impl_jhtest_push_(__FUNCTION__);\
        impl_jhtest_inside_((bool)(expr), #expr, __FUNCTION__, __FILE__, __LINE__);\
      }\
    else\
      {\
        impl_jhtest_outside_((bool)(expr), #expr, __FUNCTION__, __FILE__, __LINE__);\
      }\
  })
extern bool impl_jhtest_is_prio_501_;
extern void impl_jhtest_push_(const char *current_function);
extern void impl_jhtest_inside_(bool condition_result, const char *condition_str, const char *current_function, const char *filename, int line_number);
extern void impl_jhtest_outside_(bool condition_result, const char *condition_str, const char *current_function, const char *filename, int line_number);
#else
#define JHTEST [[unused]]
#define jhtest(expr) (void)0
#endif

#endif // ifndef JH_TESTING_MACROS_H
#if defined JHTEST_IMPLEMENTATION && !defined NDEBUG

#include <stddef.h> // size_t
#include <stdlib.h> // exit() and abort()
#include <stdio.h>  // printing and FILE

// True iff running under priority 501 constructor (JTEST).
bool impl_jhtest_is_prio_501_ = false;

// This whole struct is unused outside of JHTEST
static struct StaticState
{
  // Stack depth for jhtest() calling a jhtest(). If 0 it's the first
  size_t test_depth;

  // Constantly increment per function for the priority 502 report
  size_t test_count;
  size_t fail_count;
  
  // (Assumes different JHTEST functions have different names.)
  bool is_new_function;

  // We use pointer comparison, not strcmp, so null is an okay value
  const char *last_function;

  // Could've been hardcoded to stderr but you may need fopen and fclose
  FILE *logfile;
} ss;

[[gnu::constructor(500)]] static
void initialize_jhtest()
{
  ss.logfile = stderr;
  impl_jhtest_is_prio_501_ = true;
}

[[gnu::constructor(502)]] static
void report_on_jhtest()
{
  impl_jhtest_is_prio_501_ = false;
  
  if (ss.test_count > 0)
    {
      bool failed = ss.fail_count > 0;

      // User may not want the program to crash if the tests fail.
      // We pass the file name and line number for quick edit.
      fprintf(ss.logfile, "%s:%d: %s: %zu failed; %zu passed; %zu total\n",
              __FILE__, __LINE__, failed ? "error" : "info",
              ss.fail_count, ss.test_count - ss.fail_count, ss.test_count);
      if (failed)
        {
          exit(EXIT_FAILURE);
        }
    }
  else
    {
      fprintf(ss.logfile, "%s:%d: info: no tests\n", __FILE__, __LINE__);
    }

  ss = (struct StaticState){0};
}

void impl_jhtest_push_(const char *current_function)
{
  if (ss.test_depth == 0)
    {
      if (current_function != ss.last_function)
        {
          ss.is_new_function = true;
          ss.last_function = current_function;
          ++ss.test_count;
        }
    }
  ++ss.test_depth;
}

void impl_jhtest_outside_(bool condition_result,
                          const char *condition_str,
                          const char *function_name,
                          const char *filename,
                          int line_number)
{
  if (condition_result == false)
    {
      fprintf(stderr, "jhtest: In function `%s':\n", function_name);
      fprintf(stderr, "%s:%d: error: %s\n", filename, line_number, condition_str);
      abort();
    }
}

void impl_jhtest_inside_(bool condition_result,
                         const char *condition_str,
                         const char *current_function,
                         const char *filename,
                         int line_number)
{
  --ss.test_depth;
  if (condition_result == false)
    {
      if (ss.is_new_function)
        {
          fprintf(ss.logfile, "jhtest: In function `%s':\n", current_function);
          ++ss.fail_count;
          ss.is_new_function = false;
        }
      fprintf(ss.logfile, "%s:%d: warning: %s\n", filename, line_number, condition_str);
    }
}

#endif // if defined JHTEST_IMPLEMENTATION && !defined NDEBUG
