// reln.c ... functions on Relations
// part of signature indexed files
// Written by John Shepherd, March 2019

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "defs.h"
#include "reln.h"
#include "page.h"
#include "tuple.h"
#include "tsig.h"
#include "psig.h"
#include "bits.h"
#include "hash.h"

// open a file with a specified suffix
// - always open for both reading and writing

File openFile(char *name, char *suffix)
{
	char fname[MAXFILENAME];
	sprintf(fname,"%s.%s",name,suffix);
	File f = open(fname,O_RDWR|O_CREAT,0644);
	assert(f >= 0);
	return f;
}

// create a new relation (five files)
// data file has one empty data page

Status newRelation(char *name, Count nattrs, float pF, char sigtype,
                   Count tk, Count tm, Count pm, Count bm)
{
	Reln r = malloc(sizeof(RelnRep));
	RelnParams *p = &(r->params);
	assert(r != NULL);
	p->nattrs = nattrs;
	p->pF = pF,
	p->sigtype = sigtype;
	p->tupsize = 28 + 7*(nattrs-2);
	Count available = (PAGESIZE-sizeof(Count));
	p->tupPP = available/p->tupsize;
	p->tk = tk; 
	if (tm%8 > 0) tm += 8-(tm%8); // round up to byte size
	p->tm = tm; p->tsigSize = tm/8; p->tsigPP = available/(tm/8);
	if (pm%8 > 0) pm += 8-(pm%8); // round up to byte size
	p->pm = pm; p->psigSize = pm/8; p->psigPP = available/(pm/8);
	if (p->psigPP < 2) { free(r); return -1; }
	if (bm%8 > 0) bm += 8-(bm%8); // round up to byte size
	p->bm = bm; p->bsigSize = bm/8; p->bsigPP = available/(bm/8);
	if (p->bsigPP < 2) { free(r); return -1; }
	r->infof = openFile(name,"info");
	r->dataf = openFile(name,"data");
	r->tsigf = openFile(name,"tsig");
	r->psigf = openFile(name,"psig");
	r->bsigf = openFile(name,"bsig");
	addPage(r->dataf); p->npages = 1; p->ntups = 0;
	addPage(r->tsigf); p->tsigNpages = 1; p->ntsigs = 0;
	addPage(r->psigf); p->psigNpages = 1; p->npsigs = 0;
	addPage(r->bsigf); p->bsigNpages = 1; p->nbsigs = 0; // replace this
	// Create a file containing "pm" all-zeroes bit-strings,
    // each of which has length "bm" bits
	//TODO
	
	// bm is the width of bsig, equal to maxpages
	Bits bsig = newBits(p->bm);
	PageID pid = 0;
	Page curr = getPage(r->bsigf, pid);
	// pm is the number of bsigs
	for (int i = 0; i < p->pm; i++) {
		// check if room on last page;
		if (pageNitems(curr) == p->bsigPP) {
			putPage(r->bsigf, pid, curr);
			addPage(r->bsigf);
			p->bsigNpages++; // number of bsig pages
			pid++;
			free(curr);
			curr = newPage();
			if (curr == NULL) return -1;;
		}
		putBits(curr, pageNitems(curr), bsig);
		addOneItem(curr); // page->nitems++; 
		p->nbsigs++;     // number of bit-slice signatures (bsigs)
	}
	putPage(r->bsigf, pid, curr);
	freeBits(bsig);
	
	closeRelation(r);
	return 0;
}

// check whether a relation already exists

Bool existsRelation(char *name)
{
	char fname[MAXFILENAME];
	sprintf(fname,"%s.info",name);
	File f = open(fname,O_RDONLY);
	if (f < 0)
		return FALSE;
	else {
		close(f);
		return TRUE;
	}
}

// set up a relation descriptor from relation name
// open files, reads information from rel.info

Reln openRelation(char *name)
{
	Reln r = malloc(sizeof(RelnRep));
	assert(r != NULL);
	r->infof = openFile(name,"info");
	r->dataf = openFile(name,"data");
	r->tsigf = openFile(name,"tsig");
	r->psigf = openFile(name,"psig");
	r->bsigf = openFile(name,"bsig");
	read(r->infof, &(r->params), sizeof(RelnParams));
	return r;
}

// release files and descriptor for an open relation
// copy latest information to .info file
// note: we don't write ChoiceVector since it doesn't change

void closeRelation(Reln r)
{
	// make sure updated global data is put in info file
	lseek(r->infof, 0, SEEK_SET);
	int n = write(r->infof, &(r->params), sizeof(RelnParams));
	assert(n == sizeof(RelnParams));
	close(r->infof); close(r->dataf);
	close(r->tsigf); close(r->psigf); close(r->bsigf);
	free(r);
}

// insert a new tuple into a relation
// returns page where inserted
// returns NO_PAGE if insert fails completely

PageID addToRelation(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL && strlen(t) == tupSize(r));
	Page p;  PageID pid;
	RelnParams *rp = &(r->params);
	Bool new =  FALSE; // set as true if need to create a new page
	
	// add tuple to last page
	pid = rp->npages-1;
	p = getPage(r->dataf, pid);
	// check if room on last page; if not add new page
	if (pageNitems(p) == rp->tupPP) {
		addPage(r->dataf);
		rp->npages++;
		pid++;
		free(p);
		p = newPage();
		if (p == NULL) return NO_PAGE;
		new = TRUE;
	}
	addTupleToPage(r, p, t);
	rp->ntups++;  //written to disk in closeRelation()
	putPage(r->dataf, pid, p);
	
	// compute tuple signature and add to tsigf
	//TODO
	
	// get tsig
	Bits tsig = makeTupleSig(r, t);
	pid = rp->tsigNpages-1;
	
	// add to last page 
	p = getPage(r->tsigf, pid);
	// or new page
	if (pageNitems(p) == rp->tsigPP) {
		addPage(r->tsigf);
		rp->tsigNpages++;
		pid++;
		free(p);
		p = newPage();
		if (p == NULL) return NO_PAGE;
	}
	// add sig to page -> putBits(Page p, Offset pos, Bits b)
	// we need add to the last in this page
	putBits(p, pageNitems(p), tsig);
	addOneItem(p);
	rp->ntsigs++;
	putPage(r->tsigf, pid, p);
	freeBits(tsig);

	// compute page signature and add to psigf
	//TODO
	
	pid = rp->psigNpages-1;
	Bits psig = makePageSig(r, t);
	
	// add to last page 
	p = getPage(r->psigf, pid);
	// or need new tuple page || no any previous psig exist in this page
	// new page is for data file, not for tsig
	if (new == TRUE || pageNitems(p) == 0) {
		if (pageNitems(p) == rp->psigPP) {
			// create a new psig page to hold new psig
			// debug
			// printf("create new, %d->%d \n", pageNitems(p),rp->psigPP);
			addPage(r->psigf);
			rp->psigNpages++;
			pid++;
			free(p);
			p = newPage();
			if (p == NULL) return NO_PAGE;
		}
		putBits(p, pageNitems(p), psig);
		addOneItem(p);
		rp->npsigs++;
		putPage(r->psigf, pid, p); // after update all info, put the page.
		// showBits(cpsig); // debug
		// printf("before cpsig, %d \n", rp->npsigs);
	} else {
		//merge exit psig with new one
		Bits cpsig = newBits(psigBits(r));
		// get the last item and update
		getBits(p,pageNitems(p)-1,cpsig);
		putBits(p,pageNitems(p)-1,cpsig);
		freeBits(cpsig);
		putPage(r->psigf, pid, p);
	}	
	// freeBits(psig);

	// use page signature to update bit-slices
	//TODO
	
	// update pagesig or add psig
	// update each entry of bit-slice page
	// pm is fixed (maxpages), we don't need to change it even adding new psig.
	
	// Bits bsig = makeTupleSig(r, t);
	// pid = rp->bsigNpages-1;
	
	// p = getPage(r->bsigf, pid);
	// if (pageNitems(p) == rp->psigPP) {
	// 	addPage(r->psigf);
	// 	rp->psigNpages++;
	// 	pid++;
	// 	free(p);
	// 	p = newPage();
	// 	if (p == NULL) return NO_PAGE;
	// }
	// putBits(p, pageNitems(p)-1, psig);
	// putPage(r->psigf, pid, p);
	// rp->npsigs++;
	
	// rp->nbsigs++;
	
	for (int i = 0; i < rp->pm; i++) {
		if (bitIsSet(psig, i)) {
			Bits bsig = newBits(bsigBits(r));
			PageID bpid = i / rp->bsigPP;
			p = getPage(r->bsigf, bpid);
			Offset pos = i % rp->bsigPP;
			getBits(p, pos, bsig);
			setBit(bsig, bpid);
			putBits(p, pos, bsig);
			putPage(r->bsigf, bpid, p);
		}
	}
	
	// rp->bsigNpages++;

	return nPages(r)-1;
}

// displays info about open Reln (for debugging)

void relationStats(Reln r)
{
	RelnParams *p = &(r->params);
	printf("Global Info:\n");
	printf("Dynamic:\n");
    printf("  #items:  tuples: %d  tsigs: %d  psigs: %d  bsigs: %d\n",
			p->ntups, p->ntsigs, p->npsigs, p->nbsigs);
    printf("  #pages:  tuples: %d  tsigs: %d  psigs: %d  bsigs: %d\n",
			p->npages, p->tsigNpages, p->psigNpages, p->bsigNpages);
	printf("Static:\n");
    printf("  tups   #attrs: %d  size: %d bytes  max/page: %d\n",
			p->nattrs, p->tupsize, p->tupPP);
	printf("  sigs   %s",
            p->sigtype == 'c' ? "catc" : "simc");
    if (p->sigtype == 's')
	    printf("  bits/attr: %d", p->tk);
    printf("\n");
	printf("  tsigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->tm, p->tsigSize, p->tsigPP);
	printf("  psigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->pm, p->psigSize, p->psigPP);
	printf("  bsigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->bm, p->bsigSize, p->bsigPP);
}
