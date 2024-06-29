
// MyAJAX.js ver 1.0

   function getXMLHTTPRequest(){
    try{
        req = new XMLHttpRequest();
    }catch(err) {
    try{
        req = ActiveXObject("Msxml2.XMLHTTP");
    }catch(err2){
    try {
        req = ActiveXObject("Microsoft.XMLHTTP");
    }catch(err3){
        req = false;
    }
    }
    }
        return req;
    }

    var http = getXMLHTTPRequest();
    var timer;
    var cnt = 0;

    function SendAjaxCmd(cmd)
    {   var myurl = "/~history";

    http.open("GET", myurl, true);
    http.onreadystatechange = useHttpResponse;
    http.send(null);
    }

    function useHttpResponse(){
    if( http.readyState == 4){
    if( http.status == 200){
    var resp= http.responseText;
    document.getElementById('history').innerHTML = "";
    cnt = cnt+1;
    eval( resp);
    show_text_list();

    document.getElementById('history').innerHTML += "<p>History "+cnt+"</p>";

    timer = setTimeout( "SendAjaxCmd('')", 5000);
    }
    }
    }

// End of AJAX functions.
    
function SendCmd(cmd)
{
    var form = window.document.commandform;
    form.cmd.value = cmd;
    form.submit();
}

function Name0(cmd)
{	var cnt = cmd.indexOf(";");
	if( cnt == -1){
		return cmd;
	}
	return cmd.substring(0,cnt);
}

// Start of SKIN..
var mytext = [];
var tcnt = 0;

function skin_show_text(obj)
{
    document.getElementById('history').innerHTML += obj.msg;
}

function skin_show_eol(obj)
{
    document.getElementById('history').innerHTML += "<span class='hbr'>"+obj.msg+"</span>";
}

function skin_show_drop(obj)
{
    document.getElementById('history').innerHTML += "<span class='hdrop' OnClick='SendCmd(\"drop "+obj.msg+"\")'>"+obj.msg+"</span>\n";
}

function skin_show_get(obj)
{
    document.getElementById('history').innerHTML += "<span class='hget' OnClick='SendCmd(\"get "+obj.msg+"\")'>"+obj.msg+"</span>\n";
}

function skin_show_exit(obj)
{
    document.getElementById('history').innerHTML += "<span class='hexit' OnClick='SendCmd(\""+obj.msg+"\")'>"+obj.msg+"</span>\n";
}

function skin_show_input(obj)
{
    document.getElementById('history').innerHTML += "<span class='hinput' OnClick='SendCmd(\""+obj.msg+"\")'>"+obj.msg+"</span>\n";
}

function skin_show_room(obj)
{
    document.getElementById('titlebar').innerHTML = "<span style='color:blue;' OnClick='SendCmd(\"look\")'>"+obj.msg+"</span>\n";
}

function skin_show_room_end(obj)
{
}

function skin_show_player(obj)
{
    document.getElementById('titlebar').innerHTML += "<span style='color:green;' OnClick='SendCmd(\"inventory\")'>"+obj.msg+"</span>\n";
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

function text_elem(msg, fclass)
{
this.msg = msg;
this.fclass = fclass;
}

text_elem.prototype.show = function()
{
	eval( this.fclass);
}

function show_text_list()
{	var obj;
	var i=0;

	for(i=0; i < tcnt; i++){
	obj = mytext[i];
	obj.show();
	}
	tcnt = 0;
}


function start_room(msg)
{
	mytext[tcnt++] = new text_elem(msg, "skin_show_room(this)");
}

function end_room(msg)
{
	mytext[tcnt++] = new text_elem(msg, "skin_show_text(this)");
}

function start_history(msg)
{
	mytext[tcnt++] = new text_elem(msg, "skin_show_text(this)");
}

function end_history(msg)
{
	mytext[tcnt++] = new text_elem(msg, "skin_show_text(this)");
}

function out_player(player, pennies, chp, mhp)
{	var obj = new text_elem(player, "skin_show_player(this)");
	mytext[tcnt++] = obj;
}

function out_startcmd(msg)
{
	mytext[tcnt++] = new text_elem(msg, "skin_show_text(this)");
}

function out_endcmd(msg)
{
	mytext[tcnt++] = new text_elem(msg, "skin_show_text(this)");
}

function out_input(msg)
{
	mytext[tcnt++] = new text_elem(msg, "skin_show_input(this)");
}

function out_normal(msg)
{
	mytext[tcnt++] = new text_elem(msg, "skin_show_text(this)");

}

function out_eol(msg)
{
	mytext[tcnt++] = new text_elem(msg, "skin_show_eol(this)");
}

function out_eolx(msg)
{
	mytext[tcnt++] = new text_elem(msg, "skin_show_eol(this)");
}

function out_obvious(msg)
{
	mytext[tcnt++] = new text_elem(msg, "skin_show_text(this)");
}

function out_padding(msg)
{
	mytext[tcnt++] = new text_elem(msg, "skin_show_text(this)");
}

function out_exit(msg)
{
	mytext[tcnt++] = new text_elem(msg, "skin_show_exit(this)");
}

function out_contents(msg)
{
	mytext[tcnt++] = new text_elem(msg, "skin_show_text(this)");
}

function out_dropobj(msg)
{
	mytext[tcnt++] = new text_elem(msg, "skin_show_drop(this)");
}

function out_getobj(msg)
{
	mytext[tcnt++] = new text_elem(msg, "skin_show_get(this)");
}

