CXXFLAGS	= -march=native -O3
CXXFLAGS	+= -std=c++11
CXXFLAGS	+= -isystem ./include
CXXFLAGS	+= -MMD
CC	= g++

TARGET	=  CalcScore_FS CalcScore_ML CalcScore_GLH_FS CalcRank_FS CalcRank_GLH_FS CalcIDError_from_Rank_FS RR_FS RR_ML GLH_FS NS_FS NSEstMI_FS EvalKL EvalIDError CalcFARFRR EvalRRDistError_FS EvalGLHDistError_FS

all: $(TARGET)

CalcScore_FS: CalcScore_FS.o

CalcScore_ML: CalcScore_ML.o

CalcScore_GLH_FS: CalcScore_GLH_FS.o

CalcRank_FS: CalcRank_FS.o

CalcRank_GLH_FS: CalcRank_GLH_FS.o

CalcIDError_from_Rank_FS: CalcIDError_from_Rank_FS.o

RR_FS: RR_FS.o

RR_ML: RR_ML.o

GLH_FS: GLH_FS.o

NS_FS: NS_FS.o

NSEstMI_FS: NSEstMI_FS.o

EvalKL: EvalKL.o

EvalIDError: EvalIDError.o

CalcFARFRR: CalcFARFRR.o

EvalRRDistError_FS: EvalRRDistError_FS.o

EvalGLHDistError_FS: EvalGLHDistError_FS.o

clean:
	rm -f *~ $(TARGET) *.d *.o

-include *.d
