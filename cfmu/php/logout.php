<?php
session_start();

$tx = array ("session" => session_id(), "quit" => TRUE);

// Unset all of the session variables.
$_SESSION = array();
// If it's desired to kill the session, also delete the session cookie.
// Note: This will destroy the session, and not just the session data!
if (ini_get("session.use_cookies")) {
	$params = session_get_cookie_params();
	setcookie(session_name(), '', time() - 42000,
		  $params["path"], $params["domain"],
		  $params["secure"], $params["httponly"]);
}
session_destroy();

$hostname = $_SERVER['HTTP_HOST'];
$path = dirname($_SERVER['PHP_SELF']);

header('Location: http://' . $hostname . ($path == '/' ? '' : $path) . '/login.php');

$sock = socket_create(AF_UNIX, SOCK_SEQPACKET, 0);
if ($sock) {
	if (!socket_connect($sock, "/run/vfrnav/autoroute")) {
		socket_set_option($sock, SOL_SOCKET, SO_SNDTIMEO, array("sec" => 5, "usec" => 0));
		$txj = json_encode($tx);
		$r = socket_send($sock, $txj, strlen($txj), MSG_EOR);
	}
	socket_close($sock);
}

?>
