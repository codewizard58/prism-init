// a.js
// ver 1.0 7/12/18
//
//

var playername="guest";
var playerpass="";
var userok = 0;
var prevuserok = 0;
var ticks=0;
var mode = 0;

var dt = null;	// debugtext
var mw = null;	// mainwindow

var curroom = null;
var prevroom = null;

var phpmode = false;
var phpmsg="";    // for debugging..
var histwidth = 800;
var histsize = 800;
var hasfocus = 0;
var curfontsize = "12";
var portal=null;
var displaymode = 1;	// 0 = split, 1 = floating
var m68="MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM";
var cmdtop = 0;		// command input is at bottom of screen.

///////////////////////////////////////////////////////////////

function getTextWidth(text, size, font) {
    // re-use canvas object for better performance
	var msg = ""+size+"px "+font;
    var canvas = getTextWidth.canvas || (getTextWidth.canvas = document.createElement("canvas"));
    var context = canvas.getContext("2d");
    context.font = msg;
    var metrics = context.measureText(text);
    return metrics.width;
};

function getfontsize( w, font)
{	var w12 = getTextWidth(m68+"MMMMMMMMMM", 12, "Courier");
	var x = (w-20) / w12;

	return Math.floor( x * 12);
}

////////////////////////////////////////////////////////////////
// link list of objects
//

function objlist()
{	this.head = null;

	this.addobj = function( ob, data)
	{	var o = new obj(this, ob, data);

		o.next = this.head;
		if( this.head != null){
			o.next.prev = o;
		}
		this.head = o;
//		alert("Addobj");

		return o;
	}

	this.removeobj = function( ob)
	{
		if( ob.list != this){
		error( "REMOVE list not this");
			return;
		}

		// remove from list;
		if( ob.prev != null){
			ob.prev.next = ob.next;
		}
		if( ob.next != null){
			ob.next.prev = ob.prev;
		}
		// see if this is the head one.
		if( ob == this.head){
			this.head = ob.next;
		}
	}

	this.reverse = function()
	{	var t,t2;

		if( this.head == null){
			return;
		}
		t = this.head;
		this.head = null;

		while( t != null){
			t2 = t.next;
			t.next = this.head;
			t.prev = null;
			this.head = t;
			if( t.next != null){
				t.next.prev = t;
			}
			t = t2;
		}
	}
}

function obj(list, ob, data)
{	this.next = null;
	this.prev = null;
	this.ob = ob;
	this.data = data;
	this.list = list;

}

///////////////////////////////////////////////////////////////

var histlist = new objlist();
var curhist = null;
var debugignorecnt=0;
var actionlist = new objlist();
var numhist = 15;
var lasthist=0;
var obviouslist = null;
var hidecompass=false;
var errormsg="Welcome";
var cmddrop = "drop";
var useobj = "";		// wielding etc.

///////////////////////////////////////////////////////////////
function clearhistory()
{
	histlist.head = null;
	lasthist = 0;
	curhist = null;
}


function histobj( num)
{	this.seq = parseInt( num);
	this.data= "";
}

function processcurhist()
{	var n, s;
	var l;

	if( curhist == null){
		return;
	}
	if( curhist.data == ""){
		return;
	}

	if( histlist.head == null){
		histlist.addobj(curhist, null);
		curhist = null;
		return;
	}

	n = curhist.seq;			// seq from server
	s = histlist.head.ob.seq;	// top seq number in history
	if( n > s ||
		( s > 990000 && n < 10000 )
	){	// simple wrap around.
		histlist.addobj( curhist, null);
	}else {
		debugignorecnt++;
		// error( "DEBUG HIST: "+debugignorecnt);
	}
	curhist = null;
}

///////////////////////////////////////////////////////////////
var actx = null;
var audiotype = 0;
var player = null;

function noAudio()
{
}

function checkaudiocontext()
{
	if( typeof( AudioContext) !== "undefined" ){
		actx = new AudioContext();
		audiotype = 1;
		return;
	}
	if( typeof( webkitAudioContext) !== "undefined" ){
		actx = new webkitAudioContext();
		audiotype = 2;
		return;
	}
	actx = new noAudio();
}

var sounds = [ 
	"pardon", "/sound/pardon" ,
	"welcome", "/sound/welcome" ,
	"welcomeguest", "/sound/welcomeguest" ,
	"RESET", "/sound/reset" ,
	null, "/sound/oops"
];

function sound(name)
{	this.name = name;
	this.buffer = null;
	
	this.geturl = function()
	{	var l = 0;
		var s = this.name;
		
		while( sounds[l] != null){
			if( sounds[l] == s){
				return sounds[l+1];
			}
			l = l + 2;
		}
		return sounds[l+1];
	}
}

function onError()
{
	error("failed to load sound");
}

function soundplayer()
{	this.soundlist = new objlist();
	this.volume = 0;
	this.sndindex = 0;
	this.loadingindex = 0;

	if( audiotype == 0){
		checkaudiocontext();
	}
	
	// load a sound and play with volume v
	this.loadsound = function(s )
	{	var request = new XMLHttpRequest();
		var snd = new sound(s);
		var url = snd.geturl();
		
		this.soundlist.addobj(snd, null);
		request._player = this;
		error("Load "+s);
		
		request.open('GET', url, true);
		request.responseType = 'arraybuffer';

	  // Decode asynchronously
	  request.onload = function() {
			actx.decodeAudioData(request.response, function(buffer) {
		    snd.buffer = buffer;
		    error("Loaded "+snd.name);
		    request._player.loadingindex += 2;
		    
		}, onError);
	  }
	  request.send();
	}
	
	this.timer = function()
	{
		if( sounds[ this.sndindex] != null && this.sndindex == this.loadingindex){
      this.sndindex += 2;
      this.loadsound( sounds[ this.sndindex-2]);
      }
      }

      this.playsound = function(s)
      {	var l = this.soundlist.head;

      while( l != null){
      if( l.ob.name == s){
      this.playbuffer( l.ob.buffer);
      return;
      }
      l = l.next;
      }
      return;
      }

      this.playbuffer = function(buf, vol)
      {	var src = actx.createBufferSource();
      src.buffer = buf;
      src.connect(actx.destination);

      src.onended = function(){ error("Sound ended"); };
      src.start(0);
      }
      }


      function playsound(sound)
      {
      if( player == null){
      return;
      }

      player.playsound(sound);

      }

      ///////////////////////////////////////////////////////////////

      function cmdobj( cmd)
      {	this.cmd = cmd;
      this.data = "";
      this.contents = new objlist();
      this.exits = new objlist();
      }

      function actionobj()
      {	this.seq = -1;
      this.name= "";

      }

      function doaction( action)
      {
      if( action == "RESET" ){
      // do a look command

		  docommand("look", 1);
		  playsound("RESET");
      }else if( action == "pardon"){
	      playsound("pardon");
      }
}

///////////////////////////////////////////////////////////////////////////////////////
/////////////
/////////////
///////////////////////////////////////////////////////////////////////////////////////

function checkscreen()
{	var h;
	var w, ew;
	var hw,hh;
	var rows;

	if( screenwidth > screenheight){
		// landscape
		hw =  screenwidth-20;		// characters are narrower than tall
		if(displaymode == 0){
			histwidth = Math.floor( 0.75 * hw);
		}else{
			histwidth = hw;
		}

		hh = screenheight-30;
	}else {
		// portrait
		hw = screenwidth-20;
		histwidth = Math.floor( 0.75 * hw);
		hh = Math.floor( (2 * screenheight) / 3);
	}

	// calculate font size
	ew = getfontsize( hw);

	rows = Math.floor( hh / ew);

	if( screenwidth > screenheight){
		numhist = rows-4;
	}else{
		numhist = rows - 6;
	}

	return ""+Math.floor(ew)+"px;font-family:Courier;";
}

function UIshowcompass()
{
	if( hidecompass){
		hidecompass=false;
	}else {
		hidecompass = true;
	}
	refreshroom();
}

function showcompass(fontsize)
{	var msg="";
	var mid = Math.floor(screenwidth/2);

	if( hidecompass){
		msg += '<div class="boxred" style="position:fixed;zindex=2;right:5px;top:15px;background-color:white;" >';
		msg += '<input type="button" value="|+|" onclick="UIshowcompass();" />';
		msg += '</div>\n';
	}else {
		msg += '<div class="boxred" style="position:fixed;zindex=2;max-width='+(mid-20)+'px;top:15px;left:'+(mid-40)+'px;background-color:white;" >';
			msg += "<table><tr><td valign='top'>\n";
			msg += '<div style="font-size:'+fontsize+';" id="title">\n';
			msg += "</div>\n";
			msg += '</td><td>';
			msg += '<input type="button" value="|^|" onclick="UIshowcompass();" /></td>\n';
			msg += '</tr><tr><td >\n';
			msg += '<div style="font-size:'+fontsize+';" id="commands">';
			msg += '</div>\n';
			msg += '</td></tr><tr><td >\n';
			msg += '<div style="font-size:'+fontsize+';" id="contents">';
			msg += '</div>\n';
			msg += '</td></tr><tr><td >\n';
			msg += '<div style="font-size:'+fontsize+';" id="inventory">';
			msg += '</div>\n';
			msg += "</td></tr></table>\n";
		msg += '</div>\n';
	}
	return msg;
}

function showinput()
{	var msg = "";

	msg += '<div style="position:fixed;zindex=1;top;left:5px;background-color:white;">\n';
	msg += '<span style="font-size:'+curfontsize+'" >Cmd: </span><input type="text" id="cmdin" onchange="UIdoinput();" onfocus="UIfocus();" size="20" style="font-size:'+curfontsize+'" />\n';
	msg += "</div>\n";

	return msg;
}


function setdisplayformat()
{	var msg="";
	var fontsize = checkscreen();
	var mid = Math.floor(screenwidth/2);

	curfontsize = fontsize;

	// error("FS="+fontsize);

	if( screenheight < screenwidth){
	// landscape
	  if( displaymode == 0){
		if( cmdtop == 1){
			msg += showinput();
		}
		msg += "<table><tr><td valign='top'>\n";
		msg += '<div style="font-size:'+fontsize+';width:'+(histwidth-20)+'px;" id="history">';
		msg += '</div>\n';
		msg += "</td><td valign='top' width='"+(screenwidth-histwidth-20)+"px'>\n";

			msg += "<table><tr><td valign='top'>\n";
			msg += '<div style="font-size:'+fontsize+';" id="title">\n';
			msg += "</div>\n";
			msg += '</td></tr><tr><td >\n';
			msg += '<div style="font-size:'+fontsize+';" id="commands">';
			msg += '</div>\n';
			msg += '</td></tr><tr><td >\n';
			msg += '<div style="font-size:'+fontsize+';" id="contents">';
			msg += '</div>\n';
			msg += '</td></tr><tr><td >\n';
			msg += '<div style="font-size:'+fontsize+';" id="inventory">';
			msg += '</div>\n';
			msg += "</td></tr></table>\n";
		msg += "</td></tr></table>\n";
		if( cmdtop == 0){
			msg += showinput();
		}
	  }else {
		if( cmdtop == 1){
			msg += showinput();
		}
		msg += '<div style="font-size:'+fontsize+';width:'+(histwidth-10)+'px;" id="history">';
		msg += '</div>\n';
		msg += showcompass(fontsize);
		if( cmdtop == 0){
			msg += showinput();
		}
	  }
	}else {
		if( cmdtop == 1){
			msg += showinput();
		}
		msg += '<div style="font-size:'+fontsize+';width:'+(histwidth-20)+'px;" >';
		msg += '&nbsp;<br />\n';
		msg += '</div>\n';

		msg += "<table><tr><td valign='top'>\n";
			msg += '<div style="font-size:'+fontsize+';" id="title">\n';
			msg += "</div>\n";
		msg += '</td></tr><tr><td >\n';
		msg += '<div style="font-size:'+fontsize+';" id="history">';
		msg += '</div>\n';
			msg += '</td></tr><tr><td >\n';
			msg += '<div style="font-size:'+fontsize+';" id="commands">';
			msg += '</div>\n';
		msg += "</td></tr><tr><td>\n";
		msg += '<div style="font-size:'+fontsize+';" id="contents">';
		msg += '</div>\n';
		msg += '</td></tr><tr><td >\n';
			msg += '<div style="font-size:'+fontsize+';" id="inventory">';
			msg += '</div>\n';
		msg += "</td></tr></table>\n";
		if( cmdtop == 0){
			msg += showinput();
		}
	}
	msg += '</div>\n';

	mw.innerHTML = msg;

}

function refreshroom()
{
	setdisplayformat();
	if( prevroom != null){
		prevroom.show();
	}else {
		docommand("look" , 0);
	}
}
      
function room()
{	this.name = "";
      this.curplayer = "";
      this.pennies=0;
      this.level=0;
      this.hp=0;
      this.state = 0;		// state of the parser..
      this.cmdlist = new objlist();
      this.prehist = new cmdobj("history");
      this.history = new cmdobj("history");
      this.inventory = new objlist();
      this.description = new cmdobj("description");
      this.fontsize = "12px";
	  this.contentmsg="";
	  this.contenttitle=null;

      //
      this.setroom = function( args)
      {	var objs;
	    var n;

		objs = args[1].split(",");

		n = objs[0].trim();
		this.name = n.substr(1, n.length-2);
      }

      this.eol = function()
      {
		  if( this.eolflag == 0){
			  if( this.state == 2){	// command seq
				  this.cmdlist.head.ob.data += "<br />\n";
			  }
				this.eolflag = 1;
				if( curhist != null){
					curhist.data += "<br />\n";
			  }
		  }
	  }

	this.examine = function(obj)
	{
		return "<span onclick='UIdoexamine("+obj+");' style='cursor:pointer;color:green;'>"+obj+"</span>";
	}

	this.owneradjust = function(msg)
	{	var i = msg.indexOf( "(#" );
		var s,msg1,obj,obj2,flags;

		if( i == -1){
			return msg;
		}
		msg1 = msg.substr(0, i);
		obj = msg.substr(i+2, msg.length-i-2);

		s = obj.indexOf(")");
		obj2= obj.substr(0, s);

		s = obj2.indexOf(" ");

		flags="";
		if( s != -1){
			flags= obj2.substr(s+1, obj2.length-s-1);
			obj2 = obj2.substr(0, s);
		}
		
		return msg1+"(#"+this.examine(obj2)+" "+msg.substr(i+2+s, msg.length-i-2-s);
	}

	//
	// 
	// 4 start history
	// 5 description
	// 6 obvious exits
	// 7 contents

	this.normal = function(data)
	{	
		if( this.state == 2){	// command seq
			if( this.cmdlist.head == null ){
				this.history.data += data;
			}else {
				this.cmdlist.head.ob.data += data;
			}
		}else if( this.state == 5){
			this.description.data = data;
		}else if( this.state == 4){
			this.prehist.data += data+"<br />\n";
		}else if( this.state != 6){
			this.history.data += data+"<br />\n";
		}
		if( curhist != null){
			curhist.data += data;
		}
		this.eolflag = 0;
	}


	// create a new cmdobj 
	//
	this.input = function(args)
	{	var cmd = args[1].trim();
		
		cmd = cmd.substr(1, cmd.length-2);

		this.cmdlist.addobj( new cmdobj( cmd), null);
		this.state = 0;			// just in case we were in state 4.

		if( curhist != null){
			curhist.data += outcmd( cmd)+"<br />\n";
		}

		this.history.exits.head = null;

		this.contenttitle=null;
		this.contentmsg="";
   }

   this.player = function(data)
      {	var args = data.split(",");
		  if( this.state == 1){		// title
			  this.curplayer = args[0].trim();
			  this.curplayer = this.curplayer.substr(1, this.curplayer.length-2);

			  this.pennies = args[2].trim();
			  this.pennies = this.pennies.substr(1, this.pennies.length-2);

			  this.hp = args[3].trim();
			  this.hp = this.hp.substr(1, this.hp.length-2);

			  this.mhp = args[4].trim();
			  this.mhp = this.mhp.substr(1, this.mhp.length-2);
		  }
      }

      // title part
      this.starttitle = function()
      {
		this.state = 1;		// in title
      }

      this.endtitle = function()
      {
		  this.state = 0;		// normal
      }

      // start command sequence
      this.startcmd = function()
      {
		  if( this.cmdlist.head != null){
			  this.state = 2;		// command obj
		  }
      }

      this.endcmd = function()
      {
		this.state = 0;		// normal
		if( this.contenttitle != null && this.contentmsg != ""){
			this.normal( this.contenttitle);
			this.normal( this.contentmsg+"<br />\n");
		}
      }

      // contents
	  // called by getobj
	  //
      this.addcontents = function( data)
      {	var av = data.split(",");
		  var cl = this.cmdlist.head;
		  var obj = av[0];

		  if( this.state == 2){
			  if( cl != null){
				  cl.ob.contents.addobj( av, null);
			  }
			}else if( this.state == 3){
				this.addinventory(data);
			}else if( this.state == 7){
				if( this.contentmsg != ""){
					this.contentmsg += ",";
				}
				this.contentmsg += " "+obj.substr(1, obj.length-2);
			}else if( this.state != 4){
				this.history.contents.addobj( av, null);
			}
      }

	  //
      this.outcontents = function(data)
      {	var obj = data.trim();

		  obj = obj.substr(1, obj.length-2);

		  if( obj != "Contents:"){
			this.contenttitle=obj;
			this.contentmsg="";
			this.state = 7;
		  }
		  

      }

      // start inventory sequence
      this.startinventory = function()
      {
		  this.state = 3;		// inventory
		  this.inventory.head = null;
      }

      this.endinventory = function()
      {
		this.state = 0;		// normal
      }

      // inventory
      this.addinventory = function( data)
      {	var av = data.split(",");

		this.inventory.addobj( av, null);
      }


      // start history sequence, skip till something interesting...
      // single history object used if no command sequence..

      this.starthistory = function()
      {
	      this.state = 4;		// history
      }

      this.endhistory= function()
      {
		  this.state = 0;		// normal
      }


      // description (optional)
      this.startdesc = function()
      {
      this.state = 5;		// description
      }

      this.enddesc = function()
      {
      this.state = 0;		// normal
      }

      // exits
	  // obvious exits
	  this.startobvious = function()
	  {	
		this.state = 6;
		this.history.exits.head = null;
	  }

      this.addexit = function( data)
      {	var av = data.split(",");
		  var cl = this.cmdlist.head;

		  if( this.state == 2){
			  if( cl != null){
				  cl.ob.exits.addobj( av, null);
			  }
			}else if( this.state == 3){
			}else if( this.state != 4){
				this.history.exits.addobj( av, null);
			}
      }

      this.outexit = function( data)
      {
		this.addexit( data );
	  }


      ///////////////////////////////////////////////
      this.findaction = function(act, seq)
      {	var al,nl;

      al = actionlist.head;

      while( al != null){
		  if( al.ob.name == act){
			  if( al.ob.seq < seq || 
						(al.ob.seq > 990000 && seq < 10000) 
				){	// newer action..
						al.ob.seq = seq;
						return al.ob;
				}
				return null;
			}
			al = al.next;
		}

		nl = new actionobj();
		nl.seq = seq;
		nl.name = act;
		actionlist.addobj(nl, null);
		return nl;
	}

	this.action = function( l)
	{	var seq = l.ob.seq;
		var data= l.ob.data;
		var al, nl;
		var act, act1;

		if( data.indexOf("docommand(") > 0){
			return null;
		}

		act = data.indexOf( "RESET" );
		act1= data.indexOf( "You hear someone shout ");
		if( act > 0 && act1 == 0){
            al = this.findaction( "RESET", seq);
            }else if( data.indexOf("Pardon!") == 0){
            al = this.findaction( "pardon", seq);
            }


            if( al != null){
            // alert("Action "+al.name+" "+al.seq);
            doaction( al.name);
        }
  }

//
    this.showtitle = function()
    { var msg = "";

//	msg += '<div id="messages" style="font-size:'+this.fontsize+';">';            
//      msg += errormsg;
//      msg+= "</div>\n";

      msg+= '<table style="font-size:'+this.fontsize+'" class="box">';
      msg+= "    <tr>";
      msg += '  <td colspan="2" align="left">';
      msg+= '    <span id="player">'+this.curplayer+'</span>';
      msg+= '    <span id="health">'+this.hp+"/"+this.mhp+'</span>';
      msg+= '    <span id="pennies">'+this.pennies+'</span>';
      msg += '  </td>';
//     msg += '  <td>';
//      msg += '    <input type="button" value="Display" onclick="UIdisplay();" />';
//      msg += '    <input type="button" value="Show Debug" onclick="UIshowdebug();" />';
//      msg += '  </td>';
      msg += '</tr>\n';
      msg += '<tr>\n';
        msg+= ' <td colspan="2" align="left" >';
        msg+= '   <span >'+this.name+'</span>';
      msg += '  </td>';
      msg += '</tr>\n';
    msg += '</table>\n';
    return msg;
    }

// 
	this.showhistory = function()
	{ var msg="";
		var l;
		var n, cnt;

		// output history keep at most 40
		//		msg += "<br />\n";
		l = histlist.head;
		n = 0;
		while( n < 40 && l != null){
			n++;
			l = l.next;
		}
		if( l != null){
			l.next = null;
		}
		cnt = n;		// number of entries..

		histlist.reverse();
		l = histlist.head;

		n = 0;
		if( cnt < numhist){
			// output the blank lines..
			while( n < numhist - cnt){
				n++;
				msg += "<br />\n";
			}
			l = histlist.head;
		}else {
			// skip the oldest ones..
			l = histlist.head;
			while( cnt > numhist && l.next != null){
				l = l.next;
				cnt--;
			}
		}

		// output the history..
		while( l != null ){

			this.action(l);

			msg += this.owneradjust( l.ob.data);

			l = l.next;
		}
		histlist.reverse();
//		msg += "<br />\n";
		return msg;
	}

	// showcontents
	this.showcontents = function()
	{ var msg = "";
		var l = null;
		var cl = this.cmdlist.head;
		var n, head;

		if( cl != null){
			// output contents
			if( this.history.contents.head != null){
				l = this.history.contents.head;
			}else {
				l = cl.ob.contents.head;
			}
			head = l;
			if( l != null){
				msg += "Contents: ";
			}
			while( l != null){
				if( l != head){
					msg += ", ";
				}
				msg += getcmd( l.ob);

				l = l.next;
			}
			if( head != null){
				msg += "<br />\n";
			}
		}
		return msg;
	}

	// showexits
	this.showexits = function()
	{ var msg = "";
		var l = null;
		var cl = this.cmdlist.head;
		var n, head;

		if( cl != null){
			// output exits
			if( this.history.exits.head != null){
				l = this.history.exits.head;
			}else {
				l = cl.ob.exits.head;
			}
			head = l;
			if( l != null){
				msg += "Obvious Exits: ";
			}
			while( l != null){
				if( l != head){
					msg += ", ";
				}
				msg += outexit( l.ob);

				l = l.next;
			}
			if( head != null){
				msg += "<br />\n";
			}
		}
		return msg;
	}

////////////////////////////////////////////////////////////
// showroom
//
// three screen areas 
// historywin
// contentwin
// titlewin
//
	this.show = function()
	{	var msg, msg1, msg2;
		var w;

		this.fontsize = curfontsize;
       
		w = document.getElementById("history");
		if( w != null){
			w.innerHTML = this.showhistory();
		}

		w = document.getElementById("title");
		if( w != null){
			w.innerHTML = this.showtitle();
		}

		w = document.getElementById("commands");
		if( w != null){
			w.innerHTML = this.showcommands();
			w.innerHTML += this.showexits();
		}

		w = document.getElementById("contents");
		if( w != null){
			w.innerHTML = this.showcontents();
		}

		w = document.getElementById("inventory");
		if( w != null){
			w.innerHTML = this.showinventory();
		}

	}

// showcommands
      this.showcommands = function()
      { var msg="";
      

      msg+= '<table style="font-size:'+this.fontsize+'" > ';
        msg+= '  <tr>          ';
          msg+= '    <td>            ';
            msg+= '       <span class="exit" onclick="docommand(\'nw\', 1);">NW</span>';
            msg += '          </td>';
          msg+= '    <td>            ';
            msg+= '      <span class="exit" onclick="docommand(\'north\', 1);">North</span>';
            msg+= '          </td>';
          msg+= '    <td>            ';
            msg+= '      <span class="exit" onclick="docommand(\'ne\', 1);">NE</span>';
            msg+= '          </td>';
          msg+= '    <td>            ';
            msg+= '       <span class="exit" onclick="docommand(\'in\', 1);">In</span>';
            msg+= '          </td>';
          msg+= '    <td>            ';
            msg+= '      <span class="exit" onclick="docommand(\'out\', 1);">Out</span>';
            msg+= '          </td>';
          msg +='        </tr>';
        msg+= '  <tr>          ';
          msg+= '    <td>            ';
            msg+= '      <span class="exit" onclick="docommand(\'west\', 1);">West</span>';
            msg+= '          </td>';
          msg+= '    <td>            ';
            msg+= '      <span class="exit" onclick="docommand(\'look\', 1);">Look</span>';
            msg+= '          </td>';
          msg+= '    <td>            ';
            msg+= '       <span class="exit" onclick="docommand(\'east\', 1);">East</span>';
            msg+= '          </td>';
          msg+= '    <td>            ';
            msg+= '      <span class="exit" onclick="docommand(\'up\', 1);">Up</span>';
            msg+= '          </td>';
          msg+= '    <td>            ';
            msg+= '      <span class="exit" onclick="docommand(\'down\', 1);">Down</span>';
            msg+= '          </td>';
          msg+= '        </tr>';
        msg+= '  <tr>          ';
          msg+= '    <td>            ';
            msg+= '      <span class="exit" onclick="docommand(\'sw\', 1);">SW</span>';
            msg+= '          </td>';
          msg+= '    <td>            ';
            msg+= '      <span class="exit" onclick="docommand(\'south\', 1);">South</span>';
            msg+= '          </td>';
          msg+= '    <td>            ';
            msg+= '      <span class="exit" onclick="docommand(\'se\', 1);">SE</span>';
            msg+= '          </td>';
          msg += '        </tr>';
        msg += '      </table>';
        return msg;
      }

// show inventory
    this.showinventory = function()
    {	var msg = "";
		  var l = null;
		  var cl = this.inventory.head;

		  if( cl != null){
			msg += "You are carrying:<br />\n";
		  }else{
			msg += "You are not carrying anything<br />\n";
		  }

		  while( cl != null){
			  if( cl != this.inventory.head){
				  msg += ", ";
			  }
			  msg += dropcmd( cl.ob);

			  cl = cl.next;
		  }

        return msg;
      }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////
///////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// getcmd
function getcmd( args)
      {	var obj = args[0].substr(1, args[0].length-2);
      var msg = "";
      var hint;

      msg += "<span style='color:green;cursor:pointer;' ";
	if( args.length == 4){
		hint = args[3].trim();
		hint = hint.substr(1, hint.length-2);
		msg += "onclick=\"docommand('"+hint+" "+obj+"', 5);\" >";
	}else {
		msg += "onclick=\"docommand('get "+obj+"', 5);\" >";
	}

	msg += obj;
	msg += "</span> \n";

	return msg;
}

function dropcmd( args)
{	var objs = args;
	var obj = objs[0].substr(1, objs[0].length-2);
	var msg = "";
	
	msg += "<span style='color:green;cursor:pointer;' ";
	msg += "onclick=\"dodropcommand('"+obj+"', 1);\" >";

	msg += obj;
	msg += "</span> \n";

	return msg;
}

function outcmd( obj)
{	var msg = "";

	msg += "<span style='color:blue;cursor:pointer;' onclick=\"docommand('"+obj+"', 1);\" >";

	msg += obj;
	msg += "</span> \n";

	return msg;
}


function outexit( args)
{	var obj = args[0].substr(1, args[0].length-2);
      var msg = "";
      var hint;
	  var s = obj.indexOf(";");

	  if( s != -1){
		obj = obj.substr(0, s);
	  }


      msg += "<span style='color:red;cursor:pointer;' ";
	if( args.length == 4){
		hint = args[3].trim();
		hint = hint.substr(1, hint.length-2);
		msg += "onclick=\"docommand('"+hint+" "+obj+"', 5);\" >";
	}else {
		msg += "onclick=\"docommand('"+obj+"', 5);\" >";
	}

	msg += obj;
	msg += "</span> \n";

	return msg;
}



function decodeargs( args, reqmode)
{	var data;
	var j;
	var cmd=args[0];
	var cr = curroom;

//		alert("cmd="+args[0]+":"+args[1]);

	if( cmd == "out_eol"){			// EOL
		curroom.eol();

	}else if( cmd == "out_eolx"){	// EOLX
		curroom.eol();

	}else if( cmd == "out_normal"){	// NORMAL
		if( args.length > 2){
			data = args[1];
			for(j = 2; j < args.length; j++){
				data += "("+args[j];
			}
		}else {
			data = args[1];
		}
		data = data.trim();
		data = data.substr(1, data.length-2);

		curroom.normal(data);

	}else if( cmd == "out_input"){

		curroom.input(args);

	}else if( cmd == "out_player"){
		if( args.length > 2){
			data = args[1];
			for(j = 2; j < args.length; j++){
				data += "("+args[j];
			}
		}else {
			data = args[1];
		}
		data = data.trim();

		curroom.player(data);

	}else if( cmd == "out_room"){
		if( args.length > 2){
			data = args[1];
			for(j = 2; j < args.length; j++){
				data += "("+args[j];
			}
		}else {
			data = args[1];
		}
		data = data.trim();

		curroom.normal(data);

	}else if( cmd == "start_title"){
		curroom.starttitle();
	}else if( cmd == "end_title"){
		curroom.endtitle();
	}else if( cmd == "start_room"){
		curroom = new room( );
		curroom.setroom(args);
	}else if( cmd == "end_room"){

		if( histlist.head== null || histlist.head.ob.seq != lasthist){
			// did something change?
			curroom.show();
			prevroom = curroom;
			curroom = null;

        if( histlist.head != null){
  			lasthist = histlist.head.ob.seq;
      }
		}

	}else if( cmd == "start_history"){	// need to skip to something...
		curroom.starthistory();
	}else if( cmd == "end_history"){
		curroom.endhistory();
		processcurhist();
	}else if( cmd == "set_stylesheet"){
	}else if( cmd == "out_startcmd"){	// command seq
		curroom.startcmd();
	}else if( cmd == "out_endcmd"){
		curroom.endcmd();
	}else if( cmd == "out_getobj"){		// contents
		if( args.length > 2){
			data = args[1];
			for(j = 2; j < args.length; j++){
				data += "("+args[j];
			}
		}else {
			data = args[1];
		}
		data = data.trim();

		curroom.addcontents(data);
	}else if( cmd == "out_dropobj"){		// Inventory
		if( args.length > 2){
			data = args[1];
			for(j = 2; j < args.length; j++){
				data += "("+args[j];
			}
		}else {
			data = args[1];
		}
		data = data.trim();

		curroom.addinventory(data);

	}else if( cmd == "start_contents"){
	}else if( cmd == "end_contents"){
	}else if( cmd == "out_contents"){
		curroom.outcontents( args[1]);
	}else if( cmd == "out_obvious"){
		curroom.startobvious();
	}else if( cmd == "out_exit"){
		if( args.length > 2){
			data = args[1];
			for(j = 2; j < args.length; j++){
				data += "("+args[j];
			}
		}else {
			data = args[1];
		}
		data = data.trim();

		curroom.outexit(data);

	}else if( cmd == "out_padding"){
	}else if( cmd == "start_inventory"){
		curroom.startinventory();
	}else if( cmd == "end_inventory"){
		curroom.endinventory();
	}else if( cmd == "show_nav"){
	}else if( cmd == "out_description"){
		// curroom.normal(args[1]);
	}else if( cmd == "start_description"){
		curroom.startdesc();
	}else if( cmd == "end_description"){
		curroom.enddesc();
	}else if( cmd == "set_color"){
	}else if( cmd == "history_seq"){
		data = args[1].trim();
		data = data.substr(1, data.length-2);
		processcurhist();
		curhist = new histobj( data);
	}else if( cmd == "out_debug"){
		// error("DEBUG: "+args[1]);
	}else if( cmd == "edit_room"){
		alert("edit_room"+args[1]);
	}else if( cmd == "edit_thing"){
		alert("edit_thing"+args[1]);
	}else if( cmd == "edit_player"){
		alert("edit_player"+args[1]);
	}else if( cmd == "edit_exit"){
		alert("edit_exit"+args[1]);
	}else {
		if( "Content-Type" == cmd.substr(0, 12) ){
			userok = 0;
			alert("Logged out");
		}else {
			error("UNKNOWN: "+cmd+":"+args[1]);
		}
	}

}

function queuedata(d, reqmode)
{	var lines = d.split(/\r\n|\r|\n/g);
	var n, i, l, ch, j;
	var args, cmd;
	var data = "";

	if( reqmode == 0){
		cmd="";
	}

	for(n=0; n < lines.length; n++){
		i = 0;
		l = lines[n];
		ch = ' ';
		while( ch == ' ' || ch == '\t'){
			ch = l.charAt(i);
			i = i+1;
		}
		if( i >= l.length){
			continue;
		}
		
		if( ch == '/' && l.charAt(i) == '/'){
		// ignore;
		continue;
		}
		cmd = l.substr(i-1, l.length-i-1);
		args = cmd.split("(");

		decodeargs( args, reqmode);
    }
	if( dt !=null){
	    dt.value = d;
	}
}

function error(msg)
{	var m = document.getElementById("messages");

		if( m == null){
			return;
		}
		m.innerHTML=msg;
		errormsg = msg;
}

// do login
function dologin()
{	var x = new XMLHttpRequest();

	

	x.onreadystatechange = function()
	{
		if( this.readyState == 4){
			var d = this.responseText;

			// check return code
			if( this.status == 401){
				userok = 0;
				error("Login failed");
				return;
			}

			if( playername == "guest"){
				playername=d;
				portal="dungeon";
				playerpass=d;
				userok = 1;
				error("Welcome "+d);
				player.playsound("welcomeguest");
				return;
			}

			if( d == ""){
				userok = 0;
				alert("Logged out, Timeout or bad user/pass");
				return;
			}

			queuedata(d, 1);
			userok = 2;
			error("Welcome "+playername);
			player.playsound("welcome");
		}
	}

    if( phpmode){
      if( playername == "guest"){
		if( portal != null){
	        x.open('GET', "?player="+playername+"&portal="+portal, true);
		}else {
	        x.open('GET', "?player="+playername, true);
		}
      }else {
        x.open('GET', "?player="+playername+"&password="+playerpass, true);
      }
      }else {

      x.open('GET', "~"+playername, true);
      if( playername != "guest"){
	      x.setRequestHeader("Authorization", "Basic " + btoa(playername+":"+playerpass));
	  }else {
		if( portal != null){
	      x.setRequestHeader("Authorization", "Basic " + btoa(playername+"@"+portal+":"+playerpass));
		  error( "Portal "+playername+"@"+portal);
		}else {
	      x.setRequestHeader("Authorization", "Basic " + btoa(playername+":"+playerpass));
		}
      }
    }
    x.send(null);
}

function dodropcommand(cmd, mode)
{
    docommand(cmddrop+" "+cmd, mode);
}

function docommand(cmd, mode)
{	var x = new XMLHttpRequest();
    var reqstr="";

    x._mode = mode;

    x.onreadystatechange = function()
    {
		if( this.readyState == 4){
			var d = this.responseText;

			// check return code
			if( this.status == 401){
				userok = 0;
				// alert("Login failed");
				return;
			}

			if( this.status != 200){
				userok=0;
				alert("Got resonse code "+this.status);
				return;
			}

			if( d == ""){
				userok = 0;
				//alert("No data");
				error("Logged off");
				return;
			}

			ticks = 0;
			queuedata(d, this._mode);
		}
	}

    if( phpmode){
		reqstr="?player="+playername+"&password="+playerpass+"&lasthist="+lasthist+"&cmd="+encodeURIComponent(cmd);

		x.open('GET', reqstr, true);
	}else {
		reqstr="~"+playername+"/?="+encodeURIComponent(cmd);
		x.open('GET', reqstr, true);
		x.setRequestHeader("Authorization", "Basic " + btoa(playername+":"+playerpass));
	}
	x.send(null);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
////////////////
//////////////// User Interface functions
//////////////////////////////////////////////////////////////////////////////////////////////////

function UIlogin()
{	var pn = document.getElementById("playername");
	var pp = document.getElementById("playerpass");

	clearhistory();

	playername=pn.value.trim();
	playerpass = pp.value.trim();

	if( playername != "" && playerpass != ""){

		dologin();
	}else {
		player.playsound("pardon");
	}

}

function UIguestlogin(port)
{
	clearhistory();

    playername="guest";
	if( port != null){
		portal = port;
	}

    dologin();
}

function UIfocus()
{
	hasfocus = 100;
	error("Has focus");
}

function UIdoinput()
{	var di = document.getElementById("cmdin");
	var cmd;
	var cmd2;


	if( di == null){
		return;
	}
	cmd = di.value.trim();
	di.value="";

	if( cmd.length > 3 && cmd[0] == "@" ){
		cmd2 = cmd.substr(1, 3).toLowerCase();
		if( cmd2 == "fla"){
			window.open("docs/flags.htm");
			return;
		}else if( cmd2 == "lev"){
			window.open("docs/levels.htm");
			return;
		}else if( cmd2 == "com"){
			window.open("docs/cmds.htm");
			return;
		}
	}
	if( cmd.length > 5 && cmd[0] == "@"  ){
		cmd2 = cmd.substr(1, 5).toLowerCase();
		if( cmd2 == "locks"){
			window.open("docs/locks.htm");
			return;
		}
		if( cmd2 == "showd"){
			UIshowdebug();
			return;
		}
	}
	if( userok != 0){
		if( cmd != ""){
			hasfocus = 0;
			error("Command");
		}
		docommand( cmd, 1);
	}
}

function UIdisplay()
{
	numhist -= 5;
	if( numhist < 10){
		numhist = 20;
		lasthist = -1;
	}
	error("Screen width="+screenwidth+" height="+screenheight+" php="+phpmsg);
	docommand("look", 0);
}

function UIdoexamine(obj)
{
	docommand("@ex #"+obj, 1);
}

var debughidden=true;

function UIshowdebug()
{	var x;

	x = document.getElementById("debugtext");
	if( debughidden){
		x.style.display = "block";
		debughidden = false;
	}else {
		x.style.display = "none";
		debughidden = true;
	}

}

function showlogin()
{	var x = document.getElementById("logindiv");

	x.style.display="block";

	x = document.getElementById("gamediv");
	x.style.display="none";
	
	x = document.getElementById("debugdiv");
	x.style.display = "none";

	histlist.head = null;		// remove previous history info...
}

function hidelogin()
{	var x = document.getElementById("logindiv");

	x.style.display="none";

	x = document.getElementById("gamediv");
	x.style.display = "block";

	x = document.getElementById("debugdiv");
	x.style.display = "block";
}

var screenwidth=0;
var screenheight=0;
var lastchange = 0;

function dotimer()
{	var ld;

	if( hasfocus > 0){
		hasfocus--;
		return;
	}

	if( lastchange > 0){
		lastchange--;
	}

	if( (window.innerWidth != screenwidth ||
		window.innerHeight != screenheight ) && lastchange == 0){

		screenheight = window.innerHeight;
		screenwidth = window.innerWidth;
		// lastchange = 50;	// delay sequence of resizes.

		// error("Screen width="+screenwidth+" height="+screenheight);

		setdisplayformat();
		if( userok != 0){
			refreshroom();
			return;
		}else {
			ld = document.getElementById("logindiv");
			if( ld != null){
				if( screenwidth > 600){
						ld.style.width="600px";
				}else {
						ld.style.width="auto";
				} 
			}
		}
	}

	if( prevuserok != userok){
		if( userok == 0){
			showlogin();
		}else {
			if( prevuserok == 0){
				hidelogin();
			}
			if( userok == 1){
				userok = 2;
			}
			docommand("look", 1);	// first look...
		}
		prevuserok = userok;
	}

	if( userok != 0){
		ticks++;

		// error("Tick "+ticks);
		if( ticks == 5){
		
			docommand("", 0);	// poll for new history
		}
	}
	
	if( player != null){
		player.timer();
	}
}

function doload()
{
	mw = document.getElementById("gamediv");
	dt = document.getElementById("debugtext");
	
	player = new soundplayer();
	
	setInterval( dotimer, 100);

	setdisplayformat();
}


////////////////////////////////////////////////////////////////////////////////////////////////
////
// Pseudo js calls.
////////////////////////////////////////////////////////////////////////////////////////////////

function setphpmode(msg)
{
//  alert("Set PHP mode");
  phpmode = true;
  phpmsg = msg;
}

function out_debug(msg)
{
}

function set_stylesheet(msg)
{
}

function set_color(msg)
{
}

function 	start_room(arg0, arg1, arg2)
{
	queuedata("start_room('"+arg0+"','"+arg1+"','"+arg2+"');\n", 1);
}

function 	start_title()
{
	queuedata("start_title();\n", 1);
}


function 	out_player(arg0, arg1, arg2, arg3, arg4, arg5)
{
	queuedata("out_player('"+arg0+"','"+arg1+"','"+arg2+"','"+arg3+"','"+arg4+"','"+arg5+"');\n", 1);
}


function 	end_title()
{
	queuedata("end_title();\n", 1);
}


function 	start_history(arg0)
{
	queuedata("start_history('"+arg0+"');\n", 1);
}


function   history_seq( arg0)
{
	queuedata("history_seq('"+arg0+"');\n", 1);
}

function out_normal( arg0)
{
	queuedata("out_normal('"+arg0+"');\n", 1);
}


function out_eol( arg0)
{
	queuedata("out_eol('"+arg0+"');\n", 1);
}


function  out_endcmd( arg0)
{
	queuedata("out_endcmd('"+arg0+"');\n", 1);
}


function out_input( arg0)
{
	queuedata("out_input('"+arg0+"');\n", 1);
}


function  out_startcmd( arg0)
{
	queuedata("out_startcmd('"+arg0+"');\n", 1);
}

function end_history(arg0)
{
	queuedata("end_history('"+arg0+"');\n", 1);
}

function start_contents()
{
	queuedata("start_contents();\n", 1);
}
function end_contents()
{
	queuedata("end_contents();\n", 1);
}

function start_inventory()
{
	queuedata("start_inventory();\n", 1);
}

function end_inventory()
{
	queuedata("end_inventory();\n", 1);
}
function show_nav(arg0)
{
	queuedata("show_nav('"+arg0+"');\n", 1);
}

function end_room(arg0)
{
	queuedata("end_room('"+arg0+"');\n");
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//
</script> 
