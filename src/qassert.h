#ifndef QASSERT_H
#define QASSERT_H


#define DEBUG_ASSERT

#ifdef DEBUG_ASSERT

#define QAssert(x) \
   if (!(x)) \
   { \
      printf("-- %s: Assert: '%s' (%s, line %i)", \
         __FUNCTION__,#x,__FILE__,__LINE__); \
      (*(int *)-1)=0; \
      Abort(__FUNCTION__,"Assert: '%s' (%s, line %i)", \
         #x,__FILE__,__LINE__); \
   }

#else

#define QAssert(x)

#endif


#endif

