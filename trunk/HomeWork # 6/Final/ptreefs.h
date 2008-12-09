/* the following can be moved to ptreefs.h to make things look better: */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h> 	/* PAGE_CACHE_SIZE */
#include <linux/fs.h>     	/* This is where libfs stuff is declared */
#include <asm/atomic.h>
#include <asm/uaccess.h>	/* copy_to_user */
#include <linux/sched.h>


#define PTREEFS_MAGIC 9999999				/* our magic number */
#define FILE_PERMISSIONS		0444;
#define DIRECTORY_PERMISSIONS 	0555;			
#define MAX_FILE_LEN			16

/*	already defined in sched.h
struct ptreefs_node{
	struct dentry *file_dentry;
	struct inode *file_inode;
		
	struct dentry *dir_dentry;
	struct inode *dir_inode;
};
*/

static struct super_block 	*ptreefs_super_block;
static struct inode 		*ptreefs_root_inode;
struct dentry 				*ptreefs_root_dentry;

static int PTREE_FS_ENABLED = 0; 

struct ptreefs_node{
	struct dentry *file_dentry;
	struct inode *file_inode;
		
	struct dentry *dir_dentry;
	struct inode *dir_inode;
};



/* absolute musts for the FS */

static struct super_block *ptreefs_get_super(struct file_system_type *fst,int flags, const char *devname, void *data);
static int ptreefs_fill_super (struct super_block *sb, void *data, int silent);
static void ptreefs_kill_super(struct super_block *super);

static struct inode  *ptreefs_make_inode	(struct super_block *sb, int mode);
static struct dentry *ptreefs_create_dir 	(struct super_block *sb, struct dentry *parent, const char *name);
static struct dentry *ptreefs_create_file 	(struct super_block *sb, struct dentry *dir, const char *name);

static void ptreefs_create_files (struct super_block *sb, struct dentry *root);
static struct ptreefs_node *ptreefs_create_entry_form_PROCESS(struct task_struct *new_task, struct super_block *sb,struct dentry *parent_process_dir);

static void ptreefs_process_exit(struct task_struct * father, struct ptreefs_node *father_node);		
static void ptreefs_move_to_root(struct dentry *root, struct ptreefs_node *the_node);
static void ptreefs_precess_exec(char *new_name, struct ptreefs_node *the_node);



static struct inode_operations ptreefs_dir_inode_operations = 
{
	lookup:		simple_lookup
};

static struct file_operations  ptreefs_dir_operations =
{
	open:		dcache_dir_open,
	release:	dcache_dir_close,
	llseek:		dcache_dir_lseek,
	read:		generic_read_dir,
	readdir:	dcache_readdir
};

static struct super_operations ptreefs_superblock_ops = 
{
    statfs: simple_statfs /* Generic stats */
	/* enough for now */
};

static struct file_system_type ptreefs = 
{
  name:        "ptreefs",
  get_sb:    	ptreefs_get_super,
  kill_sb: 		ptreefs_kill_super,
  owner:        THIS_MODULE
};

static struct file_operations ptreefs_file_ops = 
{
	/* umm none... dah */
};


/* these will change if we decide to have this inside the kernel. currently its defined to work as a mofule : */
/*******************************************************************************************/
static int ptreefs_init_module(void) 
{
  register_filesystem(&ptreefs);
  /*kern_mount(&ptreefs);*/
  sys_mount("ptreefs", "/mnt/", "ptreefs", 0, NULL);
 
}

static void ptreefs_cleanup_module(void) 
{
  unregister_filesystem(&ptreefs);
}

module_init(ptreefs_init_module);
module_exit(ptreefs_cleanup_module);

MODULE_LICENSE("GPL");


