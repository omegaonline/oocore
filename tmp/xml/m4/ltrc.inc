# Required to build Windows resource file

# AM_SILENT_RULES handling

AM_V_RC = $(am__v_RC_$(V))
am__v_RC_ = $(am__v_RC_$(AM_DEFAULT_VERBOSITY))
am__v_RC_0 = @echo "  RC    " $@;

RCCOMPILE = $(RC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS)
  
LTRCCOMPILE = $(LIBTOOL) $(AM_V_lt) --tag=RC --mode=compile $(RCCOMPILE)

.rc.lo:
	$(AM_V_RC)$(LTRCCOMPILE) -i "$<" -o "$@"

.rc.o:
	$(AM_V_RC)$(RCCOMPILE) -i "$<" -o "$@"
