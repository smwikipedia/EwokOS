#include <kfile.h>
#include <kstring.h>
#include <system.h>
#include <proc.h>
#include <mm/kmalloc.h>

#define OPEN_MAX 128
static kfile_t _files[OPEN_MAX]; //file info caches 
static int32_t _p_lock = 0;

void kf_init() {
	uint32_t i = 0;
	while(i < OPEN_MAX) {
		_files[i].node_info.node = 0;
		i++;
	}
}
	
//get file info cache by node addr
static inline kfile_t* get_file(uint32_t node_addr, bool add) {
	uint32_t i = 0; 
	int32_t at = -1;
	while(i < OPEN_MAX) {
		if(_files[i].node_info.node == 0)
			at = i; //first free file item
		else if(_files[i].node_info.node == node_addr)
			return &_files[i];
		i++;
	}	

	if(at < 0 || !add)
		return NULL;

	_files[at].node_info.node = node_addr;
	_files[at].ref_r = 0;
	_files[at].ref_w = 0;
	return &_files[at];
}

int32_t kf_get_ref(uint32_t node_addr, uint32_t wr) {
	CRIT_IN(_p_lock)
	int32_t ret = -1;
	kfile_t* kf = get_file(node_addr, false);
	if(kf != NULL) {
		if(wr == 0)//read mode
			ret = kf->ref_r;
		else if(wr == 1)//write mode
			ret = kf->ref_w;
		else //all ref
			ret = kf->ref_w + kf->ref_r;
	}
	CRIT_OUT(_p_lock)
	return ret;
}

void kf_unref(kfile_t* kf, uint32_t wr) {
	if(kf == NULL)
		return;

	CRIT_IN(_p_lock)
	if(wr == 0) {//read mode
		if(kf->ref_r > 0)
			kf->ref_r--;
	}
	else {
		if(kf->ref_w > 0)
			kf->ref_w--;
	}
	if((kf->ref_w + kf->ref_r) == 0) { // close when ref cleared.
		kf->node_info.node = 0;
	}
	CRIT_OUT(_p_lock)
}

void kf_ref(kfile_t* kf, uint32_t wr) {
	if(kf == NULL)
		return;

	CRIT_IN(_p_lock)
	if(wr == 0) //read mode
		kf->ref_r++;
	else
		kf->ref_w++;
	CRIT_OUT(_p_lock)
}

//open file will get file id for this process, and cache the file info
int32_t kf_open(fs_info_t* info, int32_t wr) {
	process_t* proc = _current_proc;
	if(proc == NULL)
		return -1;

	CRIT_IN(_p_lock)
	kfile_t* kf = get_file(info->node, true);
	if(kf == NULL) {
		CRIT_OUT(_p_lock)
		return -1;
	}
	kf_ref(kf, wr);
	memcpy(&kf->node_info, info, sizeof(fs_info_t));

	int32_t i;
	for(i=0; i<FILE_MAX; i++) {
		if(proc->space->files[i].kf == NULL) {
			proc->space->files[i].kf = kf;
			proc->space->files[i].wr = wr;
			proc->space->files[i].seek = 0;
			break;
		}
	}
	if(i >= FILE_MAX) {
		kf_unref(kf, wr);
		i = -1;
	}
	CRIT_OUT(_p_lock)
	return i;
}

void kf_close(int32_t fd) {
	process_t* proc = _current_proc;
	if(proc == NULL || fd < 0 || fd >= FILE_MAX)
		return;
	
	CRIT_IN(_p_lock)
	kf_unref(proc->space->files[fd].kf, proc->space->files[fd].wr);
	proc->space->files[fd].kf = NULL;
	proc->space->files[fd].wr = 0;
	proc->space->files[fd].seek = 0;
	CRIT_OUT(_p_lock)
}

int32_t kf_node_info_by_fd(int32_t fd, fs_info_t* info) {
	CRIT_IN(_p_lock)
	int32_t ret = -1;
	process_t* proc = _current_proc;
	if(proc == NULL || fd < 0 || fd>= FILE_MAX) {
		CRIT_OUT(_p_lock)
		return ret;
	}

	kfile_t* kf = proc->space->files[fd].kf;
	if(kf != NULL) {
		memcpy(info, &kf->node_info, sizeof(fs_info_t));
		ret = 0;
	}
	CRIT_OUT(_p_lock)
	return ret;
}

int32_t kf_node_info_by_addr(uint32_t node_addr, fs_info_t* info) {
	CRIT_IN(_p_lock)
	int32_t ret = -1;
	kfile_t* kf = get_file(node_addr, false);
	if(kf != NULL) {
		memcpy(info, &kf->node_info, sizeof(fs_info_t));
		ret = 0;
	}
	CRIT_OUT(_p_lock)
	return ret;
}

int32_t kf_node_info_update(uint32_t node_addr, fs_info_t* info) {
	CRIT_IN(_p_lock)
	int32_t ret = -1;
	kfile_t* kf = get_file(node_addr, false);
	if(kf != NULL) {
		memcpy(&kf->node_info, info, sizeof(fs_info_t));
		ret = 0;
	}
	CRIT_OUT(_p_lock)
	return ret;
}
