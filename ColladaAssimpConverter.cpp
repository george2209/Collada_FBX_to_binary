// ColladaAssimpConverter.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

// xcopy /y/d "C:\\Users\\George\\Desktop\\Blender\\my_trees\\tree1.bin" "C:\\Users\\George\\AndroidStudioProjects\\PlanesAndShips\\app\\src\\main\\assets\\obj\\tree1.bin"

#include "CDAEParser.h"
#include<iostream> //cout
#include "my_utils.h"
#include <cassert>

using namespace std;

#define _MY_HELPMESSAGE_TAG_ "-h"
#define _MY_INPUT_FILE_NAME_TAG "-i="
#define _MY_OUTPUT_FILE_NAME_TAG "-o="

void printHelpMessage()
{
    cout << "Usage:" << endl;
    cout << "\tColladaAssimpConverter.exe <-h> <-i=INPUT_PATH_AND_FILE_NAME> <-o=OUTPUT_PATH_AND_FILE_NAME>" << endl;
    cout << endl << endl << "Where:" << endl;
    cout << "-h\t print this message" << endl;
    cout << "-i\t specifies the input Collada file name including the full path" << endl;
    cout << "-o\t specifies the output file name including the full path you expect to be generated by this convertor." << endl;
    cout << endl << "EXAMPLE:" << endl << "ColladaAssimpConverter.exe -i=\"C:\\George_Working_Folder\\Blender\\tmp\\tmp.dae\" -o=\"C:\\George_Working_Folder\\tmp\\tmp.bin\"" << endl;
}

int main(int argNo, char* argumentsArr[])
{
    using namespace my_collada_parser;

    //const char* pFileInput = "C:\\Users\\George\\Desktop\\Blender\\tmp\\tmp.dae";
    //const char* pFileOutput = "C:\\Users\\George\\Desktop\\Blender\\tmp\\tmp.bin";
    char* pFileInput = NULL;
    char* pFileOutput = NULL;

    if (argNo != 3)
    {
        printHelpMessage();
    } else{
        for (int i = 1; i < argNo; i++)
        {
            const char* pArgument = argumentsArr[i];
            if (strncmp(_MY_HELPMESSAGE_TAG_, pArgument, 2) == 0)
            {
                printHelpMessage();
                break;
            }
            else if (strncmp(_MY_INPUT_FILE_NAME_TAG, pArgument, 3) == 0) {
                assert(pFileInput == NULL);
                size_t lengthArgument = strlen(pArgument);
                pFileInput = new char[lengthArgument - 2];
                pFileInput[lengthArgument - 3] = NULL;
                COPY_CHAR_ARRAYS(pArgument, 3, pFileInput, 0, lengthArgument - 3);
            }
            else if (strncmp(_MY_OUTPUT_FILE_NAME_TAG, pArgument, 3) == 0) {
                assert(pFileOutput == NULL);
                size_t lengthArgument = strlen(pArgument);
                pFileOutput = new char[lengthArgument - 2];
                pFileOutput[lengthArgument - 3] = NULL;
                COPY_CHAR_ARRAYS(pArgument, 3, pFileOutput, 0, lengthArgument - 3);
            }
            else {
                cout << "Invalid argument " << pArgument << endl << "For help use:" << endl << "\tColladaAssimpConverter.exe - h" << endl;
                break;
            }
        }

        if (pFileInput != NULL && pFileOutput != NULL)
        {
            CDAEParser myParser;
            myParser.startParsing(pFileInput, pFileOutput);
        }
        else {
            printHelpMessage();
        }
    }

    DELETE_ARR(pFileInput);
    DELETE_ARR(pFileOutput);

    return 0;
}