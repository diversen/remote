### About

Shell module for CosCMS for moving files between servers with ssh and rsync.
You can move from locale to remote or from remote to locale. 

Use: 

	./coscli.sh remote -h

For all sub commands

### Exlucde files

copy rsyn_exclude.ini-dist to rsync_eclude.ini and add files: 
defaults to 

    logs/*
    htdocs/files/*

### Configuration

	; use port
	ssh_port = 22
	; use host or IP
	ssh_host = "lamp.test.org"
	; remote user
	ssh_user = 'dennis'
	; remote production path
	prod_path = "/home/remote/www/lamp.test.org"
	; locale path
	locale_path = "/home/dennis/www/coscms"



