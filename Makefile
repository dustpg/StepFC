.PHONY: clean All

All:
	@echo "----------Building project:[ stepd - Debug ]----------"
	@cd "stepd" && "$(MAKE)" -f  "stepd.mk"
clean:
	@echo "----------Cleaning project:[ stepd - Debug ]----------"
	@cd "stepd" && "$(MAKE)" -f  "stepd.mk" clean
