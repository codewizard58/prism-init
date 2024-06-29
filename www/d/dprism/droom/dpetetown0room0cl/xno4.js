// /prism/room/petetown0room0cl/no4
// ver 1.1
	set_stylesheet('/skin.css');
	set_color("grey");
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
