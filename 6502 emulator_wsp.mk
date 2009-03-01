.PHONY: clean All

All:
	@echo ----------Building project:[ 6502 - Debug ]----------
	@cd "6502" && "mingw32-make.exe"  -j 1 -f "6502.mk" 
clean:
	@echo ----------Cleaning project:[ 6502 - Debug ]----------
	@cd "6502" && "mingw32-make.exe"  -j 1 -f "6502.mk" clean
