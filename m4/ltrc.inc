# Required to build Windows resource file

RCCOMPILE = $(RC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS)
  
LTRCCOMPILE = $(LIBTOOL) --mode=compile --tag=RC $(RCCOMPILE)

.rc.lo:
	$(AM_V_GEN)$(LTRCCOMPILE) -i "$<" -o "$@"

.rc.o:
	$(AM_V_GEN)$(RCCOMPILE) -i "$<" -o "$@"

