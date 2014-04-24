### About

Shell module for CosCMS for moving files between servers with ssh and rsync.
You can move from locale to remote or from remote to locale. 

Use: 

	./coscli.sh remote -h

For all sub commands

The idea is that on your `dev` server` you will be able to push complete site 
to the `stage or the `prod` server. Be carefull as the push command will delete 
files on prod site which is not in the development site. So when pushing
it will mirror files on dev server with the one on production or stage server. 

To prevent deletion, use: 

    --no-delete

By default `logs/*` and `files/*` will not be deleted as they are excluded.  
You can edit `push_exclude.ini` and add files which should be excluded.  

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



