#ifndef INC_CHECK_BDJ
#define INC_CHECK_BDJ

#include <check.h>

#if CHECK_MINOR_VERSION < 11
# define ck_assert_ptr_nonnull(ptr) ck_assert_ptr_ne(ptr,NULL)
# define ck_assert_ptr_null(ptr) ck_assert_ptr_eq(ptr,NULL)
# define ck_assert_mem_eq(ptr,val,len) ck_assert_str_eq(ptr,val)
# define ck_assert_float_eq(f,val) ck_assert_int_eq((long)(f*1000.0),(long)(val*1000.0))
#endif

/* libcommon */
Suite *     bdjstring_suite (void);
Suite *     datafile_suite (void);
Suite *     dirop_suite (void);
Suite *     filedata_suite (void);
Suite *     filemanip_suite (void);
Suite *     fileop_suite (void);
Suite *     ilist_suite (void);
Suite *     lock_suite (void);
Suite *     nlist_suite (void);
Suite *     pathutil_suite (void);
Suite *     procutil_suite (void);
Suite *     queue_suite (void);
Suite *     rafile_suite (void);
Suite *     slist_suite (void);
Suite *     sock_suite (void);
Suite *     tmutil_suite (void);

/* libbdj4 */
Suite *     orgutil_suite (void);
Suite *     validate_suite (void);

#endif /* INC_CHECK_BDJ */
