// pete 2012-01-18 22:49:11 /prism/room/petetown0room0cl/station0road
// dprism/droom/xdefault.htm
// ver 1.0
	set_stylesheet('/skin.css');
	set_color("red");
	start_room( "$(script.self.name)", "$(script.self.path)" , "$(player.flags)");
	start_title();
	out_player("$(player.name)", "$(script.player.path)" , "$(player.pennies)", "$(player.chp)", "$(player.mhp)", "$(player.flags)" );
	end_title();
	start_history("");
        $(history)
	end_history("");
	start_contents();
	$(self.contents)
	end_contents();
	start_inventory();
	$(player.contents)
	end_inventory();
	show_nav(1);
	end_room( "");
