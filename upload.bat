@REM avrdude -v -pattiny1604 -cjtag2updi -PCOM9 -Ufuse2:w:0x02:m -Ufuse6:w:0x07:m -Ufuse8:w:0x08:m -Uflash:w:build\cryptboot_attiny1604.hex:i
@REM avrdude -v -pattiny1604 -cjtag2updi -PCOM9 -Ufuse2:w:0x02:m -Ufuse6:w:0x07:m -Ufuse8:w:0x08:m -Uflash:w:build\cryptboot_x.hex:i
avrdude -v -pattiny1604 -cjtag2updi -PCOM9 -Ufuse2:w:0x02:m -Ufuse6:w:0x07:m -Ufuse8:w:0x08:m -Uflash:w:build\cryptboot_attiny160x.hex:i
pause
