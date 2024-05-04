struct tar_data {
  char *f, *C, *I;
  struct arg_list *T, *X, *xform;
  long strip;
  char *to_command, *owner, *group, *mtime, *mode, *sort;
  struct arg_list *exclude;

  struct double_list *incl, *excl, *seen;
  struct string_list *dirs;
  char *cwd, **xfsed;
  int fd, ouid, ggid, hlc, warn, sparselen, pid, xfpipe[2];
  struct dev_ino archive_di;
  long long *sparse;
  time_t mtt;

  // hardlinks seen so far (hlc many)
  struct {
    char *arg;
    struct dev_ino di;
  } *hlx;

  // Parsed information about a tar header.
  struct tar_header {
    char *name, *link_target, *uname, *gname;
    long long size, ssize;
    uid_t uid;
    gid_t gid;
    mode_t mode;
    time_t mtime;
    dev_t device;
  } hdr;
};

extern union global_union {
	struct tar_data tar;
} this;
