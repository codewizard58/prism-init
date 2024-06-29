// pete 2011-12-31 20:08:50 /prism/room/petetown0room0cl/broadway
// ver 1.0
	set_stylesheet('/skin.css');
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
                        
