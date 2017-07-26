<?php
if ($_SERVER['REQUEST_METHOD'] == 'POST') {
	session_start();

	$username = $_POST['username'];
	$password = $_POST['password'];

	$hostname = $_SERVER['HTTP_HOST'];
	$path = dirname($_SERVER['PHP_SELF']);

	// verify username and passwd

	try {
		$db = new SQLite3('/etc/vfrnav/autoroute.db', SQLITE3_OPEN_READWRITE | SQLITE3_OPEN_CREATE);
		$db->exec('CREATE TABLE IF NOT EXISTS credentials (username TEXT UNIQUE NOT NULL, passwdclear TEXT, passwdmd5 TEXT, salt INTEGER);');
		$stmt = $db->prepare('SELECT username,passwdmd5,passwdclear,salt FROM credentials WHERE username=:user;');
		$stmt->bindValue(':user', $username, SQLITE3_TEXT);
		$result = $stmt->execute();
		$res = $result->fetchArray(SQLITE3_ASSOC);
		if ($res) {
			if ($password == $res["passwdclear"]) {
				$_SESSION['authenticated'] = true;
			}
		}
		$result->finalize();
		$db->close();
	} catch (Exception $e) {
		echo "Database Error: " . $e->getMessage() . "\n";
	}

	//if ($username == 'sailer' && $password == 'test') {
	//	$_SESSION['authenticated'] = true;
	//}

	if (isset($_SESSION['authenticated']) && $_SESSION['authenticated']) {
		// forward to protected index page
		if ($_SERVER['SERVER_PROTOCOL'] == 'HTTP/1.1') {
			if (php_sapi_name() == 'cgi') {
				header('Status: 303 See Other');
			} else {
				header('HTTP/1.1 303 See Other');
			}
		}

		header('Location: http://' . $hostname . ($path == '/' ? '' : $path) . '/index.php');
		exit;
	}
}
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="de" lang="de">
  <head>
    <title>CFMU IFR Autorouter</title>
  </head>
  <body>
    <form action="login.php" method="post">
      Username: <input type="text" name="username" /><br/>
      Password: <input type="password" name="password" /><br/>
      <input type="submit" value="Login" />
    </form>
<?php
if ($_SERVER['REQUEST_METHOD'] == 'POST' && (!isset($_SESSION['authenticated']) || $_SESSION['authenticated'] !== true)) {
	echo "<hr/>Invalid Username/Password; try again<br/>";
}
?>
  </body>
</html>
