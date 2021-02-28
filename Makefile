CPP=g++
CPPFLAG=-w -lm -lpthread 

PF_FLIES       = pf_buffermgr.cc pf_error.cc pf_filehandle.cc \
                 pf_pagehandle.cc pf_hashtable.cc pf_statistics.cc \
                 statistics.cc
RM_FILES       = rm_filehandle.cc  bitmap.cc rm_record.cc
IX_FILES       = ix_indexhandle.cc btree_node.cc
SM_FILES       = sm_tablehandle.cc

ifeq ($(shell uname), Linux)

TARGET=./wsql
LIBDIR=./lib/
SRCDIR=./src/

PF_SOURCES     = $(addprefix $(SRCDIR), $(PF_FLIES))
RM_SOURCES     = $(addprefix $(SRCDIR), $(RM_FILES))
IX_SOURCES     = $(addprefix $(SRCDIR), $(IX_FILES))
SM_SOURCES	   = $(addprefix $(SRCDIR), $(SM_FILES))

PF_LIBSO       = $(LIBDIR)libWSQLpf.so
RM_LIBSO       = $(LIBDIR)libWSQLrm.so
IX_LIBSO       = $(LIBDIR)libWSQLix.so
SM_LIBSO       = $(LIBDIR)libWSQLsm.so

$(LIBDIR):
	mkdir $(LIBDIR)

$(PF_LIBSO): $(LIBDIR)
	$(CPP) -o $(PF_LIBSO) $(PF_SOURCES) -I ./include/ -fPIC -shared 

$(RM_LIBSO): $(PF_LIBSO)
	$(CPP) -o $(RM_LIBSO) $(RM_SOURCES) $(PF_LIBSO) -I ./include/ -fPIC -shared 

$(IX_LIBSO): $(PF_LIBSO)
	$(CPP) -o $(IX_LIBSO) $(IX_SOURCES) $(PF_LIBSO) -I ./include/ -fPIC -shared 

$(SM_LIBSO): $(RM_LIBSO) $(IX_LIBSO)
	$(CPP) -o $(SM_LIBSO) $(SM_SOURCES) $(RM_LIBSO) $(IX_LIBSO) -I ./include/ -fPIC -shared

$(TARGET): $(SM_LIBSO) $(IX_LIBSO) $(RM_LIBSO) 	
	$(CPP) -o $(TARGET) $(SRCDIR)wsql.cc $(SM_LIBSO) $(IX_LIBSO) $(RM_LIBSO) $(PF_LIBSO) -w -I ./include/ 

clean: 
	rm -f $(LIBDIR)libWSQL*.so
	rm -rf $(TARGET)
	
else 

TARGET=.\wsql.exe
SRCDIR=.\src
LIBDIR=.\lib

PF_SOURCES     = $(addprefix $(SRCDIR)\, $(PF_FLIES))
RM_SOURCES     = $(addprefix $(SRCDIR)\, $(RM_FILES))
IX_SOURCES     = $(addprefix $(SRCDIR)\, $(IX_FILES))
SM_SOURCES	   = $(addprefix $(SRCDIR)\, $(SM_FILES))

PF_LIBSO       = $(LIBDIR)\WSQLpf.dll
RM_LIBSO       = $(LIBDIR)\WSQLrm.dll
IX_LIBSO       = $(LIBDIR)\WSQLix.dll
SM_LIBSO       = $(LIBDIR)\WSQLsm.dll

$(LIBDIR):
	mkdir $(LIBDIR)

$(PF_LIBSO): $(LIBDIR)
	$(CPP) -o $(PF_LIBSO) $(PF_SOURCES) -I .\include\ -fPIC -shared 

$(RM_LIBSO): $(PF_LIBSO)
	$(CPP) -o $(RM_LIBSO) $(RM_SOURCES) $(PF_LIBSO) -I .\include\ -fPIC -shared 

$(IX_LIBSO): $(PF_LIBSO)
	$(CPP) -o $(IX_LIBSO) $(IX_SOURCES) $(PF_LIBSO) -I .\include\  -fPIC -shared 

$(SM_LIBSO): $(RM_LIBSO) $(IX_LIBSO)
	$(CPP) -o $(SM_LIBSO) $(SM_SOURCES) $(RM_LIBSO) $(IX_LIBSO) -I .\include\ -fPIC -shared

$(TARGET): $(SM_LIBSO) $(IX_LIBSO) $(RM_LIBSO) 
	$(CPP) -o $(TARGET) $(SRCDIR)\wsql.cc $(SM_LIBSO) -I .\include\ 

clean: 
	del $(LIBDIR)\WSQL*.dll	
	del $(TARGET)

endif


all: $(TARGET)
