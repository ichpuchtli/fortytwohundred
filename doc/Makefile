
DOCUMENTED_SOURCES=$(wildcard srtp/*.h)

all : doc

doc : ${DOCUMENTED_SOURCES} Doxyfile
	doxygen Doxyfile

clean :
	rm -rf html
 
view:
	xdg-open html/index.html
