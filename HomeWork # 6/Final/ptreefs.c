#include <ptreefs.h>

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
	
	ptreefs_create_files(sb, ptreefs_root_dentry);
	PTREE_FS_ENABLED = 1;
	
	return 0;
}

static void ptreefs_kill_super(struct super_block *super) {
	/*	kill_litter_super  might be a better option. 
		dont forget to get rid of any allocated stuff. */
	PTREE_FS_ENABLED = 0;
	kill_litter_super(super);
	//kill_anon_super(super);
}


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

static void ptreefs_create_files(struct super_block *sb, struct dentry *root)
{
	struct task_struct *p;
	struct task_struct *parent;	
	
	p = &init_task;
	
	read_lock(&tasklist_lock);
	while ( (p = next_task(p)) != &init_task )	 
	{
		
		if (p == &init_task)
		{
			p->process_ptree_node = ptreefs_create_entry_form_PROCESS(p,sb,root);
		}	
		else
		{
			parent = p->real_parent;
			if (p->real_parent->process_ptree_node != NULL)
				p->process_ptree_node = ptreefs_create_entry_form_PROCESS(p,sb,p->real_parent->process_ptree_node->dir_dentry);
			else
				p->process_ptree_node = ptreefs_create_entry_form_PROCESS(p,sb,root);
		}
		
	}
	read_unlock(&tasklist_lock);	
}


/********** new function relating to homework !6! ************************/

static struct ptreefs_node *ptreefs_create_entry_form_PROCESS(struct task_struct *new_task, struct super_block *sb,struct dentry *parent_process_dir)
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
		directory_name[0] =  (pid / 10000) % 10 +48;	
		directory_name[1] =  (pid / 1000) % 10 +48;
		directory_name[2] =  (pid / 100) % 10 +48;
		directory_name[3] =  (pid / 10) % 10 +48;
		directory_name[4] =  pid % 10 +48;
		directory_name[5] = '\0';
	}
	else 
	if (pid > 999)
	{
		directory_name[0] = (pid / 1000) % 10 +48;
		directory_name[1] = (pid / 100) % 10 +48;
		directory_name[2] = (pid / 10) % 10 +48;
		directory_name[3] =  pid % 10 +48;
		directory_name[4] = '\0';
	}
	else 
	if (pid > 99)
	{
		directory_name[0] =  (pid / 100) % 10 +48;
		directory_name[1] =  (pid / 10) % 10 +48;
		directory_name[2] =  (pid % 10) +48;
		directory_name[3] = '\0';
	}
	else 
	if (pid > 9)
	{
		directory_name[0] = (pid / 10) % 10 +48;
		directory_name[1] = (pid % 10) +48;
		directory_name[2] = '\0';
	}
	else 
	{
		directory_name[0] = pid % 10 +48;
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
	ptreefs_delete_entry(root,father_node,father);
	
	root: ptr to the root directory (dentry)
	father_node: the  ptreefs_node of the  task struct that is exiting. 
	father - task struct that is exiting. 
*/
static void ptreefs_process_exit(struct task_struct * father, struct ptreefs_node *father_node)						 
{
	struct task_struct *p;
	struct list_head *_p, *_n;


	/* if called inside 
		/kernel/exit.c: line 650
		static void exit_notify(struct task_struct *tsk)
	
		all the right locks are held! 
		read_lock(&tasklist_lock); 
		*/
        list_for_each_safe(_p, _n, &father->children) 
		{
			p = list_entry(_p,struct task_struct,sibling);
			ptreefs_move_to_root(ptreefs_root_dentry, p->process_ptree_node);
		}			
		/*  read_unlock(&tasklist_lock);	*/
		
		
		
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
static void ptreefs_precess_exec(char *new_name, struct ptreefs_node *the_node)
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











