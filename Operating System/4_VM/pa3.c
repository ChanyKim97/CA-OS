/**********************************************************************
 * Copyright (c) 2020-2021
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "types.h"
#include "list_head.h"
#include "vm.h"

/**
 * Ready queue of the system
 */
extern struct list_head processes;

/**
 * Currently running process
 */
extern struct process *current;

/**
 * Page Table Base Register that MMU will walk through for address translation
 */
extern struct pagetable *ptbr;

/**
 * TLB of the system.
 */
extern struct tlb_entry tlb[1UL << (PTES_PER_PAGE_SHIFT * 2)];


/**
 * The number of mappings for each page frame. Can be used to determine how
 * many processes are using the page frames.
 */
extern unsigned int mapcounts[];


/**
 * lookup_tlb(@vpn, @pfn)
 *
 * DESCRIPTION
 *   Translate @vpn of the current process through TLB. DO NOT make your own
 *   data structure for TLB, but use the defined @tlb data structure
 *   to translate. If the requested VPN exists in the TLB, return true
 *   with @pfn is set to its PFN. Otherwise, return false.
 *   The framework calls this function when needed, so do not call
 *   this function manually.
 *
 * RETURN
 *   Return true if the translation is cached in the TLB.
 *   Return false otherwise
 */
bool lookup_tlb(unsigned int vpn, unsigned int *pfn)
{
	for (int i = 0; i < NR_TLB_ENTRIES; i++) {
		//�ʱⰪ�� 0 0�̶� ó���ʿ���
		if (tlb[i].valid == true) {
			if (tlb[i].vpn == vpn) {
				*pfn = tlb[i].pfn;
				return true;
			}
		}
	}
	return false;
}


/**
 * insert_tlb(@vpn, @pfn)
 *
 * DESCRIPTION
 *   Insert the mapping from @vpn to @pfn into the TLB. The framework will call
 *   this function when required, so no need to call this function manually.
 *
 */
void insert_tlb(unsigned int vpn, unsigned int pfn)
{
	for (int i = 0; i < NR_TLB_ENTRIES; i++) {
		if (!tlb[i].valid) {
			tlb[i].valid = true;
			tlb[i].vpn = vpn;
			tlb[i].pfn = pfn;
			return;
		}
	}
}


/**
 * alloc_page(@vpn, @rw)
 *
 * DESCRIPTION
 *   Allocate a page frame that is not allocated to any process, and map it
 *   to @vpn. When the system has multiple free pages, this function should
 *   allocate the page frame with the **smallest pfn**.
 *   You may construct the page table of the @current process. When the page
 *   is allocated with RW_WRITE flag, the page may be later accessed for writes.
 *   However, the pages populated with RW_READ only should not be accessed with
 *   RW_WRITE accesses.
 *
 * RETURN
 *   Return allocated page frame number.
 *   Return -1 if all page frames are allocated.
 */
unsigned int alloc_page(unsigned int vpn, unsigned int rw)
{
	//NR_PTES_PER_PAGE 4��� ����, two-level
	//pd_index =>	�� 0..
	//				�� 4..
	//				�� 8..
	//				�� 12..
	//vpn 10�̸� vpn / 4 => 2, vpn%4 => 2
	//3���� pd 3���� pte��

	//vpn / 16 => outer page directory
	//vpn % 16 => page table entry
	int pd_index = vpn / NR_PTES_PER_PAGE;
	int pte_index = vpn % NR_PTES_PER_PAGE;

	//���ۺκ�
	ptbr = &current->pagetable;
	//outer page ����ִٸ� �Ҵ�
	if (ptbr->outer_ptes[pd_index] == NULL) {
		ptbr->outer_ptes[pd_index] = malloc(sizeof(struct pte_directory));
	}

	//outer���� �ش��ϴ� frameã�Ƶ��� pte�� �ش��ϴ� ������
	struct pte* pte = &ptbr->outer_ptes[pd_index]->ptes[pte_index];

	//pte ä���
	pte->valid = true;
	if (rw == RW_READ) {
		pte->writable = false;
		pte->private = false;
	}
	else {
		pte->writable = true;
		pte->private = true;
	}
	for (int _pfn = 0; _pfn < NR_PAGEFRAMES; _pfn++) {
		//���� �������
		if (mapcounts[_pfn] == 0) {
			pte->pfn = _pfn;
			mapcounts[_pfn]++;
			return _pfn;
		}
	}

	return -1;
}


/**
 * free_page(@vpn)
 *
 * DESCRIPTION
 *   Deallocate the page from the current processor. Make sure that the fields
 *   for the corresponding PTE (valid, writable, pfn) is set @false or 0.
 *   Also, consider carefully for the case when a page is shared by two processes,
 *   and one process is to free the page.
 */
void free_page(unsigned int vpn)
{
	int pd_index = vpn / NR_PTES_PER_PAGE;
	int pte_index = vpn % NR_PTES_PER_PAGE;

	ptbr = &current->pagetable;
	struct pte* pte = &ptbr->outer_ptes[pd_index]->ptes[pte_index];

	mapcounts[pte->pfn]--;
	pte->pfn = 0;
	pte->valid = false;
	pte->writable = false;
	pte->private = false;

	for (int i = 0; i < NR_TLB_ENTRIES; i++) {
		if (tlb[i].vpn == vpn) {
			tlb[i].valid = false;
			tlb[i].vpn = 0;
			tlb[i].pfn = 0;
		}
	}
}


/**
 * handle_page_fault()
 *
 * DESCRIPTION
 *   Handle the page fault for accessing @vpn for @rw. This function is called
 *   by the framework when the __translate() for @vpn fails. This implies;
 *   0. page directory is invalid
 *   1. pte is invalid
 *   2. pte is not writable but @rw is for write
 *   This function should identify the situation, and do the copy-on-write if
 *   necessary.
 *
 * RETURN
 *   @true on successful fault handling
 *   @false otherwise
 */
bool handle_page_fault(unsigned int vpn, unsigned int rw)
{
	//return ret about page fault!
	//table access �ȵǴ�
	int pd_index = vpn / NR_PTES_PER_PAGE;
	int pte_index = vpn % NR_PTES_PER_PAGE;
	ptbr = &current->pagetable;
	struct pte* pte = &ptbr->outer_ptes[pd_index]->ptes[pte_index];

	//0. page diretory is invalid
	if (ptbr->outer_ptes[pd_index] == NULL) return false;

	//1. pte is invalid
	if (pte->valid == false) return false;

	//2. pte is not writable but @rw is for write
	if (pte->private == true) {
		if (mapcounts[pte->pfn] >= 2) {
			//���� �������
			for (int i = 0; i < NR_PAGEFRAMES; i++) {
				if (mapcounts[i] == 0) {
					//���� pfn -1
					mapcounts[pte->pfn]--;
					pte->private = false;
					//���ο� ���� �Ҵ�
					pte->pfn = i;
					pte->writable = true;
					mapcounts[i]++;
					return true;
				}
			}
		}
		else if (mapcounts[pte->pfn] == 1) {
			pte->writable = true;
			pte->private = false;
			return true;
		}
	}

	return false;
}


/**
 * switch_process()
 *
 * DESCRIPTION
 *   If there is a process with @pid in @processes, switch to the process.
 *   The @current process at the moment should be put into the @processes
 *   list, and @current should be replaced to the requested process.
 *   Make sure that the next process is unlinked from the @processes, and
 *   @ptbr is set properly.
 *
 *   If there is no process with @pid in the @processes list, fork a process
 *   from the @current. This implies the forked child process should have
 *   the identical page table entry 'values' to its parent's (i.e., @current)
 *   page table. 
 *   To implement the copy-on-write feature, you should manipulate the writable
 *   bit in PTE and mapcounts for shared pages. You may use pte->private for 
 *   storing some useful information :-)
 */
void switch_process(unsigned int pid)
{
	struct process* tmp = NULL;
	struct process* tmpN = NULL;

	//*************** ���߿� TLB ó���� ������� **********************
	for (int i = 0; i < NR_TLB_ENTRIES; i++) {
		tlb[i].valid = false;
		tlb[i].vpn = 0;
		tlb[i].pfn = 0;
	}

	//pid�� �ִ� process��� �ٷ� process switch
	list_for_each_entry_safe(tmp, tmpN, &processes, list) {
		if (tmp->pid == pid) {
			//���� process ����, pid�� process ��⿡�� ����, ���� process ����, ptbr�� ����� ���μ�����
			list_add_tail(&current->list, &processes);
			list_del_init(&tmp->list);
			current = tmp;
			ptbr = &current->pagetable;
			return;
		}
	}

	//���� ��� @current���� fork
	struct process* forked = malloc(sizeof(struct process));
	forked->pid = pid;

	for (int i = 0; i < NR_PTES_PER_PAGE; i++) {
		//�����ִ� �� �״�� �����Ҵ�
		if (current->pagetable.outer_ptes[i]) {
			forked->pagetable.outer_ptes[i] = malloc(sizeof(struct pte_directory));
			//2 level
			//for���� if�� �ۿ��� �ؼ� �Ҵ�ȵȰ����� ���Ƽ� segmentation fault
			for (int j = 0; j < NR_PTES_PER_PAGE; j++) {
				if (current->pagetable.outer_ptes[i]->ptes[j].valid) {
					forked->pagetable.outer_ptes[i]->ptes[j].valid = true;
					current->pagetable.outer_ptes[i]->ptes[j].writable = false;
					forked->pagetable.outer_ptes[i]->ptes[j].writable = false;
					forked->pagetable.outer_ptes[i]->ptes[j].pfn = current->pagetable.outer_ptes[i]->ptes[j].pfn;
					//private true�� �Ǵ� ���� �Ŀ� cow �ٸ� frame���� ó������ ��
					forked->pagetable.outer_ptes[i]->ptes[j].private = current->pagetable.outer_ptes[i]->ptes[j].private;
					mapcounts[forked->pagetable.outer_ptes[i]->ptes[j].pfn]++;
				}
			}
		}
	}

	list_add_tail(&current->list, &processes);
	current = forked;
	ptbr = &current->pagetable;
	return;
}

