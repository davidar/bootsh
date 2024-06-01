#include <unistd.h>

#include <libgen.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "sbase-box.h"

typedef int (*main_fn)(int, char **);

struct cmd {
	char *name;
	main_fn fn;
} cmds[] = {
	{"install", xinstall_main},
	{"[", test_main},
	{"basename", basename_main},
	// {"cal", cal_main},
	{"cat", cat_main},
	{"chgrp", chgrp_main},
	{"chmod", chmod_main},
	{"chown", chown_main},
	{"chroot", chroot_main},
	{"cksum", cksum_main},
	{"cmp", cmp_main},
	{"cols", cols_main},
	{"comm", comm_main},
	{"cp", cp_main},
	// {"cron", cron_main},
	// {"cut", cut_main},
	{"date", date_main},
	{"dd", dd_main},
	{"dirname", dirname_main},
	{"du", du_main},
	{"echo", echo_main},
	// {"ed", ed_main},
	{"env", env_main},
	{"expand", expand_main},
	{"expr", expr_main},
	{"false", false_main},
	{"find", find_main},
	// {"flock", flock_main},
	{"fold", fold_main},
	{"getconf", getconf_main},
	// {"grep", grep_main},
	{"head", head_main},
	{"hostname", hostname_main},
	{"join", join_main},
	{"kill", kill_main},
	{"link", link_main},
	{"ln", ln_main},
	{"logger", logger_main},
	// {"logname", logname_main},
	{"ls", ls_main},
	{"md5sum", md5sum_main},
	{"mkdir", mkdir_main},
	{"mkfifo", mkfifo_main},
	{"mknod", mknod_main},
	{"mktemp", mktemp_main},
	{"mv", mv_main},
	{"nice", nice_main},
	{"nl", nl_main},
	{"nohup", nohup_main},
	// {"od", od_main},
	{"paste", paste_main},
	{"pathchk", pathchk_main},
	{"printenv", printenv_main},
	{"printf", printf_main},
	{"pwd", pwd_main},
	{"readlink", readlink_main},
	{"renice", renice_main},
	{"rev", rev_main},
	{"rm", rm_main},
	{"rmdir", rmdir_main},
	// {"sed", sed_main},
	{"seq", seq_main},
	{"setsid", setsid_main},
	{"sha1sum", sha1sum_main},
	{"sha224sum", sha224sum_main},
	{"sha256sum", sha256sum_main},
	{"sha384sum", sha384sum_main},
	{"sha512-224sum", sha512_224sum_main},
	{"sha512-256sum", sha512_256sum_main},
	{"sha512sum", sha512sum_main},
	{"sleep", sleep_main},
	// {"sort", sort_main},
	{"split", split_main},
	{"sponge", sponge_main},
	{"strings", strings_main},
	{"sync", sync_main},
	{"tail", tail_main},
	// {"tar", tar_main},
	{"tee", tee_main},
	{"test", test_main},
	// {"tftp", tftp_main},
	{"time", time_main},
	{"touch", touch_main},
	{"tr", tr_main},
	{"true", true_main},
	{"tsort", tsort_main},
	{"tty", tty_main},
	// {"uname", uname_main},
	{"unexpand", unexpand_main},
	{"uniq", uniq_main},
	{"unlink", unlink_main},
	// {"uudecode", uudecode_main},
	// {"uuencode", uuencode_main},
	{"wc", wc_main},
	{"which", which_main},
	{"whoami", whoami_main},
	{"xargs", xargs_main},
	{"xinstall", xinstall_main},
	{"yes", yes_main},
	{NULL},
};

main_fn sbase_find(const char *s) {
	for (struct cmd *bp = cmds; bp->name; ++bp) {
		if (strcmp(bp->name, s) == 0)
			return bp->fn;
	}
	return NULL;
}

void sbase_list(void) {
	for (struct cmd *bp = cmds; bp->name; ++bp)
		puts(bp->name);
}
