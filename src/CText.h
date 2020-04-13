/*
Config parser originally write to Guitar++ https://github.com/Fabio3rs/Guitar-PlusPlus
Write by Fabio3rs - https://github.com/Fabio3rs
*/
#pragma once
#define _CRT_SECURE_NO_WARNINGS
#ifndef __GUITARPP_CTEXT_H_
#define __GUITARPP_CTEXT_H_

#include <fstream>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <string>
#include <exception>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <memory>
#include <utility>
#include <array>

#ifdef _WIN64
typedef int64_t integer;
#else
typedef int integer;
#endif

class argToString
{
	const std::string str;

public:
	const std::string &getStr() const { return str; }

	argToString(bool value) : str(value ? "true" : "false") { }

	argToString(const char *str) : str(str) { }

	argToString(const std::exception &e) : str(e.what()) { }

	argToString(const std::string &str) : str(str) { }

	template<class T>
	argToString(const T &value) : str(std::to_string(value)) { }
};

template<class... Types>
std::string multiRegister(const std::string &format, Types&&... args)
{
	const std::array < argToString, std::tuple_size<std::tuple<Types...>>::value > a = { std::forward<Types>(args)... };
	std::string printbuf, numbuf;

	bool ignoreNext = false;

	for (int i = 0, size = format.size(); i < size; i++)
	{
		auto ch = format[i];
		int ti = i + 1;

		switch (ch)
		{
		case '\\':
			if (ignoreNext)
			{
				printbuf.insert(printbuf.end(), 1, ch);
				ignoreNext = false;
				break;
			}

			ignoreNext = true;
			break;

		case '%':
			if (ignoreNext)
			{
				printbuf.insert(printbuf.end(), 1, ch);
				ignoreNext = false;
				break;
			}

			numbuf = "";

			{
				bool stringEnd = true;
				while (ti < size)
				{
					if (format[ti] < '0' || format[ti] > '9')
					{
						i = ti - 1;
						if (numbuf.size() > 0)
						{
							size_t argId = std::stoul(numbuf);

							if (argId >= 0 && argId < a.size())
							{
								printbuf += a[argId].getStr();
							}
							else
							{
								printbuf += "%";
								printbuf += numbuf;
							}

							stringEnd = false;

							break;
						}
					}
					else
					{
						numbuf.insert(numbuf.end(), 1, format[ti]);
					}

					ti++;
				}

				if (stringEnd)
				{
					i = size;
					if (numbuf.size() > 0)
					{
						size_t argId = std::stoul(numbuf);

						if (argId >= 0 && argId < a.size())
						{
							printbuf += a[argId].getStr();
						}
						else
						{
							printbuf += "%";
							printbuf += numbuf;
						}
					}
				}
			}
			break;

		default:
			ignoreNext = false;
			printbuf.insert(printbuf.end(), 1, ch);
			break;
		}
	}

	return printbuf;
}

class CText{
	class tstring : public std::string{
	public:
		int to_int(){
			return std::stoi(*this);
		}

		long to_long(){
			return std::stol(*this);
		}

		long long to_longlong(){
			return std::stol(*this);
		}

		float to_float(){
			return std::stof(*this);
		}

		double to_double(){
			return std::stod(*this);
		}

		tstring(const std::string &value) : std::string(value){}
		tstring(const char *value) : std::string(value){}

		tstring() : std::string(){}
	};

public:
	typedef struct{
		tstring name, content;
	} field_t;

	typedef struct{
		std::string name;
		std::deque<field_t> fields;

		integer getFieldIDByName(const std::string &fieldName){
			for(size_t i = 0, size = fields.size(); i < size; i++){
				if(fields[i].name == fieldName){
					return i;
				}
			}

			return -1;
		}

		tstring &operator[] (const std::string &fieldName){
			integer fieldID = getFieldIDByName(fieldName);

			if(fieldID == -1){
				throw std::logic_error("Field not found");
			}

			return fields[fieldID].content;
		}
	} table_t;


private:
	size_t						fileSize;
	std::fstream				file;
	std::deque<table_t>			tables;
	std::string					fileName;

public:
	void Parse();

	const bool is_open(){
		return file.good();
	}

	void open(const char *name, bool autoParse = true);
	void save();

	integer getTableIDByName(const std::string &tableName){
		if(tableName.length() == 0) return 0;

		for(size_t i = 0, size = tables.size(); i < size; i++){
			if(tables[i].name == tableName){
				return i;
			}
		}

		return -1;
	}

	table_t &operator[](const std::string &tableName){
		integer tableID = getTableIDByName(tableName);

		if(tableID == -1){
			throw std::logic_error("Table not found");
		}

		return tables[tableID];
	}

	CText();
	CText(const char *name, bool autoParse = true);
};

#endif
