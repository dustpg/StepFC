##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=step5
ConfigurationName      :=Debug
WorkspacePath          :=D:/github/StepFC
ProjectPath            :=D:/github/StepFC/step5
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=dustpg
Date                   :=11/09/2018
CodeLitePath           :="C:/Program Files/CodeLite"
LinkerName             :=g++
SharedObjectLinkerName :=g++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.o.i
DebugSwitch            :=-gstab
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=$(PreprocessorSwitch)_WIN32_WINNT=0x0601 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E 
ObjectsFileList        :="step5.txt"
PCHCompileFlags        :=
MakeDirCommand         :=makedir
RcCmpOptions           := 
RcCompilerName         :=windres
LinkOptions            :=  -O0
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)d3d11 $(LibrarySwitch)d2d1 $(LibrarySwitch)dxguid $(LibrarySwitch)uuid 
ArLibs                 :=  "d3d11" "d2d1" "dxguid" "uuid" 
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch). $(LibraryPathSwitch)Debug 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := ar rcus
CXX      := g++
CC       := gcc
CXXFLAGS :=  -std=c++14 -g -Wall $(Preprocessors)
CFLAGS   :=   $(Preprocessors)
ASFLAGS  := 
AS       := as


##
## User defined environment variables
##
CodeLiteDir:=C:\Program Files\CodeLite
Objects0=$(IntermediateDirectory)/sfc_ppu.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_mapper.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_cpu.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_addr4020.c$(ObjectSuffix) $(IntermediateDirectory)/main.c$(ObjectSuffix) $(IntermediateDirectory)/up_common_d2d_draw.cpp$(ObjectSuffix) $(IntermediateDirectory)/sfc_famicom.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_6502.c$(ObjectSuffix) 



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
$(IntermediateDirectory)/sfc_ppu.c$(ObjectSuffix): sfc_ppu.c $(IntermediateDirectory)/sfc_ppu.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/step5/sfc_ppu.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_ppu.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_ppu.c$(DependSuffix): sfc_ppu.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_ppu.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_ppu.c$(DependSuffix) -MM sfc_ppu.c

$(IntermediateDirectory)/sfc_ppu.c$(PreprocessSuffix): sfc_ppu.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_ppu.c$(PreprocessSuffix) sfc_ppu.c

$(IntermediateDirectory)/sfc_mapper.c$(ObjectSuffix): sfc_mapper.c $(IntermediateDirectory)/sfc_mapper.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/step5/sfc_mapper.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_mapper.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_mapper.c$(DependSuffix): sfc_mapper.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_mapper.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_mapper.c$(DependSuffix) -MM sfc_mapper.c

$(IntermediateDirectory)/sfc_mapper.c$(PreprocessSuffix): sfc_mapper.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_mapper.c$(PreprocessSuffix) sfc_mapper.c

$(IntermediateDirectory)/sfc_cpu.c$(ObjectSuffix): sfc_cpu.c $(IntermediateDirectory)/sfc_cpu.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/step5/sfc_cpu.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_cpu.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_cpu.c$(DependSuffix): sfc_cpu.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_cpu.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_cpu.c$(DependSuffix) -MM sfc_cpu.c

$(IntermediateDirectory)/sfc_cpu.c$(PreprocessSuffix): sfc_cpu.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_cpu.c$(PreprocessSuffix) sfc_cpu.c

$(IntermediateDirectory)/sfc_addr4020.c$(ObjectSuffix): sfc_addr4020.c $(IntermediateDirectory)/sfc_addr4020.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/step5/sfc_addr4020.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_addr4020.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_addr4020.c$(DependSuffix): sfc_addr4020.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_addr4020.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_addr4020.c$(DependSuffix) -MM sfc_addr4020.c

$(IntermediateDirectory)/sfc_addr4020.c$(PreprocessSuffix): sfc_addr4020.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_addr4020.c$(PreprocessSuffix) sfc_addr4020.c

$(IntermediateDirectory)/main.c$(ObjectSuffix): main.c $(IntermediateDirectory)/main.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/step5/main.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main.c$(DependSuffix): main.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/main.c$(ObjectSuffix) -MF$(IntermediateDirectory)/main.c$(DependSuffix) -MM main.c

$(IntermediateDirectory)/main.c$(PreprocessSuffix): main.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main.c$(PreprocessSuffix) main.c

$(IntermediateDirectory)/up_common_d2d_draw.cpp$(ObjectSuffix): ../common/d2d_draw.cpp $(IntermediateDirectory)/up_common_d2d_draw.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "D:/github/StepFC/common/d2d_draw.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_common_d2d_draw.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_common_d2d_draw.cpp$(DependSuffix): ../common/d2d_draw.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_common_d2d_draw.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_common_d2d_draw.cpp$(DependSuffix) -MM ../common/d2d_draw.cpp

$(IntermediateDirectory)/up_common_d2d_draw.cpp$(PreprocessSuffix): ../common/d2d_draw.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_common_d2d_draw.cpp$(PreprocessSuffix) ../common/d2d_draw.cpp

$(IntermediateDirectory)/sfc_famicom.c$(ObjectSuffix): sfc_famicom.c $(IntermediateDirectory)/sfc_famicom.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/step5/sfc_famicom.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_famicom.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_famicom.c$(DependSuffix): sfc_famicom.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_famicom.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_famicom.c$(DependSuffix) -MM sfc_famicom.c

$(IntermediateDirectory)/sfc_famicom.c$(PreprocessSuffix): sfc_famicom.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_famicom.c$(PreprocessSuffix) sfc_famicom.c

$(IntermediateDirectory)/sfc_6502.c$(ObjectSuffix): sfc_6502.c $(IntermediateDirectory)/sfc_6502.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/step5/sfc_6502.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_6502.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_6502.c$(DependSuffix): sfc_6502.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_6502.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_6502.c$(DependSuffix) -MM sfc_6502.c

$(IntermediateDirectory)/sfc_6502.c$(PreprocessSuffix): sfc_6502.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_6502.c$(PreprocessSuffix) sfc_6502.c


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Debug/


