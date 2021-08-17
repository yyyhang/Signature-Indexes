// query.c ... query scan functions
// part of signature indexed files
// Manage creating and using Query objects
// Written by John Shepherd, March 2019

#include "defs.h"
#include "query.h"
#include "reln.h"
#include "tuple.h"
#include "bits.h"
#include "tsig.h"
#include "psig.h"
#include "bsig.h"

// check whether a query is valid for a relation
// e.g. same number of attributes

int checkQuery(Reln r, char *q)
{
	if (*q == '\0')
		return 0;
	char *c;
	int nattr = 1;
	for (c = q; *c != '\0'; c++)
		if (*c == ',')
			nattr++;
	return (nattr == nAttrs(r));
}

// take a query string (e.g. "1234,?,abc,?")
// set up a QueryRep object for the scan

Query startQuery(Reln r, char *q, char sigs)
{
	Query new = malloc(sizeof(QueryRep));
	assert(new != NULL);
	if (!checkQuery(r, q))
		return NULL;
	new->rel = r;
	new->qstring = q;
	new->nsigs = new->nsigpages = 0;
	new->ntuples = new->ntuppages = new->nfalse = 0;
	new->pages = newBits(nPages(r));
	switch (sigs)
	{
	case 't':
		findPagesUsingTupSigs(new);
		break;
	case 'p':
		findPagesUsingPageSigs(new);
		break;
	case 'b':
		findPagesUsingBitSlices(new);
		break;
	default:
		setAllBits(new->pages);
		break;
	}
	new->curpage = 0;
	return new;
}

// scan through selected pages (q->pages)
// search for matching tuples and show each
// accumulate query stats

void scanAndDisplayMatchingTuples(Query q)
{
	assert(q != NULL);
	//TODO
	// init
	q->ntuppages = 0;
	// scan selected pages to find matching tuples
	Reln r = q->rel;
	Tuple qry = q->qstring;
	Bits pages = q->pages;
	File dataf = dataFile(r);
	Count npages = nPages(r); // number of data pages
	// showBits(pages);
	// printf("\n");
	
	for (q->curpage = 0; q->curpage < npages; q->curpage++){
		if (!bitIsSet(pages, q->curpage)) {
			continue;
		}
		q->ntuppages++;
		Bool miss = TRUE;
		Page curr = getPage(dataf,q->curpage);
		Count nitems = pageNitems(curr);
		
		for (q->curtup=0;q->curtup<nitems;q->curtup++){
			// get data for i'th tuple in Page (Reln r, Page p, int i)
			q->ntuples++;
			Tuple t2 = getTupleFromPage(r, curr, q->curtup); 
			if (tupleMatch(r,qry,t2)){
				showTuple(r,t2);
				miss =FALSE;
				free(t2);
			}
		}
		if (miss == TRUE) {
			q->nfalse++;
		}
		free(curr);
	}
	
}

// print statistics on query

void queryStats(Query q)
{
	printf("# sig pages read:    %d\n", q->nsigpages);
	printf("# signatures read:   %d\n", q->nsigs);
	printf("# data pages read:   %d\n", q->ntuppages);
	printf("# tuples examined:   %d\n", q->ntuples);
	printf("# false match pages: %d\n", q->nfalse);
}

// clean up a QueryRep object and associated data

void closeQuery(Query q)
{
	free(q->pages);
	free(q);
}
