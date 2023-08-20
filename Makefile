all: clean jjyRepeater

jjyRepeater: jjyRepeater.c
	gcc -O2 -o jjyRepeater jjyRepeater.c -lwiringPi -lpthread

clean:; rm -f jjyRepeater

install: jjyRepeater
	install -m 755 jjyRepeater /usr/local/bin
	install -m 644 jjyRepeater.service /etc/systemd/system
