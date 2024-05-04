// sed (help)(version)(tarxform)e*f*i:;nErz(null-data)s[+Er] (help)(version)(tarxform)e*f*i:;nErz(null-data)s[+Er]
#undef OPTSTR_sed
#define OPTSTR_sed "(help)(version)(tarxform)e*f*i:;nErz(null-data)s[+Er]"
#ifdef CLEANUP_sed
#undef CLEANUP_sed
#undef FOR_sed
#undef FLAG_s
#undef FLAG_z
#undef FLAG_r
#undef FLAG_E
#undef FLAG_n
#undef FLAG_i
#undef FLAG_f
#undef FLAG_e
#undef FLAG_tarxform
#undef FLAG_version
#undef FLAG_help
#endif

#ifdef FOR_sed
#define CLEANUP_sed
#ifndef TT
#define TT this.sed
#endif
#define FLAG_s (1LL<<0)
#define FLAG_z (1LL<<1)
#define FLAG_r (1LL<<2)
#define FLAG_E (1LL<<3)
#define FLAG_n (1LL<<4)
#define FLAG_i (1LL<<5)
#define FLAG_f (1LL<<6)
#define FLAG_e (1LL<<7)
#define FLAG_tarxform (1LL<<8)
#define FLAG_version (1LL<<9)
#define FLAG_help (1LL<<10)
#endif
