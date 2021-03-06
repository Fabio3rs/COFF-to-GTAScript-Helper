/*
Config parser originally write to Guitar++ https://github.com/Fabio3rs/Guitar-PlusPlus
Write by Fabio3rs - https://github.com/Fabio3rs
*/
#include "CText.h"

#include <cctype>
#include <iostream>

void CText::Parse(){
	if(fileName.length() == 0){
		return;
	}
	
	if(is_open()){
		file.close();
	}

	file.open(fileName, std::ios::in | std::ios::out | std::ios::binary);

	if(!is_open()){
		throw std::logic_error(std::string("Can't open file ") + fileName);
	}

	tables.clear();
	
	char *content = nullptr;
	
	file.seekg(0, std::ios::end);
	fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	if(fileSize == -1L || fileSize == 0){
		return;
	}
	
	content = new char[fileSize + 4];
	memset(content, 0, fileSize + 4);
	
	if(!content){
		throw std::logic_error("Alloc space fail");
	}
	
	file.read(content, fileSize);
	
	content[fileSize] = 0;
	
	char bufferA[128], bufferB[2048];
	
	integer workingInScope = 0;
	
	table_t globalTable;
	globalTable.name = "GLOBAL";
	tables.push_back(globalTable);

	for(size_t i = 0; i < fileSize; i++){
		while(!isprint((unsigned char)content[i])) i++;

		*bufferA = 0;
		*bufferB = 0;

		int scanResult = sscanf(&content[i], "%127s %2047[^\t\n\r]", bufferA, bufferB);
		
		if(*bufferA == '@'){
			integer tempWorkingScope = 0;

			if((tempWorkingScope = getTableIDByName(&bufferA[1])) != -1){
				workingInScope = tempWorkingScope;
			}else if(bufferA[1]){
				table_t newTable;
				newTable.name = &bufferA[1];

				tables.push_back(newTable);

				workingInScope = tables.size() - (int64_t)1;
			}else{
				workingInScope = 0;
			}
		}
		else if (*bufferA == '#'){
		}
		else{
			field_t newField;

			switch(scanResult){
			case 2:
				newField.content = bufferB;

			case 1:
				newField.name = bufferA;
				tables[workingInScope].fields.push_back(newField);
				break;
			}
		}

		while(content[i] != '\n' && content[i] != '\r' && content[i] != 0) i++;
		i--;
	}

	delete[] content;
}

void CText::open(const char *name, bool autoParse){
	fileName = name;
	if (autoParse) Parse();
}

CText::CText(){
	fileSize = 0;

}

void CText::save(){
	if (is_open()){
		file.close();
	}

	file.open(fileName, std::ios::out | std::ios::trunc);
	file.close();

	file.open(fileName, std::ios::in | std::ios::out);

	for (int i = 0, size = tables.size(); i < size; i++){
		file << "@" << tables[i].name << "\n";
		for (int j = 0, jsize = tables[i].fields.size(); j < jsize; j++){
			file << tables[i].fields[j].name << " " << tables[i].fields[j].content << "\n";
		}
		file << "\n###### fstream bugs everywhere ######";
	}
}

CText::CText(const char *name, bool autoParse){
	fileName = name;
	if(autoParse) Parse();
}

