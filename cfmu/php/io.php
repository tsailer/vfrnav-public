<?php
session_start();

$hostname = $_SERVER['HTTP_HOST'];
$path = dirname($_SERVER['PHP_SELF']);

if (!isset($_SESSION['authenticated']) || !$_SESSION['authenticated']) {
	header('Location: http://' . $hostname . ($path == '/' ? '' : $path) . '/login.php');
	exit;
}

$tx = array ("session" => session_id());
if (isset($_GET["noreply"])) {
	$tx["noreply"] = TRUE;
}

if ($_SERVER['REQUEST_METHOD'] == 'POST' || $_SERVER['REQUEST_METHOD'] == 'PUT') {
	$bodyj = @file_get_contents('php://input');
	$body = json_decode($bodyj, true);
	if (is_object($body) || is_array($body)) {
		$tx["cmds"] = $body;
	}
}

$sock = socket_create(AF_UNIX, SOCK_SEQPACKET, 0);
if (!$sock) {
	$errno = socket_last_error($sock);
	$errstr = socket_strerror($errno);
	$rx = array ("error" => "socket create $errstr ($errno)");
	exit(json_encode($rx) . "\n");
}

if (!socket_connect($sock, "/run/vfrnav/autoroute/socket")) {
	$errno = socket_last_error($sock);
	$errstr = socket_strerror($errno);
	$rx = array ("error" => "socket connect $errstr ($errno)");
	exit(json_encode($rx) . "\n");
}
socket_set_option($sock, SOL_SOCKET, SO_SNDTIMEO, array("sec" => 5, "usec" => 0));
socket_set_option($sock, SOL_SOCKET, SO_RCVTIMEO, array("sec" => 15, "usec" => 0));

$txj = json_encode($tx);
$r = socket_send($sock, $txj, strlen($txj), MSG_EOR);
if ($r === false) {
		$errno = socket_last_error($sock);
		$errstr = socket_strerror($errno);
		$rx = array ("error" => "socket send $errstr ($errno)");
		exit(json_encode($rx) . "\n");
}

$r = socket_recv($sock, $rxrj, 65536, 0);
if ($r === false) {
	$errno = socket_last_error($sock);
	if ($errno != 11) {
		$errstr = socket_strerror($errno);
		$rx = array ("error" => "socket recv $errstr ($errno)");
	}
	exit(json_encode($rx) . "\n");
}
$rxr = json_decode($rxrj, true);
$rx = array ("error" => "cannot parse JSON");
if (is_object($rxr) || is_array($rxr)) {
	if (isset($rxr["cmds"]) && (is_object($rxr["cmds"]) || is_array($rxr["cmds"]))) {
		$rx = $rxr["cmds"];
	} else {
		$rxfld = array("version","provider","error");
		$rx = array();
		foreach ($rxfld as &$i) {
			if (isset($rxr[$i])) {
				$rx[$i] = $rxr[$i];
			}
		}
	}
}
socket_close($sock);

header('Content-Type: text/json');

exit(json_encode($rx) . "\n");

?>
