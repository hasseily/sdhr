export PATH=~/cc65/bin:$PATH
cp sdhrmaster.hdv sdhr.hdv
cc65 -I ~/cc65/include/ -t apple2enh -Osir -Cl src/sdhr.c -o build/sdhr.s
ca65 -t apple2enh --include-dir ~/cc65/asminc build/sdhr.s -o build/sdhr.o
ca65 -t apple2enh --include-dir ~/cc65/asminc src/sp_dispatch.s -o build/sp_dispatch.o
ld65 -t apple2enh --cfg-path ~/cc65/cfg/ --lib-path ~/cc65/lib/ -o sdhr build/sdhr.o build/sp_dispatch.o --lib apple2enh.lib -m mapfile -vm
java -jar ~/AppleCommander-ac-1.8.0.jar -as sdhr.hdv SDHR BIN < sdhr
cp sdhr.hdv /mnt/c/Users/hasse_7x/OneDrive/Desktop/sdhr/
