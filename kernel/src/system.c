#include <system.h>
#include <types.h>
#include <mm/kalloc.h>
#include <proc.h>
#include <kstring.h>
#include <hardware.h>

#define CPSR_IRQ_INHIBIT (1<<7)
#define CPSR_FIQ_INHIBIT (1<<6)
#define NO_INT  (CPSR_IRQ_INHIBIT | CPSR_FIQ_INHIBIT)
#define DIS_INT CPSR_IRQ_INHIBIT

// disable interrupt
void cli(void) {
	uint32_t val;

	__asm__("mrs %[v], cpsr":[v]"=r"(val)::);
	val |= DIS_INT;
	__asm__("msr cpsr_c, %[v]"::[v]"r"(val):);
}

// enable interrupt
void sti(void) {
	uint32_t val;

	__asm__("mrs %[v], cpsr":[v]"=r"(val)::);
	val &= ~DIS_INT;
	__asm__("msr cpsr_c, %[v]"::[v]"r"(val):);
}

inline void loopd(uint32_t times) {
	while(times > 0)
		times--;
}

static int32_t get_procs_num() {
	int32_t res = 0;
	for(int32_t i=0; i<PROCESS_COUNT_MAX; i++) {
		if(_process_table[i].state != UNUSED && 
				(_current_proc->owner == 0 || 
				_process_table[i].owner == _current_proc->owner)) {
			res++;
		}
	}
	return res;
}

static proc_info_t* get_procs(int32_t *num) {
	*num = get_procs_num();
	if(*num == 0)
		return NULL;

	/*need to be freed later used!*/
	proc_info_t* procs = (proc_info_t*)pmalloc(sizeof(proc_info_t)*(*num));
	if(procs == NULL)
		return NULL;

	int32_t j = 0;
	for(int32_t i=0; i<PROCESS_COUNT_MAX && j<(*num); i++) {
		if(_process_table[i].state != UNUSED && 
				(_current_proc->owner == 0 ||
				 _process_table[i].owner == _current_proc->owner)) {
			procs[j].pid = _process_table[i].pid;	
			procs[j].father_pid = _process_table[i].father_pid;	
			procs[j].owner = _process_table[i].owner;	
			procs[j].heap_size = _process_table[i].space->heap_size;	
			strcpy(procs[j].cmd, _process_table[i].cmd);
			j++;
		}
	}

	*num = j;
	return procs;
}

int32_t system_cmd(int32_t cmd, int32_t arg) {
	int32_t ret = -1;
	switch(cmd) {
	case 0: //get total mem size
		ret = (int32_t)get_phy_ram_size();
		break;
	case 1: //get free mem size
		ret = (int32_t)get_free_mem_size();
		break;
	case 2: //get procs
		ret = (int32_t)get_procs((int32_t*)arg);
		break;
	default:
		break;
	}
	return ret;
}
