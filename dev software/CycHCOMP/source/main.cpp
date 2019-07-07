/*

	The C port of the GLua compiler for zCPU, HCOMP.

	
	Principle of operation:
	
	---- wire/lua/wire/stools/cpu.lua --------------------------
	1. toolgun left click -> zCPU spawned
	2. SERVER -> netMessage "ZCPU_RequestCode" -> CLIENT
	3. CPULib.Compile(
					ZCPU_Editor:GetCode(), 			//source code to compile
					ZCPU_Editor:GetChosenFile(), 	//file to compile
					compile_success,				//callback
					compile_error					//callback
					)		
	---- wire/lua/wire/cpulib.lua ------------------------------
	4. HCOMP::StartCompile(source,fileName or "source",CPULib.OnWriteByte,nil)
			init things
	5. timer.Create("cpulib_compile",1/60,0,CPULib.OnCompileTimer)
	6. OnCompileTimer:
		for 1 to compileSpeed
			all_ok, not_done = HCOMP::Compile()
	---- wire/lua/wire/client/hlzasm/hc_compiler.lua -----------
	7. HCOMP::Compile()
		if stage == tokenize then HCOMP:Tokenize() 				and return "all ok (true), not done (nil/false)"
		if stage == parse then HCOMP:Statement() 				or return "not ok (false), error (message)"
		if stage == generate then HCOMP:StageGenerateLeaf()	
		if stage == optimize then HCOMP:OptimizeCode()
		if stage == resolve then HCOMP:Resolve()
		if stage == output then HCOMP:Output()
		return "all ok (true), done (true)"
	
	note, pcall(function() return A, B end) returns "true, A, B"
	
	---- wire/lua/wire/client/hlzasm/hc_tokenize.lua -----------
	HCOMP:Tokenize():
	1. skip whitespaces
	2. read token pos
	3. check for EOF								gotToken(EOF), return "stage done/false" 
	4. check for #preproc macro (and execute)
	5. if #ifdef'ed then skip
	6. parse strings/chars							gotToken(string/char), return "stage continue/true"
	7. parse token (ident)							
	8. check if token is redefined (and replace)	gotToken(ident), return "stage continue"
	9. parse special characters
		including comments							gotToken(comment), return "stage continue"
		two-char and one-char symbols				gotToken(special), return "stage continue"
	10. error unknown token							throw self:Error()
	
	---- wire/lua/wire/client/hlzasm/hc_syntax.lua -----------
	HCOMP:Statement()
	1.  parse ';'	return continue
	2.  parse EOF	return done
	3.  parse var/func definition	
			read signature			return continue
	4.  peek struct, define var?
	5.  parse 'preserve'/'zap'		return continue
			note: BlockDepth > 0 if inside a function
	6.  parse assembly instruction	return continue
	7.  parse assembly macros:		return continue
			DATA, CODE, ORG, OFFSET,
			DB, STRING, DEFINE(?), ALLOC,
	8.  parse return					return continue
	9.  parse if						return continue
	10. parse while						return continue
	11. parse FOR						return continue
	12. parse continue
	13.	parse break
	14. parse goto
	15. parse '{' and '}'
	16. parse label def.
	17. parse expression
	18. skip a ';'.
	
	parses using MatchToken(type) and ExpectToken(type) and RestoreParserState().
	when something parsed:
		leaf = newLeaf()		leafs represent opcodes (assembly instructions? or RTL?)
		leaf.data = stuff		before their position is known
		AddLeafToTail(leaf)
	
	---- wire/lua/wire/client/hlzasm/hc_codetree.lua ---------------
	HCOMP:StageGenerateLeaf(leaf)
	1. if has parent leaf (not root) and 
		parent not referenced, then do not generate
	2. generate marker
	3. mark busy registers
	4. GenerateLeaf(leaf) -- not incremental
	5. return false;
	
	GenerateLeaf:
	1. Generate previous leaf
	2. if no opcode, and no previous leaf, and 
		either istable(leaf.constant) or leaf.forcetemporary
		then invalid code
	3. Generate leafs for operands, if any
	4. do some label magic for operands??
	5. read opearand from somewhere
	6. do something with temporaries?
	7. ???
	8. write the return result?
	9. generate opcode
	10.generate next leaf
	11.reset temporary registers
	12.return destRegister and whether it is temporary
	
	---- wire/lua/wire/client/hlzasm/hc_optimize.lua --------------
	HCOMP:OptimizeCode()					assembly-level optimizations like mov a,a -> nothing
	for all opcodes,
	 for all optimization patterns
	1. check code is long enough to fit pattern
	2. check that pattern matches
	3. delete old code
	4. add new code
	
	---- wire/lua/wire/client/hlzasm/hc_output.lua -----------------
	HCOMP:Resolve()
	1. set block offset to write pointer
	2. set pointer offset
	3. label goes before the opcode
	4. icrement write pointer by instruction size
	5. increment by extra data size
	6. add zero padding?
	7. change write pointer if requested
	8. change pointer offset if requested
	9. output resolve listing
	10. output as lib if requested (not implemented)
	
	----------------------------------------------------------------
	HCOMP:Output()
	1. output library if requested
	2. resolve constants in opcodes
		if constant or memory pointer: parse constant expressions with parser
	3. resolve constants in data
	4. print final listing
	5. WriteBlock(block)
	
	WriteBlock(block)
	1. if opcode, calculate opcode,RM,segment,and immediate bytes; output.
	2. if data, write data.
	2. write zero padding if requested.
	-----------------------------------------------------------------
	done.
	
	
	
	
	
	
	
	
	
	
*/

int main(int argc, char **argv){
	
}