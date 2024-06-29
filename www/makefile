all:  d/a PHP /data/netdbm/www/d/a

content:
	tar cf - d | ( cd /data/netdbm/www/; tar xvf - )

d/a:	d/a.htm d/a.js d/aend.htm
	cat d/a.htm d/a.js d/aend.htm >/data/netdbm/www/d/a
	cp /data/netdbm/www/d/a d/a
PHP: /data/www/games/prism/php/forward.php /data/www/games/prism/php/prism.php

/data/www/games/prism/php/forward.php: php/forward.php
	cp php/forward.php /data/www/games/prism/php/forward.php

/data/www/games/prism/php/prism.php: php/prism.php
	cp php/prism.php /data/www/games/prism/php/prism.php

