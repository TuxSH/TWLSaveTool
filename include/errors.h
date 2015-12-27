#pragma once
#include <3ds.h>
#include <stdexcept>


struct Error : public std::runtime_error {
	Error(u32 code, const char* fn, int l) : std::runtime_error("It failed"), errorCode(code), fileName(fn), line(l) {}
	 
	u32 getErrorCode(void) const throw(){
		 return errorCode;
	 }
	 
	const char* getFileName(void) const throw(){
		 return fileName;
	 }
	
	int getLine(void) const throw(){
		return line;
	} 
	 
	 virtual const char* what() const throw(){
		 return "It failed";
	 }

	 protected: 
	 u32 errorCode;
	 const char* fileName;
	 int line;
};