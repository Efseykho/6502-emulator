##
## Auto Generated makefile, please do not edit
##
WXWIN:=C:\wxWidgets-2.8.7
WXCFG:=gcc_dll\mswu
ProjectName:=6502

## Debug
ConfigurationName :=Debug
IntermediateDirectory :=./Debug
OutDir := $(IntermediateDirectory)
LinkerName:=gcc
ArchiveTool :=ar rcu
SharedObjectLinkerName :=gcc -shared -fPIC
ObjectSuffix :=.o
DebugSwitch :=-g 
IncludeSwitch :=-I
LibrarySwitch :=-l
OutputSwitch :=-o 
LibraryPathSwitch :=-L
PreprocessorSwitch :=-D
SourceSwitch :=-c 
CompilerName :=gcc
OutputFile :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors :=
ObjectSwitch :=-o 
ArchiveOutputSwitch := 
CmpOptions :=-g $(Preprocessors)
LinkOptions := 
IncludePath :=  "$(IncludeSwitch)." 
RcIncludePath :=
Libs :=
LibPath := 


Objects=$(IntermediateDirectory)/unit_test$(ObjectSuffix) $(IntermediateDirectory)/em_6502$(ObjectSuffix) $(IntermediateDirectory)/harness$(ObjectSuffix) 

##
## Main Build Tragets 
##
all: $(OutputFile)

$(OutputFile): makeDirStep  $(Objects)
	@makedir $(@D)
	$(LinkerName) $(OutputSwitch)$(OutputFile) $(Objects) $(LibPath) $(Libs) $(LinkOptions)

makeDirStep:
	@makedir "./Debug"

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/unit_test$(ObjectSuffix): unit_test.c $(IntermediateDirectory)/unit_test$(ObjectSuffix).d
	@makedir "./Debug"
	$(CompilerName) $(SourceSwitch) "C:/Documents and Settings/Leonid/Desktop/6502 emulator/6502/unit_test.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/unit_test$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/unit_test$(ObjectSuffix).d: unit_test.c
	@makedir "./Debug"
	@$(CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/unit_test$(ObjectSuffix) -MF$(IntermediateDirectory)/unit_test$(ObjectSuffix).d -MM "C:/Documents and Settings/Leonid/Desktop/6502 emulator/6502/unit_test.c"

$(IntermediateDirectory)/em_6502$(ObjectSuffix): em_6502.c $(IntermediateDirectory)/em_6502$(ObjectSuffix).d
	@makedir "./Debug"
	$(CompilerName) $(SourceSwitch) "C:/Documents and Settings/Leonid/Desktop/6502 emulator/6502/em_6502.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/em_6502$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/em_6502$(ObjectSuffix).d: em_6502.c
	@makedir "./Debug"
	@$(CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/em_6502$(ObjectSuffix) -MF$(IntermediateDirectory)/em_6502$(ObjectSuffix).d -MM "C:/Documents and Settings/Leonid/Desktop/6502 emulator/6502/em_6502.c"

$(IntermediateDirectory)/harness$(ObjectSuffix): harness.c $(IntermediateDirectory)/harness$(ObjectSuffix).d
	@makedir "./Debug"
	$(CompilerName) $(SourceSwitch) "C:/Documents and Settings/Leonid/Desktop/6502 emulator/6502/harness.c" $(CmpOptions) $(ObjectSwitch)$(IntermediateDirectory)/harness$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/harness$(ObjectSuffix).d: harness.c
	@makedir "./Debug"
	@$(CompilerName) $(CmpOptions) $(IncludePath) -MT$(IntermediateDirectory)/harness$(ObjectSuffix) -MF$(IntermediateDirectory)/harness$(ObjectSuffix).d -MM "C:/Documents and Settings/Leonid/Desktop/6502 emulator/6502/harness.c"

##
## Clean
##
clean:
	$(RM) $(IntermediateDirectory)/unit_test$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/unit_test$(ObjectSuffix).d
	$(RM) $(IntermediateDirectory)/em_6502$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/em_6502$(ObjectSuffix).d
	$(RM) $(IntermediateDirectory)/harness$(ObjectSuffix)
	$(RM) $(IntermediateDirectory)/harness$(ObjectSuffix).d
	$(RM) $(OutputFile)
	$(RM) $(OutputFile).exe

-include $(IntermediateDirectory)/*.d


