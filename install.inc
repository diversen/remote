<?php

// first we set a version for our module Note it has to be a float!
$_INSTALL['VERSION'] = 5.01;

// we specify that it is a shell module.
// this means that the source files will be loaded when the base shell
// command is getting loaded.
$_INSTALL['IS_SHELL'] = "1";

// we add a public clone url for easy distribution
$_INSTALL['PUBLIC_CLONE_URL'] = 'git://github.com/diversen/remote.git';

// and also a private url if we want to commit directly from base
// shell command
$_INSTALL['PRIVATE_CLONE_URL'] = 'git@github.com:diversen/remote.git';
