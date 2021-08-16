
/////////////////////////////////////////////////////////////////////////////////////
//
//	Application to demonstrate Customized Virtual File System 
//
//
///////////////////////////////////////////////////////////////////////////////////

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
//#include<unistd.h>		//used on linux platform
//#include<iostream>
#include<io.h>			//used on windows platform

#define MAXINODE 5			//maximum files created 50
#define READ 1
#define WRITE 2
//for read & write 3

//#define MAXFILESIZE 2048	//maximum size of file(it may be 1024 i.e.1kb)
#define MAXFILESIZE 10

#define REGULAR 1		//types of file		//used in project
#define SPECIAL 2

#define START 0				//file offset(lseek)
#define CURRENT 1
#define END 2			//hole in the file like potential gap		//file have size 1000 byte ...we write 1st 200 and lseek by 300 .point reached at 500 and from 500 we write data...  200 to 500 is hole in file

typedef struct superblock		//done		
{
	int TotalInodes;		//initially size is 50 for both
	int FreeInode;
}SUPERBLOCK, *PSUPERBLOCK;

typedef struct inode		//86 byte		//done
{
	char FileName[50];
	int InodeNumber;
	int FileSize;			//if file size is 1024 & we write 10 b data then actual size is 1024 & file size is 10b
	int FileActualSize;
	int FileType;
	char *Buffer;		//on actual buffer it stores block number
	int LinkCount;
	int ReferenceCount;
	int Permission;			//1	2 3(1+2)
	struct inode *next;		//self refencial structure
}INODE,*PINODE,**PPINODE;

typedef struct filetable			//done
{
	int readoffset;		//where to read
	int writeoffset;	//where to write	
	int count;			//always remains 1
	int mode;
	PINODE ptrinode;		//linked list points to inode
}FILETABLE,*PFILETABLE;

typedef struct ufdt		//start from here			//done
{
	PFILETABLE ptrfiletable;		//it points to file table
}UFDT;

UFDT UFDTArr[MAXINODE];		//create array of structure i.e. it is array of pointer		//Global		//done
SUPERBLOCK SUPERBLOCKobj;	//name of structure											//Global		//done
PINODE head = NULL;																		//Global		//done

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: man
//	Input	: char *
//	Output	: None
//	Description : Display Description of each command
//	Author	: Geetika Tejwani
//	Date	: 15/1/2020
/////////////////////////////////////////////////////////////////////////////////////////

void man(char *name)			//done
{
	if(name == NULL)
	{
		return;
	}

	if(strcmp(name,"create") == 0)
	{
		printf("Description : Used to create new regular file\n");
		printf("Usage : create File_Name Permission\n");
	}
	else if(strcmp(name,"read") == 0)
	{
		printf("Description : Used to read data from regular file\n");
		printf("Usage : read File_Name No_of_Bytes_to_Read\n");
	}
	else if(strcmp(name,"write") == 0)
	{
		printf("Description : Used to write data into regular file\n");
		printf("Usage : write File_Name\nAfter this enter data that we want to write\n");
	}
	else if(strcmp(name,"ls") == 0)
	{
		printf("Description : Used to list all information of files\n");
		printf("Usage : ls\n");
	}
	else if(strcmp(name,"stat") == 0)
	{
		printf("Description : Used to display information of file\n");
		printf("Usage : stat File_name\n");
	}
	else if(strcmp(name,"fstat") == 0)
	{
		printf("Description : Used to display information of file\n");
		printf("Usage : stat File_Descriptor\n");
	}
	else if(strcmp(name,"truncate") == 0)
	{
		printf("Description : Used to remove data from file\n");
		printf("Usage : truncate File_name\n");
	}
	else if(strcmp(name,"open") == 0)
	{
		printf("Description : Used to open file\n");
		printf("Usage : open File_name mode\n");
	}
	else if(strcmp(name,"close") == 0)
	{
		printf("Description : Used to read data from regular file\n");
		printf("Usage : close File_name\n");
	}
	else if(strcmp(name,"closeall") == 0)
	{
		printf("Description : Used to close all opened file\n");
		printf("Usage : closeall\n");
	}
	else if(strcmp(name,"lseek") == 0)
	{
		printf("Description : Used to change file offset\n");
		printf("Usage : lseek File_Name ChangeInOffset StartPoint\n");
	}
	else if(strcmp(name,"rm") == 0)
	{
		printf("Description : Used to delete file\n");
		printf("Usage : rm File_Name\n");
	}
	else
	{
		printf("ERROR : No manual entry is available.\n");
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: DisplayHelp
//	Input	: None
//	Output	: None
//	Description : Display List with description of each command
//	Author	: Geetika Tejwani
//	Date	: 15/1/2020
/////////////////////////////////////////////////////////////////////////////////////////

void DisplayHelp()		//done
{
	printf("ls			: To List out all files.\n");
	printf("clear		: To clear console.\n");
	printf("open		: To open the file.\n");
	printf("close		: To close the file.\n");
	printf("closeall	: To all close the files\n");
	printf("read		: To read the contents from file.\n");
	printf("write		: To write contents into file.\n");
	printf("exit		: To terminate file system.\n");
	printf("stat		: To Display inforamtion of file using name.\n");
	printf("fstat		: To Display information of file using file descriptor.\n");
	printf("truncate	: To Remove all data from file.\n");
	printf("rm			: To delete file.\n");
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: GetFDFromName
//	Input	: char *
//	Output	: integer
//	Description : To get file descriptor value
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

int GetFDFromName(char *name)
{
	int i = 0;

	while(i < MAXINODE)
	{
		if(UFDTArr[i].ptrfiletable != NULL)
		{
			if(strcmp((UFDTArr[i].ptrfiletable ->ptrinode->FileName),name) == 0)
			{
				break;
			}
		}
		i++;
	}

	if(i == MAXINODE)
	{
		return -1;
	}
	else
	{
		return i;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: Get_Inode
//	Input	: char *
//	Output	: PINODE
//	Description : Return Inode value of file
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

PINODE Get_Inode(char *name)		//Demo.txt	//to check duplicate files are available in inode or not
{
	PINODE temp = head;		//100
	int i = 0;		

	if(name == NULL)
	{
		return NULL;
	}

	while(temp != NULL)
	{
		if(strcmp(name,temp->FileName) == 0)
		{
			break;
		}
		temp = temp -> next;
	}
	return temp;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: CreateDILB
//	Input	: None
//	Output	: None
//	Description : 
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

void CreateDILB()		//done
{
	int i = 1;
	PINODE newn = NULL;
	PINODE temp = head;

	while(i <= MAXINODE)		//it creates linked list(insert last)
	{
		newn = (PINODE)malloc(sizeof(INODE));		//86 bytes are get allocated
		newn->LinkCount = 0;
		newn->ReferenceCount = 0;
		newn->FileType = 0;
		newn->FileSize = 0;
		newn -> Buffer = NULL;
		newn -> next = NULL;
		newn->InodeNumber = i;

		if(temp == NULL)
		{
			head = newn;
			temp = head;
		}
		else
		{
			temp->next = newn;
			temp = temp->next;
		}

		i++;
	}
	printf("DILB create successfully\n");
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: InitialiseSuperBlock
//	Input	: None
//	Output	: None
//	Description : Initialize Inode values
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

void InitialiseSuperBlock()			//done
{
	int i = 0;		
	while(i < MAXINODE)
	{
		UFDTArr[i].ptrfiletable = NULL;		//set next pointer to NULL
		i++;
	}

	SUPERBLOCKobj.TotalInodes = MAXINODE;
	SUPERBLOCKobj.FreeInode = MAXINODE;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: CreateFile
//	Input	: char , integer
//	Output	: None
//	Description : Create a new file
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

int CreateFile(char *name,int Permission)		//
{
	int i = 0;
	PINODE temp = head;

	if((name == NULL) || (Permission == 0) || (Permission > 3))
	{
		return -1;
	}

	if(SUPERBLOCKobj.FreeInode == 0)
	{
		return -2;
	}
	(SUPERBLOCKobj.FreeInode)--;

	if(Get_Inode(name) != NULL)		//NULL != NULL
	{
		return -3;
	}

	while(temp != NULL)			//100 != NULL
	{
		if(temp->FileType == 0)
		{
			break;
		}
		temp = temp->next;
	}

	while(i < MAXINODE)
	{
		if(UFDTArr[i].ptrfiletable == NULL)			//
		{
			break;
		}
		i++;
	}

	UFDTArr[i].ptrfiletable = (PFILETABLE)malloc(sizeof(FILETABLE));

	UFDTArr[i].ptrfiletable->count = 1;
	UFDTArr[i].ptrfiletable->mode = Permission;
	UFDTArr[i].ptrfiletable->readoffset = 0;
	UFDTArr[i].ptrfiletable->writeoffset = 0;
	UFDTArr[i].ptrfiletable->ptrinode = temp;

	strcpy(UFDTArr[i].ptrfiletable->ptrinode->FileName,name);
	UFDTArr[i].ptrfiletable->ptrinode->FileType = REGULAR;
	UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount = 1;
	UFDTArr[i].ptrfiletable->ptrinode->LinkCount = 1;
	UFDTArr[i].ptrfiletable->ptrinode->FileActualSize = 0;
	UFDTArr[i].ptrfiletable->ptrinode->FileSize = MAXFILESIZE;
	UFDTArr[i].ptrfiletable->ptrinode->Permission = Permission;
	UFDTArr[i].ptrfiletable->ptrinode->Buffer = (char *)malloc(MAXFILESIZE);

	return i;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: rm_File
//	Input	: char *
//	Output	: Integer
//	Description : Remove file
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

//rm_File("Demo.txt")
int rm_File(char *Name)			//done
{
	int fd = 0;

	fd = GetFDFromName(Name);
	if(fd == -1)
	{
		return -1;
	}
	(UFDTArr[fd].ptrfiletable->ptrinode->LinkCount)--;

	if(UFDTArr[fd].ptrfiletable->ptrinode->LinkCount == 0)
	{
		UFDTArr[fd].ptrfiletable->ptrinode->FileType = 0;
		free(UFDTArr[fd].ptrfiletable->ptrinode->Buffer);
		strcpy(UFDTArr[fd].ptrfiletable->ptrinode->FileName,"");
		UFDTArr[fd].ptrfiletable->ptrinode->ReferenceCount = 0;
		UFDTArr[fd].ptrfiletable->ptrinode->Permission = 0;
		UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize = 0;
		free(UFDTArr[fd].ptrfiletable);
	}

	UFDTArr[fd].ptrfiletable = NULL;
	(SUPERBLOCKobj.FreeInode)++;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: ReadFile
//	Input	: integer, char, int
//	Output	: integer
//	Description : Read data from file
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

int ReadFile(int fd,char *arr,int isize)		//(0,70,3)
{
	int read_size = 0;
	
	if(UFDTArr[fd].ptrfiletable == NULL)		
	{
		return -1;
	}

	if(UFDTArr[fd].ptrfiletable->mode != READ && UFDTArr[fd].ptrfiletable->mode != READ+WRITE)		//(3!=1) && (3!=3)
	{
		return -2;
	}

	if(UFDTArr[fd].ptrfiletable->ptrinode->Permission != READ && UFDTArr[fd].ptrfiletable->ptrinode->Permission != READ+WRITE)		//(3!=1) && (3!=3)
	{
		return -2;
	}

	if(UFDTArr[fd].ptrfiletable->readoffset == UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)	//(0 == 4)
	{
		return -3;
	}

	if(UFDTArr[fd].ptrfiletable->ptrinode->FileType != REGULAR)
	{
		return -4;
	}

	read_size = (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) - (UFDTArr[fd].ptrfiletable->readoffset);	//4 - 0

	if(read_size < isize)		//4 < 3
	{
		strncpy(arr,(UFDTArr[fd].ptrfiletable->ptrinode->Buffer) + (UFDTArr[fd].ptrfiletable->readoffset),read_size);
		UFDTArr[fd].ptrfiletable->readoffset = UFDTArr[fd].ptrfiletable->readoffset + read_size;
	}
	else
	{
		strncpy(arr,(UFDTArr[fd].ptrfiletable->ptrinode->Buffer) + (UFDTArr[fd].ptrfiletable->readoffset),isize);	//strncpy(70,1000 + 0,3)
		(UFDTArr[fd].ptrfiletable->readoffset) = (UFDTArr[fd].ptrfiletable->readoffset) + isize;
	}

	return isize;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: WriteFile
//	Input	: integer, char, integer
//	Output	: Integer
//	Description : Write data in file
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

int WriteFile(int fd,char *arr,int isize)		//(0,Welcome,7)		//done
{
	//conditions to check file have write permission or not
	if(((UFDTArr[fd].ptrfiletable->mode) != WRITE) && ((UFDTArr[fd].ptrfiletable->mode) != READ+WRITE))		//(3 != 2) && (3 != 3)
	{
		return -1;
	}

	if(((UFDTArr[fd].ptrfiletable->ptrinode->Permission) != WRITE) && ((UFDTArr[fd].ptrfiletable->ptrinode->Permission) != READ+WRITE))		//(3 != 2) && (3 != 3)
	{
		return -1;
	}

	if((UFDTArr[fd].ptrfiletable->writeoffset) == MAXFILESIZE)		// 0 == 10
	{
		return -2;
	}

	if((UFDTArr[fd].ptrfiletable->ptrinode->FileType) != REGULAR)		//1 != 1
	{
		return -3;
	}
	if(((UFDTArr[fd].ptrfiletable->ptrinode->FileSize) - (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)) < isize)
	{
		return -4;
	}

	strncpy((UFDTArr[fd].ptrfiletable->ptrinode->Buffer) + (UFDTArr[fd].ptrfiletable->writeoffset),arr,isize);		//strncpy(1000 + 0,Welcome,4)
	(UFDTArr[fd].ptrfiletable->writeoffset) = (UFDTArr[fd].ptrfiletable->writeoffset)+isize;						//0 + 4
	(UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) = (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + isize;	//0 + 4

	return isize;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: OpenFile
//	Input	: char , integer
//	Output	: Integer
//	Description : OPen an existing file
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

int OpenFile(char *name,int mode)
{
	int i = 0;
	PINODE temp = NULL;

	if(name == NULL || mode <= 0)
	{
		return -1;
	}
	temp = Get_Inode(name);
	if(temp == NULL)
	{
		return -2;
	}

	if(temp -> Permission < mode)
	{
		return -3;
	}

	while(i < MAXINODE)
	{
		if(UFDTArr[i].ptrfiletable == NULL)
		{
			break;
		}
		i++;
	}

	UFDTArr[i].ptrfiletable = (PFILETABLE)malloc(sizeof(FILETABLE));
	if(UFDTArr[i].ptrfiletable == NULL)
	{
		return -1;
	}

	UFDTArr[i].ptrfiletable->count = 1;
	UFDTArr[i].ptrfiletable->mode = mode;

	if(mode == READ + WRITE)
	{
		UFDTArr[i].ptrfiletable->readoffset = 0;
		UFDTArr[i].ptrfiletable->writeoffset = 0;
	}
	else if(mode == READ)
	{
		UFDTArr[i].ptrfiletable->readoffset = 0;
	}
	else if(mode == WRITE)
	{
		UFDTArr[i].ptrfiletable->writeoffset = 0;
	}
	UFDTArr[i].ptrfiletable->ptrinode = temp;
	(UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount)++;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: CloseFileByName
//	Input	: integer
//	Output	: None
//	Description : Close existing file by file descriptor
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

void CloseFileByName(int fd)			//done
{
	UFDTArr[fd].ptrfiletable->readoffset = 0;
	UFDTArr[fd].ptrfiletable->writeoffset = 0;
	(UFDTArr[fd].ptrfiletable->ptrinode->ReferenceCount)--;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: CloseFileByName
//	Input	: char
//	Output	: Integer
//	Description : Close the existing file by name
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

int CloseFileByName(char *name)			//done
{
	int i = 0;
	i = GetFDFromName(name);

	if(i == -1)
	{
		return -1;
	}

	UFDTArr[i].ptrfiletable->readoffset = 0;
	UFDTArr[i].ptrfiletable->writeoffset = 0;
	(UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount)--;

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: CloseAll
//	Input	: None
//	Output	: None
//	Description : Close all existing file
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

void CloseAll()			//done
{
	int i = 0;
	while(i < MAXINODE)
	{
		if(UFDTArr[i].ptrfiletable != NULL)
		{
			UFDTArr[i].ptrfiletable->readoffset = 0;
			UFDTArr[i].ptrfiletable->writeoffset = 0;
			//(UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount)--;
			UFDTArr[i].ptrfiletable->ptrinode->ReferenceCount = 0;
		}
		i++;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: LseekFile
//	Input	: int, int, int
//	Output	: Integer
//	Description : write data in file from perticular position
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

int LseekFile(int fd,int size,int from)			//(0,3,1)			//done
{
	if((fd < 0) || (from > 2))			//
	{
		return -1;
	}
	if(UFDTArr[fd].ptrfiletable == NULL)
	{
		return -1;
	}

	if((UFDTArr[fd].ptrfiletable->mode == READ) || (UFDTArr[fd].ptrfiletable->mode == READ+WRITE))
	{
		if(from == CURRENT)
		{
			if(((UFDTArr[fd].ptrfiletable->readoffset) + size) > UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize)	//3>4
			{
				return -1;
			}
			(UFDTArr[fd].ptrfiletable->readoffset) = (UFDTArr[fd].ptrfiletable->readoffset) + size;
		}
		else if(from == START)
		{
			if(size > (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize))
			{
				return -1;
			}
			if(size < 0)
			{
				return -1;
			}
			(UFDTArr[fd].ptrfiletable->readoffset) = size;
		}
		else if(from == END)
		{
			if((UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + size > MAXFILESIZE)
			{
				return -1;
			}
			if(((UFDTArr[fd].ptrfiletable->readoffset) + size) < 0)
			{
				return -1;
			}
			(UFDTArr[fd].ptrfiletable->readoffset) = (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + size;
		}
	}
	else if(UFDTArr[fd].ptrfiletable->mode == WRITE)
	{
		if(from == CURRENT)
		{
			if(((UFDTArr[fd].ptrfiletable->writeoffset) + size) > MAXFILESIZE)
			{
				return -1;
			}
			if(((UFDTArr[fd].ptrfiletable->writeoffset) + size) < 0)
			{
				return -1;
			}
			if(((UFDTArr[fd].ptrfiletable->writeoffset) + size) > (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize))
			{
				(UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) = (UFDTArr[fd].ptrfiletable->writeoffset) + size;
			}
			(UFDTArr[fd].ptrfiletable->writeoffset) = (UFDTArr[fd].ptrfiletable->writeoffset) + size;
		}
		else if(from == START)
		{
			if(size > MAXFILESIZE)
			{
				return -1;
			}
			if(size < 0)
			{
				return -1;
			}
			if(size > (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize))
			{
				(UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) = size;
			}
			(UFDTArr[fd].ptrfiletable->writeoffset) = size;
		}
		else if(from == END)
		{
			if((UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + size > MAXFILESIZE)
			{
				return -1;
			}
			if(((UFDTArr[fd].ptrfiletable->writeoffset) + size) < 0)
			{
				return -1;
			}
			(UFDTArr[fd].ptrfiletable->writeoffset) = (UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize) + size;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: ls_File
//	Input	: None
//	Output	: None
//	Description : List out existing file names
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

void ls_file()				//done
{
	int i = 0;
	PINODE temp = head;
	int iCnt =0;

	if(SUPERBLOCKobj.FreeInode == MAXINODE)
	{
		printf("Error : There are no files\n");
		return;
	}

	printf("\nFile Name\tInode Number\tFile size\tLink Count\n");
	printf("------------------------------------------------------------\n");

	//set one counter to restrict 

	while(temp != NULL)
	{
		if(temp->FileType != 0)
		{
			printf("%s\t\t%d\t\t%d\t\t%d\n",temp->FileName,temp->InodeNumber,temp->FileActualSize,temp->LinkCount);
		}
		temp = temp -> next;
	}
	printf("--------------------------------------------------------------\n");
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: fstat_file
//	Input	: integer
//	Output	: Integer
//	Description : To show statistical information about file by using file descriptor
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

int fstat_file(int fd)		//done
{
	PINODE temp = head;
	int i = 0;

	if(fd < 0)
	{
		return -1;
	}

	if(UFDTArr[fd].ptrfiletable == NULL)
	{
		return -2;
	}

	temp = UFDTArr[fd].ptrfiletable->ptrinode;

	if(temp->FileType == 0)
	{
		return -2;
	}

	printf("\n---------------Statistical Information about file-------------------------\n");
	printf("File name: %s\n",temp->FileName);
	printf("Inode Number: %d\n",temp->InodeNumber);
	printf("File Size: %d\n",temp->FileSize);
	printf("Actual File size: %d\n",temp->FileActualSize);
	printf("Link count : %d\n",temp->LinkCount);
	printf("Reference count : %d\n",temp->ReferenceCount);

	if(temp -> Permission == 1)
	{
		printf("File  Permission: Read Only\n");
	}
	else if(temp -> Permission == 2)
	{
		printf("File  Permission: Write Only\n");
	}
	else if(temp -> Permission == 3)
	{
		printf("File  Permission: Read and write\n");
	}
	printf("---------------------------------------------------------------------------\n");
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: stat_file
//	Input	: char
//	Output	: Integer
//	Description : To show statistical information about file by file name
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

int stat_file(char *name)			//done
{
	PINODE temp = head;

	if(name == NULL)
	{
		return -1;
	}
	/*if(SUPERBLOCK.FreeInode == MAXINODE)
	{
	}*/
	while(temp != NULL)
	{
		if(strcmp(name,temp->FileName) == 0)
		{
			break;
		}
		temp = temp -> next;
	}

	if(temp == NULL)
	{
		return -2;
	}

	printf("\n---------------Statistical Information about file-------------------------\n");
	printf("File name: %s\n",temp->FileName);
	printf("Inode Number: %d\n",temp->InodeNumber);
	printf("File Size: %d\n",temp->FileSize);
	printf("Actual File size: %d\n",temp->FileActualSize);
	printf("Link count : %d\n",temp->LinkCount);
	printf("Reference count : %d\n",temp->ReferenceCount);

	if(temp -> Permission == 1)
	{
		printf("File  Permission: Read Only\n");
	}
	else if(temp -> Permission == 2)
	{
		printf("File  Permission: Write Only\n");
	}
	else if(temp -> Permission == 3)
	{
		printf("File  Permission: Read and write\n");
	}
	printf("---------------------------------------------------------------------------\n");
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: truncate_File
//	Input	: char
//	Output	: Integer
//	Description : Delete data from file
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

int truncate_File(char *name)		//Demo.txt			//done
{
	int fd = GetFDFromName(name);
	if(fd == -1)
	{
		return -1;
	}

	memset(UFDTArr[fd].ptrfiletable->ptrinode->Buffer,0,MAXFILESIZE);		//1000,0,10
	UFDTArr[fd].ptrfiletable->readoffset = 0;
	UFDTArr[fd].ptrfiletable->writeoffset = 0;
	UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//	Name	: main
//	Input	: None
//	Output	: Integer
//	Description : Entry point function
//	Author	: Geetika Tejwani
//	Date	: 15/01/2020
/////////////////////////////////////////////////////////////////////////////////////////

int main()
{
	char *ptr = NULL;
	int ret = 0,fd = 0,count = 0;
	char command[4][80],str[80],arr[MAXFILESIZE];

	InitialiseSuperBlock();
	CreateDILB();

	while(1)
	{
		fflush(stdin);
		strcpy(str,"");

		printf("\nMarvellous VFS:>");
		fgets(str,80,stdin);	
		//scanf("%[^'\n']s",str);

		count = sscanf(str,"%s %s %s %s",command[0],command[1],command[2],command[3]);

		if(count == 1)
		{
			if(strcmp(command[0],"ls") == 0)		//done
			{
				ls_file();
			}
			else if(strcmp(command[0],"closeall") == 0)			//done
			{
				CloseAll();
				printf("All files closed successfully\n");
				continue;
			}
			else if(strcmp(command[0],"clear") == 0)		//done
			{
				system("cls");
				continue;
			}
			else if(strcmp(command[0],"help") == 0)			//done
			{
				DisplayHelp();
				continue;
			}
			else if(strcmp(command[0],"exit") == 0)			//done
			{
				printf("Terminating the marvellous Virtual File System.\n");
				break;
			}
			else
			{
				printf("ERROR : Command not found!!\n");
				continue;
			}
		}
		else if(count == 2)
		{
			if(strcmp(command[0],"stat") == 0)				//done
			{
				ret = stat_file(command[1]);
				if(ret == -1)
				{
					printf("ERROR : Incorrect parameters\n");
				}
				if(ret == -2)
				{
					printf("ERROR : There is no such file\n");
				}
				continue;
			}
			else if(strcmp(command[0],"fstat") == 0)			//file statistics		fstat 0		//done
			{
				ret = fstat_file(atoi(command[1]));
				if(ret == -1)
				{
					printf("ERROR : Incorrect parameters\n");
				}
				if(ret == -2)
				{
					printf("ERROR : There is no such file\n");
				}
				continue;
			}
			else if(strcmp(command[0],"close") == 0)		//done
			{
				ret = CloseFileByName(command[1]);				//close 0
				if(ret == -1)
				{
					printf("ERROR : There is no such file\n");
				}
				continue;
			}
			else if(strcmp(command[0],"rm") == 0)			//rm Demo.txt		//done
			{
				ret = rm_File(command[1]);
				if(ret == -1)
				{
					printf("ERROR : There is no such file\n");
				}
				continue;
			}
			else if(strcmp(command[0],"man") == 0)			//mannual page		//done
			{
				man(command[1]);
			}
			else if(strcmp(command[0],"write") == 0)		//write Demo.txt
			{
				fd = GetFDFromName(command[1]);				//get fd
				if(fd == -1)
				{
					printf("ERROR : Incorrect Parameter\n");
					continue;
				}
				printf("Enter the data:\n");
				fflush(stdin);		//empty input buffer
				scanf("%[^\n]s",arr);			//Welcome

				ret = strlen(arr);
				if(ret == 0)
				{
					printf("ERROR : Incorrect parameter\n");
					continue;
				}

				
				ret = WriteFile(fd,arr,ret);			//WriteFile(0,addressofarr,7)
				if(ret == -1)
				{
					printf("ERROR : Permission denied.\n");
				}
				if(ret == -2)
				{
					printf("ERROR : There is no sufficient memory.\n");
				}
				if(ret == -3)
				{
					printf("ERROR : It is not regular file\n");
				}
				if(ret == -4)
				{
					printf("ERROR : There is no sufficient memory available\n");
				}
				if(ret > 0)
				{
					printf("Success : %d bytes successfully written\n",ret);
				}
			}
			else if(strcmp(command[0],"truncate") == 0)			//done
			{
				ret = truncate_File(command[1]);
				if(ret == -1)
				{
					printf("ERROR : Incorrect parameter.\n");
				}
			}
			else
			{
				printf("\nERROR : Command not found.\n");
				continue;
			}
		}
		else if(count == 3)
		{
			if(strcmp(command[0],"create") == 0)			//done
			{
				ret = CreateFile(command[1],atoi(command[2]));		//ASCII to Integer		//if 3 then get 51
				//CreateFile(Demo.txt,3)
				if(ret >= 0)
				{
					printf("File successfully created with file descriptor : %d\n",ret);
				}
				if(ret == -1)
				{
					printf("ERROR : Incorrect parameters\n");
				}
				if(ret == -2)
				{
					printf("ERROR : There is no inodes\n");
				}
				if(ret == -3)
				{
					printf("ERROR : file already exist\n");
				}
				if(ret == -4)
				{
					printf("ERROR : Memory allocation failure\n");
				}
				continue;
			}
			else if(strcmp(command[0],"open") == 0)			//done
			{
				ret = OpenFile(command[1],atoi(command[2]));

				if(ret >= 0)
				{
					printf("File successfully created with file descriptor : %d\n",ret);
				}
				if(ret == -1)
				{
					printf("ERROR : Incorrect parameters\n");
				}
				if(ret == -2)
				{
					printf("ERROR : File not present\n");
				}
				if(ret == -3)
				{
					printf("ERROR : Permission denied\n");
				}
				continue;
			}
			else if(strcmp(command[0],"read") == 0)				//done
			{
				fd = GetFDFromName(command[1]);
				if(fd == -1)
				{
					printf("ERROR : Incorrect parameters\n");
					continue;
				}
				ptr = (char *)malloc(sizeof(atoi(command[2])) + 1);

				if(ptr == NULL)
				{
					printf("ERROR : Memory allocation failure\n");
					continue;
				}
				ret = ReadFile(fd,ptr,atoi(command[2]));		//(0,70,3)

				if(ret == -1)
				{
					printf("ERROR : File not existing.\n");
				}
				if(ret == -2)
				{
					printf("ERROR : Permission denied.\n");
				}
				if(ret == -3)
				{
					printf("ERROR : Reached at end of file.\n");
				}
				if(ret == -4)
				{
					printf("ERROR : It is not regular file.\n");
				}
				if(ret > 0)
				{
					write(1,ptr,ret);		//system call to write in file like printf()  //Printf search \0 therefore we write write system call write	//1 is stdout
				}
				continue;
			}
			else
			{
				printf("\nERROR : Command not found!!\n");
				continue;
			}
		}
		else if(count == 4)
		{
			if(strcmp(command[0],"lseek") == 0)			//change the offset in file(read or write)			//done
			{
				fd = GetFDFromName(command[1]);
				if(fd == -1)
				{
					printf("ERROR : Incorrecr Parameter\n");
					continue;
				}
				ret = LseekFile(fd,atoi(command[2]),atoi(command[3]));		//lseekfile(0,atoi(2),atoi(1)); //command[2] displacement
				if(ret == -1)
				{
					printf("ERROR : Unable to perform lseek\n");
				}
			}
			else
			{
				printf("\nERROR : Command not found!!\n");
				continue;
			}
		}
		else
		{
			printf("\nERROR : Command not found!!\n");
			continue;
		}
	}
	return 0;
}
