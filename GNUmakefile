# 8 november 2016

OUT = barspy.exe

CXXFILES = \
	checkmark.cpp \
	common.cpp \
	dllgetversion.cpp \
	dumphglobal.cpp \
	enum.cpp \
	flags.cpp \
	freelibrary.cpp \
	getwindowtheme.cpp \
	layout.cpp \
	loadlibrary.cpp \
	main.cpp \
	mainwin.cpp \
	panic.cpp \
	process.cpp \
	prochelper.cpp \
	tab.cpp \
	toolbargeneral.cpp \
	toolbartab.cpp \
	util.cpp \
	writeimglist5.cpp \
	writeimglist6.cpp

HFILES = \
	barspy.hpp \
	resources.h \
	winapi.hpp

RCFILES = \
	resources.rc

OBJDIR = .obj

OFILES = \
	$(CXXFILES:%.cpp=$(OBJDIR)/%.o) \
	$(RCFILES:%.rc=$(OBJDIR)/%.o)

CXXFLAGS = \
	-W4 \
	-wd4100 \
	-TP \
	-bigobj -nologo \
	-RTC1 -RTCs -RTCu \
	-EHsc \
	-Zi

LDFLAGS = \
	-largeaddressaware -nologo -incremental:no -debug \
	user32.lib kernel32.lib gdi32.lib comctl32.lib uxtheme.lib msimg32.lib comdlg32.lib ole32.lib oleaut32.lib oleacc.lib uuid.lib shlwapi.lib psapi.lib

$(OUT): $(OFILES)
	link -out:$(OUT) $(OFILES) $(LDFLAGS)

$(OBJDIR)/%.o: %.cpp $(HFILES) | $(OBJDIR)
	cl -Fo:$@ -c $< $(CXXFLAGS) -Fd$@.pdb

$(OBJDIR)/%.o: %.rc $(HFILES) | $(OBJDIR)
	rc -nologo -fo $@ $(RCFLAGS) $<

$(OBJDIR):
	mkdir $(OBJDIR)

clean:
	rm -rf $(OUT) $(OBJDIR) $(OUT:%.exe=%.pdb)
.PHONY: clean
