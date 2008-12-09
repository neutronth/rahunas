# Rahunas configuration and database file

install-data-local: install-weblogin

install-weblogin:
	cp -R * $(DESTDIR)$(datadir)/rahunas
	rm -f $(DESTDIR)$(datadir)/rahunas/Makefile*
	chmod 711 $(DESTDIR)$(datadir)/rahunas
	find $(DESTDIR)$(datadir)/rahunas -type f -exec chmod 755 {} \;
	find $(DESTDIR)$(datadir)/rahunas -type d -exec chmod 711 {} \;

uninstall-local:
	rm -rf $(DESTDIR)$(datadir)/rahunas
