#pragma once
#include "TConfig.h"
#include "TSymbolTable.h"

class TOutput {
public:
	TOutput(TConfig& cfg, TSymbolTable& symTable);
	TConfig& cfg;
	TSymbolTable& symTable;

	void write_symbol_table();
};