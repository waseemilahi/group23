/* the following can be moved to ptreefs.h to make things look better: */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h> 	/* PAGE_CACHE_SIZE */
#include <linux/fs.h>     	/* This is where libfs stuff is declared */
#include <asm/atomic.h>
#include <asm/uaccess.h>	/* copy_to_user */
#include <linux/sched.h>

/* static stuff : */
#define PTREEFS_MAGIC 9999999	/* our magic number */
#define FILE_PERMISSIONS	0444;
#define DIRECTORY_PERMISSIONS 0555;
#define MAX_FILE_LEN	16

/* since all files are going to be created internally, we should keep some impirtant pointers here so we can reference the root dir and superblock from within out code. (normally, the VFS would have provided us with those... */
struct super_block 	*ptreefs_super_block;
struct inode 		*ptreefs_root_inode;
struct dentry 				*ptreefs_root_dentry;

static struct super_block *ptreefs_get_super(struct file_system_type *fst,int flags, const char *devname, void *data);
static int ptreefs_fill_super (struct super_block *sb, void *data, int silent);
static void ptreefs_kill_super(struct super_block *super);
static struct inode *ptreefs_make_inode(struct super_block *sb, int mode);
static struct dentry *ptreefs_create_dir (struct super_block *sb, struct dentry *parent, const char *name);
static struct dentry *ptreefs_create_file (struct super_block *sb, struct dentry *dir, const char *name);
static void ptreefs_create_files (struct super_block *sb, struct dentry *root);
struct ptreefs_node *ptreefs_create_entry(pid_t pid, struct super_block *sb, struct dentry *parent_process_dir);

/* functions for in kernel implementation !!!!!!!!!!!!!! *****************************************/
/* static void ptreefs_delete_entry(pid_t pid, struct super_block *sb, struct dentry *parent_process_dir);*/
void ptreefs_delete_entry(struct dentry *root, struct ptreefs_node *father_node, struct task_struct * father);

void ptreefs_move_to_root(struct dentry *root, struct ptreefs_node *the_node);
void ptreefs_rename_node(char *new_name, struct ptreefs_node *the_node);
struct ptreefs_node *ptreefs_create_entry_form_PROCESS(struct task_struct *new_task, struct super_block *sb);

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
/* the code */

/* everyone needs a superblock for their file system: */
static struct super_block *ptreefs_get_super(struct file_system_type *fst,int flags, const char *devname, void *data)
{
	/* lets pass off the hard work of creating a superblock to someone else : */
	ptreefs_super_block = get_sb_single(fst, flags, data, ptreefs_fill_super);
	/*
		fst:			file system type. defined by us in the .h section.
		flags, 		generic
		data, 			generic	
		ptreefs_fill_super	a function defined by us. 
	*/
	return ptreefs_super_block;
	/* get sb single doesnt really do ALL the work for us, we still need to help it out a bit: with our function to fill the superblock */
}

/* the ptreefs_get_super function is dependand on  : */
static int ptreefs_fill_super (struct super_block *sb, void *data, int silent)
{
	/* we have a SB that needs some fields set : */
	sb->s_blocksize = 		PAGE_CACHE_SIZE;
	sb->s_blocksize_bits = 	PAGE_CACHE_SHIFT;
	sb->s_magic = 			PTREEFS_MAGIC;		/* defined by us */
	
	sb->s_op = &ptreefs_superblock_ops; 		/* defined by us */
	sb->s_type = &ptreefs; 				/* defined by us */
	
	ptreefs_root_inode = ptreefs_make_inode (sb, S_IFDIR | 0555);	/* ptreefs_make_inode defined by us, arguments: directory with 0755 permission */
	if (!ptreefs_root_inode) return -ENOMEM;
	
	ptreefs_root_inode->i_op = &ptreefs_dir_inode_operations;	/* the inode operationsdefined by us */
	ptreefs_root_inode->i_fop = &ptreefs_dir_operations;		/* the directory operations defined by us */

	/* now lets make a dentry for the root directory */
	ptreefs_root_dentry = d_alloc_root(ptreefs_root_inode);		/* a dentry for the root directory... */
	if (! ptreefs_root_dentry) iput(ptreefs_root_inode);	/* err checking and undo */
	
	sb->s_root = ptreefs_root_dentry;							/* let the superblock know we have a root directory */
	
	(find_task_by_pid(1))->process_ptree_node = ptreefs_create_entry((pid_t)1,sb,ptreefs_root_dentry);

	 //	ptreefs_create_files(sb, ptreefs_root_dentry);
	
	return 0;
}

static void ptreefs_kill_super(struct super_block *super) {
	/*	kill_litter_super  might be a better option. 
		dont forget to get rid of any allocated stuff. */
	kill_litter_super(super);
	//kill_anon_super(super);
}

/******** totally temp *****************/
static struct inode *ptreefs_make_inode(struct super_block *sb, int mode)
{
	struct inode *ret = new_inode(sb);

	if (ret) {
		ret->i_mode = mode;
		ret->i_uid = ret->i_gid = 0;
		ret->i_blksize = PAGE_CACHE_SIZE;
		ret->i_blocks = 0;
		ret->i_atime = ret->i_mtime = ret->i_ctime = CURRENT_TIME;
	}
	return ret;
}

static struct dentry *ptreefs_create_dir (struct super_block *sb, struct dentry *parent, const char *name)
{
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;

	qname.name = name;
	qname.len = strlen (name);
	qname.hash = full_name_hash(name, qname.len);
	dentry = d_alloc(parent, &qname);					/* create a dentry for the directory */
	if (! dentry) goto out;

	inode = ptreefs_make_inode(sb, S_IFDIR | 0555);		/* now lets make an inode... for that directory */
	if (! inode) goto out_dput;
	inode->i_op = &ptreefs_dir_inode_operations;
	inode->i_fop = &ptreefs_dir_operations;
	

	d_add(dentry, inode);								/* associate the inode with the dentry */
	return dentry;										/* were done, return the new directory */

  out_dput:
	dput(dentry);
  out:
	return 0;
}

static struct dentry *ptreefs_create_file (struct super_block *sb, struct dentry *dir, const char *name)
{
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;

	qname.name = name;
	qname.len = strlen (name);
	qname.hash = full_name_hash(name, qname.len);
	
	dentry = d_alloc(dir, &qname);
	if (! dentry)
		goto out;
	inode = ptreefs_make_inode(sb, S_IFREG | 0444);
	if (! inode)
		goto out_dput;
	inode->i_fop = &ptreefs_file_ops;
		
	d_add(dentry, inode);
	return dentry;

	out_dput:
		dput(dentry);
	out:
		return 0;
}

static void ptreefs_create_files (struct super_block *sb, struct dentry *root)
{
	struct ptreefs_node* arr[4];
	int i;
	char *name = "new_name";
	
/*
 * One counter in the top-level directory.
 */
	ptreefs_create_file(sb, root, "root_process_name.a");
/*
 * And one in a subdirectory.
 */
	/*
	subdir = ptreefs_create_dir(sb, root, "A_process");
	if (subdir) ptreefs_create_file(sb, subdir, "process_name.a");
	*/
	
	for (i=0;i<4;i++)
	{
		if (i == 0) arr[i] = ptreefs_create_entry((pid_t)1, sb, root);
		else 
			{
				arr[i] = ptreefs_create_entry((pid_t)i+1, sb, arr[i-1]->dir_dentry);
			}
	}
	
	ptreefs_rename_node(name,arr[1]);
	
}

/****** end temp **************/

/********** new function relating to homework !6! ************************/

struct ptreefs_node *ptreefs_create_entry(pid_t pid, struct super_block *sb, struct dentry *parent_process_dir)
{
	/* get the task! */
	char file_name[MAX_FILE_LEN+5];
	char directory_name[6];
	int i;
	struct task_struct *tsk;

	char c;
	struct ptreefs_node *result =  kmalloc(sizeof(struct ptreefs_node), GFP_KERNEL);
	if (!result)
		return NULL;

	read_lock(&tasklist_lock);
	
	tsk = find_task_by_pid(pid);
	if (!tsk) 
	{
	  read_unlock(&tasklist_lock);
	  return NULL;
	}
	
	get_task_struct(tsk);

	for (i=0;i<MAX_FILE_LEN;i++)
	{
		c = tsk->comm[i];
		if (c == '\0') break;
		if (c == (char)47) file_name[i] = '_';
		else file_name[i] = c;		
	}
	read_unlock(&tasklist_lock);
	
	if (pid > 9999)
	{
		directory_name[0] = (char)(((pid / 10000) % 10) +48);	
		pid = pid/10;
		directory_name[1] = (char)(((pid / 1000) % 10) +48);
		pid = pid/10;
		directory_name[2] = (char)(((pid / 100) % 10) +48);
		pid = pid/10;
		directory_name[3] = (char)(((pid / 10) % 10) +48);
		pid = pid/10;
		directory_name[4] = (char)(pid % 10) +48;
		directory_name[5] = '\0';
	}
	else 
	if (pid > 999)
	{
		directory_name[0] = (char)(((pid / 1000) % 10) +48);
		pid = pid/10;
		directory_name[1] = (char)(((pid / 100) % 10) +48);
		pid = pid/10;
		directory_name[2] = (char)(((pid / 10) % 10) +48);
		pid = pid/10;
		directory_name[3] = (char)(pid % 10) +48;
		directory_name[4] = '\0';
	}
	else 
	if (pid > 99)
	{
		directory_name[0] = (char)(((pid / 100) % 10) +48);
		pid = pid/10;
		directory_name[1] = (char)(((pid / 10) % 10) +48);
		pid = pid/10;
		directory_name[2] = (char)(pid % 10) +48;
		directory_name[3] = '\0';
	}
	else 
	if (pid > 9)
	{
		directory_name[0] = (char)(((pid / 10) % 10) +48);
		pid = pid/10;
		directory_name[1] = (char)(pid % 10) +48;
		directory_name[2] = '\0';
	}
	else 
	{
		directory_name[0] = (char)(pid % 10) +48;
		directory_name[1] = '\0';
	}
	
	/* k we got the tasks name lets make a directory for it */
	 /* the dir we are creating */
	
	result->dir_dentry = ptreefs_create_dir(sb, parent_process_dir, directory_name);
		
	file_name[i] = '.';
	file_name[i+1] = 'n';
	file_name[i+2] = 'a';
	file_name[i+3] = 'm';
	file_name[i+4] = 'e';
	file_name[i+5] = '\0';	
	
	if (result->dir_dentry) 
		result->file_dentry = ptreefs_create_file(sb, result->dir_dentry, file_name);	
	
	
	result->file_inode = result->file_dentry->d_inode;
	result->dir_inode = result->dir_dentry->d_inode;
	
	return result;
}

/*
	usage: 
	
	process_ptree_node =  ptreefs_create_entry_form_PROCESS(new_task, sb, parent_process_dir);
	
	new_task: 			ptr to the new task. 
	sb	     : 			ptr to ptreefs's SuperBlock
*/
struct ptreefs_node *ptreefs_create_entry_form_PROCESS(struct task_struct *new_task, struct super_block *sb)
{
	/* get the task! */
	char file_name[MAX_FILE_LEN+5];
	char directory_name[6];
	int i;
	char c;
	int pid;
	
	struct ptreefs_node *result =  kmalloc(sizeof(struct ptreefs_node), GFP_KERNEL);
	if (!result)
		return NULL;

	for (i=0;i<MAX_FILE_LEN;i++)
	{
		c = new_task->comm[i];
		if (c == '\0') break;
		if (c == (char)47) file_name[i] = '_';
		else file_name[i] = c;		
	}
	
	pid = (int)new_task->pid;
	
	if (pid > 9999)
	{
		directory_name[0] = (char)(((pid / 10000) % 10) +48);	
		pid = pid/10;
		directory_name[1] = (char)(((pid / 1000) % 10) +48);
		pid = pid/10;
		directory_name[2] = (char)(((pid / 100) % 10) +48);
		pid = pid/10;
		directory_name[3] = (char)(((pid / 10) % 10) +48);
		pid = pid/10;
		directory_name[4] = (char)(pid % 10) +48;
		directory_name[5] = '\0';
	}
	else 
	if (pid > 999)
	{
		directory_name[0] = (char)(((pid / 1000) % 10) +48);
		pid = pid/10;
		directory_name[1] = (char)(((pid / 100) % 10) +48);
		pid = pid/10;
		directory_name[2] = (char)(((pid / 10) % 10) +48);
		pid = pid/10;
		directory_name[3] = (char)(pid % 10) +48;
		directory_name[4] = '\0';
	}
	else 
	if (pid > 99)
	{
		directory_name[0] = (char)(((pid / 100) % 10) +48);
		pid = pid/10;
		directory_name[1] = (char)(((pid / 10) % 10) +48);
		pid = pid/10;
		directory_name[2] = (char)(pid % 10) +48;
		directory_name[3] = '\0';
	}
	else 
	if (pid > 9)
	{
		directory_name[0] = (char)(((pid / 10) % 10) +48);
		pid = pid/10;
		directory_name[1] = (char)(pid % 10) +48;
		directory_name[2] = '\0';
	}
	else 
	{
		directory_name[0] = (char)(pid % 10) +48;
		directory_name[1] = '\0';
	}
	
	/* k we got the tasks name lets make a directory for it */
	 /* the dir we are creating */
	
	result->dir_dentry = ptreefs_create_dir(sb, new_task->real_parent->process_ptree_node->dir_dentry,directory_name);
		
	file_name[i] = '.';
	file_name[i+1] = 'n';
	file_name[i+2] = 'a';
	file_name[i+3] = 'm';
	file_name[i+4] = 'e';
	file_name[i+5] = '\0';	
	
	if (result->dir_dentry) 
		result->file_dentry = ptreefs_create_file(sb, result->dir_dentry, file_name);	
	
	
	result->file_inode = result->file_dentry->d_inode;
	result->dir_inode = result->dir_dentry->d_inode;
	
	return result;
}


/* 
	ptreefs_delete_entry(root,father_node,father);
	
	root: ptr to the root directory (dentry)
	father_node: the  ptreefs_node of the  task struct that is exiting. 
	father - task struct that is exiting. 
*/
static void ptreefs_delete_entry(struct dentry *root, struct ptreefs_node *father_node, struct task_struct * father)						 
{
	struct task_struct *p;
	struct list_head *_p, *_n;
 
         
        list_for_each_safe(_p, _n, &father->children) 
		{
			p = list_entry(_p,struct task_struct,sibling);
			ptreefs_move_to_root(root, p->process_ptree_node);
		}	
		
		father_node->file_dentry->d_inode = NULL;
		father_node->dir_dentry->d_inode = NULL;
		
		dput(father_node->file_dentry);
		dput(father_node->dir_dentry);
		/* d_delete maybe? */
		
		iput(father_node->file_inode);
		iput(father_node->dir_inode);	
	/* umm delete the entry. and free the node! */	
}

static void ptreefs_move_to_root(struct dentry *root, struct ptreefs_node *the_node)
{
	struct dentry *new_dentry;
	struct dentry *tmp = new_dentry;
	
	new_dentry = d_alloc(root, &the_node->dir_dentry->d_name);	
	if (new_dentry)
	{
		d_move(the_node->dir_dentry, new_dentry);
		the_node->dir_dentry = tmp;
		the_node->dir_inode = the_node->dir_dentry->d_inode;//tmp change: no approval from Ben :)
	}
}

/*
	to be used in EXEC ( after the new name is given to the process!!! ) 
	ptreefs_rename_node(new_name, the_node)
	
	new_name 	= should be : current->comm
	the node 	= current->process_ptree_node
	
	that simple
*/
static void ptreefs_rename_node(char *new_name, struct ptreefs_node *the_node)
{
	struct qstr qname;
	struct dentry *new_dentry;
	struct dentry *tmp = new_dentry;
	int i; char c;
	char file_name[MAX_FILE_LEN+5];
	
	for (i=0;i<MAX_FILE_LEN;i++)
	{
		c = new_name[i];
		if (c == '\0') break;
		if (c == (char)47) file_name[i] = '_';
		else file_name[i] = c;		
	}
	file_name[i] = '.';
	file_name[i+1] = 'n';
	file_name[i+2] = 'a';
	file_name[i+3] = 'm';
	file_name[i+4] = 'e';
	file_name[i+5] = '\0';	
	
	
	qname.name = file_name;
	qname.len = strlen (file_name);
	qname.hash = full_name_hash(file_name, qname.len);
	
	new_dentry = d_alloc(the_node->file_dentry->d_parent, &qname);
	if (new_dentry)
	{
		d_move(the_node->file_dentry, new_dentry);
		the_node->file_dentry = tmp;
		the_node->dir_inode = the_node->file_dentry->d_inode;
	}
}


/**************end new function relating to homework5***************************************/

/* these will change if we decide to have this inside the kernel. currently its defined to work as a mofule : */
/*******************************************************************************************/
int ptreefs_init_module(void) 
{
  return register_filesystem(&ptreefs);
}

static void ptreefs_cleanup_module(void) 
{
  unregister_filesystem(&ptreefs);
}

module_init(ptreefs_init_module);
module_exit(ptreefs_cleanup_module);

MODULE_LICENSE("GPL");