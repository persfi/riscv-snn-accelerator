PYTHON := .venv/bin/python
PIP    := .venv/bin/pip

freeze:
	{ echo "--extra-index-url https://download.pytorch.org/whl/cpu"; $(PIP) freeze; } > requirements.txt

clean:
	rm -rf verif/build

.PHONY: freeze clean