#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/mm.h>
#include <asm/insn.h>

#define NBYTES 8

/* names of symbols (functions) to be checked for splicing */
const char *chameo_syms[] = { "vfs_read" };


/* for accessing func prologue */
union chameo_ptr{
	unsigned long  c_int;
	void *        ptr;
	unsigned char byte[8];
};

int chameo_init(void)
{
	printk("\nchameo: init_module().\t sizeof chameo_syms = %lu\n",
	        sizeof chameo_syms);
	return 0;
}

unsigned long chameo_get_target_addr(const char * name)
{
	unsigned long addr = kallsyms_lookup_name(name);
	return addr;
}

int init_module(void)
{	
	unsigned long func_addr;
	union chameo_ptr prolog;
	
	int ret;
	unsigned char i, copy[NBYTES+1]="ABCDE";
	
	prolog.ptr = (void *)0xCACACACA;
	if ((ret = chameo_init()))
	    return ret;
	    
	/* checking vfs_read */
	func_addr = kallsyms_lookup_name(chameo_syms[0]);
	if (func_addr){
		printk("chameo: found %s at %lu\n", chameo_syms[0], func_addr);
	    prolog.c_int = func_addr;
	    
	    printk("chameo: prolog.ptr is 0x%p=%lu\nchameo: prologue bytes: ",
	            prolog.ptr, (unsigned long)(prolog.ptr));
	    
	    memcpy(copy, prolog.ptr, NBYTES);
	    printk("chameo: memcpy() done. prologue bytes: ");
	    for (i = 0; i<NBYTES; i+=1){
			printk("0x%02x ", (char)(*(copy+i)));
	    }
	    printk("\nchameo: %s checking done!\n", chameo_syms[0]);
	}
    
	return -5;
}

void cleanup_module(void)
{
	printk("\nchameo: cleanup_module()\n");
}

MODULE_LICENSE("GPL");
