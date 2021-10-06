avrdude -v -pattiny1604 -cjtag2updi -PCOM9 -Ufuse2:w:0x02:m -Ufuse6:w:0x04:m -Ufuse8:w:0x00:m -Uflash:w:build\cryptboot.hex:i
pause
