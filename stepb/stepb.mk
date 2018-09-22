##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=stepb
ConfigurationName      :=Debug
WorkspacePath          :=D:/github/StepFC
ProjectPath            :=D:/github/StepFC/stepb
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=dustpg
Date                   :=22/09/2018
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
ObjectsFileList        :="stepb.txt"
PCHCompileFlags        :=
MakeDirCommand         :=makedir
RcCmpOptions           := 
RcCompilerName         :=windres
LinkOptions            :=  -O0
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)d3d11 $(LibrarySwitch)d2d1 $(LibrarySwitch)dxguid $(LibrarySwitch)uuid $(LibrarySwitch)Ole32 
ArLibs                 :=  "d3d11" "d2d1" "dxguid" "uuid" "Ole32" 
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch). $(LibraryPathSwitch)Debug 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := ar rcus
CXX      := g++
CC       := gcc
CXXFLAGS :=  -std=c++14 -g -Wall $(Preprocessors)
CFLAGS   :=  -mssse3 $(Preprocessors)
ASFLAGS  := 
AS       := as


##
## User defined environment variables
##
CodeLiteDir:=C:\Program Files\CodeLite
Objects0=$(IntermediateDirectory)/up_common_d2d_draw1.cpp$(ObjectSuffix) $(IntermediateDirectory)/sfc_mapper_004_mmc3.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_mapper_003_cnrom.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_mapper_002_uxrom.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_mapper_001_mmc1.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_6502.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_state_sl.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_cpu.c$(ObjectSuffix) $(IntermediateDirectory)/up_common_xa2_play1.cpp$(ObjectSuffix) $(IntermediateDirectory)/sfc_mapper_074_mmc3_mo.c$(ObjectSuffix) \
	$(IntermediateDirectory)/sfc_famicom.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_mapper.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_addr4020.c$(ObjectSuffix) $(IntermediateDirectory)/main.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_crc32b.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_config.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_ppu.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_play_ez.c$(ObjectSuffix) $(IntermediateDirectory)/sfc_render_ez.c$(ObjectSuffix) 



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
$(IntermediateDirectory)/up_common_d2d_draw1.cpp$(ObjectSuffix): ../common/d2d_draw1.cpp $(IntermediateDirectory)/up_common_d2d_draw1.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "D:/github/StepFC/common/d2d_draw1.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_common_d2d_draw1.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_common_d2d_draw1.cpp$(DependSuffix): ../common/d2d_draw1.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_common_d2d_draw1.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_common_d2d_draw1.cpp$(DependSuffix) -MM ../common/d2d_draw1.cpp

$(IntermediateDirectory)/up_common_d2d_draw1.cpp$(PreprocessSuffix): ../common/d2d_draw1.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_common_d2d_draw1.cpp$(PreprocessSuffix) ../common/d2d_draw1.cpp

$(IntermediateDirectory)/sfc_mapper_004_mmc3.c$(ObjectSuffix): sfc_mapper_004_mmc3.c $(IntermediateDirectory)/sfc_mapper_004_mmc3.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/sfc_mapper_004_mmc3.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_mapper_004_mmc3.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_mapper_004_mmc3.c$(DependSuffix): sfc_mapper_004_mmc3.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_mapper_004_mmc3.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_mapper_004_mmc3.c$(DependSuffix) -MM sfc_mapper_004_mmc3.c

$(IntermediateDirectory)/sfc_mapper_004_mmc3.c$(PreprocessSuffix): sfc_mapper_004_mmc3.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_mapper_004_mmc3.c$(PreprocessSuffix) sfc_mapper_004_mmc3.c

$(IntermediateDirectory)/sfc_mapper_003_cnrom.c$(ObjectSuffix): sfc_mapper_003_cnrom.c $(IntermediateDirectory)/sfc_mapper_003_cnrom.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/sfc_mapper_003_cnrom.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_mapper_003_cnrom.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_mapper_003_cnrom.c$(DependSuffix): sfc_mapper_003_cnrom.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_mapper_003_cnrom.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_mapper_003_cnrom.c$(DependSuffix) -MM sfc_mapper_003_cnrom.c

$(IntermediateDirectory)/sfc_mapper_003_cnrom.c$(PreprocessSuffix): sfc_mapper_003_cnrom.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_mapper_003_cnrom.c$(PreprocessSuffix) sfc_mapper_003_cnrom.c

$(IntermediateDirectory)/sfc_mapper_002_uxrom.c$(ObjectSuffix): sfc_mapper_002_uxrom.c $(IntermediateDirectory)/sfc_mapper_002_uxrom.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/sfc_mapper_002_uxrom.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_mapper_002_uxrom.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_mapper_002_uxrom.c$(DependSuffix): sfc_mapper_002_uxrom.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_mapper_002_uxrom.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_mapper_002_uxrom.c$(DependSuffix) -MM sfc_mapper_002_uxrom.c

$(IntermediateDirectory)/sfc_mapper_002_uxrom.c$(PreprocessSuffix): sfc_mapper_002_uxrom.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_mapper_002_uxrom.c$(PreprocessSuffix) sfc_mapper_002_uxrom.c

$(IntermediateDirectory)/sfc_mapper_001_mmc1.c$(ObjectSuffix): sfc_mapper_001_mmc1.c $(IntermediateDirectory)/sfc_mapper_001_mmc1.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/sfc_mapper_001_mmc1.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_mapper_001_mmc1.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_mapper_001_mmc1.c$(DependSuffix): sfc_mapper_001_mmc1.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_mapper_001_mmc1.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_mapper_001_mmc1.c$(DependSuffix) -MM sfc_mapper_001_mmc1.c

$(IntermediateDirectory)/sfc_mapper_001_mmc1.c$(PreprocessSuffix): sfc_mapper_001_mmc1.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_mapper_001_mmc1.c$(PreprocessSuffix) sfc_mapper_001_mmc1.c

$(IntermediateDirectory)/sfc_6502.c$(ObjectSuffix): sfc_6502.c $(IntermediateDirectory)/sfc_6502.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/sfc_6502.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_6502.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_6502.c$(DependSuffix): sfc_6502.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_6502.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_6502.c$(DependSuffix) -MM sfc_6502.c

$(IntermediateDirectory)/sfc_6502.c$(PreprocessSuffix): sfc_6502.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_6502.c$(PreprocessSuffix) sfc_6502.c

$(IntermediateDirectory)/sfc_state_sl.c$(ObjectSuffix): sfc_state_sl.c $(IntermediateDirectory)/sfc_state_sl.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/sfc_state_sl.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_state_sl.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_state_sl.c$(DependSuffix): sfc_state_sl.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_state_sl.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_state_sl.c$(DependSuffix) -MM sfc_state_sl.c

$(IntermediateDirectory)/sfc_state_sl.c$(PreprocessSuffix): sfc_state_sl.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_state_sl.c$(PreprocessSuffix) sfc_state_sl.c

$(IntermediateDirectory)/sfc_cpu.c$(ObjectSuffix): sfc_cpu.c $(IntermediateDirectory)/sfc_cpu.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/sfc_cpu.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_cpu.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_cpu.c$(DependSuffix): sfc_cpu.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_cpu.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_cpu.c$(DependSuffix) -MM sfc_cpu.c

$(IntermediateDirectory)/sfc_cpu.c$(PreprocessSuffix): sfc_cpu.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_cpu.c$(PreprocessSuffix) sfc_cpu.c

$(IntermediateDirectory)/up_common_xa2_play1.cpp$(ObjectSuffix): ../common/xa2_play1.cpp $(IntermediateDirectory)/up_common_xa2_play1.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "D:/github/StepFC/common/xa2_play1.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/up_common_xa2_play1.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/up_common_xa2_play1.cpp$(DependSuffix): ../common/xa2_play1.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/up_common_xa2_play1.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/up_common_xa2_play1.cpp$(DependSuffix) -MM ../common/xa2_play1.cpp

$(IntermediateDirectory)/up_common_xa2_play1.cpp$(PreprocessSuffix): ../common/xa2_play1.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/up_common_xa2_play1.cpp$(PreprocessSuffix) ../common/xa2_play1.cpp

$(IntermediateDirectory)/sfc_mapper_074_mmc3_mo.c$(ObjectSuffix): sfc_mapper_074_mmc3_mo.c $(IntermediateDirectory)/sfc_mapper_074_mmc3_mo.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/sfc_mapper_074_mmc3_mo.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_mapper_074_mmc3_mo.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_mapper_074_mmc3_mo.c$(DependSuffix): sfc_mapper_074_mmc3_mo.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_mapper_074_mmc3_mo.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_mapper_074_mmc3_mo.c$(DependSuffix) -MM sfc_mapper_074_mmc3_mo.c

$(IntermediateDirectory)/sfc_mapper_074_mmc3_mo.c$(PreprocessSuffix): sfc_mapper_074_mmc3_mo.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_mapper_074_mmc3_mo.c$(PreprocessSuffix) sfc_mapper_074_mmc3_mo.c

$(IntermediateDirectory)/sfc_famicom.c$(ObjectSuffix): sfc_famicom.c $(IntermediateDirectory)/sfc_famicom.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/sfc_famicom.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_famicom.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_famicom.c$(DependSuffix): sfc_famicom.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_famicom.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_famicom.c$(DependSuffix) -MM sfc_famicom.c

$(IntermediateDirectory)/sfc_famicom.c$(PreprocessSuffix): sfc_famicom.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_famicom.c$(PreprocessSuffix) sfc_famicom.c

$(IntermediateDirectory)/sfc_mapper.c$(ObjectSuffix): sfc_mapper.c $(IntermediateDirectory)/sfc_mapper.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/sfc_mapper.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_mapper.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_mapper.c$(DependSuffix): sfc_mapper.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_mapper.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_mapper.c$(DependSuffix) -MM sfc_mapper.c

$(IntermediateDirectory)/sfc_mapper.c$(PreprocessSuffix): sfc_mapper.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_mapper.c$(PreprocessSuffix) sfc_mapper.c

$(IntermediateDirectory)/sfc_addr4020.c$(ObjectSuffix): sfc_addr4020.c $(IntermediateDirectory)/sfc_addr4020.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/sfc_addr4020.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_addr4020.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_addr4020.c$(DependSuffix): sfc_addr4020.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_addr4020.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_addr4020.c$(DependSuffix) -MM sfc_addr4020.c

$(IntermediateDirectory)/sfc_addr4020.c$(PreprocessSuffix): sfc_addr4020.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_addr4020.c$(PreprocessSuffix) sfc_addr4020.c

$(IntermediateDirectory)/main.c$(ObjectSuffix): main.c $(IntermediateDirectory)/main.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/main.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main.c$(DependSuffix): main.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/main.c$(ObjectSuffix) -MF$(IntermediateDirectory)/main.c$(DependSuffix) -MM main.c

$(IntermediateDirectory)/main.c$(PreprocessSuffix): main.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main.c$(PreprocessSuffix) main.c

$(IntermediateDirectory)/sfc_crc32b.c$(ObjectSuffix): sfc_crc32b.c $(IntermediateDirectory)/sfc_crc32b.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/sfc_crc32b.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_crc32b.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_crc32b.c$(DependSuffix): sfc_crc32b.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_crc32b.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_crc32b.c$(DependSuffix) -MM sfc_crc32b.c

$(IntermediateDirectory)/sfc_crc32b.c$(PreprocessSuffix): sfc_crc32b.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_crc32b.c$(PreprocessSuffix) sfc_crc32b.c

$(IntermediateDirectory)/sfc_config.c$(ObjectSuffix): sfc_config.c $(IntermediateDirectory)/sfc_config.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/sfc_config.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_config.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_config.c$(DependSuffix): sfc_config.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_config.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_config.c$(DependSuffix) -MM sfc_config.c

$(IntermediateDirectory)/sfc_config.c$(PreprocessSuffix): sfc_config.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_config.c$(PreprocessSuffix) sfc_config.c

$(IntermediateDirectory)/sfc_ppu.c$(ObjectSuffix): sfc_ppu.c $(IntermediateDirectory)/sfc_ppu.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/sfc_ppu.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_ppu.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_ppu.c$(DependSuffix): sfc_ppu.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_ppu.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_ppu.c$(DependSuffix) -MM sfc_ppu.c

$(IntermediateDirectory)/sfc_ppu.c$(PreprocessSuffix): sfc_ppu.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_ppu.c$(PreprocessSuffix) sfc_ppu.c

$(IntermediateDirectory)/sfc_play_ez.c$(ObjectSuffix): sfc_play_ez.c $(IntermediateDirectory)/sfc_play_ez.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/sfc_play_ez.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_play_ez.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_play_ez.c$(DependSuffix): sfc_play_ez.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_play_ez.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_play_ez.c$(DependSuffix) -MM sfc_play_ez.c

$(IntermediateDirectory)/sfc_play_ez.c$(PreprocessSuffix): sfc_play_ez.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_play_ez.c$(PreprocessSuffix) sfc_play_ez.c

$(IntermediateDirectory)/sfc_render_ez.c$(ObjectSuffix): sfc_render_ez.c $(IntermediateDirectory)/sfc_render_ez.c$(DependSuffix)
	$(CC) $(SourceSwitch) "D:/github/StepFC/stepb/sfc_render_ez.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sfc_render_ez.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sfc_render_ez.c$(DependSuffix): sfc_render_ez.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sfc_render_ez.c$(ObjectSuffix) -MF$(IntermediateDirectory)/sfc_render_ez.c$(DependSuffix) -MM sfc_render_ez.c

$(IntermediateDirectory)/sfc_render_ez.c$(PreprocessSuffix): sfc_render_ez.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sfc_render_ez.c$(PreprocessSuffix) sfc_render_ez.c


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Debug/


