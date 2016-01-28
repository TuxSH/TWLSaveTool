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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/
 */

#pragma once

#include <3ds.h>
#include <string>
#include <utility>
#include <algorithm>
#include <cstdio>
#include "errors.h"
#include "SPI.h"

class TWLCard{
public:
	struct Header {
		std::string gameTitle, gameCode, makerCode;
		bool isTWL; // "DSi enhanced"
		
		Header(u8* data = NULL);
	 };
	 
	std::string generateFileName(void) const;
	bool isTWL(void) const;
	Header cardHeader(void) const;
	u32 saveSize(void) const;
	CardType cardType(void) const;
	
	void backupSaveFile(u8* out, void (*cb)(u32, u32)) const;
	void backupSaveFile(std::string const& filename, void (*cb)(u32, u32)) const;
	
	void restoreSaveFile(u8* in, void (*cb)(u32, u32)) const;
	void restoreSaveFile(std::string const& filename, void (*cb)(u32, u32)) const;	
	 
	void eraseSaveData(void (*cb)(u32, u32)) const;
	
	TWLCard(void);
private:
	bool twl;
	Header h;
	CardType cardType_;
};

 
 
 