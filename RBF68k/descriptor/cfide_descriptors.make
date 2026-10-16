#############################################################################
# RBF descriptor modules
#############################################################################

# disable implicit rules as we call the assembler and linker directly
-bo

SDIR		= $(SRCROOT)/IO/RBF/DESC	# RBF descriptor sources
ODIR		= ../CMDS/BOOTOBJS
RDIR		= ./RELS
MAKER		= $(CFIDE)/cfide_descriptors.make	# this file
FLAGFILE	= $(ODIR)/.updated
SYSDEFS		= ../systype.d
RFLAGS		= -q -u=. -u=$(OSDEFS) -u=$(SDIR)
SLIB		= $(SYSRELS)/sys.l
LFLAGS		= -l=$(SLIB) -gu=0.0

DESC		= c0
DESCSRC		= $(CFIDE)/$(DESC).a
DESCMOD		= $(ODIR)/$(DESC)
DESCREL		= $(RDIR)/$(DESC).r
DESCMOD_FMT	= $(ODIR)/$(DESC)_fmt
DESCREL_FMT	= $(RDIR)/$(DESC)_fmt.r
DDMOD		= $(ODIR)/dd

# Additional partition descriptors
# SC145 RC2014 CF card reader
E0MOD		= $(ODIR)/e0
E0REL		= $(RDIR)/e0.r
F0MOD		= $(ODIR)/f0
F0REL		= $(RDIR)/f0.r

C1MOD		= $(ODIR)/c1
C1REL		= $(RDIR)/c1.r
C2MOD		= $(ODIR)/c2
C2REL		= $(RDIR)/c2.r
C3MOD		= $(ODIR)/c3
C3REL		= $(RDIR)/c3.r

build: $(RDIR) $(ODIR) $(DESCMOD) $(DESCMOD_FMT) $(DDMOD) $(E0MOD) $(F0MOD) $(C1MOD) $(C2MOD) $(C3MOD)

$(DESCMOD): $(DESCREL) $(SLIB)
	$(LC) $(LFLAGS) $(DESCREL) -O=$@
	$(TOUCH) $(FLAGFILE)

$(DESCREL): $(DESCSRC) $(SYSDEFS) $(MAKER)
	$(RC) $(RFLAGS) $(DESCSRC) -O=$@

$(DESCMOD_FMT): $(DESCREL_FMT) $(SLIB)
	$(LC) $(LFLAGS) $(DESCREL_FMT) -O=$@
	$(TOUCH) $(FLAGFILE)

$(DESCREL_FMT): $(DESCSRC) $(SYSDEFS) $(MAKER)
	$(RC) $(RFLAGS) -aFMT_ENABLE $(DESCSRC) -O=$@

$(DDMOD): $(DESCREL) $(SLIB)
	$(LC) $(LFLAGS) $(DESCREL) -O=$@ -n=dd
	$(TOUCH) $(FLAGFILE)

# Build rules for SC145 RC2014 CF card reader (e0=Master, f0=Slave)
$(E0MOD): $(E0REL) $(SLIB)
	$(LC) $(LFLAGS) $(E0REL) -O=$@
	$(TOUCH) $(FLAGFILE)

$(E0REL): $(CFIDE)/e0.a $(SYSDEFS) $(MAKER)
	$(RC) $(RFLAGS) $(CFIDE)/e0.a -O=$@

$(F0MOD): $(F0REL) $(SLIB)
	$(LC) $(LFLAGS) $(F0REL) -O=$@
	$(TOUCH) $(FLAGFILE)

$(F0REL): $(CFIDE)/f0.a $(SYSDEFS) $(MAKER)
	$(RC) $(RFLAGS) $(CFIDE)/f0.a -O=$@

# Build rules for partition descriptors c1, c2, c3
$(C1MOD): $(C1REL) $(SLIB)
	$(LC) $(LFLAGS) $(C1REL) -O=$@
	$(TOUCH) $(FLAGFILE)

$(C1REL): $(CFIDE)/c1.a $(SYSDEFS) $(MAKER)
	$(RC) $(RFLAGS) $(CFIDE)/c1.a -O=$@

$(C2MOD): $(C2REL) $(SLIB)
	$(LC) $(LFLAGS) $(C2REL) -O=$@
	$(TOUCH) $(FLAGFILE)

$(C2REL): $(CFIDE)/c2.a $(SYSDEFS) $(MAKER)
	$(RC) $(RFLAGS) $(CFIDE)/c2.a -O=$@

$(C3MOD): $(C3REL) $(SLIB)
	$(LC) $(LFLAGS) $(C3REL) -O=$@
	$(TOUCH) $(FLAGFILE)

$(C3REL): $(CFIDE)/c3.a $(SYSDEFS) $(MAKER)
	$(RC) $(RFLAGS) $(CFIDE)/c3.a -O=$@

$(ODIR) $(RDIR):
	@$(MD) $@

clean:
	$(RM) $(DESCMOD) $(DESCREL) $(DESCMOD_FMT) $(DESCREL_FMT) $(DDMOD)
	$(RM) $(E0MOD) $(E0REL) $(F0MOD) $(F0REL)
	$(RM) $(C1MOD) $(C1REL) $(C2MOD) $(C2REL) $(C3MOD) $(C3REL)
