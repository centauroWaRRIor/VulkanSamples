# Makefile generated by XPJ for linux-aarch64
-include Makefile.custom
ProjectName = NvImage
NvImage_cppfiles   += ./../../src/NvImage/BlockDXT.cpp
NvImage_cppfiles   += ./../../src/NvImage/ColorBlock.cpp
NvImage_cppfiles   += ./../../src/NvImage/NvFilePtr.cpp
NvImage_cppfiles   += ./../../src/NvImage/NvImage.cpp
NvImage_cppfiles   += ./../../src/NvImage/NvImageDDS.cpp

NvImage_cpp_debug_dep    = $(addprefix $(DEPSDIR)/NvImage/debug/, $(subst ./, , $(subst ../, , $(patsubst %.cpp, %.cpp.P, $(NvImage_cppfiles)))))
NvImage_cc_debug_dep    = $(addprefix $(DEPSDIR)/, $(subst ./, , $(subst ../, , $(patsubst %.cc, %.cc.debug.P, $(NvImage_ccfiles)))))
NvImage_c_debug_dep      = $(addprefix $(DEPSDIR)/NvImage/debug/, $(subst ./, , $(subst ../, , $(patsubst %.c, %.c.P, $(NvImage_cfiles)))))
NvImage_debug_dep      = $(NvImage_cpp_debug_dep) $(NvImage_cc_debug_dep) $(NvImage_c_debug_dep)
-include $(NvImage_debug_dep)
NvImage_cpp_release_dep    = $(addprefix $(DEPSDIR)/NvImage/release/, $(subst ./, , $(subst ../, , $(patsubst %.cpp, %.cpp.P, $(NvImage_cppfiles)))))
NvImage_cc_release_dep    = $(addprefix $(DEPSDIR)/, $(subst ./, , $(subst ../, , $(patsubst %.cc, %.cc.release.P, $(NvImage_ccfiles)))))
NvImage_c_release_dep      = $(addprefix $(DEPSDIR)/NvImage/release/, $(subst ./, , $(subst ../, , $(patsubst %.c, %.c.P, $(NvImage_cfiles)))))
NvImage_release_dep      = $(NvImage_cpp_release_dep) $(NvImage_cc_release_dep) $(NvImage_c_release_dep)
-include $(NvImage_release_dep)
NvImage_debug_hpaths    := 
NvImage_debug_hpaths    += ./../../src/NvImage
NvImage_debug_hpaths    += ./../../include
NvImage_debug_hpaths    += ./../../include/NsFoundation
NvImage_debug_hpaths    += ./../../include/NvFoundation
NvImage_debug_hpaths    += ./../../externals/include
NvImage_debug_hpaths    += ./../../externals/include/GLFW
NvImage_debug_lpaths    := 
NvImage_debug_defines   := $(NvImage_custom_defines)
NvImage_debug_defines   += LINUX=1
NvImage_debug_defines   += NV_LINUX
NvImage_debug_defines   += GLEW_NO_GLU=1
NvImage_debug_defines   += _DEBUG
NvImage_debug_libraries := 
NvImage_debug_common_cflags	:= $(NvImage_custom_cflags)
NvImage_debug_common_cflags    += -MMD
NvImage_debug_common_cflags    += $(addprefix -D, $(NvImage_debug_defines))
NvImage_debug_common_cflags    += $(addprefix -I, $(NvImage_debug_hpaths))
NvImage_debug_common_cflags  += -funwind-tables -Wall -Wextra -Wno-unused-parameter -Wno-ignored-qualifiers -Wno-unused-but-set-variable -Wno-switch -Wno-unused-variable -Wno-unused-function -pthread
NvImage_debug_common_cflags  += -funwind-tables -O0 -g -ggdb -fno-omit-frame-pointer
NvImage_debug_cflags	:= $(NvImage_debug_common_cflags)
NvImage_debug_cppflags	:= $(NvImage_debug_common_cflags)
NvImage_debug_cppflags  += -Wno-reorder -std=c++11
NvImage_debug_cppflags  += -Wno-reorder
NvImage_debug_lflags    := $(NvImage_custom_lflags)
NvImage_debug_lflags    += $(addprefix -L, $(NvImage_debug_lpaths))
NvImage_debug_lflags    += -Wl,--start-group $(addprefix -l, $(NvImage_debug_libraries)) -Wl,--end-group
NvImage_debug_lflags  += -Wl,--unresolved-symbols=ignore-in-shared-libs -pthread
NvImage_debug_objsdir  = $(OBJS_DIR)/NvImage_debug
NvImage_debug_cpp_o    = $(addprefix $(NvImage_debug_objsdir)/, $(subst ./, , $(subst ../, , $(patsubst %.cpp, %.cpp.o, $(NvImage_cppfiles)))))
NvImage_debug_cc_o    = $(addprefix $(NvImage_debug_objsdir)/, $(subst ./, , $(subst ../, , $(patsubst %.cc, %.cc.o, $(NvImage_ccfiles)))))
NvImage_debug_c_o      = $(addprefix $(NvImage_debug_objsdir)/, $(subst ./, , $(subst ../, , $(patsubst %.c, %.c.o, $(NvImage_cfiles)))))
NvImage_debug_obj      =  $(NvImage_debug_cpp_o) $(NvImage_debug_cc_o) $(NvImage_debug_c_o) 
NvImage_debug_bin      := ./../../lib/linux-aarch64/libNvImageD.a

clean_NvImage_debug: 
	@$(ECHO) clean NvImage debug
	@$(RMDIR) $(NvImage_debug_objsdir)
	@$(RMDIR) $(NvImage_debug_bin)
	@$(RMDIR) $(DEPSDIR)/NvImage/debug

build_NvImage_debug: postbuild_NvImage_debug
postbuild_NvImage_debug: mainbuild_NvImage_debug
mainbuild_NvImage_debug: prebuild_NvImage_debug $(NvImage_debug_bin)
prebuild_NvImage_debug:

$(NvImage_debug_bin): $(NvImage_debug_obj) 
	mkdir -p `dirname ./../../lib/linux-aarch64/libNvImageD.a`
	@$(AR) rcs $(NvImage_debug_bin) $(NvImage_debug_obj)
	$(ECHO) building $@ complete!

NvImage_debug_DEPDIR = $(dir $(@))/$(*F)
$(NvImage_debug_cpp_o): $(NvImage_debug_objsdir)/%.o:
	$(ECHO) NvImage: compiling debug $(filter %$(strip $(subst .cpp.o,.cpp, $(subst $(NvImage_debug_objsdir),, $@))), $(NvImage_cppfiles))...
	mkdir -p $(dir $(@))
	$(CXX) $(NvImage_debug_cppflags) -c $(filter %$(strip $(subst .cpp.o,.cpp, $(subst $(NvImage_debug_objsdir),, $@))), $(NvImage_cppfiles)) -o $@
	@mkdir -p $(dir $(addprefix $(DEPSDIR)/NvImage/debug/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .cpp.o,.cpp, $(subst $(NvImage_debug_objsdir),, $@))), $(NvImage_cppfiles))))))
	cp $(NvImage_debug_DEPDIR).d $(addprefix $(DEPSDIR)/NvImage/debug/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .cpp.o,.cpp, $(subst $(NvImage_debug_objsdir),, $@))), $(NvImage_cppfiles))))).P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(NvImage_debug_DEPDIR).d >> $(addprefix $(DEPSDIR)/NvImage/debug/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .cpp.o,.cpp, $(subst $(NvImage_debug_objsdir),, $@))), $(NvImage_cppfiles))))).P; \
	  rm -f $(NvImage_debug_DEPDIR).d

$(NvImage_debug_cc_o): $(NvImage_debug_objsdir)/%.o:
	$(ECHO) NvImage: compiling debug $(filter %$(strip $(subst .cc.o,.cc, $(subst $(NvImage_debug_objsdir),, $@))), $(NvImage_ccfiles))...
	mkdir -p $(dir $(@))
	$(CXX) $(NvImage_debug_cppflags) -c $(filter %$(strip $(subst .cc.o,.cc, $(subst $(NvImage_debug_objsdir),, $@))), $(NvImage_ccfiles)) -o $@
	mkdir -p $(dir $(addprefix $(DEPSDIR)/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .cc.o,.cc, $(subst $(NvImage_debug_objsdir),, $@))), $(NvImage_ccfiles))))))
	cp $(NvImage_debug_DEPDIR).d $(addprefix $(DEPSDIR)/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .cc.o,.cc, $(subst $(NvImage_debug_objsdir),, $@))), $(NvImage_ccfiles))))).debug.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(NvImage_debug_DEPDIR).d >> $(addprefix $(DEPSDIR)/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .cc.o,.cc, $(subst $(NvImage_debug_objsdir),, $@))), $(NvImage_ccfiles))))).debug.P; \
	  rm -f $(NvImage_debug_DEPDIR).d

$(NvImage_debug_c_o): $(NvImage_debug_objsdir)/%.o:
	$(ECHO) NvImage: compiling debug $(filter %$(strip $(subst .c.o,.c, $(subst $(NvImage_debug_objsdir),, $@))), $(NvImage_cfiles))...
	mkdir -p $(dir $(@))
	$(CC) $(NvImage_debug_cflags) -c $(filter %$(strip $(subst .c.o,.c, $(subst $(NvImage_debug_objsdir),, $@))), $(NvImage_cfiles)) -o $@ 
	@mkdir -p $(dir $(addprefix $(DEPSDIR)/NvImage/debug/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .c.o,.c, $(subst $(NvImage_debug_objsdir),, $@))), $(NvImage_cfiles))))))
	cp $(NvImage_debug_DEPDIR).d $(addprefix $(DEPSDIR)/NvImage/debug/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .c.o,.c, $(subst $(NvImage_debug_objsdir),, $@))), $(NvImage_cfiles))))).P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(NvImage_debug_DEPDIR).d >> $(addprefix $(DEPSDIR)/NvImage/debug/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .c.o,.c, $(subst $(NvImage_debug_objsdir),, $@))), $(NvImage_cfiles))))).P; \
	  rm -f $(NvImage_debug_DEPDIR).d

NvImage_release_hpaths    := 
NvImage_release_hpaths    += ./../../src/NvImage
NvImage_release_hpaths    += ./../../include
NvImage_release_hpaths    += ./../../include/NsFoundation
NvImage_release_hpaths    += ./../../include/NvFoundation
NvImage_release_hpaths    += ./../../externals/include
NvImage_release_hpaths    += ./../../externals/include/GLFW
NvImage_release_lpaths    := 
NvImage_release_defines   := $(NvImage_custom_defines)
NvImage_release_defines   += LINUX=1
NvImage_release_defines   += NV_LINUX
NvImage_release_defines   += GLEW_NO_GLU=1
NvImage_release_defines   += NDEBUG
NvImage_release_libraries := 
NvImage_release_common_cflags	:= $(NvImage_custom_cflags)
NvImage_release_common_cflags    += -MMD
NvImage_release_common_cflags    += $(addprefix -D, $(NvImage_release_defines))
NvImage_release_common_cflags    += $(addprefix -I, $(NvImage_release_hpaths))
NvImage_release_common_cflags  += -funwind-tables -Wall -Wextra -Wno-unused-parameter -Wno-ignored-qualifiers -Wno-unused-but-set-variable -Wno-switch -Wno-unused-variable -Wno-unused-function -pthread
NvImage_release_cflags	:= $(NvImage_release_common_cflags)
NvImage_release_cppflags	:= $(NvImage_release_common_cflags)
NvImage_release_cppflags  += -Wno-reorder -std=c++11
NvImage_release_cppflags  += -Wno-reorder
NvImage_release_lflags    := $(NvImage_custom_lflags)
NvImage_release_lflags    += $(addprefix -L, $(NvImage_release_lpaths))
NvImage_release_lflags    += -Wl,--start-group $(addprefix -l, $(NvImage_release_libraries)) -Wl,--end-group
NvImage_release_lflags  += -Wl,--unresolved-symbols=ignore-in-shared-libs -pthread
NvImage_release_objsdir  = $(OBJS_DIR)/NvImage_release
NvImage_release_cpp_o    = $(addprefix $(NvImage_release_objsdir)/, $(subst ./, , $(subst ../, , $(patsubst %.cpp, %.cpp.o, $(NvImage_cppfiles)))))
NvImage_release_cc_o    = $(addprefix $(NvImage_release_objsdir)/, $(subst ./, , $(subst ../, , $(patsubst %.cc, %.cc.o, $(NvImage_ccfiles)))))
NvImage_release_c_o      = $(addprefix $(NvImage_release_objsdir)/, $(subst ./, , $(subst ../, , $(patsubst %.c, %.c.o, $(NvImage_cfiles)))))
NvImage_release_obj      =  $(NvImage_release_cpp_o) $(NvImage_release_cc_o) $(NvImage_release_c_o) 
NvImage_release_bin      := ./../../lib/linux-aarch64/libNvImage.a

clean_NvImage_release: 
	@$(ECHO) clean NvImage release
	@$(RMDIR) $(NvImage_release_objsdir)
	@$(RMDIR) $(NvImage_release_bin)
	@$(RMDIR) $(DEPSDIR)/NvImage/release

build_NvImage_release: postbuild_NvImage_release
postbuild_NvImage_release: mainbuild_NvImage_release
mainbuild_NvImage_release: prebuild_NvImage_release $(NvImage_release_bin)
prebuild_NvImage_release:

$(NvImage_release_bin): $(NvImage_release_obj) 
	mkdir -p `dirname ./../../lib/linux-aarch64/libNvImage.a`
	@$(AR) rcs $(NvImage_release_bin) $(NvImage_release_obj)
	$(ECHO) building $@ complete!

NvImage_release_DEPDIR = $(dir $(@))/$(*F)
$(NvImage_release_cpp_o): $(NvImage_release_objsdir)/%.o:
	$(ECHO) NvImage: compiling release $(filter %$(strip $(subst .cpp.o,.cpp, $(subst $(NvImage_release_objsdir),, $@))), $(NvImage_cppfiles))...
	mkdir -p $(dir $(@))
	$(CXX) $(NvImage_release_cppflags) -c $(filter %$(strip $(subst .cpp.o,.cpp, $(subst $(NvImage_release_objsdir),, $@))), $(NvImage_cppfiles)) -o $@
	@mkdir -p $(dir $(addprefix $(DEPSDIR)/NvImage/release/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .cpp.o,.cpp, $(subst $(NvImage_release_objsdir),, $@))), $(NvImage_cppfiles))))))
	cp $(NvImage_release_DEPDIR).d $(addprefix $(DEPSDIR)/NvImage/release/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .cpp.o,.cpp, $(subst $(NvImage_release_objsdir),, $@))), $(NvImage_cppfiles))))).P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(NvImage_release_DEPDIR).d >> $(addprefix $(DEPSDIR)/NvImage/release/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .cpp.o,.cpp, $(subst $(NvImage_release_objsdir),, $@))), $(NvImage_cppfiles))))).P; \
	  rm -f $(NvImage_release_DEPDIR).d

$(NvImage_release_cc_o): $(NvImage_release_objsdir)/%.o:
	$(ECHO) NvImage: compiling release $(filter %$(strip $(subst .cc.o,.cc, $(subst $(NvImage_release_objsdir),, $@))), $(NvImage_ccfiles))...
	mkdir -p $(dir $(@))
	$(CXX) $(NvImage_release_cppflags) -c $(filter %$(strip $(subst .cc.o,.cc, $(subst $(NvImage_release_objsdir),, $@))), $(NvImage_ccfiles)) -o $@
	mkdir -p $(dir $(addprefix $(DEPSDIR)/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .cc.o,.cc, $(subst $(NvImage_release_objsdir),, $@))), $(NvImage_ccfiles))))))
	cp $(NvImage_release_DEPDIR).d $(addprefix $(DEPSDIR)/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .cc.o,.cc, $(subst $(NvImage_release_objsdir),, $@))), $(NvImage_ccfiles))))).release.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(NvImage_release_DEPDIR).d >> $(addprefix $(DEPSDIR)/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .cc.o,.cc, $(subst $(NvImage_release_objsdir),, $@))), $(NvImage_ccfiles))))).release.P; \
	  rm -f $(NvImage_release_DEPDIR).d

$(NvImage_release_c_o): $(NvImage_release_objsdir)/%.o:
	$(ECHO) NvImage: compiling release $(filter %$(strip $(subst .c.o,.c, $(subst $(NvImage_release_objsdir),, $@))), $(NvImage_cfiles))...
	mkdir -p $(dir $(@))
	$(CC) $(NvImage_release_cflags) -c $(filter %$(strip $(subst .c.o,.c, $(subst $(NvImage_release_objsdir),, $@))), $(NvImage_cfiles)) -o $@ 
	@mkdir -p $(dir $(addprefix $(DEPSDIR)/NvImage/release/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .c.o,.c, $(subst $(NvImage_release_objsdir),, $@))), $(NvImage_cfiles))))))
	cp $(NvImage_release_DEPDIR).d $(addprefix $(DEPSDIR)/NvImage/release/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .c.o,.c, $(subst $(NvImage_release_objsdir),, $@))), $(NvImage_cfiles))))).P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(NvImage_release_DEPDIR).d >> $(addprefix $(DEPSDIR)/NvImage/release/, $(subst ./, , $(subst ../, , $(filter %$(strip $(subst .c.o,.c, $(subst $(NvImage_release_objsdir),, $@))), $(NvImage_cfiles))))).P; \
	  rm -f $(NvImage_release_DEPDIR).d

clean_NvImage:  clean_NvImage_debug clean_NvImage_release
	rm -rf $(DEPSDIR)

export VERBOSE
ifndef VERBOSE
.SILENT:
endif
