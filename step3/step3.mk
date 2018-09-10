##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=step3
ConfigurationName      :=Debug
WorkspacePath          :=D:/github/StepFC
ProjectPath            :=D:/github/StepFC/step3
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=dustpg
Date                   :=10/09/2018
CodeLitePath           :="C:/Program Files/CodeLite"
LinkerName             :=gcc
SharedObjectLinkerName :=gcc -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.o.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E 
ObjectsFileList        :="step3.txt"
PCHCompileFlags        :=
MakeDirCommand         :=makedir
RcCmpOptions           := 
RcCompilerName         :=windres
LinkOptions            :=  -O0
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch). $(LibraryPathSwitch)Debug 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := ar rcus
CXX      := gcc
CC       := gcc
CXXFLAGS :=  -g -Wall $(Preprocessors)
CFLAGS   :=   $(Preprocessors)
ASFLAGS  := 
AS       := as


##
## User defined environment variables
##
CodeLiteDir:=C:\Program Files\CodeLite
Objects0=$(IntermediateDirectory)/sfc_mapper.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_famicom.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_cpu.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_6502.c$(ObjectSuffix) $(IntermediateDirectory)/main.c$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

MakeIntermediateDirs:
	@$(MakeDirCommand) "./Debug"


$(IntermediateDirectory)/.d:
	@$(MakeDirCommand) "./Debug"

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/sfc_mapper.c$(ObjectSuffix): sfc_mapper.c $(IntermediateDirectory)/sfc_mapper.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/step3/sfc_mapper.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_mapper.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_mapper.c$(DependSuffix): sfc_mapper.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_mapper.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_mapper.c$(DependSuffix) -MM sfc_mapper.c

$(IntermediateDirectory)/sfc_mapper.c$(PreprocessSuffix): sfc_mapper.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_mapper.c$(PreprocessSuffix) sfc_mapper.c

$(IntermediateDirectory)/sfc_famicom.c$(ObjectSuffix): sfc_famicom.c $(IntermediateDirectory)/sfc_famicom.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/step3/sfc_famicom.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_famicom.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_famicom.c$(DependSuffix): sfc_famicom.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_famicom.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_famicom.c$(DependSuffix) -MM sfc_famicom.c

$(IntermediateDirectory)/sfc_famicom.c$(PreprocessSuffix): sfc_famicom.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_famicom.c$(PreprocessSuffix) sfc_famicom.c

$(IntermediateDirectory)/sfc_cpu.c$(ObjectSuffix): sfc_cpu.c $(IntermediateDirectory)/sfc_cpu.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/step3/sfc_cpu.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_cpu.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_cpu.c$(DependSuffix): sfc_cpu.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_cpu.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_cpu.c$(DependSuffix) -MM sfc_cpu.c

$(IntermediateDirectory)/sfc_cpu.c$(PreprocessSuffix): sfc_cpu.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_cpu.c$(PreprocessSuffix) sfc_cpu.c

$(IntermediateDirectory)/sfc_6502.c$(ObjectSuffix): sfc_6502.c $(IntermediateDirectory)/sfc_6502.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/step3/sfc_6502.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_6502.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_6502.c$(DependSuffix): sfc_6502.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_6502.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_6502.c$(DependSuffix) -MM sfc_6502.c

$(IntermediateDirectory)/sfc_6502.c$(PreprocessSuffix): sfc_6502.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_6502.c$(PreprocessSuffix) sfc_6502.c

$(IntermediateDirectory)/main.c$(ObjectSuffix): main.c $(IntermediateDirectory)/main.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/step3/main.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main.c$(DependSuffix): main.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/main.c$(ObjectSuffix) -MF$(IntermediateDirectory)/main.c$(DependSuffix) -MM main.c

$(IntermediateDirectory)/main.c$(PreprocessSuffix): main.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main.c$(PreprocessSuffix) main.c


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Debug/


