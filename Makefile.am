## Makefile.am -- Process this file with automake to produce Makefile.in

SUBDIRS			= src doc

AUX_DIST		= $(ac_aux_dir)/config.guess \
				$(ac_aux_dir)/config.sub \
				$(ac_aux_dir)/depcomp \
				$(ac_aux_dir)/install-sh \
				$(ac_aux_dir)/ltconfig \
				$(ac_aux_dir)/ltmain.sh \
				$(ac_aux_dir)/missing
AUX_DIST_EXTRA		=
EXTRA_DIST		= bootstrap.sh \
				build-everything.sh \
				generate-docs.sh

MAINTAINERCLEANFILES	= Makefile.in acinclude.m4 aclocal.m4 \
				configure config-h.in \
				stamp-h.in $(AUX_DIST)

dist-hook:
	(cd $(distdir) && mkdir $(ac_aux_dir))
	for file in $(AUX_DIST) $(AUX_DIST_EXTRA); do \
		cp $$file $(distdir)/$$file; \
	done
