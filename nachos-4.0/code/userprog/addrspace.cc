// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -n -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "addrspace.h"
#include "machine.h"
#include "noff.h"
#include "disk.h"
//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//----------------------------------------------------------------------
bool AddrSpace::running[NumPhysPages] = {0};
bool AddrSpace::usedSectors[NumSectors] = {0};
AddrSpace::AddrSpace()
{
//    pageTable = new TranslationEntry[NumPhysPages];
//    for (unsigned int i = 0; i < NumPhysPages; i++) {
//	pageTable[i].virtualPage = i;	// for now, virt page # = phys page #
//	pageTable[i].physicalPage = i;
//	pageTable[i].physicalPage = 0;
//	pageTable[i].valid = TRUE;
//	pageTable[i].valid = FALSE;
//	pageTable[i].use = FALSE;
//	pageTable[i].dirty = FALSE;
//	pageTable[i].readOnly = FALSE;
//    }
    
    // zero out the entire address space
//    bzero(kernel->machine->mainMemory, MemorySize);
    ID = (kernel->ID_number)++;
    kernel->ID_number = (kernel->ID_number)++;
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
//   for(unsigned int i=0;i<numPages;i++)
//	AddrSpace::running[pageTable[i].physicalPage] = false;
   delete pageTable;
}


//----------------------------------------------------------------------
// AddrSpace::Load
// 	Load a user program into memory from a file.
//
//	Assumes that the page table has been initialized, and that
//	the object code file is in NOFF format.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

bool AddrSpace::Load(char *fileName) 
{
    OpenFile *executable = kernel->fileSystem->Open(fileName);
    NoffHeader noffH;
    unsigned int size;
    unsigned int tmp;
    if (executable == NULL) {
	cerr << "Unable to open file " << fileName << "\n";
	return FALSE;
    }
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);

    pageTable = new TranslationEntry[numPages];
//    for(unsigned int i=0, j=0; i<numPages;i++){
//	pageTable[i].virtualPage = i;
//	while(j < NumPhysPages && AddrSpace::running[j] == true)
//	    j++;
//	AddrSpace::running[j] = true;
//	pageTable[i].physicalPage = j;
//	pageTable[i].valid = true;
//	pageTable[i].use = false;
//	pageTable[i].dirty = false;
//	pageTable[i].readOnly = false;
//	cout << "number of pages of " << fileName<< " is "<<numPages<<endl;
//    }
    unsigned int allocatedPages;
    allocatedPages = numPages;
    unsigned int index = 0;
    /*    
    while(allocatedPages > 0){
	if(index < NumPhysPages){
	    while(index < NumPhysPages && AddrSpace::running[index] == true)
	        index ++;
        }else{
	    while(AddrSpace::usedSectors[index - NumPhysPages] == true)
		index ++;
	}
	if(index < NumPhysPages){
		AddrSpace::running[index] = true;
		pageTable[numPages - allocatedPages].virtualPage = allocatedPages - 1;
		pageTable[numPages - allocatedPages].physicalPage = index;
		pageTable[numPages - allocatedPages].valid = true;
		pageTable[numPages - allocatedPages].use = false;
		pageTable[numPages - allocatedPages].dirty = false;
		pageTable[numPages - allocatedPages].readOnly = false;
		allocatedPages --;
		index ++;
		executable->ReadAt(&(kernel->machine->mainMemory[index*PageSize]), PageSize, noffH.code.inFileAddr + ((numPages - allocatedPages) * PageSize));
	}
	else{
		AddrSpace::usedSectors[index - NumPhysPages] = true;
		pageTable[numPages - allocatedPages].virtualPage = allocatedPages - 1;
		pageTable[numPages - allocatedPages].physicalPage = index - NumPhysPages;
		pageTable[numPages - allocatedPages].valid = false;
		pageTable[numPages - allocatedPages].use = false;
		pageTable[numPages - allocatedPages].dirty = false;
		pageTable[numPages - allocatedPages].readOnly = false;
		allocatedPages --;
		char *buffer;
		buffer = new char [PageSize];
		executable->ReadAt(buffer, PageSize, noffH.code.inFileAddr + ((numPages - allocatedPages) * PageSize));
		kernel->swapDisk->WriteSector(index - NumPhysPages, buffer);
	}
    }
    for(int i=0;i<numPages;i++){
	printf("%d, %d, %d\n", i, pageTable[i].physicalPage, pageTable[i].valid);
    }*/
    size = numPages * PageSize;
    //pageTable = new TranslationEntry[numPages];

//    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG(dbgAddr, "Initializing address space: " << numPages << ", " << size);
    if(noffH.code.size > 0){
	for(unsigned int j=0, i=0; i < numPages; i++){
	    j = 0;
	    while(kernel->UsedPhyPage[j] != false && j < NumPhysPages)
		j++;
	    if(j < NumPhysPages){
		kernel->UsedPhyPage[j] = true;
		kernel->PhyPageInfo[j] = ID;
		kernel->main_tab[j] = &pageTable[i];
		pageTable[i].physicalPage = j;
		pageTable[i].valid = true;
		pageTable[i].use = false;
		pageTable[i].dirty = false;
		pageTable[i].readOnly = false;
		//pageTable[i].ID = ID;
		//pageTable[i].LRU_counter++;
		kernel->ID[i] = ID;
		kernel->LRU_counter[i] ++;
		executable->ReadAt(&(kernel->machine->mainMemory[j * PageSize]), PageSize, noffH.code.inFileAddr + (i * PageSize));
	    }else{
		char *buffer;
		buffer = new char[PageSize];
		tmp = 0;
		while(kernel->UsedVirtualPage[tmp] != false){
			tmp ++;
		}
		kernel->UsedVirtualPage[tmp] = true;
		pageTable[i].virtualPage = tmp;
		pageTable[i].valid = false;
		pageTable[i].use = false;
		pageTable[i].dirty = false;
		pageTable[i].readOnly = false;
		//pageTable[i].ID = ID;
		kernel->ID[i] = ID;
		executable->ReadAt(buffer, PageSize, noffH.code.inFileAddr + (i * PageSize));
		kernel->swapDisk->WriteSector(tmp, buffer);
	    }

	}
    }
    if(noffH.initData.size > 0){
	executable->ReadAt(&(kernel->machine->mainMemory[noffH.initData.virtualAddr]), noffH.initData.size, noffH.initData.inFileAddr);
    }

// then, copy in the code and data segments into memory
/*
	if (noffH.code.size > 0) {
        DEBUG(dbgAddr, "Initializing code segment.");
	DEBUG(dbgAddr, noffH.code.virtualAddr << ", " << noffH.code.size);
        unsigned int nPage = pageTable[noffH.code.virtualAddr / PageSize].physicalPage;
	unsigned int offset = noffH.code.virtualAddr % PageSize;
	executable->ReadAt(
		&(kernel->machine->mainMemory[nPage * PageSize + offset]), 
			noffH.code.size, noffH.code.inFileAddr);
    }
	if (noffH.initData.size > 0) {
        DEBUG(dbgAddr, "Initializing data segment.");
	DEBUG(dbgAddr, noffH.initData.virtualAddr << ", " << noffH.initData.size);
        //executable->ReadAt(
	//	&(kernel->machine->mainMemory[pageTable[noffH.initData.virtualAddr/PageSize].physicalPage * PageSize + (noffH.code.virtualAddr % PageSize)]),
	//		noffH.initData.size, noffH.initData.inFileAddr);
        unsigned int nPage = pageTable[noffH.initData.virtualAddr / PageSize].physicalPage;
	unsigned int offset = noffH.initData.virtualAddr % PageSize;
	executable->ReadAt(
		&(kernel->machine->mainMemory[nPage * PageSize + offset]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }
*/
    delete executable;			// close file
    return TRUE;			// success
}

//----------------------------------------------------------------------
// AddrSpace::Execute
// 	Run a user program.  Load the executable into memory, then
//	(for now) use our own thread to run it.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

void 
AddrSpace::Execute(char *fileName) 
{
    pageTable_is_load = false;
    if (!Load(fileName)) {
	cout << "inside !Load(FileName)" << endl;
	return;				// executable not found
    }
    pageTable_is_load = true;
    //kernel->currentThread->space = this;
    this->InitRegisters();		// set the initial register values
    this->RestoreState();		// load page table register

    kernel->machine->Run();		// jump to the user progam

    ASSERTNOTREACHED();			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}


//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    Machine *machine = kernel->machine;
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG(dbgAddr, "Initializing stack pointer: " << numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, don't need to save anything!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
	if(pageTable_is_load){
        pageTable=kernel->machine->pageTable;
        numPages=kernel->machine->pageTableSize;
	}
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    kernel->machine->pageTable = pageTable;
    kernel->machine->pageTableSize = numPages;
}
