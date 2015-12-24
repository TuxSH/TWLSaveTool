 #pragma once
 #include <3ds.h>
 #include <stdexcept>
 
struct Error : public std::runtime_error {
	Error(u32 code) : std::runtime_error("It failed"), errorCode(code) {}
	 
	 u32 getErrorCode(void) const throw(){
		 return errorCode;
	 }
	 
	 virtual const char* what() const throw(){
		 return "It failed";
	 }

	 protected: 
	 u32 errorCode;
};