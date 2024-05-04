// tar &(one-file-system)(no-ignore-case)(ignore-case)(no-anchored)(anchored)(no-wildcards)(wildcards)(no-wildcards-match-slash)(wildcards-match-slash)(show-transformed-names)(selinux)(restrict)(full-time)(no-recursion)(null)(numeric-owner)(no-same-permissions)(overwrite)(exclude)*(sort);:(mode):(mtime):(group):(owner):(to-command):~(strip-components)(strip)#~(transform)(xform)*o(no-same-owner)p(same-permissions)k(keep-old)c(create)|h(dereference)x(extract)|t(list)|v(verbose)J(xz)j(bzip2)z(gzip)S(sparse)O(to-stdout)P(absolute-names)m(touch)X(exclude-from)*T(files-from)*I(use-compress-program):C(directory):f(file):as[!txc][!jzJa] &(one-file-system)(no-ignore-case)(ignore-case)(no-anchored)(anchored)(no-wildcards)(wildcards)(no-wildcards-match-slash)(wildcards-match-slash)(show-transformed-names)(selinux)(restrict)(full-time)(no-recursion)(null)(numeric-owner)(no-same-permissions)(overwrite)(exclude)*(sort);:(mode):(mtime):(group):(owner):(to-command):~(strip-components)(strip)#~(transform)(xform)*o(no-same-owner)p(same-permissions)k(keep-old)c(create)|h(dereference)x(extract)|t(list)|v(verbose)J(xz)j(bzip2)z(gzip)S(sparse)O(to-stdout)P(absolute-names)m(touch)X(exclude-from)*T(files-from)*I(use-compress-program):C(directory):f(file):as[!txc][!jzJa]
#undef OPTSTR_tar
#define OPTSTR_tar "&(one-file-system)(no-ignore-case)(ignore-case)(no-anchored)(anchored)(no-wildcards)(wildcards)(no-wildcards-match-slash)(wildcards-match-slash)(show-transformed-names)(selinux)(restrict)(full-time)(no-recursion)(null)(numeric-owner)(no-same-permissions)(overwrite)(exclude)*(sort);:(mode):(mtime):(group):(owner):(to-command):~(strip-components)(strip)#~(transform)(xform)*o(no-same-owner)p(same-permissions)k(keep-old)c(create)|h(dereference)x(extract)|t(list)|v(verbose)J(xz)j(bzip2)z(gzip)S(sparse)O(to-stdout)P(absolute-names)m(touch)X(exclude-from)*T(files-from)*I(use-compress-program):C(directory):f(file):as[!txc][!jzJa]"
#ifdef CLEANUP_tar
#undef CLEANUP_tar
#undef FOR_tar
#undef FLAG_s
#undef FLAG_a
#undef FLAG_f
#undef FLAG_C
#undef FLAG_I
#undef FLAG_T
#undef FLAG_X
#undef FLAG_m
#undef FLAG_P
#undef FLAG_O
#undef FLAG_S
#undef FLAG_z
#undef FLAG_j
#undef FLAG_J
#undef FLAG_v
#undef FLAG_t
#undef FLAG_x
#undef FLAG_h
#undef FLAG_c
#undef FLAG_k
#undef FLAG_p
#undef FLAG_o
#undef FLAG_xform
#undef FLAG_strip
#undef FLAG_to_command
#undef FLAG_owner
#undef FLAG_group
#undef FLAG_mtime
#undef FLAG_mode
#undef FLAG_sort
#undef FLAG_exclude
#undef FLAG_overwrite
#undef FLAG_no_same_permissions
#undef FLAG_numeric_owner
#undef FLAG_null
#undef FLAG_no_recursion
#undef FLAG_full_time
#undef FLAG_restrict
#undef FLAG_selinux
#undef FLAG_show_transformed_names
#undef FLAG_wildcards_match_slash
#undef FLAG_no_wildcards_match_slash
#undef FLAG_wildcards
#undef FLAG_no_wildcards
#undef FLAG_anchored
#undef FLAG_no_anchored
#undef FLAG_ignore_case
#undef FLAG_no_ignore_case
#undef FLAG_one_file_system
#endif

#ifdef FOR_tar
#define CLEANUP_tar
#ifndef TT
#define TT this.tar
#endif
#define FLAG_s (1LL<<0)
#define FLAG_a (1LL<<1)
#define FLAG_f (1LL<<2)
#define FLAG_C (1LL<<3)
#define FLAG_I (1LL<<4)
#define FLAG_T (1LL<<5)
#define FLAG_X (1LL<<6)
#define FLAG_m (1LL<<7)
#define FLAG_P (1LL<<8)
#define FLAG_O (1LL<<9)
#define FLAG_S (1LL<<10)
#define FLAG_z (1LL<<11)
#define FLAG_j (1LL<<12)
#define FLAG_J (1LL<<13)
#define FLAG_v (1LL<<14)
#define FLAG_t (1LL<<15)
#define FLAG_x (1LL<<16)
#define FLAG_h (1LL<<17)
#define FLAG_c (1LL<<18)
#define FLAG_k (1LL<<19)
#define FLAG_p (1LL<<20)
#define FLAG_o (1LL<<21)
#define FLAG_xform (1LL<<22)
#define FLAG_strip (1LL<<23)
#define FLAG_to_command (1LL<<24)
#define FLAG_owner (1LL<<25)
#define FLAG_group (1LL<<26)
#define FLAG_mtime (1LL<<27)
#define FLAG_mode (1LL<<28)
#define FLAG_sort (1LL<<29)
#define FLAG_exclude (1LL<<30)
#define FLAG_overwrite (1LL<<31)
#define FLAG_no_same_permissions (1LL<<32)
#define FLAG_numeric_owner (1LL<<33)
#define FLAG_null (1LL<<34)
#define FLAG_no_recursion (1LL<<35)
#define FLAG_full_time (1LL<<36)
#define FLAG_restrict (1LL<<37)
#define FLAG_selinux (1LL<<38)
#define FLAG_show_transformed_names (1LL<<39)
#define FLAG_wildcards_match_slash (1LL<<40)
#define FLAG_no_wildcards_match_slash (1LL<<41)
#define FLAG_wildcards (1LL<<42)
#define FLAG_no_wildcards (1LL<<43)
#define FLAG_anchored (1LL<<44)
#define FLAG_no_anchored (1LL<<45)
#define FLAG_ignore_case (1LL<<46)
#define FLAG_no_ignore_case (1LL<<47)
#define FLAG_one_file_system (1LL<<48)
#endif
