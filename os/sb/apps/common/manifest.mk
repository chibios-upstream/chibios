# Deployable sandbox command images.
# Paths are relative to os/sb/apps. Install paths are relative to the
# sandbox VFS root.

SBAPP_DEPLOY_APPS := cat chedit cmp cp head hexdump ls msh sbsh sleep stat systime wc
SBAPP_POSIX_APPS  := cat chedit cmp cp head hexdump ls sbsh sleep stat systime wc

SBAPP_DEPLOY_cat_MAKEFILE := make/cat-rambox-deploy.make
SBAPP_DEPLOY_cat_ARTIFACT := cat.elf
SBAPP_DEPLOY_cat_PATH     := bin/cat.elf

SBAPP_DEPLOY_chedit_MAKEFILE := make/chedit-rambox-deploy.make
SBAPP_DEPLOY_chedit_ARTIFACT := chedit.elf
SBAPP_DEPLOY_chedit_PATH     := bin/chedit.elf

SBAPP_DEPLOY_cmp_MAKEFILE := make/cmp-rambox-deploy.make
SBAPP_DEPLOY_cmp_ARTIFACT := cmp.elf
SBAPP_DEPLOY_cmp_PATH     := bin/cmp.elf

SBAPP_DEPLOY_cp_MAKEFILE := make/cp-rambox-deploy.make
SBAPP_DEPLOY_cp_ARTIFACT := cp.elf
SBAPP_DEPLOY_cp_PATH     := bin/cp.elf

SBAPP_DEPLOY_head_MAKEFILE := make/head-rambox-deploy.make
SBAPP_DEPLOY_head_ARTIFACT := head.elf
SBAPP_DEPLOY_head_PATH     := bin/head.elf

SBAPP_DEPLOY_hexdump_MAKEFILE := make/hexdump-rambox-deploy.make
SBAPP_DEPLOY_hexdump_ARTIFACT := hexdump.elf
SBAPP_DEPLOY_hexdump_PATH     := bin/hexdump.elf

SBAPP_DEPLOY_ls_MAKEFILE := make/ls-rambox-deploy.make
SBAPP_DEPLOY_ls_ARTIFACT := ls.elf
SBAPP_DEPLOY_ls_PATH     := bin/ls.elf

SBAPP_DEPLOY_msh_MAKEFILE := make/msh-rambox-deploy.make
SBAPP_DEPLOY_msh_ARTIFACT := msh.elf
SBAPP_DEPLOY_msh_PATH     := bin/msh.elf

SBAPP_DEPLOY_sbsh_MAKEFILE := make/sbsh-rambox-deploy.make
SBAPP_DEPLOY_sbsh_ARTIFACT := sbsh.elf
SBAPP_DEPLOY_sbsh_PATH     := bin/sbsh.elf

SBAPP_DEPLOY_sleep_MAKEFILE := make/sleep-rambox-deploy.make
SBAPP_DEPLOY_sleep_ARTIFACT := sleep.elf
SBAPP_DEPLOY_sleep_PATH     := bin/sleep.elf

SBAPP_DEPLOY_stat_MAKEFILE := make/stat-rambox-deploy.make
SBAPP_DEPLOY_stat_ARTIFACT := stat.elf
SBAPP_DEPLOY_stat_PATH     := bin/stat.elf

SBAPP_DEPLOY_systime_MAKEFILE := make/systime-rambox-deploy.make
SBAPP_DEPLOY_systime_ARTIFACT := systime.elf
SBAPP_DEPLOY_systime_PATH     := bin/systime.elf

SBAPP_DEPLOY_wc_MAKEFILE := make/wc-rambox-deploy.make
SBAPP_DEPLOY_wc_ARTIFACT := wc.elf
SBAPP_DEPLOY_wc_PATH     := bin/wc.elf
