#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>

#include "mt_spm_internal.h"
#include <mach/mt_sleep.h>

/**************************************
 * Macro and Inline
 **************************************/
#define DEFINE_ATTR_RO(_name)			\
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = #_name,			\
		.mode = 0444,			\
	},					\
	.show	= _name##_show,			\
}

#define DEFINE_ATTR_RW(_name)			\
static struct kobj_attribute _name##_attr = {	\
	.attr	= {				\
		.name = #_name,			\
		.mode = 0644,			\
	},					\
	.show	= _name##_show,			\
	.store	= _name##_store,		\
}

#define __ATTR_OF(_name)	(&_name##_attr.attr)

#include <linux/slab.h> // for kmalloc/kfree
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>

#if 0
#define PCM_FIRMWARE_SIZE   0x1000
#define DYNA_LOAD_PCM_PATH_SIZE 64

enum dyna_load_pcm_index
{
    DYNA_LOAD_PCM_SUSPEND = 0,
    DYNA_LOAD_PCM_SODI,
    DYNA_LOAD_PCM_MCDI,
    DYNA_LOAD_PCM_DEEPIDLE,
    DYNA_LOAD_PCM_DEEPIDLE_AUDIO,
    DYNA_LOAD_PCM_VCORE_DVFS,
    DYNA_LOAD_PCM_MAX,
};

#endif

struct dyna_load_pcm_t dyna_load_pcm[DYNA_LOAD_PCM_MAX];

static char *dyna_load_pcm_name[] =
{
    [DYNA_LOAD_PCM_SUSPEND]         = "suspend",
    [DYNA_LOAD_PCM_SODI]            = "sodi",
    [DYNA_LOAD_PCM_MCDI]            = "mcdi",
    [DYNA_LOAD_PCM_DEEPIDLE]        = "deepidle",
    [DYNA_LOAD_PCM_DEEPIDLE_AUDIO]  = "deepidle_audio",
    [DYNA_LOAD_PCM_VCORE_DVFS]      = "vcore_dvfs",
    [DYNA_LOAD_PCM_MAX]             = "pcm_max",
};

/**************************************
 * xxx_pcm_show Function
 **************************************/
static ssize_t show_pcm_desc(const struct pcm_desc *pcmdesc, char *buf)
{
	char *p = buf;

	p += sprintf(p, "version = %s\n", pcmdesc->version);
	p += sprintf(p, "base = 0x%p\n" , pcmdesc->base);
	p += sprintf(p, "size = %u\n"   , pcmdesc->size);
	p += sprintf(p, "sess = %u\n"   , pcmdesc->sess);
	p += sprintf(p, "replace = %u\n", pcmdesc->replace);

	p += sprintf(p, "vec0 = 0x%x\n" , pcmdesc->vec0);
	p += sprintf(p, "vec1 = 0x%x\n" , pcmdesc->vec1);
	p += sprintf(p, "vec2 = 0x%x\n" , pcmdesc->vec2);
	p += sprintf(p, "vec3 = 0x%x\n" , pcmdesc->vec3);
	p += sprintf(p, "vec4 = 0x%x\n" , pcmdesc->vec4);
	p += sprintf(p, "vec5 = 0x%x\n" , pcmdesc->vec5);
	p += sprintf(p, "vec6 = 0x%x\n" , pcmdesc->vec6);
	p += sprintf(p, "vec7 = 0x%x\n" , pcmdesc->vec7);

	BUG_ON(p - buf >= PAGE_SIZE);
	return p - buf;
}
#if 0 /* FIXME */
static ssize_t suspend_pcm_show(struct kobject *kobj, struct kobj_attribute *attr,
				char *buf)
{
	return show_pcm_desc(__spm_suspend.pcmdesc, buf);
}

static ssize_t dpidle_pcm_show(struct kobject *kobj, struct kobj_attribute *attr,
			       char *buf)
{
	return show_pcm_desc(__spm_dpidle.pcmdesc, buf);
}

static ssize_t sodi_pcm_show(struct kobject *kobj, struct kobj_attribute *attr,
			     char *buf)
{
	return show_pcm_desc(__spm_sodi.pcmdesc, buf);
}

static ssize_t mcdi_pcm_show(struct kobject *kobj, struct kobj_attribute *attr,
			     char *buf)
{
	return show_pcm_desc(__spm_mcdi.pcmdesc, buf);
}
#endif

static ssize_t talking_pcm_show(struct kobject *kobj, struct kobj_attribute *attr,
				char *buf)
{
//FIXME: for K2 bring up
#if 0
	return show_pcm_desc(__spm_talking.pcmdesc, buf);
#else
    return 0;
#endif
}

static ssize_t ddrdfs_pcm_show(struct kobject *kobj, struct kobj_attribute *attr,
			       char *buf)
{
//FIXME: for K2 bring up
#if 0
	return show_pcm_desc(__spm_ddrdfs.pcmdesc, buf);
#else
    return 0;
#endif
}


/**************************************
 * xxx_ctrl_show Function
 **************************************/
static ssize_t show_pwr_ctrl(const struct pwr_ctrl *pwrctrl, char *buf)
{
	char *p = buf;

	p += sprintf(p, "pcm_flags = 0x%x\n"      , pwrctrl->pcm_flags);
	p += sprintf(p, "pcm_flags_cust = 0x%x\n" , pwrctrl->pcm_flags_cust);
	p += sprintf(p, "pcm_reserve = 0x%x\n"    , pwrctrl->pcm_reserve);
	p += sprintf(p, "timer_val = 0x%x\n"      , pwrctrl->timer_val);
	p += sprintf(p, "timer_val_cust = 0x%x\n" , pwrctrl->timer_val_cust);
	p += sprintf(p, "wake_src = 0x%x\n"       , pwrctrl->wake_src);
	p += sprintf(p, "wake_src_cust = 0x%x\n"  , pwrctrl->wake_src_cust);
	p += sprintf(p, "wake_src_md32 = 0x%x\n"  , pwrctrl->wake_src_md32);
	p += sprintf(p, "r0_ctrl_en = %u\n"       , pwrctrl->r0_ctrl_en);
	p += sprintf(p, "r7_ctrl_en = %u\n"       , pwrctrl->r7_ctrl_en);
	p += sprintf(p, "infra_dcm_lock = %u\n"   , pwrctrl->infra_dcm_lock);
	p += sprintf(p, "pcm_apsrc_req = %u\n"    , pwrctrl->pcm_apsrc_req);
        p += sprintf(p, "pcm_f26m_req = %u\n"    , pwrctrl->pcm_f26m_req);

	p += sprintf(p, "mcusys_idle_mask = %u\n" , pwrctrl->mcusys_idle_mask);
	p += sprintf(p, "ca15top_idle_mask = %u\n", pwrctrl->ca15top_idle_mask);
	p += sprintf(p, "ca7top_idle_mask = %u\n" , pwrctrl->ca7top_idle_mask);
	p += sprintf(p, "wfi_op = %u\n"           , pwrctrl->wfi_op);
	p += sprintf(p, "ca15_wfi0_en = %u\n"     , pwrctrl->ca15_wfi0_en);
	p += sprintf(p, "ca15_wfi1_en = %u\n"     , pwrctrl->ca15_wfi1_en);
	p += sprintf(p, "ca15_wfi2_en = %u\n"     , pwrctrl->ca15_wfi2_en);
	p += sprintf(p, "ca15_wfi3_en = %u\n"     , pwrctrl->ca15_wfi3_en);
	p += sprintf(p, "ca7_wfi0_en = %u\n"      , pwrctrl->ca7_wfi0_en);
	p += sprintf(p, "ca7_wfi1_en = %u\n"      , pwrctrl->ca7_wfi1_en);
	p += sprintf(p, "ca7_wfi2_en = %u\n"      , pwrctrl->ca7_wfi2_en);
	p += sprintf(p, "ca7_wfi3_en = %u\n"      , pwrctrl->ca7_wfi3_en);

	p += sprintf(p, "md1_req_mask = %u\n"     , pwrctrl->md1_req_mask);
	p += sprintf(p, "md2_req_mask = %u\n"     , pwrctrl->md2_req_mask);
	p += sprintf(p, "md_apsrc_sel = %u\n"     , pwrctrl->md_apsrc_sel);
        p += sprintf(p, "md2_apsrc_sel = %u\n"     , pwrctrl->md2_apsrc_sel);
	p += sprintf(p, "gce_req_mask = %u\n"    , pwrctrl->gce_req_mask); //mt6735//mt6580
	p += sprintf(p, "ccif0_to_ap_mask = %u\n"  , pwrctrl->ccif0_to_ap_mask);
	p += sprintf(p, "ccif0_to_md_mask = %u\n"  , pwrctrl->ccif0_to_md_mask);
	p += sprintf(p, "ccif1_to_ap_mask = %u\n"  , pwrctrl->ccif1_to_ap_mask);
	p += sprintf(p, "ccif1_to_md_mask = %u\n"  , pwrctrl->ccif1_to_md_mask);
        p += sprintf(p, "lte_mask = %u\n"  , pwrctrl->lte_mask);
        p += sprintf(p, "ccifmd_md1_event_mask = %u\n"  , pwrctrl->ccifmd_md1_event_mask);
        p += sprintf(p, "ccifmd_md2_event_mask = %u\n"  , pwrctrl->ccifmd_md2_event_mask);

        p += sprintf(p, "conn_mask = %u\n"  , pwrctrl->conn_mask);

	p += sprintf(p, "disp_req_mask = %u\n"    , pwrctrl->disp_req_mask);
	p += sprintf(p, "mfg_req_mask = %u\n"     , pwrctrl->mfg_req_mask);
	p += sprintf(p, "dsi0_ddr_en_mask = %u\n" , pwrctrl->dsi0_ddr_en_mask);
	p += sprintf(p, "dsi1_ddr_en_mask = %u\n" , pwrctrl->dsi1_ddr_en_mask);
	p += sprintf(p, "dpi_ddr_en_mask = %u\n"  , pwrctrl->dpi_ddr_en_mask);
	p += sprintf(p, "isp0_ddr_en_mask = %u\n" , pwrctrl->isp0_ddr_en_mask);
	p += sprintf(p, "isp1_ddr_en_mask = %u\n" , pwrctrl->isp1_ddr_en_mask);

	p += sprintf(p, "md32_req_mask = %u\n"    , pwrctrl->md32_req_mask);
	p += sprintf(p, "syspwreq_mask = %u\n"    , pwrctrl->syspwreq_mask);
	p += sprintf(p, "srclkenai_mask = %u\n"   , pwrctrl->srclkenai_mask);

	p += sprintf(p, "param1 = 0x%x\n"         , pwrctrl->param1);
	p += sprintf(p, "param2 = 0x%x\n"         , pwrctrl->param2);
	p += sprintf(p, "param3 = 0x%x\n"         , pwrctrl->param3);

	BUG_ON(p - buf >= PAGE_SIZE);
	return p - buf;
}

#if 0 /* FIXME */
static ssize_t suspend_ctrl_show(struct kobject *kobj, struct kobj_attribute *attr,
				 char *buf)
{
	return show_pwr_ctrl(__spm_suspend.pwrctrl, buf);
}
#endif
#if 0 /* FIXME */
static ssize_t dpidle_ctrl_show(struct kobject *kobj, struct kobj_attribute *attr,
				char *buf)
{
	return show_pwr_ctrl(__spm_dpidle.pwrctrl, buf);
}
#endif
#if 0 /* FIXME */
static ssize_t sodi_ctrl_show(struct kobject *kobj, struct kobj_attribute *attr,
			      char *buf)
{
	return show_pwr_ctrl(__spm_sodi.pwrctrl, buf);
}
#endif
#if 0 /* FIXME */
static ssize_t mcdi_ctrl_show(struct kobject *kobj, struct kobj_attribute *attr,
			      char *buf)
{
	return show_pwr_ctrl(__spm_mcdi.pwrctrl, buf);
}
#endif
static ssize_t talking_ctrl_show(struct kobject *kobj, struct kobj_attribute *attr,
				 char *buf)
{
//FIXME: for K2 bring up
#if 0
	return show_pwr_ctrl(__spm_talking.pwrctrl, buf);
#else
    return 0;
#endif
}

static ssize_t ddrdfs_ctrl_show(struct kobject *kobj, struct kobj_attribute *attr,
				char *buf)
{
//FIXME: for K2 bring up
#if 0
	return show_pwr_ctrl(__spm_ddrdfs.pwrctrl, buf);
#else
    return 0;
#endif
}


/**************************************
 * xxx_ctrl_store Function
 **************************************/
static ssize_t store_pwr_ctrl(struct pwr_ctrl *pwrctrl, const char *buf, size_t count)
{
	u32 val;
	char cmd[32];

	if (sscanf(buf, "%31s %x", cmd, &val) != 2)
		return -EPERM;

	spm_debug("pwr_ctrl: cmd = %s, val = 0x%x\n", cmd, val);

	if (!strcmp(cmd, "pcm_flags"))
		pwrctrl->pcm_flags = val;
	else if (!strcmp(cmd, "pcm_flags_cust"))
		pwrctrl->pcm_flags_cust = val;
	else if (!strcmp(cmd, "pcm_reserve"))
		pwrctrl->pcm_reserve = val;
	else if (!strcmp(cmd, "timer_val"))
		pwrctrl->timer_val = val;
	else if (!strcmp(cmd, "timer_val_cust"))
		pwrctrl->timer_val_cust = val;
	else if (!strcmp(cmd, "wake_src"))
		pwrctrl->wake_src = val;
	else if (!strcmp(cmd, "wake_src_cust"))
		pwrctrl->wake_src_cust = val;
	else if (!strcmp(cmd, "wake_src_md32"))
		pwrctrl->wake_src_md32 = val;
	else if (!strcmp(cmd, "r0_ctrl_en"))
		pwrctrl->r0_ctrl_en = val;
	else if (!strcmp(cmd, "r7_ctrl_en"))
		pwrctrl->r7_ctrl_en = val;
	else if (!strcmp(cmd, "infra_dcm_lock"))
		pwrctrl->infra_dcm_lock = val;
	else if (!strcmp(cmd, "pcm_apsrc_req"))
		pwrctrl->pcm_apsrc_req = val;
        else if (!strcmp(cmd, "pcm_f26m_req"))
		pwrctrl->pcm_f26m_req = val;

	else if (!strcmp(cmd, "mcusys_idle_mask"))
		pwrctrl->mcusys_idle_mask = val;
	else if (!strcmp(cmd, "ca15top_idle_mask"))
		pwrctrl->ca15top_idle_mask = val;
	else if (!strcmp(cmd, "ca7top_idle_mask"))
		pwrctrl->ca7top_idle_mask = val;
	else if (!strcmp(cmd, "wfi_op"))
		pwrctrl->wfi_op = val;
	else if (!strcmp(cmd, "ca15_wfi0_en"))
		pwrctrl->ca15_wfi0_en = val;
	else if (!strcmp(cmd, "ca15_wfi1_en"))
		pwrctrl->ca15_wfi1_en = val;
	else if (!strcmp(cmd, "ca15_wfi2_en"))
		pwrctrl->ca15_wfi2_en = val;
	else if (!strcmp(cmd, "ca15_wfi3_en"))
		pwrctrl->ca15_wfi3_en = val;
	else if (!strcmp(cmd, "ca7_wfi0_en"))
		pwrctrl->ca7_wfi0_en = val;
	else if (!strcmp(cmd, "ca7_wfi1_en"))
		pwrctrl->ca7_wfi1_en = val;
	else if (!strcmp(cmd, "ca7_wfi2_en"))
		pwrctrl->ca7_wfi2_en = val;
	else if (!strcmp(cmd, "ca7_wfi3_en"))
		pwrctrl->ca7_wfi3_en = val;

	else if (!strcmp(cmd, "md1_req_mask"))
		pwrctrl->md1_req_mask = val;
	else if (!strcmp(cmd, "md2_req_mask"))
		pwrctrl->md2_req_mask = val;
	else if (!strcmp(cmd, "md_apsrc_sel"))
		pwrctrl->md_apsrc_sel = val;
	else if (!strcmp(cmd, "md2_apsrc_sel"))
		pwrctrl->md2_apsrc_sel = val;
	else if (!strcmp(cmd, "gce_req_mask")) //mt6735//mt6580
		pwrctrl->gce_req_mask = val;
	else if (!strcmp(cmd, "ccif0_to_ap_mask"))
		pwrctrl->ccif0_to_ap_mask = val;
	else if (!strcmp(cmd, "ccif0_to_md_mask"))
		pwrctrl->ccif0_to_md_mask = val;
	else if (!strcmp(cmd, "ccif1_to_ap_mask"))
		pwrctrl->ccif1_to_ap_mask = val;
	else if (!strcmp(cmd, "ccif1_to_md_mask"))
		pwrctrl->ccif1_to_md_mask = val;
        else if (!strcmp(cmd, "lte_mask"))
		pwrctrl->lte_mask = val;
        else if (!strcmp(cmd, "ccifmd_md1_event_mask"))
		pwrctrl->ccifmd_md1_event_mask = val;
        else if (!strcmp(cmd, "ccifmd_md2_event_mask"))
		pwrctrl->ccifmd_md2_event_mask = val;

	else if (!strcmp(cmd, "conn_mask"))
		pwrctrl->conn_mask = val;

	else if (!strcmp(cmd, "disp_req_mask"))
		pwrctrl->disp_req_mask = val;
	else if (!strcmp(cmd, "mfg_req_mask"))
		pwrctrl->mfg_req_mask = val;
	else if (!strcmp(cmd, "dsi0_ddr_en_mask"))
		pwrctrl->dsi0_ddr_en_mask = val;
	else if (!strcmp(cmd, "dsi1_ddr_en_mask"))
		pwrctrl->dsi1_ddr_en_mask = val;
	else if (!strcmp(cmd, "dpi_ddr_en_mask"))
		pwrctrl->dpi_ddr_en_mask = val;
	else if (!strcmp(cmd, "isp0_ddr_en_mask"))
		pwrctrl->isp0_ddr_en_mask = val;
	else if (!strcmp(cmd, "isp1_ddr_en_mask"))
		pwrctrl->isp1_ddr_en_mask = val;

	else if (!strcmp(cmd, "md32_req_mask"))
		pwrctrl->md32_req_mask = val;
	else if (!strcmp(cmd, "syspwreq_mask"))
		pwrctrl->syspwreq_mask = val;
	else if (!strcmp(cmd, "srclkenai_mask"))
		pwrctrl->srclkenai_mask = val;

	else if (!strcmp(cmd, "param1"))
		pwrctrl->param1 = val;
	else if (!strcmp(cmd, "param2"))
		pwrctrl->param2 = val;
	else if (!strcmp(cmd, "param3"))
		pwrctrl->param3 = val;
	else
		return -EINVAL;

	return count;
}
#if 0 /* FIXME */
static ssize_t suspend_ctrl_store(struct kobject *kobj, struct kobj_attribute *attr,
				  const char *buf, size_t count)
{
	return store_pwr_ctrl(__spm_suspend.pwrctrl, buf, count);
}
#endif
#if 0 /* FIXME */
static ssize_t dpidle_ctrl_store(struct kobject *kobj, struct kobj_attribute *attr,
				 const char *buf, size_t count)
{
	return store_pwr_ctrl(__spm_dpidle.pwrctrl, buf, count);
}
#endif
#if 0 /* FIXME */
static ssize_t sodi_ctrl_store(struct kobject *kobj, struct kobj_attribute *attr,
			       const char *buf, size_t count)
{
	return store_pwr_ctrl(__spm_sodi.pwrctrl, buf, count);
}
#endif
#if 0 /* FIXME */
static ssize_t mcdi_ctrl_store(struct kobject *kobj, struct kobj_attribute *attr,
			       const char *buf, size_t count)
{
	return store_pwr_ctrl(__spm_mcdi.pwrctrl, buf, count);
}
#endif
static ssize_t talking_ctrl_store(struct kobject *kobj, struct kobj_attribute *attr,
				  const char *buf, size_t count)
{
//FIXME: for K2 bring up
#if 0
	return store_pwr_ctrl(__spm_talking.pwrctrl, buf, count);
#else
    return 0;
#endif
}

static ssize_t ddrdfs_ctrl_store(struct kobject *kobj, struct kobj_attribute *attr,
				 const char *buf, size_t count)
{
//FIXME: for K2 bring up
#if 0
	return store_pwr_ctrl(__spm_ddrdfs.pwrctrl, buf, count);
#else
    return 0;
#endif
}


/**************************************
 * ddren_debug_xxx Function
 **************************************/
static ssize_t ddren_debug_show(struct kobject *kobj, struct kobj_attribute *attr,
				char *buf)
{
	char *p = buf;

	p += sprintf(p, "PCM_REG13_DATA = 0x%x\n", spm_read(SPM_PCM_REG13_DATA));
	p += sprintf(p, "AP_STANBY_CON = 0x%x\n" , spm_read(SPM_AP_STANBY_CON));
	p += sprintf(p, "PCM_DEBUG_CON = 0x%x\n" , spm_read(SPM_PCM_DEBUG_CON));
	p += sprintf(p, "PCM_PASR_DPD_2 = 0x%x\n", spm_read(SPM_PCM_PASR_DPD_2));

	BUG_ON(p - buf >= PAGE_SIZE);
	return p - buf;
}

static ssize_t ddren_debug_store(struct kobject *kobj, struct kobj_attribute *attr,
				 const char *buf, size_t count)
{
	u32 val, con;
	char cmd[32];
	unsigned long flags;

	if (sscanf(buf, "%31s %x", cmd, &val) != 2)
		return -EPERM;

	spm_debug("ddren_debug: cmd = %s, val = 0x%x\n", cmd, val);

	if (!strcmp(cmd, "ddr_en_sel")) {
		spin_lock_irqsave(&__spm_lock, flags);
		con = spm_read(SPM_AP_STANBY_CON) & ~(1U << 22);
		spm_write(SPM_AP_STANBY_CON, con | (!!val << 22));
		spin_unlock_irqrestore(&__spm_lock, flags);
	} else if (!strcmp(cmd, "md_ddr_en_out")) {
		spin_lock_irqsave(&__spm_lock, flags);
		__spm_dbgout_md_ddr_en(val);
		spin_unlock_irqrestore(&__spm_lock, flags);
	} else if (!strcmp(cmd, "mm_ddr_en_mask")) {
		spin_lock_irqsave(&__spm_lock, flags);
		spm_write(SPM_PCM_PASR_DPD_2, ~val & 0x1f);
		spin_unlock_irqrestore(&__spm_lock, flags);
	} else {
		return -EINVAL;
	}

	return count;
}


/**************************************
 * golden_dump_xxx Function
 **************************************/
static ssize_t golden_dump_show(struct kobject *kobj, struct kobj_attribute *attr,
				char *buf)
{
	char *p = buf;

	//spm_golden_setting_cmp(1); //TODO: wait DRAMC golden setting check enable

	BUG_ON(p - buf >= PAGE_SIZE);
	return p - buf;
}

/**************************************
 * auto_suspend_resume_xxx Function
 **************************************/
#if 0 /* FIXME */
static ssize_t auto_suspend_resume_show(struct kobject *kobj, struct kobj_attribute *attr,
				char *buf)
{
	char *p = buf;
    u8 i;

	p += sprintf(p, "auto_suspend_resume:%d times\n", 10);

    for (i=0;i<10;i++)
    {
        p += sprintf(p, "[%d]wakeup:0x%x,timer:0x%x,r13:0x%x,event=0x%x,flag=0x%x\n",
                         __spm_suspend.wakestatus[i].log_index,
                         __spm_suspend.wakestatus[i].r12,
                         __spm_suspend.wakestatus[i].timer_out,
                         __spm_suspend.wakestatus[i].r13,
                         __spm_suspend.wakestatus[i].event_reg,
                         __spm_suspend.wakestatus[i].debug_flag);
        if (0x90100000 != __spm_suspend.wakestatus[i].event_reg)
            p += sprintf(p, "SLEEP_ABORT\n");
        else
        if (0xf != (__spm_suspend.wakestatus[i].debug_flag & 0xF))//TODO: check Rainier valid suspend debug flag
            p += sprintf(p, "NOT_DEEP_SLEEP\n");
        else
            p += sprintf(p, "SLEEP_PASS\n");
    }

    slp_set_auto_suspend_wakelock(0);

	BUG_ON(p - buf >= PAGE_SIZE);
	return p - buf;
}

static ssize_t auto_suspend_resume_store(struct kobject *kobj, struct kobj_attribute *attr,
				 const char *buf, size_t count)
{
	u32 val, pcm_sec;

	if (sscanf(buf, "%d %d", &val, &pcm_sec) != 2)
    {
        spm_debug("auto_suspend_resume parameter fail\n");
		return -EPERM;
    }
    spm_debug("auto_suspend_resume val = %d, pcm_sec = %d\n", val, pcm_sec);
    __spm_suspend.pwrctrl->timer_val_cust = pcm_sec*32768;
    slp_create_auto_suspend_resume_thread();
    slp_start_auto_suspend_resume_timer(val);

	return count;
}
#endif /* 0 */
/**************************************
 * dyna_load_pcm Function
 **************************************/
struct file* file_open(const char* path, int flags, int rights)
{
    struct file* filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);
    if (IS_ERR(filp))
    {
        err = PTR_ERR(filp);
        return NULL;
    }
    return filp;
}

void file_close(struct file* file)
{
    filp_close(file, NULL);
}

int file_read(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size)
{
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = vfs_read(file, data, size, &offset);

    set_fs(oldfs);
    return ret;
}

int file_write(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size)
{
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = vfs_write(file, data, size, &offset);

    set_fs(oldfs);
    return ret;
}

int file_sync(struct file* file)
{
    vfs_fsync(file, 0);
    return 0;
}

static ssize_t dyna_load_pcm_show(struct kobject *kobj, struct kobj_attribute *attr,
                                  char *buf)
{
    char *p = buf;
    u8 i;
    struct pcm_desc *pdesc = NULL;

    for (i = DYNA_LOAD_PCM_SUSPEND; i < DYNA_LOAD_PCM_MAX; i++)
    {
        pdesc = &(dyna_load_pcm[i].desc);
        p += sprintf(p, " %s - %s\n",
                     dyna_load_pcm_name[i],
                     dyna_load_pcm[i].path);

        if (pdesc->base)
        {
            p += sprintf(p, "  version = %s\n", pdesc->version);
            p += sprintf(p, "  base = 0x%p\n" , pdesc->base);
            p += sprintf(p, "  size = %u\n"   , pdesc->size);
            p += sprintf(p, "  sess = %u\n"   , pdesc->sess);
            p += sprintf(p, "  replace = %u\n", pdesc->replace);
            p += sprintf(p, "  vec0 = 0x%x\n" , pdesc->vec0);
            p += sprintf(p, "  vec1 = 0x%x\n" , pdesc->vec1);
            p += sprintf(p, "  vec2 = 0x%x\n" , pdesc->vec2);
            p += sprintf(p, "  vec3 = 0x%x\n" , pdesc->vec3);
            p += sprintf(p, "  vec4 = 0x%x\n" , pdesc->vec4);
            p += sprintf(p, "  vec5 = 0x%x\n" , pdesc->vec5);
            p += sprintf(p, "  vec6 = 0x%x\n" , pdesc->vec6);
            p += sprintf(p, "  vec7 = 0x%x\n" , pdesc->vec7);
        }
    }

    BUG_ON(p - buf >= PAGE_SIZE);
    return p - buf;
}

static ssize_t dyna_load_pcm_store(struct kobject *kobj, struct kobj_attribute *attr,
                                   const char *buf, size_t count)
{
    int i = 0, found = -1, ret = 0;
    char cmd[32];
    char path[DYNA_LOAD_PCM_PATH_SIZE];
    struct file *file = NULL;
    void *ptr = NULL;
    struct pcm_desc *pdesc = NULL;
    char *ppath = NULL;

    if (sscanf(buf, "%s%s", cmd, path) != 2)
        return -EPERM;

    for (i = DYNA_LOAD_PCM_SUSPEND; i < DYNA_LOAD_PCM_MAX; i++)
    {
        if (strstr(dyna_load_pcm_name[i], cmd))
        {
            found = i;
            pdesc = &(dyna_load_pcm[found].desc);
            ppath = dyna_load_pcm[found].path;
            break;
        }
    }

    if (found == -1)
    {
        spm_debug(" Cannot find %s\n", cmd);
        return count;
    }

    if (strstr(path, "xxx"))   // remove path
    {
        if (pdesc->base)
        {
            kfree((const void *)pdesc->base);
            memset(&dyna_load_pcm[found], 0, sizeof(struct dyna_load_pcm_t));
            //pdesc->base = NULL;
            //memset(ppath, 0, DYNA_LOAD_PCM_PATH_SIZE);
        }
        return count;
    }
    else   // a real file path
    {
        if (pdesc->base)
        {
            ptr = (void *)pdesc->base;
            pdesc->base = NULL;
            memset(ppath, 0, DYNA_LOAD_PCM_PATH_SIZE);
        }
        else
        {
            ptr = kmalloc(PCM_FIRMWARE_SIZE, GFP_KERNEL);
            if (!ptr)
            {
                spm_debug(" Cannot malloc memory\n");
                return -ENOMEM;
            }
        }
    }

    // open file
    file = file_open(path, O_RDONLY, (int)NULL);
    if (!file)
    {
        spm_debug(" Open file %s fail\n", path);
        kfree(ptr);
        return count;
    }
    // read file
    ret = file_read(file, 0x0, (unsigned char*)ptr, PCM_FIRMWARE_SIZE);
    if (ret < PCM_FIRMWARE_SIZE)
    {
        spm_debug(" Read file %d/%d fail\n", ret, PCM_FIRMWARE_SIZE);
        kfree(ptr);
        file_close(file);
        return count;
    }

    ret = file_read(file, PCM_FIRMWARE_SIZE, (unsigned char*)&(dyna_load_pcm[found].desc.size), 0x24);
    if (ret < 0x24)
    {
        spm_debug(" Read file %d/%d fail\n", ret, 0x24);
        kfree(ptr);
        file_close(file);
        return count;
    }

    // close file
    file_close(file);

    pdesc->version = "dyna_load_pcm test";
    pdesc->base = (unsigned int *)ptr;

    memcpy(ppath, path, DYNA_LOAD_PCM_PATH_SIZE);
    spm_debug(" %s - %s\n", dyna_load_pcm_name[found], ppath);
    spm_debug(" version = %s\n", pdesc->version);
    spm_debug(" base = 0x%p\n" , pdesc->base);
    spm_debug(" size = %u\n"   , pdesc->size);
    spm_debug(" sess = %u\n"   , pdesc->sess);
    spm_debug(" replace = %u\n", pdesc->replace);
    spm_debug(" vec0 = 0x%x\n" , pdesc->vec0);
    spm_debug(" vec1 = 0x%x\n" , pdesc->vec1);
    spm_debug(" vec2 = 0x%x\n" , pdesc->vec2);
    spm_debug(" vec3 = 0x%x\n" , pdesc->vec3);
    spm_debug(" vec4 = 0x%x\n" , pdesc->vec4);
    spm_debug(" vec5 = 0x%x\n" , pdesc->vec5);
    spm_debug(" vec6 = 0x%x\n" , pdesc->vec6);
    spm_debug(" vec7 = 0x%x\n" , pdesc->vec7);

    return count;
}
/**************************************
 * Init Function
 **************************************/
//FIXME
//DEFINE_ATTR_RO(suspend_pcm);
//DEFINE_ATTR_RO(dpidle_pcm);
//DEFINE_ATTR_RO(sodi_pcm);
//DEFINE_ATTR_RO(mcdi_pcm);
DEFINE_ATTR_RO(talking_pcm);
DEFINE_ATTR_RO(ddrdfs_pcm);

//FIXME
//DEFINE_ATTR_RW(suspend_ctrl);
//DEFINE_ATTR_RW(dpidle_ctrl);
//DEFINE_ATTR_RW(sodi_ctrl);
//DEFINE_ATTR_RW(mcdi_ctrl);
DEFINE_ATTR_RW(talking_ctrl);
DEFINE_ATTR_RW(ddrdfs_ctrl);

DEFINE_ATTR_RW(ddren_debug);
DEFINE_ATTR_RO(golden_dump);

//FIXME 
//DEFINE_ATTR_RW(auto_suspend_resume);
DEFINE_ATTR_RW(dyna_load_pcm);

static struct attribute *spm_attrs[] = {
	/* for spm_lp_scen.pcmdesc */
	//FIXME
	//__ATTR_OF(suspend_pcm),
	//__ATTR_OF(dpidle_pcm),
	//__ATTR_OF(sodi_pcm),
	//__ATTR_OF(mcdi_pcm),
	__ATTR_OF(talking_pcm),
	__ATTR_OF(ddrdfs_pcm),

	/* for spm_lp_scen.pwrctrl */
	//FIXME
	//__ATTR_OF(suspend_ctrl),
	//__ATTR_OF(dpidle_ctrl),
	//__ATTR_OF(sodi_ctrl),
	//__ATTR_OF(mcdi_ctrl),
	__ATTR_OF(talking_ctrl),
	__ATTR_OF(ddrdfs_ctrl),

	/* other debug interface */
	__ATTR_OF(ddren_debug),

   //FIXME
   //__ATTR_OF(auto_suspend_resume),
    __ATTR_OF(dyna_load_pcm),

	/* must */
	NULL,
};

static struct attribute_group spm_attr_group = {
	.name	= "spm",
	.attrs	= spm_attrs,
};

int spm_fs_init(void)
{
    int r;
    int i;

    for (i = DYNA_LOAD_PCM_SUSPEND; i < DYNA_LOAD_PCM_MAX; i++)
    {
        memset(&dyna_load_pcm[i], 0, sizeof(struct dyna_load_pcm_t));
    }

	/* create /sys/power/spm/xxx */
	r = sysfs_create_group(power_kobj, &spm_attr_group);
	if (r)
		spm_err("FAILED TO CREATE /sys/power/spm (%d)\n", r);

	return r;
}

MODULE_DESCRIPTION("SPM-FS Driver v0.1");
