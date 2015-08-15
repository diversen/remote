<?php

use diversen\conf;
use diversen\cli\common;

/**
 * rsync locale path with remote server
 */
function remote_move_file_to_remote($options){

    if (!isset($options['source'])){
        common::abort('You need to specify locale path to move');
    }
    
    $prod_path = conf::getModuleIni('prod_path');
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
    $prod_path = conf::getModuleIni('prod_path');
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
    common::execCommand($command);
    $prod_path = conf::getModuleIni('prod_path') . "/backup/sql/latest.sql";
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
    $ssh_user = remote_get_user();
    
    $command = 'ssh -p' . conf::getModuleIni('ssh_port') . ' -t ';
    $command.= $ssh_user . '@';
    $command.= config::getModuleIni('ssh_host') . ' ';
    $command.= '"';
    $command.= "cd " . conf::getModuleIni('prod_path') . " && ";
    $command.= $exec_command;
    $command.= '"';
    passthru($command);  
}

/**
 * fetch latest files from production and move them to locale
 * @param array $options
 */
function remote_fetch_files ($options) {
    $archive = "./backup/files/latest.tar.gz";
    $command = "sudo ./coscli.sh backup --public $archive ";
    
    common::echoMessage('Connecting to remote ssh server. Will create archive ...');
    remote_sudo_system($command);
    
    $ssh_user = remote_get_user();
    $command = "sudo chown $user:$user $archive";
    
    common::echoMessage('Connecting to remote ssh server. Will chown of archive ...');
    remote_sudo_system($command);
    
    $remote_path = conf::getModuleIni('prod_path');
    $remote_path.= "/backup/files/latest.tar.gz";
    $locale_path = "./backup/files/latest.tar.gz";
    
    common::echoMessage('Fetching remote archive with rsync  ...');
    remote_rsync_locale($remote_path, $locale_path);
    
    common::echoMessage('Extracting rsynced archive with tar  ...');
    backup_files_restore(array ('File' => $archive));
}

/** 
 * execute shell command on remote server. e.g.:
 * $command.= "./coscli.sh db --dump-db-file ./backup/sql/latest.sql;";
 * @param string $exec_command 
 */
function remote_system ($exec_command) {
    $ssh_user = remote_get_user();
    
    $command = 'ssh -p' . conf::getModuleIni('ssh_port') . ' ';
    $command.= $ssh_user . '@';
    $command.= conf::getModuleIni('ssh_host') . ' ';
    $command.= '"';
    $command.= "cd " . conf::getModuleIni('prod_path') . " && ";
    $command.= $exec_command;
    $command.= '"';
    $ret = common::systemCommand($command);
    return $ret;
}

/**
 * rsyncs locale path according to remote path  
 * @param type $remote_path e.g. 
 * @param type $locale_path 
 */
function remote_rsync_locale ($remote_path, $locale_path) {
    $ssh_user = remote_get_user();
    
    $command = "rsync -e 'ssh -p " . conf::getModuleIni('ssh_port') . "' -avl ";
    $command.= $ssh_user . "@";
    $command.= conf::getModuleIni('ssh_host') . ":";
    $command.= "$remote_path $locale_path";
    passthru($command);
}

function remote_get_user () {
    $ssh_user = conf::getModuleIni('ssh_user');
    if (empty($ssh_user)) {
        $ssh_user = getenv('USER');
    }
    return $ssh_user;
}

/**
 * rsyncs locale path or file to remote server
 * @param string $locale_path the path to rsync 
 */
function remote_rsync_remote ($locale_path, $prod_path) {
    $ssh_user = remote_get_user();

    $command = "rsync -e 'ssh -p " . conf::getModuleIni('ssh_port') . "' -avl ";
    
    if (!conf::getMainIni('remote_no_delete')) {
        $command.= "--delete "; 
    }

    
    // exclude logs and public files
    $push_exclude = conf::pathModules() . "/remote/push_exclude.ini";
    if (!file_exists($push_exclude)) {
        copy($push_exclude . "-dist", $push_exclude);
    }
    
    $command.= "--exclude-from '$push_exclude' ";    
    $command.= "$locale_path "; // move all except hidden files
    $command.=
        $ssh_user . "@" .
        conf::getModuleIni('ssh_host') . ":" .
        $prod_path;
    $ret = 1;
    passthru($command, $ret);
    return $ret;
}

/**
 * function for creating latest dump,
 *          moving remote dump from remote to locale via ssh server.
 *
 * @return int  $ret value from exec command
 */
function remote_load_locale(){

    // require confirm
    if (!common::readlineConfirm('Locale database will be destroyed ')){
        exit(1);
    }

    $command = "./coscli.sh db --dump-db-file ./backup/sql/latest.sql";    
    remote_system($command);

    $remote_path = conf::getModuleIni('prod_path') . "/backup/sql/latest.sql";
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

    // confirm
    if (!common::readlineConfirm('Locale source will be overwritten. Locale database overwritten with remote ')){
        exit(1);
    }

    $command = "./coscli.sh db --dump-db-file ./backup/sql/latest.sql;";
    remote_system($command);
    
    $remote_path = conf::getModuleIni('prod_path') . "/*";
    $locale_path = ".";
    remote_rsync_locale($remote_path, $locale_path);

    load_db_file(array('File' => './backup/sql/latest.sql'));
    cos_chmod_files();
}


/**
 * function for creating latest dump,
 *          moving remote dump from remote to locale via ssh server.
 *
 * @return int  $ret value from exec command
 */
 
function remote_connect(){
    echo conf::getModuleIni('prod_path');
}

function remote_no_delete ($options = array ()) {
    conf::setMainIni('remote_no_delete', 1);
}


mainCli::setCommand('remote', array(
    'description' => "Move files between servers using rsync and ssh. ",
));

mainCli::setOption('remote_no_delete', array(
    'long_name'   => '--no-delete',
    'description' => 'Will prevent deletion of files when pushing source',
    'action'      => 'StoreTrue'
));

mainCli::setOption('remote_move_to_remote', array(
    'long_name'   => '--push',
    'description' => 'Will push complete source tree to remote ssh server',
    'action'      => 'StoreTrue'
));

// test
/*
mainCli::setOption('remote_connect', array(
    'long_name'   => '--connect',
    'description' => 'Will connect to remote server through ssh',
    'action'      => 'StoreTrue'
)); */ 


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

