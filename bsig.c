// bsig.c ... functions on Tuple Signatures (bsig's)
// part of signature indexed files
// Written by John Shepherd, March 2019

#include "defs.h"
#include "reln.h"
#include "query.h"
#include "bsig.h"
#include "psig.h"

void findPagesUsingBitSlices(Query q)
{
	assert(q != NULL);
	//TODO
	// init, AllZeroBits for pages
	// unsetAllBits(q->pages); 
	// q->nsigpages = 0;
	// q->nsigs = 0;
	
	// we need to get the psig of query, and then compare it with all bsig
	Reln r = q->rel;
	Tuple qrt = q->qstring;
	Bits query_sig = makePageSig(r,qrt);
	// showBits(query_sig);
	// printf("\n");
	setAllBits(q->pages);
	// we need to get the psig from file
	File bsig_pages = bsigFile(r);
	// Count psigNpages = nPsigPages(r); //  number of page signatures (psigs)
	
	Count bm = bsigBits(r); // bm == npages
	Count pm = psigBits(r); //width of page sig
	Count bsigPP = maxBsigsPP(r);
	
	PageID pid = -1;
	
	// iterate; exclusion; similar with human's method, find vertically
	// 1. find ones in qsig; 
	// 2. for every one in qsig, find corresponding bit slices (pid & position), which is rotated
	// 3. in that bit slice, find which are not one, and exclude that page.
	for (int ith = 0; ith < pm; ith++) {
		if (bitIsSet(query_sig,ith)){
			// check if in same bsig page
			int pth = ith / maxBsigsPP(r);
			if (pth != pid) {
				q->nsigpages++;
				pid = pth;
			}
			q->curpage = ith / bsigPP;
			Page curr = getPage(bsig_pages, q->curpage); // (File f, PageID pid)
			Bits curr_sig = newBits(bm);
			q->curtup = ith % bsigPP;
			getBits(curr, q->curtup, curr_sig);
			for (int pth = 0; pth < (nPages(r)); pth++){
				if(!bitIsSet(curr_sig, pth)){
					unsetBit(q->pages, pth);
				}
			}
			q->nsigs++;
			free(curr);
		}
	}
	// showBits(q->pages);
	// printf("\n");
	free(query_sig);
}

