// psig.c ... functions on page signatures (psig's)
// part of signature indexed files
// Written by John Shepherd, March 2019

#include "defs.h"
#include "reln.h"
#include "query.h"
#include "psig.h"
#include "hash.h"

// The srandom() function sets its argument as the seed for a new sequence of pseudo-random integers to be 
// returned by random(). These sequences are repeatable by calling srandom() with the same seed value. If no 
// seed value is provided, the random() function is automatically seeded with a value of 1.

// NB: Bits more than one byte, we cannot simply use |=

Bits codeword(char *attr_value, Reln r) {
	Count pm = psigBits(r);// width of page signature (#bits)
	Count tk = codeBits(r);// bits set per attribute
	int counter = 0;
	Bits cw = newBits(pm);
	srandom(hash_any(attr_value, strlen(attr_value)));  // seed
	while (counter < tk) {
		int i = random() % pm;    // random # from 0 to pm-1
		if (!bitIsSet(cw,i)){       // check if set to 1
			setBit(cw,i);
			counter++;
		}   
	}
	return cw; // m-bits with k 1-bits and m-k 0-bits
}

Bits makePageSig(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL);
	//TODO
	// SIMC
	Bits desc = newBits(psigBits(r));
	char **A = tupleVals(r, t);// extract values into an array of strings
	Count n = nAttrs(r);// number of attributes
	// desc(t) = cw(A1) OR cw(A2) OR ... OR cw(An)
	// maybe not all n attributes are used in descriptor, say (Perryridge, ?, ?, ?)
	for (int i = 0; i < n; i++) {
		// return 0 if same; 0 is false in our case
		if (strcmp(A[i],"?")){
			Bits cw = codeword(A[i], r); // get codeword for each attribute
			orBits(desc,cw);
		}
	}
	return desc;
}

void findPagesUsingPageSigs(Query q)
{
	assert(q != NULL);
	//TODO
	// init, AllZeroBits for pages
	unsetAllBits(q->pages); 
	q->nsigpages = 0;
	q->nsigs = 0;
	
	// we need to get the psig of query, and then compare it with all psig
	Reln r = q->rel;
	Tuple qrt = q->qstring;
	Bits query_sig = makePageSig(r,qrt);
	
	// we need to get the psig from file
	File psig_pages = psigFile(r);
	Count psigNpages = nPsigPages(r); //  number of page signatures (psigs)
	Count pm = psigBits(r); // width of psig
	
	// iterate all items in each page
	for (q->curpage = 0; q->curpage < psigNpages; q->curpage++) {
		Page curr = getPage(psig_pages, q->curpage); // (File f, PageID pid) => Page
		Count npitem = pageNitems(curr); // number of items in this page
		for (q->curtup = 0; q->curtup < npitem; q->curtup++) {
			Bits curr_sig = newBits(pm);
			// unsetAllBits(curr_sig); seems no need to do this
			// (Byte *)(&(p->items[0]) + size*off); so no need to consider nitems in the memory
			// get nth item in the page, and put in curr_sig
			getBits(curr, q->curtup, curr_sig); 
			if (isSubset(query_sig, curr_sig)){
				// include PID in Pages
				// first page is zero
				// showBits(query_sig);
				// printf("\n");
				// showBits(curr_sig);
				// printf("\n");
				setBit(q->pages, q->nsigs);
				// printf("pid: %d\n",q->nsigs);
			}
			q->nsigs++;
			// printf("pid: %d\n",q->nsigs);
		}
		q->nsigpages++;
		free(curr);
	}
}

