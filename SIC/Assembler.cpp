#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <malloc.h>  


/* Data Structure */
	/* OpcodeTable: Define the format of opcode - 11B.
		01 ArrChar: mnemonicOparand[8] - 8B. 
		02 Char: format - 1B. 
		03 UnShort: machineCode - 2B (Only use 15b).
	*/
typedef struct OPTAB {
	char mnemonic[8];  
	char format;
    unsigned short int macCode;  
} OPTAB;

	/* SymbolTable: Define the fomat of symbol - 14B.
		01 ArrChar: label[10] - 10B. 
		02 Int: address - 4B.
	*/
typedef struct SYMTAB {  
    char label[10];  
	int addr;
} SYMTAB;
SYMTAB symTab[20];  

	/* IntermediateRecord: Define the format of intermediateRecord.
		01 UnShort: lineIndex, location - 2B/ 2B
		02 Unlong: objectCode - 8B.
		03 ArrChar: labelField[32], operatorField[32], operandField[32] - .
	*/
typedef struct IMRecord{  
    unsigned short int lineIndex;  
    unsigned short int loc;  
    unsigned long int objCode;  

    char labelField[32];  
    char operatorField[32];  
    char operandField[32];  
} IMRecord; 
IMRecord* imrArray[100];


/* Variable */
	
int progLength;
int macCode, startAddr;
int i, index, count; 
int locctrCount = 0;  
int symCount = 0;
int arrIndex = 0;  
int locctr[100]; 

unsigned int foundOnSymtabFlag = 0;  
unsigned int foundOnOptabFlag = 0; 

char buffer[256];  
char label[32];  
char mnemonic[32];  
char operand[32];  
static char filename[10];  

	/* opcodeTable:	Establish the opcodeTable for SIC/XE.
		01 FORMAT: {"opCode", "Format", "machineCode"} 
		02 NOTICE: Unable to process floating point.
	*/
static OPTAB opTable[] =  
{  
    { "ADD", '3', 0x18},  { "AND", '3', 0x40},  {"COMP", '3', 0x28},  { "DIV", '3', 0x24},  
	{   "J", '3', 0x3C},  { "JEQ", '3', 0x30},  { "JGT", '3', 0x34},  { "JLT", '3', 0x38},  
	{"JSUB", '3', 0x48},  { "LDA", '3', 0x00},  {"LDCH", '3', 0x50},  { "LDL", '3', 0x08},  
    { "LDX", '3', 0x04},  { "MUL", '3', 0x20},  {  "OR", '3', 0x44},  {  "RD", '3', 0xD8},  
	{"RSUB", '3', 0x4C},  { "STA", '3', 0x0C},  {"STCH", '3', 0x54},  { "STL", '3', 0x14},  
	{"STSW", '3', 0xE8},  { "STX", '3', 0x10},  { "SUB", '3', 0x1C},  {  "TD", '3', 0xE0},  
	{ "TIX", '3', 0x2C},  {  "WD", '3', 0xDC} 
};  


/* Method */
	/* ReadLabel: char*.
		01 Get label from the buffer and set into the label array.
		02 RETURN: arrChar label[32].
	*/
char* ReadLabel(){  
    i = 0;   // Initialize.

    while(buffer[index] != ' ' && buffer[index] != '\t' && buffer[index] != '\n')  
        label[i++] = buffer[index++];  

    label[i] = '\0';   // Null charater.

    return label;  
} 

	/* ReadOprator: char*.
		01 Get operator from the buffer and set into the mnemonic array.
		02 RETURN: arrChar mnemonic[32].
	*/
char* ReadOprator(){  
    i = 0;    // Initialize.

    while(buffer[index] != ' ' && buffer[index] != '\t' && buffer[index] != '\n')  
        mnemonic[i++] = buffer[index++];  

    mnemonic[i] = '\0';   // Null charater.

    return mnemonic;  
}  

	/* ReadOperand: char*.
		01 Get operand from the buffer and set into the operand array.
		02 RETURN: arrChar operand[32].
	*/
char* ReadOperand(){  
    i = 0;    // Initialize.

    while(buffer[index] != ' ' && buffer[index] != '\t' && buffer[index] != '\n')  
        operand[i++] = buffer[index++];  

    operand[i] = '\0';   // Null charater.

    return operand;  
} 

	/* SkipSpace: void.
		01 Skip a space while reading an empty char or a '\t'.
	*/
void SkipSpace(){  
    while(buffer[index] == ' ' || buffer[index] =='\t')  
        index++;  
}  

	/* RecordSymtab: void(char* label).
		01 arrChar label[32] --> symTab[symCount].label.
		02 arrInt locctr[locctrCount-1] --> symTab[symCount].addr.
	*/
void RecordSymtab(char* label){  
    strcpy(symTab[symCount].label, label);  
    symTab[symCount].addr = locctr[locctrCount-1];  
    symCount++;      
}  

	/* SearchSymtab: int(char* label).
		01 Check the symbol table whether the label has been defined.
			(foundOnSymtabFlag - F:0/ T:1)
		02 RETURN: int foundOnSymtabFlag.
	*/
int SearchSymtab(char* label){  
    foundOnSymtabFlag = 0;   // Initialize.  

    for(int i = 0; i <= symCount; i++){  
        if(!strcmp(symTab[i].label, label)){  
            foundOnSymtabFlag = 1;  
            return foundOnSymtabFlag;    
        }  
    }  

    return foundOnSymtabFlag;  
}  

	/* SearchOptab: int(char* mnem).
		01 Check the opCode table whether the mnemonic has in opcodeTable.
			(foundOnOptabFlag - F:0/ T:1)
		02 RETURN: int foundOnOptabFlag.
	*/
int SearchOptab(char* mnem){  
	foundOnOptabFlag = 0;   // Initialize.  

    int size = sizeof(opTable) / sizeof(OPTAB);   // The number of the opCode.

    for(int i = 0; i < size; i++){  
        if(!strcmp(mnem, opTable[i].mnemonic)){  
            count = i;  
            foundOnOptabFlag = 1;  
        }  
    }  

    return foundOnOptabFlag;  
}  

	/* StrToDec: int(char* c).
		01 Convert the operand in string into decimal.
		02 RETURN: int decNum.
	*/
int StrToDec(char* c){  
    int decNum = 0;   // Initialize.  
    char temp[10];  

    strcpy(temp, c);
	int len = strlen(c);  

	// Conversion.
    for(int i = (len-1), j = 1; i >= 0; i--){  
        decNum = decNum + (int)(temp[i] - '0') * j;  
        j *= 10;  
    }  
    return decNum;   // Decimal.  
}  

	/* StrToHex: int(char* c).
		01 Covert the operand in sting into hexadecimal.
		02 RETURN: int hexNum.
	*/
int StrToHex(char* c){  
    int hexNum = 0;   // Initialize.    
    char temp[10]; 

    strcpy(temp, c);  
    int len = strlen(temp);  

	// Conversion.
    for(int i = (len-1), j = 1; i >= 0; i--){  
        if(temp[i] >= '0' && temp[i] <= '9')  
            hexNum = hexNum + (int)(temp[i] - '0') * j;  
        else if(temp[i] >= 'A' && temp[i] <= 'F')  
            hexNum = hexNum + (int)(temp[i] - 'A' + 10) * j;  
        else if(temp[i] >= 'a' && temp[i] >= 'f')  
            hexNum = hexNum + (int)(temp[i] - 'a' + 10) * j;  
      
        j *= 16;   // hexadecimal.  
    }  

    return hexNum;  
}  

	/* ComputeLen: int(char* c).
		01 Compute the length of each operand.
		02 RETURN: int length.
	*/
int ComputeLen(char* c){  
    unsigned int length;  
    char len[32];  
      
    strcpy(len, c);

    if(len[0] == 'C' || len[0] == 'c' && len[1] == '\''){
        for(length = 2; length <= strlen(len); length++)
            if(len[length] == '\''){
                length -= 2;
				break;
			}
	}   
    else if(len[0] == 'X' || len[0] == 'x' && len[1] == '\'')  
        length = 1;  

    return length;  
}  

	/* CreateProgramList: void.
		01 Create a program list file.
			(1) Process the operator without needing object code.
			(2) Process the operator needed object code.
		02 EXCEPTION:
			(1) Unable to open the list file.
		
	*/
void CreateProgramList(){  
    int loop; 

	char tempName[10];
	strcpy(tempName, filename);

	// Open list file and set "write" mode.
    FILE* fptrList;   
    fptrList = fopen(strcat(tempName, ".list"), "w");  

	// EXCEPTION: Unable to open the list file.
    if(fptrList == NULL){  
        printf("ERROR: Unable to open the sic.list. \n\n");  
		system("pause"); 
    }  

    fprintf(fptrList, "%-4s\t%-10s%-10s%-10s\t%s \n", "LOC", "LABEL", "OPERATOR", "OPERAND", "OBJECT CODE");  

	// Creata a program list file.
    for(loop = 0; loop < arrIndex; loop++){  
        // Process the operator without needing object code. (START/ RESW / RESB/ END)
		if(!strcmp((imrArray[loop] -> operatorField), "START") || 
			!strcmp((imrArray[loop] -> operatorField), "RESW") || 
			!strcmp((imrArray[loop] -> operatorField), "RESB") || 
			!strcmp((imrArray[loop] -> operatorField), "END"))  
            fprintf(fptrList, "%04x\t%-10s%-10s%-10s \n", 
					(imrArray[loop] -> loc), 
					(imrArray[loop] -> labelField), 
					(imrArray[loop] -> operatorField), 
					(imrArray[loop] -> operandField));  
		// Process the operator needed object code.
        else  
            fprintf(fptrList, "%04x\t%-10s%-10s%-10s\t%06x \n", 
					(imrArray[loop] -> loc), 
					(imrArray[loop] -> labelField), 
					(imrArray[loop] -> operatorField), 
					(imrArray[loop] -> operandField),
					(imrArray[loop] -> objCode));  
    }  

    fclose(fptrList);   // Close the list file.
}  

	/* CreateObjectCode: void.
		01 Create a object code.
			(1) Process the operator without needing object code.
			(2) Process the operator needed object code.
		02 EXCEPTION:
			(1) Unable to open the the object code file.
		
	*/
void CreateObjectCode(){  
    int firstAddr, lastAddr, tempAddr;
	int firstIndex, lastIndex; 
	int i, j, loop;  

    int tempObjCode[30];
    char tempOperator[12][10];  
    char tempOperand[12][10];

	// Open object file and set "write" mode.
    FILE* fptrObj;   
    fptrObj = fopen(strcat(filename, ".obj"), "w"); 

	// EXCEPTION: Unable to open the the object code file.
    if(fptrObj == NULL){  
        printf("ERROR: Unable to open the sic.obj. \n\n");  
        system("pause");
    } 

	// HINT: Creating the object code.
    printf("               << Creating Object Code... >> \n\n");
	printf("               <<      Object Code...     >> \n\n");  
    loop = 0;  

	// Process the operator "START" and show the header of object code.
    if(!strcmp((imrArray[loop] -> operatorField), "START")){  
        printf("H%-6s%06x%06x\n", (imrArray[loop] -> labelField), startAddr, progLength);  
        fprintf(fptrObj, "H^%-6s^%06x^%06x \n", (imrArray[loop] -> labelField), startAddr, progLength);  
        loop++;  
    }  

	// Process other operators.
    while(true){  
        firstAddr = (imrArray[loop] -> loc);  
        lastAddr = (imrArray[loop] -> loc) + 27;  
        firstIndex = loop;  

		// Process the operator "RESB", "RESW", and "END".
        for(i = 0, tempAddr = firstAddr; tempAddr <= lastAddr; loop++){  
			if (!strcmp((imrArray[loop] -> operatorField), "END"))       
                break;
			// Process the operator "RESB" and "RESW".
			else if(strcmp((imrArray[loop] -> operatorField), "RESB") && strcmp((imrArray[loop] -> operatorField), "RESW")){  
                tempObjCode[i] = (imrArray[loop] -> objCode);  
                strcpy(tempOperator[i], (imrArray[loop] -> operatorField));  
                strcpy(tempOperand[i], (imrArray[loop] -> operandField));  

                lastIndex = loop + 1;  
                i++;  
            }  

			tempAddr = (imrArray[loop+1] -> loc);  
        }  

		// Show the textfield of object code.
        printf("T%06x%02x", firstAddr, (imrArray[lastIndex] -> loc) - (imrArray[firstIndex] -> loc));  
        fprintf(fptrObj, "T^%06x^%02x", firstAddr, (imrArray[lastIndex] -> loc) - (imrArray[firstIndex] -> loc));

		// Process the operator "BYTE" and "WORD".
        for(j = 0; j < i; j++){  
			// Process the operator "BYTE".
            if((strcmp(tempOperator[j], "BYTE") == 0) && (tempOperand[j][0] == 'X' || tempOperand[j][0] == 'x')){  
				printf("%02x", tempObjCode[j]);  
				fprintf(fptrObj, "^%02x", tempObjCode[j]);  
            } 
			// Process the operator "WORD".
            else{  
                printf("%06x", tempObjCode[j]);  
                fprintf(fptrObj, "^%06x", tempObjCode[j]);  
            }  
        }  

        printf("\n");  
        fprintf(fptrObj, "\n");  

		// Process the operator "END".
        if(!strcmp((imrArray[loop] -> operatorField), "END"))  
            break;  
    }  

	// Show the end record  of the object code.
    printf("E%06x \n\n", startAddr);  
    fprintf(fptrObj, "E^%06x \n\n", startAddr);  

	// Close the object file.
    fclose(fptrObj);  
}  

/* Main Method */  
int main (){  
    FILE* fptr;  

    int loc = 0;  
    int line = 0;  
    int loop;  
    int isEmptyLine, isComment;  

    char label[32];  
    char opcode[32];  
    char operand[32];
	char tempname[10];

	// HINT: Let user open the assembly file (filename.asm).
	printf("          <<  S I C / X E  A S S E M B L E R  >> \n");
    printf("Enter the filename of assembly program (filename): ");  
    scanf("%s", tempname);  
	strcpy(filename, tempname);
    fptr = fopen(strcat(tempname, ".asm"), "r");   // Open the assembly file and set "read" mode.

	// EXCEPTION: Unable to open the file.
    if(fptr == NULL){  
        printf("ERROR: Unable to open the %s file. \n\n", filename);  
        system("pause");
    }  

	// HINT: PASS 1 Processing.  
    printf("               <<   PASS 1 Processing...  >> \n\n"); 
	printf("               <<    Assembly Program...  >> \n\n");  

    while(fgets(buffer, 256, fptr) != NULL){ 
		printf(buffer);
        isEmptyLine = strlen(buffer);  
        i = 0;
		index = 0;  

        strcpy(label, ReadLabel()); 

		// Process the comments.
        if(label[0] == '.')  
            isComment = 1;  
        else  
            isComment = 0;  

		// Process the assembly codes.
        if(isEmptyLine > 1 && isComment != 1){  
			i = 0;
            index = 0;  
          
			/* Initialize the intermediateRecord index.
				01 (IMRecord*)malloc(sizeof(IMRecord)) --> imrArray[arrIndex].
				02 arrIndex --> (imrArray[arrIndex] -> lineIndex).
			*/
            imrArray[arrIndex] = (IMRecord*)malloc(sizeof(IMRecord)); 
            (imrArray[arrIndex] -> lineIndex) = arrIndex; 

			/* Process the label fields.
				01 char* ReadLabel() --> label.
				02 arrChar label --> (imrArray[arrIndex] -> labelField).
			*/
            strcpy(label, ReadLabel());  
            strcpy((imrArray[arrIndex] -> labelField), label);  

            SkipSpace();  

            if(line == 0){  
				/* Process the opCode fields.
					01 char* ReadOprator() --> opcode.
					02 arrChar opcode --> (imrArray[arrIndex] -> operatorField).
				*/
                strcpy(opcode, ReadOprator());  
                strcpy((imrArray[arrIndex] -> operatorField), opcode); 

                if(!strcmp(opcode, "START")){  
                    SkipSpace();  

					/* Process the operand fields.
						01 char* ReadOperand() --> operand.
						02 arrChar operand --> (imrArray[arrIndex] -> operandField).
					*/
                    strcpy(operand, ReadOperand());  
                    strcpy((imrArray[arrIndex] -> operandField), operand);

					// Process the operator "START" and set start address.
                    locctr[locctrCount] = StrToHex(operand);
					startAddr = locctr[locctrCount];  
                }  
                else{
					// Process the operator "START" and set start address.
                    locctr[locctrCount] = 0;
					startAddr = locctr[locctrCount];  
				}
            }  
            else{  
				/* Process the opCode fields.
					01 char* ReadOprator() --> opcode.
					02 arrChar opcode --> (imrArray[arrIndex] -> operatorField).
				*/
                strcpy(opcode, ReadOprator());  
                strcpy((imrArray[arrIndex] -> operatorField), opcode);  

                SkipSpace();  

				/* Process the operand fields.
					01 char* ReadOperand() --> operand.
					02 arrChar operand --> (imrArray[arrIndex] -> operandField).
				*/
                strcpy(operand, ReadOperand());  
                strcpy((imrArray[arrIndex] -> operandField), operand); 

                if(strcmp(opcode, "END")){  
                    if(label[0] != '\0'){  
						// EXCEPTION: Duplicate symbol.
                        if(SearchSymtab(label)){  
                            fclose(fptr);  
                            printf("ERROR: Duplicate symbol. \n\n");  
                            foundOnSymtabFlag = 0;          
							system("pause");
                        }  

                        RecordSymtab(label);  
                    } 

					// Process the oprator "BYTE", "WORD", ""RESB", and "RESW".
                    if(SearchOptab(opcode))  
                        locctr[locctrCount] = loc + (int)(opTable[count].format - '0');  
					else if(!strcmp(opcode, "BYTE"))  
                        locctr[locctrCount] = loc + ComputeLen(operand);  
                    else if(!strcmp(opcode, "WORD"))  
                        locctr[locctrCount] = loc + 3;  
                    else if(!strcmp(opcode, "RESB"))  
                        locctr[locctrCount] = loc + StrToDec(operand);  
                    else if(!strcmp(opcode, "RESW"))  
                        locctr[locctrCount] = loc + 3 * StrToDec(operand);   
                    else{  
						// EXCEPTION: Invalid operation code.
                        fclose(fptr);   
						printf("ERROR: Invalid operation code. \n\n");  
						system("pause");
                    }  
                }
            }  

			/* Process the location fields.
				01 arrInt locctr[locctrCount] --> location.
				02 arrInt locctr[locctrCount-1] --> (imrArray[arrIndex] -> loc).
			*/
            loc = locctr[locctrCount];  
            (imrArray[arrIndex] -> loc) = locctr[locctrCount-1];  
            locctrCount++;  
            arrIndex++;  
        }
		
		line += 1;  
        foundOnOptabFlag = 0;
    }
    progLength = locctr[locctrCount-2] - locctr[0];
	printf("\n\n               <<   PASS 1 Successful...  >> \n\n"); 

	// HINT: PASS 2 Processing. 
    printf("               <<   PASS 2 Processing...  >> \n\n"); 

	/*	PASS 2 Variables:
		01 UnLong instantFormat.
		02 UnLong instantFormatIndex
		03 UnLong instantFormatOpcode
		04 UnLong instantFormatAddress
	*/
    unsigned long instFmt;
    unsigned long instFmtIndex;
    unsigned long instFmtOpcode;
    unsigned long instFmtAddr; 

    for(loop = 1; loop < arrIndex; loop++){
		// Initializing.
		instFmtIndex = 0; 
        instFmtOpcode = 0;  
        instFmtAddr = 0;  

        strcpy(opcode, (imrArray[loop] -> operatorField));  

		// Process the opCodes.
        if(SearchOptab(opcode)){  
            instFmtOpcode = opTable[count].macCode;  
            instFmtOpcode <<= 16;  
            (imrArray[loop] -> objCode) = instFmtOpcode;  
            strcpy(operand, (imrArray[loop] -> operandField));  

			// Process the literal 'X'.
            if(operand[strlen(operand)-2] == ',' && operand[strlen(operand)-1] == 'X'){  
                instFmtIndex = 0x008000;  
                operand[strlen(operand)-2] = '\0';  
            }  
            else  
                instFmtIndex = 0x000000;  

			// Process the labels.
            for(int searchSymtab = 0; searchSymtab < symCount; searchSymtab++){  
                if(!strcmp(operand, symTab[searchSymtab].label))  
                    instFmtAddr = (long)symTab[searchSymtab].addr;  
            }  

            instFmt =  instFmtOpcode + instFmtIndex + instFmtAddr;  
            (imrArray[loop] -> objCode) = instFmt;  
        }  
		// Process the operator while it is "BYTE". 
        else if(!strcmp(opcode, "BYTE")){  
            strcpy(operand, (imrArray[loop] -> operandField));  
            (imrArray[loop] -> objCode) = 0;  

			// Process the literal 'c'.
            if(operand[0] == 'c' || operand[0] == 'C' && operand[1] == '\''){  
                for(int i = 2; i <= (int)(strlen(operand) - 2); i++){  
                    (imrArray[loop] -> objCode) = (imrArray[loop] -> objCode) + (int)(operand[i]);  
                    (imrArray[loop] -> objCode) <<= 8;  
                }  
            } 

			// Process the literal 'x'.
            if(operand[0] == 'x' || operand[0] == 'X' && operand[1] == '\''){  
                char* operandPtr; 

                operandPtr = &operand[2];  
                *(operandPtr + 2) = '\0';  

                for(int i = 2; i <= (int)(strlen(operand) - 2); i++){  
                    (imrArray[loop] -> objCode) = (imrArray[loop] -> objCode) + StrToHex(operandPtr);  
                    (imrArray[loop] -> objCode) <<= 8;  
                }  
            }  

            (imrArray[loop] -> objCode) >>= 8;  
        }
		/* Process the operator while it is "WORD". 
			01 arrChar operand --> (imrArray[loop] -> operandField).
			02 arrInt StrToDec(operand) -->  (imrArray[loop] -> objCode)
		*/
		else if(!strcmp(opcode, "WORD")){  
            strcpy(operand, (imrArray[loop] -> operandField));  
            (imrArray[loop] -> objCode) = StrToDec(operand);  
        }  
    }  

	// Create program list file and object code file.
	CreateProgramList();  
	CreateObjectCode(); 

	// Free the memory.
	for (loop = 0; loop < arrIndex; loop++)  
        free(imrArray[loop]);  

	// HINT: Completed assembly.
	printf("               <<   PASS 2 Successful...  >> \n\n"); 
	printf("               <<  Completed Assembly...  >> \n\n");
    fclose(fptr);  
	system("pause");

	return 0;
}  