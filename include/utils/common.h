// #define UT

// Weak attribute for mocked functions enabled only for Unit Tests
#ifdef UT
#define WEAK_FOR_UT __attribute__((weak))
#else
#define WEAK_FOR_UT
#endif

// Enable external linkage of normally static functions just for Unit Tests
#ifdef UT
#define STATIC
#else
#define STATIC static
#endif