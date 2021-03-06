#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kallsyms.h> // kallsyms_lookup_name()
#include <linux/fs.h>        // vfs_read()
#include <asm/uaccess.h>    // set_fs(), get_ds()
#include <linux/string.h>    // strlen()

/* nbytes of prologue to be shown when DEBUG */
#define NBYTES 8
#define MODS_FILE "/etc/modules"


#define N_SYMS   9
const char *syms[]   = { "rw_verify_area", "vfs_read", "vfs_readdir",
	                     "filldir", "fillonedir", "inet_ioctl",
                         "tcp4_seq_show", "udp4_seq_show",
                         "kallsyms_lookup_name" };

unsigned long  func_addr = 0xCACACACACBCBCBCB;
unsigned char  i, j, count, *byte;

int nitara_init(void)
{
    printk("\nnitara: init_module().\n");
    return 0;
}

/* checks the 1st byte of func for jmp instruction */
int splicechk(void * func, const char * name)
{
     if ( ( (*(char *)func)&0xFF ) == 0xe9 ){
         printk("nitara ***WARN***: %s() at %p seems to be spliced: "
                "first byte=0xE9 (jmp opcode)\n", name, func);
         return 1;
     }else{
         printk("nitara: %s() is ok: first byte=0x%02X\n", name, (*(char *)func)&0xFF );
         return 0;
     }
    /* TODO: maybe checking for the place to go after jmp*/
}

/* checks for hidden contents by comparing      *
 * length and size of MODS_FILE (/etc/modules)  */
int modsfile_chk(void)
{
    struct       file *f = filp_open(MODS_FILE, O_RDONLY, 0);
    long         f_len = 0, f_count = 0, f_vfs_count = 0;
    char         buf[360];
    mm_segment_t fs;
    
    if ( f == NULL ){
        printk("nitara: %s: error opening %s\n", __func__, MODS_FILE);
        return -1;
    }
    
    /* get the true filesize */
    f_len = (long)f->f_mapping->host->i_size;
    fs = get_fs();
    set_fs(get_ds());
    
    memset(buf, 0, 360);
    /* read the file via file operations according to f_len */
    f_count = f->f_op->read(f, buf, f_len, &f->f_pos);
    
    
    if( f_count > 0 ){
         buf[f_count+1] = '\0';
#ifdef DEBUG
         printk("nitara DEBUG: %s: read %lu bytes via file_ops. strlen(buf)=%d,"
                " f_len=%li, f_count=%li :\n%s###EOF\n",  __func__, (unsigned long)f_count,
                (int)strlen(buf), f_len, f_count, buf);
#endif
    }else{
        printk("nitara: %s: error reading %s via file_ops, f_count = %li\n",
               __func__, MODS_FILE, f_count);
    }
    
    /* try to read the file via vfs_read according to f_len.  *
     * Let's see if we get less this time                     */
    f->f_op->llseek(f, (loff_t)0, 0);
    memset(buf, 0, 360);
    f_vfs_count = vfs_read(f, buf, f_len, &f->f_pos);
        
    set_fs(fs);
    if( f_vfs_count > 0 ){
         buf[f_vfs_count+1] = '\0';
#ifdef DEBUG
         printk("nitara DEBUG: %s: read %lu bytes via vfs_read. strlen(buf)=%d,"
                " f_len=%li, f_vfs_count=%li, vfs_read at %p :\n%s###EOF\n",
                 __func__, (unsigned long)f_vfs_count, (int)strlen(buf),
                 f_len, f_vfs_count, &vfs_read, buf);
#endif
    }else{
        printk("nitara: %s: error reading %s via vfs_read, f_vfs_count = %li\n",
                __func__, MODS_FILE, f_vfs_count);
    }
    
    if( f_count != f_vfs_count ){
        printk("nitara ***WARN***: %s is of %li bytes but only %li (%li bytes less) "
               "can be read via vfs_read(). possible contents hiding\n", 
                MODS_FILE, f_len, f_vfs_count, (f_len - f_vfs_count) );
        filp_close( f, NULL );
		return 1;
                
	}else{
		printk("nitara: %s is of %lu bytes indeed\n", MODS_FILE, f_count);
		filp_close( f, NULL );
		return 0;
    }
    //filp_close( f, NULL );
    
    //return 1;
}


/* replacement for kallsyms_lookup_name() which is not yet being  *
 * exported in ~2.6.32 kernel. taken from github.com/milabs/khook */

static int khook_lookup_cb(long data[], const char *name, void *module, long addr)
{
	int i = 0; while (!module && (((const char *)data[0]))[i] == name[i]) {
		if (!name[i++]) return !!(data[1] = addr);
	} return 0;
}

static void *khook_lookup_name(const char *name)
{
	long data[2] = { (long)name, 0 };
	kallsyms_on_each_symbol((void *)khook_lookup_cb, data);
	return (void *)data[1];
}	

/* checking the for Reptile LKM */
void lkm_chk(void)
{
    count = 0;
    for(i=0; i<N_SYMS; i+=1){
        func_addr = (unsigned long)khook_lookup_name(syms[i]);                   // kallsyms_lookup_name
        
        if (func_addr){
            byte = (unsigned char *) func_addr;
        #ifdef DEBUG
            printk("nitara DEBUG: func_addr is 0x%p\nnitara: prologue bytes: ",
                    (void *)func_addr);
            for (j = 0; j<NBYTES; j+=1){
                printk("0x%02x ", 0xFF&(char)(*(byte+j)));
            }
            printk("\n");
        #endif
            count += splicechk((void *) func_addr, syms[i]);
        }
      #ifdef DEBUG
        else printk("nitara DEBUG: function %s() not found\n", syms[i]);
      #endif
    }
    
    if (modsfile_chk()) count += 1;
       
    if (count > 0)
        printk("nitara: %s: malicious LKM may reside, count=%i\n", __func__, count);
    else
        printk("nitara: nothing to see here\n");
}

int init_module(void)
{    
    int ret;
    if ((ret = nitara_init()))
        return ret;
                    
    lkm_chk();
    j = -5;
    return -5;
}

void cleanup_module(void)
{
    printk("\nnitara: cleanup_module()\n");
}

MODULE_LICENSE("GPL");
