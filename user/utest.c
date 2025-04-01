// #include "user/user.h"

// // sskip 依赖于iswhitespace()输入'\0'返回0
// char *
// sskip_2(const char *start, const char *end)
// {
//   printf("Calling sskip(\"%s\")\n", end);
//   const char *p;

//   p = start;
//   /* 如果c='\0', iswhitespace(c)返回零，
//    * 故 (c)&&iswhitespace(c) 中第一项是多余的*/
//   // 从前往后搜索，直到字符串结尾
//   if (end == 0)
//   {
//     for( ; iswhitespace(*p); p++)
//       printf("skip: \"%s\"\n", p+1);
//   }
//   else  // end != 0
//   {
//     if (start < end)
//     {
//         for( ; (p < end) && iswhitespace(*p); p++)
//         printf("skip: \"%s\"\n", p+1);
//     }
//     else if (start > end)
//     {
//         printf("reverse\n");
//         p = start - 1;
//         for( ; (p >= end) && iswhitespace(*p); p--)
//         printf("skip: \"%s\"\n", p-1);
//         if (p <= end)   p = end;    // 如果全部是空格，p此时等于end-1
//     }
//   }
//   printf("resulting string is \"%s\"\n", p);
//   return (char *)p;
// }

// int
// main(int argc, char *argv[])
// {
//     char *s = "  ";
//     sskip(s, s+strlen(s));
// }
