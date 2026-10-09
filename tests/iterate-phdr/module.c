// SPDX-License-Identifier: Apache-2.0
#ifdef BROKEN
extern int atl_missing_dependency(void);
int atl_fixture(void) { return atl_missing_dependency(); }
#else
int atl_fixture(void) { return 42; }
#endif
