// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
	int	type = kernel->machine->ReadRegister(2);
	int	val;
	unsigned int vpn;
	unsigned int offset;
	bool valid;
	bool valin;
	char *data;
	int physSector;
	int translatedVirPage;
	int translatedPhyPage;
	char memoryData[PageSize];
	unsigned int j;
	char *buffer;
	char *buffer1;
	char *buffer2;
	unsigned int victim;
    switch (which) {
	case SyscallException:
	    switch(type) {
		case SC_Halt:
		    DEBUG(dbgAddr, "Shutdown, initiated by user program.\n");
   		    kernel->interrupt->Halt();
		    break;
		case SC_PrintInt:
			val=kernel->machine->ReadRegister(4);
			cout << "Print integer:" <<val << endl;
			return;
		case SC_Sleep:
		//	cout << "SC Sleep\n";
			val=kernel->machine->ReadRegister(4);
			kernel->alarm->WaitUntil(val);
			return;
/*		case SC_Exec:
			DEBUG(dbgAddr, "Exec\n");
			val = kernel->machine->ReadRegister(4);
			kernel->StringCopy(tmpStr, retVal, 1024);
			cout << "Exec: " << val << endl;
			val = kernel->Exec(val);
			kernel->machine->WriteRegister(2, val);
			return;
*/		case SC_Exit:
			DEBUG(dbgAddr, "Program exit\n");
			val=kernel->machine->ReadRegister(4);
			cout << "return value:" << val << endl;
			kernel->currentThread->Finish();
			break;
		default:
		    cerr << "Unexpected system call " << type << "\n";
 		    break;
	    }
	    break;
	case PageFaultException:
		//printf("PageFaultException\n");
		val = kernel->machine->ReadRegister(BadVAddrReg);
		vpn = (unsigned) val / PageSize;
		j = 0;
		while(kernel->UsedPhyPage[j] != false && j < NumPhysPages){
			j++;
		}
		//printf("%d\n", j);
		if(j < NumPhysPages){
			buffer = new char [PageSize];
			kernel->UsedPhyPage[j] = true;
			kernel->PhyPageInfo[j] = kernel->ID[vpn];
			kernel->main_tab[j]    = &kernel->machine->pageTable[vpn];
			kernel->machine->pageTable[vpn].physicalPage = j;
			kernel->machine->pageTable[vpn].valid = true;
			//kernel->machine->pageTable[vpn].LRU_counter ++;
			kernel->LRU_counter[vpn] ++;

			kernel->swapDisk->ReadSector(kernel->machine->pageTable[vpn].virtualPage, buffer);
			bcopy(buffer, &kernel->machine->mainMemory[j * PageSize], PageSize);
		}else{
			buffer1 = new char [PageSize];
			buffer2 = new char [PageSize];
			victim = (rand() % 32);
			
			//int min = kernel->machine->pageTable[0].LRU_counter;
			
			/*int min = kernel->LRU_counter[0];
			victim = 0;
			for(int index = 0; index < 32; index ++){
				//if(min > kernel->machine->pageTable[index].LRU_counter){
				if(min > kernel->LRU_counter[index]){
					victim = index;
				}
			}
			//kernel->machine->pageTable[victim].LRU_counter++;
			kernel->LRU_counter[victim] ++;
			*/
			bcopy(&kernel->machine->mainMemory[victim * PageSize], buffer1, PageSize);
			kernel->swapDisk->ReadSector(kernel->machine->pageTable[vpn].virtualPage, buffer2);
			bcopy(buffer2, &kernel->machine->mainMemory[victim * PageSize], PageSize);
			kernel->swapDisk->WriteSector(kernel->machine->pageTable[vpn].virtualPage, buffer1);

			kernel->main_tab[victim]->virtualPage = kernel->machine->pageTable[vpn].virtualPage;
			kernel->main_tab[victim]->valid = false;

			kernel->machine->pageTable[vpn].valid = true;
			kernel->machine->pageTable[vpn].physicalPage = victim;
			//kernel->PhyPageInfo[victim] = kernel->machine->pageTable[vpn].ID;
			kernel->PhyPageInfo[victim] = kernel->ID[vpn];
			kernel->main_tab[victim] = &kernel->machine->pageTable[vpn];
		}
			 

		return;
	default:
	    cerr << "Unexpected user mode exception" << which << "\n";
	    break;
    }
    ASSERTNOTREACHED();
}
