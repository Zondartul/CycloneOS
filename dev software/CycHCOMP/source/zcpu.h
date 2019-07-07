/*
	How is a zCPU initialized?

	1. take out a toolgun in zCPU mode
	2. open editor
	3. select code, click "save and exit"
	4. shoot ground
	
	
	------------- wire/lua/wire/tool_loader.lua -------------
	see https://wiki.garrysmod.com/page/Structures/TOOL for basic tool setup
	
	the wiremod cpu s.tool is a gmod lua "TOOL".
	
	hook TOOL:LeftClick() -> for WireToolObj also calls
		self:LeftClick_Make(trace,ply) and
		self:LeftClick_PostMake
		
	WireToolObj:LeftClick_Make(trace,ply):
		CheckHitOwnClass(trace) -> 
			LeftClick_Update(trace)
		self:MakeEnt(ply, model, ang, trace)
		self:PostMake_SetPos(ent, trace)
	
	WireToolObj:MakeEnt(ply, model, ang, trace)
		WireLib.MakeWireEnt( 
			ply, 
			{
				Class = self.WireClass, 
				Pos=trace.HitPos, 
				Angle=Ang, 
				Model=model
			}, 
			self:GetConVars() )
		
	WireToolObj:LeftClick_Update(trace)
		trace.Entity:Setup(self:GetConVars())
		
	WireToolObj:LeftClick_PostMake(ent,ply,trace)
		parents, welds, nocollides
		self:PostMake()
		duplicator.ApplyEntityModifiers(ply,ent)
		
	WireToolObj:Reload(trace)
		stuff for plain welders
		
	WireToolObj:Think()
		self:UpdateGhost()
		
	WireToolObj:CheckHitOwnClass(trace)
		trace.Entity:GetClass() = self.WireClass
		
	also, LeftClock, RightClick, Reload fire in SP
	because of
		util.AddNetworkString("wire_singleplayer_tool_wtfgarry")
			
	------------- wire/lua/wire/stools/cpu.lua --------------
	
	
	
	1. init: 
	WireToolSetup.setCategory( "Chips, Gates", "Advanced" )
		every following tool will be in this category in the menu
	
	WireToolSetup.open( "cpu", "CPU", "gmod_wire_cpu", nil, "CPUs" )
		s_mode = "cpu" // Tool_mode
		s_name = "CPU" // Proper name
		s_class = "gmod_wire_cpu" //for tools that make a device, can be nil
		f_toolmakeent = nil //serverside function for making the tools device
		s_pluralname = "CPUs" //plural proper name
		
		TOOL.WireClass = s_class
		
	2. right click: open editor
		net.Start("ZCPU_OpenEditor") net.Send(self:GetOwner())
		...
		ZCPU_Editor = vgui.Create("Expression2EditorFrame")
		ZCPU_Editor:Setup("ZCPU Editor", "cpuchip", "CPU")
		ZCPU_Editor:Open()
		
	3. editor stuff
	???
	
	4. left click on ground: TOOL:MakeEnt(...)
		ent = WireLib.MakeWireEnt(
			ply,
			{
				Class = self.WireClass, //= "gmod_wire_cpu"
				Pos = trace.HitPos,
				Angle = Ang,
				Model = model
			})
		ent:setMemoryModel(self:GetClientInfo("memorymodel")) //64krom 
		self:LeftClick_Update(trace)
		
	5.  after shooting ground or when shooting existing cpu:	
		TOOL:LeftClick_Update(trace)
		CPULib.SetUploadTarget(trace.Entity, owner)
			CPULib.DataBuffer[player:userID()] =
				{
					Entity = entity (the zcpu chip)
					Player = player (owner)
					Data = {}
				}
		net.Start("ZCPU_RequestCode")
		net.Start("CPULib.InvalidateDebugger")
		...
		ZCPU_RequestCode():
		----------------------------------------------------
		|	CPULib.Compile(
		|		ZCPU_Editor:GetCode(),
		|		ZCPU_Editor:GetChosenFile(),
		|		compile_success (callback),
		|		compile_error (callback)
		|		)
		|		HCOMP:StartCompile(source,filename or "source",
		|							CPULib.OnWriteByte, nil)
		|		
		|		hc_output.lua calls HCOMP:WriteByte(byte,block)
		|			which calls
		|		HCOMP.WriteByteCallback
		|			which is set to
		|		CPULib.OnWriteByte(HCOMP.WriteByteCaller,HCOMP.WritePointer,byte)
		|			which does
		|		CPULib.OnWriteByte(caller,address,byte)	
		|			CPULib.CurrentBuffer[address] = byte
		|			//calller is ignored
		|		end
		|	
		|       	sucessful compile calls:		
		|	compile_sucess()
		|		CPULib.Upload()
		|	end
		----------------------------------------------------		
		results in a call to CPULib.Upload()
		
		CPULib.Upload():
			net.Start("wire_cpulib_bufferstart"), string CPULib.CPUName
			CPULib.RemainingData <- CPULib.Buffer
			timer.Create("cpulib_upload",0.5,0,CPULib.OnUploadTimer)
		
		CPULib.OnUploadTimer():
			net.Start("wire_cpulib_buffer")
				add every memcell as uint and also double
			net.SendToServer()
			
		net.Receive("wire_cpulib_bufferstart"):
			(server) set DataBuffer to player,cpuchip entity.
			net.Start("CPULib.ServerUploading")
		
		------------- cpu STOOL ---------------------
		net.Receive("CPULib.ServerUploading")
			CPULib.ServerUploading = bit
			
		------------- cpulib.lua --------------------
		net.Receive("wire_cpulib_buffer")
  ! ------->Buffer.Entity:FlashData(Buffer.data)
		

	---------------------------------------------------------
	
	
	so tl;dr: 
	------------------------------
	|cpuchip = WireLib.MakeWireEnt(
	|			player owner,
	|			{
	|				Class = "gmod_wire_cpu"
	|				Pos = trace.HitPos,
	|				Angle = Ang,
	|				Model = model
	|			})
	|cpuchip:setMemoryModel("64krom") 
	|	
	|setUploadTarget(cpuchip entity, owner player)
	|CPULib.Compile(
	|			source_code = myCode
	|			file = "source"
	|			compile_success = function() CPULib.upload() end,
	|			compile_error = function() print("error") end
	|			)
	------------------------

	for output: write to port 0 serially?
	


----------------- wire/lua/wire/server/wirelib.lua --------------------------------
	WireLib.MakeWireEnt(pl, Data, ...)
		ent:Spawn()
			initializes the entity and starts its networking.
			calls ENTITY:Initialize()
		
		ent:Activate()
			"activates the entity"
	
		ent:Setup(...)
----------------- wire/lua/entities/gmod_wire_cpu.lua -----------------------------
	ENT:Initialize()
		Wire_CreateInputs, Wire_CreateOutputs,
		
	self.VM = CPULib.VirtualMachine()
	
	ENT:ReadCell(Address)
	ENT:WriteCell(Address,Value)


*/