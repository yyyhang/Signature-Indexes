// tsig.c ... functions on Tuple Signatures (tsig's)
// part of signature indexed files
// Written by John Shepherd, March 2019

#include <unistd.h>
#include <string.h>
#include "defs.h"
#include "tsig.h"
#include "reln.h"
#include "hash.h"
#include "bits.h"

Bits codewordTuple(char *attr_value, Reln r) {
	Count tm = tsigBits(r);// width of tuple signature (#bits)
	Count tk = codeBits(r);// bits set per attribute
	int counter = 0;
	Bits cw = newBits(tm);
	srandom(hash_any(attr_value, strlen(attr_value)));  // seed
	while (counter < tk) {
		int i = random() % tm;    // random # from 0 to tm-1
		if (!bitIsSet(cw,i)){       // check if set to 1
			setBit(cw,i);
			counter++;
		}   
	}
	return cw; // m-bits with k 1-bits and m-k 0-bits
}

// make a tuple signature

Bits makeTupleSig(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL);
	//TODO
	Bits desc = newBits(tsigBits(r));
	// extract values into an array of strings
	char **A = tupleVals(r, t);
	Count n = nAttrs(r);
	// desc(t) = cw(A1) OR cw(A2) OR ... OR cw(An)
	// maybe not all n attributes are used in descriptor, say (Perryridge, ?, ?, ?)
	for (int i = 0; i < n; i++) {
		// return 0 if same; False is 0
		if (strcmp(A[i],"?")){
			Bits cw = codewordTuple(A[i], r); // get codeword for each attribute
			orBits(desc,cw);
		}
	}
	return desc;
}

// find "matching" pages using tuple signatures

void findPagesUsingTupSigs(Query q)
{
	assert(q != NULL);
	//TODO
	
	// init, AllZeroBits for pages
	unsetAllBits(q->pages); 
	q->nsigpages = 0;
	q->nsigs = 0;
	
	// generate the tsig from query
	Reln r = q->rel;
	Tuple qrt = q->qstring;
	Bits query_sig = makeTupleSig(r,qrt);
	// get the tsig from file of each page
	File tsig_pages = tsigFile(r);
	Count ntsig = nTsigPages(r); //  number of tsig pages
	Count tm = tsigBits(r); // width of tsig
	Count tupPP = maxTupsPP(r);
	
	// iterate all items in each page
	for (q->curpage = 0; q->curpage < ntsig; q->curpage++) {
		Page curr = getPage(tsig_pages, q->curpage); // (File f, PageID pid) => Page
		Count npitem = pageNitems(curr); // number of items in this page
		for (q->curtup = 0; q->curtup < npitem; q->curtup++) {
			Bits curr_sig = newBits(tm);
			getBits(curr, q->curtup, curr_sig); // get nth item in page, and put in curr_sig
			if (isSubset(query_sig, curr_sig)){
				// include PID in Pages, which is nth page in the data file
				// To include PID in pages means setting the corresponding bit in Pages to 1
					// showBits(query_sig);
					// printf("\n");
					// showBits(curr_sig);
					// printf("\n");
				Count pid = q->nsigs / tupPP;
				// printf("pid: %d\n",pid);
				setBit(q->pages, pid);
			}
			q->nsigs++; // next item
		}
		q->nsigpages++; // next page
	}	
	

	// The printf below is primarily for debugging
	// Remove it before submitting this function
	// printf("Matched Pages:"); showBits(q->pages); putchar('\n');
}
