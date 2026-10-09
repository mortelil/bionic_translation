// SPDX-License-Identifier: Apache-2.0
#ifdef CALLER
__attribute__((visibility("default"))) int atl_cfi_invoke(int (*f)(int)) { return f(41); }
#else
__attribute__((visibility("default"))) int atl_cfi_add(int n) { return n+1; }
__attribute__((visibility("default"))) double atl_cfi_wrong(double n) { return n+1; }
#endif
