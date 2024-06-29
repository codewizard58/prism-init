#
# Whatever you put in for $(CC) must be able to grok ANSI C.
#
#CC=gcc
#OPTIM= -g 
.c.o: 
	$(CC) -c $(CFLAGS) $*.c
CC=cc
OPTIM=

#LIBS= -lcurses -ldnet
LIBS= -lcurses 
# 
# To log failed commands (HUH's) to stderr, include -DLOG_FAILED_COMMANDS
# To restricted object-creating commands to users with the BUILDER bit,
#   include -DRESTRICTED_BUILDING
# To log all commands, include -DLOG_COMMANDS
# To force fork_and_dump() to use vfork() instead of fork(), include 
#   -DUSE_VFORK.
#
# include -DSTATUE to have statues.
#
DEFS= -DLOG_FAILED_COMMANDS  -DRESTRICTED_BUILDING -DOUTPUT_WRAP \
	-DNO_PROTOS -DNO_CONST -DEXTRACT -DAUTO_TOAD -DTCPIP -DNO232COMS  \
	-DHPUX
CFLAGS= $(OPTIM) $(DEFS) -I/usr/include -I../prism

# Everything except interfac.c --- allows for multiple interfacs
CFILES= create.c game.c help.c look.c match.c move.c player.c predicat.c \
	set.c speech.c stringut.c utils.c wiz.c db.c game.c \
	boolexp.c unparse.c text.c socket.c fight.c robot.c process.c notify.c \
	zone.c parsbool.c objstore.c extract.c misc.c connect.c ver.c http.c

# .o versions of above
OFILES= create.o game.o help.o look.o \
	match.o move.o player.o predicat.o \
	set.o speech.o stringut.o \
	utils.o wiz.o db.o boolexp.o \
	unparse.o text.o socket.o fight.o robot.o process.o notify.o zone.o \
	parsbool.o objstore.o extract.o misc.o connect.o ver.o media.o http.o

# Files in the standard distribution
DISTFILES= $(CFILES) config.h db.h externs.h interfac.h match.h \
	interfac.c \
	help.txt small.db minimal.db restart-cmu README small.db.README \
	CHANGES Makefile copyrite.h

OUTFILES= netmud dump paths TAGS

all: dos2unix ver netmud

ver: ver.c version
	./version UNIX
	cc -c ver.c
ver.c:  ../prism/ver.c version.h
	cp ../prism/ver.c .
	./dos2unix ver.c
version: version.c
	cc -o version version.c
version.c: ../prism/version.c
	cp ../prism/version.c .
	./dos2unix version.c
version.h: ../prism/version.h
	cp ../prism/version.h .
	./dos2unix version.h
install: netmud 
	cp netmud /usr/spool/netdbm/netdbm.new
	chown netdbm /usr/spool/netdbm/netdbm.new

TAGS: *.c *.h
	ctags *.c *.h

netmud: interfac.o $(OFILES)
	-mv -f netmud netmud~
	$(CC) $(CFLAGS) -o netmud interfac.o $(OFILES) $(LIBS)
./dos2unix: dos2unix.c
	cc -o ./dos2unix dos2unix.c

pretty: pretty.o ${OFILES}
	-rm -f pretty
	$(CC) $(CFLAGS) -o pretty pretty.o ${OFILES} $(LIBS)

clean:
	-rm -f *.o a.out core gmon.out $(OUTFILES)

dist.tar.Z: $(DISTFILES)
	tar cvf - $(DISTFILES) | compress -c > dist.tar.Z.NEW
	mv dist.tar.Z.NEW dist.tar.Z

makefile: ../unix/makefile
	cp ../unix/makefile .
	make

# DO NOT REMOVE THIS LINE OR CHANGE ANYTHING AFTER IT #
boolexp.o: boolexp.c copyrite.h db.h match.h externs.h config.h interfac.h boolexp.h
connect.o: connect.c copyrite.h db.h config.h interfac.h externs.h fight.h match.h bsx.h net.h
create.o: create.c copyrite.h db.h config.h interfac.h externs.h fight.h match.h bsx.h
db.o: db.c copyrite.h db.h config.h boolexp.h
dump.o: dump.c copyrite.h db.h
fight.o: fight.c db.h externs.h
fix.o: fix.c copyrite.h db.h config.h
foo.o: foo.c
game.o: game.c copyrite.h db.h config.h interfac.h match.h externs.h
help.o: help.c copyrite.h db.h config.h interfac.h externs.h
http.o: http.c descript.h tcpcoms.h misc.h os.h net.h mud_set.h
interfac.o: interfac.c copyrite.h db.h interfac.h config.h externs.h bsx.h net.h mud_set.h
kludge.o: kludge.c config.h
look.o: look.c copyrite.h db.h config.h interfac.h match.h externs.h fight.h
match.o: match.c copyrite.h db.h config.h match.h externs.h boolexp.h
media.o: media.c db.h
misc.o: misc.c  mud_set.h
move.o: move.c copyrite.h db.h config.h interfac.h match.h externs.h fight.h
notify.o: notify.c db.h interfac.h descript.h externs.h
old.o: old.c copyrite.h
parsbool.o: parsbool.c db.h boolexp.h match.h
paths.o: paths.c copyrite.h db.h config.h
player.o: player.c copyrite.h db.h config.h interfac.h externs.h fight.h
predicat.o: predicat.c copyrite.h db.h interfac.h config.h externs.h
pretty.o: ../prism/pretty.c db.h
	cp ../prism/$*.c .
	./dos2unix $*.c
	$(CC) -c $(CFLAGS) $*.c
process.o: process.c db.h interfac.h externs.h
robot.o: robot.c db.h fight.h
set.o: set.c copyrite.h db.h config.h match.h interfac.h externs.h
socket.o: socket.c descript.h tcpcoms.h misc.h os.h net.h mud_set.h
speech.o: speech.c copyrite.h db.h interfac.h match.h config.h externs.h
stringut.o: stringut.c copyrite.h externs.h
text.o: text.c interfac.h db.h externs.h
unparse.o: unparse.c db.h externs.h config.h interfac.h boolexp.h
utils.o: utils.c copyrite.h db.h externs.h
wiz.o: wiz.c copyrite.h db.h interfac.h match.h externs.h
config.h: copyrite.h
copyrite.h:
db.h: copyrite.h
externs.h: copyrite.h db.h
interfac.h: copyrite.h db.h
match.h: copyrite.h db.h
pp.o: pp.c db.h
zone.o: zone.c copyrite.h db.h interfac.h match.h externs.h
# get header files.
boolexp.h: ../prism/boolexp.h
	cp ../prism/boolexp.h .
	./dos2unix boolexp.h
bsx.h:
	echo '/* */' >bsx.h

config.h: ../prism/config.h
	cp ../prism/config.h .
	./dos2unix config.h
db.h: ../prism/db.h proto.h
	cp ../prism/db.h .
	./dos2unix db.h
externs.h: ../prism/externs.h
	cp ../prism/externs.h .
	./dos2unix externs.h
interfac.h: ../prism/interfac.h os.h descript.h copyrite.h
	cp ../prism/interfac.h .
	./dos2unix interfac.h
match.h: ../prism/match.h
	cp ../prism/match.h .
	./dos2unix match.h
misc.h: ../prism/misc.h
	cp ../prism/misc.h .
	./dos2unix misc.h
copyrite.h: ../prism/copyrite.h
	cp ../prism/copyrite.h .
	./dos2unix copyrite.h
descript.h: ../prism/descript.h
	cp ../prism/descript.h .
	./dos2unix descript.h
proto.h: ../prism/proto.h
	cp ../prism/proto.h .
	./dos2unix proto.h
os.h: ../prism/os.h
	cp ../prism/os.h .
	./dos2unix os.h
fight.h: ../prism/fight.h
	cp ../prism/fight.h .
	./dos2unix fight.h
tcpcoms.h: ../prism/tcpcoms.h
	cp ../prism/tcpcoms.h .
	./dos2unix tcpcoms.h
net.h: ../prism/net.h
	cp ../prism/net.h .
	./dos2unix net.h
mud_set.h: ../prism/mud_set.h
	cp ../prism/mud_set.h .
	./dos2unix mud_set.h
#
boolexp.c: ../prism/boolexp.c
	cp ../prism/boolexp.c .
	./dos2unix boolexp.c
connect.c: ../prism/connect.c
	cp ../prism/connect.c .
	./dos2unix connect.c
create.c: ../prism/create.c
	cp ../prism/create.c .
	./dos2unix create.c
db.c: ../prism/db.c
	cp ../prism/db.c .
	./dos2unix db.c
dump.c: ../prism/dump.c
	cp ../prism/dump.c .
	./dos2unix dump.c
extract.c: ../prism/extract.c
	cp ../prism/extract.c .
	./dos2unix extract.c
fight.c: ../prism/fight.c
	cp ../prism/fight.c .
	./dos2unix fight.c
fix.c: ../prism/fix.c
	cp ../prism/fix.c .
	./dos2unix fix.c
foo.c: ../prism/foo.c
	cp ../prism/foo.c .
	./dos2unix foo.c
game.c: ../prism/game.c
	cp ../prism/game.c .
	./dos2unix game.c
help.c: ../prism/help.c
	cp ../prism/help.c .
	./dos2unix help.c
http.c: ../prism/http.c
	cp ../prism/http.c .
	./dos2unix http.c
interfac.c: ../prism/interfac.c
	cp ../prism/interfac.c .
	./dos2unix interfac.c
kludge.c: ../prism/kludge.c
	cp ../prism/kludge.c .
	./dos2unix kludge.c
look.c: ../prism/look.c
	cp ../prism/look.c .
	./dos2unix look.c
match.c: ../prism/match.c
	cp ../prism/match.c .
	./dos2unix match.c
media.c: ../prism/media.c
	cp ../prism/media.c .
	./dos2unix media.c
misc.c: ../prism/misc.c
	cp ../prism/misc.c .
	./dos2unix misc.c
move.c: ../prism/move.c
	cp ../prism/move.c .
	./dos2unix move.c
notify.c: ../prism/notify.c
	cp ../prism/notify.c .
	./dos2unix notify.c
objstore.c: ../prism/objstore.c
	cp ../prism/objstore.c .
	./dos2unix objstore.c
old.c: ../prism/old.c
	cp ../prism/old.c .
	./dos2unix old.c
parsbool.c: ../prism/parsbool.c
	cp ../prism/parsbool.c .
	./dos2unix parsbool.c
paths.c: ../prism/paths.c
	cp ../prism/paths.c .
	./dos2unix paths.c
player.c: ../prism/player.c
	cp ../prism/player.c .
	./dos2unix player.c
predicat.c: ../prism/predicat.c
	cp ../prism/predicat.c .
	./dos2unix predicat.c
process.c: ../prism/process.c
	cp ../prism/process.c .
	./dos2unix process.c
robot.c: ../prism/robot.c
	cp ../prism/robot.c .
	./dos2unix robot.c
set.c: ../prism/set.c
	cp ../prism/set.c .
	./dos2unix set.c
socket.c: ../prism/socket.c
	cp ../prism/socket.c .
	./dos2unix socket.c
speech.c: ../prism/speech.c
	cp ../prism/speech.c .
	./dos2unix speech.c
stringut.c: ../prism/stringut.c
	cp ../prism/stringut.c .
	./dos2unix stringut.c
text.c: ../prism/text.c
	cp ../prism/text.c .
	./dos2unix text.c
unparse.c: ../prism/unparse.c
	cp ../prism/unparse.c .
	./dos2unix unparse.c
utils.c: ../prism/utils.c
	cp ../prism/utils.c .
	./dos2unix utils.c
wiz.c: ../prism/wiz.c
	cp ../prism/wiz.c .
	./dos2unix wiz.c
zone.c: ../prism/zone.c
	cp ../prism/zone.c .
	./dos2unix zone.c
