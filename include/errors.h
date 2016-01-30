/*
 *  This file is part of TWLSaveTool.
 *  Copyright (C) 2015-2016 TuxSH
 *
 *  TWLSaveTool is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>
 */
 
#pragma once
#include <3ds.h>
#include <stdexcept>
#include <cstdio>
#include <string>

struct Error : public std::runtime_error {
	Error(u32 code, const char* fn, int l) : std::runtime_error("result != 0"), errorCode(code), fileName(fn), line(l) {}
	 
	u32 getErrorCode(void) const throw(){
		return errorCode;
	 }
	 
	const char* getFileName(void) const throw(){
		return fileName;
	 }
	
	int getLine(void) const throw(){
		return line;
	} 
	 
	virtual const char* what(void) const throw(){
		return "result != 0";
	}
	
	void describe(void) const throw(){
		std::string fname(fileName);
		fname = fname.substr(fname.find_last_of('/'));
		
		printf("\x1B[31mAn error occured:\x1B[0m\n");
		printf("\t\x1B[31m- Error code: %lx\x1B[0m\n", errorCode);
		printf("\t\x1B[31m- File: %s, line %d\x1B[0m\n\n", fname.c_str(), line);
	}
	 

protected: 
	u32 errorCode;
	const char* fileName;
	int line;
};