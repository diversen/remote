<?php

/**
 * rsync locale path with remote server
 */
function remote_move_file_to_remote($options){

    if (!isset($options['source'])){
        cos_cli_abort('You need to specify locale path to move');
    }
    
    $prod_path = mainCli::$ini['remote']['prod_path'];
    if (isset($options['dest'])) {
        $prod_path.= "/$options[dest]";
    }
    //echo $prod_path; die;
    remote_rsync_remote($options['source'], $prod_path);
}

/**
 * function for moving all local files to remote via ssh server
 *
 * @return int  value from exec command
 */
function remote_move_to_remote(){
    $prod_path = mainCli::$ini['remote']['prod_path'];
    $locale_path = ".";
    remote_rsync_remote($locale_path, $prod_path);
}

/**
 * function for moving local files to remote via ssh server
 *
 * @return int  value from exec command
 */
function remote_move_to_remote_latest_dump(){
    $command = "./coscli.sh db --dump-db-file ./backup/sql/latest.sql"; 
    cos_exec($command);
    $prod_path = mainCli::$ini['remote']['prod_path'] . "/backup/sql/latest.sql";
    $locale_path = "./backup/sql/latest.sql";
    remote_rsync_remote($locale_path, $prod_path);
}

/**
 * function for creating latest dump,
 *          moving local files to remote via ssh server
 *          loading latest dump on production server.
 *
 * @return int  value from exec command
 */
function remote_load_remote(){
    dump_db_file();
    remote_move_to_remote_latest_dump();
    $command = "./coscli.sh db --load-db-file ./backup/sql/latest.sql";
    $ret = remote_system($command);
    return $ret;
}

/**
 * function for removing files on remote server
 * practical before using rsync for moving files.
 *
 * @return int  $ret value from exec command
 */
function remote_rm_files_remote(){
    $command.= "sudo ./coscli.sh file --rm";
    return remote_system($command);
}

/**
 * method for executing (passthru) a command as super user (sudo)
 * @param string $exec_command 
 */
function remote_sudo_system ($exec_command) {
    if (empty(mainCli::$ini['remote']['ssh_user'])){
        mainCli::$ini['remote']['ssh_user'] = getenv('USER');
    }
    
    $command = 'ssh -p' . mainCli::$ini['remote']['ssh_port'] . ' -t ';
    $command.= mainCli::$ini['remote']['ssh_user'] . '@';
    $command.= mainCli::$ini['remote']['ssh_host'] . ' ';
    $command.= '"';
    $command.= "cd " . mainCli::$ini['remote']['prod_path'] . " && ";
    $command.= $exec_command;
    $command.= '"';
    passthru($command);  
}

function remote_fetch_files ($options) {
    $archive = "./backup/files/latest.tar.gz";
    $command = "./coscli.sh backup --files-backup $archive ";
    
    cos_cli_print('Connecting to remote ssh server. Will create archive ...');
    remote_sudo_system($command);
    
    $user = mainCli::$ini['remote']['ssh_user'];
    $command = "sudo chown $user:$user $archive";
    
    cos_cli_print('Connecting to remote ssh server. Will chown of archive ...');
    remote_sudo_system($command);
    
    $remote_path = mainCli::$ini['remote']['prod_path'];
    $remote_path.= "/backup/files/latest.tar.gz";
    $locale_path = "./backup/files/latest.tar.gz";
    
    cos_cli_print('Fetching remote archive with rsync  ...');
    remote_rsync_locale($remote_path, $locale_path);
    
    cos_cli_print('Extracting rsynced archive with tar  ...');
    backup_files_restore(array ('File' => $archive));
}

/** 
 * execute shell command on remote server. e.g.:
 * $command.= "./coscli.sh db --dump-db-file ./backup/sql/latest.sql;";
 * @param string $exec_command 
 */
function remote_system ($exec_command) {
    if (empty(mainCli::$ini['remote']['ssh_user'])){
        mainCli::$ini['remote']['ssh_user'] = getenv('USER');
    }
    
    $command = 'ssh -p' . mainCli::$ini['remote']['ssh_port'] . ' ';
    $command.= mainCli::$ini['remote']['ssh_user'] . '@';
    $command.= mainCli::$ini['remote']['ssh_host'] . ' ';
    $command.= '"';
    $command.= "cd " . mainCli::$ini['remote']['prod_path'] . " && ";
    $command.= $exec_command;
    $command.= '"';
    $ret = cos_system($command);
    return $ret;
}

/**
 * rsyncs locale path according to remote path  
 * @param type $remote_path e.g. 
 * @param type $locale_path 
 */
function remote_rsync_locale ($remote_path, $locale_path) {
    if (empty(mainCli::$ini['remote']['ssh_user'])){
        mainCli::$ini['remote']['ssh_user'] = getenv('USER');
    }
    
    $command = "rsync -e 'ssh -p " . mainCli::$ini['remote']['ssh_port'] . "' -avl ";
    $command.= mainCli::$ini['remote']['ssh_user'] . "@";
    $command.= mainCli::$ini['remote']['ssh_host'] . ":";
    $command.= "$remote_path $locale_path";
    passthru($command);
}

/**
 * rsyncs locale path or file to remote server
 * @param string $locale_path the path to rsync 
 */
function remote_rsync_remote ($locale_path, $prod_path) {
      
    if (empty(mainCli::$ini['remote']['ssh_user'])){
        mainCli::$ini['remote']['ssh_user'] = getenv('USER');
    }

    //echo $locale_path;
    //echo $prod_path;
    //die;
    $command = "rsync -e 'ssh -p " . mainCli::$ini['remote']['ssh_port'] . "' -avl ";
    $command.= "$locale_path "; // move all except hidden files
    $command.=
        mainCli::$ini['remote']['ssh_user'] . "@" .
        mainCli::$ini['remote']['ssh_host'] . ":" .
        $prod_path;
    passthru($command, $ret);
}

/**
 * function for creating latest dump,
 *          moving remote dump from remote to locale via ssh server.
 *
 * @return int  $ret value from exec command
 */
function remote_load_locale(){

    if (empty(mainCli::$ini['remote']['ssh_user'])){
        mainCli::$ini['remote']['ssh_user'] = getenv('USER');
    }

    // require confirm
    if (!cos_confirm_readline('Locale database will be destroyed ')){
        exit(1);
    }

    $command = "./coscli.sh db --dump-db-file ./backup/sql/latest.sql";    
    remote_system($command);

    $remote_path = mainCli::$ini['remote']['prod_path'] . "/backup/sql/latest.sql";
    $locale_path = "./backup/sql/latest.sql";
    
    remote_rsync_locale($remote_path, $locale_path);
    load_db_file(array('File' => './backup/sql/latest.sql'));
}

/**
 * 
 * moving remote files to locale via ssh and rsync
 * loading latest sql on locale
 *
 * @return int  value from exec command
 */
function remote_move_to_locale(){
    if (empty(mainCli::$ini['remote']['ssh_user'])){
        mainCli::$ini['remote']['ssh_user'] = getenv('USER');
    }

    // confirm
    if (!cos_confirm_readline('Locale source will be overwritten. Locale database overwritten with remote ')){
        exit(1);
    }

    $command = "./coscli.sh db --dump-db-file ./backup/sql/latest.sql;";
    remote_system($command);
    chmod_files_to_owner();
    
    $remote_path = mainCli::$ini['remote']['prod_path'] . "/*";
    $locale_path = ".";
    remote_rsync_locale($remote_path, $locale_path);

    load_db_file(array('File' => './backup/sql/latest.sql'));
    chmod_files();
}

mainCli::setCommand('remote', array(
    'description' => "Commands for moving files between servers using rsync and ssh. ",
));

mainCli::setOption('remote_move_to_remote', array(
    'long_name'   => '--push',
    'description' => 'Will push complete source tree to remote ssh server',
    'action'      => 'StoreTrue'
));

mainCli::setOption('remote_move_file_to_remote', array(
    'long_name'   => '--single-push',
    'description' => 'Will move single specified file to remote ssh server',
    'action'      => 'StoreTrue'
));

mainCli::setOption('remote_load_remote', array(
    'long_name'   => '--load-remote',
    'description' => 'Will load latest dump onto production server',
    'action'      => 'StoreTrue'
));

mainCli::setOption('remote_move_to_locale', array(
    'long_name'   => '--pull',
    'description' => 'Will pull complete source tree and files to locale server',
    'action'      => 'StoreTrue'
));

mainCli::setOption('remote_load_locale', array(
    'long_name'   => '--load-locale',
    'description' => 'Will get and load latest dump from production to locale server',
    'action'      => 'StoreTrue'
));

mainCli::setOption('remote_fetch_files', array(
    'long_name'   => '--fetch-files',
    'description' => 'Will fetch remote htdocs/files and extract to locale',
    'action'      => 'StoreTrue'
));

mainCli::setArgument(
    'source',
    array('description'=> 'Specify optional source path',
        'optional' => true,
));

mainCli::setArgument(
    'dest',
    array('description'=> 'Specify optional destination path',
        'optional' => true,
));