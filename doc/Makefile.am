noinst_DATA = flightdeck.pdf


XDVI		=xdvi
LATEX		=latex
PDFLATEX	=pdflatex
BIBTEX		=bibtex
DVIPS		=dvips
FIG2DEV		=fig2dev
PS2PDF		=ps2pdf
EPSTOPDF	=epstopdf
PDFTOPS		=pdftops
TIFF2PS		=tiff2ps
IMAGEMAGICK	=convert


HAMPROPEPS	= propafcpct.eps propafcpcterr.eps propcpstal.eps propcpstalerr.eps \
		  propctstal.eps propctstalerr.eps proppbl.eps \
		  propbldangcp1.eps propbldangct1.eps propbldangcp1d.eps propbldangct1d.eps \
		  propbldangcp2.eps propbldangct2.eps propbldangcp2d.eps propbldangct2d.eps \
		  propbldangcp3.eps propbldangct3.eps propbldangcp3d.eps propbldangct3d.eps \
		  propbldangcp4.eps propbldangct4.eps propbldangcp4d.eps propbldangct4d.eps

P28RPERFEPS	= p28ratmo.eps p28rio360pwr.eps p28rio360pwrdensity.eps p28rslgndrun.eps p28rslgndrunprop.eps \
		  p28rclimbforce.eps p28rclimbprop.eps p28rclimbforcegd.eps p28rclimbpropgd.eps \
		  p28rroc2700.eps p28rroceff2700.eps p28rrocpitch2700.eps \
		  p28rroc2500.eps p28rroceff2500.eps p28rrocpitch2500.eps \
		  p28rcruisespeed.eps p28rcruiseeff.eps p28rcruisepitch.eps \
		  p28rcruisespeedgd.eps p28rcruiseeffgd.eps p28rcruisepitchgd.eps \
		  p28rbestglidepp14.eps p28rbestglideproppp14.eps p28rgliderodpp14.eps p28rglidedistpp14.eps p28rgliderpmpp14.eps \
		  p28rbestglidepp29.eps p28rbestglideproppp29.eps p28rgliderodpp29.eps p28rglidedistpp29.eps p28rgliderpmpp29.eps

HF7666A2AF	= hartzellf7666a2af.eps

TEXSRC		= flightdeck.tex
FIGSRC		= wptseq.fig vertex.fig lininterp.fig contour110.fig contour111.fig contour1.fig contour2.fig contour3.fig \
		  barycentric1.fig barycentric2.fig
EPSSRC		= 
BUILTEPSSRC	= $(HAMPROPEPS) $(P28RPERFEPS) $(HF7666A2AF)
PSSRC		= 
JPGSRC		= 
TIFFSRC		= 
SVGSRC		= routenetwork.svg
DIASRC		= 
PDFSRC		= pbxif.pdfsrc
PNGSRC		= coordeditor.png fplbrowser.png fpleditor.png fpltextentry.png holddialog.png screenkbd.png \
		  splashdialog.png wptarpt1.png wptarpt2.png wptint.png wptmapel.png wptnavaid.png navmap.png \
		  navarpt.png altcfgdlg.png hsicfgdlg.png mapcfgdlg.png docdirpage.png docpage.png perfpage.png \
		  srsspage.png logpage.png

SRC		= $(TEXSRC) $(FIGSRC:.fig=.eps) $(EPSSRC) $(BUILTEPSSRC) $(PSSRC) $(JPGSRC:.jpg=.eps) \
		  $(TIFFSRC:.tiff=.eps) $(SVGSRC:.svg=.eps) $(DIASRC:.dia=.eps) $(PDFSRC:.pdfsrc=.eps) $(PNGSRC:.png=.eps)

SRCPDF		= $(TEXSRC) $(FIGSRC:.fig=.pdf) $(EPSSRC:.eps=.pdf) $(BUILTEPSSRC:.eps=.pdf) $(PSSRC:.ps=.pdf) $(JPGSRC) \
		  $(TIFFSRC:.tiff=.png) $(SVGSRC:.svg=.pdf) $(DIASRC:.dia=.pdf) $(PDFSRC:.pdfsrc=.pdf) $(PNGSRC)

# implicit rules

.tex.dvi:
		TEXINPUTS=$${TEXINPUTS}:$(srcdir) $(LATEX) -output-format=dvi $<
		$(BIBTEX) $* || echo "No BiBTeX"
		TEXINPUTS=$${TEXINPUTS}:$(srcdir) $(LATEX) -output-format=dvi $<

.tex.pdf:
		for i in $(PNGSRC) $(JPGSRC); do cp $(srcdir)/$$i .; done; true
		TEXINPUTS=$${TEXINPUTS}:$(srcdir) $(PDFLATEX) $<
		$(BIBTEX) $* || echo "No BiBTeX"
		TEXINPUTS=$${TEXINPUTS}:$(srcdir) $(PDFLATEX) $<

.dvi.ps:
		$(DVIPS) -f $< > $@

#.svg.eps:
#		inkscape --without-gui --export-area-drawing --file=$< --export-eps=$@
#
#.svg.pdf:
#		inkscape --without-gui --export-area-drawing --file=$< --export-pdf=$@

%.pdf %.eps:	%.svg
		inkscape --without-gui --export-area-drawing --file=$< --export-eps=$*.eps
		inkscape --without-gui --export-area-drawing --file=$< --export-pdf=$*.pdf

.fig.eps:
		$(FIG2DEV) -L eps -m 1 $< $@

.fig.svg:
		$(FIG2DEV) -L svg -m 1 $< $@

.fig.emf:
		$(FIG2DEV) -L emf -m 1 $< $@

.png.eps:
                $(IMAGEMAGICK) $< ps:$@

.tiff.eps:
		$(TIFF2PS) -e1 $< > $@

.tiff.png:
		$(IMAGEMAGICK) -verbose $< $@

.eps.pdf:
		$(EPSTOPDF) $<

.pdfsrc.eps:
		$(PDFTOPS) -eps $< $@

.pdfsrc.pdf:
		cp $< $@

.fig.latex:
		$(FIG2DEV) -L latex -m 0.5 $< $@

.fig.pstex:
		$(FIG2DEV) -L pstex -m 1 $< $@

%.pstex_t:	%.fig %.pstex
		$(FIG2DEV) -L pstex_t -m 1 -p $*.pstex $< $@

.dia.eps:
		dia -e $@ -t eps-builtin $<

.eps.ai:
		gs -q -dNODISPLAY /usr/share/ghostscript/8.15/lib/ps2ai.ps -c 0.25 dup scale -f $< > $@

# explicit rules
flightdeck.pdf:	$(SRCPDF)

flightdeck.dvi:	$(SRC)

EXTRA_DIST	= $(TEXSRC) $(FIGSRC) $(EPSSRC) $(PSSRC) $(JPGSRC) $(TIFFSRC) $(SVGSRC) $(DIASRC) \
		  $(PDFSRC) $(PNGSRC) hamprop.cc propeller.h hamproptbl.m p28rperf.m \
		  hartzellf7666a2af.m

CLEANFILES	= flightdeck.aux flightdeck.bbl \
		flightdeck.blg flightdeck.log flightdeck.out \
		flightdeck.pdf flightdeck.toc flightdeck.lof flightdeck.lot \
		$(FIGSRC:.fig=.eps) $(FIGSRC:.fig=.pdf) $(EPSSRC:.eps=.pdf) $(PSSRC:.ps=.pdf)  \
		$(JPGSRC:.jpg=.eps) $(TIFFSRC:.tiff=.eps) $(TIFFSRC:.tiff=.png) $(TEXSRC:.tex=.aux) \
		$(SVGSRC:.svg=.eps) $(SVGSRC:.svg=.pdf) $(BUILTEPSSRC) $(BUILTEPSSRC:.eps=.pdf) \
		$(PDFSRC:.pdfsrc=.eps) $(PDFSRC:.pdfsrc=.pdf) $(DVI) proptbl.cc p28rperf.done

$(HAMPROPEPS):	proptbl.cc
		@:

proptbl.cc:	hamproptbl.m
		octave -q $< > proptbl.cc

hamprop.oct:	hamprop.cc propeller.h proptbl.cc
		CC=@BUILD_CC@ CXX=@BUILD_CXX@ CPPFLAGS=-I/usr/include mkoctfile -o $@ -I$(srcdir) -I. $<

p28rperf.done:	p28rperf.m hamprop.oct
		octave -q $<
		touch p28rperf.done

$(P28RPERFEPS):	p28rperf.done
		@:

$(HF7666A2AF):	hartzellf7666a2af.m
		octave -q $<
