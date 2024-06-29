<?php
// version 1.1
//

$player="";
$pass = "";
$cmd = "";
$portal="";
$lasthist="";

function trace($msg)
{
//	echo "<!-- TRACE $msg -->";
	$log = fopen("/data/netdbm/logs/prism/php.log", "a");
	fwrite($log, "PHP: $msg\n" );
	fclose($log);
}

function badname($name)
{
  if( strpos( $name, ";") !== false){
    return true;
  }
  if( strpos( $name, '"') !== false){
    return true;
  }
  if( strpos( $name, "'") !== false){
    return true;
  }
  if( strpos( $name, "&") !== false){
    return true;
  }
  if( strpos( $name, "|") !== false){
    return true;
  }
  if( strpos( $name, "`") !== false){
    return true;
  }
  return false;
}

if( isset( $_GET["player"] ) ){
        $player=$_GET["player"];
        if( badname($player) ){
          trace("Bad player name");
          die("");
        }
}
if( isset( $_GET["password"] ) ){
        $pass=$_GET["password"];
        if( badname($pass) ){
          trace("Bad player password");
          die("");
        }
}
if( isset( $_GET["portal"] ) ){
        $portal=$_GET["portal"];
}
if( isset( $_GET["cmd"] ) ){
        $cmd=$_GET["cmd"];
        if( $cmd != ""){
          // trace("CMD=".$cmd);
          $cmd = urlencode( $cmd);
          // trace("URL=".$cmd);
        }
}

if( isset( $_GET["lasthist"] ) ){
        $lasthist=$_GET["lasthist"];
}

$action="";
if( isset( $_GET["action"] ) ){
	$action = $_GET["action"];
}
if( $action != ""){
	trace("Action: $action");
	}
if( $action == "init"){
		echo "<head></head>\n";
		echo "<body onload='doload();' >\n";

		echo "<div id='gamediv'></div>\n";
		echo "<div id='logindiv' style='display:none;'></div>\n";
		echo "<div id='debugdiv'></div>\n";
		echo "<style type='text/css'>\n";
		echo ".boxred { color:red; border-style: solid; border-width: 2px; }\n";
		echo ".exit { cursor: pointer; color:blue; }\n";
		echo "</style>\n";

		echo "<script type='text/javascript'>\n";
		include("../d/a.js");
		echo "<script type='text/javascript'>\n";

		echo "playername='guest';\n";
		echo "hidelogin();\n";
		echo "setphpmode('prism.php');\n";
		echo "UIguestlogin('shades');\n";

		echo "</script>\n";
		echo "</body>\n";
		// trace("Done INIT");
	die("");
}

if( $player != ""){
  if( $player == "guest"){
	    if( $portal != ""){
    	   	passthru("/usr/bin/curl -u guest@$portal:guest --fail http://192.168.0.10:4202/~guest/?=",  $res);
   	    }else {
    	   	passthru("/usr/bin/curl -u guest:guest --fail http://192.168.0.10:4202/~guest/?=",  $res);
    	    }
			// trace("Procesed guest");
  }else if( $player== "search"){
       exec("/usr/bin/curl -u search:search --fail http://192.168.0.10:4202/~search/?=$cmd",  $output, $res);
       if( count($output) > 0){
        for($i = 0; $i < count($output); $i++){
          echo $output[$i]."<br />\n";
        }
       }
  }else {
    echo "// action=$action\n";
    echo "out_debug(\"lasthist=$lasthist\");\n";

    passthru("/usr/bin/curl -u $player:$pass --fail http://192.168.0.10:4202/~$player/?=$cmd",  $res);
  }
  die("");
}

?>

